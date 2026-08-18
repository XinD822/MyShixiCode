/**
 * @file board_config.h
 * @brief 板级统一配置 — 唯一需要改的配置文件
 *
 * 合并了原 hal_config.h + board_config.h + st_platform.h。
 * 移植时只改这一个文件（或用 Keil Define 覆盖）。
 */

#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#include <stdint.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════
 * 1. 芯片系列选择
 *    Keil C/C++ Define 优先；都没定义时默认 F407
 * ═══════════════════════════════════════════════════════════ */
#if defined(CHIP_SERIES_F103) && defined(CHIP_SERIES_F407)
  #error "Only one of CHIP_SERIES_F103 / CHIP_SERIES_F407 can be defined"
#endif
#if !defined(CHIP_SERIES_F103) && !defined(CHIP_SERIES_F407)
  #define CHIP_SERIES_F407
#endif

/* ──── 平台头文件自动包含 ──── */
#if defined(CHIP_SERIES_F103)
  #include "stm32f10x.h"
  #include "stm32f10x_conf.h"
  #define MCU_ID1  (*(uint32_t *)0x1FFFF7E8)
  #define MCU_ID2  (*(uint32_t *)0x1FFFF7EC)
  #define MCU_ID3  (*(uint32_t *)0x1FFFF7F0)
#elif defined(CHIP_SERIES_F407)
  #include "stm32f4xx.h"
  #include "stm32f4xx_conf.h"
  #define MCU_ID1  (*(uint32_t *)0x1FFF7A10)
  #define MCU_ID2  (*(uint32_t *)0x1FFF7A14)
  #define MCU_ID3  (*(uint32_t *)0x1FFF7A18)
#endif

/* ═══════════════════════════════════════════════════════════
 * 2. RTOS 选择
 * ═══════════════════════════════════════════════════════════ */
#define USE_BAREMETAL
/* #define USE_UCOS2 */

#ifdef USE_UCOS2
#define CONFIG_USB_USE_UCOS2
#endif

/* ═══════════════════════════════════════════════════════════
 * 3. 主频配置（MHz）
 * ═══════════════════════════════════════════════════════════ */
#if defined(CHIP_SERIES_F103)
  #define SYSTEM_CORE_CLOCK   72
#elif defined(CHIP_SERIES_F407)
  #define SYSTEM_CORE_CLOCK   168
#endif

/* ═══════════════════════════════════════════════════════════
 * 4. USB 模式选择（上电决定 device / host）
 *    默认 DEVICE；将来加按键切换时改这里或运行时调用
 *    USB_Mode_Set(USB_MODE_HOST)
 * ═══════════════════════════════════════════════════════════ */
#define USB_MODE_DEFAULT_DEVICE   0
#define USB_MODE_DEFAULT_HOST     1
#define USB_MODE_DEFAULT          USB_MODE_DEFAULT_DEVICE

/* USB DWC2 需要 HAL_PCD_MODULE_ENABLED */
#define HAL_PCD_MODULE_ENABLED    1

/* ═══════════════════════════════════════════════════════════
 * 5. 升级来源（可多选，1=启用，0=禁用）
 * ═══════════════════════════════════════════════════════════ */
#define UPGRADE_SRC_USB_DRAG    1
#define UPGRADE_SRC_SD_CARD     0
#define UPGRADE_SRC_USB_DRIVE   1

/* ═══════════════════════════════════════════════════════════
 * 6. SPI Flash 配置
 * ═══════════════════════════════════════════════════════════ */
#define FLASH_DRIVER_W25Q128    1
#define FLASH_SPI_SOFTWARE      1       /* 1=软件SPI, 0=硬件SPI */

#if FLASH_DRIVER_W25Q128
#define FLASH_NAME      "W25Q128"
#define FLASH_CAPACITY  (16 * 1024 * 1024)
#define FLASH_ID_EXPECT 0x5217u         /* NM25Q128 实际 ID */
#endif

/* ──── SPI Flash 引脚（软件 SPI） ──── */
/* 正点原子 F407：CS=PB14, SCK=PB3, MISO=PB4, MOSI=PB5 */
#define FLASH_CS_PORT       GPIOB
#define FLASH_CS_PIN        GPIO_Pin_14
#define FLASH_SCK_PORT      GPIOB
#define FLASH_SCK_PIN       GPIO_Pin_3
#define FLASH_MISO_PORT     GPIOB
#define FLASH_MISO_PIN      GPIO_Pin_4
#define FLASH_MOSI_PORT     GPIOB
#define FLASH_MOSI_PIN      GPIO_Pin_5

