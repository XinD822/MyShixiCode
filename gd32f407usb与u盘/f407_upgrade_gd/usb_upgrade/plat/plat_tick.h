/**
 * @file plat_tick.h
 * @brief 时间抽象 — 裸机用 TIM（本工程自带或外部注入），uCOS 用 OS tick
 */
#ifndef __PLAT_TICK_H
#define __PLAT_TICK_H
#include "plat_config.h"
#include <stdint.h>

void     plat_tick_init(void);
uint32_t plat_get_tick_ms(void);
void     plat_delay_ms(uint32_t ms);
void     plat_delay_us(uint32_t us);

/**
 * @brief 关闭 tick 中断源（跳转 Bootloader 前必须调用）
 *
 * 防止跳转后 tick 中断命中 Bootloader 的默认死循环 handler 而卡死。
 * INTERNAL 模式：关 TIMER2 中断 + 清 pending；
 * EXTERNAL 模式：调用 plat_tick_stop_hw() 停止外部定时器
 *   （默认 GD32 标准库实现，仅用 EXT_TIM_* 宏；若目标工程库不同，
 *    写同名函数覆盖即可，无需修改本文件）；
 * OSTICK 模式：仅清计数（OS tick 由设备复位流程管理）。
 */
void     plat_tick_deinit(void);

/**
 * @brief 外部定时器喂 tick（仅 PLAT_TIMER_EXTERNAL 模式使用）
 *
 * 目标工程的定时器中断里调用（每个中断周期调用一次）：
 *   1ms 中断 → 每次调 1 次；10ms 中断 → 每次调 10 次。
 */
void     plat_tick_isr(void);

#endif /* __PLAT_TICK_H */
