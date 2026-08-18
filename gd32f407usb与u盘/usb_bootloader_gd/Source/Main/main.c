/**
 * @file main.c
 * @brief Bootloader - USB 升级引导程序（含 A/B 回滚）
 *
 * 流程：
 *   1. 读取升级标志 flag
 *   2. flag != YES → 校验 APP MSP 头
 *      - 合法 → 跳转 APP
 *      - 非法 → 从 Slot B 恢复 → 跳转 APP（或 halt）
 *   3. flag == YES →
 *      a. 备份当前 APP 到 Slot B
 *      b. 擦除 APP 区
 *      c. 从 Slot A 搬运到内部 Flash
 *      d. 校验 MSP 头
 *         - 成功 → 清 flag → DONE → 跳转 APP
 *         - 失败 → 从 Slot B 恢复 → 跳转 APP
 *
 * W25Q128 分区：
 *   0x5E0000-0x8DFFFF: Slot A（新固件写入位置）
 *   0x8E0000-0xBDFFFF: Slot B（当前 APP 备份，用于回滚）
 *   0xFE0000-0xFFFFFF: 配置区（flag / state / size / active_slot）
 *
 * STM32F103 内部 Flash：
 *   0x08000000-0x0800FFFF: Bootloader 64KB
 *   0x08010000-0x0807FFFF: APP      448KB
 */

#include "config.h"
#include "md5.h"

/* ──── 固件搬运缓冲区（4KB，静态分配避免栈溢出）──── */
static uint8_t g_firmware_buf[FIRMWARE_BUF_SIZE];

/* ──── 函数声明 ──── */
static void EraseAppArea(uint32_t start_addr, uint32_t size);
static void CopySlotToFlash(uint32_t src_addr, uint32_t size);
static void BackupAppToSlot(uint32_t slot_addr);
static uint8_t RestoreSlotToApp(uint32_t slot_addr, uint32_t size);
static uint8_t VerifyAppMsp(void);
static void CalcSlotMd5(uint32_t addr, uint32_t size, uint8_t md5_out[16]);
static uint8_t VerifySlotMd5(uint32_t addr, uint32_t size, uint32_t md5_field_addr);
static void CalcInternalFlashMd5(uint32_t addr, uint32_t size, uint8_t md5_out[16]);
static uint8_t VerifyInternalFlashMd5(uint32_t addr, uint32_t size, uint32_t md5_field_addr);
void JumpToApp(void);

/* ═══════════════════════════════════════════════════════════
 * F103: 按页擦除(2KB)，半字写入
 * F407: 按扇区擦除，字(32bit)写入
 * ═══════════════════════════════════════════════════════════ */
#if defined(CHIP_SERIES_F103)

static void EraseAppArea(uint32_t start_addr, uint32_t size)
{
    uint32_t total_pages = (size + 2047) / 2048;

    for (uint32_t i = 0; i < total_pages; i++)
    {
        FLASH_Unlock();
        FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
        FLASH_ErasePage(start_addr + i * 2048);
        FLASH_Lock();
    }
    printf("[BL] Erased %d pages\r\n", (int)total_pages);
}

static void CopySlotToFlash(uint32_t src_addr, uint32_t size)
{
    uint8_t *buf = g_firmware_buf;
    uint32_t remain = size;
    uint32_t ext_addr = src_addr;
    uint32_t int_addr = APP_ADDR;

    while (remain > 0)
    {
        uint32_t chunk = (remain > FIRMWARE_BUF_SIZE) ? FIRMWARE_BUF_SIZE : remain;
        W25QXX_Read(buf, ext_addr, chunk);

        FLASH_Unlock();
        for (uint32_t i = 0; i + 1 < chunk; i += 2)
        {
            uint16_t half_word = buf[i] | ((uint16_t)buf[i + 1] << 8);
            FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
            FLASH_ProgramHalfWord(int_addr + i, half_word);
        }
        FLASH_Lock();

        ext_addr += chunk;
        int_addr += chunk;
        remain  -= chunk;

        printf("[BL] Copied %d/%d\r\n", (int)(size - remain), (int)size);
    }
    printf("[BL] %d bytes copied\r\n", (int)size);
}

