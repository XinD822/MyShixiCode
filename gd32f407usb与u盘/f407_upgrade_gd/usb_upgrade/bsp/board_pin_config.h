/**
 * @file board_pin_config.h
 * @brief 板级引脚配置 — 移植到新设备时，改这一个文件就够了
 */
#ifndef __BOARD_PIN_CONFIG_H
#define __BOARD_PIN_CONFIG_H

#include "plat_config.h"

/* ═══ 1. SPI Flash 引脚（W25Q128 软件 SPI） ═══ */
/* 正点原子 F407：CS=PB14, SCK=PB3, MISO=PB4, MOSI=PB5 */
#define FLASH_CS_PORT       GPIOB
#define FLASH_CS_PIN        GPIO_PIN_14
#define FLASH_CS_CLK        RCU_GPIOB

#define FLASH_SCK_PORT      GPIOB
#define FLASH_SCK_PIN       GPIO_PIN_3
#define FLASH_SCK_CLK       RCU_GPIOB

#define FLASH_MISO_PORT     GPIOB
#define FLASH_MISO_PIN      GPIO_PIN_4
#define FLASH_MISO_CLK      RCU_GPIOB

#define FLASH_MOSI_PORT     GPIOB
#define FLASH_MOSI_PIN      GPIO_PIN_5
#define FLASH_MOSI_CLK      RCU_GPIOB

#define FLASH_CS_LOW()      gpio_bit_reset(FLASH_CS_PORT, FLASH_CS_PIN)
#define FLASH_CS_HIGH()     gpio_bit_set(FLASH_CS_PORT, FLASH_CS_PIN)
#define FLASH_SCK_LOW()     gpio_bit_reset(FLASH_SCK_PORT, FLASH_SCK_PIN)
#define FLASH_SCK_HIGH()    gpio_bit_set(FLASH_SCK_PORT, FLASH_SCK_PIN)
#define FLASH_MOSI_LOW()    gpio_bit_reset(FLASH_MOSI_PORT, FLASH_MOSI_PIN)
#define FLASH_MOSI_HIGH()   gpio_bit_set(FLASH_MOSI_PORT, FLASH_MOSI_PIN)
#define FLASH_MISO_READ()   gpio_input_bit_get(FLASH_MISO_PORT, FLASH_MISO_PIN)

#define FLASH_USE_HW_SPI    0
#if FLASH_USE_HW_SPI
  #define FLASH_SPI_PERIPH    SPI0
  #define FLASH_SPI_RCC       RCU_SPI0
#endif

/* Flash 芯片配置 */
#define FLASH_DRIVER_W25Q128   1       /* 选中 W25Q128 驱动 */
#define FLASH_NAME              "W25Q128"
#define FLASH_CAPACITY          (16 * 1024 * 1024)
#define FLASH_ID_EXPECT         0x5217u

/* ═══ 2. USB 引脚配置 ═══ */
/* GD32F407 USBFS：DM=PA11, DP=PA12, AF=10 */
#define USB_DM_PORT         GPIOA
#define USB_DM_PIN          GPIO_PIN_11
#define USB_DP_PORT         GPIOA
#define USB_DP_PIN          GPIO_PIN_12
#define USB_GPIO_AF         GPIO_AF_10
#define USB_GPIO_CLK        RCU_GPIOA

#define USB_PERIPH_CLK      RCU_USBFS
#define USB_PERIPH_RST      RCU_USBFSRST
#define USB_IRQn            USBFS_IRQn
#define USB_IRQ_PRIORITY    0
#define USB_IRQ_SUBPRIORITY 3

#define USB_VBUS_CTRL_ENABLE  0
#if USB_VBUS_CTRL_ENABLE
  #define USB_VBUS_PORT     GPIOA
  #define USB_VBUS_PIN      GPIO_PIN_15
  #define USB_VBUS_CLK      RCU_GPIOA
  #define USB_VBUS_ON()     gpio_bit_set(USB_VBUS_PORT, USB_VBUS_PIN)
  #define USB_VBUS_OFF()    gpio_bit_reset(USB_VBUS_PORT, USB_VBUS_PIN)
#endif

/* ═══ 3. 调试串口引脚 ═══ */
/* GD32 USART1：TX=PA2, RX=PA3, AF=7 */
#define DEBUG_UART_ENABLE   1
#if DEBUG_UART_ENABLE
  #define DEBUG_UART              USART1
  #define DEBUG_UART_RCC          RCU_USART1
  #define DEBUG_UART_IRQn         USART1_IRQn
  #define DEBUG_UART_TX_PORT      GPIOA
  #define DEBUG_UART_TX_PIN       GPIO_PIN_2
  #define DEBUG_UART_RX_PORT      GPIOA
  #define DEBUG_UART_RX_PIN       GPIO_PIN_3
  #define DEBUG_UART_TX_AF        GPIO_AF_7
  #define DEBUG_UART_RX_AF        GPIO_AF_7
  #define DEBUG_UART_RCC_GPIO     RCU_GPIOA
  #define USART_BAUDRATE          115200
#endif

/* ═══ 4. 模式切换按钮引脚 ═══ */
/* 正点原子 F407：KEY0 = PE4（低电平有效） */
#define MODE_BTN_PORT       GPIOE
#define MODE_BTN_PIN        GPIO_PIN_4
#define MODE_BTN_CLK        RCU_GPIOE
#define MODE_BTN_ACTIVE_LOW 1

/* ═══ 5. LED 状态指示引脚（可选） ═══ */
/* 正点原子 F407：LED0 = PF9, LED1 = PF10（低电平有效） */
#define LED_ENABLE          1
#if LED_ENABLE
  #define LED0_PORT           GPIOF
  #define LED0_PIN            GPIO_PIN_9
  #define LED0_CLK            RCU_GPIOF
  #define LED0_ACTIVE_LOW     1
  #define LED1_PORT           GPIOF
  #define LED1_PIN            GPIO_PIN_10
  #define LED1_CLK            RCU_GPIOF
  #define LED1_ACTIVE_LOW     1
#endif

/* ═══ 6. 外部定时器引脚（仅 PLAT_TIMER_EXTERNAL 模式） ═══ */
#if (PLAT_TIMER_SOURCE == PLAT_TIMER_EXTERNAL)
  #define EXT_TIM_PERIPH      TIMER2
  #define EXT_TIM_IRQn        TIMER2_IRQn
  #define EXT_TIM_RCC         RCU_TIMER2
  #define EXT_TIM_IRQHandler  TIMER2_IRQHandler
  #define EXT_TIM_CLK_HZ      84000000u
#endif

/* ═══ 7. 本工程自带 tick 定时器（仅 PLAT_TIMER_INTERNAL 模式） ═══
 * 独立运行/调试时用本工程自带 TIMER2 做 1ms tick 中断；
 * 移植到目标设备且该设备已有定时器时，改用 PLAT_TIMER_EXTERNAL。 */
#if (PLAT_TIMER_SOURCE == PLAT_TIMER_INTERNAL)
  #define TICK_TIM_PERIPH      TIMER2
  #define TICK_TIM_IRQn        TIMER2_IRQn
  #define TICK_TIM_RCC         RCU_TIMER2
  #define TICK_TIM_IRQHandler  TIMER2_IRQHandler
  #define TICK_TIM_CLK_HZ      84000000u
#endif

#endif /* __BOARD_PIN_CONFIG_H */
