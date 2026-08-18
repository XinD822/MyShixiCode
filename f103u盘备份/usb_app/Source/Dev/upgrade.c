/**
 * @file upgrade.c
 * @brief 升级模块实现 - USB拖拽升级的核心逻辑
 * 
 * 功能说明：
 *   1. 检测U盘中是否存在firmware.bin文件
 *   2. 验证固件文件的合法性（大小、格式）
 *   3. 将固件从FatFS数据区写入W25Q128的Slot A
 *   4. 设置升级标志，触发Bootloader完成最终烧写
 * 
 * 升级流程：
 *   用户在PC上把firmware.bin拖入U盘
 *     ↓
 *   APP检测到新固件文件
 *     ↓
 *   从FatFS读取文件 → 写入W25Q128的Slot A
 *     ↓
 *   设置flag=0x12345678, size=文件大小
 *     ↓
 *   跳转到Bootloader，由Bootloader完成最终烧写
 */

#include "upgrade.h"
#include "firmware_check.h"
#include "spi_flash.h"
#include "Delay.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>

/* ──── 全局变量 ──── */
static UpgradeState g_upgradeState = UPGRADE_IDLE;  // 升级状态机
static UpgradeResult g_lastError = UPGRADE_OK;       // 最后的错误码

static uint8_t firmware_buf[FIRMWARE_BUF_SIZE];      // 固件读取缓冲区（4KB）

static char g_binFilename[FIRMWARE_FILENAME_MAX] = {0};  // 固件文件名
static uint32_t g_binFileSize = 0;                       // 固件文件大小

/* 固定目标固件文件名 */
static const char *FIRMWARE_TARGET_NAME = "firmware.bin";

/**
 * @brief 不区分大小写的字符串比较
 * 
 * @param s1 字符串1
 * @param s2 字符串2
 * @return 0=相等，非0=不等
 */
static int strcasecmp(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
        /* 转换为小写后比较 */
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? (*s1 + 32) : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? (*s2 + 32) : *s2;
        if (c1 != c2) return c1 - c2;
        s1++; s2++;
    }
    return *s1 - *s2;
}

/**
 * @brief 检查固件文件是否存在
 * 
 * 扫描U盘根目录，查找firmware.bin文件
 * 验证文件大小和可打开性
 * 
 * @return 1=找到有效固件，0=未找到
 */
uint8_t Check_Firmware_Exists(void)
{
    DIR dir;
    FILINFO fno;
    FIL file;
    FRESULT res;

    /* 打开根目录 */
    if (f_opendir(&dir, "0:/") != FR_OK) {
        printf("[UPG] f_opendir failed\r\n");
        return 0;
    }

    printf("[UPG] Scanning root dir...\r\n");
    
    /* 遍历目录中的所有文件 */
    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
        /* 跳过子目录 */
        if (fno.fattrib & AM_DIR) continue;

        printf("[UPG]  fname='%s', size=%d\r\n", fno.fname, fno.fsize);

        /* 检查文件名是否匹配（不区分大小写） */
        if (strcasecmp(fno.fname, FIRMWARE_TARGET_NAME) == 0) {
            /* 检查文件大小是否合法 */
            if (fno.fsize <= 256 || fno.fsize > FIRMWARE_MAX_SIZE) {
                printf("[UPG] Invalid file size: %d bytes\r\n", fno.fsize);
                continue;
            }

            /* 尝试打开文件（最多重试3次） */
            uint8_t open_success = 0;
            for (int i = 0; i < 3; i++) {
                res = f_open(&file, fno.fname, FA_READ);
                if (res == FR_OK) {
                    open_success = 1;
                    f_close(&file);
                    break;
                }
                Delay_ms(100);
            }

            if (open_success) {
                /* 找到有效固件，保存文件名和大小 */
                strcpy(g_binFilename, fno.fname);
                g_binFileSize = fno.fsize;
                f_closedir(&dir);
                printf("[UPG] Valid firmware found, size: %d bytes\r\n", g_binFileSize);
                return 1;
            } else {
                printf("[UPG] Failed to open firmware file\r\n");
            }
        }
    }

    printf("[UPG] No matching firmware file found\r\n");
    f_closedir(&dir);
    return 0;
}

/**
 * @brief 初始化升级模块
 */
void Upgrade_Init(void)
{
    g_upgradeState = UPGRADE_IDLE;
    g_lastError = UPGRADE_OK;
    memset(g_binFilename, 0, sizeof(g_binFilename));
    g_binFileSize = 0;
}

/**
 * @brief 扫描目录查找firmware.bin
 * 
 * @param path 目录路径
 * @param bin_filename 输出：找到的文件名
 * @param file_size 输出：文件大小
 * @return FR_OK=找到，其他=未找到
 */
