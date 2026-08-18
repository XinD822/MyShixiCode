/**
 * @file st_uart.c
 * @brief STM32 USART 驱动实现（可配置串口，可完全禁用）
 */

#include "hal_config.h"
#include "board_config.h"

#ifdef PLATFORM_STM32
#ifdef USB_UPGRADE_USE_UART

/* ──── HAL 接口实现 ──── */

static void st_uart_init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    RCC_APB2PeriphClockCmd(DEBUG_UART_RCC_GPIO | DEBUG_UART_RCC, ENABLE);

    /* TX */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Pin = DEBUG_UART_TX_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DEBUG_UART_TX_PORT, &GPIO_InitStruct);

    /* RX */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Pin = DEBUG_UART_RX_PIN;
    GPIO_Init(DEBUG_UART_RX_PORT, &GPIO_InitStruct);

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

static void st_uart_send_byte(uint8_t ch)
{
    USART_SendData(DEBUG_UART, (uint16_t)ch);
    while (USART_GetFlagStatus(DEBUG_UART, USART_FLAG_TC) == RESET);
}

static void st_uart_send_string(const char *str)
{
    while (*str) {
        st_uart_send_byte(*str++);
    }
}

const HAL_Uart_Drv_t ST_Uart_Drv = {
    .init       = st_uart_init,
    .send_byte  = st_uart_send_byte,
    .send_string = st_uart_send_string,
};

const HAL_Uart_Drv_t *HAL_Uart = &ST_Uart_Drv;

/* ──── 中断处理（函数名由宏决定） ──── */

void DEBUG_UART_IRQHandler(void)
{
    if (USART_GetITStatus(DEBUG_UART, USART_IT_RXNE) == SET) {
        (void)USART_ReceiveData(DEBUG_UART);
        USART_ClearITPendingBit(DEBUG_UART, USART_IT_RXNE);
    }
}

/* ──── printf 重定向（可选） ──── */

#ifdef USB_UPGRADE_IMPLEMENT_FPUTC
int fputc(int ch, FILE *f)
{
    (void)f;
    st_uart_send_byte((uint8_t)ch);
    return ch;
}
#endif

/* ──── 兼容旧接口（可选） ──── */

#ifdef USB_UPGRADE_COMPAT_FUNCTIONS
void UsartInit(void)                    { st_uart_init(115200); }
void UsartSendByte(USART_TypeDef *x, uint8_t ch) { st_uart_send_byte(ch); (void)x; }
void UsartSendString(USART_TypeDef *x, uint8_t *str) { st_uart_send_string((const char *)str); (void)x; }
#endif

#endif /* USB_UPGRADE_USE_UART */
#endif /* PLATFORM_STM32 */
