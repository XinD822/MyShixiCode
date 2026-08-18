/**
 * @file usb_task.c
 * @brief USB 拖拽升级任务（模块内部）
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

void USB_Task_Init(void)
{
    Safe_FatFs_Mount();
    UpgradeSource_Register(&Source_USB_Drag);
    usb_msc_device_init();
    Upgrade_Init();
}

void USB_Task_Run(void)
{
    uint8_t usb_configured = usb_msc_is_configured();

    /* USB 连接 */
    if (!usb_active && usb_configured) {
        DBG_PRINTF("[USB] USB in\r\n");
        Safe_FatFs_Unmount();
        usb_active = 1;
        g_last_scsi_tick = HAL_Tick->get_tick();
        prev_usb_configured = 1;
        return;
    }

    /* USB 空闲检测 */
    if (usb_active) {
        uint8_t should_exit = 0;

        if (prev_usb_configured && !usb_configured) {
            /* USB 物理断开 */
            should_exit = 1;
            DBG_PRINTF("[USB] USB out\r\n");
        }
        else if (usb_configured &&
                 (HAL_Tick->get_tick() - g_last_scsi_tick > 10000)) {
            /* USB 连接但 10 秒无 SCSI 活动，认为空闲 */
            should_exit = 1;
            DBG_PRINTF("[USB] USB idle\r\n");
        }

        if (should_exit) {
            usb_msc_device_deinit();
            HAL_Tick->delay_ms(500);
            Safe_FatFs_Mount();

            /* 检查固件 */
            if (UpgradeSource_Get() && UpgradeSource_Get()->detect()) {
                if (Upgrade_Check() == UPGRADE_OK) {
                    if (Upgrade_Execute() == UPGRADE_OK) {
                        /* 升级成功，删除固件文件防止重复升级 */
                        const UpgradeSource_t *src = UpgradeSource_Get();
                        DBG_PRINTF("[UPG] cleanup start\r\n");
                        if (src && src->cleanup) {
                            src->cleanup();
                        }
                        FlashService_FlushCache();
                        DBG_PRINTF("[UPG] after cleanup detect=%d\r\n", src ? src->detect() : 0);

                        /* 标记本次升级已完成；Bootloader 搬运后若没写 DONE，
                           下次启动 Upgrade_StateCheck 也会据此清掉升级 flag，
                           避免 PC 重新同步 firmware.bin 导致死循环。 */
                        uint32_t done = UPGRADE_STATE_DONE;
                        FlashService_Write((uint8_t *)&done, UPGRADE_STATE_ADDR, sizeof(done));
                        FlashService_FlushCache();
                        DBG_PRINTF("[UPG] state -> DONE, reset\r\n");

                        HAL_Tick->delay_ms(500);
                        HAL_Pwr->system_reset();
                    }
                }
            }

            usb_active = 0;
            usb_msc_device_init();
        }
    }

    prev_usb_configured = usb_configured;
}
