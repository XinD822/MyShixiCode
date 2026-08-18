/**
 * @file config.h
 * @brief 升级模块内部统一头文件
 *
 * 模块内部所有 .c 文件都 include 这个。
 * 配置入口（按优先级）：
 *   1. plat_config.h      — 平台开关（OS / 芯片 / 日志级别 / 升级来源）
 *   2. board_pin_config.h — 引脚定义（SPI Flash / USB / UART / 按钮 / LED）
 *   3. partition_table.h  — Flash 分区表
 */

#ifndef __USB_UPGRADE_CONFIG_H
#define __USB_UPGRADE_CONFIG_H

#include <string.h>

/* ──── 三大配置头文件 ──── */
#include "plat_config.h"
#include "board_pin_config.h"
#include "partition_table.h"

/* ──── 平台抽象层 ──── */
#include "plat_flash.h"
#include "plat_tick.h"
#include "plat_log.h"
#include "plat_timer.h"
#include "plat_button.h"
#include "plat_reset.h"

/* ──── FatFS ──── */
#include "ff.h"

/* ──── 调试输出兼容宏 ────
 * 新代码用 LOGD / LOGI / LOGE；
 * DBG_PRINTF 保留兼容，等价于 LOGD。 */
#define DBG_PRINTF(...)  LOGD(__VA_ARGS__)

/* ──── 旧驱动头文件兼容（过渡期） ──── */
#include "w25q128_drv.h"   /* Flash_Init/ReadID/Read/Reset 等 → plat_flash_* */
#include "tick_drv.h"      /* Tick_* → plat_tick_*（宏重定向） */

/* ──── 模块内部头文件 ──── */
#include "flash_service.h"
#include "fatfs_system.h"
#include "upgrade_config.h"   /* → partition_table.h 的别名 */
#include "upgrade_source.h"
#include "upgrade.h"
#include "firmware_check.h"
#include "usb_mode.h"
#include "usb_task.h"
#include "usb_host_task.h"

#endif /* __USB_UPGRADE_CONFIG_H */
