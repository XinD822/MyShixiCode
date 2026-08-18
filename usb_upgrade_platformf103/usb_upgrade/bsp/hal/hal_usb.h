/**
 * @file hal_usb.h
 * @brief USB 底层 HAL 接口
 *
 * 仅负责 USB 外设的时钟和中断管理。
 * USB 协议层由 CherryUSB 处理，不在此抽象。
 */

#ifndef __HAL_USB_H
#define __HAL_USB_H

#include <stdint.h>

typedef struct {
    void (*clock_enable)(void);
    void (*clock_disable)(void);
    void (*nvic_enable)(void);
    void (*nvic_disable)(void);
    void (*reset)(void);
} HAL_Usb_Drv_t;

extern const HAL_Usb_Drv_t *HAL_Usb;

#endif /* __HAL_USB_H */
