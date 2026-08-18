/**
 * @file firmware_check.c
 * @brief 固件校验实现（纯算法，零硬件依赖）
 */

#include "firmware_check.h"
#include "upgrade_config.h"
#include "ff.h"
#include <stdio.h>
#include <stdint.h>

uint8_t Firmware_CheckHeader(const uint8_t *header)
{
    uint32_t msp = *(uint32_t *)header;
    /* MSP 必须在 RAM 范围内 */
    return ((msp & 0x2FFE0000) == 0x20000000) ? FIRMWARE_OK : FIRMWARE_ERR;
}

uint32_t Firmware_CRC32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else         crc >>= 1;
        }
    }
    return ~crc;
}

uint8_t Firmware_Check(const char *filename, uint32_t size)
{
    FIL file;
    UINT br;
    uint8_t header[4];

    if (size <= 256 || size > FIRMWARE_MAX_SIZE) {
        return FIRMWARE_ERR;
    }

    if (f_open(&file, filename, FA_READ) != FR_OK) {
        return FIRMWARE_ERR;
    }

    if (f_read(&file, header, 4, &br) != FR_OK || br != 4) {
        f_close(&file);
        return FIRMWARE_ERR;
    }

    f_close(&file);

    return Firmware_CheckHeader(header);
}