static FRESULT ScanBinFile(const char *path, char *bin_filename, uint32_t *file_size)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;

    res = f_opendir(&dir, path);
    if (res != FR_OK) {
        return res;
    }

    /* 遍历目录 */
    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
        if (strcasecmp(fno.fname, FIRMWARE_TARGET_NAME) == 0) {
            strcpy(bin_filename, fno.fname);
            *file_size = fno.fsize;
            f_closedir(&dir);
            return FR_OK;
        }
    }

    f_closedir(&dir);
    return FR_NO_FILE;
}

/**
 * @brief 检查固件是否合法
 * 
 * 检查内容：
 *   1. 扫描目录查找firmware.bin
 *   2. 验证固件文件格式和大小
 * 
 * @return UPGRADE_OK=合法，其他=不合法
 */
UpgradeResult Upgrade_Check(void)
{
    FRESULT res;
    uint8_t check_result;

    g_upgradeState = UPGRADE_CHECKING;
    g_lastError = UPGRADE_OK;

    /* 扫描根目录查找firmware.bin */
    res = ScanBinFile("0:/", g_binFilename, &g_binFileSize);
    if (res != FR_OK) {
        g_upgradeState = UPGRADE_IDLE;
        g_lastError = UPGRADE_ERR_NO_FILE;
        return UPGRADE_ERR_NO_FILE;
    }

    /* 验证固件文件 */
    check_result = Firmware_Check(g_binFilename, g_binFileSize);
    if (check_result != FIRMWARE_OK) {
        printf("[UPG] File check fail\r\n");
        g_upgradeState = UPGRADE_IDLE;
        g_lastError = UPGRADE_ERR_CHECK;
        return UPGRADE_ERR_CHECK;
    }

    g_upgradeState = UPGRADE_READY;
    return UPGRADE_OK;
}

/**
 * @brief 执行升级 - 分块读取并写入Flash
 * 
 * 操作流程：
 *   1. 打开固件文件
 *   2. 擦除Slot A（按64KB块擦除）
 *   3. 分块读取文件内容（每次4KB）
 *   4. 写入W25Q128 Slot A
 *   5. 回读校验头部MSP
 * 
 * @return UPGRADE_OK=成功，其他=失败
 */
UpgradeResult Upgrade_Execute(void)
{
    FRESULT res;
    FIL file;
    UINT br;
    uint32_t total_read = 0;
    uint32_t flash_addr = FIRMWARE_SLOT_A_ADDR;

    /* 检查状态 */
    if (g_upgradeState != UPGRADE_READY) {
        g_lastError = UPGRADE_ERR_STATE;
        return UPGRADE_ERR_STATE;
    }

    g_upgradeState = UPGRADE_BURNING;
    printf("[UPG] %s %dB\r\n", g_binFilename, g_binFileSize);

    /* 打开固件文件 */
    res = f_open(&file, g_binFilename, FA_READ);
    if (res != FR_OK) {
        printf("[UPG] Open fail\r\n");
        g_upgradeState = UPGRADE_IDLE;
        g_lastError = UPGRADE_ERR_BURN;
        return UPGRADE_ERR_BURN;
    }

    /* 擦除Slot A（按64KB块擦除） */
    printf("[UPG] Erase...\r\n");
    uint32_t blocks_needed = (g_binFileSize + 65535) / 65536;  // 向上取整
    if (blocks_needed == 0) blocks_needed = 1;
    for (uint32_t i = 0; i < blocks_needed; i++) {
        W25QXX_Erase_Block(FIRMWARE_SLOT_A_ADDR + i * 65536);
    }

    /* 分块读取文件并写入Flash */
    printf("[UPG] Write...\r\n");
    while (1) {
        /* 从文件读取4KB到缓冲区 */
        res = f_read(&file, firmware_buf, FIRMWARE_BUF_SIZE, &br);
        if (res != FR_OK || br == 0) break;

        /* 写入W25Q128 */
        W25QXX_Write_NoCheck(firmware_buf, flash_addr, br);
        flash_addr += br;
        total_read += br;

        /* 如果读取的数据不足4KB，说明是最后一块 */
        if (br < FIRMWARE_BUF_SIZE) break;
    }
    f_close(&file);

    /* 文件大小安全检查 */
    if (total_read < 256) {
        printf("[UPG] WARNING: File size only %d bytes! Normal firmware > 1KB.\r\n", total_read);
    }

    /* 回读校验：验证文件头部MSP是否写入正确 */
    printf("[UPG] Verify...\r\n");
    {
        uint8_t verify_header[4];
        W25QXX_Read(verify_header, FIRMWARE_SLOT_A_ADDR, 4);
        if (!Firmware_CheckHeader(verify_header)) {
            printf("[UPG] Verify FAIL! Header mismatch\r\n");
            g_upgradeState = UPGRADE_ERROR;
            g_lastError = UPGRADE_ERR_BURN;
            return UPGRADE_ERR_BURN;
        }
        printf("[UPG] Verify OK\r\n");
    }

    g_upgradeState = UPGRADE_DONE;
    g_binFileSize = total_read;
    printf("[UPG] %dB written to Slot A\r\n", total_read);
    return UPGRADE_OK;
}

