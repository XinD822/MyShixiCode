/**
  * @file    usb_msc_device.h
  * @brief   USB MSC 设备封装接口（供 usb_task.c 调用）
  */

#ifndef __USB_MSC_DEVICE_H
#define __USB_MSC_DEVICE_H

#include <stdint.h>

/**
 * @brief 初始化 USB MSC 设备（调用 USBD_Init）
 */
void usb_msc_device_init(void);

/**
 * @brief 反初始化 USB MSC 设备
 *        断开 D+ 上拉，关闭中断
 */
void usb_msc_device_deinit(void);

/**
 * @brief 查询 USB 是否已枚举配置完成
 * @return 1=已连接, 0=未连接
 */
uint8_t usb_msc_is_configured(void);

#endif /* __USB_MSC_DEVICE_H */
