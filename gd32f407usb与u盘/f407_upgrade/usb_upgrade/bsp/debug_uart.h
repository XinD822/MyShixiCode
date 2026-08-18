/**
 * @file debug_uart.h
 * @brief 调试串口驱动接口
 */

#ifndef __DEBUG_UART_H
#define __DEBUG_UART_H

#include <stdint.h>

void Debug_UART_Init(uint32_t baudrate);
void Debug_UART_SendByte(uint8_t ch);
void Debug_UART_SendString(const char *str);

#endif /* __DEBUG_UART_H */
