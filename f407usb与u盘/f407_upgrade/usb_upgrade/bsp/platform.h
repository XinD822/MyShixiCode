/**
 * @file platform.h
 * @brief 平台底层接口（电源/复位/中断控制 + USB 时钟）
 *
 * 合并了原 hal_pwr + hal_usb + st_pwr + st_usb 的底层功能。
 */

#ifndef __PLATFORM_H
#define __PLATFORM_H

#include <stdint.h>

/* ──── 中断 / 复位 / VTOR ──── */
void Platform_DisableIRQ(void);
void Platform_EnableIRQ(void);
void Platform_SetMSP(uint32_t addr);
void Platform_SetVTOR(uint32_t addr);
void Platform_SystemReset(void);
void Platform_NVICPriorityGroup(uint32_t group);

/* ──── USB 时钟 / NVIC ──── */
void Platform_USB_ClockEnable(void);
void Platform_USB_ClockDisable(void);
void Platform_USB_NVICEnable(void);
void Platform_USB_NVICDisable(void);

#endif /* __PLATFORM_H */
