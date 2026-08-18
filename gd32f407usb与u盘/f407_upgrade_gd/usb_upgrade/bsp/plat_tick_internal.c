/**
 * @file plat_tick_internal.c
 * @brief 裸机 plat_tick 实现 — 本工程自带 TIMER2 1ms 中断驱动
 *
 * 用于独立运行 / 调试：占用本工程一个 TIM（默认 TIMER2）。
 * 移植到目标设备且目标设备已有定时器时，改用 plat_tick_external.c
 * （由 PLAT_TIMER_SOURCE 开关选择，见 plat_config.h）。
 *
 * 微秒延时用 DWT 周期计数器，不触碰 SysTick / TIMER2 寄存器，避免冲突。
 */
#include "plat_tick.h"
#include "plat_timer.h"
#include "board_pin_config.h"

#if (PLAT_SELECT == PLAT_BARE_METAL) && (PLAT_TIMER_SOURCE == PLAT_TIMER_INTERNAL)

static volatile uint32_t g_tick_ms;

/* DWT 周期计数器初始化（Cortex-M3/M4 支持，微秒延时用） */
static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void plat_tick_init(void)
{
    timer_parameter_struct timer_initpara;

    dwt_init();

    /* 使能 TIM 时钟 */
    rcu_periph_clock_enable(TICK_TIM_RCC);

    /* 1ms 周期：预分频到 1MHz，周期 1000 */
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = (TICK_TIM_CLK_HZ / 1000000u) - 1;
    timer_initpara.period            = 1000 - 1;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_init(TICK_TIM_PERIPH, &timer_initpara);

    /* 清更新标志，使能更新中断 + 定时器 */
    timer_interrupt_flag_clear(TICK_TIM_PERIPH, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TICK_TIM_PERIPH, TIMER_INT_UP);
    timer_enable(TICK_TIM_PERIPH);

    /* 使能 NVIC 中断 */
    nvic_irq_enable(TICK_TIM_IRQn, 1U, 0U);
}

/* TIM 更新中断：驱动 tick 计数 + timer 回调 */
void TICK_TIM_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TICK_TIM_PERIPH, TIMER_INT_FLAG_UP) == SET) {
        timer_interrupt_flag_clear(TICK_TIM_PERIPH, TIMER_INT_FLAG_UP);
        g_tick_ms++;
        plat_timer_poll();
    }
}

uint32_t plat_get_tick_ms(void)
{
    return g_tick_ms;
}

void plat_delay_ms(uint32_t ms)
{
    uint32_t start = g_tick_ms;
    while ((g_tick_ms - start) < ms);
}

void plat_delay_us(uint32_t us)
{
    /* DWT 周期计数器 — 不干扰 TIM 中断 */
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000u);
    while ((DWT->CYCCNT - start) < cycles);
}

/* 跳转 Bootloader 前调用：关闭 tick 中断源，防止跳转后中断
 * 命中 Bootloader 的默认死循环 handler 而卡死 */
void plat_tick_deinit(void)
{
    timer_interrupt_disable(TICK_TIM_PERIPH, TIMER_INT_UP);
    timer_disable(TICK_TIM_PERIPH);
    NVIC_DisableIRQ(TICK_TIM_IRQn);
    NVIC_ClearPendingIRQ(TICK_TIM_IRQn);
}

#endif /* PLAT_BARE_METAL && PLAT_TIMER_INTERNAL */
