/**
 * @file usb_host_task.c
 * @brief USB Host 任务（GD32F4xx_usb_library 版）— U盘插入升级
 *
 * 对照 STM32 版（f407_upgrade）移植：
 * - ST USB_OTG_CORE_HANDLE + USBH_HOST → GD usbh_host + usb_core_driver
 * - ST USBH_Init/USBH_Process → GD usbh_init/usbh_class_register/usbh_core_task
 * - ST HCD_IsDeviceConnected → GD usbh_core.host.connect_status
 * - 中断模式：USBFS_IRQHandler 调 usbh_isr(&usbh_core)（usb_hw.c）
 *
 * 流程：枚举成功 → usbh_user 回调置 g_udisk_ready →
 *       usbh_usr_msc_application 调 USB_Host_Do_Upgrade：
 *       挂载 FatFs "1:" → 检测 firmware.bin → 校验 → 搬运 → 写标志 → 复位
 */

#include "config.h"
#include "usb_host_task.h"
#include "usbh_usr.h"
#include "usbh_msc_core.h"
#include "drv_usb_core.h"
#include "drv_usb_host.h"
#include "drv_usb_hw.h"        /* usb_rcu_config / usb_gpio_config / usb_timer_init */
#include "usb_mode.h"         /* USB_Mode_WriteFlag */
#include "plat_reset.h"          /* plat_usb_nvic_disable / plat_system_reset */
#include "ff.h"               /* FATFS, f_mount */
#include "upgrade_config.h"   /* UPGRADE_STATE_ADDR / APP_SIZE */
#include "flash_service.h"
#include "fatfs_system.h"     /* FatFs_Mount / FatFs_Unmount (本地卷 0:) */
#include "upgrade_source.h"   /* Source_USB_Drag */

/* ════ GD Host 全局实例 ════ */
usbh_host       usb_host_msc;
usb_core_driver usbh_core;

/* 调试跟踪变量 */
static uint32_t g_host_debug_tick = 0;
static uint8_t  g_prev_pcsts = 0;
static uint8_t  g_prev_gstate = 0xFF;

/* ──── 公开接口：执行 U盘升级 ────
 * 从 usbh_usr_msc_application 回调中调用（在 usbh_core_task 内部）。
 * U盘枚举成功后直接执行挂载 FatFs、检测 firmware.bin、升级。 */
