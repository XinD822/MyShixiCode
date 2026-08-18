/**
 * @file usb_host_task.c
 * @brief USB Host 模式任务实现
 *
 * 中断模式：USBH_Init 启用 OTG_FS 硬件中断，OTG_FS_IRQHandler 自动
 * 调用 USBH_OTG_ISR_Handler 处理 USB 事件（与正点原子 Host 例程一致）。
 *
 * 升级流程与正点原子一致：
 *   USBH_Process → USBH_MSC_Handle → USBH_USR_MSC_Application → USB_Host_Do_Upgrade
 * USBH_UDISK_Read 在 USBH_Process 内部调用，确保 BOT 状态机正确管理。
 */

#include "config.h"
#include "usb_host_task.h"
#include "usbh_usr.h"
#include "usbh_msc_core.h"
#include "usbh_msc_scsi.h"   /* USBH_MSC_Param */
#include "usb_hcd_int.h"
#include "usb_bsp.h"
#include "usb_mode.h"        /* USB_Mode_WriteFlag */
#include "platform.h"         /* Platform_USB_ClockDisable / Platform_USB_NVICDisable */
#include "stm32f4xx_gpio.h"
#include "ff.h"              /* FATFS, f_mount */
#include "upgrade_config.h"  /* UPGRADE_STATE_ADDR */
#include "w25q128_drv.h"     /* Flash_Read (直接 SPI Flash 读取) */

/* USB OTG 核心和 Host 状态机 */
USB_OTG_CORE_HANDLE USB_OTG_Core;
USBH_HOST           USB_Host;

/* 调试跟踪变量 */
static uint32_t g_host_debug_tick = 0;
static uint8_t  g_prev_pcsts = 0;
static uint8_t  g_prev_gstate = 0xFF;

/* Host 模式 OTG_FS 硬件中断计数器（定义在 usbd_usr.c） */
extern volatile uint32_t g_host_isr_count;

/* ──── 公开接口：执行 U盘升级 ────
 * 从 USBH_USR_MSC_Application 回调中调用（在 USBH_Process 内部）。
 * 与正点原子 Host 例程结构一致：U盘枚举成功后直接在回调中执行
 * 挂载 FatFs、检测 firmware.bin、升级。
 *
 * 关键：USBH_UDISK_Read 在此函数内部调用，而此函数从 USBH_MSC_Handle
 * 的 DEFAULT_APPLI_STATE 分支调用。Read10 完成后 DECODE_CSW 会设置
 * MSCState = MSCStateCurrent（ST 库 bug：Read10 未设 MSCStateCurrent），
 * 但返回到 USBH_MSC_Handle 后 MSCState 被覆盖为 DEFAULT_APPLI_STATE，
 * 从而避免了状态机混乱。 */
