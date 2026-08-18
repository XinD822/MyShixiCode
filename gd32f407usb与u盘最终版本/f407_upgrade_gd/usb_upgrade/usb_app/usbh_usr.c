/**
 * @file usbh_usr.c
 * @brief USB Host 用户回调 + U盘读写封装（GD32F4xx_usb_library 版）
 *
 * 回调只设标志，升级在 USB_Host_Task_Run 主循环中执行。
 * 读写使用 usbh_msc_read/write 的原始阻塞方式。
 */
#include "usbh_usr.h"
#include "usbh_msc_core.h"
#include "usbh_msc_scsi.h"
#include "usbh_msc_bbb.h"
#include "usbh_pipe.h"
#include "drv_usb_core.h"
#include "usb_host_task.h"
#include "config.h"
#include "ff.h"
#include <string.h>

/* 全局变量（定义在 usb_host_task.c） */
extern usbh_host       usb_host_msc;
extern usb_core_driver usbh_core;

/* U盘就绪标志和重试计数 */
volatile uint8_t g_udisk_ready = 0;
uint8_t g_retry_count = 0;

/* ──── Host 用户回调函数实现 ──── */
static void usbh_user_init(void)
{
    LOGD("[HOST] USB Host Library v3.x (GD)\r\n");
}

static void usbh_user_deinit(void)
{
    g_udisk_ready = 0;
    g_retry_count = 0;
}

static void usbh_user_device_connected(void)
{
    LOGD("[HOST] Device attached\r\n");
}

static void usbh_user_device_reset(void)
{
    LOGD("[HOST] Reset device\r\n");
}

static void usbh_user_device_disconnected(void)
{
    g_udisk_ready = 0;
    g_retry_count = 0;
    LOGD("[HOST] Device disconnected\r\n");
}

static void usbh_user_over_current_detected(void)
{
    LOGD("[HOST] Over current detected\r\n");
}

static void usbh_user_device_speed_detected(uint32_t DeviceSpeed)
{
    /* PORT_SPEED_HIGH=0 / PORT_SPEED_FULL=1 / PORT_SPEED_LOW=2 */
    LOGD("[HOST] %s device\r\n",
               DeviceSpeed == PORT_SPEED_LOW ? "Low speed" : "Full speed");
}

static void usbh_user_device_desc_available(void *DeviceDesc)
{
    uint8_t *desc = (uint8_t *)DeviceDesc;
    LOGD("[HOST] VID: %02X%02X, PID: %02X%02X\r\n", desc[9], desc[8], desc[11], desc[10]);
}

static void usbh_user_device_address_assigned(void)
{
    LOGD("[HOST] Address assigned\r\n");
}

static void usbh_user_configuration_descavailable(usb_desc_config *cfg,
                                                   usb_desc_itf *itf,
                                                   usb_desc_ep *ep)
{
    (void)cfg; (void)ep;
    if (itf->bInterfaceClass == 0x08) {
        LOGD("[HOST] MSC device (mass storage)\r\n");
    }
}

static void usbh_user_manufacturer_string(void *ManufacturerString)
{
    (void)ManufacturerString;
}

static void usbh_user_product_string(void *ProductString)
{
    (void)ProductString;
}

static void usbh_user_serialnum_string(void *SerialNumString)
{
    (void)SerialNumString;
}

static void usbh_user_enumeration_finish(void)
{
    LOGD("[HOST] Enumeration done\r\n");
}

static usbh_user_status usbh_user_userinput(void)
{
    return USR_IN_RESP_OK;
}

static void usbh_user_device_not_supported(void)
{
    LOGD("[HOST] Device not supported\r\n");
}

static void usbh_user_unrecovered_error(void)
{
    LOGD("[HOST] Unrecovered error\r\n");
}

/**
 * @brief MSC 类应用回调 — U盘枚举成功后被反复调用
 *        只设标志，不在 callback 里执行升级（避免 usbh_msc_read 在
 *        usbh_core_task 内部调用导致状态机死锁）。
 *        实际升级在 USB_Host_Task_Run 主循环中执行。
 */
static int usbh_usr_msc_application(void)
{
    if (!g_udisk_ready) {
        g_udisk_ready = 1;
        LOGD("[HOST] MSC_Application: U-disk ready, will upgrade in main loop\r\n");
    }
    return 0;
}

