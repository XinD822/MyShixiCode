/**
 * @file usbh_usr.c
 * @brief USB Host 用户回调 + U盘读写封装
 *
 * 基于正点原子 USB Host 例程适配：
 * - 用 DBG_PRINTF 替代 printf
 * - 中断模式：OTG_FS_IRQHandler 自动处理 USB 中断，
 *   读写循环中仅调用 USBH_MSC_HandleBOTXfer 轮询 URB 状态
 * - 不定义 OTG_FS_IRQHandler（已在 usbd_usr.c 中处理）
 */

#include "usbh_usr.h"
#include "usbh_msc_scsi.h"
#include "usbh_msc_bot.h"
#include "usb_hcd_int.h"    /* USBH_OTG_ISR_Handler */
#include "usb_host_task.h"  /* USB_Host_Do_Upgrade */
#include "config.h"

/* USB OTG 核心和 Host 状态机（定义在 usb_host_task.c） */
extern USB_OTG_CORE_HANDLE USB_OTG_Core;
extern USBH_HOST           USB_Host;

/* U盘就绪标志和重试计数 */
volatile uint8_t g_udisk_ready = 0;
static uint8_t g_retry_count = 0;
#define UPGRADE_MAX_RETRIES  3

/* ──── Host 用户回调函数前向声明 ──── */
void USBH_USR_Init(void);
void USBH_USR_DeInit(void);
void USBH_USR_DeviceAttached(void);
void USBH_USR_ResetDevice(void);
void USBH_USR_DeviceDisconnected(void);
void USBH_USR_OverCurrentDetected(void);
void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed);
void USBH_USR_Device_DescAvailable(void *DeviceDesc);
void USBH_USR_DeviceAddressAssigned(void);
void USBH_USR_Configuration_DescAvailable(USBH_CfgDesc_TypeDef *cfgDesc,
                                          USBH_InterfaceDesc_TypeDef *itfDesc,
                                          USBH_EpDesc_TypeDef *epDesc);
void USBH_USR_Manufacturer_String(void *ManufacturerString);
void USBH_USR_Product_String(void *ProductString);
void USBH_USR_SerialNum_String(void *SerialNumString);
void USBH_USR_EnumerationDone(void);
USBH_USR_Status USBH_USR_UserInput(void);
int USBH_USR_MSC_Application(void);
void USBH_USR_DeviceNotSupported(void);
void USBH_USR_UnrecoveredError(void);

/* ──── Host 用户回调表 ──── */
USBH_Usr_cb_TypeDef USR_Callbacks = {
    USBH_USR_Init,
    USBH_USR_DeInit,
    USBH_USR_DeviceAttached,
    USBH_USR_ResetDevice,
    USBH_USR_DeviceDisconnected,
    USBH_USR_OverCurrentDetected,
    USBH_USR_DeviceSpeedDetected,
    USBH_USR_Device_DescAvailable,
    USBH_USR_DeviceAddressAssigned,
    USBH_USR_Configuration_DescAvailable,
    USBH_USR_Manufacturer_String,
    USBH_USR_Product_String,
    USBH_USR_SerialNum_String,
    USBH_USR_EnumerationDone,
    USBH_USR_UserInput,
    USBH_USR_MSC_Application,
    USBH_USR_DeviceNotSupported,
    USBH_USR_UnrecoveredError
};

/* ──── 回调函数实现 ──── */

void USBH_USR_Init(void)
{
    DBG_PRINTF("[HOST] USB Host Library v2.1.0\r\n");
}

void USBH_USR_DeInit(void)
{
    g_udisk_ready = 0;
    g_retry_count = 0;  /* 设备断开时重置重试计数 */
}

void USBH_USR_DeviceAttached(void)
{
    DBG_PRINTF("[HOST] Device attached\r\n");
}

void USBH_USR_DeviceDisconnected(void)
{
    DBG_PRINTF("[HOST] Device disconnected\r\n");
    g_udisk_ready = 0;
}

void USBH_USR_ResetDevice(void)
{
    DBG_PRINTF("[HOST] Reset device\r\n");
}

void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed)
{
    if (DeviceSpeed == HPRT0_PRTSPD_HIGH_SPEED)
        DBG_PRINTF("[HOST] High speed device\r\n");
    else if (DeviceSpeed == HPRT0_PRTSPD_FULL_SPEED)
        DBG_PRINTF("[HOST] Full speed device\r\n");
    else if (DeviceSpeed == HPRT0_PRTSPD_LOW_SPEED)
        DBG_PRINTF("[HOST] Low speed device\r\n");
    else
        DBG_PRINTF("[HOST] Device error (speed)\r\n");
}

void USBH_USR_Device_DescAvailable(void *DeviceDesc)
{
    USBH_DevDesc_TypeDef *hs = (USBH_DevDesc_TypeDef *)DeviceDesc;
    DBG_PRINTF("[HOST] VID: %04X, PID: %04X\r\n",
               (uint32_t)hs->idVendor, (uint32_t)hs->idProduct);
}

