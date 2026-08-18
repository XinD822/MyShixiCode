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
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructurre;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_USART1,ENABLE);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	USART_InitStructure.USART_BaudRate=USART1_BAUD;
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;
	USART_InitStructure.USART_StopBits=USART_StopBits_1;
	USART_InitStructure.USART_Parity=USART_Parity_No;//校验位
	USART_InitStructure.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;//硬件流控制
	USART_Init(USART1,&USART_InitStructure);
	
	NVIC_InitStructurre.NVIC_IRQChannel=USART1_IRQn;//中断通道
	NVIC_InitStructurre.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitStructurre.NVIC_IRQChannelPreemptionPriority=1;
	NVIC_InitStructurre.NVIC_IRQChannelSubPriority=2;
	NVIC_Init(&NVIC_InitStructurre);
	
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);//开启串口中断配置.参数二：串口1的什么中断
	
	USART_Cmd(USART1,ENABLE);
	USART_ClearFlag(USART1,USART_FLAG_TC);//第二个参数是发送完成的标志位
	
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
	while( USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET );//第一步，读状态寄存器SR
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
	
	USART_SendData(USART1,(u16)ch);//发送一个数据到USART1
	while( USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET );//等待发送完成
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


















	
