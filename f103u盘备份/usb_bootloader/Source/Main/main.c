/**
 * @file main.c
 * @brief Bootloader主程序 - USB拖拽升级系统的引导程序
 * 
 * 功能说明：
 * 1. 读取升级状态和标志
 * 2. 如果有升级请求，备份当前App到Slot B，然后从Slot A拷贝到内部Flash
 * 3. 如果上次升级中断或失败，从Slot B回滚
 * 4. 正常情况跳转到APP
 * 
 * 升级流程：
 *   APP检测到新固件 → 写入Slot A → 设标志 → 重启
 *   Bootloader读标志 → 备份当前APP到Slot B → 拷贝Slot A到内部Flash → 重启
 * 
 * 回滚机制：
 *   如果升级过程中断（掉电、看门狗超时），状态停留在BURNING
 *   下次启动时Bootloader会从Slot B恢复旧APP
 * 
 * W25Q128 Flash分区：
 *   0x000000-0x5DFFFF: 资源区 (字库/图片) 5.875MB
 *   0x5E0000-0x8DFFFF: 固件Slot A (当前) 3MB
 *   0x8E0000-0xBDFFFF: 固件Slot B (备份) 3MB
 *   0xBE0000-0xFDFFFF: 数据区 (FatFS) 4MB
 *   0xFE0000-0xFFFFFF: 配置区 128KB
 * 
 * STM32F103内部Flash分区：
 *   0x08000000-0x0800FFFF: Bootloader 64KB
 *   0x08010000-0x0807FFFF: APP 448KB
 */

#include "config.h"

/* ──── 固件拷贝缓冲区（4KB，太大不能放在栈上，放全局静态）──── */
static uint8_t g_firmware_buf[FIRMWARE_BUF_SIZE];

/* ──── 函数声明 ──── */
void IWDG_Init(void);
void IWDG_Feed(void);
uint32_t CheckAppValid(uint32_t app_addr);
void JumpToApp(uint32_t app_addr);
uint8_t CopyFirmwareFromSlot(uint32_t src_addr, uint32_t size);
void BackupAppToSlotB(void);
void RollbackFirmware(void);
uint8_t VerifyFirmwareInFlash(uint32_t app_addr, uint32_t size);

/**
 * @brief 初始化独立看门狗（IWDG）
 * 
 * 超时时间计算：
 *   LSI时钟 = 40kHz
 *   分频128 → 40000/128 = 312.5 Hz
 *   计数器4095 → 4096/312.5 ≈ 13秒
 * 
 * 使用场景：只在升级烧写过程中启用，防止程序卡死
 */
void IWDG_Init(void)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  // 使能写访问
    IWDG_SetPrescaler(IWDG_Prescaler_128);         // 设置分频器128
    IWDG_SetReload(4095);                          // 设置重装载值，约13秒超时
    IWDG_ReloadCounter();                          // 立即喂狗
    IWDG_Enable();                                 // 启用看门狗
}

/**
 * @brief 喂狗（重装载看门狗计数器）
 * 
 * 必须在超时时间内调用，否则系统复位
 */
void IWDG_Feed(void)
{
    IWDG_ReloadCounter();
}

/**
 * @brief 校验APP是否有效
 * 
 * 校验原理：ARM Cortex-M中断向量表前两项固定格式
 *   +0x00: MSP（主堆栈指针），必须指向RAM区域
 *   +0x04: Reset_Handler（复位向量），必须指向APP代码区
 * 
 * @param app_addr APP起始地址（通常是0x08010000）
 * @return 1=有效，0=无效
 */
uint32_t CheckAppValid(uint32_t app_addr)
{
    /* 读取中断向量表前两项 */
    uint32_t msp = *(__IO uint32_t*)app_addr;        // 读取MSP初始值
    uint32_t reset = *(__IO uint32_t*)(app_addr + 4); // 读取Reset_Handler地址
    
    /* 校验MSP：必须在RAM范围内 (0x20000000 - 0x20010000 for ZET6) */
    /* 使用掩码0x2FFE0000检查高13位，确保地址在RAM区域 */
    if ((msp & 0x2FFE0000) != 0x20000000) return 0;
    
    /* 校验Reset_Handler：必须在APP代码区范围内 */
    if (reset < APP_ADDR || reset > APP_ADDR + APP_SIZE) return 0;
    
    return 1;  // 校验通过
}

