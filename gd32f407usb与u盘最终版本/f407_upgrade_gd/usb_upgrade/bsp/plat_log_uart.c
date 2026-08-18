/**
 * @file plat_log_uart.c
 * @brief UART 日志实现 — 引用 board_pin_config.h
 */
#include "plat_log.h"
#include "board_pin_config.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if DEBUG_UART_ENABLE

static void uart_init(void)
{
    static uint8_t inited = 0;
    if (inited) return;
    inited = 1;

    rcu_periph_clock_enable(DEBUG_UART_RCC_GPIO);
    rcu_periph_clock_enable(DEBUG_UART_RCC);

    gpio_af_set(DEBUG_UART_TX_PORT, DEBUG_UART_TX_AF, DEBUG_UART_TX_PIN);
    gpio_af_set(DEBUG_UART_RX_PORT, DEBUG_UART_RX_AF, DEBUG_UART_RX_PIN);
    gpio_mode_set(DEBUG_UART_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DEBUG_UART_TX_PIN);
    gpio_mode_set(DEBUG_UART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DEBUG_UART_RX_PIN);
    gpio_output_options_set(DEBUG_UART_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, DEBUG_UART_TX_PIN);

    usart_deinit(DEBUG_UART);
    usart_baudrate_set(DEBUG_UART, USART_BAUDRATE);
    usart_receive_config(DEBUG_UART, USART_RECEIVE_ENABLE);
    usart_transmit_config(DEBUG_UART, USART_TRANSMIT_ENABLE);
    usart_enable(DEBUG_UART);
}

void plat_log(plat_log_level_t lv, const char *file, int line, const char *fmt, ...)
{
    uart_init();

    char buf[256];
    int len = 0;

    /* PLAT_LOG_LEVEL 是常量，lv 是枚举（有符号 int），比较安全 */
    if ((int)lv >= (int)PLAT_LOG_LEVEL) {
        va_list ap;
        va_start(ap, fmt);
        len = vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        if (len < 0) len = 0;
        if ((uint32_t)len >= sizeof(buf)) len = sizeof(buf) - 1;
    }

    uint32_t i;
    for (i = 0; i < (uint32_t)len; i++) {
        while (RESET == usart_flag_get(DEBUG_UART, USART_FLAG_TBE));
        usart_data_transmit(DEBUG_UART, buf[i]);
    }
}

#else /* DEBUG_UART_ENABLE = 0 */

void plat_log(plat_log_level_t lv, const char *file, int line, const char *fmt, ...)
{
    (void)lv; (void)file; (void)line; (void)fmt;
}

#endif /* DEBUG_UART_ENABLE */