#elif defined(CHIP_SERIES_F407)

/* GD32F407 内部 Flash 扇区号（从 0 开始，0-3=16KB, 4-11=128KB） */
static uint32_t GetFlashSector(uint32_t addr)
{
    if      (addr < 0x08004000) return CTL_SECTOR_NUMBER_0;
    else if (addr < 0x08008000) return CTL_SECTOR_NUMBER_1;
    else if (addr < 0x0800C000) return CTL_SECTOR_NUMBER_2;
    else if (addr < 0x08010000) return CTL_SECTOR_NUMBER_3;
    else if (addr < 0x08020000) return CTL_SECTOR_NUMBER_4;
    else if (addr < 0x08040000) return CTL_SECTOR_NUMBER_5;
    else if (addr < 0x08060000) return CTL_SECTOR_NUMBER_6;
    else if (addr < 0x08080000) return CTL_SECTOR_NUMBER_7;
    else if (addr < 0x080A0000) return CTL_SECTOR_NUMBER_8;
    else if (addr < 0x080C0000) return CTL_SECTOR_NUMBER_9;
    else if (addr < 0x080E0000) return CTL_SECTOR_NUMBER_10;
    else                        return CTL_SECTOR_NUMBER_11;
}

static void EraseAppArea(uint32_t start_addr, uint32_t size)
{
    uint32_t end_addr = start_addr + size;
    uint32_t addr = start_addr;
    uint32_t sectors = 0;

    while (addr < end_addr)
    {
        fmc_unlock();
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR |
                       FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR);
        fmc_sector_erase(GetFlashSector(addr));
        fmc_lock();

        if      (addr < 0x08004000) addr = 0x08004000;  /* 16KB */
        else if (addr < 0x08008000) addr = 0x08008000;
        else if (addr < 0x0800C000) addr = 0x0800C000;
        else if (addr < 0x08010000) addr = 0x08010000;
        else if (addr < 0x08020000) addr = 0x08020000;  /* 64KB */
        else                        addr += 0x20000;    /* 128KB */
        sectors++;
    }
    printf("[BL] Erased %d sectors\r\n", (int)sectors);
}

static void CopySlotToFlash(uint32_t src_addr, uint32_t size)
{
    uint8_t *buf = g_firmware_buf;
    uint32_t remain = size;
    uint32_t ext_addr = src_addr;
    uint32_t int_addr = APP_ADDR;

    while (remain > 0)
    {
        uint32_t chunk = (remain > FIRMWARE_BUF_SIZE) ? FIRMWARE_BUF_SIZE : remain;
        /* 按 4 字节对齐 chunk，避免越界写入 */
        chunk &= ~3u;
        if (chunk == 0) chunk = 4;

        W25QXX_Read(buf, ext_addr, chunk);

        fmc_unlock();
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR |
                       FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR);
        for (uint32_t i = 0; i + 3 < chunk; i += 4)
        {
            uint32_t word = buf[i]
                          | ((uint32_t)buf[i + 1] << 8)
                          | ((uint32_t)buf[i + 2] << 16)
                          | ((uint32_t)buf[i + 3] << 24);
            fmc_word_program(int_addr + i, word);
        }
        fmc_lock();

        ext_addr += chunk;
        int_addr += chunk;
        remain  -= chunk;

        printf("[BL] Copied %d/%d\r\n", (int)(size - remain), (int)size);
    }
    printf("[BL] %d bytes copied\r\n", (int)size);
}

#endif

/* ═══════════════════════════════════════════════════════════
 * MD5 辅助函数（固件完整性校验）
 * ═══════════════════════════════════════════════════════════ */

/**
 * @brief 计算外部 Flash 一段区域（固件）的 MD5
 * @param addr    外部 Flash 起始地址（Slot A/B）
 * @param size    计算长度（字节）
 * @param md5_out 输出 16 字节摘要
 */
