#include "config.h"

//GD32 USART1 TX PA2		全双工模式		复用推挽输出
//GD32 USART1 RX PA3		全双工模式		上拉输入
//USART1 挂在 APB1 上

u8	USART1_RxBUF[USART1_RXBUF_SIZE];//接收传来的字符串
u16 USART1_RecPos=0; //控制接收后存放的位置


/**
 * @brief 		串口1初始化
 * @detail
 * @param
 * @return
 * @note
*/
void UsartInit(void)
{
	/* 开时钟：GPIO + USART */
	rcu_periph_clock_enable(BOOTLOADER_USART_RCC_GPIO);
	rcu_periph_clock_enable(BOOTLOADER_USART_RCC_UART);

	/* TX 复用推挽 */
	GPIO_AF_INIT_AF_PP(BOOTLOADER_USART_TX_PORT, BOOTLOADER_USART_TX_PIN,
	                   BOOTLOADER_USART_TX_SOURCE, BOOTLOADER_USART_TX_AF, GPIO_OSPEED_50MHZ);
	/* RX 上拉输入 */
	GPIO_AF_INIT_IPU(BOOTLOADER_USART_RX_PORT, BOOTLOADER_USART_RX_PIN);

	usart_deinit(BOOTLOADER_USART);
	usart_baudrate_set(BOOTLOADER_USART, BOOTLOADER_USART_BAUD);
	usart_word_length_set(BOOTLOADER_USART, USART_WL_8BIT);
	usart_stop_bit_set(BOOTLOADER_USART, USART_STB_1BIT);
	usart_parity_config(BOOTLOADER_USART, USART_PM_NONE);
	usart_transmit_config(BOOTLOADER_USART, USART_TRANSMIT_ENABLE);
	usart_receive_config(BOOTLOADER_USART, USART_RECEIVE_ENABLE);
	usart_enable(BOOTLOADER_USART);

	usart_flag_clear(BOOTLOADER_USART, USART_FLAG_TC);
}


/**
 * @brief			发送字节的函数
 * @detail		None
 * @param			USARTx：使用串口x进行发送
 * @param			ch：要发送的数据(字节)
 * @return		None
*/
void UsartSendByte(uint32_t USARTx,u8 ch)
{
	usart_data_transmit(USARTx,(uint16_t)ch);
	while( usart_flag_get(USARTx,USART_FLAG_TC)==RESET );
}


/**
 * @brief			发送字符串的函数
 * @detail		None
 * @param			USARTx：使用串口x进行发送
 * @param			str：要发送的数据(字符串)
 * @return		None
*/
void UsartSendString(uint32_t USARTx,u8 * str)
{
	u32 pos=0;//字符串所在位置
	while( *(str+pos) !='\0')
	{
		UsartSendByte(USARTx,*(str+pos));
		pos++;
	}
}


/**
 * @brief			重定义fputc函数到串口printf函数
 * @detail		None
 * @param			ch：要写入的字符
 * @param			f：指向FILE结构的指针
 * @return		None
*/
int fputc(int ch,FILE * f)
{
	usart_data_transmit(BOOTLOADER_USART,(uint16_t)ch);
	while( usart_flag_get(BOOTLOADER_USART,USART_FLAG_TC)==RESET );
	return (ch);
}


/**
 * @brief			串口1中断接收函数
 * @detail		None
 * @param			None
 * @return		将接收到的数据存储到 数组中
*/
void USART1_IRQHandler(void)
{
	u8 RecCh;
	if( usart_flag_get(BOOTLOADER_USART,USART_FLAG_RBNE)==SET )
	{
		RecCh = (u8) usart_data_receive(BOOTLOADER_USART);
		USART1_RxBUF[USART1_RecPos++]=RecCh;

		usart_interrupt_flag_clear(BOOTLOADER_USART,USART_INT_FLAG_RBNE);
	}
}
