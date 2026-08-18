/**
  * @file    usb_conf.h
  * @brief   USB OTG FS 配置（仅设备模式）
  *          基于正点原子 STM32F407 USB读卡器例程，直接定义 USE_USB_OTG_FS
  */

#ifndef __USB_CONF__H__
#define __USB_CONF__H__
#include "stm32f4xx.h"

/* ════ USB OTG FS PHY 配置 ════
 * 直接定义 USE_USB_OTG_FS，不依赖 Keil 预处理器宏 */
#define USE_USB_OTG_FS
#ifdef USE_USB_OTG_FS
 #define USB_OTG_FS_CORE
#endif

/* ════ USB OTG FS FIFO 大小配置 ════ */
#ifdef USB_OTG_FS_CORE
 #define RX_FIFO_FS_SIZE                          128
 #define TX0_FIFO_FS_SIZE                          64
 #define TX1_FIFO_FS_SIZE                         128
 #define TX2_FIFO_FS_SIZE                          0
 #define TX3_FIFO_FS_SIZE                          0
 /* Host 模式 FIFO 大小（USB_OTG_CoreInitHost 使用） */
 #define TXH_NP_FS_FIFOSIZ                         96
 #define TXH_P_FS_FIFOSIZ                          96
#endif

/* ════ VBUS 检测 ════
 * 正点原子 F407 最小系统板未使用 VBUS 检测，禁用 */
//#define VBUS_SENSING_ENABLED

/* ════ USB 模式选择 ════
 * 同时编译 Host 和 Device 驱动，运行时二选一 */
#define USE_HOST_MODE
#define USE_DEVICE_MODE
//#define USE_OTG_MODE

#ifndef USB_OTG_FS_CORE
 #ifndef USB_OTG_HS_CORE
    #error  "USB_OTG_HS_CORE or USB_OTG_FS_CORE should be defined"
 #endif
#endif

#ifndef USE_DEVICE_MODE
 #ifndef USE_HOST_MODE
    #error  "USE_DEVICE_MODE or USE_HOST_MODE should be defined"
 #endif
#endif

/* ════ 编译器相关关键字 ════ */
#if defined (__CC_ARM)         /* ARM Compiler */
  #define __ALIGN_BEGIN    __align(4)
  #define __ALIGN_END
  #define __packed    __packed
#elif defined (__ICCARM__)     /* IAR Compiler */
  #define __ALIGN_BEGIN
  #define __ALIGN_END
  #define __packed    __packed
#elif defined   ( __GNUC__ )   /* GNU Compiler */
  #define __ALIGN_BEGIN
  #define __ALIGN_END    __attribute__ ((aligned (4)))
  #define __packed    __attribute__ ((__packed__))
#elif defined   (__TASKING__)  /* TASKING Compiler */
  #define __ALIGN_BEGIN    __align(4)
  #define __ALIGN_END
  #define __packed    __unaligned
#endif

#endif /* __USB_CONF__H__ */
