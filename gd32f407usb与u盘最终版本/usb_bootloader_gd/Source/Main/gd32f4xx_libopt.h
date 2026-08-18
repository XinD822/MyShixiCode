/*!
    \file    gd32f4xx_libopt.h
    \brief   library configuration for GD32F4xx bootloader
    \note    gd32f4xx.h 中已 `#include "gd32f4xx_libopt.h"`（在 USE_STDPERIPH_DRIVER 下）。
             本文件由 bootloader 工程维护，只包含用到的外设驱动头文件。
*/

#ifndef GD32F4XX_LIBOPT_H
#define GD32F4XX_LIBOPT_H

#if defined (GD32F407) || defined (GD32F427)
    #include "gd32f4xx_fmc.h"
    #include "gd32f4xx_gpio.h"
    #include "gd32f4xx_misc.h"
    #include "gd32f4xx_pmu.h"
    #include "gd32f4xx_rcu.h"
    #include "gd32f4xx_timer.h"
    #include "gd32f4xx_usart.h"
#endif

#endif /* GD32F4XX_LIBOPT_H */
