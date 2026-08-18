#include "TIM2.h"


u32 Task_Timer[MAXTIMER]={0};


/**
 * @brief			TIM2��ʼ����ʵ��1ms�Ķ�ʱ
 * @detail	
 * @param
 * @return
 * @note			Ƶ�� = 72000000/[��psc+1��*(arr+1) ]
							���� = 1/Ƶ��
*/
void TIM2_Init(void)
{
	/*1������ʱ��*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	
	/*2��ʱ����Ԫ��ʼ��*/
	TIM_TimeBaseInitTypeDef TIM_TimerBaseInitStructure;
	TIM_TimerBaseInitStructure.TIM_Prescaler = 72-1;
	TIM_TimerBaseInitStructure.TIM_Period = 1000-1;
	TIM_TimerBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimerBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM2,&TIM_TimerBaseInitStructure);
	
	/*3������жϱ�־λ��������ʱ�������ж�*/
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
	
	/*4�������ж����ȼ�*/
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;//��stm32f10x.h������
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//�����ȼ�
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;//�����ȼ�
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	/*5��ʹ�ܶ�ʱ��*/
	TIM_Cmd(TIM2,ENABLE);
}



/**
 * @brief			����ʱ���ʼ������
 * @detail	
 * @param
 * @return
*/
void Task_Time_Init(void)
{
	for(int i=0;i<MAXTIMER;i++)
	{
		Task_Timer[ i ] = 0;
	}
}	







/**
 * @brief			���뼶��ʱ����
 * @detail	
 * @param
 * @return
*/
void TIM2_Delay_Ms(u32 timer)
{
	Delay_Timer = timer;
	while( Delay_Timer ){};
}




/**
 * @brief			��ʱ��1�жϷ�����
 * @detail		�𵽷����������->ʵ������ʱ��ݼ�
 * @param
 * @return
*/
void TIM2_IRQHandler(void)
{

	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET)
	{
			for(int i=0;i < MAXTIMER;i++)
			{
				if(Task_Timer[i])
					Task_Timer[i]--;
			}

		Delay_TickInc();

		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	}

}
