/*
 * CherryUSB Configuration for STM32F103 + MSC
 * 
 * 切换裸机/uCOS-II模式：
 *   方式1：取消注释下面的 CONFIG_USB_USE_UCOS2
 *   方式2：编译器预定义 -DCONFIG_USB_USE_UCOS2
 */

#ifndef USB_CONFIG_H
#define USB_CONFIG_H

/* ══════════════════════════════════════════════════════════════
 * 模式选择：注释掉 = 裸机，取消注释 = uCOS-II
 * ══════════════════════════════════════════════════════════════ */
// #define CONFIG_USB_USE_UCOS2

/* ══════════════════════════════════════════════════════════════
 * RTOS 相关配置（仅 CONFIG_USB_USE_UCOS2 时生效）
 * ══════════════════════════════════════════════════════════════ */
#ifdef CONFIG_USB_USE_UCOS2

/* EP0 和 MSC 使用独立线程处理 */
#define CONFIG_USBDEV_EP0_THREAD
#define CONFIG_USBDEV_MSC_THREAD

/*
 * uCOS-II 静态栈大小（单位：OS_STK 个数，1个 = 4字节）
 * 这些值仅用于 usb_osal.c 中的静态栈数组
 * CherryUSB 内部的 CONFIG_USBDEV_EP0_STACKSIZE 等是字节数，不要混淆
 */
#define USB_UCOS_EP0_STK_SIZE   512   /* EP0 栈 = 2KB */
#define USB_UCOS_MSC_STK_SIZE   512   /* MSC 栈 = 2KB */

#endif /* CONFIG_USB_USE_UCOS2 */

/* ══════════════════════════════════════════════════════════════
 * USB 通用配置
 * ══════════════════════════════════════════════════════════════ */

/* STM32F103 没有 DMA cache */
#ifndef USB_NOCACHE_RAM_SECTION
#define USB_NOCACHE_RAM_SECTION
#endif

/* 调试输出 */
#define CONFIG_USB_PRINTF(...) printf(__VA_ARGS__)

/* 调试级别 */
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO

/* 数据对齐 */
#define CONFIG_USB_ALIGN_SIZE 4

/* 不使用自定义 memcpy */
#define CONFIG_USB_MEMCPY_DISABLE

/* ══════════════════════════════════════════════════════════════
 * USB 设备栈配置
 * ══════════════════════════════════════════════════════════════ */

/* EP0 缓冲区大小 */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 64

/* 使用高级描述符注册 API */
#define CONFIG_USBDEV_ADVANCE_DESC

/* EP0 线程配置（CherryUSB 内部用，单位：字节） */
#ifndef CONFIG_USBDEV_EP0_PRIO
#define CONFIG_USBDEV_EP0_PRIO          4
#endif
#ifndef CONFIG_USBDEV_EP0_STACKSIZE
#define CONFIG_USBDEV_EP0_STACKSIZE     2048
#endif

/* ══════════════════════════════════════════════════════════════
 * MSC 配置
 * ══════════════════════════════════════════════════════════════ */

#define CONFIG_USBDEV_MSC_MAX_LUN       1
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE   512

#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING "STM32"
#define CONFIG_USBDEV_MSC_PRODUCT_STRING      "USB Mass Storage"
#define CONFIG_USBDEV_MSC_VERSION_STRING      "1.00"

/* MSC 线程配置（CherryUSB 内部用，单位：字节） */
#ifndef CONFIG_USBDEV_MSC_PRIO
#define CONFIG_USBDEV_MSC_PRIO          6
#endif
#ifndef CONFIG_USBDEV_MSC_STACKSIZE
#define CONFIG_USBDEV_MSC_STACKSIZE     2048
#endif

/* ══════════════════════════════════════════════════════════════
 * USB 端口配置
 * ══════════════════════════════════════════════════════════════ */

#define CONFIG_USBDEV_MAX_BUS           1
#define CONFIG_USBDEV_FSDEV_PMA_ACCESS  2

#endif /* USB_CONFIG_H */
