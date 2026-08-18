/**
 * @file hal_config.h
 * @brief 平台总配置 — 唯一需要改的配置文件
 *
 * 移植时只改这个文件，其他所有代码不动。
 */

#ifndef __HAL_CONFIG_H
#define __HAL_CONFIG_H

/* ═══════════════════════════════════════════════════════════
 * 平台选择（只改这里）
 * ═══════════════════════════════════════════════════════════ */
#define PLATFORM_STM32
// #define PLATFORM_GD32

/* ═══════════════════════════════════════════════════════════
 * 芯片系列
 * ═══════════════════════════════════════════════════════════ */
#define CHIP_SERIES_F103
// #define CHIP_SERIES_F407

/* ═══════════════════════════════════════════════════════════
 * RTOS 选择
 * ═══════════════════════════════════════════════════════════ */
#define USE_BAREMETAL
// #define USE_UCOS2

/* ═══════════════════════════════════════════════════════════
 * 升级来源（可多选，1=启用，0=禁用）
 * ═══════════════════════════════════════════════════════════ */
#define UPGRADE_SRC_USB_DRAG    1       /* USB 拖拽升级 */
#define UPGRADE_SRC_SD_CARD     0       /* SD 卡升级（预留） */
#define UPGRADE_SRC_USB_DRIVE   0       /* U 盘升级（预留） */

/* ═══════════════════════════════════════════════════════════
 * Flash 驱动选择
 * ═══════════════════════════════════════════════════════════ */
#define FLASH_DRIVER_W25Q128    1
#define FLASH_DRIVER_W25Q256    0
#define FLASH_DRIVER_INTERNAL   0

/* ═══════════════════════════════════════════════════════════
 * SPI Flash 连接方式
 * ═══════════════════════════════════════════════════════════ */
#define FLASH_SPI_SOFTWARE      1       /* 1=软件SPI, 0=硬件SPI */

/* ═══════════════════════════════════════════════════════════
 * USB CherryUSB 模式切换（传递给 usb_config.h）
 * ═══════════════════════════════════════════════════════════ */
#ifdef USE_UCOS2
#define CONFIG_USB_USE_UCOS2
#endif

/* ═══════════════════════════════════════════════════════════
 * 平台自动包含
 * ═══════════════════════════════════════════════════════════ */
#if defined(PLATFORM_STM32)
  #include "st/st_platform.h"
#elif defined(PLATFORM_GD32)
  #include "gd/gd_platform.h"
#else
  #error "Please define PLATFORM_STM32 or PLATFORM_GD32 in hal_config.h"
#endif

/* 包含所有 HAL 接口头文件 */
#include "hal/hal_flash.h"
#include "hal/hal_tick.h"
#include "hal/hal_uart.h"
#include "hal/hal_gpio.h"
#include "hal/hal_usb.h"
#include "hal/hal_pwr.h"

#endif /* __HAL_CONFIG_H */