static void CalcSlotMd5(uint32_t addr, uint32_t size, uint8_t md5_out[16])
{
    MD5_CTX ctx;
    uint32_t remain = size;
    uint32_t ext_addr = addr;

    MD5_Init(&ctx);
    while (remain > 0) {
        uint32_t chunk = (remain > FIRMWARE_BUF_SIZE) ? FIRMWARE_BUF_SIZE : remain;
        W25QXX_Read(g_firmware_buf, ext_addr, chunk);
        MD5_Update(&ctx, g_firmware_buf, chunk);
        ext_addr += chunk;
        remain  -= chunk;
    }
    MD5_Final(md5_out, &ctx);
}

/**
 * @brief 校验外部 Flash 区域 MD5 是否与配置区保存值一致
 * @param addr            外部 Flash 起始地址
 * @param size            计算长度
 * @param md5_field_addr  配置区中保存的 MD5 地址
 * @return 1=一致, 0=不一致
 */
static uint8_t VerifySlotMd5(uint32_t addr, uint32_t size, uint32_t md5_field_addr)
{
    uint8_t expect[16];
    uint8_t calc[16];

    W25QXX_Read(expect, md5_field_addr, 16);
    CalcSlotMd5(addr, size, calc);

    if (memcmp(expect, calc, 16) == 0) {
        return 1;
    }
    return 0;
}

/**
 * @brief 计算内部 Flash 一段区域（搬运后的固件）的 MD5
 * @param addr    内部 Flash 起始地址（APP_ADDR）
 * @param size    计算长度（字节）
 * @param md5_out 输出 16 字节摘要
 *
 * 内部 Flash 支持直接内存映射读取，无需走 SPI。
 */
static void CalcInternalFlashMd5(uint32_t addr, uint32_t size, uint8_t md5_out[16])
{
    MD5_CTX ctx;
    uint32_t remain = size;
    uint32_t int_addr = addr;

    MD5_Init(&ctx);
    while (remain > 0) {
        uint32_t chunk = (remain > FIRMWARE_BUF_SIZE) ? FIRMWARE_BUF_SIZE : remain;
        memcpy(g_firmware_buf, (void *)int_addr, chunk);
        MD5_Update(&ctx, g_firmware_buf, chunk);
        int_addr += chunk;
        remain  -= chunk;
    }
    MD5_Final(md5_out, &ctx);
}

/**
 * @brief 校验内部 Flash 区域 MD5 是否与配置区保存值一致
 * @param addr            内部 Flash 起始地址
 * @param size            计算长度
 * @param md5_field_addr  配置区中保存的 MD5 地址（MD5_A）
 * @return 1=一致, 0=不一致
 */
static uint8_t VerifyInternalFlashMd5(uint32_t addr, uint32_t size, uint32_t md5_field_addr)
{
    uint8_t expect[16];
    uint8_t calc[16];

    W25QXX_Read(expect, md5_field_addr, 16);
    CalcInternalFlashMd5(addr, size, calc);

    if (memcmp(expect, calc, 16) == 0) {
        return 1;
    }
    return 0;
}

/* ═══════════════════════════════════════════════════════════
 * A/B 回滚支持函数
 * ═══════════════════════════════════════════════════════════ */

/**
 * @brief 备份内部 Flash APP 到外部 Slot
 * @param slot_addr  目标 Slot 起始地址（SLOT_A 或 SLOT_B）
 *
 * 先擦除目标 Slot，再逐块从内部 Flash 读出写入。
 * 使用 g_firmware_buf 做中转，4KB 一块。
 */
