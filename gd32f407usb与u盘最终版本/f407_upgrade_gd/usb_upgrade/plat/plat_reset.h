/**
 * @file plat_reset.h
 * @brief 系统复位抽象
 */
#ifndef __PLAT_RESET_H
#define __PLAT_RESET_H
#include "plat_config.h"
#include <stdint.h>

void plat_system_reset(void);
void plat_disable_irq(void);
void plat_enable_irq(void);
void plat_set_msp(uint32_t addr);
void plat_set_vtor(uint32_t addr);
void plat_nvic_priority_group(uint32_t group);

/* USB 时钟 / NVIC */
void plat_usb_clock_enable(void);
void plat_usb_clock_disable(void);
void plat_usb_nvic_enable(void);
void plat_usb_nvic_disable(void);

#endif /* __PLAT_RESET_H */
