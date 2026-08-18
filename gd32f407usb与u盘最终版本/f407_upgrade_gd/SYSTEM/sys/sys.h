#ifndef __SYS_H
#define __SYS_H
#include "gd32f4xx.h"

/* GCC-style 类型别名（STM32 库由 stm32f4xx.h 提供，GD32 库无，这里补充） */
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
//////////////////////////////////////////////////////////////////////////////////
// 本代码仅供学习使用，未经允许，不得用于任何其他用途
// ALIENTEK STM32F407 开发板
// 系统时钟初始化
// 正点原子@ALIENTEK
// 技术论坛:www.openedv.com
// 创建日期:2014/5/2
// 版本：V1.0
// 版权所有，盗版必究
// Copyright(C) 广州市星翼电子科技有限公司 2014-2024
// All rights reserved
//********************************************************************************
// 修改说明
// 无
//////////////////////////////////////////////////////////////////////////////////


// 0,不支持ucos
// 1,支持ucos
#define SYSTEM_SUPPORT_OS		0		// 设置系统文件是否支持UCOS

// 位带操作,实现51类似的GPIO控制功能
// 具体实现思想,参考<<CM3权威指南>>中文版(87页~92页).M4同M3一样,只是寄存器地址变了.
// IO内部宏定义
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2))
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr))
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum))
// IO口寄存器地址映射 — GD32F4xx 用 GPIO_OCTL/GPIO_ISTAT 宏取寄存器地址
#define GPIOA_ODR_Addr    (GPIO_OCTL(GPIOA)) //0x40020014
#define GPIOB_ODR_Addr    (GPIO_OCTL(GPIOB)) //0x40020414
#define GPIOC_ODR_Addr    (GPIO_OCTL(GPIOC)) //0x40020814
#define GPIOD_ODR_Addr    (GPIO_OCTL(GPIOD)) //0x40020C14
#define GPIOE_ODR_Addr    (GPIO_OCTL(GPIOE)) //0x40021014
#define GPIOF_ODR_Addr    (GPIO_OCTL(GPIOF)) //0x40021414
#define GPIOG_ODR_Addr    (GPIO_OCTL(GPIOG)) //0x40021814
#define GPIOH_ODR_Addr    (GPIO_OCTL(GPIOH)) //0x40021C14
#define GPIOI_ODR_Addr    (GPIO_OCTL(GPIOI)) //0x40022014

#define GPIOA_IDR_Addr    (GPIO_ISTAT(GPIOA)) //0x40020010
#define GPIOB_IDR_Addr    (GPIO_ISTAT(GPIOB)) //0x40020410
#define GPIOC_IDR_Addr    (GPIO_ISTAT(GPIOC)) //0x40020810
#define GPIOD_IDR_Addr    (GPIO_ISTAT(GPIOD)) //0x40020C10
#define GPIOE_IDR_Addr    (GPIO_ISTAT(GPIOE)) //0x40021010
#define GPIOF_IDR_Addr    (GPIO_ISTAT(GPIOF)) //0x40021410
#define GPIOG_IDR_Addr    (GPIO_ISTAT(GPIOG)) //0x40021810
#define GPIOH_IDR_Addr    (GPIO_ISTAT(GPIOH)) //0x40021C10
#define GPIOI_IDR_Addr    (GPIO_ISTAT(GPIOI)) //0x40022010

// IO口操作,只对单一的IO口!
// 确保n的值小于16!
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  // 输出
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  // 输入

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)  // 输出
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)  // 输入

#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  // 输出
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)  // 输入

#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)  // 输出
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)  // 输入

#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)  // 输出
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)  // 输入

#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)  // 输出
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)  // 输入

#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  // 输出
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  // 输入

#define PHout(n)   BIT_ADDR(GPIOH_ODR_Addr,n)  // 输出
#define PHin(n)    BIT_ADDR(GPIOH_IDR_Addr,n)  // 输入

#define PIout(n)   BIT_ADDR(GPIOI_ODR_Addr,n)  // 输出
#define PIin(n)    BIT_ADDR(GPIOI_IDR_Addr,n)  // 输入

// 以下为自写函数
void WFI_SET(void);		// 执行WFI指令
void INTX_DISABLE(void);// 关闭所有中断
void INTX_ENABLE(void);	// 打开所有中断
void MSR_MSP(u32 addr);	// 设置栈地址
#endif
