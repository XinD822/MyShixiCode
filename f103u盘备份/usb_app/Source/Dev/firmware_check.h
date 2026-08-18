/**
 * @file firmware_check.h
 * @brief 固件校验模块头文件
 * 
 * 功能说明：
 *   - 文件大小检查
 *   - STM32 bin文件头检查（MSP校验）
 *   - CRC32校验（可选）
 * 
 * 校验原理：
 *   STM32 bin文件的前4字节是主堆栈指针（MSP）初始值
 *   对于STM32F103，MSP应该在0x20000000附近（SRAM起始地址）
 */

#ifndef __FIRMWARE_CHECK_H
#define __FIRMWARE_CHECK_H

#include "stm32f10x.h"

/* ──── 固件校验错误码 ──── */
#define FIRMWARE_OK             0   // 校验通过
#define FIRMWARE_ERR_OPEN       1   // 打开文件失败
#define FIRMWARE_ERR_SIZE       2   // 文件大小异常
#define FIRMWARE_ERR_HEADER     3   // 文件头异常（MSP不合法）
#define FIRMWARE_ERR_CRC        4   // CRC校验失败

/**
 * @brief 校验固件文件
 * 
 * 检查内容：
 *   1. 文件能否打开
 *   2. 文件大小是否在合法范围内
 *   3. 文件头MSP是否指向RAM区域
 * 
 * @param filename 固件文件路径，例如 "0:/firmware.bin"
 * @param expected_size 期望的文件大小（0表示不检查大小）
 * @return FIRMWARE_OK: 校验通过
 *         其他: 错误码
 */
uint8_t Firmware_Check(const char *filename, uint32_t expected_size);

/**
 * @brief 计算CRC32
 * 
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return CRC32值
 */
uint32_t Firmware_CRC32(const uint8_t *data, uint32_t length);

/**
 * @brief 检查STM32 bin文件头
 * 
 * STM32 bin文件前4字节是栈地址（MSP），应该在0x20000000附近
 * 
 * @param header 文件头数据（4字节）
 * @return 1: 有效  0: 无效
 */
uint8_t Firmware_CheckHeader(const uint8_t *header);

#endif /* __FIRMWARE_CHECK_H */
