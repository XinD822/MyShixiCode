/**
 * @file fatfs_system.c
 * @brief FatFS文件系统封装层实现
 * 
 * 功能说明：
 *   对FatFS库的上层封装，提供易用的文件操作接口
 *   包含互斥锁保护，防止USB和FatFS同时访问SPI Flash
 * 
 * 互斥锁使用场景：
 *   USB MSC和FatFS都需要访问W25Q128
 *   如果同时访问会导致数据损坏
 *   使用互斥锁确保同一时间只有一个模块访问
 */

#include "FatFs_system.h"
#include "mutex.h"
#include "error_handler.h"

/* ──── 全局变量 ──── */
FATFS sFLASH_FatFs;     // FatFS文件系统对象
FIL file;               // 文件对象（全局复用）
BYTE work[FF_MAX_SS];   // 格式化工作缓冲区

/* FatFS访问互斥锁 */
static Mutex_t fatfs_mutex;

/**
 * @brief 获取磁盘剩余空间
 * 
 * 计算公式：
 *   总扇区数 = (最大簇数 - 2) * 每簇扇区数
 *   剩余扇区数 = 空闲簇数 * 每簇扇区数
 *   转换为KB：扇区数 / 2（每扇区512字节）
 * 
 * @param drv 驱动器路径
 * @param total 输出：总空间（KB）
 * @param free 输出：剩余空间（KB）
 * @return FRESULT 错误码
 */
FRESULT exf_getfree(char *drv, u32 *total, u32 *free)
{
	FATFS *fs1;
	FRESULT res;
	u32 fre_clust = 0, fre_sect = 0, tot_sect = 0;

	res = f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
	if (res == 0)
	{
		tot_sect = (fs1->n_fatent - 2) * fs1->csize;
		fre_sect = fre_clust * fs1->csize;
#if FF_MAX_SS != 512
		tot_sect *= fs1->ssize / 512;
		fre_sect *= fs1->ssize / 512;
#endif
		*total = tot_sect >> 1;  // 转换为KB
		*free = fre_sect >> 1;
	}

	return res;
}

/**
 * @brief 格式化磁盘
 * 
 * 注意：会擦除所有数据！
 * 使用场景：首次使用或文件系统损坏时
 */
void FatFs_format(void)
{
	FRESULT MyFile_Res;

	MyFile_Res = f_mkfs("0:", 0, work, sizeof(work));
	if (MyFile_Res != FR_OK)
	{
		printf("[FatFS] ERR: format %d\r\n", MyFile_Res);
	}
}

/**
 * @brief 挂载文件系统
 * 
 * 执行流程：
 *   1. 初始化互斥锁（首次调用时）
 *   2. 获取互斥锁（防止并发访问）
 *   3. 挂载文件系统
 *   4. 检查文件系统完整性
 *   5. 释放互斥锁
 * 
 * @return FRESULT 错误码
 */
FRESULT FatFs_Mount(void)
{
	FRESULT MyFile_Res;

	/* 首次调用时初始化互斥锁 */
	static uint8_t mutex_initialized = 0;
	if (!mutex_initialized) {
		Mutex_Init(&fatfs_mutex);
		mutex_initialized = 1;
	}

	/* 获取互斥锁（1秒超时） */
	if (!Mutex_Lock(&fatfs_mutex, 1000)) {
		Error_Handler_Record(ERR_MUTEX_TIMEOUT);
		return FR_TIMEOUT;
	}

	/* 挂载文件系统 */
	MyFile_Res = f_mount(&sFLASH_FatFs, "0:", 1);

	if (MyFile_Res == FR_OK)
		printf("[FatFS] Mount OK\r\n");
	else
		printf("[FatFS] ERR: mount %d\r\n", MyFile_Res);

	/* 释放互斥锁 */
	Mutex_Unlock(&fatfs_mutex);
	return MyFile_Res;
}

/**
 * @brief 创建目录
 * 
 * @param path 目录路径
 * @return FRESULT 错误码
 */
FRESULT FatFs_mkdir_wrapper(const char *path)
{
	return f_mkdir(path);
}

/**
 * @brief 删除文件或目录
 * 
 * @param path 文件/目录路径
 * @return FRESULT 错误码
 */
FRESULT FatFs_unlink_wrapper(const char *path)
{
	return f_unlink(path);
}

/**
 * @brief 写入文件
 * 
 * 如果文件不存在会创建，如果已存在会覆盖
 * 
 * @param path 文件路径
 * @param content 文件内容
 */
void FatFs_write_file(const char *path, const char *content)
{
	FRESULT res = f_open(&file, path, FA_CREATE_NEW | FA_WRITE);

	if (res == FR_OK)
	{
		UINT bw;
		f_write(&file, content, strlen(content), &bw);
		f_close(&file);
	}
}

/**
 * @brief 读取文件内容
 * 
 * @param path 文件路径
 */
void FatFs_read_file(const char *path)
{
	char buffer[128];

	FRESULT res = f_open(&file, path, FA_READ);

	if (res == FR_OK)
	{
		UINT br;
		f_read(&file, buffer, sizeof(buffer), &br);
		buffer[br] = '\0';
		f_close(&file);
	}
}

/**
 * @brief 列出目录内容
 * 
 * @param path 目录路径
 */
void FatFs_list_dir(const char *path)
{
	DIR dir;
	FILINFO fno;

	if (f_opendir(&dir, path) == FR_OK)
	{
		while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0])
		{
			printf("  %s\r\n", fno.fname);
		}
		f_closedir(&dir);
	}
}

/**
 * @brief FatFS功能测试
 * 
 * 测试流程：
 *   1. 创建目录
 *   2. 写入文件
 *   3. 读取文件
 * 
 * @param wrapperpath 目录路径
 * @param filepath 文件路径
 * @param content 文件内容
 */
void FatFs_Test(const char *wrapperpath, const char *filepath, const char *content)
{
	FatFs_mkdir_wrapper(wrapperpath);
	FatFs_write_file(filepath, content);
	FatFs_read_file(filepath);
}


