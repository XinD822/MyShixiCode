/**
  ******************************************************************************
  * @file    mass_mal.c
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   Medium Access Layer interface
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "mass_mal.h"

#include "config.h"
#include "mutex.h"
#include "error_handler.h"
#include "upgrade_config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t Mass_Memory_Size[2];
uint32_t Mass_Block_Size[2];
uint32_t Mass_Block_Count[2];
__IO uint32_t Status = 0;

#if defined(USE_STM3210E_EVAL) || defined(USE_STM32L152D_EVAL)
SD_CardInfo mSDCardInfo;
#endif

/* 4KB write cache to avoid repeated erase within same physical sector */
static uint8_t  mal_cache[4096];
static uint32_t mal_cache_addr = 0xFFFFFFFF;
static uint8_t  mal_cache_dirty = 0;

/* Mutex for SPI Flash access */
static Mutex_t spi_mutex;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : MAL_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Init(uint8_t lun)
{
  uint16_t status = MAL_OK;

  // Initialize mutex
  Mutex_Init(&spi_mutex);

  switch (lun)
  {
    case 0:
				W25QXX_Init();
				if(W25QXX_ReadID()==W25Q128)
				{
					printf("FLASH init OK\n");
					status = MAL_OK;
				}
				else
					status = MAL_FAIL;
      break;
    default:
      return MAL_FAIL;
	}
  return status;
}

/*******************************************************************************
* Function Name  : MAL_Write
* Description    : Write sectors (512-byte sector with 4KB cache)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Write(uint8_t lun, uint32_t Memory_Offset, uint32_t *Writebuff, uint16_t Transfer_Length)
{
	// 边界检查：防止越界写入
	if (Memory_Offset + Transfer_Length > FATFS_SIZE) {
		Error_Handler_Record(ERR_OUT_OF_RANGE);
		return MAL_FAIL;
	}

	// 加 FatFS 分区偏移，USB 只操作数据区
	uint32_t phys_addr = FATFS_BASE_ADDR + Memory_Offset;

	// Lock SPI mutex
	if (!Mutex_Lock(&spi_mutex, 100)) {
		Error_Handler_Record(ERR_MUTEX_TIMEOUT);
		return MAL_FAIL;
	}

	switch (lun)
  {
    case 0:
		{
			uint32_t cache_phys = phys_addr & ~0xFFF;
			uint16_t offset_in_sector = phys_addr & 0xFFF;

			if (cache_phys != mal_cache_addr) {
				if (mal_cache_dirty) {
					uint8_t all_ff = 1;
					for (uint32_t i = 0; i < 4096; i++) {
						if (mal_cache[i] != 0xFF) { all_ff = 0; break; }
					}
					if (!all_ff) {
						W25QXX_Erase_Sector(mal_cache_addr);
					}
					W25QXX_Write_NoCheck(mal_cache, mal_cache_addr, 4096);
					mal_cache_dirty = 0;
				}
				W25QXX_Read(mal_cache, cache_phys, 4096);
				mal_cache_addr = cache_phys;
			}

			memcpy(mal_cache + offset_in_sector, (uint8_t*)Writebuff, Transfer_Length);
			mal_cache_dirty = 1;
		}
      break;
    default:
      Mutex_Unlock(&spi_mutex);
      return MAL_FAIL;
  }

  Mutex_Unlock(&spi_mutex);
  return MAL_OK;
}


/*******************************************************************************
* Function Name  : MAL_Read 
* Description    : Read sectors (with cache priority)
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/
uint16_t MAL_Read(uint8_t lun, uint32_t Memory_Offset, uint32_t *Readbuff, uint16_t Transfer_Length)
{
	// 边界检查：防止越界访问
	if (Memory_Offset + Transfer_Length > FATFS_SIZE) {
		Error_Handler_Record(ERR_OUT_OF_RANGE);
		return MAL_FAIL;
	}

	// 加 FatFS 分区偏移，USB 只操作数据区
	uint32_t phys_addr = FATFS_BASE_ADDR + Memory_Offset;

	// Lock SPI mutex
	if (!Mutex_Lock(&spi_mutex, 100)) {
		Error_Handler_Record(ERR_MUTEX_TIMEOUT);
		return MAL_FAIL;
	}

	switch (lun)
  {
    case 0:
		{
			uint32_t cache_phys = phys_addr & ~0xFFF;
			uint16_t offset_in_sector = phys_addr & 0xFFF;

			if (cache_phys == mal_cache_addr && mal_cache_dirty) {
				memcpy((uint8_t*)Readbuff, mal_cache + offset_in_sector, Transfer_Length);
				break;
			}
			W25QXX_Read((uint8_t *)Readbuff, phys_addr, Transfer_Length);
		}
      break;
    default:
      Mutex_Unlock(&spi_mutex);
      return MAL_FAIL;
  }

  Mutex_Unlock(&spi_mutex);
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : MAL_FlushCache
* Description    : Flush pending write cache to SPI Flash
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void MAL_FlushCache(void)
{
	if (mal_cache_dirty) {
		// Lock SPI mutex
		if (!Mutex_Lock(&spi_mutex, 1000)) {
			Error_Handler_Record(ERR_MUTEX_TIMEOUT);
			return;
		}

		// Disable USB interrupt during flash write
		uint32_t primask = __get_PRIMASK();
		__disable_irq();

		uint8_t all_ff = 1;
		for (uint32_t i = 0; i < 4096; i++) {
			if (mal_cache[i] != 0xFF) { all_ff = 0; break; }
		}
		if (!all_ff) {
			W25QXX_Erase_Sector(mal_cache_addr);
		}
		W25QXX_Write_NoCheck(mal_cache, mal_cache_addr, 4096);
		mal_cache_dirty = 0;

		// Restore interrupt
		__set_PRIMASK(primask);

		Mutex_Unlock(&spi_mutex);
		printf("[MAL] Cache flushed at 0x%06X\r\n", mal_cache_addr);
	}
}

/*******************************************************************************
* Function Name  : MAL_IsCacheDirty
* Description    : Check if cache has pending writes
* Input          : None
* Output         : None
* Return         : 1=dirty, 0=clean
*******************************************************************************/
uint8_t MAL_IsCacheDirty(void)
{
	return mal_cache_dirty;
}

/*******************************************************************************
* Function Name  : MAL_GetCacheAddr
* Description    : Get current cache address
* Input          : None
* Output         : None
* Return         : Cache address
*******************************************************************************/
uint32_t MAL_GetCacheAddr(void)
{
	return mal_cache_addr;
}

/*******************************************************************************
* Function Name  : MAL_GetStatus 获取SPI_FLASH状态
* Description    : Get status
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_GetStatus (uint8_t lun)
{
  if (lun == 0)
  {
    W25QXX_Init();
		if(W25QXX_ReadID()==W25Q128)
    {
			Mass_Block_Size[0]  = 512;               // 扇区大小 512，兼容Windows
			Mass_Block_Count[0] = FATFS_SECTOR_COUNT; // 4MB数据区
			Mass_Memory_Size[0] = 512 * FATFS_SECTOR_COUNT;
      return MAL_OK;
    }
  }
	return MAL_FAIL;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
