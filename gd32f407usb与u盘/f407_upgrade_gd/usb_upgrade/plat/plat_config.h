/**
 * @file plat_config.h
 * @brief 整个工程唯一的平台开关 — 移植时只改这一个文件
 */
#ifndef __PLAT_CONFIG_H
#define __PLAT_CONFIG_H

#include <stdint.h>

/* ═══ 运行环境选择（二选一） ═══ */
#define PLAT_BARE_METAL   0
#define PLAT_UCOS2        1

/* ←←← 移植时只改这一行 →→→ */
#define PLAT_SELECT       PLAT_BARE_METAL

/* ═══ 裸机 tick 来源（INTERNAL/EXTERNAL 二选一） ═══
 * INTERNAL : 用本工程自带的 TIMER2（1ms 中断驱动 tick）——独立运行/调试用
 * EXTERNAL : 复用目标工程已有的定时器（外部 1ms 中断里调 plat_tick_isr()）
 *            ——移植到已有 TIM 的设备时选这个，不占用新外设
 * uCOS 模式自动用 OS tick（PLAT_TIMER_OSTICK），无需配置 */
#define PLAT_TIMER_INTERNAL   0
#define PLAT_TIMER_EXTERNAL   1
#define PLAT_TIMER_OSTICK     2

#if (PLAT_SELECT == PLAT_UCOS2)
  #define PLAT_TIMER_SOURCE   PLAT_TIMER_OSTICK
#else
  /* ←←← 移植到已有 TIM 的工程时，改成 PLAT_TIMER_EXTERNAL →→→ */
  #define PLAT_TIMER_SOURCE   PLAT_TIMER_INTERNAL
#endif

/* ═══ 芯片系列选择 ═══ */
#define CHIP_GD32F407     1

#if (CHIP_GD32F407)
  #include "gd32f4xx.h"
  #include "gd32f4xx_libopt.h"
  #define MCU_ID1  (*(uint32_t *)0x1FFF7A10)
  #define MCU_ID2  (*(uint32_t *)0x1FFF7A14)
  #define MCU_ID3  (*(uint32_t *)0x1FFF7A18)
  #define SYSTEM_CORE_CLOCK_MHZ  168
#endif

/* ═══ FatFS 卷号分配（与目标设备协商） ═══ */
#define PLAT_FATFS_VOL_LOCAL   "0:"   /* 本地数据区 / 设备存储 */
#define PLAT_FATFS_VOL_UDISK   "1:"   /* U 盘 */
#define PLAT_FATFS_PDRV_LOCAL  0
#define PLAT_FATFS_PDRV_UDISK  1

/* ═══ 日志级别 ═══ */
#define PLAT_LOG_DBG   0
#define PLAT_LOG_INFO  1
#define PLAT_LOG_ERR   2
#define PLAT_LOG_LEVEL PLAT_LOG_DBG   /* 量产改 PLAT_LOG_ERR */

/* ═══ OS tick 频率 ═══ */
#if (PLAT_SELECT == PLAT_UCOS2)
  #include "os.h"
  #define PLAT_OS_TICK_HZ   OS_TICKS_PER_SEC
#else
  #define PLAT_OS_TICK_HZ   1000  /* SysTick 1ms */
#endif

/* ═══ 升级来源（可多选，1=启用，0=禁用） ═══ */
#define UPGRADE_SRC_USB_DRAG    1
#define UPGRADE_SRC_SD_CARD     0
#define UPGRADE_SRC_USB_DRIVE   1

/* ═══ USB 模式默认值 ═══ */
#define USB_MODE_DEFAULT_DEVICE   0
#define USB_MODE_DEFAULT_HOST     1
#define USB_MODE_DEFAULT          USB_MODE_DEFAULT_DEVICE

/* ═══ 升级重试次数 ═══ */
#define UPGRADE_MAX_RETRIES       3

#endif /* __PLAT_CONFIG_H */
