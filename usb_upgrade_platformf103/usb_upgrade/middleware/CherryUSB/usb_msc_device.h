/*
 * CherryUSB MSC Device Header
 */

#ifndef USB_MSC_DEVICE_H
#define USB_MSC_DEVICE_H

#include <stdint.h>

/* 初始化USB MSC设备 */
void usb_msc_device_init(void);

/* 反初始化USB（关闭中断和时钟，释放SPI总线） */
void usb_msc_device_deinit(void);

/* 查询USB是否已配置 */
uint8_t usb_msc_is_configured(void);

#endif /* USB_MSC_DEVICE_H */
