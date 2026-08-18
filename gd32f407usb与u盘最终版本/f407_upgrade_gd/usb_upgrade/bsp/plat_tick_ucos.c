/**
 * @file plat_tick_ucos.c
 * @brief uCOS-II 模式 plat_tick 实现 — 直接用 OS tick + DWT 微秒延时
 */
#include "plat_tick.h"
#include "plat_timer.h"

#if (PLAT_SELECT == PLAT_UCOS2)

void plat_tick_init(void)
{
    /* OS 已跑，无需初始化 */
}

uint32_t plat_get_tick_ms(void)
{
    return OSTimeGet() * (1000u / PLAT_OS_TICK_HZ);
}

void plat_delay_ms(uint32_t ms)
{
    if (ms == 0) return;
    uint32_t ticks = (ms * PLAT_OS_TICK_HZ + 999) / 1000u;
    if (ticks == 0) ticks = 1;

    /* OSTimeDly 参数为 INT16U（单次最大 65535 tick）：
     * 长延时（如 tick=1000 时 ms>65535）若直接传入会被截断，
     * 需循环分段延时，保证总时长正确。 */
    while (ticks > 0) {
        uint16_t chunk = (ticks > 65535u) ? 65535u : (uint16_t)ticks;
        OSTimeDly(chunk);
        ticks -= chunk;
    }
}

void plat_delay_us(uint32_t us)
{
    /* DWT 周期计数器 — 不干扰 OS tick */
    static uint8_t dwt_inited = 0;
    if (!dwt_inited) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        dwt_inited = 1;
    }
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000u);
    while ((DWT->CYCCNT - start) < cycles);
}

/* 跳转 Bootloader 前调用：uCOS 模式 OS tick 由目标工程管理，
 * 复位流程由目标工程负责停自己的 OS tick 中断源，此处无硬件动作。 */
void plat_tick_deinit(void)
{
    /* OS tick 归目标工程管，无需在此关闭；若目标工程需要，可在此
     * 调用其停止 tick 的接口（如 SysTick 关中断），保持为空以保证链接。 */
}

/* uCOS 模式不使用外部注入 tick（OS tick 由 OSTimeTick 驱动），
 * 此空桩仅为满足 plat_tick.h 声明一致性；目标工程不应调用它。 */
void plat_tick_isr(void)
{
}

/* uCOS tick hook — 驱动 timer 回调
 * 定义为 __weak：若目标工程 os_cpu_c.c 已提供非 weak 的 OSTimeTickHook
 * （标准 uCOS-II 移植通常如此），链接器优先使用目标工程的强定义，
 * 本弱定义被覆盖，不会产生符号冲突。
 * ⚠ 此时需在目标工程的 OSTimeTickHook 里补一行 plat_timer_poll()，
 *   否则升级模块的 timer 回调不会被驱动（见移植说明）。 */
#if (PLAT_TIMER_SOURCE == PLAT_TIMER_OSTICK)
__weak void OSTimeTickHook(void)
{
    plat_timer_poll();
}
#endif

#endif /* PLAT_UCOS2 */
