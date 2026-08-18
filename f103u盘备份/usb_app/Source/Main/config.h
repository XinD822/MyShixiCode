/**
 * @file config.h
 * @brief USB APP配置头文件
 * 
 * 功能说明：
 *   包含USB APP所需的所有头文件和宏定义
 *   统一管理中断使能/禁用宏
 *   包含USB MSC、FatFS、升级模块等依赖
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

/* ──── 中断控制宏 ──── */
/* PRIMASK寄存器：0=中断开启，1=中断关闭 */
#define CPU_INT_ENABLE() {__set_PRIMASK(0);}   /* 使能中断 */
#define CPU_INT_DISABLE() {__set_PRIMASK(1);}  /* 禁用中断 */

/* ──── STM32标准外设库 ──── */
#include "stm32f10x.h"

/* ──── C标准库 ──── */
#include "stdio.h"     /* printf等IO函数 */
#include "string.h"    /* memcpy、memset等字符串函数 */

/* ──── 硬件驱动 ──── */
#include "RemapGPIO.h" /* GPIO重映射配置 */
#include "Delay.h"     /* 延时函数 */
#include "system.h"    /* 系统初始化函数 */
#include "TIM2.h"      /* 定时器2配置 */
#include "usart.h"     /* 串口通信 */

/* ──── 文件系统 ──── */
#include "ff.h"            /* FatFS文件系统 */
#include "spi_flash.h"     /* W25Q128 SPI Flash驱动 */
#include "fatfs_system.h"  /* FatFS系统接口 */

/* ──── 外设驱动 ──── */
#include "led.h"  /* LED指示灯驱动 */
#include "sys.h"  /* 系统配置 */

/* ──── 升级模块 ──── */
#include "upgrade_config.h"   /* 升级相关地址和配置 */
#include "firmware_check.h"   /* 固件校验函数 */
#include "upgrade.h"          /* 升级模块接口 */
#include "mutex.h"            /* 互斥锁（防止USB和FatFS冲突） */
#include "error_handler.h"    /* 错误处理模块 */

/* ──── USB MSC模块（CherryUSB） ──── */
#include "usb_msc_device.h"  /* CherryUSB MSC设备接口 */

#endif /* __CONFIG_H__ */
