/**
  * @file    usbd_conf.h
  * @brief   GD32F407 USB 设备 (MSC) 配置
  *          适配 GD32F4xx_usb_library — usbd_msc_core/bbb/scsi
  */

#ifndef __USBD_CONF_H
#define __USBD_CONF_H

#include "usb_conf.h"

#define USBD_CFG_MAX_NUM                    1
#define USBD_ITF_MAX_NUM                    1

#define USBD_MSC_INTERFACE                  0

#define USB_STR_DESC_MAX_SIZE               255

/* MSC 端点（全速：IN=EP1, OUT=EP2） */
#define MSC_IN_EP                           EP1_IN
#define MSC_OUT_EP                          EP2_OUT

/* MSC BOT 数据包大小（全速端点，最大 64 字节） */
#define MSC_DATA_PACKET_SIZE                64

/* 单次介质搬运缓冲（BBB 传输缓冲，字节）
   512B×8 扇区 = 4KB，用于 bbb_data */
#define MSC_MEDIA_PACKET_SIZE               4096

/* 存储介质 LUN 数量（单 LUN） */
#define MEM_LUN_NUM                         1

#endif /* __USBD_CONF_H */
