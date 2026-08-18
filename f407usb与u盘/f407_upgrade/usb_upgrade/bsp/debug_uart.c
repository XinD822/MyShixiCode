/**
 * @file debug_uart.c
 * @brief 调试串口驱动实现
 *
 * 从原 st_uart.c 迁移，移除函数指针封装层。
 */

#include "board_config.h"

#ifdef USB_UPGRADE_USE_UART

#include "debug_uart.h"

void Debug_UART_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

#if defined(CHIP_SERIES_F103)
    RCC_APB2PeriphClockCmd(DEBUG_UART_RCC_GPIO | DEBUG_UART_RCC, ENABLE);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Pin = DEBUG_UART_TX_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DEBUG_UART_TX_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Pin = DEBUG_UART_RX_PIN;
    GPIO_Init(DEBUG_UART_RX_PORT, &GPIO_InitStruct);
#elif defined(CHIP_SERIES_F407)
    RCC_AHB1PeriphClockCmd(DEBUG_UART_RCC_GPIO, ENABLE);
    RCC_APB1PeriphClockCmd(DEBUG_UART_RCC, ENABLE);
    GPIO_PinAFConfig(DEBUG_UART_TX_PORT, DEBUG_UART_TX_PIN_SRC, DEBUG_UART_TX_AF);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Pin = DEBUG_UART_TX_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(DEBUG_UART_TX_PORT, &GPIO_InitStruct);
    GPIO_PinAFConfig(DEBUG_UART_RX_PORT, DEBUG_UART_RX_PIN_SRC, DEBUG_UART_RX_AF);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Pin = DEBUG_UART_RX_PIN;
    GPIO_Init(DEBUG_UART_RX_PORT, &GPIO_InitStruct);
#endif

    USART_InitStruct.USART_BaudRate = baudrate;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(DEBUG_UART, &USART_InitStruct);

    NVIC_InitStruct.NVIC_IRQChannel = DEBUG_UART_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = DEBUG_UART_PREEMPTION;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = DEBUG_UART_SUBPRIORITY;
    NVIC_Init(&NVIC_InitStruct);

    USART_ITConfig(DEBUG_UART, USART_IT_RXNE, ENABLE);
    USART_Cmd(DEBUG_UART, ENABLE);
    USART_ClearFlag(DEBUG_UART, USART_FLAG_TC);
}

void Debug_UART_SendByte(uint8_t ch)
{
    USART_SendData(DEBUG_UART, (uint16_t)ch);
    while (USART_GetFlagStatus(DEBUG_UART, USART_FLAG_TC) == RESET);
}

void Debug_UART_SendString(const char *str)
{
    while (*str) {
        Debug_UART_SendByte(*str++);
    }
}

/* ──── 中断处理（函数名由宏决定） ──── */
void DEBUG_UART_IRQHandler(void)
{
    if (USART_GetITStatus(DEBUG_UART, USART_IT_RXNE) == SET) {
        (void)USART_ReceiveData(DEBUG_UART);
        USART_ClearITPendingBit(DEBUG_UART, USART_IT_RXNE);
    }
}

#endif /* USB_UPGRADE_USE_UART */
