/**
 * @file w25q128_drv.c
 * @brief W25Q128 SPI Flash 驱动实现（软件 SPI，直接函数调用）
 *
 * 从原 st_flash.c 迁移，移除函数指针封装层。
 * 逻辑完全不变，只是直接暴露函数。
 */

#include "board_config.h"
#include "w25q128_drv.h"

#if FLASH_DRIVER_W25Q128

#include "gd32f4xx.h"

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

/* 前向声明 */
static void flash_erase_sector_impl(uint32_t addr);

/* SPI Mode 0: CPOL=0, CPHA=0
   空闲 SCK=低, 上升沿采样数据, 下降沿切换数据 */
static uint8_t spi_readwrite_byte(uint8_t dat)
{
    uint8_t i, read_data = 0;

    FLASH_SCK_LOW();

    for (i = 0; i < 8; i++) {
        if (dat & 0x80) FLASH_MOSI_HIGH();
        else            FLASH_MOSI_LOW();
        dat <<= 1;

        FLASH_SCK_HIGH();
        read_data <<= 1;
        if (FLASH_MISO_READ()) {
            read_data |= 0x01;
        }
        FLASH_SCK_LOW();
    }

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
    uint32_t timeout = 0xFFFFF;
    while ((flash_read_sr() & 0x01) == 0x01) {
        if (--timeout == 0) break;
    }
}

/* ──── 公开接口 ──── */

void Flash_Init(void)
{
#if defined(CHIP_SERIES_F103)
    rcu_periph_clock_enable(FLASH_CS_CLK);
    rcu_periph_clock_enable(FLASH_SCK_CLK);
    rcu_periph_clock_enable(FLASH_MISO_CLK);
    rcu_periph_clock_enable(FLASH_MOSI_CLK);
#elif defined(CHIP_SERIES_F407)
    /* ⚠ 不能用 OR 组合 rcu_periph_enum！这些是 (寄存器偏移<<6)|位号 的打包值，
       不是位掩码。OR 后位号字段被破坏：GPIOB(0xC01)|GPIOG(0xC06)=0xC07
       → 解析成 bit7=GPIOH，GPIOB 时钟实际未使能，SPI 引脚全部失效
       （表现为 Flash_ReadID 读到 0x0000）。必须逐个使能。 */
    rcu_periph_clock_enable(RCU_GPIOB);   /* FLASH_CS/SCK/MISO/MOSI 均在 GPIOB */
    rcu_periph_clock_enable(RCU_GPIOG);

    /* 释放 PB3 (JTDO/SWO) 和 PB4 (NJTRST) 从调试功能，
       否则 PB3 作为 SPI SCK 输出可能被调试器 TRACE 功能覆盖。
       DBG_CTL0 bit5 (TRACE_IOEN) = 0: 禁用 TRACE 引脚，释放 PB3。 */
    DBG_CTL0 &= ~DBG_CTL0_TRACE_IOEN;

    gpio_mode_set(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_7);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    gpio_bit_set(GPIOG, GPIO_PIN_7);
#endif

    gpio_mode_set(FLASH_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, FLASH_CS_PIN);
    gpio_output_options_set(FLASH_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_CS_PIN);

    gpio_mode_set(FLASH_SCK_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, FLASH_SCK_PIN);
    gpio_output_options_set(FLASH_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_SCK_PIN);

    gpio_mode_set(FLASH_MOSI_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, FLASH_MOSI_PIN);
    gpio_output_options_set(FLASH_MOSI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_MOSI_PIN);

#if defined(CHIP_SERIES_F103)
    gpio_mode_set(FLASH_MISO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, FLASH_MISO_PIN);
#elif defined(CHIP_SERIES_F407)
    gpio_mode_set(FLASH_MISO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, FLASH_MISO_PIN);
#endif

    FLASH_CS_HIGH();
    FLASH_SCK_LOW();

    /* 延时等待 Flash 上电就绪 */
    for (volatile uint32_t i = 0; i < 100000; i++);
}

uint16_t Flash_ReadID(void)
{
    uint16_t temp = 0;
    uint8_t id_h, id_l;

    FLASH_CS_LOW();
    spi_readwrite_byte(W25X_ReleasePowerDown);
    FLASH_CS_HIGH();

    for (volatile uint32_t i = 0; i < 1000; i++);

    FLASH_CS_LOW();
    spi_readwrite_byte(W25X_ManufactDeviceID);
    spi_readwrite_byte(0x00);
    spi_readwrite_byte(0x00);
    spi_readwrite_byte(0x00);
    id_h = spi_readwrite_byte(0xFF);
    id_l = spi_readwrite_byte(0xFF);
    FLASH_CS_HIGH();

    temp = (id_h << 8) | id_l;
    return temp;
}

void Flash_Read(uint8_t *buf, uint32_t addr, uint32_t len)
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

static void flash_write_page(const uint8_t *buf, uint32_t addr, uint16_t len)
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

void Flash_WriteNoCheck(const uint8_t *buf, uint32_t addr, uint32_t len)
{
    uint16_t pageremain = 256 - (addr % 256);
    if (len <= pageremain) pageremain = len;

    while (1) {
        flash_write_page(buf, addr, pageremain);

        if (pageremain == len) break;

        buf += pageremain;
        addr += pageremain;
        len -= pageremain;

        pageremain = (len > 256) ? 256 : len;
    }
}

void Flash_Write(const uint8_t *buf, uint32_t addr, uint32_t len)
{
    static uint8_t sector_buf[4096];
    uint32_t secpos = addr / 4096;
    uint16_t secoff = addr % 4096;
    uint16_t secremain = 4096 - secoff;

    if (len <= secremain) secremain = len;

    while (1) {
        Flash_Read(sector_buf, secpos * 4096, 4096);

        uint16_t i;
        for (i = 0; i < secremain; i++) {
            if (sector_buf[secoff + i] != 0xFF) break;
        }

        if (i < secremain) {
            flash_erase_sector_impl(secpos * 4096);
            for (i = 0; i < secremain; i++) {
                sector_buf[secoff + i] = buf[i];
            }
            Flash_WriteNoCheck(sector_buf, secpos * 4096, 4096);
        } else {
            Flash_WriteNoCheck(buf, addr, secremain);
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

static void flash_erase_sector_impl(uint32_t addr)
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

void Flash_EraseSector(uint32_t addr)
{
    flash_erase_sector_impl(addr);
}

void Flash_EraseBlock(uint32_t addr)
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

uint8_t Flash_IsBusy(void)
{
    return (flash_read_sr() & 0x01);
}

void Flash_Reset(void)
{
    /* W25Q128 软件复位序列：0x66 + 0x99
     * 复位内部状态机到上电默认状态。
     * 复位后等待 30μs（datasheet tRST=30μs）。 */
    FLASH_CS_LOW();
    spi_readwrite_byte(0x66);   /* Enable Reset */
    FLASH_CS_HIGH();

    FLASH_CS_LOW();
    spi_readwrite_byte(0x99);   /* Reset Device */
    FLASH_CS_HIGH();

    /* 等待复位完成（datasheet: tRST = 30μs） */
    for (volatile uint32_t i = 0; i < 500; i++) __NOP();
}

#endif /* FLASH_DRIVER_W25Q128 */
