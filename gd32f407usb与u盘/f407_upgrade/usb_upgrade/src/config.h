/**
 * @file config.h
 * @brief 升级模块内部统一头文件
 *
 * 模块内部所有 .c 文件都 include 这个。
 * 唯一配置入口：board_config.h
 */

#ifndef __USB_UPGRADE_CONFIG_H
#define __USB_UPGRADE_CONFIG_H

#include "board_config.h"
#include <string.h>

/* ──── 驱动头文件 ──── */
#include "w25q128_drv.h"
#include "tick_drv.h"
#include "platform.h"

#ifdef USB_UPGRADE_USE_UART
#include "debug_uart.h"
#endif

/* ──── 调试输出宏 ──── */
#ifdef USB_UPGRADE_USE_UART
  #include <stdarg.h>
  void USB_Upgrade_Printf(const char *fmt, ...);
  #define DBG_PRINTF(...) USB_Upgrade_Printf(__VA_ARGS__)
#else
  #define DBG_PRINTF(...) ((void)0)
#endif

/* ──── 模块内部头文件 ──── */
#include "flash_service.h"
#include "fatfs_system.h"
#include "upgrade_config.h"
#include "upgrade_source.h"
#include "upgrade.h"
#include "firmware_check.h"
#include "usb_mode.h"
#include "usb_host_task.h"

#endif /* __USB_UPGRADE_CONFIG_H */
