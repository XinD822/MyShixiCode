/**
 * @file fatfs_system.h
 * @brief FatFS文件系统封装层头文件
 * 
 * 功能说明：
 *   对FatFS库的上层封装，提供易用的文件操作接口
 *   包含互斥锁保护，防止USB和FatFS同时访问SPI Flash
 * 
 * 使用场景：
 *   - 挂载/卸载文件系统
 *   - 创建/删除目录
 *   - 读写文件
 *   - 获取磁盘空间信息
 */

#ifndef __FATFS_SYSTEM_H
#define __FATFS_SYSTEM_H

#include "config.h"

/**
 * @brief 获取磁盘剩余空间
 * 
 * @param drv 驱动器路径，例如 "0:"
 * @param total 输出：总空间（KB）
 * @param free 输出：剩余空间（KB）
 * @return FRESULT 错误码
 */
FRESULT exf_getfree(char *drv, u32 *total, u32 *free);

/**
 * @brief 格式化磁盘
 * 
 * 注意：会擦除所有数据！
 */
void FatFs_format(void);

/**
 * @brief 挂载文件系统
 * 
 * 包含互斥锁保护，防止并发访问
 * 
 * @return FRESULT 错误码
 */
FRESULT FatFs_Mount(void);

/**
 * @brief 创建目录
 * 
 * @param path 目录路径
 * @return FRESULT 错误码
 */
FRESULT FatFs_mkdir_wrapper(const char *path);

/**
 * @brief 删除文件或目录
 * 
 * @param path 文件/目录路径
 * @return FRESULT 错误码
 */
FRESULT FatFs_unlink_wrapper(const char *path);

/**
 * @brief 写入文件
 * 
 * 如果文件不存在会创建，如果已存在会覆盖
 * 
 * @param path 文件路径
 * @param content 文件内容
 */
void FatFs_write_file(const char *path, const char *content);

/**
 * @brief 读取文件内容
 * 
 * @param path 文件路径
 */
void FatFs_read_file(const char *path);

/**
 * @brief 列出目录内容
 * 
 * @param path 目录路径
 */
void FatFs_list_dir(const char *path);

/**
 * @brief FatFS功能测试
 * 
 * 创建目录 → 写入文件 → 读取文件
 * 
 * @param wrapperpath 目录路径
 * @param filepath 文件路径
 * @param content 文件内容
 */
void FatFs_Test(const char * wrapperpath, const char * filepath, const char *content);

#endif /* __FATFS_SYSTEM_H */
