/**
 * @file hal_tick.h
 * @brief 系统 Tick / 延时 HAL 接口
 */

#ifndef __HAL_TICK_H
#define __HAL_TICK_H

#include <stdint.h>

typedef struct {
    void     (*init)(void);
    uint32_t (*get_tick)(void);      /* 获取毫秒 tick */
    void     (*delay_ms)(uint32_t ms);
    void     (*delay_us)(uint32_t us);
} HAL_Tick_Drv_t;

extern const HAL_Tick_Drv_t *HAL_Tick;

#endif /* __HAL_TICK_H */
