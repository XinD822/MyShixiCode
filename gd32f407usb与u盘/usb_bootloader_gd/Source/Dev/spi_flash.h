#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

#include "platform.h"
#include <stdio.h>


#define W25Q80              0XEF13
#define W25Q16              0XEF14
#define W25Q32              0XEF15
#define W25Q64              0XEF16
#define W25Q128             0XEF17
#define W25Q10BW            0xFF5E

#define SPI_FLASH_PageSize              256
#define SPI_FLASH_PerWritePageSize      256

/* 命令定义 */
#define W25X_WriteEnable        0x06
#define W25X_WriteDisable       0x04
#define W25X_ReadStatusReg      0x05
#define W25X_WriteStatusReg     0x01
#define W25X_ReadData           0x03
#define W25X_FastReadData       0x0B
#define W25X_FastReadDual       0x3B
#define W25X_PageProgram        0x02
#define W25X_BlockErase         0xD8
#define W25X_SectorErase        0x20
#define W25X_ChipErase          0xC7
#define W25X_PowerDown          0xB9
#define W25X_ReleasePowerDown   0xAB
#define W25X_DeviceID           0xAB
#define W25X_ManufactDeviceID   0x90
#define W25X_JedecDeviceID      0x9F

#define WIP_Flag        0x01
#define Dummy_Byte      0xFF

/* SPI 接口定义（软件 SPI）
 * F103: CS=PB12, SCK=PB13, MISO=PB14, MOSI=PB15
 * F407: CS=PB14, SCK=PB3,  MISO=PB4,  MOSI=PB5
 */
#if defined(CHIP_SERIES_F103)
  #define FLASH_SPI_CLK_CMD        RCC_APB2PeriphClockCmd
  #define FLASH_SPI_GPIO_CLK       RCC_APB2Periph_GPIOB
  #define FLASH_SPI_CS_PORT        GPIOB
  #define FLASH_SPI_CS_PIN         GPIO_Pin_12
  #define FLASH_SPI_SCK_PORT       GPIOB
  #define FLASH_SPI_SCK_PIN        GPIO_Pin_13
  #define FLASH_SPI_MISO_PORT      GPIOB
  #define FLASH_SPI_MISO_PIN       GPIO_Pin_14
  #define FLASH_SPI_MOSI_PORT      GPIOB
  #define FLASH_SPI_MOSI_PIN       GPIO_Pin_15
#elif defined(CHIP_SERIES_F407)
  #define FLASH_SPI_GPIO_CLK       RCU_GPIOB
  #define FLASH_SPI_CS_PORT        GPIOB
  #define FLASH_SPI_CS_PIN         GPIO_PIN_14
  #define FLASH_SPI_SCK_PORT       GPIOB
  #define FLASH_SPI_SCK_PIN        GPIO_PIN_3
  #define FLASH_SPI_MISO_PORT      GPIOB
  #define FLASH_SPI_MISO_PIN       GPIO_PIN_4
  #define FLASH_SPI_MOSI_PORT      GPIOB
  #define FLASH_SPI_MOSI_PIN       GPIO_PIN_5
#endif

#define SPI_FLASH_CS_LOW()     gpio_bit_reset(FLASH_SPI_CS_PORT, FLASH_SPI_CS_PIN)
#define SPI_FLASH_CS_HIGH()    gpio_bit_set(FLASH_SPI_CS_PORT, FLASH_SPI_CS_PIN)
#define SPI_FLASH_SCLK_LOW()   gpio_bit_reset(FLASH_SPI_SCK_PORT, FLASH_SPI_SCK_PIN)
#define SPI_FLASH_SCLK_HIGH()  gpio_bit_set(FLASH_SPI_SCK_PORT, FLASH_SPI_SCK_PIN)
#define SPI_FLASH_MOSI_LOW()   gpio_bit_reset(FLASH_SPI_MOSI_PORT, FLASH_SPI_MOSI_PIN)
#define SPI_FLASH_MOSI_HIGH()  gpio_bit_set(FLASH_SPI_MOSI_PORT, FLASH_SPI_MOSI_PIN)


void W25QXX_Init(void);
uint8_t SPI1_ReadWriteByte(u8 dat);
uint8_t W25QXX_ReadSR(void);
void W25QXX_Write_SR(u8 sr);
void W25QXX_Write_Enable(void);
void W25QXX_Write_Disable(void);
u16 W25QXX_ReadID(void);
void W25QXX_Read(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
void W25QXX_Write_Page(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
void W25QXX_Write_NoCheck(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
void W25QXX_Write(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
void W25QXX_Erase_Chip(void);
void W25QXX_Erase_Sector(u32 Dst_Addr);
void W25QXX_Erase_Block(u32 Dst_Addr);
void W25QXX_Wait_Busy(void);
void W25QXX_PowerDown(void);
void W25QXX_WAKEUP(void);


#endif /* __SPI_FLASH_H */
