/*
 * CherryUSB Configuration for STM32F103 + MSC
 * Baremetal mode (no RTOS)
 */

#ifndef USB_CONFIG_H
#define USB_CONFIG_H

/* STM32F103 没有 DMA cache，无需特殊 no-cache 段 */
#ifndef USB_NOCACHE_RAM_SECTION
#define USB_NOCACHE_RAM_SECTION
#endif

/* ================ USB通用配置 ================ */

// 调试输出
#define CONFIG_USB_PRINTF(...) printf(__VA_ARGS__)

// 调试级别
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO

// 数据对齐（裸机不需要DMA对齐）
#define CONFIG_USB_ALIGN_SIZE 4

// 不使用自定义memcpy
#define CONFIG_USB_MEMCPY_DISABLE

/* ================ USB设备栈配置 ================ */

// EP0缓冲区大小
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 64

// 使用高级描述符注册API
#define CONFIG_USBDEV_ADVANCE_DESC

// 不使用EP0线程（裸机模式）
// #define CONFIG_USBDEV_EP0_THREAD

/* ================ MSC配置 ================ */

// 最大逻辑单元数（只有1个W25Q128）
#define CONFIG_USBDEV_MSC_MAX_LUN 1

// MSC缓冲区大小
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE 512

// MSC字符串
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING "STM32"
#define CONFIG_USBDEV_MSC_PRODUCT_STRING "USB Mass Storage"
#define CONFIG_USBDEV_MSC_VERSION_STRING "1.00"

// 不使用MSC线程（裸机模式）
// #define CONFIG_USBDEV_MSC_THREAD

// 不使用MSC轮询模式
// #define CONFIG_USBDEV_MSC_POLLING

/* ================ USB端口配置 ================ */

// 最大USB设备数
#define CONFIG_USBDEV_MAX_BUS 1

// 不使能SOF
// #define CONFIG_USBDEV_SOF_ENABLE

// FSDEV配置（STM32F103使用，PMA访问模式2）
#define CONFIG_USBDEV_FSDEV_PMA_ACCESS 2

#endif /* USB_CONFIG_H */
