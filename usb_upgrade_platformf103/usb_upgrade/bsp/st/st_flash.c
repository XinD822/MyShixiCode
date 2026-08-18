/**
 * @file st_flash.c
 * @brief STM32 W25Q128 SPI Flash 驱动实现（HAL 接口封装）
 *
 * 从原 spi_flash.c 迁移，封装为 HAL_Flash_Drv_t 接口。
 * 逻辑不变，只是套了一层函数指针。
 */

#include "hal_config.h"

#ifdef PLATFORM_STM32
#if FLASH_DRIVER_W25Q128

#include "board_config.h"

/* ──── Flash 命令定义 ──── */
#define W25X_WriteEnable        0x06
#define W25X_WriteDisable       0x04
#define W25X_ReadStatusReg      0x05
#define W25X_WriteStatusReg     0x01
#define W25X_ReadData           0x03
#define W25X_PageProgram        0x02
#define W25X_BlockErase         0xD8
#define W25X_SectorErase        0x20
#define W25X_ChipErase          0xC7
#define W25X_ManufactDeviceID   0x90
#define W25X_ReleasePowerDown   0xAB

#define W25Q128_ID              0xEF17

/* ──── 底层 SPI 操作 ──── */

static uint8_t spi_readwrite_byte(uint8_t dat)
{
    uint8_t i, read_data = 0;

    FLASH_SCK_HIGH();

    for (i = 0; i < 8; i++) {
        read_data <<= 1;
        if (dat & 0x80) FLASH_MOSI_HIGH();
        else            FLASH_MOSI_LOW();
        dat <<= 1;

        FLASH_SCK_LOW();
        FLASH_SCK_HIGH();

        if (FLASH_MISO_READ()) {
            read_data |= 0x01;
        }
    }

    FLASH_SCK_HIGH();
    return read_data;
}

static void flash_write_enable(void)
{
    FLASH_CS_LOW();
    spi_readwrite_byte(W25X_WriteEnable);
    FLASH_CS_HIGH();
}

static uint8_t flash_read_sr(void)
{
    uint8_t byte;
    FLASH_CS_LOW();
    spi_readwrite_byte(W25X_ReadStatusReg);
    byte = spi_readwrite_byte(0xFF);
    FLASH_CS_HIGH();
    return byte;
}

static void flash_wait_busy(void)
{
    while ((flash_read_sr() & 0x01) == 0x01);
}

/* ──── HAL 接口实现 ──── */

