/**
  ******************************************************************************
  * @file    stm32f10x_it.c
  * @brief   Main Interrupt Service Routines
  ******************************************************************************
  */

#include "stm32f10x_it.h"
#include "config.h"   // for printf / NVIC_SystemReset

/* Cortex-M3 Processor Exceptions Handlers */

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    __disable_irq();
    printf("\r\n[BOOT] HardFault! Resetting...\r\n");
    NVIC_SystemReset();
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

// SysTick_Handler is handled by Delay module