void USBH_USR_DeviceAddressAssigned(void)
{
    DBG_PRINTF("[HOST] Address assigned\r\n");
}

void USBH_USR_Configuration_DescAvailable(USBH_CfgDesc_TypeDef *cfgDesc,
                                          USBH_InterfaceDesc_TypeDef *itfDesc,
                                          USBH_EpDesc_TypeDef *epDesc)
{
    (void)cfgDesc;
    (void)epDesc;
    if (itfDesc->bInterfaceClass == 0x08)
        DBG_PRINTF("[HOST] MSC device (mass storage)\r\n");
    else
        DBG_PRINTF("[HOST] Interface class: 0x%02X\r\n", itfDesc->bInterfaceClass);
}

void USBH_USR_Manufacturer_String(void *ManufacturerString)
{
    (void)ManufacturerString;
}

void USBH_USR_Product_String(void *ProductString)
{
    (void)ProductString;
}

void USBH_USR_SerialNum_String(void *SerialNumString)
{
    (void)SerialNumString;
}

void USBH_USR_EnumerationDone(void)
{
    DBG_PRINTF("[HOST] Enumeration done\r\n");
}

USBH_USR_Status USBH_USR_UserInput(void)
{
    return USBH_USR_RESP_OK;
}

void USBH_USR_OverCurrentDetected(void)
{
    DBG_PRINTF("[HOST] Over current!\r\n");
}

void USBH_USR_DeviceNotSupported(void)
{
    DBG_PRINTF("[HOST] Device not supported\r\n");
}

void USBH_USR_UnrecoveredError(void)
{
    DBG_PRINTF("[HOST] Unrecovered error\r\n");
}

/**
 * @brief MSC 类应用回调 — U盘枚举成功后被反复调用
 *        首次调用时执行升级流程（挂载 FatFs、检测 firmware.bin、升级），
 *        升级成功后系统会自动复位；失败则允许重试（最多 UPGRADE_MAX_RETRIES 次）。
 *        设备断开重连后重试计数清零。
 */
int USBH_USR_MSC_Application(void)
{
    if (!g_udisk_ready) {
        g_udisk_ready = 1;
        DBG_PRINTF("[HOST] MSC_Application: attempt %d\r\n", g_retry_count + 1);
        USB_Host_Do_Upgrade();
        /* 如果走到这里说明升级失败了（成功路径会复位MCU） */
        g_retry_count++;
        if (g_retry_count < UPGRADE_MAX_RETRIES) {
            DBG_PRINTF("[HOST] upgrade failed, will retry (%d/%d)\r\n",
                       g_retry_count, UPGRADE_MAX_RETRIES);
            g_udisk_ready = 0;  /* 允许下次重试 */
        } else {
            DBG_PRINTF("[HOST] upgrade failed after %d attempts, giving up\r\n",
                       UPGRADE_MAX_RETRIES);
        }
    }
    return 0;
}

/* ──── U盘读写封装（供 diskio.c 调用） ────
 * 纯硬件中断模式：OTG_FS_IRQHandler 自动处理 USB 事件（读取 Rx FIFO、
 * 更新 URB 状态），读写循环中仅调用 USBH_MSC_HandleBOTXfer 轮询 URB
 * 状态推进 BOT 状态机。
 *
 * 不禁用 OTG_FS 中断，不手动调用 USBH_OTG_ISR_Handler ——
 * 与正点原子 Host 例程一致，避免手动 ISR 与 BOT 状态机时序不同步
 * 导致 Rx FIFO 数据错位（boot sector 读出全零）。 */

uint8_t USBH_UDISK_Status(void)
{
    return (uint8_t)HCD_IsDeviceConnected(&USB_OTG_Core);
}

uint8_t USBH_UDISK_Read(uint8_t *buf, uint32_t sector, uint32_t cnt)
{
    uint8_t res = 1;

    if (HCD_IsDeviceConnected(&USB_OTG_Core)) {
        do {
            res = USBH_MSC_Read10(&USB_OTG_Core, buf, sector, 512 * cnt);
            USBH_MSC_HandleBOTXfer(&USB_OTG_Core, &USB_Host);
            if (!HCD_IsDeviceConnected(&USB_OTG_Core)) {
                res = 1;
                break;
            }
        } while (res == USBH_MSC_BUSY);
    }

    if (res == USBH_MSC_OK) res = 0;
    return res;
}

uint8_t USBH_UDISK_Write(uint8_t *buf, uint32_t sector, uint32_t cnt)
{
    uint8_t res = 1;

    if (HCD_IsDeviceConnected(&USB_OTG_Core)) {
        do {
            res = USBH_MSC_Write10(&USB_OTG_Core, buf, sector, 512 * cnt);
            USBH_MSC_HandleBOTXfer(&USB_OTG_Core, &USB_Host);
            if (!HCD_IsDeviceConnected(&USB_OTG_Core)) {
                res = 1;
                break;
            }
        } while (res == USBH_MSC_BUSY);
    }

    if (res == USBH_MSC_OK) res = 0;
    return res;
}
