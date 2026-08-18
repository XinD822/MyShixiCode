/**
 * @file board_config.h
 * @brief 板级引脚配置
 *
 * 换板子时只改这个文件，HAL 接口和业务代码不动。
 */

#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

/* ═══ SPI Flash 引脚（软件 SPI） ═══ */
#define FLASH_CS_PORT       GPIOB
#define FLASH_CS_PIN        GPIO_Pin_12
#define FLASH_CS_CLK        RCC_APB2Periph_GPIOB

#define FLASH_SCK_PORT      GPIOB
#define FLASH_SCK_PIN       GPIO_Pin_13
#define FLASH_SCK_CLK       RCC_APB2Periph_GPIOB

#define FLASH_MISO_PORT     GPIOB
#define FLASH_MISO_PIN      GPIO_Pin_14
#define FLASH_MISO_CLK      RCC_APB2Periph_GPIOB

#define FLASH_MOSI_PORT     GPIOB
#define FLASH_MOSI_PIN      GPIO_Pin_15
#define FLASH_MOSI_CLK      RCC_APB2Periph_GPIOB

/* ═══ SPI Flash GPIO 操作宏 ═══ */
#define FLASH_CS_LOW()      GPIO_ResetBits(FLASH_CS_PORT, FLASH_CS_PIN)
#define FLASH_CS_HIGH()     GPIO_SetBits(FLASH_CS_PORT, FLASH_CS_PIN)
#define FLASH_SCK_LOW()     GPIO_ResetBits(FLASH_SCK_PORT, FLASH_SCK_PIN)
#define FLASH_SCK_HIGH()    GPIO_SetBits(FLASH_SCK_PORT, FLASH_SCK_PIN)
#define FLASH_MOSI_LOW()    GPIO_ResetBits(FLASH_MOSI_PORT, FLASH_MOSI_PIN)
#define FLASH_MOSI_HIGH()   GPIO_SetBits(FLASH_MOSI_PORT, FLASH_MOSI_PIN)
#define FLASH_MISO_READ()   GPIO_ReadInputDataBit(FLASH_MISO_PORT, FLASH_MISO_PIN)

/* ═══════════════════════════════════════════════════════════
 * 定时器选择（用于 Tick 计时）
 * ═══════════════════════════════════════════════════════════
 *
 * 三种模式（三选一）：
 *
 * 模式 A：使用模块自带定时器（默认 TIM2）
 *   - 新工程推荐，开箱即用
 *   - 改 TICK_TIM_xxx 宏可切换到其他定时器
 *
 * 模式 B：外部 Tick 注入（定义 TICK_EXTERNAL）
 *   - 老工程已有 SysTick/TIM 驱动，不想额外开定时器
 *   - 开发者只需在自己的定时器中断里调用 USB_Upgrade_TickInc()
 *   - 模块不再初始化任何定时器，也不定义 IRQHandler
 */

/* --- 默认模式：使用模块自带定时器 --- */
/* 如果有冲突，改成 TIM3/TIM4/TIM5 等 */
#define TICK_TIM                TIM2
#define TICK_TIM_IRQn           TIM2_IRQn
#define TICK_TIM_IRQHandler     TIM2_IRQHandler
#define TICK_TIM_RCC            RCC_APB1Periph_TIM2
#define TICK_TIM_PREEMPTION     2
#define TICK_TIM_SUBPRIORITY    2

/* --- 外部 Tick 注入模式（取消注释则启用） --- */
/* 启用后上面的 TICK_TIM_xxx 全部失效 */
// #define TICK_EXTERNAL

/* ═══════════════════════════════════════════════════════════
 * 调试串口（可选）
 * ═══════════════════════════════════════════════════════════
 *
 * 定义 USB_UPGRADE_USE_UART 则启用调试串口（printf 输出）
 * 注释掉则完全不使用串口，不占用任何 USART 资源
 */
// #define USB_UPGRADE_USE_UART

#ifdef USB_UPGRADE_USE_UART
#define DEBUG_UART              USART1
#define DEBUG_UART_IRQn         USART1_IRQn
#define DEBUG_UART_IRQHandler   USART1_IRQHandler
#define DEBUG_UART_RCC          RCC_APB2Periph_USART1
#define DEBUG_UART_RCC_GPIO     RCC_APB2Periph_GPIOA
#define DEBUG_UART_TX_PORT      GPIOA
#define DEBUG_UART_TX_PIN       GPIO_Pin_9
#define DEBUG_UART_RX_PORT      GPIOA
#define DEBUG_UART_RX_PIN       GPIO_Pin_10
#define DEBUG_UART_PREEMPTION   1
#define DEBUG_UART_SUBPRIORITY  2
#define USART_BAUDRATE          115200
/* printf 重定向（定义则本模块实现 fputc，注释掉则不实现） */
#define USB_UPGRADE_IMPLEMENT_FPUTC
#endif

/* ═══════════════════════════════════════════════════════════
 * LED 指示灯（可选）
 * ═══════════════════════════════════════════════════════════
 *
 * 定义 USB_UPGRADE_USE_LED 则启用 LED 驱动
 * 注释掉则完全不使用 LED，不占用任何 GPIO 资源
 */
// #define USB_UPGRADE_USE_LED

#ifdef USB_UPGRADE_USE_LED
#define LED0_PORT           GPIOB
#define LED0_PIN            GPIO_Pin_5
#define LED1_PORT           GPIOE
#define LED1_PIN            GPIO_Pin_5
#define LED_ID_0            0
#define LED_ID_1            1
#endif

/* ═══ 兼容旧接口控制 ═══ */
/* 定义此宏则导出 Delay_*, W25QXX_* 等兼容函数 */
#define USB_UPGRADE_COMPAT_FUNCTIONS

/* ═══ 主频配置 ═══ */
#define SYSTEM_CORE_CLOCK       72      /* 单位 MHz，用于 SysTick 延时计算 */

#endif /* __BOARD_CONFIG_H */
