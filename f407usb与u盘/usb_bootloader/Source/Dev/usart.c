#include "config.h"

//Usart1 TX PA9		重映像PB6		全双工模式		推挽复用输出
//Usart1 RX PA10	重映像PB7		全双工模式		浮空输入或上拉输入
//Usart-APB1,Usart2345a-APB2

u8	USART1_RxBUF[USART1_RXBUF_SIZE];//接收传来的字符串
u16 USART1_RecPos=0; //控制接收后存放的位置




/**
 * @brief 		串口1初始化
 * @detail
 * @param
 * @return
 * @note			注意开启串口中断配置，和清除标志位
*/
void UsartInit(void)
{
	USART_InitTypeDef USART_InitStructure;

	/* 开时钟：GPIO + USART */
	BOOTLOADER_USART_GPIO_CLK_CMD(BOOTLOADER_USART_RCC_GPIO, ENABLE);
	BOOTLOADER_USART_UART_CLK_CMD(BOOTLOADER_USART_RCC_UART, ENABLE);

	/* TX 复用推挽 */
	GPIO_AF_INIT_AF_PP(BOOTLOADER_USART_TX_PORT, BOOTLOADER_USART_TX_PIN,
	                   BOOTLOADER_USART_TX_SOURCE, BOOTLOADER_USART_TX_AF, GPIO_Speed_50MHz);
	/* RX 上拉输入 */
	GPIO_AF_INIT_IPU(BOOTLOADER_USART_RX_PORT, BOOTLOADER_USART_RX_PIN);
	
	USART_InitStructure.USART_BaudRate=BOOTLOADER_USART_BAUD;
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;
	USART_InitStructure.USART_StopBits=USART_StopBits_1;
	USART_InitStructure.USART_Parity=USART_Parity_No;
	USART_InitStructure.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	USART_Init(BOOTLOADER_USART,&USART_InitStructure);
	
	USART_Cmd(BOOTLOADER_USART,ENABLE);
	USART_ClearFlag(BOOTLOADER_USART,USART_FLAG_TC);
}


/**
 * @brief			发送字节的函数
 * @detail		None
 * @param			USARTx：使用串口x进行发送
 * @param			ch：要发送的数据(字节)
 * @return		None
*/
void UsartSendByte(USART_TypeDef* USARTx,u8 ch)
{
	USART_SendData(USARTx,(u16)ch);//第二步，写状态寄存器DR
	while( USART_GetFlagStatus(USARTx,USART_FLAG_TC)==RESET );
	//这两步做完，自动清除标志位
}


/**
 * @brief			发送字符串的函数
 * @detail		None
 * @param			USARTx：使用串口x进行发送
 * @param			ch：要发送的数据(字符串)
 * @return		None
*/
void UsartSendString(USART_TypeDef* USARTx,u8 * str)
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
	USART_SendData(BOOTLOADER_USART,(u16)ch);
	while( USART_GetFlagStatus(BOOTLOADER_USART,USART_FLAG_TC)==RESET );
	return (ch);
}


/**
 * @brief			串口中断接收函数
 * @detail		None
 * @param			None
 * @return		将接收到的数据存储到 数组中
*/
void USART1_IRQHandler(void)
{
	u8 RecCh;
	if( USART_GetITStatus(USART1,USART_IT_RXNE)==SET ) //判断是不是接收中断进来的
	{
		//sysTimer[5] = 10：
		RecCh= (u8) USART_ReceiveData(USART1);
		USART1_RxBUF[USART1_RecPos++]=RecCh;
		
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);//清除中断标志位
	}
}


















	
