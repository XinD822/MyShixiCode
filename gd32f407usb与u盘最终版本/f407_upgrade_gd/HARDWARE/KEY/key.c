#include "key.h"
#include "delay.h"
#include "gd32f4xx.h"

// 按键初始化
void KEY_Init(void)
{
	rcu_periph_clock_enable(RCU_GPIOA);   // 使能 GPIOA 时钟
	rcu_periph_clock_enable(RCU_GPIOE);   // 使能 GPIOE 时钟

	// KEY0 对应 PE4，下拉
	gpio_mode_set(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_4);
	gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);

	// WK_UP 对应 PA0，下拉
	gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_0);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
}

// 按键扫描函数
// 返回按键值：
// mode:0, 不支持连续按; 1, 支持连续按;
// 0: 没有任何按键按下
// 1: KEY0 按下
// 2: KEY1 按下
// 3: KEY2 按下
// 4: WKUP 按下 (WK_UP)
// 注意：此函数有响应优先级, KEY0>KEY1>KEY2>WK_UP!!
u8 KEY_Scan(u8 mode)
{
	static u8 key_up=1;            // 按键松开标志
	if(mode)key_up=1;              // 支持连按
	if(key_up&&(KEY0==1||WK_UP==1))
	{
		delay_ms(10);              // 去抖动
		key_up=0;
		if(KEY0==1)return KEY0_PRES;
		else if(WK_UP==1)return WKUP_PRES;
	}else if(KEY0==0&&WK_UP==0)key_up=1;
	return 0;                      // 无按键按下
}
