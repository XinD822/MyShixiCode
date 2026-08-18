/**
  * @file    usbd_usr.c
  * @brief   USB Device 用户回调
  *          基于正点原子例程，bDeviceState 用于连接状态检测
  */

#include "usbd_usr.h"
#include "usb_dcd_int.h"
#include "usb_hcd_int.h"
#include "usb_regs.h"
#include "config.h"

/* USB 设备连接状态：0=未连接, 1=已连接 */
volatile uint8_t bDeviceState = 0;

/* 轮询模式统计 */
volatile uint32_t g_usb_isr_count = 0;
volatile uint32_t g_usb_poll_gintsts = 0;

/* Host 模式 OTG_FS 硬件中断计数器（用于调试中断是否触发） */
volatile uint32_t g_host_isr_count = 0;

extern USB_OTG_CORE_HANDLE  USB_OTG_dev;
extern USB_OTG_CORE_HANDLE  USB_OTG_Core;  /* Host 模式（定义在 usb_host_task.c） */

/**
  * @brief  USB 轮询处理 — 在主循环中调用，替代中断
  *         检查 GINTSTS，如果有 pending 中断则调用 handler
  */
void USB_Poll_Handler(void)
{
  uint32_t gintsts = USB_OTG_READ_REG32(&USB_OTG_dev.regs.GREGS->GINTSTS);
  uint32_t gintmsk = USB_OTG_READ_REG32(&USB_OTG_dev.regs.GREGS->GINTMSK);

  if (gintsts & gintmsk) {
    g_usb_poll_gintsts = gintsts;
    g_usb_isr_count++;
    USBD_OTG_ISR_Handler(&USB_OTG_dev);
  }
}

/**
  * @brief  OTG_FS 中断处理 — 根据当前 USB 模式分发。
  *         Host:   硬件中断已启用，OTG_FS_IRQHandler 自动调用
  *                 USBH_OTG_ISR_Handler 处理端口连接/枚举等事件。
  *         Device: 硬件中断已禁用（轮询模式），此函数仅作安全网，
  *                 清除所有 pending 标志防止中断重入。
  */
void OTG_FS_IRQHandler(void)
{
  if (USB_Mode_Get() == USB_MODE_HOST) {
    g_host_isr_count++;
    USBH_OTG_ISR_Handler(&USB_OTG_Core);
  } else {
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->GINTSTS, 0xBFFFFFFF);
  }
}

USBD_Usr_cb_TypeDef USR_cb =
{
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,
  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,
};

void USBD_USR_Init(void)
{
}

void USBD_USR_DeviceReset (uint8_t speed)
{
  (void)speed;
}

void USBD_USR_DeviceConfigured (void)
{
  /* VBUS_SENSING_ENABLED 未定义时，DevConnected 回调不会被调用，
     在此设置 bDeviceState 以正确反映设备已枚举配置完成 */
  bDeviceState = 1;
  DBG_PRINTF("[USB] MSC Configured\r\n");
}

void USBD_USR_DeviceSuspended(void)
{
  bDeviceState = 0;
  DBG_PRINTF("[USB] Suspended -> disconnected\r\n");
}

void USBD_USR_DeviceResumed(void)
{
}

void USBD_USR_DeviceConnected (void)
{
  bDeviceState = 1;
  DBG_PRINTF("[USB] Connected\r\n");
}

void USBD_USR_DeviceDisconnected (void)
{
  bDeviceState = 0;
  DBG_PRINTF("[USB] Disconnected\r\n");
}
