/*!
    \file    gd32f4xx_it.c
    \brief   interrupt service routines
*/

#include "gd32f4xx_it.h"

/*!< this function handles NMI exception */
void NMI_Handler(void)
{
}

/*!< this function handles HardFault exception */
void HardFault_Handler(void)
{
    while(1);
}

/*!< this function handles MemManage exception */
void MemManage_Handler(void)
{
    while(1);
}

/*!< this function handles BusFault exception */
void BusFault_Handler(void)
{
    while(1);
}

/*!< this function handles UsageFault exception */
void UsageFault_Handler(void)
{
    while(1);
}

/*!< this function handles SVC exception */
void SVC_Handler(void)
{
}

/*!< this function handles DebugMon exception */
void DebugMon_Handler(void)
{
}

/*!< this function handles PendSV_Handler exception */
void PendSV_Handler(void)
{
}

/* SysTick_Handler 已迁移到 plat_tick_systick.c，
 * 由平台抽象层统一实现（驱动 tick 计数 + timer 回调）。 */