void USB_Host_Do_Upgrade(void)
{
    static FATFS udisk_fs;
    if (!usbh_core.host.connect_status) return;

    /* 打印 MSC 枚举参数 — 确认容量和块大小正确 */
    msc_lun lun_info;
    if (USBH_OK == usbh_msc_lun_info_get(&usb_host_msc, 0U, &lun_info)) {
        /* 容量 = 扇区数 × 块大小。必须用 64 位计算：>4GB 的 U 盘
           blk_nbr×512 会溢出 uint32（曾打印出错误容量 1738538496），
           >>20 换算成 MB 也避开 %llu 对 printf 实现的要求 */
        LOGD("[HOST] Disk capacity: %lu MB\r\n",
                   (unsigned long)(((uint64_t)lun_info.capacity.block_nbr *
                                    (uint64_t)lun_info.capacity.block_size) >> 20));
        LOGD("[HOST]   blk_nbr=%lu blk_size=%lu\r\n",
                   (unsigned long)lun_info.capacity.block_nbr,
                   (unsigned long)lun_info.capacity.block_size);
    }

    /* ──── 扇区 0 预读诊断 ────
     * 扇区 0 不一定是 FAT 引导扇区（VBR）：
     *   - 带分区表的 U 盘（MBR）：扇区 0 是 MBR，分区表在偏移 446，
     *     首字节通常为 0x00，510-511 的 0x55 0xAA 是 MBR 签名；
     *     FAT 卷实际在分区偏移处，FatFS 通过自动分区搜索找到它。
     *   - 无分区表的 U 盘：扇区 0 才是 FAT VBR，首字节 0xEB/0xE9。
     * 因此这里只验证 USBH_UDISK_Read 能读到数据 + 签名 55AA，
     * 不把"首字节 0xEB"当作必要条件；真正的 FAT 有效性由 f_mount 判定。 */
    {
        static uint8_t boot_buf[512];
        uint8_t r = USBH_UDISK_Read(boot_buf, 0, 1);
        LOGD("[HOST] Sector0 read: r=%d, first 8 bytes:\r\n", (int)r);
        if (r == 0) {
            for (int i = 0; i < 8; i++) {
                LOGD("[HOST]   %02X: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                           i * 8,
                           boot_buf[i*8+0], boot_buf[i*8+1], boot_buf[i*8+2], boot_buf[i*8+3],
                           boot_buf[i*8+4], boot_buf[i*8+5], boot_buf[i*8+6], boot_buf[i*8+7]);
            }
            if (boot_buf[0] == 0xEB || boot_buf[0] == 0xE9) {
                LOGD("[HOST] Sector0 = FAT VBR (no MBR)\r\n");
            } else {
                LOGD("[HOST] Sector0 = MBR (partitioned disk, FAT in partition)\r\n");
            }
            if (boot_buf[510] == 0x55 && boot_buf[511] == 0xAA) {
                LOGD("[HOST] Signature 55AA OK\r\n");
            } else {
                LOGD("[HOST] *** Signature MISMATCH: %02X %02X (expect 55 AA) ***\r\n",
                           boot_buf[510], boot_buf[511]);
            }
        } else {
            LOGD("[HOST] *** Sector0 read FAILED (r=%d) ***\r\n", (int)r);
        }
    }

    /* 等待 200ms 让 U 盘内部状态稳定 */
    plat_delay_ms(200);

    /* 挂载 FatFs（卷 "1:" = U盘，pdrv=1） */
    LOGD("[HOST] U-disk ready, mounting...\r\n");
    FRESULT fr = FR_NO_FILESYSTEM;
    for (int attempt = 0; attempt < 3; attempt++) {
        fr = f_mount(&udisk_fs, "1:", 1);
        if (fr == FR_OK) break;
        LOGD("[HOST] Mount attempt %d failed: fr=%d, retrying...\r\n",
                   attempt + 1, (int)fr);
        plat_delay_ms(300);
    }

    if (fr != FR_OK) {
        LOGD("[HOST] U-disk mount failed after 3 attempts: fr=%d\r\n", (int)fr);
        return;
    }

    LOGD("[HOST] U-disk mounted, checking firmware...\r\n");

    /* 检测 firmware.bin 并升级 */
    if (!UpgradeSource_Get()) {
        LOGD("[HOST] no source registered\r\n");
        f_mount(NULL, "1:", 1);
        return;
    }
    if (!UpgradeSource_Get()->detect()) {
        LOGD("[HOST] no firmware.bin on U-disk\r\n");
        f_mount(NULL, "1:", 1);
        return;
    }

    LOGD("[HOST] firmware detected, checking...\r\n");
    UpgradeResult chk = Upgrade_Check();
    if (chk != UPGRADE_OK) {
        LOGD("[HOST] check failed: %d\r\n", (int)chk);
        f_mount(NULL, "1:", 1);
        return;
    }

    /* ──── 两段式：先把 U盘固件拷入本地数据区(卷 0:)，再走拖拽同款 Upgrade_Execute ────
     * 与拖拽升级（DEVICE 模式）完全同构：
     *   拖拽：PC 把 firmware.bin 拖入数据区 → Upgrade_Execute 读本地 → 写 Slot A
     *   U盘 : Host 把 firmware.bin 拷入数据区 → Upgrade_Execute 读本地 → 写 Slot A
     * 收益：
     *   ① Slot A 的 64KB 块擦除（150ms~2s）发生在"本地 Flash → 本地 Flash"阶段，
     *      此阶段已卸载 U盘卷、不再触碰 USB，彻底规避 Host 模式下擦除停摆破坏
     *      USB BOT 状态机的问题（此前 CSW 全 0 / URB 不完成的根因）；
     *   ② 固件大小只受本地数据区容量 / APP_SIZE 限制，不再受 SRAM 缓冲限制，
     *      可支持接近 448KB（APP_SIZE）的大固件。
     * 拷贝阶段（U盘 → 本地）仅涉及 4KB 扇区擦除（~50ms），远小于此前失败的
     * 64KB 块擦除，且与拖拽时 PC 经 MSC 写入数据区的擦除节奏一致。 */
    {
        static uint8_t stage_buf[FIRMWARE_BUF_SIZE];
        FIL  src_file;                 /* U盘上的 firmware.bin */
        FIL  dst_file;                 /* 本地数据区的 firmware.bin */
        uint32_t fw_size = 0;
        uint32_t total = 0;
        UINT br = 0, bw = 0;

        /* 1. 挂载本地数据区（卷 0:）；无文件系统时自动格式化 */
        FRESULT mres = FatFs_Mount();
        if (mres != FR_OK) {
            LOGD("[HOST] local FatFS mount failed: %d\r\n", (int)mres);
            f_mount(NULL, "1:", 1);
            return;
        }

        /* 2. 打开 U盘上的 firmware.bin */
        FRESULT open_res = UpgradeSource_Get()->open(&src_file, &fw_size);
        LOGD("[HOST] open result=%d, fw_size=%lu\r\n", (int)open_res, fw_size);
        if (open_res != FR_OK) {
            FatFs_Unmount();
            f_mount(NULL, "1:", 1);
            return;
        }
        /* 与内部 Flash APP 分区对齐：固件超过 APP_SIZE 会被 Bootloader 拒绝，
           这里提前拦截，避免 Bootloader 因 size 非法而 halt */
        if (fw_size == 0 || fw_size > APP_SIZE) {
            LOGD("[HOST] fw_size invalid or > APP_SIZE: %lu\r\n",
                       (unsigned long)fw_size);
            UpgradeSource_Get()->close(&src_file);
            FatFs_Unmount();
            f_mount(NULL, "1:", 1);
            return;
        }

        /* 3. 在本地数据区创建 firmware.bin（覆盖旧文件） */
        FRESULT cres = f_open(&dst_file, "0:/firmware.bin",
                              FA_CREATE_ALWAYS | FA_WRITE);
        if (cres != FR_OK) {
            LOGD("[HOST] create local firmware.bin failed: %d\r\n", (int)cres);
            UpgradeSource_Get()->close(&src_file);
            FatFs_Unmount();
            f_mount(NULL, "1:", 1);
            return;
        }

        /* 4. 拷贝：U盘 → 本地数据区 */
        while (total < fw_size) {
            UINT want = (fw_size - total > FIRMWARE_BUF_SIZE)
                        ? FIRMWARE_BUF_SIZE : (fw_size - total);
            FRESULT rd = UpgradeSource_Get()->read(&src_file, stage_buf, want, &br);
            if (rd != FR_OK || br == 0) {
                LOGD("[HOST] U-disk read fail rd=%d at %lu\r\n",
                           (int)rd, (unsigned long)total);
                break;
            }
            FRESULT wr = f_write(&dst_file, stage_buf, br, &bw);
            if (wr != FR_OK || bw != br) {
                LOGD("[HOST] local write fail wr=%d at %lu\r\n",
                           (int)wr, (unsigned long)total);
                break;
            }
            total += br;
        }
        f_sync(&dst_file);
        UpgradeSource_Get()->close(&src_file);
        f_close(&dst_file);
        LOGD("[HOST] staged into local FatFS: %lu/%lu bytes\r\n",
                   (unsigned long)total, (unsigned long)fw_size);

        if (total != fw_size) {
            LOGD("[HOST] staging incomplete, abort\r\n");
            f_unlink("0:/firmware.bin");
            FatFs_Unmount();
            f_mount(NULL, "1:", 1);
            return;
        }

        /* 5. USB 访问到此结束：卸载 U盘卷（卷 1:） */
        f_mount(NULL, "1:", 1);

        /* 6. 切换到本地拖拽源，走与拖拽升级完全相同的 Upgrade_Execute：
              从本地数据区读 → 擦除 Slot A → 写入 Slot A → 设置升级标志 */
        UpgradeSource_Register(&Source_USB_Drag);

        UpgradeResult chk2 = Upgrade_Check();
        if (chk2 != UPGRADE_OK) {
            LOGD("[HOST] re-check failed: %d\r\n", (int)chk2);
            f_unlink("0:/firmware.bin");
            FatFs_Unmount();
            UpgradeSource_Register(&Source_USB_Drive);
            return;
        }

        LOGD("[HOST] executing upgrade from local staging (drag-style)...\r\n");
        UpgradeResult exec = Upgrade_Execute();
        if (exec != UPGRADE_OK) {
            LOGD("[HOST] execute failed: %d\r\n", (int)exec);
            f_unlink("0:/firmware.bin");
            FatFs_Unmount();
            UpgradeSource_Register(&Source_USB_Drive);
            return;
        }

        /* 7. 升级成功：删除本地暂存文件，防止下次 DEVICE 模式误触发重复升级 */
        if (UpgradeSource_Get() && UpgradeSource_Get()->cleanup) {
            UpgradeSource_Get()->cleanup();
        }
        FatFs_Unmount();
        UpgradeSource_Register(&Source_USB_Drive);
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
        plat_flash_read((uint8_t *)&raw_flag, UPGRADE_FLAG_ADDR, 4);
        LOGD("[HOST] Verify (raw SPI): flag=0x%08X\r\n", (unsigned int)raw_flag);

        if (raw_flag != UPGRADE_FLAG_YES) {
            LOGD("[HOST] *** ERROR: raw SPI read failed! ***\r\n");
            f_mount(NULL, "1:", 1);
            return;
        }
        LOGD("[HOST] Flag verified OK in SPI Flash\r\n");
    }

    /* 升级成功后切换回 DEVICE 模式，复位后自动进入拖拽模式 */
    LOGD("[HOST] upgrade done, switching to DEVICE mode...\r\n");
    USB_Mode_WriteFlag(USB_MODE_DEVICE);

    /* SPI Flash 软件复位 + FWDGT 硬件复位 */
    plat_flash_reset();
    LOGD("[HOST] resetting...\r\n");
    plat_system_reset();
    /* 不会执行到这里 */
}

