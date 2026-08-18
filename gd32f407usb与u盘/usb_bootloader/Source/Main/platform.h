/**
 * @file platform.h
 * @brief Bootloader 平台抽象层（F103/F407 共用）
 *
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
  #include "stm32f10x.h"
  #include "stm32f10x_conf.h"

  /* F103 标准库已有 u8/u16/u32，这里保持一致性 */
  typedef uint8_t  u8;
  typedef uint16_t u16;
  typedef uint32_t u32;

  #define STACK_VALID_MASK    0x2FFF0000u   /* SRAM 64KB: 0x20000000 ~ 0x20010000 */
  #define STACK_VALID_BASE    0x20000000u

#elif defined(CHIP_SERIES_F407)
  #include "stm32f4xx.h"
  #include "stm32f4xx_conf.h"

  typedef uint8_t  u8;
  typedef uint16_t u16;
  typedef uint32_t u32;

  #define STACK_VALID_MASK    0x2FFC0000u   /* SRAM 128KB: 0x20000000 ~ 0x20020000 */
  #define STACK_VALID_BASE    0x20000000u

#else
  #error "Unsupported chip series"
#endif

/* ═══════════════════════════════════════════════════════════
 * GPIO 初始化便捷宏（消除 F103/F407 API 差异）
 * 用于 spi_flash.c / usart.c 中配置普通 GPIO 或 AF。
 * ═══════════════════════════════════════════════════════════ */

#if defined(CHIP_SERIES_F103)

  #define GPIO_AF_INIT_OUT_PP(gpio, pin, speed)  do { \
      GPIO_InitTypeDef _init; \
      _init.GPIO_Pin   = (pin); \
      _init.GPIO_Mode  = GPIO_Mode_Out_PP; \
      _init.GPIO_Speed = (speed); \
      GPIO_Init((gpio), &_init); \
  } while(0)

  /* F103 无独立 AF 映射，source/af 参数被忽略；保持与 F407 一致的 5 参数签名 */
  #define GPIO_AF_INIT_AF_PP(gpio, pin, src, af, speed)  do { \
      GPIO_InitTypeDef _init; \
      _init.GPIO_Pin   = (pin); \
      _init.GPIO_Mode  = GPIO_Mode_AF_PP; \
      _init.GPIO_Speed = (speed); \
      GPIO_Init((gpio), &_init); \
      (void)(src); (void)(af); \
  } while(0)

  #define GPIO_AF_INIT_IPU(gpio, pin)  do { \
      GPIO_InitTypeDef _init; \
      _init.GPIO_Pin   = (pin); \
      _init.GPIO_Mode  = GPIO_Mode_IPU; \
      GPIO_Init((gpio), &_init); \
  } while(0)

  #define GPIO_AF_CONFIG(port, pin_source, af)  ((void)0)  /* F103 无 AF 映射 */

#elif defined(CHIP_SERIES_F407)

  #define GPIO_AF_INIT_OUT_PP(gpio, pin, speed)  do { \
      GPIO_InitTypeDef _init; \
      _init.GPIO_Pin   = (pin); \
      _init.GPIO_Mode  = GPIO_Mode_OUT; \
      _init.GPIO_OType = GPIO_OType_PP; \
      _init.GPIO_Speed = (speed); \
      _init.GPIO_PuPd  = GPIO_PuPd_NOPULL; \
      GPIO_Init((gpio), &_init); \
  } while(0)

  #define GPIO_AF_INIT_AF_PP(gpio, pin, source, af, speed)  do { \
      GPIO_PinAFConfig((gpio), (source), (af)); \
      GPIO_InitTypeDef _init; \
      _init.GPIO_Pin   = (pin); \
      _init.GPIO_Mode  = GPIO_Mode_AF; \
      _init.GPIO_OType = GPIO_OType_PP; \
      _init.GPIO_Speed = (speed); \
      _init.GPIO_PuPd  = GPIO_PuPd_UP; \
      GPIO_Init((gpio), &_init); \
  } while(0)

  #define GPIO_AF_INIT_IPU(gpio, pin)  do { \
      GPIO_InitTypeDef _init; \
      _init.GPIO_Pin   = (pin); \
      _init.GPIO_Mode  = GPIO_Mode_IN; \
      _init.GPIO_PuPd  = GPIO_PuPd_UP; \
      GPIO_Init((gpio), &_init); \
  } while(0)

