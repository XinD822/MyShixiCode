/**
 * @file usbh_usr.h
 * @brief USB Host 用户回调 + U盘读写封装（GD32F4xx_usb_library 版）
 */

#ifndef __USBH_USR_H__
#define __USBH_USR_H__

#include "usbh_core.h"
#include "usbh_msc_core.h"

/* Host 用户回调表（GD 库定义在 usbh_usr.c） */
extern usbh_user_cb usr_cb;

/* U盘就绪标志（枚举成功后置 1） */
extern volatile uint8_t g_udisk_ready;

/* 重试计数（升级失败后重试） */
extern uint8_t g_retry_count;
#define UPGRADE_MAX_RETRIES  3

/* U盘读写封装（供 diskio.c 调用） */
uint8_t USBH_UDISK_Status(void);
uint8_t USBH_UDISK_Read(uint8_t *buf, uint32_t sector, uint32_t cnt);
uint8_t USBH_UDISK_Write(uint8_t *buf, uint32_t sector, uint32_t cnt);

#endif /* __USBH_USR_H__ */
