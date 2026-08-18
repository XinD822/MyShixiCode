/**
 * @file diskio.c
 * @brief FatFS磁盘I/O底层驱动
 * 
 * 功能说明：
 *   实现FatFS库所需的底层磁盘操作接口
 *   将FatFS的读写请求转换为W25Q128 SPI Flash操作
 * 
 * 关键设计：
 *   1. 读操作：直接从W25Q128读取数据
 *   2. 写操作：4KB read-modify-write（因为W25Q128必须先擦后写）
 *   3. 地址映射：FatFS逻辑地址 → W25Q128物理地址
 * 
 * 地址映射：
 *   FatFS扇区0 → W25Q128地址 FATFS_BASE_ADDR (0xBE0000)
 *   FatFS扇区1 → W25Q128地址 FATFS_BASE_ADDR + 512
 *   ...
 */

#include "ff.h"            /* FatFS库头文件 */
#include "diskio.h"        /* 磁盘操作接口 */
#include "spi_flash.h"     /* W25Q128驱动 */
#include "upgrade_config.h" /* FATFS_BASE_ADDR */

/* ──── 驱动器编号定义 ──── */
#define DEV_W25Qxx		0	/* 外部SPI Flash */

/**
 * @brief 获取驱动器状态
 * 
 * 通过读取W25Q128的芯片ID判断是否在线
 * W25Q128的ID应该是0xEF17
 * 
 * @param pdrv 驱动器编号
 * @return 状态寄存器（STA_NOINIT表示未初始化）
 */
DSTATUS disk_status ( BYTE pdrv	)
{
	DSTATUS status = STA_NOINIT;
	
	switch (pdrv) 
	{
		case DEV_W25Qxx :
		{
			/* 读取芯片ID，验证是否为W25Q128 */
			if (0XEF17 == W25QXX_ReadID())
			{
				status &= ~STA_NOINIT;  /* 清除未初始化标志 */
			}
			else
			{
				status = STA_NOINIT;
			}
			break;
		}

		default:
			status = STA_NOINIT;
	}
	return status;
}

/**
 * @brief 初始化磁盘
 * 
 * 初始化W25Q128并验证是否在线
 * 
 * @param pdrv 驱动器编号
 * @return 状态寄存器
 */
DSTATUS disk_initialize ( BYTE pdrv )
{
	uint16_t i;
	DSTATUS status = STA_NOINIT;
 
	switch (pdrv)
	{
		case DEV_W25Qxx :
		{
			/* 初始化W25Q128 */
			W25QXX_Init();
			i = 500;
			while(--i);    /* 短暂延时等待稳定 */
			
			/* 获取Flash状态 */
			status = disk_status(DEV_W25Qxx);    
			break;
		}

		default:
			status = STA_NOINIT;
	}
	return status;
}

/**
 * @brief 读取扇区
 * 
 * 将FatFS逻辑扇区号转换为W25Q128物理地址
 * 逻辑扇区号 * 512 + FATFS_BASE_ADDR = 物理地址
 * 
 * @param pdrv 驱动器编号
 * @param buff 数据缓冲区
 * @param sector 起始扇区号（LBA）
 * @param count 扇区数量
 * @return 操作结果
 */
DRESULT disk_read (
	BYTE pdrv,		/* 驱动器编号 */
	BYTE *buff,		/* 数据缓冲区 */
	LBA_t sector,	/* 起始扇区号 */
	UINT count		/* 扇区数量 */
)
{
	DRESULT status = RES_PARERR;
	
	switch (pdrv) 
	{
		case DEV_W25Qxx :
		{
			/* 计算物理地址：FatFS数据区起始于FATFS_BASE_ADDR */
			uint32_t addr = FATFS_BASE_ADDR + (sector << 9);  /* sector * 512 */
			W25QXX_Read(buff, addr, count << 9);  /* count * 512 */
			status = RES_OK;
			break;
		}

		default:
			status = RES_PARERR;
	}
 
	return status;
}

/**
 * @brief 写入扇区
 * 
 * 由于W25Q128必须先擦后写，采用4KB read-modify-write策略：
 *   1. 读取整个4KB物理扇区到缓冲区
 *   2. 修改缓冲区中对应的部分
 *   3. 擦除整个4KB物理扇区
 *   4. 将缓冲区写回
 * 
 * @param pdrv 驱动器编号
 * @param buff 数据缓冲区
 * @param sector 起始扇区号
 * @param count 扇区数量
 * @return 操作结果
 */
#if FF_FS_READONLY == 0

DRESULT disk_write ( BYTE pdrv, const BYTE *buff,	LBA_t sector, UINT count)
{
	DRESULT status = RES_PARERR;
	
	if (!count)
	{
		return RES_PARERR;		
	}
 
	switch (pdrv)
	{
		case DEV_W25Qxx :
		{
			/* 计算物理地址范围 */
			uint32_t start_addr = FATFS_BASE_ADDR + (sector << 9);
			uint32_t total_len = count << 9;
			uint32_t end_addr = start_addr + total_len;
			uint32_t phys_start = start_addr & ~0xFFF;  /* 4KB对齐 */
			uint32_t phys_end = (end_addr + 4095) & ~0xFFF;
			static uint8_t sector_buf[4096];

			/* 遍历每个受影响的4KB物理扇区 */
			for (uint32_t s = phys_start; s < phys_end; s += 4096)
			{
				/* 计算当前物理扇区中需要写入的范围 */
				uint32_t wr_start = (s < start_addr) ? start_addr : s;
				uint32_t wr_end = (s + 4096 > end_addr) ? end_addr : (s + 4096);
				uint32_t len = wr_end - wr_start;
				uint32_t offset = wr_start - s;

				/* Read: 读取整个4KB物理扇区 */
				W25QXX_Read(sector_buf, s, 4096);
				
				/* Modify: 修改对应部分 */
				memcpy(sector_buf + offset, buff + (wr_start - start_addr), len);
				
				/* Erase: 擦除整个4KB物理扇区 */
				W25QXX_Erase_Sector(s);
				
				/* Write: 写回整个4KB */
				W25QXX_Write_NoCheck(sector_buf, s, 4096);
			}
			status = RES_OK;
			break;
		}

		default:
			status = RES_PARERR;
	}
 
	return status;
}

#endif

/**
 * @brief 磁盘控制命令
 * 
 * 处理FatFS的控制请求：
 *   GET_SECTOR_COUNT: 获取总扇区数
 *   GET_SECTOR_SIZE: 获取扇区大小（512字节）
 *   GET_BLOCK_SIZE: 获取擦除块大小（4096/512=8个扇区）
 * 
 * @param pdrv 驱动器编号
 * @param cmd 控制命令
 * @param buff 数据缓冲区
 * @return 操作结果
 */
DRESULT disk_ioctl (
	BYTE pdrv,		/* 驱动器编号 */
	BYTE cmd,		/* 控制命令 */
	void *buff		/* 数据缓冲区 */
)
{
	DRESULT status = RES_PARERR;
	
	switch (pdrv) 
	{
		case DEV_W25Qxx:
			switch(cmd)
			{
				case GET_SECTOR_COUNT:
					*(DWORD *)buff = FATFS_SECTOR_COUNT;  /* FatFS分区扇区数 */
					break;

				case GET_SECTOR_SIZE:
					*(WORD *)buff = 512;  /* 扇区大小512字节 */
					break;

				case GET_BLOCK_SIZE:
					*(DWORD *)buff = 8;  /* 擦除块大小：4096/512=8个扇区 */
					break;
			}
			status = RES_OK;
			break;
			
		default:
			status = RES_PARERR;
	}
 
	return status;
}