/* ──── Host 用户回调表 ──── */
usbh_user_cb usr_cb = {
    usbh_user_init,
    usbh_user_deinit,
    usbh_user_device_connected,
    usbh_user_device_reset,
    usbh_user_device_disconnected,
    usbh_user_over_current_detected,
    usbh_user_device_speed_detected,
    usbh_user_device_desc_available,
    usbh_user_device_address_assigned,
    usbh_user_configuration_descavailable,
    usbh_user_manufacturer_string,
    usbh_user_product_string,
    usbh_user_serialnum_string,
    usbh_user_enumeration_finish,
    usbh_user_userinput,
    usbh_usr_msc_application,
    usbh_user_device_not_supported,
    usbh_user_unrecovered_error
};

/* ──── U盘读写封装（供 diskio.c 调用） ──── */

uint8_t USBH_UDISK_Status(void)
{
    /* 与 STM32 原版 HCD_IsDeviceConnected 语义一致：已连接返回 1。
       ⚠ 之前写成 connect_status ? 0 : 1 是反的：diskio.c 的
       disk_initialize 用 if (USBH_UDISK_Status()) 判定就绪，
       连接时返回 0 会让 disk_stat_usb = STA_NOINIT，
       导致 f_mount 直接返回 FR_NOT_READY (fr=3)。 */
    return (uint8_t)usbh_core.host.connect_status;
}

/* ──── U盘读写封装（供 diskio.c 调用） ────
 * 采用官方 GD32F4xx_usb_library 标准用法：直接调用阻塞式
 * usbh_msc_read/usbh_msc_write（V3.3.3 库自带），函数内部会
 * 提交 CBW → 轮询 usbh_msc_rdwr_process 推进 BOT（数据按
 * ep_size_in=512B 逐 URB 接收，规避多包 URB 的 toggle/CSW 问题）→
 * 收 CSW → 返回 USBH_OK/USBH_FAIL。第 5 参为扇区数。
 * 与官方 usbh_msc_fatfs.c 的 disk_read 一致。 */

