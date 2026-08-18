#ifndef __USART_H__
#define __USART_H__

#include "stm32f10x.h"
#define USART1_RXBUF_SIZE	256
#define USART1_BAUD	115200


void UsartInit(void);
void UsartSendByte(USART_TypeDef* USARTx,u8 ch);
void UsartSendString(USART_TypeDef* USARTx,u8 * str);
void USART1_IRQHandler(void);

#endif

