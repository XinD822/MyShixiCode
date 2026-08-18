#include "TIM2.h"


u32 Task_Timer[MAXTIMER]={0};


/**
 * @brief			TIM2初始化，实现1ms的定时
 * @detail			ST TIM2 -> GD32 TIMER2
 * @param
 * @return
 * @note			频率 = 84000000/[(psc+1)*(arr+1)]
							周期 = 1/频率
*/
void TIM2_Init(void)
{
	/*1、开定时器时钟*/
	rcu_periph_clock_enable(RCU_TIMER2);

	/*2、定时器单元初始化*/
	timer_parameter_struct timer_initpara;
	timer_struct_para_init(&timer_initpara);
	timer_initpara.prescaler        = (APB1_TIMER_HZ / 1000000u) - 1;
	timer_initpara.period           = 1000 - 1;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.clockdivision    = TIMER_CKDIV_DIV1;
	timer_initpara.alignedmode      = TIMER_COUNTER_EDGE;
	timer_init(TIMER2, &timer_initpara);

	/*3、清更新标志位，使能更新中断*/
	TIMER_INTF(TIMER2) = 0;
	timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_UP);
	timer_interrupt_enable(TIMER2, TIMER_INT_UP);

	/*4、设置中断优先级*/
	nvic_irq_enable(TIMER2_IRQn, 2, 2);

	/*5、使能定时器*/
	timer_enable(TIMER2);
}



/**
 * @brief			定时器初始化计数器
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
 * @brief			软件级定时器
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
 * @brief			定时器1中断服务函数
 * @detail		进入中断服务函数->实现任务时间递减
 * @param
 * @return
*/
void TIMER2_IRQHandler(void)
{

	if(timer_interrupt_flag_get(TIMER2, TIMER_INT_FLAG_UP)==SET)
	{
			for(int i=0;i < MAXTIMER;i++)
			{
				if(Task_Timer[i])
					Task_Timer[i]--;
			}

		Delay_TickInc();

		timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_UP);
	}

}
