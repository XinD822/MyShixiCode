/**
 * @file st_pwr.c
 * @brief STM32 电源 / 复位 / 中断控制实现
 */

#include "hal_config.h"

#ifdef PLATFORM_STM32

static void st_pwr_disable_irq(void)    { __disable_irq(); }
static void st_pwr_enable_irq(void)     { __set_PRIMASK(0); }
static void st_pwr_set_msp(uint32_t a)  { __set_MSP(a); }

static void st_pwr_set_vtor(uint32_t addr)
{
    SCB->VTOR = addr;
}

static void st_pwr_system_reset(void)
{
    NVIC_SystemReset();
}

static void st_pwr_nvic_priority_group(uint32_t group)
{
    NVIC_PriorityGroupConfig(group);
}

const HAL_Pwr_Drv_t ST_Pwr_Drv = {
    .disable_irq        = st_pwr_disable_irq,
    .enable_irq         = st_pwr_enable_irq,
    .set_msp            = st_pwr_set_msp,
    .set_vtor           = st_pwr_set_vtor,
    .system_reset       = st_pwr_system_reset,
    .nvic_priority_group = st_pwr_nvic_priority_group,
};

const HAL_Pwr_Drv_t *HAL_Pwr = &ST_Pwr_Drv;

#endif /* PLATFORM_STM32 */
