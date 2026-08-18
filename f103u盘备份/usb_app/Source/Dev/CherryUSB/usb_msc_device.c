/*
 * CherryUSB MSC Device for STM32F103 + W25Q128
 * Connects CherryUSB MSC to existing mass_mal.c
 */

#include "usbd_core.h"
#include "usbd_msc.h"
#include "upgrade_config.h"
#include "mass_mal.h"

/* MAL 层提供的容量信息（定义在 mass_mal.c） */
extern uint32_t Mass_Block_Count[];
extern uint32_t Mass_Block_Size[];

/* 兼容旧版 system.c 中的超时变量 */
volatile uint32_t g_last_scsi_tick = 0;

/* ──── 端点配置（和你原来的一样） ──── */
#define MSC_IN_EP  0x81
#define MSC_OUT_EP 0x02

/* ──── USB设备信息 ──── */
#define USBD_VID           0x0483
#define USBD_PID           0x5720
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define USB_CONFIG_SIZE (9 + MSC_DESCRIPTOR_LEN)

#define MSC_MAX_MPS 64

/* ──── 描述符定义（CherryUSB宏生成） ──── */

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01)
};

static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MSC_DESCRIPTOR_INIT(0x00, MSC_OUT_EP, MSC_IN_EP, MSC_MAX_MPS, 0x02)
};

static const uint8_t device_quality_descriptor[] = {
    0x0a, USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 },  /* Langid */
    "STMicroelectronics",           /* Manufacturer */
    "STM32 Mass Storage",           /* Product */
    "STM32F103",                    /* Serial Number */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    return device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    if (index >= (sizeof(string_descriptors) / sizeof(char *))) {
        return NULL;
    }
    return string_descriptors[index];
}

const struct usb_descriptor msc_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback
};

/* ──── USB事件回调 ──── */

static volatile uint8_t usb_configured = 0;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
        case USBD_EVENT_CONFIGURED:
            usb_configured = 1;
            break;
        case USBD_EVENT_DISCONNECTED:
            usb_configured = 0;
            break;
        default:
            break;
    }
}

/* ──── MSC回调函数（对接你现有的mass_mal.c） ──── */

void usbd_msc_get_cap(uint8_t busid, uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
    *block_num = Mass_Block_Count[lun];
    *block_size = Mass_Block_Size[lun];
}

int usbd_msc_sector_read(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    /* sector是扇区号，需要转成字节偏移 */
    MAL_Read(lun, sector * 512, (uint32_t*)buffer, (uint16_t)length);
    return 0;
}

int usbd_msc_sector_write(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    /* sector是扇区号，需要转成字节偏移 */
    MAL_Write(lun, sector * 512, (uint32_t*)buffer, (uint16_t)length);
    return 0;
}

/* ──── 初始化函数 ──── */

static struct usbd_interface intf0;

void usb_msc_device_init(void)
{
    MAL_Init(0);
    MAL_GetStatus(0);
    
    usbd_desc_register(0, &msc_descriptor);
    usbd_add_interface(0, usbd_msc_init_intf(0, &intf0, MSC_OUT_EP, MSC_IN_EP));
    usbd_initialize(0, 0x40005C00, usbd_event_handler);
}

/* ──── USB状态查询 ──── */

uint8_t usb_msc_is_configured(void)
{
    return usb_configured;
}

/* ──── USB关闭（断开后调用，释放SPI总线） ──── */

extern void usb_dc_low_level_deinit(uint8_t busid);

void usb_msc_device_deinit(void)
{
    usb_dc_low_level_deinit(0);
    usb_configured = 0;
}