/* ──── 公开接口 ──── */

void USB_Host_Poll_Handler(void)
{
    /* 硬件中断模式：USBFS_IRQHandler 自动处理 USB 中断。
       usbh_core_task 在 USB_Host_Task_Run 中调用，推进 Host 状态机。 */
}

/**
 * @brief 彻底重建 USB Host 核心（硬件级复位 + 重新枚举）
 *
 * 升级读取失败后若 USBFS 硬件通道状态已损坏（CSW 全 0 / URB 不完成），
 * 仅软件 reset / pipe 重建无法恢复，必须重跑 usbh_init：
 *   usbh_deinit（软件状态复位）→ usb_core_init + usb_host_init（硬件软复位）
 * 之后 U 盘重新枚举，MSC_Application 回调会重新置 g_udisk_ready。
 */
void USB_Host_Task_Reinit(void)
{
    /* 关闭 USBFS 中断，防止初始化过程中断触发 */
    plat_usb_nvic_disable();

    /* 重新初始化整个 USB host 核心（内部含硬件软复位 + FIFO/通道重建） */
    usbh_init(&usb_host_msc, &usbh_core, USB_CORE_ENUM_FS, &usr_cb);

    /* 重新使能 USBFS 中断 */
    NVIC_ClearPendingIRQ(USB_IRQn);
    plat_usb_nvic_enable();

    /* 等待重新枚举：connect_status 会先变 0 再变 1，callback 重新置 g_udisk_ready */
    g_udisk_ready = 0;

    LOGD("[HOST] USB host re-initialized, re-enumerating...\r\n");
}

