/**
  * @file    usb_conf.h
  * @brief   USB core driver basic configuration (GD32F407 USBFS)
  */

#ifndef __USB_CONF_H
#define __USB_CONF_H

#include <stdlib.h>
#include "gd32f4xx.h"

/* 编译器相关关键字（GD 库描述符使用） */
#if defined (__GNUC__)         /* GNU Compiler */
    #define __ALIGN_END        __attribute__ ((aligned (4)))
    #define __ALIGN_BEGIN
#else
    #define __ALIGN_END

    #if defined (__CC_ARM)     /* ARM Compiler */
        #define __ALIGN_BEGIN  __align(4)
    #elif defined (__ICCARM__) /* IAR Compiler */
        #define __ALIGN_BEGIN
    #elif defined (__TASKING__)/* TASKING Compiler */
        #define __ALIGN_BEGIN  __align(4)
    #endif /* __CC_ARM */
#endif /* __GNUC__ */

/* USB 核心全局配置（drv_usb_core.c 直接使用） */
#define USB_SOF_OUTPUT                  0
#define USB_LOW_POWER                   0

#define USE_USB_FS

#ifdef USE_USB_FS
    #define USB_FS_CORE
#endif

#ifdef USE_USB_HS
    #define USB_HS_CORE
#endif

/* USBFS 内部 FIFO 分配（1.25KB = 320 words） */
#ifdef USB_FS_CORE
    /* Device 模式 FIFO */
    #define RX_FIFO_FS_SIZE                 128
    #define TX0_FIFO_FS_SIZE                64
    #define TX1_FIFO_FS_SIZE                128
    #define TX2_FIFO_FS_SIZE                0
    #define TX3_FIFO_FS_SIZE                0

    /* Host 模式 FIFO（drv_usb_host.c 使用，宏名与官方 USB_HOST 例程一致） */
    #define USB_RX_FIFO_FS_SIZE             128
    #define USB_HTX_NPFIFO_FS_SIZE          96
    #define USB_HTX_PFIFO_FS_SIZE           96

    #define USBFS_SOF_OUTPUT                0
    #define USBFS_LOW_POWER                 0
#endif

#ifdef USB_HS_CORE
    #define RX_FIFO_HS_SIZE                 512
    #define TX0_FIFO_HS_SIZE                128
    #define TX1_FIFO_HS_SIZE                128
    #define TX2_FIFO_HS_SIZE                128
    #define TX3_FIFO_HS_SIZE                0
    #define TX4_FIFO_HS_SIZE                0
    #define TX5_FIFO_HS_SIZE                0

    #define USBHS_SOF_OUTPUT                0
    #define USBHS_LOW_POWER                 0
#endif

/* 模式选择：DEVICE 与 HOST 双模式编译，运行时按 Flash 标志切换
   （对照 STM32 版：DEVICE=拖拽升级，HOST=U盘插入升级）。
   注意：drv_usb_hw.h 中 usb_vbus_config/usb_vbus_drive 的声明
   在 #ifdef USE_HOST_MODE 内，而 drv_usb_host.c 的 usb_portvbus_switch
   无条件调用 usb_vbus_drive —— 不定义 USE_HOST_MODE 会链接失败。 */
//#define VBUS_SENSING_ENABLED

#define USE_DEVICE_MODE
#define USE_HOST_MODE
//#define USE_OTG_MODE

#ifndef USE_OTG_MODE
    #ifndef USE_HOST_MODE
        #define USE_DEVICE_MODE
    #endif
#endif

#endif /* __USB_CONF_H */