/**
 * @brief 跳转到APP执行
 * 
 * 跳转前必须完成的操作：
 *   1. 关闭所有中断，防止跳转过程中被打断
 *   2. 关闭USB时钟，让APP重新初始化
 *   3. 设置主堆栈指针（MSP）为APP的初始栈顶
 *   4. 设置中断向量表偏移（VTOR）指向APP
 *   5. 读取APP的Reset_Handler地址并跳转
 * 
 * @param app_addr APP起始地址（通常是0x08010000）
 */
void JumpToApp(uint32_t app_addr)
{
    typedef void (*pFunction)(void);
    pFunction Jump_To_Application;
    uint32_t JumpAddress;

    printf("[BL] Jump -> 0x%08X\r\n", app_addr);

    if (!CheckAppValid(app_addr))
    {
        printf("[BL] App invalid\r\n");
        return;
    }

    __disable_irq();
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE);
    __set_MSP(*(__IO uint32_t*)app_addr);
    SCB->VTOR = app_addr;

    JumpAddress = *(__IO uint32_t*)(app_addr + 4);
    Jump_To_Application = (pFunction)JumpAddress;
    Jump_To_Application();
}

/**
 * @brief 备份当前APP到Slot B
 * 
 * 数据流：内部Flash → RAM缓冲区 → W25Q128 Slot B
 * 
 * 备份原因：升级失败时可以从Slot B恢复旧APP
 * 备份内容：完整的APP代码（448KB）
 */
void BackupAppToSlotB(void)
{
    uint8_t *buf = g_firmware_buf;
    
    for (uint32_t i = 0; i < 48; i++)
    {
        W25QXX_Erase_Block(FIRMWARE_SLOT_B_ADDR + i * 0x10000);
        IWDG_Feed();
    }
    
    for (uint32_t offset = 0; offset < APP_SIZE; offset += FIRMWARE_BUF_SIZE)
    {
        memcpy(buf, (void*)(APP_ADDR + offset), FIRMWARE_BUF_SIZE);
        W25QXX_Write_NoCheck(buf, FIRMWARE_SLOT_B_ADDR + offset, FIRMWARE_BUF_SIZE);
        
        if (offset % (FIRMWARE_BUF_SIZE * 100) == 0)
        {
            printf("[BL] BAK %dKB\r\n", offset/1024);
            IWDG_Feed();
        }
    }
}

/**
 * @brief 从指定Slot拷贝固件到内部Flash
 * 
 * 数据流：W25Q128 Slot A/B → RAM缓冲区 → STM32内部Flash
 * 
 * 操作步骤：
 *   1. 解锁内部Flash
 *   2. 擦除APP区（按1KB页擦除）
 *   3. 从Slot读取数据（每次4KB）
 *   4. 写入内部Flash（每次4字节）
 *   5. 锁定内部Flash
 * 
 * @param src_addr 源地址（Slot A或Slot B）
 * @param size 固件大小（字节）
 * @return 0=成功，1=失败
 */
uint8_t CopyFirmwareFromSlot(uint32_t src_addr, uint32_t size)
{
    uint8_t *buf = g_firmware_buf;
    FLASH_Status status;
    
    FLASH_Unlock();
    
    for (uint32_t i = 0; i < (size + 1023) / 1024; i++)
    {
        status = FLASH_ErasePage(APP_ADDR + i * 1024);
        if (status != FLASH_COMPLETE)
        {
            printf("[BL] ERR: erase %d\r\n", (int)status);
            FLASH_Lock();
            return 1;
        }
        IWDG_Feed();
    }
    
    for (uint32_t offset = 0; offset < size; offset += FIRMWARE_BUF_SIZE)
    {
        uint32_t chunk = (size - offset > FIRMWARE_BUF_SIZE) ? FIRMWARE_BUF_SIZE : (size - offset);
        W25QXX_Read(buf, src_addr + offset, chunk);
        
        for (uint32_t i = 0; i < chunk; i += 4)
        {
            status = FLASH_ProgramWord(APP_ADDR + offset + i, *(uint32_t*)(buf + i));
            if (status != FLASH_COMPLETE)
            {
                printf("[BL] ERR: write %d\r\n", (int)status);
                FLASH_Lock();
                return 1;
            }
        }
        
        printf("[BL] WR %dKB\r\n", (offset + chunk)/1024);
        IWDG_Feed();
    }
    
    FLASH_Lock();
    return 0;
}

