#include "led.h"
#include "gd32f4xx.h"

// LED IO 初始化
void LED_Init(void)
{
	rcu_periph_clock_enable(RCU_GPIOF);              // 使能 GPIOF 时钟

	// GPIOF9, F10 初始化
	gpio_mode_set(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_9 | GPIO_PIN_10);
	gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9 | GPIO_PIN_10);

	gpio_bit_set(GPIOF, GPIO_PIN_9 | GPIO_PIN_10);   // GPIOF9, F10 置高，关 LED

	/* 开机自检：拉低 PF9 点亮 LED0，延时后恢复 */
	gpio_bit_reset(GPIOF, GPIO_PIN_9);
	for (volatile uint32_t i = 0; i < 2000000; i++);
	gpio_bit_set(GPIOF, GPIO_PIN_9);
}
