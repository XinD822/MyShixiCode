/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  */

#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_tim.h"

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    /* 点亮 LED1 (PF9) 指示 HardFault — 正点原子 F407 LED 低电平点亮 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
    GPIOF->MODER |= (1U << (9 * 2));   /* PF9 输出模式 */
    GPIOF->OTYPER &= ~(1U << 9);       /* 推挽 */
    GPIOF->ODR &= ~(1U << 9);          /* 输出低电平，点亮 LED1 */

    while (1) {}
}

void MemManage_Handler(void)
{
    while (1) {}
}

void BusFault_Handler(void)
{
    while (1) {}
}

void UsageFault_Handler(void)
{
    while (1) {}
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
}

/* TIM3_IRQHandler 已在 st_tick.c 中定义 */
