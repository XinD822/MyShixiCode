/*!
    \file    gd32f4xx_libopt.h
    \brief   library configuration for GD32F4xx - user select which peripheral drivers to include

    \attention
        gd32f4xx_standard_peripheral.h 在 gd32f4xx.h 中被引用，本文件由用户维护。
        默认使能全部标准外设驱动头文件。
*/

#ifndef GD32F4XX_LIBOPT_H
#define GD32F4XX_LIBOPT_H

/* device selection
 * 说明: GD32F407 宏已在 Keil 编译选项 Define 中定义 (见 .uvprojx <Define>GD32F407</Define>)，
 *       此处无需重复定义，避免重复宏告警。
 *       若用 GCC 直接编译，请去掉下面一行的注释。
 */
/* #define GD32F407 */

#if defined (GD32F407) || defined (GD32F427)
    #include "gd32f4xx_adc.h"
    #include "gd32f4xx_can.h"
    #include "gd32f4xx_crc.h"
    #include "gd32f4xx_ctc.h"
    #include "gd32f4xx_dac.h"
    #include "gd32f4xx_dbg.h"
    #include "gd32f4xx_dci.h"
    #include "gd32f4xx_dma.h"
    #include "gd32f4xx_enet.h"
    #include "gd32f4xx_exmc.h"
    #include "gd32f4xx_exti.h"
    #include "gd32f4xx_fmc.h"
    #include "gd32f4xx_fwdgt.h"
    #include "gd32f4xx_gpio.h"
    #include "gd32f4xx_i2c.h"
    #include "gd32f4xx_ipa.h"
    #include "gd32f4xx_iref.h"
    #include "gd32f4xx_misc.h"
    #include "gd32f4xx_pmu.h"
    #include "gd32f4xx_rcu.h"
    #include "gd32f4xx_rtc.h"
    #include "gd32f4xx_sdio.h"
    #include "gd32f4xx_spi.h"
    #include "gd32f4xx_syscfg.h"
    #include "gd32f4xx_timer.h"
    #include "gd32f4xx_tli.h"
    #include "gd32f4xx_trng.h"
    #include "gd32f4xx_usart.h"
    #include "gd32f4xx_wwdgt.h"

    /* optionally disable unused drivers to save flash/ram:
       #undef / 或注释掉对应 #include 即可 */
#endif

#endif /* __GD32F4XX_LIBOPT_H */