/**
 * @brief 回读校验内部Flash与Slot数据是否一致
 * 
 * 校验方式：简单头部校验（只比较MSP值）
 * 校验原理：MSP是最关键的值，如果写错了APP肯定无法运行
 * 
 * @param app_addr 内部Flash地址
 * @param size 固件大小（未使用，保留参数）
 * @return 0=校验通过，1=校验失败
 */
uint8_t VerifyFirmwareInFlash(uint32_t app_addr, uint32_t size)
{
    (void)size;
    
    uint32_t flash_msp = *(__IO uint32_t*)app_addr;
    uint32_t slot_msp;
    W25QXX_Read((uint8_t*)&slot_msp, app_addr == APP_ADDR ? FIRMWARE_SLOT_A_ADDR : FIRMWARE_SLOT_B_ADDR, 4);
    
    if (flash_msp != slot_msp)
    {
        printf("[BL] ERR: MSP %08X!=%08X\r\n", flash_msp, slot_msp);
        return 1;
    }
    
    return 0;
}

/**
 * @brief 回滚固件（从Slot B恢复旧APP）
 * 
 * 使用场景：升级失败或新固件有问题时，恢复旧版本
 * 操作流程：
 *   1. 从Slot B拷贝旧APP到内部Flash
 *   2. 校验拷贝结果
 * 
 * 注意：必须按APP_SIZE恢复，不能用新固件size
 *       因为Slot B备份的是完整APP，新固件可能更小
 */
void RollbackFirmware(void)
{
    printf("[BL] Rollback...\r\n");
    
    if (CopyFirmwareFromSlot(FIRMWARE_SLOT_B_ADDR, APP_SIZE) != 0)
    {
        printf("[BL] RB FAIL\r\n");
        return;
    }
    
    if (VerifyFirmwareInFlash(APP_ADDR, APP_SIZE) != 0)
    {
        printf("[BL] RB FAIL\r\n");
        return;
    }
    
    printf("[BL] RB OK\r\n");
}

/**
 * @brief 校验Slot B备份是否成功
 * 
 * 校验方式：
 *   1. 比较内部Flash头部MSP和Slot B头部MSP
 *   2. 比较内部Flash尾部数据和Slot B尾部数据
 * 
 * @return 0=校验通过，1=校验失败
 */
uint8_t VerifySlotBBackup(void)
{
    uint32_t app_msp = *(__IO uint32_t*)APP_ADDR;
    uint32_t slot_b_msp;
    W25QXX_Read((uint8_t*)&slot_b_msp, FIRMWARE_SLOT_B_ADDR, 4);
    
    if (app_msp != slot_b_msp)
    {
        printf("[BL] ERR: backup MSP\r\n");
        return 1;
    }
    
    uint32_t tail_offset = APP_SIZE - 4096;
    uint32_t app_tail;
    uint32_t slot_b_tail;
    
    memcpy(&app_tail, (void*)(APP_ADDR + tail_offset), 4);
    W25QXX_Read((uint8_t*)&slot_b_tail, FIRMWARE_SLOT_B_ADDR + tail_offset, 4);
    
    if (app_tail != slot_b_tail)
    {
        printf("[BL] ERR: backup tail\r\n");
        return 1;
    }
    
    return 0;
}

/**
 * @brief 主函数 - Bootloader入口
 * 
 * 状态机逻辑：
 *   1. 读取配置区的flag、state、size
 *   2. 根据状态决定操作：
 *      - state == BURNING: 上次升级中断，执行回滚
 *      - state == CONFIRMED: APP已确认存活，清除配置
 *      - flag == YES: 有升级请求，执行升级
 *      - 其他: 正常跳转到APP
 * 
 * @return 无（嵌入式系统永不返回）
 */
