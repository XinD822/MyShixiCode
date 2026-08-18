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

/* “USB out” 去抖：主机在 SET_CONFIGURATION 后常做一次内部复位
   （Windows USBSTOR 初始化），会产生短暂的“未配置”状态。若立即当
   物理拔出处理并物理重连，会陷入“连接→复位→重连”的自维持死循环。
   此处要求未配置状态持续超过阈值才判定为真实断开。 */
#define USB_UNCONFIG_DEBOUNCE_MS   1000u
static uint8_t  unconfig_watch = 0;
static uint32_t unconfig_start = 0;

static void Safe_FatFs_Mount(void)
{
    if (!fatfs_mounted) {
        if (FatFs_Mount() == FR_OK) {
            fatfs_mounted = 1;
        }
    }
}

/* 安全挂载（禁止自动格式化）：
 * 仅用于 USB 连接期间的 10s 空闲检测——此时 Windows 正通过 MSC 占用
 * 该卷，若挂载失败就格式化会清空 Windows 正在使用的文件系统。 */
static void Safe_FatFs_MountNoFormat(void)
{
    if (!fatfs_mounted) {
        if (FatFs_MountNoFormat() == FR_OK) {
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
        LOGD("[UPG] no source registered\r\n");
        return 0;
    }
    if (!UpgradeSource_Get()->detect()) {
        LOGD("[UPG] no firmware file\r\n");
        return 0;
    }

    LOGD("[UPG] firmware detected, checking...\r\n");

    UpgradeResult chk = Upgrade_Check();
    LOGD("[UPG] check result=%d\r\n", (int)chk);
    if (chk != UPGRADE_OK) {
        return 0;
    }

    LOGD("[UPG] executing upgrade...\r\n");
    UpgradeResult exec = Upgrade_Execute();
    LOGD("[UPG] execute result=%d\r\n", (int)exec);
    if (exec != UPGRADE_OK) {
        return 0;
    }

    /* 升级成功，删除固件文件防止重复升级 */
    const UpgradeSource_t *src = UpgradeSource_Get();
    LOGD("[UPG] cleanup start\r\n");
    if (src && src->cleanup) {
        src->cleanup();
    }
    FlashService_FlushCache();
    LOGD("[UPG] after cleanup detect=%d\r\n",
         src ? src->detect() : 0);

    /* 标记本次升级已完成 */
    uint32_t done = UPGRADE_STATE_DONE;
    FlashService_Write((uint8_t *)&done, UPGRADE_STATE_ADDR, sizeof(done));
    FlashService_FlushCache();
    LOGD("[UPG] state -> DONE, reset\r\n");

    plat_delay_ms(500);
    plat_system_reset();
    return 1;  /* 不会执行到这里 */
}

void USB_Task_Init(void)
{
    LOGD("[TASK] FatFs_Mount...\r\n");
    Safe_FatFs_Mount();
    LOGD("[TASK] FatFs_Mount OK\r\n");

    LOGD("[TASK] Register source...\r\n");
    UpgradeSource_Register(&Source_USB_Drag);
    LOGD("[TASK] Register OK\r\n");

    LOGD("[TASK] usb_msc_device_init...\r\n");
    usb_msc_device_init();
    LOGD("[TASK] usb_msc_device_init OK\r\n");

    LOGD("[TASK] Upgrade_Init...\r\n");
    Upgrade_Init();
    LOGD("[TASK] Upgrade_Init OK\r\n");
}

void USB_Task_Run(void)
{
    uint8_t usb_configured = usb_msc_is_configured();

    /* ════ USB 连接 ════ */
    if (!usb_active && usb_configured) {
        LOGD("[USB] USB in - unmount FatFs\r\n");
        Safe_FatFs_Unmount();
        usb_active = 1;
        g_last_scsi_tick = plat_get_tick_ms();
        prev_usb_configured = 1;
        return;
    }

    if (!usb_active) {
        /* USB 未连接，不做任何事，等待枚举完成 */
        return;
    }

    /* ════ USB 曾配置、现未配置 — 去抖后确认，避免误判主机复位 ════ */
    if (prev_usb_configured && !usb_configured) {
        if (!unconfig_watch) {
            unconfig_watch = 1;
            unconfig_start = plat_get_tick_ms();
            return;
        }

        if (plat_get_tick_ms() - unconfig_start < USB_UNCONFIG_DEBOUNCE_MS) {
            /* 未配置尚未稳定（主机复位/重枚举中），继续观察 */
            return;
        }

        /* 持续未配置超过阈值 — 确认为真实断开 */
        unconfig_watch = 0;
        LOGD("[USB] USB out (confirmed)\r\n");
        usb_msc_device_deinit();
        plat_delay_ms(500);
        Safe_FatFs_Mount();
        LOGD("[USB] FatFs remounted, mounted=%d\r\n", fatfs_mounted);

        /* 物理断开后检测固件 */
        Do_Upgrade();

        /* 无固件或升级失败，清理状态后重连 USB */
        FlashService_FlushCache();
        Safe_FatFs_Unmount();
        usb_active = 0;
        g_last_scsi_tick = plat_get_tick_ms();
        LOGD("[USB] reconnect USB MSC\r\n");
        usb_msc_device_init();
        prev_usb_configured = 0;
        return;
    }

    /* 已重新配置 / 保持连接 — 复位去抖观察状态 */
    unconfig_watch = 0;

    /* ════ 10 秒无 SCSI 活动 — 后台检测 bin 文件（USB 保持连接） ════ */
    if (usb_configured &&
        (plat_get_tick_ms() - g_last_scsi_tick > 10000)) {

        /* 重置时间戳，10 秒后再检测 */
        g_last_scsi_tick = plat_get_tick_ms();

        LOGD("[USB] 10s idle, checking firmware (USB stays connected)...\r\n");

        /* 挂载 FatFS 前先把 Windows 经 MSC 写入的 4KB 缓存落盘：
         * 若 Windows 刚写过 FAT 表/目录项，缓存中是最新数据，
         * 而 Flash 上还是旧数据，直接挂载会读到不一致的 FAT，
         * 轻则误判文件系统损坏，重则触发格式化破坏卷。
         * 注意：USB 连接期间用"禁止格式化"版本挂载——此时 Windows
         * 正占用该卷，挂载失败绝不能自动重建文件系统。 */
        FlashService_FlushCache();

        /* 临时挂载 FatFs 检测 bin 文件（不格式化） */
        Safe_FatFs_MountNoFormat();

        if (UpgradeSource_Get() && UpgradeSource_Get()->detect()) {
            /* 检测到固件 — 断开 USB 进行升级 */
            LOGD("[UPG] firmware detected - disconnect USB for upgrade\r\n");
            usb_msc_device_deinit();
            plat_delay_ms(500);
            /* FatFs 已挂载，直接执行升级 */
            Do_Upgrade();

            /* 升级失败或无固件，清理状态后重连 USB */
            FlashService_FlushCache();
            Safe_FatFs_Unmount();
            usb_active = 0;
            g_last_scsi_tick = plat_get_tick_ms();
            LOGD("[USB] reconnect USB MSC after upgrade check\r\n");
            usb_msc_device_init();
        } else {
            /* 没有固件 — 卸载 FatFs，USB 保持连接 */
            Safe_FatFs_Unmount();
        }
    }

    prev_usb_configured = usb_configured;
}
