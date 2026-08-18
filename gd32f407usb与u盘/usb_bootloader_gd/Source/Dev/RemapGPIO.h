/*--------------------------------------------------------------------
 * 文件: RemapGPIO.h
 * 描述: GPIO 位带操作宏（F103/F407 共用）
 *
 *   位带区+位带别名区是 ARM Cortex-M 通用的，公式相同。
 *   仅 GPIO 的 ODR/IDR 寄存器偏移因芯片而异：
 *     F103: ODR=0x0C, IDR=0x08
 *     F407: ODR=0x14, IDR=0x10
 *-------------------------------------------------------------------*/
#ifndef __REMAPGPIO_H__
#define __REMAPGPIO_H__

#include "platform.h"

/* 位带公式（ARM Cortex-M 通用） */
#define BITBAND(addr, bitnum)  ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2))
#define MEM_ADDR(addr)         (*((volatile unsigned int  *)(addr)))
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND(addr, bitnum))

/* GPIO ODR/IDR 偏移 */
#if defined(CHIP_SERIES_F103)
  #define GPIO_ODR_OFFSET   0x0C
  #define GPIO_IDR_OFFSET   0x08
#elif defined(CHIP_SERIES_F407)
  #define GPIO_ODR_OFFSET   0x14
  #define GPIO_IDR_OFFSET   0x10
#endif

#define GPIOA_ODR_Addr  (GPIOA_BASE + GPIO_ODR_OFFSET)
#define GPIOB_ODR_Addr  (GPIOB_BASE + GPIO_ODR_OFFSET)
#define GPIOC_ODR_Addr  (GPIOC_BASE + GPIO_ODR_OFFSET)
#define GPIOD_ODR_Addr  (GPIOD_BASE + GPIO_ODR_OFFSET)
#define GPIOE_ODR_Addr  (GPIOE_BASE + GPIO_ODR_OFFSET)
#define GPIOF_ODR_Addr  (GPIOF_BASE + GPIO_ODR_OFFSET)
#define GPIOG_ODR_Addr  (GPIOG_BASE + GPIO_ODR_OFFSET)

#define GPIOA_IDR_Addr  (GPIOA_BASE + GPIO_IDR_OFFSET)
#define GPIOB_IDR_Addr  (GPIOB_BASE + GPIO_IDR_OFFSET)
#define GPIOC_IDR_Addr  (GPIOC_BASE + GPIO_IDR_OFFSET)
#define GPIOD_IDR_Addr  (GPIOD_BASE + GPIO_IDR_OFFSET)
#define GPIOE_IDR_Addr  (GPIOE_BASE + GPIO_IDR_OFFSET)
#define GPIOF_IDR_Addr  (GPIOF_BASE + GPIO_IDR_OFFSET)
#define GPIOG_IDR_Addr  (GPIOG_BASE + GPIO_IDR_OFFSET)

#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)
#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)
#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)
#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)
#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)
#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)
#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)

#endif
