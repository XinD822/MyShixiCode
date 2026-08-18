/**
 * @file platform.h
 * @brief Bootloader 平台抽象层（GD32F407）
 *
 * 由 STM32F407 版移植而来：标准外设库 ST → GD32F4xx 标准外设库。
 * 统一包含头文件、提供同名的 GPIO 初始化宏、Flash API 别名等，
 * 让业务代码（main.c/TIM2/SPI/USART）与具体芯片解耦。
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "chip_select.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* ═══════════════════════════════════════════════════════════
 * 芯片相关头文件与基本类型
 * ═══════════════════════════════════════════════════════════ */
#if defined(CHIP_SERIES_F103)
  #error "GD32F407 bootloader: CHIP_SERIES_F103 is not supported"
#elif defined(CHIP_SERIES_F407)
  /* GD32F4xx 标准外设库（gd32f4xx.h 会自动包含 gd32f4xx_libopt.h） */
  #include "gd32f4xx.h"

  typedef uint8_t  u8;
  typedef uint16_t u16;
  typedef uint32_t u32;

  /* GD32F407ZG: SRAM 192KB @ 0x20000000 ~ 0x2002FFFF */
  #define STACK_VALID_MASK    0x2FFC0000u   /* 覆盖 256KB 区间的 TOP10 位 */
  #define STACK_VALID_BASE    0x20000000u

#else
  #error "Unsupported chip series"
#endif

/* ═══════════════════════════════════════════════════════════
 * GPIO 初始化便捷宏（GD32F4xx API）
 * 用于 spi_flash.c / usart.c 中配置普通 GPIO 或 AF。
 * ═══════════════════════════════════════════════════════════ */

#if defined(CHIP_SERIES_F103)
  /* 保留 F103 占位，本工程不允许编译 */
#elif defined(CHIP_SERIES_F407)

  #define GPIO_AF_INIT_OUT_PP(gpio, pin, speed)  do { \
      gpio_mode_set((gpio), GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, (pin)); \
      gpio_output_options_set((gpio), GPIO_OTYPE_PP, (speed), (pin)); \
      gpio_bit_set((gpio), (pin)); \
  } while(0)

  #define GPIO_AF_INIT_AF_PP(gpio, pin, source, af, speed)  do { \
      gpio_af_set((gpio), (af), (source)); \
      gpio_mode_set((gpio), GPIO_MODE_AF, GPIO_PUPD_PULLUP, (pin)); \
      gpio_output_options_set((gpio), GPIO_OTYPE_PP, (speed), (pin)); \
  } while(0)

  #define GPIO_AF_INIT_IPU(gpio, pin)  do { \
      gpio_mode_set((gpio), GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, (pin)); \
  } while(0)

#endif

/* ═══════════════════════════════════════════════════════════
 * 系统时钟/定时器常数
 *   GD32F407: HCLK 168MHz, APB1 42MHz, 定时器时钟 2x = 84MHz
 * ═══════════════════════════════════════════════════════════ */
#if defined(CHIP_SERIES_F407)
  #define SYSTEM_HCLK_HZ        168000000u
  #define APB1_TIMER_HZ         84000000u
  #define SYSTICK_HCLK_HZ       168000000u
#endif

/* ═══════════════════════════════════════════════════════════
 * 调试串口配置
 *   F407: USART2 (PA2/PA3)  → GD32: USART1 (PA2/PA3), AF7
 *   与 APP 的 DBG_PRINTF 共用同一串口
 * ═══════════════════════════════════════════════════════════ */

#if defined(CHIP_SERIES_F407)
  #define BOOTLOADER_USART        USART1
  #define BOOTLOADER_USART_BAUD   115200
  #define BOOTLOADER_USART_RCC_GPIO   RCU_GPIOA
  #define BOOTLOADER_USART_RCC_UART   RCU_USART1
  #define BOOTLOADER_USART_TX_PIN     GPIO_PIN_2
  #define BOOTLOADER_USART_RX_PIN     GPIO_PIN_3
  #define BOOTLOADER_USART_TX_PORT    GPIOA
  #define BOOTLOADER_USART_RX_PORT    GPIOA
  #define BOOTLOADER_USART_TX_SOURCE  GPIO_PIN_2
  #define BOOTLOADER_USART_RX_SOURCE  GPIO_PIN_3
  #define BOOTLOADER_USART_TX_AF      GPIO_AF_7
  #define BOOTLOADER_USART_RX_AF      GPIO_AF_7
#endif

#endif /* __PLATFORM_H__ */
