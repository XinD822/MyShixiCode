/**
  * @file    usb_bsp.c
  * @brief   USB OTG FS BSP 层（STM32F407）
  *          基于正点原子例程，适配本工程延时函数
  */

#include "usb_bsp.h"
#include "usb_dcd_int.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "misc.h"
#include "board_config.h"
#include "config.h"

/* 延时函数声明（定义在 delay.c） */
extern void delay_us(uint32_t us);
extern void delay_ms(uint32_t ms);

void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);

  /* PA11=OTG_FS_DM, PA12=OTG_FS_DP */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* 原理图确认：VBUS 直连 USB_5V（常开），PA15 未接 USB 电源控制。
   * 正点原子例程虽然配置 PA15，但本板硬件没有该电源开关电路。
   * 不配置 PA15，避免干扰其他可能的 PA15 功能。 */

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG_FS);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG_FS);
}

void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
  NVIC_InitTypeDef   NVIC_InitStructure;
  (void)pdev;
  NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void USB_OTG_BSP_DisableInterrupt(void)
{
  NVIC_DisableIRQ(OTG_FS_IRQn);
}

void USB_OTG_BSP_DriveVBUS(USB_OTG_CORE_HANDLE *pdev, uint8_t state)
{
  (void)pdev;
  (void)state;
}

void  USB_OTG_BSP_ConfigVBUS(USB_OTG_CORE_HANDLE *pdev)
{
  (void)pdev;
}

void USB_OTG_BSP_uDelay (const uint32_t usec)
{
  delay_us(usec);
}

void USB_OTG_BSP_mDelay (const uint32_t msec)
{
  delay_ms(msec);
}
