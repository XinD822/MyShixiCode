/**
 * @file hal_uart.h
 * @brief 串口 HAL 接口
 */

#ifndef __HAL_UART_H
#define __HAL_UART_H

#include <stdint.h>

typedef struct {
    void (*init)(uint32_t baudrate);
    void (*send_byte)(uint8_t ch);
    void (*send_string)(const char *str);
} HAL_Uart_Drv_t;

extern const HAL_Uart_Drv_t *HAL_Uart;

#endif /* __HAL_UART_H */
