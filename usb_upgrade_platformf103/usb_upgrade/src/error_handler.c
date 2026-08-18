/**
 * @file error_handler.c
 * @brief 错误处理模块实现
 */

#include "error_handler.h"
#include "config.h"

static uint32_t   err_count = 0;
static ErrorCode_t err_last = ERR_NONE;
static uint32_t   err_timestamp = 0;

void Error_Handler_Record(ErrorCode_t code)
{
    err_count++;
    err_last = code;
    err_timestamp = HAL_Tick->get_tick();
    DBG_PRINTF("[ERR] code=%d, count=%d\r\n", (int)code, (int)err_count);
}

uint32_t Error_Handler_GetCount(void)
{
    return err_count;
}

ErrorCode_t Error_Handler_GetLast(void)
{
    return err_last;
}

uint32_t Error_Handler_GetTimestamp(void)
{
    return err_timestamp;
}