#if defined(CHIP_SERIES_F103)
  #define FLASH_CS_CLK      RCC_APB2Periph_GPIOB
  #define FLASH_SCK_CLK     RCC_APB2Periph_GPIOB
  #define FLASH_MISO_CLK    RCC_APB2Periph_GPIOB
  #define FLASH_MOSI_CLK    RCC_APB2Periph_GPIOB
#elif defined(CHIP_SERIES_F407)
  #define FLASH_CS_CLK      RCC_AHB1Periph_GPIOB
  #define FLASH_SCK_CLK     RCC_AHB1Periph_GPIOB
  #define FLASH_MISO_CLK    RCC_AHB1Periph_GPIOB
  #define FLASH_MOSI_CLK    RCC_AHB1Periph_GPIOB
#endif

#define FLASH_CS_LOW()      GPIO_ResetBits(FLASH_CS_PORT, FLASH_CS_PIN)
#define FLASH_CS_HIGH()     GPIO_SetBits(FLASH_CS_PORT, FLASH_CS_PIN)
#define FLASH_SCK_LOW()     GPIO_ResetBits(FLASH_SCK_PORT, FLASH_SCK_PIN)
#define FLASH_SCK_HIGH()    GPIO_SetBits(FLASH_SCK_PORT, FLASH_SCK_PIN)
#define FLASH_MOSI_LOW()    GPIO_ResetBits(FLASH_MOSI_PORT, FLASH_MOSI_PIN)
#define FLASH_MOSI_HIGH()   GPIO_SetBits(FLASH_MOSI_PORT, FLASH_MOSI_PIN)
#define FLASH_MISO_READ()   GPIO_ReadInputDataBit(FLASH_MISO_PORT, FLASH_MISO_PIN)

/* ═══════════════════════════════════════════════════════════
 * 7. 定时器选择（用于 1ms Tick）
 *    F103 用 TIM2，F407 用 TIM3
 * ═══════════════════════════════════════════════════════════ */
#if defined(CHIP_SERIES_F103)
  #define TICK_TIM                TIM2
  #define TICK_TIM_IRQn           TIM2_IRQn
  #define TICK_TIM_IRQHandler     TIM2_IRQHandler
  #define TICK_TIM_RCC            RCC_APB1Periph_TIM2
  #define TICK_TIM_CLK_HZ         72000000u
#elif defined(CHIP_SERIES_F407)
  #define TICK_TIM                TIM3
  #define TICK_TIM_IRQn           TIM3_IRQn
  #define TICK_TIM_IRQHandler     TIM3_IRQHandler
  #define TICK_TIM_RCC            RCC_APB1Periph_TIM3
  #define TICK_TIM_CLK_HZ         84000000u
#endif

/* ═══════════════════════════════════════════════════════════
 * 8. 调试串口（可选）
 *    默认 USART2：TX=PA2, RX=PA3
 * ═══════════════════════════════════════════════════════════ */
#define USB_UPGRADE_USE_UART

#ifdef USB_UPGRADE_USE_UART
#define DEBUG_UART              USART2
#define DEBUG_UART_IRQn         USART2_IRQn
#define DEBUG_UART_IRQHandler   USART2_IRQHandler
#define DEBUG_UART_RCC          RCC_APB1Periph_USART2
#define DEBUG_UART_TX_PORT      GPIOA
#define DEBUG_UART_TX_PIN       GPIO_Pin_2
#define DEBUG_UART_RX_PORT      GPIOA
#define DEBUG_UART_RX_PIN       GPIO_Pin_3
#define DEBUG_UART_PREEMPTION   1
#define DEBUG_UART_SUBPRIORITY  2
#define USART_BAUDRATE          115200

#if defined(CHIP_SERIES_F103)
  #define DEBUG_UART_RCC_GPIO     RCC_APB2Periph_GPIOA
  #define DEBUG_UART_TX_AF        0
  #define DEBUG_UART_RX_AF        0
  #define DEBUG_UART_TX_PIN_SRC   GPIO_PinSource2
  #define DEBUG_UART_RX_PIN_SRC   GPIO_PinSource3
#elif defined(CHIP_SERIES_F407)
  #define DEBUG_UART_RCC_GPIO     RCC_AHB1Periph_GPIOA
  #define DEBUG_UART_TX_AF        GPIO_AF_USART2
  #define DEBUG_UART_RX_AF        GPIO_AF_USART2
  #define DEBUG_UART_TX_PIN_SRC   GPIO_PinSource2
  #define DEBUG_UART_RX_PIN_SRC   GPIO_PinSource3
#endif
#endif /* USB_UPGRADE_USE_UART */

#endif /* __BOARD_CONFIG_H */