int main(void)
{
    uint32_t state, flag, size;

    System_Init();
    IWDG_Feed();
    
    printf("\r\n[BL] v1.0\r\n");
    
    W25QXX_Read((uint8_t*)&flag, UPGRADE_FLAG_ADDR, 4);
    W25QXX_Read((uint8_t*)&state, UPGRADE_STATE_ADDR, 4);
    W25QXX_Read((uint8_t*)&size, FIRMWARE_SIZE_ADDR, 4);
    
    printf("[BL] F:%08X S:%08X\r\n", flag, state);
    
    /* Case 1: Last upgrade interrupted - rollback */
    if (state == UPGRADE_STATE_BURNING)
    {
        IWDG_Feed();
        IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
        IWDG_SetPrescaler(IWDG_Prescaler_128);
        IWDG_SetReload(4095);
        IWDG_ReloadCounter();
        
        RollbackFirmware();

        if (CheckAppValid(APP_ADDR))
        {
            uint32_t zero = 0;
            W25QXX_Erase_Sector(CONFIG_AREA_ADDR);
            W25QXX_Write_NoCheck((uint8_t*)&zero, UPGRADE_STATE_ADDR, 4);
            W25QXX_Write_NoCheck((uint8_t*)&zero, UPGRADE_FLAG_ADDR, 4);
            W25QXX_Write_NoCheck((uint8_t*)&zero, FIRMWARE_SIZE_ADDR, 4);

            printf("[BL] RB done, reboot\r\n");
            Delay_ms(500);
            NVIC_SystemReset();
        }
    }

    /* Case 2: APP confirmed alive */
    if (state == UPGRADE_STATE_CONFIRMED)
    {
        uint32_t zero = 0;
        W25QXX_Erase_Sector(CONFIG_AREA_ADDR);
        W25QXX_Write_NoCheck((uint8_t*)&zero, UPGRADE_STATE_ADDR, 4);
        W25QXX_Write_NoCheck((uint8_t*)&zero, UPGRADE_FLAG_ADDR, 4);
        W25QXX_Write_NoCheck((uint8_t*)&zero, FIRMWARE_SIZE_ADDR, 4);
    }
    
    /* Case 3: Upgrade requested */
    if (flag == UPGRADE_FLAG_YES && size > 0 && size < APP_SIZE)
    {
        printf("[BL] Upgrade %dB\r\n", size);

        IWDG_Init();
        
        uint32_t burning = UPGRADE_STATE_BURNING;
        W25QXX_Erase_Sector(CONFIG_AREA_ADDR);
        W25QXX_Write_NoCheck((uint8_t*)&flag, UPGRADE_FLAG_ADDR, 4);
        W25QXX_Write_NoCheck((uint8_t*)&burning, UPGRADE_STATE_ADDR, 4);
        W25QXX_Write_NoCheck((uint8_t*)&size, FIRMWARE_SIZE_ADDR, 4);
        IWDG_Feed();
        
        BackupAppToSlotB();
        IWDG_Feed();
        
        if (VerifySlotBBackup() != 0)
        {
            printf("[BL] ERR: backup verify\r\n");
            uint32_t zero = 0;
            W25QXX_Erase_Sector(CONFIG_AREA_ADDR);
            W25QXX_Write_NoCheck((uint8_t*)&zero, UPGRADE_STATE_ADDR, 4);
            W25QXX_Write_NoCheck((uint8_t*)&zero, UPGRADE_FLAG_ADDR, 4);
            W25QXX_Write_NoCheck((uint8_t*)&zero, FIRMWARE_SIZE_ADDR, 4);
            Delay_ms(500);
            NVIC_SystemReset();
        }
        
        if (CopyFirmwareFromSlot(FIRMWARE_SLOT_A_ADDR, size) != 0)
        {
            printf("[BL] ERR: copy, rollback\r\n");
            RollbackFirmware();
            Delay_ms(500);
            NVIC_SystemReset();
        }
        IWDG_Feed();
        
        uint32_t done = UPGRADE_STATE_DONE;
        uint32_t no_flag = UPGRADE_FLAG_NO;
        W25QXX_Erase_Sector(CONFIG_AREA_ADDR);
        W25QXX_Write_NoCheck((uint8_t*)&no_flag, UPGRADE_FLAG_ADDR, 4);
        W25QXX_Write_NoCheck((uint8_t*)&done, UPGRADE_STATE_ADDR, 4);
        W25QXX_Write_NoCheck((uint8_t*)&size, FIRMWARE_SIZE_ADDR, 4);

        printf("[BL] Done, reboot\r\n");
        for(int i=0; i<1000000; i++);
        NVIC_SystemReset();
    }
    
    /* Case 4: Normal jump */
    JumpToApp(APP_ADDR);
    
    printf("[BL] ERR: jump fail\r\n");
    Delay_ms(500);
    NVIC_SystemReset();
    while(1);
}
