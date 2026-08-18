/**
 * @file tick_drv.h
 * @brief 系统 Tick / 延时驱动接口（轮询模式，无中断）
 */

#ifndef __TICK_DRV_H
#define __TICK_DRV_H

#include <stdint.h>

void     Tick_Init(void);
uint32_t Tick_GetMs(void);
void     Tick_DelayMs(uint32_t ms);
void     Tick_DelayUs(uint32_t us);

/**
 * @brief 轮询 TIM 更新标志，累加毫秒计数器
 * 必须在主循环中频繁调用，替代硬件中断
 */
void     Tick_Poll(void);

#endif /* __TICK_DRV_H */
