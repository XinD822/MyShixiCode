/**
 * @file mass_mal.c
 * @brief Medium Access Layer 实现（HAL 解耦版）
 *
 * 通过 FlashService 访问存储，统一缓存路径。
 * 不直接调用 HAL_Flash，避免多重缓存冲突。
 */

#include "mass_mal.h"
#include "config.h"
#include "upgrade_config.h"
#include "flash_service.h"
#include "error_handler.h"
#include <string.h>

uint32_t Mass_Memory_Size[2];
uint32_t Mass_Block_Size[2];
uint32_t Mass_Block_Count[2];

uint16_t MAL_Init(uint8_t lun)
{
    if (lun != 0) return MAL_FAIL;

    HAL_Flash->init();

    if (HAL_Flash->read_id() != 0) {
        DBG_PRINTF("[MAL] Flash: %s\r\n", HAL_Flash->name);
        return MAL_OK;
    }

    return MAL_FAIL;
}

uint16_t MAL_GetStatus(uint8_t lun)
{
    if (lun == 0 && HAL_Flash->read_id() != 0) {
        Mass_Block_Size[0] = 512;
        Mass_Block_Count[0] = FATFS_SECTOR_COUNT;
        Mass_Memory_Size[0] = 512 * FATFS_SECTOR_COUNT;
        return MAL_OK;
    }
    return MAL_FAIL;
}

uint16_t MAL_Read(uint8_t lun, uint32_t offset, uint32_t *buf, uint16_t len)
{
    if (lun != 0) return MAL_FAIL;
    if (offset + len > FATFS_SIZE) {
        Error_Handler_Record(ERR_OUT_OF_RANGE);
        return MAL_FAIL;
    }

    FlashService_Read((uint8_t *)buf, FATFS_BASE_ADDR + offset, len);
    return MAL_OK;
}

uint16_t MAL_Write(uint8_t lun, uint32_t offset, uint32_t *buf, uint16_t len)
{
    if (lun != 0) return MAL_FAIL;
    if (offset + len > FATFS_SIZE) {
        Error_Handler_Record(ERR_OUT_OF_RANGE);
        return MAL_FAIL;
    }

    FlashService_Write((const uint8_t *)buf, FATFS_BASE_ADDR + offset, len);
    return MAL_OK;
}

void MAL_FlushCache(void)
{
    FlashService_FlushCache();
}
