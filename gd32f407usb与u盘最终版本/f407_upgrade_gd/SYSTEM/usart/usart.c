#include "sys.h"
#include "usart.h"
#include "gd32f4xx.h"

#if 1
#pragma import(__use_no_semihosting)
struct __FILE
{
	int handle;
};

FILE __stdout;
void _sys_exit(int x)
{
	x = x;
}
int fputc(int ch, FILE *f)
{
	while((USART_STAT0(USART0) & 0x40) == 0);
	USART_DATA(USART0) = (u8)ch;
	return ch;
}
#endif

#if EN_USART1_RX
u8 USART_RX_BUF[USART_REC_LEN];
u16 USART_RX_STA=0;

void uart_init(u32 bound)
{
	/* 正点原子模板的 USART1(PA9/PA10) 在 GD32F407 上对应 USART0，
	 * AF 复用号同为 7 */
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_USART0);

	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9 | GPIO_PIN_10);
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9 | GPIO_PIN_10);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9 | GPIO_PIN_10);

	usart_deinit(USART0);
	usart_baudrate_set(USART0, bound);
	usart_word_length_set(USART0, USART_WL_8BIT);
	usart_stop_bit_set(USART0, USART_STB_1BIT);
	usart_parity_config(USART0, USART_PM_NONE);
	usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
	usart_receive_config(USART0, USART_RECEIVE_ENABLE);
	usart_enable(USART0);

#if EN_USART1_RX
	usart_interrupt_enable(USART0, USART_INT_RBNE);

	nvic_irq_enable(USART0_IRQn, 3, 3);
#endif
}

void USART0_IRQHandler(void)
{
	u8 Res;
	if (usart_flag_get(USART0, USART_FLAG_RBNE) != RESET)
	{
		Res = usart_data_receive(USART0);
		if((USART_RX_STA&0x8000)==0)
		{
			if(USART_RX_STA&0x4000)
			{
				if(Res!=0x0a)USART_RX_STA=0;
				else USART_RX_STA|=0x8000;
			}
			else
			{
				if(Res==0x0d)USART_RX_STA|=0x4000;
				else
				{
					USART_RX_BUF[USART_RX_STA&0X3FFF]=Res ;
					USART_RX_STA++;
					if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;
				}
			}
		}
	}
}
#endif