static void BackupAppToSlot(uint32_t slot_addr)
{
    uint32_t size = APP_SIZE;
    uint32_t remain = size;
    uint32_t int_addr = APP_ADDR;
    uint32_t ext_addr = slot_addr;

    /* 擦除 Slot（按 64KB 块） */
    uint32_t blocks = (size + 65535) / 65536;
    for (uint32_t i = 0; i < blocks; i++) {
        W25QXX_Erase_Block(slot_addr + i * 65536);
    }

    /* 从内部 Flash 读出 → 写入外部 Slot */
    while (remain > 0) {
        uint32_t chunk = (remain > FIRMWARE_BUF_SIZE) ? FIRMWARE_BUF_SIZE : remain;
#if defined(CHIP_SERIES_F103)
        /* F103 内部 Flash 直接内存映射，可以直接读 */
        memcpy(g_firmware_buf, (void *)int_addr, chunk);
#elif defined(CHIP_SERIES_F407)
        memcpy(g_firmware_buf, (void *)int_addr, chunk);
#endif
        W25QXX_Write_NoCheck(g_firmware_buf, ext_addr, chunk);

        int_addr += chunk;
        ext_addr += chunk;
        remain   -= chunk;
    }
    printf("[BL] Backup %d bytes -> 0x%06X\r\n", (int)size, (unsigned int)slot_addr);

    /* 记录 Slot B 备份内容的 MD5（回滚前校验用）。
     * 写入 MD5_B 独立扇区 0xFE3000，只擦该扇区，不影响 MD5_A（0xFE2000），
     * 保证断电后重试升级时仍可校验 Slot A。 */
    {
        uint8_t md5_b[16];
        CalcSlotMd5(slot_addr, size, md5_b);
        W25QXX_Erase_Sector(FIRMWARE_MD5_B_ADDR);
        W25QXX_Write_NoCheck(md5_b, FIRMWARE_MD5_B_ADDR, 16);
        printf("[BL] Slot B MD5 recorded\r\n");
    }
}

/**
 * @brief 从外部 Slot 恢复固件到内部 Flash APP 区
 * @param slot_addr  源 Slot 起始地址
 * @param size       固件大小
 * @return 1=恢复成功, 0=失败
 *
 * 先擦 APP 区，再从 Slot 读出写入内部 Flash。
 */
static uint8_t RestoreSlotToApp(uint32_t slot_addr, uint32_t size)
{
    if (size == 0 || size > APP_SIZE) {
        printf("[BL] Restore: invalid size %d\r\n", (int)size);
        return 0;
    }

    printf("[BL] Restoring from 0x%06X, %d bytes...\r\n",
           (unsigned int)slot_addr, (int)size);

    EraseAppArea(APP_ADDR, size);
    CopySlotToFlash(slot_addr, size);

    /* 校验恢复后的 MSP 头 */
    if (!VerifyAppMsp()) {
        printf("[BL] Restore verify FAILED\r\n");
        return 0;
    }

    printf("[BL] Restore OK\r\n");
    return 1;
}

/**
 * @brief 校验内部 Flash APP 区的 MSP 头是否合法
 * @return 1=合法, 0=非法
 *
 * APP 第一个字是栈顶指针，必须在 SRAM 范围内。
 */
static uint8_t VerifyAppMsp(void)
{
    uint32_t stack_top = *(volatile uint32_t *)APP_ADDR;
    if ((stack_top & STACK_VALID_MASK) != STACK_VALID_BASE) {
        printf("[BL] MSP invalid: 0x%08X\r\n", (unsigned int)stack_top);
        return 0;
    }
    return 1;
}

