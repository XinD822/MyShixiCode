/**
 * @file Delay.h
 * @brief 延时兼容头文件
 *
 * 提供旧接口声明，实际实现通过 HAL_Tick。
 * 用于兼容不直接使用 HAL 接口的旧代码。
 */

#ifndef __DELAY_H
#define __DELAY_H

#include "hal_config.h"

/* 旧接口声明（实现在 st_tick.c 中） */
uint32_t Delay_GetTick(void);
void     Delay_ms(uint32_t ms);
void     Delay_us(uint32_t us);

#endif /* __DELAY_H */
