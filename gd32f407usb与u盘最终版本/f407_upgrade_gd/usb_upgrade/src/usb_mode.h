/**
 * @file usb_mode.h
 * @brief USB 模式抽象（DEVICE / HOST）
 *
 * 上电时从 Flash 配置区读取模式标志，二选一初始化。
 * 运行时按键可切换标志位并软件复位，重新上电后生效。
 */

#ifndef __USB_MODE_H
#define __USB_MODE_H

#include <stdint.h>

typedef enum {
    USB_MODE_DEVICE = 0,    /* Device 模式，PC 拖拽升级 */
    USB_MODE_HOST   = 1,    /* Host 模式，U盘读取升级 */
} USB_Mode_t;

/**
 * @brief 获取当前 USB 模式
 */
USB_Mode_t USB_Mode_Get(void);

/**
 * @brief 手动设置 USB 模式（仅内存，不写 Flash）
 */
void USB_Mode_Set(USB_Mode_t mode);

/**
 * @brief 从 Flash 配置区读取模式标志
 * @return USB_MODE_HOST 或 USB_MODE_DEVICE
 */
USB_Mode_t USB_Mode_ReadFlag(void);

/**
 * @brief 将模式标志写入 Flash 配置区（独立扇区，不影响其他配置）
 */
void USB_Mode_WriteFlag(USB_Mode_t mode);

/**
 * @brief 切换 Flash 中的模式标志（DEVICE↔HOST）
 */
void USB_Mode_ToggleFlag(void);

/**
 * @brief 按当前模式初始化 USB 外设
 *        DEVICE: 初始化 MSC 设备
 *        HOST:   初始化 USB Host
 */
void USB_Mode_Init(void);

/**
 * @brief 按当前模式运行 USB 任务
 *        放在主循环中
 */
void USB_Mode_Run(void);

/**
 * @brief 强制下次 ReadFlag 返回 DEVICE（恢复模式）
 *        用于 KEY0 长按开机时绕过 Flash 中的 HOST 标志，
 *        提供安全恢复路径。
 */
void USB_Mode_ForceDevice(void);

#endif /* __USB_MODE_H */
