#ifndef __LED_H
#define __LED_H
#include "sys.h"

//////////////////////////////////////////////////////////////////////////////////
// 本代码仅供学习使用，未经允许，不得用于任何其他用途
// ALIENTEK STM32F407 开发板
// LED 驱动实验
// 正点原子@ALIENTEK
// 技术论坛:www.openedv.com
// 创建日期:2014/5/2
// 版本：V1.0
// 版权所有，盗版必究
// Copyright(C) 广州市星翼电子科技有限公司 2014-2024
// All rights reserved
//////////////////////////////////////////////////////////////////////////////////


// LED 端口定义
/* ALIENTEK 探索者 F407：LED0=PF9, LED1=PF10（低电平点亮） */
#define LED0 PFout(9)	// LED0
#define LED1 PFout(10)	// LED1

void LED_Init(void);// 初始化
#endif
