/**
 * @file platform.c
 * @brief 平台底层实现（电源/复位/中断控制 + USB 时钟）
 *
 * 合并原 st_pwr.c + st_usb.c 的底层功能，移除函数指针封装层。
 */

#include "board_config.h"
#include "platform.h"
#include "debug_uart.h"   /* Debug_UART_SendString (诊断打印，定位完成后移除) */

/* ──── 中断 / 复位 / VTOR ──── */

void Platform_DisableIRQ(void)  { __disable_irq(); }
void Platform_EnableIRQ(void)   { __set_PRIMASK(0); }
void Platform_SetMSP(uint32_t addr) { __set_MSP(addr); }

void Platform_SetVTOR(uint32_t addr)
{
    SCB->VTOR = addr;
}

void Platform_SystemReset(void)
{
    /* ──── 复位策略：NVIC 软复位 + IWDG 硬件兜底 ────
     * 临时诊断版：每步打印，定位卡死位置。
     * 定位完成后移除打印。 */

    /* 0) 打印进入函数（此时中断还开着，UART 能正常发） */
    Debug_UART_SendString("[RST] enter\r\n");

    /* 1) 关 OTG_FS 中断，避免复位瞬间 USB 中断误触发 */
    NVIC_DisableIRQ(OTG_FS_IRQn);

    /* 2) 关全局中断，保证后续寄存器写入不被打断 */
    __disable_irq();
    Debug_UART_SendString("[RST] irq off\r\n");

    /* 2.5) 彻底硬件复位 USB OTG FS 外设
     * DEVICE 模式下 USB OTG FS 与电脑保持连接、外设处于活跃状态，
     * 直接软复位可能因 OTG FS 内部状态导致 SYSRESETREQ 不生效
     * （表现为卡在 resetting...）。用 RCC_AHB2RSTR 复位整个 OTG FS
     * 模块到上电默认状态，等同于"冷启动 USB"，软复位即可生效。 */
    RCC->AHB2RSTR |=  RCC_AHB2RSTR_OTGFSRST;   /* 置位复位 OTG FS */
    for (volatile uint32_t i = 0; i < 1000; i++) __NOP();
    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_OTGFSRST;   /* 清除复位，释放外设 */
    for (volatile uint32_t i = 0; i < 1000; i++) __NOP();
    Debug_UART_SendString("[RST] otg reset done\r\n");

    /* 3) 启动 IWDG 硬件兜底（寄存器直操作，零库依赖）
     *    LSI = 32kHz，预分频 32 → 计数器时钟 1kHz，
     *    重载值 300 → 超时 ≈ 300ms。 */
    /* 强制 LSI 重新起振：先关→等停→再开
     * 防止 LSION 已为1时写入无效，LSIRDY 是残留标志而非真实状态 */
    RCC->CSR &= ~RCC_CSR_LSION;
    {
        uint32_t timeout = 100000;
        while (timeout--) __NOP();
    }
    RCC->CSR |= RCC_CSR_LSION;
    /* 等 LSI 真正从 0→1 起振完成 */
    {
        uint32_t timeout = 100000;
        while ((RCC->CSR & RCC_CSR_LSIRDY) == 0) {
            if (--timeout == 0) break;
            __NOP();
        }
    }
    Debug_UART_SendString("[RST] lsi ready\r\n");
    /* 清除 DBGMCU_APB1FZ 的 IWDG 冻结位(bit12)。 */
    DBGMCU->APB1FZ &= ~DBGMCU_APB1_FZ_DBG_IWDG_STOP;

    /* IWDG 配置（带重试）：
     * 0x5555 解锁命令需同步到 LSI 域（32kHz，周期≈31µs）。
     * AHB2RSTR 复位 OTG FS 后总线可能不稳定，解锁/写入可能被丢弃。
     * 解决：写入后读回验证，失败则重试整个解锁+写入流程。 */
    {
        const uint8_t target_pr = 0x03;    /* 预分频 /32 */
        const uint16_t target_rlr = 300;   /* 重载值 300 (≈300ms) */
        int ok = 0;

        for (int attempt = 0; attempt < 10 && !ok; attempt++) {
            /* 解锁 */
            IWDG->KR = 0x5555;
            for (volatile uint32_t i = 0; i < 40000; i++) __NOP();

            /* 写 PR */
            IWDG->PR = target_pr;
            {
                uint32_t to = 200000;
                while ((IWDG->SR & 0x01) && --to) __NOP();
            }

            /* 写 RLR */
            IWDG->RLR = target_rlr;
            {
                uint32_t to = 200000;
                while ((IWDG->SR & 0x02) && --to) __NOP();
            }

            /* 读回验证 */
            if (IWDG->PR == target_pr && IWDG->RLR == target_rlr) {
                ok = 1;
            }
        }

        if (!ok) {
            Debug_UART_SendString("[RST] WARN: IWDG config failed after 10 attempts\r\n");
        }
    }

    IWDG->KR  = 0xAAAA;                             /* 装载重载值到计数器 */
    for (volatile uint32_t i = 0; i < 40000; i++) __NOP();
    IWDG->KR  = 0xCCCC;                             /* 启动看门狗（此后必在 300ms 内复位）*/

    /* 诊断：读 IWDG 寄存器，确认配置生效 */
    {
        static char buf[40];
        uint32_t sr = IWDG->SR;   /* bit0=PVU bit1=RVU — 若非0说明上次写还没同步完 */
        uint32_t pr = IWDG->PR;
        uint32_t rlr = IWDG->RLR;
        const char *hex = "0123456789ABCDEF";
        int p = 0;
        buf[p++]='['; buf[p++]='R'; buf[p++]='S'; buf[p++]='T'; buf[p++]=']';
        buf[p++]=' '; buf[p++]='S'; buf[p++]='R'; buf[p++]='='; buf[p++]=hex[(sr>>4)&0xF];
        buf[p++]=' '; buf[p++]='P'; buf[p++]='R'; buf[p++]='='; buf[p++]=hex[pr&0xF];
        buf[p++]=' '; buf[p++]='R'; buf[p++]='L'; buf[p++]='R'; buf[p++]='='; buf[p++]=hex[(rlr>>8)&0xF]; buf[p++]=hex[(rlr>>4)&0xF]; buf[p++]=hex[rlr&0xF];
        buf[p++]='\r'; buf[p++]='\n';
        for (int i=0;i<p;i++) Debug_UART_SendByte((uint8_t)buf[i]);
    }
    Debug_UART_SendString("[RST] iwdg started\r\n");

    /* 4) 不再用 NVIC_SystemReset()（DEVICE 模式下不生效），
     *    直接靠 IWDG 超时触发硬件复位。等 IWDG 计数到 0（约300ms）。 */
    Debug_UART_SendString("[RST] wait iwdg timeout...\r\n");
    while (1) {
        __NOP();
    }
}

