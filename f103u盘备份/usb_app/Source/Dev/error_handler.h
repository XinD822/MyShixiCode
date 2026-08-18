/**
 * @file error_handler.h
 * @brief 错误处理模块头文件
 * 
 * 功能说明：
 *   记录和管理系统运行时的错误
 *   支持错误计数、时间戳记录、错误查询
 * 
 * 使用场景：
 *   - USB通信超时
 *   - FatFS挂载失败
 *   - 固件校验失败
 *   - Flash读写错误
 *   - 互斥锁超时
 */

#ifndef __ERROR_HANDLER_H__
#define __ERROR_HANDLER_H__

#include "stm32f10x.h"

/**
 * @brief 错误类型枚举
 */
typedef enum {
    ERR_NONE = 0,           /* 无错误 */
    ERR_USB_TIMEOUT,        /* USB通信超时 */
    ERR_FATFS_MOUNT,        /* FatFS挂载失败 */
    ERR_FATFS_UNMOUNT,      /* FatFS卸载失败 */
    ERR_FIRMWARE_CHECK,     /* 固件校验失败 */
    ERR_FIRMWARE_READ,      /* 固件读取失败 */
    ERR_FLASH_WRITE,        /* Flash写入失败 */
    ERR_FLASH_ERASE,        /* Flash擦除失败 */
    ERR_MUTEX_TIMEOUT,      /* 互斥锁获取超时 */
    ERR_OUT_OF_RANGE,       /* 地址越界 */
    ERR_MAX                 /* 错误类型数量 */
} ErrorType_t;

/**
 * @brief 错误信息结构
 */
typedef struct {
    ErrorType_t type;       /* 错误类型 */
    uint32_t count;         /* 错误计数 */
    uint32_t last_time;     /* 最后发生时间（系统tick） */
} ErrorInfo_t;

/**
 * @brief 初始化错误处理器
 * 
 * 清零所有错误计数和时间戳
 */
void Error_Handler_Init(void);

/**
 * @brief 记录错误
 * 
 * 增加错误计数，记录时间戳，打印错误信息
 * 
 * @param error 错误类型
 */
void Error_Handler_Record(ErrorType_t error);

/**
 * @brief 获取错误计数
 * 
 * @param error 错误类型
 * @return 错误发生次数
 */
uint32_t Error_Handler_GetCount(ErrorType_t error);

/**
 * @brief 重置所有错误计数
 */
void Error_Handler_Reset(void);

/**
 * @brief 重置指定错误计数
 * 
 * @param error 错误类型
 */
void Error_Handler_ResetSingle(ErrorType_t error);

/**
 * @brief 检查是否有错误
 * 
 * @return 1=有错误，0=无错误
 */
uint8_t Error_Handler_HasError(void);

/**
 * @brief 获取最后发生的错误
 * 
 * @return 最后发生的错误类型
 */
ErrorType_t Error_Handler_GetLastError(void);

/**
 * @brief 打印错误报告
 * 
 * 输出所有非零错误的计数和时间戳
 */
void Error_Handler_Print(void);

#endif /* __ERROR_HANDLER_H__ */