void USB_Host_Do_Upgrade(void)
{
    static FATFS udisk_fs;
    uint8_t connected = (uint8_t)HCD_IsDeviceConnected(&USB_OTG_Core);
    if (!connected) return;

    /* 打印 MSC 枚举参数 — 确认容量和块大小正确 */
    DBG_PRINTF("[HOST] MSC params: capacity=%lu (last LBA), pageLen=%u, writeProtect=%u\r\n",
               (unsigned long)USBH_MSC_Param.MSCapacity,
               (unsigned int)USBH_MSC_Param.MSPageLength,
               (unsigned int)USBH_MSC_Param.MSWriteProtect);
    DBG_PRINTF("[HOST] Total sectors=%lu, bulkInEp=0x%02X, bulkOutEp=0x%02X\r\n",
               (unsigned long)(USBH_MSC_Param.MSCapacity + 1),
               (unsigned int)MSC_Machine.MSBulkInEp,
               (unsigned int)MSC_Machine.MSBulkOutEp);

    /* ──── 引导扇区预读验证 ────
     * 在 f_mount 之前先手动读取扇区 0，dump 前 32 字节，
     * 确认 USBH_UDISK_Read 返回的数据是有效的 FAT 引导扇区。
     * 正常 FAT 引导扇区：首字节 0xEB 或 0xE9，偏移 510-511 = 0x55 0xAA */
    {
        static uint8_t boot_buf[512];
        uint8_t r = USBH_UDISK_Read(boot_buf, 0, 1);
        DBG_PRINTF("[HOST] Boot sector read: r=%d, first 32 bytes:\r\n", (int)r);
        if (r == 0) {
            for (int i = 0; i < 32; i += 16) {
                DBG_PRINTF("[HOST]   %03X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                           i,
                           boot_buf[i+0], boot_buf[i+1], boot_buf[i+2], boot_buf[i+3],
                           boot_buf[i+4], boot_buf[i+5], boot_buf[i+6], boot_buf[i+7],
                           boot_buf[i+8], boot_buf[i+9], boot_buf[i+10], boot_buf[i+11],
                           boot_buf[i+12], boot_buf[i+13], boot_buf[i+14], boot_buf[i+15]);
            }
            if (boot_buf[510] == 0x55 && boot_buf[511] == 0xAA) {
                DBG_PRINTF("[HOST] Boot signature 55AA OK\r\n");
            } else {
                DBG_PRINTF("[HOST] *** Boot signature MISMATCH: %02X %02X (expect 55 AA) ***\r\n",
                           boot_buf[510], boot_buf[511]);
            }
        } else {
            DBG_PRINTF("[HOST] *** Boot sector read FAILED (r=%d) ***\r\n", (int)r);
        }
    }

    /* 等待 200ms 让 U 盘内部状态稳定 */
    Tick_DelayMs(200);

    /* 挂载 FatFs */
    DBG_PRINTF("[HOST] U-disk ready, mounting...\r\n");
    FRESULT fr = FR_NO_FILESYSTEM;
    for (int attempt = 0; attempt < 3; attempt++) {
        fr = f_mount(&udisk_fs, "1:", 1);
        if (fr == FR_OK) break;
        DBG_PRINTF("[HOST] Mount attempt %d failed: fr=%d, retrying...\r\n",
                   attempt + 1, (int)fr);
        Tick_DelayMs(300);
    }

    if (fr != FR_OK) {
        DBG_PRINTF("[HOST] U-disk mount failed after 3 attempts: fr=%d\r\n", (int)fr);
        return;
    }

    DBG_PRINTF("[HOST] U-disk mounted, checking firmware...\r\n");

    /* 检测 firmware.bin 并升级 */
    if (!UpgradeSource_Get()) {
        DBG_PRINTF("[HOST] no source registered\r\n");
        f_mount(NULL, "1:", 1);
        return;
    }
    if (!UpgradeSource_Get()->detect()) {
        DBG_PRINTF("[HOST] no firmware.bin on U-disk\r\n");
        f_mount(NULL, "1:", 1);
        return;
    }

    DBG_PRINTF("[HOST] firmware detected, checking...\r\n");
    UpgradeResult chk = Upgrade_Check();
    if (chk != UPGRADE_OK) {
        DBG_PRINTF("[HOST] check failed: %d\r\n", (int)chk);
        f_mount(NULL, "1:", 1);
        return;
    }

    DBG_PRINTF("[HOST] executing upgrade...\r\n");
    UpgradeResult exec = Upgrade_Execute();
    if (exec != UPGRADE_OK) {
        DBG_PRINTF("[HOST] execute failed: %d\r\n", (int)exec);
        f_mount(NULL, "1:", 1);
        return;
    }

    /* 不删除 U盘上的 firmware.bin — 用户要求保留 */
    FlashService_FlushCache();

    /* 标记升级完成 */
    uint32_t done = UPGRADE_STATE_DONE;
    FlashService_Write((uint8_t *)&done, UPGRADE_STATE_ADDR, sizeof(done));
    FlashService_FlushCache();

    /* ──── 回读验证：确认升级标志已持久化到 SPI Flash ──── */
    {
        uint32_t raw_flag = 0;
        Flash_Read((uint8_t *)&raw_flag, UPGRADE_FLAG_ADDR, 4);
        DBG_PRINTF("[HOST] Verify (raw SPI): flag=0x%08X\r\n", (unsigned int)raw_flag);

        if (raw_flag != UPGRADE_FLAG_YES) {
            DBG_PRINTF("[HOST] *** ERROR: raw SPI read failed! ***\r\n");
            f_mount(NULL, "1:", 1);
            return;
        }
        DBG_PRINTF("[HOST] Flag verified OK in SPI Flash\r\n");
    }

    /* 升级成功后切换回 DEVICE 模式，复位后自动进入拖拽模式 */
    DBG_PRINTF("[HOST] upgrade done, switching to DEVICE mode...\r\n");
    USB_Mode_WriteFlag(USB_MODE_DEVICE);

    /* ──── 最终验证：USB_Mode_WriteFlag 之后再读一次 ────
     * 确认写 USB 模式标志（0xFE1000 扇区）没有破坏升级标志（0xFE0000 扇区） */
    {
        uint32_t f1 = 0, f2 = 0, f3 = 0;
        Flash_Read((uint8_t *)&f1, UPGRADE_FLAG_ADDR, 4);
        Flash_Read((uint8_t *)&f2, UPGRADE_STATE_ADDR, 4);
        Flash_Read((uint8_t *)&f3, FIRMWARE_SIZE_ADDR, 4);
        DBG_PRINTF("[HOST] Final check: flag=0x%08X state=0x%08X size=%lu\r\n",
                   (unsigned int)f1, (unsigned int)f2, (unsigned long)f3);
        /* 也读一下 USB 模式标志扇区，确认没有越界 */
        uint32_t um = 0;
        Flash_Read((uint8_t *)&um, USB_MODE_FLAG_ADDR, 4);
        DBG_PRINTF("[HOST] USB_MODE_FLAG @ 0x%06X = 0x%08X\r\n",
                   (unsigned int)USB_MODE_FLAG_ADDR, (unsigned int)um);
    }

    /* SPI Flash 软件复位 + RCC归位 + NVIC内核软件复位 */
    Flash_Reset();
    DBG_PRINTF("[HOST] resetting...\r\n");
    Platform_SystemReset();
    /* 不会执行到这里 */
}

