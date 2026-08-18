/**
 * @file tick_drv.c
 * @brief 系统 Tick / 延时实现（轮询模式，无中断）
 *
 * 从原 st_tick.c 迁移，移除函数指针封装层。
 * TIM3 轮询模式：主循环调 Tick_Poll() 累加毫秒计数器。
 */

#include "board_config.h"
#include "tick_drv.h"

volatile uint32_t g_tick_ms = 0;

static void tick_hw_init(void)
{
    RCC_APB1PeriphClockCmd(TICK_TIM_RCC, ENABLE);

    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_InitStruct.TIM_Prescaler = (TICK_TIM_CLK_HZ / 1000000u) - 1;
    TIM_InitStruct.TIM_Period = 1000 - 1;
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TICK_TIM, &TIM_InitStruct);

    /* TIM_TimeBaseInit 会产生一次更新事件设置 UIF，清除它 */
    TICK_TIM->SR = 0;

    /* 不启用 NVIC 中断，不启用 DIER.UIE — 纯轮询模式 */
    TIM_Cmd(TICK_TIM, ENABLE);
    TICK_TIM->SR = 0;
}

/* 安全网：如果 TIM3 中断意外触发（不应发生），此函数防止跑飞 */
void TICK_TIM_IRQHandler(void)
{
    TICK_TIM->SR = 0;
}

void Tick_Init(void)
{
    tick_hw_init();
}

uint32_t Tick_GetMs(void)
{
    return g_tick_ms;
}

void Tick_Poll(void)
{
    if (TICK_TIM->SR & TIM_IT_Update) {
        TICK_TIM->SR = (uint16_t)~TIM_IT_Update;
        g_tick_ms++;
    }
}

void Tick_DelayUs(uint32_t us)
{
    /* 使用 HCLK/8 时钟源，与 delay_init() 保持一致 */
    SysTick->LOAD = (SYSTEM_CORE_CLOCK / 8) * us;
    SysTick->VAL = 0x00;
    SysTick->CTRL = 0x00000001;       /* Enable, HCLK/8 source (bit2=0) */
    while (!(SysTick->CTRL & 0x00010000));
    SysTick->CTRL = 0x00000000;
    SysTick->VAL = 0x00;
}

void Tick_DelayMs(uint32_t ms)
{
    while (ms--) {
        Tick_DelayUs(1000);
    }
}
