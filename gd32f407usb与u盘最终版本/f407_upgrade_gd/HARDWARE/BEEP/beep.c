#include "beep.h"
#include "gd32f4xx.h"
//BEEP IO初始化
void BEEP_Init(void)
{
	rcu_periph_clock_enable(RCU_GPIOF);   //使能GPIOF时钟

	//GPIOF8初始化，下拉输出
	gpio_mode_set(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_8);
	gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);

	gpio_bit_reset(GPIOF, GPIO_PIN_8);    //GPIOF8置低，关蜂鸣器
}
