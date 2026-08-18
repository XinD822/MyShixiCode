/**
 * @file config.h
 * @brief Bootloader 配置头文件
 *
 * 功能说明：
 *   包含 Bootloader 所需的所有头文件和宏定义
 *   统一管理中断使能/禁用宏
 *
 * 芯片切换：
 *   在 chip_select.h 中定义 CHIP_SERIES_F103 或 CHIP_SERIES_F407，
 *   platform.h 会自动包含对应的标准库头文件和统一宏。
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

/* ──── 芯片系列选择 + 平台抽象 ──── */
#include "chip_select.h"
#include "platform.h"

/* ──── 中断控制宏 ──── */
/* PRIMASK寄存器：0=中断开启，1=中断关闭 */
#define CPU_INT_ENABLE() {__set_PRIMASK(0);}   /* 使能中断 */
#define CPU_INT_DISABLE() {__set_PRIMASK(1);}  /* 禁用中断 */

/* ──── C标准库 ──── */
#include "stdio.h"     /* printf等IO函数 */
#include "string.h"    /* memcpy、memset等字符串函数 */

/* ──── 硬件驱动 ──── */
#include "RemapGPIO.h" /* GPIO重映射配置 */
#include "Delay.h"     /* 延时函数 */
#include "system.h"    /* 系统初始化函数 */
#include "TIM2.h"      /* 定时器2配置 */
#include "usart.h"     /* 串口通信 */
#include "spi_flash.h" /* W25Q128 SPI Flash驱动 */

/* ──── 升级配置 ──── */
#include "upgrade_config.h"  /* 升级相关地址和配置 */

#endif /* __CONFIG_H__ */
