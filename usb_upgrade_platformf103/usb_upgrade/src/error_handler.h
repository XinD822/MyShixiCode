/**
 * @file error_handler.h
 * @brief 错误处理模块
 */

#ifndef __ERROR_HANDLER_H
#define __ERROR_HANDLER_H

#include <stdint.h>

typedef enum {
    ERR_NONE = 0,
    ERR_OUT_OF_RANGE,
    ERR_MUTEX_TIMEOUT,
    ERR_FLASH_WRITE,
    ERR_FLASH_READ,
    ERR_UPGRADE_FAIL,
} ErrorCode_t;

void    Error_Handler_Record(ErrorCode_t code);
uint32_t Error_Handler_GetCount(void);
ErrorCode_t Error_Handler_GetLast(void);
uint32_t Error_Handler_GetTimestamp(void);

#endif /* __ERROR_HANDLER_H */