void Platform_NVICPriorityGroup(uint32_t group)
{
    NVIC_PriorityGroupConfig(group);
}

/* ──── USB 时钟 / NVIC ──── */

#if defined(CHIP_SERIES_F103)

void Platform_USB_ClockEnable(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

void Platform_USB_ClockDisable(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE);
}

void Platform_USB_NVICEnable(void)
{
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

void Platform_USB_NVICDisable(void)
{
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
}

#elif defined(CHIP_SERIES_F407)

void Platform_USB_ClockEnable(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);
}

void Platform_USB_ClockDisable(void)
{
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, DISABLE);
}

void Platform_USB_NVICEnable(void)
{
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = OTG_FS_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

void Platform_USB_NVICDisable(void)
{
    NVIC_DisableIRQ(OTG_FS_IRQn);
}

/**
 * @brief HAL_PCD_MspInit - USB 设备模式硬件初始化
 * @note  usb_glue_st.c:usb_dc_low_level_init() 调用此函数
 */
void HAL_PCD_MspInit(void *hpcd)
{
    (void)hpcd;

    GPIO_InitTypeDef GPIO_InitStruct;

    Platform_USB_ClockEnable();

    /* PA11=OTG_FS_DM, PA12=OTG_FS_DP */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG_FS);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG_FS);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    Platform_USB_NVICEnable();
}

/**
 * @brief HAL_PCD_MspDeInit - USB 设备模式硬件反初始化
 */
void HAL_PCD_MspDeInit(void *hpcd)
{
    (void)hpcd;
    Platform_USB_NVICDisable();
    Platform_USB_ClockDisable();
}

#endif /* CHIP_SERIES_xxx */
