/**
  * @file    usbd_conf.h
  * @brief   USB Device 配置
  *          来自正点原子 STM32F407 USB读卡器例程
  */

#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__

#include "usb_conf.h"

#define USBD_CFG_MAX_NUM            1
#define USBD_ITF_MAX_NUM            1
#define USB_MAX_STR_DESC_SIZ        64

#define USBD_SELF_POWERED

/* Class Layer Parameter */
#define MSC_IN_EP                   0x81
#define MSC_OUT_EP                  0x01
#ifdef USE_USB_OTG_HS
#ifdef USE_ULPI_PHY
#define MSC_MAX_PACKET              512
#else
#define MSC_MAX_PACKET              64
#endif
#else  /*USE_USB_OTG_FS*/
#define MSC_MAX_PACKET              64
#endif

#define MSC_MEDIA_PACKET            12*1024

#endif /* __USBD_CONF__H__ */