/* ──── 公开接口 ──── */

void USB_Host_Poll_Handler(void)
{
    /* 硬件中断模式：OTG_FS_IRQHandler 自动处理 USB 中断。
     * USBH_Process 在 USB_Host_Task_Run 中调用，推进 Host 状态机。 */
}

void USB_Host_Task_Init(void)
{
    /* 先关闭 OTG_FS 中断，防止 USBH_Init 内部触发中断时 VTOR 仍可能有误 */
    NVIC_DisableIRQ(OTG_FS_IRQn);

    DBG_PRINTF("[HOST] USBH_Init...\r\n");
    USBH_Init(&USB_OTG_Core, USB_OTG_FS_CORE_ID, &USB_Host,
              &USBH_MSC_cb, &USR_Callbacks);
    DBG_PRINTF("[HOST] USBH_Init done\r\n");

    /* USBH_Init 内部已调用 USB_OTG_BSP_EnableInterrupt，
     * 但我们在上面 DisableIRQ 了，现在重新启用。
     * 先清除所有 pending 中断标志，防止初始化期间的残留中断触发。 */
    USB_OTG_WRITE_REG32(&USB_OTG_Core.regs.GREGS->GINTSTS, 0xFFFFFFFF);
    NVIC_ClearPendingIRQ(OTG_FS_IRQn);
    NVIC_EnableIRQ(OTG_FS_IRQn);

    DBG_PRINTF("[HOST] OTG_FS IRQ enabled (interrupt mode)\r\n");

    /* 注册 U盘升级来源 */
    UpgradeSource_Register(&Source_USB_Drive);
    Upgrade_Init();

    DBG_PRINTF("[HOST] Init done, waiting for U-disk...\r\n");

    /* 打印初始寄存器状态，确认 Host 模式正确进入 */
    {
        uint32_t gintsts = USB_OTG_READ_REG32(&USB_OTG_Core.regs.GREGS->GINTSTS);
        uint32_t gusbcfg = USB_OTG_READ_REG32(&USB_OTG_Core.regs.GREGS->GUSBCFG);
        uint32_t gccfg   = USB_OTG_READ_REG32(&USB_OTG_Core.regs.GREGS->GCCFG);
        uint32_t hprt0   = USB_OTG_READ_REG32(USB_OTG_Core.regs.HPRT0);
        uint32_t gotgctl = USB_OTG_READ_REG32(&USB_OTG_Core.regs.GREGS->GOTGCTL);
        uint32_t gintmsk = USB_OTG_READ_REG32(&USB_OTG_Core.regs.GREGS->GINTMSK);
        uint32_t gahbcfg = USB_OTG_READ_REG32(&USB_OTG_Core.regs.GREGS->GAHBCFG);

        DBG_PRINTF("[HOST] GINTSTS=0x%08X (bit0=CMOD, 1=Host)\r\n", (unsigned int)gintsts);
        DBG_PRINTF("[HOST] GUSBCFG=0x%08X (bit29=FHMOD, bit30=FDMOD)\r\n", (unsigned int)gusbcfg);
        DBG_PRINTF("[HOST] GCCFG=0x%08X (bit16=PWRDWN, bit21=NOVBUSSENS)\r\n", (unsigned int)gccfg);
        DBG_PRINTF("[HOST] HPRT0=0x%08X (bit0=PCSTS, bit12=PPWR)\r\n", (unsigned int)hprt0);
        DBG_PRINTF("[HOST] GOTGCTL=0x%08X\r\n", (unsigned int)gotgctl);
        DBG_PRINTF("[HOST] GINTMSK=0x%08X (bit24=prtint, bit25=hcintr)\r\n", (unsigned int)gintmsk);
        DBG_PRINTF("[HOST] GAHBCFG=0x%08X (bit0=GINT, 1=global int enabled)\r\n", (unsigned int)gahbcfg);
        DBG_PRINTF("[HOST]   CMOD=%lu FHMOD=%lu PWRDWN=%lu NOVBUSSENS=%lu PPWR=%lu\r\n",
                   gintsts & 1,
                   (gusbcfg >> 29) & 1,
                   (gccfg >> 16) & 1,
                   (gccfg >> 21) & 1,
                   (hprt0 >> 12) & 1);
        DBG_PRINTF("[HOST]   ID=%lu ASVLD=%lu BSESVLD=%lu (GOTGCTL)\r\n",
                   (gotgctl >> 16) & 1,
                   (gotgctl >> 18) & 1,
                   (gotgctl >> 19) & 1);
    }

    g_host_debug_tick = Tick_GetMs();
    g_prev_pcsts = 0;
    g_prev_gstate = 0xFF;
}