/**
 * @brief 设置升级请求标志和固件大小
 * 
 * 先擦除配置区扇区，再写入标志和大小
 * 避免W25QXX_Write的read-modify-write破坏其他数据
 * 
 * @param flag 升级标志（UPGRADE_FLAG_YES或UPGRADE_FLAG_NO）
 * @param size 固件大小（字节）
 */
void Upgrade_SetFlag(uint32_t flag, uint32_t size)
{
    W25QXX_Erase_Sector(CONFIG_AREA_ADDR);
    W25QXX_Write_NoCheck((uint8_t*)&flag, UPGRADE_FLAG_ADDR, sizeof(flag));
    W25QXX_Write_NoCheck((uint8_t*)&size, FIRMWARE_SIZE_ADDR, sizeof(size));
    printf("[Upgrade] Set flag: 0x%08X, size: %d\r\n", flag, size);
}

/**
 * @brief 读取升级标志
 * 
 * @return 升级标志值
 */
uint32_t Upgrade_GetFlag(void)
{
    uint32_t flag = 0;
    W25QXX_Read((uint8_t*)&flag, UPGRADE_FLAG_ADDR, sizeof(flag));
    return flag;
}

/**
 * @brief 跳转到Bootloader执行升级
 * 
 * 操作流程：
 *   1. 设置升级请求标志和固件大小
 *   2. 关闭中断
 *   3. 跳转到Bootloader
 */
void Upgrade_JumpToBootloader(void)
{
    printf("[Upgrade] Jump to bootloader (0x%06X)\r\n", BOOTLOADER_ADDR);
    
    /* 设置升级请求标志 */
    Upgrade_SetFlag(UPGRADE_FLAG_YES, g_binFileSize);
    
    /* 关闭中断 */
    __disable_irq();
    
    /* 跳转到Bootloader */
    Upgrade_JumpToApp(BOOTLOADER_ADDR);
}

/**
 * @brief 跳转到指定地址执行
 * 
 * 跳转前必须完成的操作：
 *   1. 禁用USB中断
 *   2. 清除挂起的USB中断
 *   3. 复位USB外设并关闭时钟
 *   4. 关闭全局中断
 *   5. 设置栈指针并跳转
 * 
 * @param app_addr 目标地址
 */
void Upgrade_JumpToApp(uint32_t app_addr)
{
    typedef void (*pFunction)(void);
    pFunction Jump_To_Application;
    uint32_t JumpAddress;
    uint32_t app_stack;

    printf("[Upgrade] Jump to: 0x%08X\r\n", app_addr);

    /* 读取目标地址的MSP初始值 */
    app_stack = *(__IO uint32_t*) app_addr;
    printf("[Upgrade] MSP: 0x%08X\r\n", app_stack);

    /* 校验MSP是否在RAM范围内 */
    if ((app_stack & 0x2FFE0000) == 0x20000000) {
        printf("[Upgrade] Address valid, jumping...\r\n");

        /* 1. 禁用USB中断 */
        NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
        NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);
        NVIC_DisableIRQ(USBWakeUp_IRQn);

        /* 2. 清除挂起的USB中断 */
        NVIC_ClearPendingIRQ(USB_LP_CAN1_RX0_IRQn);
        NVIC_ClearPendingIRQ(USB_HP_CAN1_TX_IRQn);
        NVIC_ClearPendingIRQ(USBWakeUp_IRQn);

        /* 3. 复位USB外设并关闭时钟 */
        RCC_APB1PeriphResetCmd(RCC_APB1Periph_USB, ENABLE);   // 复位
        RCC_APB1PeriphResetCmd(RCC_APB1Periph_USB, DISABLE);  // 释放复位
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE);  // 关闭时钟

        /* 4. 关闭全局中断并跳转 */
        __disable_irq();
        __set_MSP(app_stack);
        JumpAddress = *(__IO uint32_t*) (app_addr + 4);
        Jump_To_Application = (pFunction) JumpAddress;
        Jump_To_Application();
    } else {
        printf("[Upgrade] Jump fail: invalid address\r\n");
    }
}

/**
 * @brief 获取当前升级状态
 * 
 * @return 升级状态枚举值
 */
UpgradeState Upgrade_GetState(void)
{
    return g_upgradeState;
}

/**
 * @brief 获取最后的错误码
 * 
 * @return 错误码枚举值
 */
UpgradeResult Upgrade_GetLastError(void)
{
    return g_lastError;
}