uint8_t USBH_UDISK_Read(uint8_t *buf, uint32_t sector, uint32_t cnt)
{
    if (cnt == 0U) return 0U;

    if (usbh_core.host.connect_status) {
        usbh_msc_handler *msc = (usbh_msc_handler *)usb_host_msc.active_class->class_data;
        uint32_t t0 = usb_host_msc.control.timer;
        uint8_t st0 = (uint8_t)usb_host_msc.cur_state;
        uint8_t unit_st0 = msc ? (uint8_t)msc->unit[0].state : 0xFFU;
        uint8_t bbb_st0 = msc ? (uint8_t)msc->bbb.state : 0xFFU;
        static uint32_t rd_cnt = 0;
        rd_cnt++;

        usbh_status res = usbh_msc_read(&usb_host_msc, 0U, sector, buf, cnt);
        if (USBH_OK == res) {
            return 0U;
        }

        /* ── 失败诊断 ── */
        uint32_t dt = usb_host_msc.control.timer - t0;
        LOGD("[DISK] read FAIL #%lu s=%lu c=%u res=%d dt=%lu\r\n",
                   (unsigned long)rd_cnt, (unsigned long)sector, (unsigned)cnt,
                   (int)res, (unsigned long)dt);
        LOGD("[DISK]   entry: cur_st=%u conn=%u unit_st=%u bbb_st=%u\r\n",
                   (unsigned)st0, (unsigned)usbh_core.host.connect_status,
                   (unsigned)unit_st0, (unsigned)bbb_st0);
        if (msc) {
            LOGD("[DISK]   now: msc.st=%u unit.st=%u bbb.st=%u cmd=%u\r\n",
                       (unsigned)msc->state, (unsigned)msc->unit[0].state,
                       (unsigned)msc->bbb.state, (unsigned)msc->bbb.cmd_state);
            LOGD("[DISK]   pipes: in=%u out=%u blksz=%lu blknbr=%lu\r\n",
                       (unsigned)msc->pipe_in, (unsigned)msc->pipe_out,
                       (unsigned long)msc->unit[0].capacity.block_size,
                       (unsigned long)msc->unit[0].capacity.block_nbr);
            LOGD("[DISK]   CBW(31): %02X %02X %02X %02X %02X %02X %02X %02X "
                       "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X "
                       "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                       msc->bbb.cbw.CBWArray[0], msc->bbb.cbw.CBWArray[1],
                       msc->bbb.cbw.CBWArray[2], msc->bbb.cbw.CBWArray[3],
                       msc->bbb.cbw.CBWArray[4], msc->bbb.cbw.CBWArray[5],
                       msc->bbb.cbw.CBWArray[6], msc->bbb.cbw.CBWArray[7],
                       msc->bbb.cbw.CBWArray[8], msc->bbb.cbw.CBWArray[9],
                       msc->bbb.cbw.CBWArray[10], msc->bbb.cbw.CBWArray[11],
                       msc->bbb.cbw.CBWArray[12], msc->bbb.cbw.CBWArray[13],
                       msc->bbb.cbw.CBWArray[14], msc->bbb.cbw.CBWArray[15],
                       msc->bbb.cbw.CBWArray[16], msc->bbb.cbw.CBWArray[17],
                       msc->bbb.cbw.CBWArray[18], msc->bbb.cbw.CBWArray[19],
                       msc->bbb.cbw.CBWArray[20], msc->bbb.cbw.CBWArray[21],
                       msc->bbb.cbw.CBWArray[22], msc->bbb.cbw.CBWArray[23],
                       msc->bbb.cbw.CBWArray[24], msc->bbb.cbw.CBWArray[25],
                       msc->bbb.cbw.CBWArray[26], msc->bbb.cbw.CBWArray[27],
                       msc->bbb.cbw.CBWArray[28], msc->bbb.cbw.CBWArray[29],
                       msc->bbb.cbw.CBWArray[30]);
            LOGD("[DISK]   CBWlen=%lu CDB_sect=%u\r\n",
                       (unsigned long)msc->bbb.cbw.field.dCBWDataTransferLength,
                       (unsigned)((msc->bbb.cbw.field.CBWCB[7] << 8) | msc->bbb.cbw.field.CBWCB[8]));
            LOGD("[DISK]   CSW: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                       msc->bbb.csw.CSWArray[0], msc->bbb.csw.CSWArray[1],
                       msc->bbb.csw.CSWArray[2], msc->bbb.csw.CSWArray[3],
                       msc->bbb.csw.CSWArray[4], msc->bbb.csw.CSWArray[5],
                       msc->bbb.csw.CSWArray[6], msc->bbb.csw.CSWArray[7],
                       msc->bbb.csw.CSWArray[8], msc->bbb.csw.CSWArray[9],
                       msc->bbb.csw.CSWArray[10], msc->bbb.csw.CSWArray[11],
                       msc->bbb.csw.CSWArray[12]);
            LOGD("[DISK]   in : urb=%d xfer=%lu tog=%d len=%lu\r\n",
                       (int)usbh_core.host.pipe[msc->pipe_in].urb_state,
                       (unsigned long)usbh_core.host.pipe[msc->pipe_in].xfer_count,
                       (int)usbh_core.host.pipe[msc->pipe_in].data_toggle_in,
                       (unsigned long)usbh_core.host.pipe[msc->pipe_in].xfer_len);
            LOGD("[DISK]   inr: HCHLEN=%08X HCHCTL=%08X HCHINTF=%08X\r\n",
                       (unsigned)usbh_core.regs.pr[msc->pipe_in]->HCHLEN,
                       (unsigned)usbh_core.regs.pr[msc->pipe_in]->HCHCTL,
                       (unsigned)usbh_core.regs.pr[msc->pipe_in]->HCHINTF);
            LOGD("[DISK]   out: urb=%d tog=%d HCHINTF=%08X GRSTATP=%08X\r\n",
                       (int)usbh_core.host.pipe[msc->pipe_out].urb_state,
                       (int)usbh_core.host.pipe[msc->pipe_out].data_toggle_out,
                       (unsigned)usbh_core.regs.pr[msc->pipe_out]->HCHINTF,
                       (unsigned)usbh_core.regs.gr->GRSTATP);
        }
    }
    return 1U;
}

uint8_t USBH_UDISK_Write(uint8_t *buf, uint32_t sector, uint32_t cnt)
{
    if (cnt == 0U) return 0U;

    if (usbh_core.host.connect_status) {
        if (USBH_OK == usbh_msc_write(&usb_host_msc, 0U, sector, buf, cnt)) {
            return 0U;
        }
        LOGD("[DISK] USB write fail: sector=%lu cnt=%lu\r\n",
                   (unsigned long)sector, (unsigned long)cnt);
    }
    return 1U;
}
