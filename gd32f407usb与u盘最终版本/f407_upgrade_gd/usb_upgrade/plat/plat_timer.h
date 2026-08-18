/**
 * @file plat_timer.h
 * @brief 定时器回调抽象 — 支持复用已有 TIM 或独立 TIM
 */
#ifndef __PLAT_TIMER_H
#define __PLAT_TIMER_H
#include "plat_config.h"
#include <stdint.h>

typedef void (*plat_timer_cb_t)(void *arg);

int  plat_timer_register(plat_timer_cb_t cb, void *arg, uint32_t period_ms);
void plat_timer_unregister(int handle);
void plat_timer_poll(void);

/* PLAT_TIMER_SOURCE 由 plat_config.h 定义：
 *   PLAT_TIMER_INTERNAL = 本工程自带 TIMER2（1ms 中断）
 *   PLAT_TIMER_EXTERNAL = 复用目标工程已有 TIM（外部注入 plat_tick_isr）
 *   PLAT_TIMER_OSTICK   = uCOS 系统 tick */

#endif /* __PLAT_TIMER_H */
