/**
 * @file error_handler.c
 * @brief 错误处理模块实现
 * 
 * 功能说明：
 *   记录和管理系统运行时的错误
 *   每种错误类型独立计数，记录最后发生时间
 * 
 * 使用示例：
 *   Error_Handler_Record(ERR_MUTEX_TIMEOUT);
 *   if (Error_Handler_GetCount(ERR_MUTEX_TIMEOUT) > 10) {
 *       // 连续超时，可能需要重启
 *   }
 */

#include "error_handler.h"
#include "usart.h"
#include "Delay.h"
#include <stdio.h>

/* ──── 全局变量 ──── */
static ErrorInfo_t error_info[ERR_MAX];  /* 错误信息数组 */
static ErrorType_t last_error = ERR_NONE; /* 最后发生的错误 */

/**
 * @brief 初始化错误处理器
 * 
 * 清零所有错误计数和时间戳
 */
void Error_Handler_Init(void)
{
    uint32_t i;
    for (i = 0; i < ERR_MAX; i++) {
        error_info[i].type = (ErrorType_t)i;
        error_info[i].count = 0;
        error_info[i].last_time = 0;
    }
    last_error = ERR_NONE;
}

/**
 * @brief 记录错误
 * 
 * 操作：
 *   1. 增加错误计数
 *   2. 记录当前系统tick
 *   3. 更新最后错误
 *   4. 打印错误信息
 * 
 * @param error 错误类型
 */
void Error_Handler_Record(ErrorType_t error)
{
    if (error >= ERR_MAX) return;
    
    error_info[error].count++;
    error_info[error].last_time = Delay_GetTick();
    last_error = error;
    
    printf("[ERR] Type=%d, Count=%d, Time=%d\r\n", 
           error, error_info[error].count, error_info[error].last_time);
}

/**
 * @brief 获取错误计数
 * 
 * @param error 错误类型
 * @return 错误发生次数
 */
uint32_t Error_Handler_GetCount(ErrorType_t error)
{
    if (error >= ERR_MAX) return 0;
    return error_info[error].count;
}

/**
 * @brief 重置所有错误计数
 */
void Error_Handler_Reset(void)
{
    uint32_t i;
    for (i = 0; i < ERR_MAX; i++) {
        error_info[i].count = 0;
        error_info[i].last_time = 0;
    }
    last_error = ERR_NONE;
}

/**
 * @brief 重置指定错误计数
 * 
 * @param error 错误类型
 */
void Error_Handler_ResetSingle(ErrorType_t error)
{
    if (error >= ERR_MAX) return;
    error_info[error].count = 0;
    error_info[error].last_time = 0;
}

/**
 * @brief 检查是否有错误
 * 
 * @return 1=有错误，0=无错误
 */
uint8_t Error_Handler_HasError(void)
{
    uint32_t i;
    for (i = 0; i < ERR_MAX; i++) {
        if (error_info[i].count > 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief 获取最后发生的错误
 * 
 * @return 最后发生的错误类型
 */
ErrorType_t Error_Handler_GetLastError(void)
{
    return last_error;
}

/**
 * @brief 打印错误报告
 * 
 * 输出所有非零错误的计数和时间戳
 * 格式：[ERR] Error Report:
 *        Type X: count=Y, last_time=Z
 */
void Error_Handler_Print(void)
{
    uint32_t i;
    printf("[ERR] Error Report:\r\n");
    for (i = 0; i < ERR_MAX; i++) {
        if (error_info[i].count > 0) {
            printf("  Type %d: count=%d, last_time=%d\r\n",
                   i, error_info[i].count, error_info[i].last_time);
        }
    }
}
