/**
  * @file    usb_msc_device.c
  * @brief   USB MSC 设备封装 — 对接 ST 官方 USB 库
  *
  * 暴露 usb_msc_device_init / usb_msc_is_configured / usb_msc_device_deinit
  * 供 usb_task.c 调用，内部调用 USBD_Init / DCD_DevDisconnect。
  *
  * 关键设计：USBD_Init 只调用一次（首次初始化），后续断开/重连仅使用
  * DCD_DevDisconnect / DCD_DevConnect 控制 D+ 上拉。
  * ST 库没有完整的 DeInit，重复调用 USBD_Init 会导致 USB 核心软复位挂死。
  */

#include "usb_msc_device.h"
#include "config.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc_core.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usb_dcd_int.h"
#include "usb_dcd.h"
#include "usb_defines.h"
#include "delay.h"

/* 全局 USB OTG 设备句柄（usb_bsp.c 的 OTG_FS_IRQHandler 引用） */
USB_OTG_CORE_HANDLE USB_OTG_dev;

/* bDeviceState 定义在 usbd_usr.c */
extern volatile uint8_t bDeviceState;

/* USB 核心是否已完成初始化（USBD_Init 只允许调用一次） */
static uint8_t usb_core_inited = 0;

void usb_msc_device_init(void)
{
  if (usb_core_inited) {
    /*
     * 核心已初始化 — 仅重新连接 D+ 上拉，不重复调用 USBD_Init。
     *
     * 主机检测到 D+ 上拉后会发送 USB Reset，ST 库的 Reset 处理会：
     *   1. 清除设备地址 → DCD_EPSetAddress(0)
     *   2. 重置所有端点
     *   3. 重新调用 MSC class Init（通过 SET_CONFIGURATION）
     * 因此无需手动重置 MSC 层状态。
     */
    DBG_PRINTF("[MSC] Reconnect (DevConnect only)\r\n");

    /* 清除可能残留的中断标志，避免轮询处理器误触发 */
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->GINTSTS, 0xBFFFFFFF);

    /* 确保 NVIC 仍然关闭 — 用轮询模式处理 */
    NVIC_DisableIRQ(OTG_FS_IRQn);

    /* 重新使能 D+ 内部上拉 */
    DCD_DevConnect(&USB_OTG_dev);
    DBG_PRINTF("[MSC] Reconnect done\r\n");
    return;
  }

  /* ════ 首次初始化（仅执行一次） ════ */
  DBG_PRINTF("[MSC] USBD_Init start\r\n");

  USBD_Init(&USB_OTG_dev,
            USB_OTG_FS_CORE_ID,
            &USR_desc,
            &USBD_MSC_cb,
            &USR_cb);

  DBG_PRINTF("[MSC] USBD_Init done\r\n");

  /* 禁用 USB 硬件中断 — 改用主循环轮询 (USB_Poll_Handler) 处理 USB 事件。
     USBD_Init 内部调用 USB_OTG_BSP_EnableInterrupt 启用了 NVIC，
     此处立即关闭，防止 ISR 触发导致系统崩溃。 */
  NVIC_DisableIRQ(OTG_FS_IRQn);

  /* 使能 D+ 内部上拉，让电脑检测到设备接入 */
  DCD_DevConnect(&USB_OTG_dev);
  DBG_PRINTF("[MSC] DCD_DevConnect done\r\n");

  /* 打印关键寄存器值，用于验证 USB 控制器状态 */
  DBG_PRINTF("[MSC] GUSBCFG=0x%08X (bit30=FDMOD should be 1)\r\n",
             USB_OTG_READ_REG32(&USB_OTG_dev.regs.GREGS->GUSBCFG));
  DBG_PRINTF("[MSC] GCCFG=0x%08X (bit16=PWRDWN, bit21=NOVBUSSENS)\r\n",
             USB_OTG_READ_REG32(&USB_OTG_dev.regs.GREGS->GCCFG));
  DBG_PRINTF("[MSC] GINTSTS=0x%08X (bit0=CMOD, 0=device mode)\r\n",
             USB_OTG_READ_REG32(&USB_OTG_dev.regs.GREGS->GINTSTS));
  DBG_PRINTF("[MSC] DCTL=0x%08X (bit1=SFTDISCON, 0=connected)\r\n",
             USB_OTG_READ_REG32(&USB_OTG_dev.regs.DREGS->DCTL));
  DBG_PRINTF("[MSC] GAHBCFG=0x%08X (bit0=GINT, should be 1)\r\n",
             USB_OTG_READ_REG32(&USB_OTG_dev.regs.GREGS->GAHBCFG));
  DBG_PRINTF("[MSC] GOTGCTL=0x%08X (bit19=BSESVLD, 1=VBUS present)\r\n",
             USB_OTG_READ_REG32(&USB_OTG_dev.regs.GREGS->GOTGCTL));

  usb_core_inited = 1;
  DBG_PRINTF("[MSC] Init done (core_inited=1)\r\n");
}

void usb_msc_device_deinit(void)
{
  /* 断开 D+ 上拉，让电脑识别为拔出 */
  DCD_DevDisconnect(&USB_OTG_dev);
  NVIC_DisableIRQ(OTG_FS_IRQn);
  bDeviceState = 0;

  DBG_PRINTF("[MSC] DevDisconnect done (core still inited)\r\n");
}

uint8_t usb_msc_is_configured(void)
{
  return bDeviceState;
}
