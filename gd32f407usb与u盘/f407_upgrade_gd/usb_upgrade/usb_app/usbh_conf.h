/**
 * @file usbh_conf.h
 * @brief GD32F4xx_usb_library Host 配置
 */

#ifndef USBH_CONF_H
#define USBH_CONF_H

#define USBH_MAX_EP_NUM                         2U
#define USBH_MAX_INTERFACES_NUM                 2U
#define USBH_MAX_ALT_SETTING                    2U
#define USBH_MAX_SUPPORTED_CLASS                2U

#define USBH_DATA_BUF_MAX_LEN                   0x200U
#define USBH_CFGSET_MAX_LEN                     0x200U

#endif /* USBH_CONF_H */
