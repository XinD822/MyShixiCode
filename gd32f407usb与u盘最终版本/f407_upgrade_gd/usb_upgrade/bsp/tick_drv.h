/**
 * @file tick_drv.h
 * @brief 已迁移到 plat_tick.h — 此文件保留向后兼容
 *
 * 新代码直接 #include "plat_tick.h"
 * 注意：新 tick 基于 SysTick 中断，Tick_Poll() 不再需要（已改为空操作）。
 */
#ifndef __TICK_DRV_H
#define __TICK_DRV_H

#include "plat_tick.h"

/* ──── 向后兼容宏（旧函数名 → 新函数名） ──── */
#define Tick_Init     plat_tick_init
#define Tick_GetMs    plat_get_tick_ms
#define Tick_DelayMs  plat_delay_ms
#define Tick_DelayUs  plat_delay_us

/* 旧轮询模式下需要主循环调用，新 SysTick 中断模式不再需要 */
#define Tick_Poll()   ((void)0)

#endif /* __TICK_DRV_H */