/* ──── 跳转到 APP ──── */
void JumpToApp(void)
{
    uint32_t stack_top = *(volatile uint32_t *)APP_ADDR;

    /* 校验栈顶是否在 SRAM 范围内 */
    if ((stack_top & STACK_VALID_MASK) != STACK_VALID_BASE)
    {
        printf("[BL] Invalid APP stack 0x%08X, halt\r\n", (unsigned int)stack_top);
        while (1);
    }

    printf("[BL] Jump -> 0x%08X\r\n", (unsigned int)APP_ADDR);

    __disable_irq();

    /* 1. 停掉 bootloader 的 TIMER2，清理 NVIC */
    timer_disable(TIMER2);
    timer_interrupt_disable(TIMER2, TIMER_INT_UP);
    NVIC_DisableIRQ(TIMER2_IRQn);
    NVIC_ClearPendingIRQ(TIMER2_IRQn);

    /* 2. 关掉 USB 时钟（F103 有 USB，F407 在 APP 中初始化 OTG，这里忽略） */
#if defined(CHIP_SERIES_F103)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE);
#endif

    /* 3. 清掉所有 NVIC pending，防止 bootloader 残留中断在 APP 里误触发 */
    for (int i = 0; i < 8; i++) {
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    /* 4. 设 APP 栈、向量表 */
    __set_MSP(stack_top);
    SCB->VTOR = APP_ADDR;

    /* 5. 恢复 PRIMASK=0，让 APP 的中断能正常工作 */
    __enable_irq();

    /* 6. 跳转到 APP Reset_Handler */
    void (*app_entry)(void) = (void (*)(void))(*(volatile uint32_t *)(APP_ADDR + 4));
    app_entry();

    /* should never reach */
    while (1);
}

/* ──── 辅助：清除升级标志 + 写状态 ──── */
static void ClearUpgradeFlag(void)
{
    uint32_t no_flag = UPGRADE_FLAG_NO;
    uint32_t fw_size;
    W25QXX_Read((uint8_t *)&fw_size, FIRMWARE_SIZE_ADDR, 4);
    W25QXX_Erase_Sector(CONFIG_AREA_ADDR);
    W25QXX_Write_NoCheck((uint8_t *)&no_flag, UPGRADE_FLAG_ADDR, 4);
    /* 擦除后回写固件大小，供后续回滚恢复使用 */
    W25QXX_Write_NoCheck((uint8_t *)&fw_size, FIRMWARE_SIZE_ADDR, 4);
}

static void WriteUpgradeState(uint32_t state)
{
    W25QXX_Write_NoCheck((uint8_t *)&state, UPGRADE_STATE_ADDR, 4);
}

/* ──── 主函数 ──── */
int main(void)
{
    uint32_t flag;
    uint32_t fw_size;

    /* 1. 初始化 */
    System_Init();

    printf("\r\n[BL] Bootloader start\r\n");

    /* 2. 读取升级标志 */
    W25QXX_Read((uint8_t *)&flag, UPGRADE_FLAG_ADDR, 4);
    printf("[BL] flag=0x%08X\r\n", (unsigned int)flag);

    /* 3. 无升级请求 → 校验 APP 后跳转 */
    if (flag != UPGRADE_FLAG_YES)
    {
        if (VerifyAppMsp()) {
            printf("[BL] APP valid, jump\r\n");
            JumpToApp();
            while (1);
        }

        /* APP MSP 非法 — 尝试从 Slot B 恢复（先校验备份 MD5_B，再按 APP_SIZE 全量恢复） */
        printf("[BL] APP corrupted, trying Slot B restore\r\n");

        if (VerifySlotMd5(FIRMWARE_SLOT_B_ADDR, APP_SIZE, FIRMWARE_MD5_B_ADDR) &&
            RestoreSlotToApp(FIRMWARE_SLOT_B_ADDR, APP_SIZE)) {
            printf("[BL] Slot B restore OK, jump\r\n");
            JumpToApp();
        }

        /* 恢复也失败 — 死循环，需要重新烧录 */
        printf("[BL] No valid APP, halt\r\n");
        while (1);
    }

    /* ═══ 有升级请求 ═══ */

    /* 4. 读取固件大小 */
    W25QXX_Read((uint8_t *)&fw_size, FIRMWARE_SIZE_ADDR, 4);
    printf("[BL] fw_size=%d\r\n", (int)fw_size);

    if (fw_size == 0 || fw_size > APP_SIZE)
    {
        printf("[BL] Invalid fw_size, clear flag and halt\r\n");
        ClearUpgradeFlag();
        while (1);
    }

    /* 4.5 搬运前校验 Slot A 固件 MD5（与 App 升级时写入的 MD5_A 比对）。
     * 必须放在备份之前：BackupAppToSlot 会擦除 MD5 独立扇区 0xFE2000
     * 并写入 MD5_B，届时 MD5_A 已被清除，无法再校验。 */
    if (!VerifySlotMd5(FIRMWARE_SLOT_A_ADDR, fw_size, FIRMWARE_MD5_A_ADDR)) {
        printf("[BL] Slot A MD5 MISMATCH, cannot upgrade\r\n");
        /* 尝试从 Slot B 恢复旧固件（回滚前同样校验 MD5_B） */
        if (VerifySlotMd5(FIRMWARE_SLOT_B_ADDR, APP_SIZE, FIRMWARE_MD5_B_ADDR) &&
            RestoreSlotToApp(FIRMWARE_SLOT_B_ADDR, APP_SIZE)) {
            ClearUpgradeFlag();
            WriteUpgradeState(UPGRADE_STATE_NONE);
            printf("[BL] Slot B restore OK, jump\r\n");
            JumpToApp();
        }
        printf("[BL] No valid firmware, halt\r\n");
        ClearUpgradeFlag();
        while (1);
    }
    printf("[BL] Slot A MD5 verified OK\r\n");

    /* 5. 检查升级状态，区分首次/重试 */
    uint32_t state;
    W25QXX_Read((uint8_t *)&state, UPGRADE_STATE_ADDR, 4);
    printf("[BL] state=0x%08X\r\n", (unsigned int)state);

    if (state != UPGRADE_STATE_BURNING) {
        /* 首次升级：标记 BURNING → 备份当前 APP 到 Slot B */
        printf("[BL] First upgrade, backup APP -> Slot B\r\n");
        uint32_t burning = UPGRADE_STATE_BURNING;
        W25QXX_Erase_Sector(CONFIG_AREA_ADDR);
        W25QXX_Write_NoCheck((uint8_t *)&flag, UPGRADE_FLAG_ADDR, 4);
        W25QXX_Write_NoCheck((uint8_t *)&fw_size, FIRMWARE_SIZE_ADDR, 4);
        W25QXX_Write_NoCheck((uint8_t *)&burning, UPGRADE_STATE_ADDR, 4);
        BackupAppToSlot(FIRMWARE_SLOT_B_ADDR);
    } else {
        /* 重试升级：Slot B 仍是旧固件，跳过备份 */
        printf("[BL] Retry upgrade, skip backup (Slot B preserved)\r\n");
    }

    /* 6. 擦除内部 Flash APP 区 */
    EraseAppArea(APP_ADDR, fw_size);

    /* 7. 从 Slot A 搬运到内部 Flash */
    CopySlotToFlash(FIRMWARE_SLOT_A_ADDR, fw_size);

    /* 8. 校验搬运结果：
     *    先对内部 Flash 全量重算 MD5 与 MD5_A 比对（搬运写入是否完整正确），
     *    再查 MSP 头作为第二道快速校验。任一失败 → 回滚 Slot B。 */
    if (!VerifyInternalFlashMd5(APP_ADDR, fw_size, FIRMWARE_MD5_A_ADDR) ||
        !VerifyAppMsp())
    {
        printf("[BL] New APP verify FAILED, rollback to Slot B\r\n");

        /* 从 Slot B 恢复旧固件（先校验备份 MD5_B，再按 APP_SIZE 全量恢复） */
        if (VerifySlotMd5(FIRMWARE_SLOT_B_ADDR, APP_SIZE, FIRMWARE_MD5_B_ADDR) &&
            RestoreSlotToApp(FIRMWARE_SLOT_B_ADDR, APP_SIZE)) {
            ClearUpgradeFlag();
            WriteUpgradeState(UPGRADE_STATE_NONE);
            printf("[BL] Rollback OK, jump\r\n");
            JumpToApp();
        }

        /* 回滚也失败 — 死循环 */
        printf("[BL] Rollback FAILED, halt\r\n");
        while (1);
    }

    /* 9. 升级成功 */
    ClearUpgradeFlag();
    WriteUpgradeState(UPGRADE_STATE_DONE);

    printf("[BL] Upgrade done, jump\r\n");
    Delay_ms(100);

    /* 10. 跳转到 APP */
    JumpToApp();

    /* should never reach */
    printf("[BL] Jump failed! halt\r\n");
    while (1);
}
