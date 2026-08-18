/**
 * @file firmware_check.c
 * @brief 固件校验模块实现
 * 
 * 功能说明：
 *   1. Firmware_CheckHeader() - 检查STM32 bin文件头MSP是否合法
 *   2. Firmware_CRC32() - 计算数据的CRC32校验值
 *   3. Firmware_Check() - 完整的固件文件校验流程
 * 
 * 校验流程：
 *   打开文件 → 检查大小 → 读取头部 → 校验MSP → 计算CRC32 → 关闭文件
 */

#include "firmware_check.h"
#include "upgrade_config.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 检查STM32 bin文件头
 * 
 * STM32中断向量表格式：
 *   +0x00: MSP（主堆栈指针初始值）
 *   +0x04: Reset_Handler（复位向量）
 * 
 * MSP校验逻辑：
 *   STM32F103的SRAM起始地址是0x20000000
 *   MSP必须指向SRAM区域，所以高16位应该是0x2000
 * 
 * @param header 文件头数据（4字节，小端序）
 * @return 1: 有效，0: 无效
 */
uint8_t Firmware_CheckHeader(const uint8_t *header)
{
    uint32_t msp_value;

    /* 将4字节小端序数据转换为32位整数 */
    msp_value = (uint32_t)header[0] |
                ((uint32_t)header[1] << 8) |
                ((uint32_t)header[2] << 16) |
                ((uint32_t)header[3] << 24);

    /* 检查MSP是否在SRAM区域（0x20000000 - 0x2000FFFF） */
    /* 使用掩码0xFFFF0000检查高16位 */
    if ((msp_value & 0xFFFF0000) == 0x20000000) {
        return 1;  // 有效
    }

    return 0;  // 无效
}

/**
 * @brief 计算CRC32
 * 
 * CRC32算法：使用标准多项式0xEDB88320
 * 这是Ethernet/ZIP/PNG等标准使用的CRC32算法
 * 
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return CRC32值
 */
uint32_t Firmware_CRC32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;  // 初始值
    uint32_t i, j;

    for (i = 0; i < length; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;  // 多项式
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;  // 最终异或
}

/**
 * @brief CRC32增量更新（用于分块计算）
 * 
 * 当数据量太大无法一次读入内存时，可以分块计算
 * 每次调用时传入上一次的CRC值和新的数据块
 * 
 * @param crc 上一次的CRC值
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return 更新后的CRC值
 */
static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t length)
{
    uint32_t i, j;
    for (i = 0; i < length; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return crc;
}

/**
 * @brief 校验固件文件
 * 
 * 校验流程：
 *   1. 打开文件
 *   2. 检查文件大小（0 ~ FIRMWARE_MAX_SIZE）
 *   3. 读取并校验头部MSP
 *   4. 计算完整文件的CRC32
 *   5. 关闭文件并返回结果
 * 
 * @param filename 固件文件路径，例如 "0:/firmware.bin"
 * @param expected_size 期望的文件大小（0表示不检查大小）
 * @return FIRMWARE_OK: 校验通过，其他: 错误码
 */
uint8_t Firmware_Check(const char *filename, uint32_t expected_size)
{
    FIL file;
    FRESULT fres;
    UINT br;
    uint8_t header[4] = {0};
    uint32_t file_size = 0;
    uint8_t result = FIRMWARE_OK;
    uint32_t crc = 0xFFFFFFFF;
    uint8_t buf[512];

    /* 第一步：打开文件 */
    if (f_open(&file, filename, FA_READ) != FR_OK) {
        return FIRMWARE_ERR_OPEN;
    }

    /* 第二步：获取文件大小 */
    file_size = f_size(&file);

    /* 第三步：检查文件大小 */
    if (file_size == 0 || file_size > FIRMWARE_MAX_SIZE) {
        result = FIRMWARE_ERR_SIZE;
        goto exit;
    }

    /* 第四步：读取并校验头部MSP */
    if (f_read(&file, header, 4, &br) != FR_OK || br != 4) {
        result = FIRMWARE_ERR_HEADER;
        goto exit;
    }

    if (!Firmware_CheckHeader(header)) {
        result = FIRMWARE_ERR_HEADER;
        goto exit;
    }

    /* 第五步：回到文件头，计算完整CRC32 */
    f_lseek(&file, 0);
    while (1) {
        fres = f_read(&file, buf, sizeof(buf), &br);
        if (fres != FR_OK || br == 0) break;
        crc = crc32_update(crc, buf, br);
    }

    crc ^= 0xFFFFFFFF;  // CRC32最终异或
    printf("[FW] CRC32 = 0x%08X, size = %d\r\n", crc, file_size);

exit:
    f_close(&file);
    return result;
}
