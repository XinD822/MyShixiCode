/**
 * @file usb_task.c
 * @brief USB 拖拽升级任务（模块内部）
 *
 * 工作流程：
 *   1. 上电 → 挂载 FatFs → USB MSC 连接（PC 看到U盘）
 *   2. USB 连接时卸载 FatFs，避免和 MSC 同时写 Flash 冲突
 *   3. USB 保持连接，每 10 秒无 SCSI 活动时后台检测 bin 文件
 *      — 无 bin 文件：卸载 FatFs，USB 保持连接（不断开重连）
 *      — 有 bin 文件：断开 USB → 搬运固件到 Slot A → 重启
 *   4. USB 物理断开时：挂载 FatFs → 检测 bin 文件 → 重连 USB
 */

#include "config.h"
#include "usb_task.h"
#include "usb_msc_device.h"

extern volatile uint32_t g_last_scsi_tick;

static uint8_t usb_active = 0;
static uint8_t fatfs_mounted = 0;
static uint8_t prev_usb_configured = 0;

static void Safe_FatFs_Mount(void)
{
    if (!fatfs_mounted) {
        if (FatFs_Mount() == FR_OK) {
            fatfs_mounted = 1;
        }
    }
}

static void Safe_FatFs_Unmount(void)
{
    if (fatfs_mounted) {
        FatFs_Unmount();
        fatfs_mounted = 0;
    }
}

/**
 * @brief 执行升级流程（检测 + 搬运 + 设置标志 + 重启）
 * @return 1=已触发重启（不会返回），0=无固件或升级失败
 */
static uint8_t Do_Upgrade(void)
{
    if (!UpgradeSource_Get()) {
        DBG_PRINTF("[UPG] no source registered\r\n");
        return 0;
    }
    if (!UpgradeSource_Get()->detect()) {
        DBG_PRINTF("[UPG] no firmware file\r\n");
        return 0;
    }

    DBG_PRINTF("[UPG] firmware detected, checking...\r\n");

    UpgradeResult chk = Upgrade_Check();
    DBG_PRINTF("[UPG] check result=%d\r\n", (int)chk);
    if (chk != UPGRADE_OK) {
        return 0;
    }

    DBG_PRINTF("[UPG] executing upgrade...\r\n");
    UpgradeResult exec = Upgrade_Execute();
    DBG_PRINTF("[UPG] execute result=%d\r\n", (int)exec);
    if (exec != UPGRADE_OK) {
        return 0;
    }

    /* 升级成功，删除固件文件防止重复升级 */
    const UpgradeSource_t *src = UpgradeSource_Get();
    DBG_PRINTF("[UPG] cleanup start\r\n");
    if (src && src->cleanup) {
        src->cleanup();
    }
    FlashService_FlushCache();
    DBG_PRINTF("[UPG] after cleanup detect=%d\r\n",
               src ? src->detect() : 0);

    /* 标记本次升级已完成 */
    uint32_t done = UPGRADE_STATE_DONE;
    FlashService_Write((uint8_t *)&done, UPGRADE_STATE_ADDR, sizeof(done));
    FlashService_FlushCache();
    DBG_PRINTF("[UPG] state -> DONE, reset\r\n");

    Tick_DelayMs(500);
    Platform_SystemReset();
    return 1;  /* 不会执行到这里 */
}

void USB_Task_Init(void)
{
    DBG_PRINTF("[TASK] FatFs_Mount...\r\n");
    Safe_FatFs_Mount();
    DBG_PRINTF("[TASK] FatFs_Mount OK\r\n");

    DBG_PRINTF("[TASK] Register source...\r\n");
    UpgradeSource_Register(&Source_USB_Drag);
    DBG_PRINTF("[TASK] Register OK\r\n");

    DBG_PRINTF("[TASK] usb_msc_device_init...\r\n");
    usb_msc_device_init();
    DBG_PRINTF("[TASK] usb_msc_device_init OK\r\n");

    DBG_PRINTF("[TASK] Upgrade_Init...\r\n");
    Upgrade_Init();
    DBG_PRINTF("[TASK] Upgrade_Init OK\r\n");
}

void USB_Task_Run(void)
{
    uint8_t usb_configured = usb_msc_is_configured();

    /* ════ USB 连接 ════ */
    if (!usb_active && usb_configured) {
        DBG_PRINTF("[USB] USB in — unmount FatFs\r\n");
        Safe_FatFs_Unmount();
        usb_active = 1;
        g_last_scsi_tick = Tick_GetMs();
        prev_usb_configured = 1;
        return;
    }

    if (!usb_active) {
        /* USB 未连接，不做任何事，等待枚举完成 */
        return;
    }

    /* ════ USB 物理断开 ════ */
    if (prev_usb_configured && !usb_configured) {
        DBG_PRINTF("[USB] USB out\r\n");
        usb_msc_device_deinit();
        Tick_DelayMs(500);
        Safe_FatFs_Mount();
        DBG_PRINTF("[USB] FatFs remounted, mounted=%d\r\n", fatfs_mounted);

        /* 物理断开后检测固件 */
        Do_Upgrade();

        /* 无固件或升级失败，清理状态后重连 USB */
        FlashService_FlushCache();
        Safe_FatFs_Unmount();
        usb_active = 0;
        g_last_scsi_tick = Tick_GetMs();
        DBG_PRINTF("[USB] reconnect USB MSC\r\n");
        usb_msc_device_init();
        prev_usb_configured = 0;
        return;
    }

    /* ════ 10 秒无 SCSI 活动 — 后台检测 bin 文件（USB 保持连接） ════ */
    if (usb_configured &&
        (Tick_GetMs() - g_last_scsi_tick > 10000)) {

        /* 重置时间戳，10 秒后再检测 */
        g_last_scsi_tick = Tick_GetMs();

        DBG_PRINTF("[USB] 10s idle, checking firmware (USB stays connected)...\r\n");

        /* 临时挂载 FatFs 检测 bin 文件 */
        Safe_FatFs_Mount();

        if (UpgradeSource_Get() && UpgradeSource_Get()->detect()) {
            /* 检测到固件 — 断开 USB 进行升级 */
            DBG_PRINTF("[UPG] firmware detected — disconnect USB for upgrade\r\n");
            usb_msc_device_deinit();
            Tick_DelayMs(500);
            /* FatFs 已挂载，直接执行升级 */
            Do_Upgrade();

            /* 升级失败或无固件，清理状态后重连 USB */
            FlashService_FlushCache();
            Safe_FatFs_Unmount();
            usb_active = 0;
            g_last_scsi_tick = Tick_GetMs();
            DBG_PRINTF("[USB] reconnect USB MSC after upgrade check\r\n");
            usb_msc_device_init();
        } else {
            /* 没有固件 — 卸载 FatFs，USB 保持连接 */
            Safe_FatFs_Unmount();
        }
    }

    prev_usb_configured = usb_configured;
}
