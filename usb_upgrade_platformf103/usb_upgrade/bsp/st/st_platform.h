/**
 * @file st_platform.h
 * @brief STM32 平台通用头文件（支持 F103/F407）
 */

#ifndef __ST_PLATFORM_H
#define __ST_PLATFORM_H

#include <stdint.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════
 * 根据芯片系列包含不同的头文件
 * ═══════════════════════════════════════════════════════════ */
#if defined(CHIP_SERIES_F103)
  #include "stm32f10x.h"
  #include "stm32f10x_conf.h"
  /* F103 MCU 唯一 ID 地址 */
  #define MCU_ID1  (*(uint32_t *)0x1FFFF7E8)
  #define MCU_ID2  (*(uint32_t *)0x1FFFF7EC)
  #define MCU_ID3  (*(uint32_t *)0x1FFFF7F0)
#elif defined(CHIP_SERIES_F407)
  #include "stm32f4xx.h"
  #include "stm32f4xx_conf.h"
  /* F407 MCU 唯一 ID 地址 */
  #define MCU_ID1  (*(uint32_t *)0x1FFF7A10)
  #define MCU_ID2  (*(uint32_t *)0x1FFF7A14)
  #define MCU_ID3  (*(uint32_t *)0x1FFF7A18)
#else
  #error "Please define CHIP_SERIES_F103 or CHIP_SERIES_F407"
#endif

#endif /* __ST_PLATFORM_H */
