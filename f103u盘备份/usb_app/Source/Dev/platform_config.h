/**
  ******************************************************************************
  * @file    platform_config.h
  * @brief   Platform configuration for mass_mal.c compatibility
  ******************************************************************************
  */

#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

#include "stm32f10x.h"
#include <stdio.h>
#include "usart.h"
#include "spi_flash.h"

#define ID1 (0x1FFFF7E8)
#define ID2 (0x1FFFF7EC)
#define ID3 (0x1FFFF7F0)

#endif /* __PLATFORM_CONFIG_H */