void USB_Host_Task_Run(void)
{
    /* 推进 Host 状态机 */
    USBH_Process(&USB_OTG_Core, &USB_Host);

    /* 与正点原子例程一致：每次轮询后延时 1ms，避免状态机运转过快 */
    Tick_DelayMs(1);

    /* ──── Host 状态机状态变化检测 ──── */
    {
        uint8_t gstate = (uint8_t)USB_Host.gState;
        if (gstate != g_prev_gstate) {
            DBG_PRINTF("[HOST] *** State changed: %d -> %d ***\r\n",
                       g_prev_gstate, gstate);
            g_prev_gstate = gstate;
        }
    }

    /* ──── 即时 PCSTS 变化检测：插入/拔出 U 盘时立即打印 ──── */
    {
        uint32_t hprt0 = USB_OTG_READ_REG32(USB_OTG_Core.regs.HPRT0);
        uint8_t pcsts = (uint8_t)(hprt0 & 1);
        if (pcsts != g_prev_pcsts) {
            DBG_PRINTF("[HOST] *** PCSTS changed: %d -> %d (HPRT0=0x%08X) ***\r\n",
                       g_prev_pcsts, pcsts, (unsigned int)hprt0);
            g_prev_pcsts = pcsts;
        }
    }

    /* ──── 定期调试输出：每 3 秒打印一次 USB 寄存器状态 ──── */
    {
        uint32_t now = Tick_GetMs();
        if (now - g_host_debug_tick >= 3000) {
            g_host_debug_tick = now;
            uint32_t gintsts = USB_OTG_READ_REG32(&USB_OTG_Core.regs.GREGS->GINTSTS);
            uint32_t hprt0   = USB_OTG_READ_REG32(USB_OTG_Core.regs.HPRT0);
            uint32_t gotgctl = USB_OTG_READ_REG32(&USB_OTG_Core.regs.GREGS->GOTGCTL);
            DBG_PRINTF("[HOST] GINTSTS=0x%08X HPRT0=0x%08X state=%d ISR=%lu\r\n",
                       (unsigned int)gintsts, (unsigned int)hprt0, (int)USB_Host.gState,
                       (unsigned long)g_host_isr_count);
            DBG_PRINTF("[HOST]   CMOD=%lu PCSTS=%lu PPWR=%lu ASVLD=%lu BSESVLD=%lu\r\n",
                       gintsts & 1,
                       hprt0 & 1,
                       (hprt0 >> 12) & 1,
                       (gotgctl >> 18) & 1,
                       (gotgctl >> 19) & 1);
        }
    }
}
