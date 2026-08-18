/**
 * @file usb_mode.c
 * @brief USB 模式管理实现
 *
 * 上电读 Flash 标志选模式；运行时按键切标志 + 软件复位。
 * DEVICE → USB_Task（拖拽升级）
 * HOST   → USB_Host_Task（U盘读取升级）
 */

#include "config.h"
#include "usb_mode.h"

static USB_Mode_t g_usb_mode = USB_MODE_DEVICE;
static uint8_t    g_force_device = 0;   /* 恢复模式：KEY0 长按开机时置 1 */

void USB_Mode_ForceDevice(void)
{
    g_force_device = 1;
}

USB_Mode_t USB_Mode_Get(void)
{
    return g_usb_mode;
}

void USB_Mode_Set(USB_Mode_t mode)
{
    g_usb_mode = mode;
    LOGD("[USB_MODE] set to %s\r\n",
         mode == USB_MODE_DEVICE ? "DEVICE" : "HOST");
}

USB_Mode_t USB_Mode_ReadFlag(void)
{
    /* 恢复模式：绕过 Flash 标志，直接返回 DEVICE */
    if (g_force_device) {
        LOGD("[USB_MODE] *** RECOVERY: forcing DEVICE mode ***\r\n");
        return USB_MODE_DEVICE;
    }

    uint32_t flag = 0;
    FlashService_Read((uint8_t *)&flag, USB_MODE_FLAG_ADDR, sizeof(flag));
    if (flag == USB_MODE_FLAG_HOST) {
        LOGD("[USB_MODE] Flash flag = HOST\r\n");
        return USB_MODE_HOST;
    }
    LOGD("[USB_MODE] Flash flag = DEVICE (flag=0x%08X)\r\n",
         (unsigned int)flag);
    return USB_MODE_DEVICE;
}

void USB_Mode_WriteFlag(USB_Mode_t mode)
{
    uint32_t flag = (mode == USB_MODE_HOST) ? USB_MODE_FLAG_HOST : USB_MODE_FLAG_DEVICE;
    /* 擦除独立扇区（0xFE1000），不影响升级配置区（0xFE0000） */
    FlashService_EraseSector(USB_MODE_FLAG_ADDR);
    FlashService_InvalidateCache();
    FlashService_Write((uint8_t *)&flag, USB_MODE_FLAG_ADDR, sizeof(flag));
    FlashService_FlushCache();
    LOGD("[USB_MODE] Flash flag written: %s\r\n",
         mode == USB_MODE_HOST ? "HOST" : "DEVICE");
}

void USB_Mode_ToggleFlag(void)
{
    USB_Mode_t current = USB_Mode_ReadFlag();
    USB_Mode_t new_mode = (current == USB_MODE_HOST) ? USB_MODE_DEVICE : USB_MODE_HOST;
    USB_Mode_WriteFlag(new_mode);
}

void USB_Mode_Init(void)
{
    if (g_usb_mode == USB_MODE_DEVICE) {
        USB_Task_Init();
    } else {
        USB_Host_Task_Init();
    }
}

void USB_Mode_Run(void)
{
    if (g_usb_mode == USB_MODE_DEVICE) {
        USB_Task_Run();
    } else {
        USB_Host_Task_Run();
    }
}
