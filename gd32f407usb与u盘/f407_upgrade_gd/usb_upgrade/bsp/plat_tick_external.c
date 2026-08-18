/**
 * @file plat_tick_external.c
 * @brief 裸机 plat_tick 实现 — 复用目标工程已有的定时器（外部注入）
 *
 * 移植到目标设备且该设备已有 1ms（或 N ms）定时器中断时使用：
 * 不占用任何新外设，目标工程只需在它自己的定时器中断里调用
 * plat_tick_isr() 喂 tick（见 plat_tick.h 说明）。
 * 由 PLAT_TIMER_SOURCE 开关选择（见 plat_config.h）。
 *
 * 跳转 Bootloader 前停止外部定时器：默认提供 __weak 的
 * plat_tick_stop_hw()（GD32 标准库实现，仅用 EXT_TIM_* 宏），
 * 目标工程无需手写；若其库不同，写同名函数覆盖即可。
 *
 * 微秒延时用 DWT 周期计数器，不触碰任何 TIM / SysTick。
 */
#include "plat_tick.h"
#include "plat_timer.h"
#include "board_pin_config.h"   /* EXT_TIM_PERIPH / EXT_TIM_IRQn */

#if (PLAT_SELECT == PLAT_BARE_METAL) && (PLAT_TIMER_SOURCE == PLAT_TIMER_EXTERNAL)

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
    dwt_init();
    g_tick_ms = 0;
    /* 定时器由目标工程管理，此处不配置任何硬件 */
}

/* 目标工程的定时器中断里调用（每个中断周期调用一次）：
 *   示例：目标工程 1ms 中断 → 每中断调 1 次；
 *        目标工程 10ms 中断 → 每中断调 10 次，保证 ms 计数正确 */
void plat_tick_isr(void)
{
    g_tick_ms++;
    plat_timer_poll();
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
    /* DWT 周期计数器 — 不干扰外部 TIM */
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000u);
    while ((DWT->CYCCNT - start) < cycles);
}

/**
 * @brief 停止外部定时器（跳转 Bootloader 前调用）— 弱函数，可覆盖
 *
 * 默认实现：GD32 标准库四步停止（关更新中断使能 → 停计数器 →
 * 关 NVIC → 清 pending），定时器编号取自 EXT_TIM_* 宏。
 * 目标工程若用其他库（如 HAL），写同名函数覆盖即可：
 *     void plat_tick_stop_hw(void) { ...自己实现... }
 */
__weak void plat_tick_stop_hw(void)
{
    timer_interrupt_disable(EXT_TIM_PERIPH, TIMER_INT_UP);   /* 1 关更新中断使能 */
    timer_disable(EXT_TIM_PERIPH);                           /* 2 停计数器 */
    NVIC_DisableIRQ(EXT_TIM_IRQn);                           /* 3 关 NVIC */
    NVIC_ClearPendingIRQ(EXT_TIM_IRQn);                      /* 4 清 pending */
}

/* 跳转 Bootloader 前调用：停止外部定时器中断，防止跳转后中断
 * 命中 Bootloader 的死循环 handler 而卡死 */
void plat_tick_deinit(void)
{
    g_tick_ms = 0;
    plat_tick_stop_hw();
}

#endif /* PLAT_BARE_METAL && PLAT_TIMER_EXTERNAL */