#endif

/* ═══════════════════════════════════════════════════════════
 * 系统时钟/定时器常数
 * ═══════════════════════════════════════════════════════════ */
#if defined(CHIP_SERIES_F103)
  #define SYSTEM_HCLK_HZ        72000000u
  #define APB1_TIMER_HZ         72000000u   /* F103 APB1 timer = HCLK */
  #define SYSTICK_HCLK_HZ       72000000u
#elif defined(CHIP_SERIES_F407)
  #define SYSTEM_HCLK_HZ        168000000u
  #define APB1_TIMER_HZ         84000000u   /* F407 APB1 = 42MHz, timer = 2x */
  #define SYSTICK_HCLK_HZ       168000000u
#endif

/* ═══════════════════════════════════════════════════════════
 * 调试串口配置
 *   F103: USART1 (PA9/PA10)
 *   F407: USART2 (PA2/PA3)  — 与 APP 的 DBG_PRINTF 共用同一串口
 * ═══════════════════════════════════════════════════════════ */

#if defined(CHIP_SERIES_F103)
  #define BOOTLOADER_USART        USART1
  #define BOOTLOADER_USART_BAUD   115200
  #define BOOTLOADER_USART_RCC_GPIO   RCC_APB2Periph_GPIOA
  #define BOOTLOADER_USART_RCC_UART   RCC_APB2Periph_USART1
  #define BOOTLOADER_USART_GPIO_CLK_CMD   RCC_APB2PeriphClockCmd
  #define BOOTLOADER_USART_UART_CLK_CMD   RCC_APB2PeriphClockCmd
  #define BOOTLOADER_USART_TX_PIN     GPIO_Pin_9
  #define BOOTLOADER_USART_RX_PIN     GPIO_Pin_10
  #define BOOTLOADER_USART_TX_PORT    GPIOA
  #define BOOTLOADER_USART_RX_PORT    GPIOA
  #define BOOTLOADER_USART_TX_SOURCE  GPIO_PinSource9
  #define BOOTLOADER_USART_RX_SOURCE  GPIO_PinSource10
  #define BOOTLOADER_USART_TX_AF      0
  #define BOOTLOADER_USART_RX_AF      0
#elif defined(CHIP_SERIES_F407)
  #define BOOTLOADER_USART        USART2
  #define BOOTLOADER_USART_BAUD   115200
  #define BOOTLOADER_USART_RCC_GPIO   RCC_AHB1Periph_GPIOA
  #define BOOTLOADER_USART_RCC_UART   RCC_APB1Periph_USART2
  #define BOOTLOADER_USART_GPIO_CLK_CMD   RCC_AHB1PeriphClockCmd
  #define BOOTLOADER_USART_UART_CLK_CMD   RCC_APB1PeriphClockCmd
  #define BOOTLOADER_USART_TX_PIN     GPIO_Pin_2
  #define BOOTLOADER_USART_RX_PIN     GPIO_Pin_3
  #define BOOTLOADER_USART_TX_PORT    GPIOA
  #define BOOTLOADER_USART_RX_PORT    GPIOA
  #define BOOTLOADER_USART_TX_SOURCE  GPIO_PinSource2
  #define BOOTLOADER_USART_RX_SOURCE  GPIO_PinSource3
  #define BOOTLOADER_USART_TX_AF      GPIO_AF_USART2
  #define BOOTLOADER_USART_RX_AF      GPIO_AF_USART2
#endif

#endif /* __PLATFORM_H__ */
