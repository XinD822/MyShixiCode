#ifndef __KEY_H
#define __KEY_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////
// 本代码仅供学习使用，未经允许，不得用于任何其他用途
// ALIENTEK STM32F407 开发板
// 按键输入实验
// 正点原子@ALIENTEK
// 技术论坛:www.openedv.com
// 创建日期:2014/5/3
// 版本：V1.0
// 版权所有，盗版必究
// Copyright(C) 广州市星翼电子科技有限公司 2014-2024
// All rights reserved
//////////////////////////////////////////////////////////////////////////////////

/* 按键读取方式一：直接调用库函数方式读取 IO */
#define KEY0 		gpio_input_bit_get(GPIOE,GPIO_PIN_4) //PE4
#define WK_UP 		gpio_input_bit_get(GPIOA,GPIO_PIN_0)	//PA0
#define KEY1 		gpio_input_bit_get(GPIOE,GPIO_PIN_3)	//PE3
#define KEY2 		gpio_input_bit_get(GPIOE,GPIO_PIN_2) //PE2

/* 按键读取方式二：通过位带操作方式读取 IO（备选） */
/*
#define KEY0 		PEin(4)   	//PE4
#define KEY1 		PEin(3)		//PE3
#define KEY2 		PEin(2)		//PE2
#define WK_UP 	PAin(0)		//PA0
*/


#define KEY0_PRES 	1	//KEY0 按下
#define KEY1_PRES	2	//KEY1 按下
#define KEY2_PRES	3	//KEY2 按下
#define WKUP_PRES   4	//KEY_UP 按下(即 WK_UP)

void KEY_Init(void);	// IO 初始化
u8 KEY_Scan(u8);  		// 按键扫描函数

#endif
