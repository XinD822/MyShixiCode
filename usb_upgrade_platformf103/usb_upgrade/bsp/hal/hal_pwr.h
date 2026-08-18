/**
 * @file hal_pwr.h
 * @brief 电源 / 复位 / 中断控制 HAL 接口
 */

#ifndef __HAL_PWR_H
#define __HAL_PWR_H

#include <stdint.h>

typedef struct {
    void (*disable_irq)(void);
    void (*enable_irq)(void);
    void (*set_msp)(uint32_t addr);
    void (*set_vtor)(uint32_t addr);
    void (*system_reset)(void);
    void (*nvic_priority_group)(uint32_t group);
} HAL_Pwr_Drv_t;

extern const HAL_Pwr_Drv_t *HAL_Pwr;

#endif /* __HAL_PWR_H */