void USB_Host_Task_Init(void)
{
    /* 先关闭 USBFS 中断，防止 usbh_init 内部触发中断时 VTOR 仍可能有误 */
    plat_usb_nvic_disable();

    LOGD("[HOST] usb_rcu_config / usb_gpio_config...\r\n");
    usb_rcu_config();
    usb_gpio_config();
    usb_timer_init();

    LOGD("[HOST] usbh_class_register...\r\n");
    (void)usbh_class_register(&usb_host_msc, &usbh_msc);

    LOGD("[HOST] usbh_init...\r\n");
    usbh_init(&usb_host_msc, &usbh_core, USB_CORE_ENUM_FS, &usr_cb);
    LOGD("[HOST] usbh_init done\r\n");

    /* 使能 USBFS 硬件中断（中断模式处理 USB 事件，usbh_core_task 推进状态机） */
    NVIC_ClearPendingIRQ(USB_IRQn);
    plat_usb_nvic_enable();

    LOGD("[HOST] USBFS IRQ enabled (interrupt mode)\r\n");

    /* 注册 U盘升级来源 */
    UpgradeSource_Register(&Source_USB_Drive);
    Upgrade_Init();

    LOGD("[HOST] Init done, waiting for U-disk...\r\n");

    g_host_debug_tick = plat_get_tick_ms();
    g_prev_pcsts = 0;
    g_prev_gstate = 0xFF;
}

