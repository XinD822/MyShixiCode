/**
 * @file st_tick.c
 * @brief STM32 Tick / 延时实现
 *
 * 支持三种模式：
 *   A. 使用模块自带定时器（默认 TIM2，可配置）
 *   B. 外部 Tick 注入（开发者在自己的中断里调用 USB_Upgrade_TickInc()）
 */

#include "hal_config.h"
#include "board_config.h"

#ifdef PLATFORM_STM32

static volatile uint32_t g_tick_ms = 0;

/* ═══════════════════════════════════════════════════════════
 * 模式 A/B：使用模块自带定时器
 * ═══════════════════════════════════════════════════════════ */
#ifndef TICK_EXTERNAL

/* ──── 定时器初始化（1ms 中断） ──── */

static void st_tick_hw_init(void)
{
    RCC_APB1PeriphClockCmd(TICK_TIM_RCC, ENABLE);

    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_InitStruct.TIM_Prescaler = (SYSTEM_CORE_CLOCK) - 1;
    TIM_InitStruct.TIM_Period = 1000 - 1;
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TICK_TIM, &TIM_InitStruct);

    TIM_ClearITPendingBit(TICK_TIM, TIM_IT_Update);
    TIM_ITConfig(TICK_TIM, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = TICK_TIM_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = TICK_TIM_PREEMPTION;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = TICK_TIM_SUBPRIORITY;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    TIM_Cmd(TICK_TIM, ENABLE);
}

/* ──── 中断处理函数名由宏决定，切换定时器时自动跟随 ──── */

void TICK_TIM_IRQHandler(void)
{
    if (TIM_GetITStatus(TICK_TIM, TIM_IT_Update) == SET) {
        g_tick_ms++;
        TIM_ClearITPendingBit(TICK_TIM, TIM_IT_Update);
    }
}

static void st_tick_init(void)
{
    st_tick_hw_init();
}

#else /* TICK_EXTERNAL */

/* ═══════════════════════════════════════════════════════════
 * 模式 C：外部 Tick 注入
 * 模块不初始化定时器，开发者在自己的中断里调用 USB_Upgrade_TickInc()
 * ═══════════════════════════════════════════════════════════ */

static void st_tick_init(void)
{
    /* 不初始化任何定时器，外部已提供 Tick */
}

#endif /* TICK_EXTERNAL */

/* ──── 公共接口 ──── */

uint32_t USB_Upgrade_TickGet(void)
{
    return g_tick_ms;
}

void USB_Upgrade_TickInc(void)
{
    g_tick_ms++;
}

/* ──── SysTick 延时（使用主频宏） ──── */

static void st_delay_us(uint32_t xus)
{
    SysTick->LOAD = SYSTEM_CORE_CLOCK * xus;
    SysTick->VAL = 0x00;
    SysTick->CTRL = 0x00000005;
    while (!(SysTick->CTRL & 0x00010000));
    SysTick->CTRL = 0x00000004;
}

static void st_delay_ms(uint32_t xms)
{
    while (xms--) {
        st_delay_us(1000);
    }
}

/* ──── HAL 驱动实例 ──── */

const HAL_Tick_Drv_t ST_Tick_Drv = {
    .init      = st_tick_init,
    .get_tick  = USB_Upgrade_TickGet,
    .delay_ms  = st_delay_ms,
    .delay_us  = st_delay_us,
};

/* 全局指针 */
const HAL_Tick_Drv_t *HAL_Tick = &ST_Tick_Drv;

/* ──── 兼容旧接口（可选） ──── */

#ifdef USB_UPGRADE_COMPAT_FUNCTIONS
uint32_t Delay_GetTick(void)    { return g_tick_ms; }
void     Delay_ms(uint32_t ms)  { st_delay_ms(ms); }
void     Delay_us(uint32_t us)  { st_delay_us(us); }
void     Delay_TickInc(void)    { g_tick_ms++; }
#endif

#endif /* PLATFORM_STM32 */
