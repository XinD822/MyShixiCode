/**
 * @file usb_upgrade.h
 * @brief USB 拖拽升级模块 — 对外唯一接口
 *
 * 使用方法（在你的 main.c 中加 3 行）：
 *
 *   #include "usb_upgrade.h"
 *
 *   int main(void)
 *   {
 *       // ... 你原来的初始化 ...
 *       USB_Upgrade_Init();
 *
 *       while (1)
 *       {
 *           // ... 你原来的主循环 ...
 *           USB_Upgrade_Run();
 *       }
 *   }
 *
 * 外部 Tick 注入模式（board_config.h 中定义 TICK_EXTERNAL 时）：
 *
 *   // 在你的定时器中断中每 1ms 调用一次：
 *   void SysTick_Handler(void)
 *   {
 *       // ... 你原来的代码 ...
 *       USB_Upgrade_TickInc();
 *   }
 */

#ifndef __USB_UPGRADE_H
#define __USB_UPGRADE_H

#include <stdint.h>

/**
 * @brief 初始化升级模块
 *
 * 内部自动完成：Flash 初始化、FatFS 挂载、USB MSC 初始化、升级模块初始化
 * 调用一次即可，放在 main 初始化阶段
 */
void USB_Upgrade_Init(void);

/**
 * @brief 运行升级任务
 *
 * 每次主循环调用一次，内部处理：
 * - USB 连接/断开检测
 * - 固件文件检测
 * - 固件搬运到 Slot A
 * - 设升级标志并重启
 *
 * 放在 while(1) 中
 */
void USB_Upgrade_Run(void);

/**
 * @brief Tick 自增（外部注入模式专用）
 *
 * 仅在 board_config.h 定义了 TICK_EXTERNAL 时需要调用。
 * 在你的定时器中断中每 1ms 调用一次。
 * 如果使用模块自带定时器（默认模式），不需要调用此函数。
 */
void USB_Upgrade_TickInc(void);

/**
 * @brief 获取当前 Tick 值
 *
 * 返回模块内部的毫秒计数器值。
 */
uint32_t USB_Upgrade_TickGet(void);

#endif /* __USB_UPGRADE_H */
