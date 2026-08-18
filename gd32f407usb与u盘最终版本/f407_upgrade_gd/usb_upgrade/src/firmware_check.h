/**
 * @file firmware_check.h
 * @brief 固件校验接口
 */

#ifndef __FIRMWARE_CHECK_H
#define __FIRMWARE_CHECK_H

#include <stdint.h>

#define FIRMWARE_OK     0
#define FIRMWARE_ERR    1

uint8_t Firmware_CheckHeader(const uint8_t *header);
uint32_t Firmware_CRC32(const uint8_t *data, uint32_t len);
uint8_t Firmware_Check(const char *filename, uint32_t size);

#endif /* __FIRMWARE_CHECK_H */
