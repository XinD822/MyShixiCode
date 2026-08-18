/**
 * @file board_config.h
 * @brief 板级配置 — 已拆分为 plat_config.h + board_pin_config.h
 *
 * 此文件保留仅为向后兼容，新代码直接包含：
 *   #include "plat_config.h"       // 平台开关（OS/芯片/日志）
 *   #include "board_pin_config.h"  // 引脚定义
 *   #include "partition_table.h"   // Flash 分区
 */
#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#include "plat_config.h"
#include "board_pin_config.h"
#include "partition_table.h"

/* ──── 向后兼容宏 ──── */
#define CHIP_SERIES_F407
#define USE_BAREMETAL   (PLAT_SELECT == PLAT_BARE_METAL)
#define USE_UCOS2       (PLAT_SELECT == PLAT_UCOS2)
#define SYSTEM_CORE_CLOCK  (SYSTEM_CORE_CLOCK_MHZ * 1000000u)
#define USB_UPGRADE_USE_UART  DEBUG_UART_ENABLE
#define CONFIG_USB_USE_UCOS2  (PLAT_SELECT == PLAT_UCOS2)

#endif /* __BOARD_CONFIG_H */