void USB_Host_Task_Run(void)
{
    /* 推进 Host 状态机 */
    usbh_core_task(&usb_host_msc);

    /* 与官方例程一致：每次轮询后延时 1ms，避免状态机运转过快 */
    plat_delay_ms(1);

    /* ──── 主循环中执行升级（不在 callback 里调用） ────
     * GD32 的 usbh_msc_read 是同步阻塞函数，需要 usbh_core_task
     * 持续推进 USB 状态机。在 callback 里调用会导致状态机死锁。
     * 解决：callback 只设 g_udisk_ready 标志，这里检测到标志后
     * 在主循环中执行升级，此时 usbh_core_task 正常推进。 */
    {
        static uint8_t upgrade_done = 0;

        /* U盘拔出或重试时重置 upgrade_done */
        if (!usbh_core.host.connect_status || !g_udisk_ready) {
            upgrade_done = 0;
        }

        if (g_udisk_ready && usbh_core.host.connect_status && !upgrade_done) {
            upgrade_done = 1;
            LOGD("[HOST] Starting upgrade from main loop...\r\n");
            USB_Host_Do_Upgrade();
            /* 如果走到这里说明升级失败了（成功路径会复位MCU） */
            g_retry_count++;
            if (g_retry_count < UPGRADE_MAX_RETRIES) {
                LOGD("[HOST] upgrade failed, will retry (%d/%d)\r\n",
                           g_retry_count, UPGRADE_MAX_RETRIES);
                /* 读取失败说明 USBFS 硬件通道状态已损坏（CSW 全 0 / URB 不完成），
                   仅清标志等 callback 重新触发无法恢复，必须硬件级重建 + 重新枚举 */
                USB_Host_Task_Reinit();
            } else {
                LOGD("[HOST] upgrade failed after %d attempts, giving up\r\n",
                           UPGRADE_MAX_RETRIES);
            }
        }
    }

    /* U盘拔出后重置状态 */
    if (!usbh_core.host.connect_status) {
        g_udisk_ready = 0;
        g_retry_count = 0;
    }

    /* ──── Host 状态机状态变化检测 ──── */
    {
        uint8_t gstate = (uint8_t)usb_host_msc.cur_state;
        if (gstate != g_prev_gstate) {
            LOGD("[HOST] *** State changed: %d -> %d ***\r\n",
                       g_prev_gstate, gstate);
            g_prev_gstate = gstate;
        }
    }

    /* ──── 即时连接状态变化检测：插入/拔出 U 盘时立即打印 ──── */
    {
        uint8_t pcsts = (uint8_t)usbh_core.host.connect_status;
        if (pcsts != g_prev_pcsts) {
            LOGD("[HOST] *** connect_status changed: %d -> %d ***\r\n",
                       g_prev_pcsts, pcsts);
            g_prev_pcsts = pcsts;
        }
    }

    /* ──── 定期调试输出：每 3 秒打印一次 Host 状态 ──── */
    {
        uint32_t now = plat_get_tick_ms();
        if (now - g_host_debug_tick >= 3000) {
            g_host_debug_tick = now;
            LOGD("[HOST] state=%d connect=%d\r\n",
                       (int)usb_host_msc.cur_state,
                       (int)usbh_core.host.connect_status);
        }
    }
}