static void w25q128_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(
        FLASH_CS_CLK | FLASH_SCK_CLK | FLASH_MISO_CLK | FLASH_MOSI_CLK,
        ENABLE);

    /* CS, SCK, MOSI - 推挽输出 */
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;

    GPIO_InitStruct.GPIO_Pin = FLASH_CS_PIN;
    GPIO_Init(FLASH_CS_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = FLASH_SCK_PIN;
    GPIO_Init(FLASH_SCK_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = FLASH_MOSI_PIN;
    GPIO_Init(FLASH_MOSI_PORT, &GPIO_InitStruct);

    /* MISO - 上拉输入 */
    GPIO_InitStruct.GPIO_Pin = FLASH_MISO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(FLASH_MISO_PORT, &GPIO_InitStruct);

    FLASH_CS_HIGH();
}

static uint16_t w25q128_read_id(void)
{
    uint16_t temp = 0;
    FLASH_CS_LOW();
    spi_readwrite_byte(W25X_ManufactDeviceID);
    spi_readwrite_byte(0x00);
    spi_readwrite_byte(0x00);
    spi_readwrite_byte(0x00);
    temp |= (spi_readwrite_byte(0xFF) << 8);
    temp |= spi_readwrite_byte(0xFF);
    FLASH_CS_HIGH();
    return temp;
}

static void w25q128_read(uint8_t *buf, uint32_t addr, uint32_t len)
{
    FLASH_CS_LOW();
    spi_readwrite_byte(W25X_ReadData);
    spi_readwrite_byte((uint8_t)(addr >> 16));
    spi_readwrite_byte((uint8_t)(addr >> 8));
    spi_readwrite_byte((uint8_t)addr);

    for (uint32_t i = 0; i < len; i++) {
        buf[i] = spi_readwrite_byte(0xFF);
    }
    FLASH_CS_HIGH();
}

static void w25q128_erase_sector(uint32_t addr);

static void w25q128_write_page(const uint8_t *buf, uint32_t addr, uint16_t len)
{
    if (len > 256) len = 256;

    flash_write_enable();
    FLASH_CS_LOW();
    spi_readwrite_byte(W25X_PageProgram);
    spi_readwrite_byte((uint8_t)(addr >> 16));
    spi_readwrite_byte((uint8_t)(addr >> 8));
    spi_readwrite_byte((uint8_t)addr);

    for (uint16_t i = 0; i < len; i++) {
        spi_readwrite_byte(buf[i]);
    }
    FLASH_CS_HIGH();
    flash_wait_busy();
}

static void w25q128_write_nocheck(const uint8_t *buf, uint32_t addr, uint32_t len)
{
    uint16_t pageremain = 256 - (addr % 256);
    if (len <= pageremain) pageremain = len;

    while (1) {
        w25q128_write_page(buf, addr, pageremain);

        if (pageremain == len) break;

        buf += pageremain;
        addr += pageremain;
        len -= pageremain;

        pageremain = (len > 256) ? 256 : len;
    }
}

static void w25q128_write(const uint8_t *buf, uint32_t addr, uint32_t len)
{
    static uint8_t sector_buf[4096];
    uint32_t secpos = addr / 4096;
    uint16_t secoff = addr % 4096;
    uint16_t secremain = 4096 - secoff;

    if (len <= secremain) secremain = len;

    while (1) {
        w25q128_read(sector_buf, secpos * 4096, 4096);

        uint16_t i;
        for (i = 0; i < secremain; i++) {
            if (sector_buf[secoff + i] != 0xFF) break;
        }

        if (i < secremain) {
            /* 需要擦除 */
            w25q128_erase_sector(secpos * 4096);
            for (i = 0; i < secremain; i++) {
                sector_buf[secoff + i] = buf[i];
            }
            w25q128_write_nocheck(sector_buf, secpos * 4096, 4096);
        } else {
            w25q128_write_nocheck(buf, addr, secremain);
        }

        if (len == secremain) break;

        secpos++;
        secoff = 0;
        buf += secremain;
        addr += secremain;
        len -= secremain;
        secremain = (len > 4096) ? 4096 : len;
    }
}

static void w25q128_erase_sector(uint32_t addr)
{
    flash_write_enable();
    flash_wait_busy();

    FLASH_CS_LOW();
    spi_readwrite_byte(W25X_SectorErase);
    spi_readwrite_byte((uint8_t)(addr >> 16));
    spi_readwrite_byte((uint8_t)(addr >> 8));
    spi_readwrite_byte((uint8_t)addr);
    FLASH_CS_HIGH();
    flash_wait_busy();
}

static void w25q128_erase_block(uint32_t addr)
{
    flash_write_enable();
    flash_wait_busy();

    FLASH_CS_LOW();
    spi_readwrite_byte(W25X_BlockErase);
    spi_readwrite_byte((uint8_t)(addr >> 16));
    spi_readwrite_byte((uint8_t)(addr >> 8));
    spi_readwrite_byte((uint8_t)addr);
    FLASH_CS_HIGH();
    flash_wait_busy();
}

static uint8_t w25q128_is_busy(void)
{
    return (flash_read_sr() & 0x01);
}

/* ──── 驱动实例 ──── */

const HAL_Flash_Drv_t W25Q128_Drv = {
    .name           = "W25Q128",
    .capacity       = 16 * 1024 * 1024,   /* 16MB */
    .sector_size    = 4096,
    .block_size     = 65536,
    .init           = w25q128_init,
    .read_id        = w25q128_read_id,
    .read           = w25q128_read,
    .write_nocheck  = w25q128_write_nocheck,
    .write          = w25q128_write,
    .erase_sector   = w25q128_erase_sector,
    .erase_block    = w25q128_erase_block,
    .is_busy        = w25q128_is_busy,
};

/* 全局驱动指针 */
const HAL_Flash_Drv_t *HAL_Flash = &W25Q128_Drv;

/* ──── 兼容旧接口（可选） ──── */

#ifdef USB_UPGRADE_COMPAT_FUNCTIONS
void     W25QXX_Init(void)                                          { w25q128_init(); }
uint16_t W25QXX_ReadID(void)                                        { return w25q128_read_id(); }
void     W25QXX_Read(uint8_t *b, uint32_t a, uint32_t l)           { w25q128_read(b, a, l); }
void     W25QXX_Write_NoCheck(uint8_t *b, uint32_t a, uint32_t l)  { w25q128_write_nocheck(b, a, l); }
void     W25QXX_Write(uint8_t *b, uint32_t a, uint32_t l)          { w25q128_write(b, a, l); }
void     W25QXX_Erase_Sector(uint32_t a)                            { w25q128_erase_sector(a); }
void     W25QXX_Erase_Block(uint32_t a)                             { w25q128_erase_block(a); }
void     W25QXX_Wait_Busy(void)                                     { flash_wait_busy(); }
#endif

#endif /* FLASH_DRIVER_W25Q128 */
#endif /* PLATFORM_STM32 */
