/**
 * @file config.h
 * @brief 升级模块内部统一头文件
 *
 * 模块内部所有 .c 文件都 include 这个，不需要直接 include hal_config.h
 */

#ifndef __USB_UPGRADE_CONFIG_H
#define __USB_UPGRADE_CONFIG_H

#include "hal_config.h"
#include "board_config.h"
#include <string.h>

/* 调试输出宏：启用串口时用 printf，否则静默 */
#ifdef USB_UPGRADE_USE_UART
  #include <stdio.h>
  #define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINTF(...) ((void)0)
#endif

#include "flash_service.h"
#include "fatfs_system.h"
#include "upgrade_config.h"
#include "upgrade_source.h"
#include "upgrade.h"
#include "firmware_check.h"
#include "error_handler.h"
#include "mutex.h"

#endif /* __USB_UPGRADE_CONFIG_H */
