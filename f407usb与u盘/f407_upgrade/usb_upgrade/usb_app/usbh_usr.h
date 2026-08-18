/**
 * @file usbh_usr.h
 * @brief USB Host 用户回调 + U盘读写封装
 *
 * 基于正点原子 USB Host 例程适配，适配轮询模式（无硬件中断）。
 */

#ifndef __USBH_USR_H__
#define __USBH_USR_H__

#include "usbh_core.h"
#include "usbh_msc_core.h"
#include "usb_hcd_int.h"
#include "usb_conf.h"

/* Host 用户回调表 */
extern USBH_Usr_cb_TypeDef USR_Callbacks;

/* U盘就绪标志（枚举成功后置 1） */
extern volatile uint8_t g_udisk_ready;

/* U盘读写封装（供 diskio.c 调用） */
uint8_t USBH_UDISK_Status(void);
uint8_t USBH_UDISK_Read(uint8_t *buf, uint32_t sector, uint32_t cnt);
uint8_t USBH_UDISK_Write(uint8_t *buf, uint32_t sector, uint32_t cnt);

#endif /* __USBH_USR_H__ */
