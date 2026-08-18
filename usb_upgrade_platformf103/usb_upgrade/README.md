# USB 拖拽升级模块 (usb_upgrade)

## 目录

- [1. 模块简介](#1-模块简介)
- [2. 目录结构](#2-目录结构)
- [3. 快速集成](#3-快速集成)
- [4. uCOS-II 集成指南](#4-ucos-ii-集成指南)
- [5. 分区配置修改](#5-分区配置修改)
- [6. 硬件资源配置](#6-硬件资源配置)
- [7. 移植指南](#7-移植指南)
- [8. 支持的芯片](#8-支持的芯片)
- [9. 安全机制](#9-安全机制)
- [10. 常见问题](#10-常见问题)
- [11. API 参考](#11-api-参考)

---

## 1. 模块简介

USB 拖拽升级模块是一个**可移植、可解耦**的固件升级解决方案。用户只需将固件文件 `firmware.bin` 拖入 U 盘即可完成升级。

### 特性

- **极简集成**：只需 3 行代码
- **最小依赖**：只需 SPI Flash + USB，串口和 LED 均为可选
- **平台无关**：通过 HAL 抽象层支持多 MCU 平台
- **RTOS 兼容**：支持裸机和 uCOS-II 双模式
- **掉电安全**：完整的状态机保护，任何阶段掉电都能安全恢复
- **防死循环**：自动清理固件文件，防止重复升级
- **可扩展**：预留 SD 卡和 U 盘升级接口

### 升级流程

```
PC 拖入 firmware.bin → USB MSC 写入 W25Q128 数据区
                            ↓
                    APP 检测到固件 → 写入 Slot A → 设标志 → 重启
                            ↓
                    Bootloader 读标志 → 搬运到内部 Flash → 跳转 APP
                            ↓
                    APP 确认存活 → 清除标志
```

---

## 2. 目录结构

```
usb_upgrade/
├── inc/
│   └── usb_upgrade.h          # 对外唯一接口
├── src/
│   ├── usb_upgrade.c          # 模块入口
│   ├── usb_task.c/h           # USB 任务处理
│   ├── upgrade.c/h            # 升级状态机
│   ├── upgrade_config.h       # 分区配置（改这里）
│   ├── upgrade_source.c/h     # 升级来源管理
│   ├── source_usb_drag.c      # USB 拖拽来源实现
│   ├── flash_service.c/h      # Flash 读写服务（带缓存）
│   ├── firmware_check.c/h     # 固件校验
│   ├── fatfs_system.c/h       # FatFS 封装
│   ├── error_handler.c/h      # 错误处理
│   ├── mutex.c/h              # 互斥锁（裸机/RTOS 双模式）
│   └── config.h               # 内部统一头文件
├── bsp/
│   ├── board_config.h         # 板级引脚配置
│   ├── hal_config.h           # 平台总配置（改这里）
│   ├── Delay.h                # 延时接口
│   ├── hal/                   # HAL 接口定义
│   │   ├── hal_flash.h
│   │   ├── hal_gpio.h
│   │   ├── hal_pwr.h
│   │   ├── hal_tick.h
│   │   ├── hal_uart.h
│   │   └── hal_usb.h
│   └── st/                    # STM32 平台实现
│       ├── st_platform.h
│       ├── st_flash.c
│       ├── st_gpio.c
│       ├── st_pwr.c
│       ├── st_tick.c
│       ├── st_uart.c
│       └── st_usb.c
└── middleware/
    ├── CherryUSB/              # USB 协议栈
    │   ├── usb_msc_device.c/h  # MSC 设备封装
│   │   ├── mass_mal.c/h       # 存储介质层
│   │   ├── usb_config.h       # USB 配置
│   │   ├── usb_osal.c         # OS 抽象层
│   │   ├── core/              # CherryUSB 核心
│   │   ├── class/             # USB 类实现
│   │   ├── common/            # 公共头文件
│   │   └── port/              # 平台移植层
    └── FatFS/                  # FAT 文件系统
        ├── ff.c/h
        ├── ffconf.h
        ├── diskio.c/h
        └── ...
```

---

## 3. 快速集成

### 3.1 裸机模式

**步骤 1：复制模块**

将整个 `usb_upgrade/` 目录复制到你的工程中。

**步骤 2：配置 Keil 工程**

添加以下目录到 Include Paths：
```
usb_upgrade/inc
usb_upgrade/src
usb_upgrade/bsp
usb_upgrade/bsp/hal
usb_upgrade/bsp/st
usb_upgrade/middleware/CherryUSB
usb_upgrade/middleware/CherryUSB/core
usb_upgrade/middleware/CherryUSB/class
usb_upgrade/middleware/CherryUSB/common
usb_upgrade/middleware/CherryUSB/port/st
usb_upgrade/middleware/FatFS
```

添加源文件到工程：
```
usb_upgrade/src/*.c
usb_upgrade/bsp/st/*.c
usb_upgrade/middleware/CherryUSB/usb_msc_device.c
usb_upgrade/middleware/CherryUSB/mass_mal.c
usb_upgrade/middleware/CherryUSB/usb_osal.c
usb_upgrade/middleware/CherryUSB/core/usbd_core.c
usb_upgrade/middleware/CherryUSB/class/usbd_msc.c
usb_upgrade/middleware/CherryUSB/port/st/usb_dc_fsdev.c
usb_upgrade/middleware/CherryUSB/port/st/usb_glue_st.c
usb_upgrade/middleware/FatFS/*.c
```

**步骤 3：修改 main.c**

```c
#include "stm32f10x.h"
#include "usb_upgrade.h"    // 1. 包含头文件

int main(void)
{
    // ... 你的初始化代码 ...
    
    USB_Upgrade_Init();      // 2. 初始化升级模块
    
    while (1)
    {
        // ... 你的主循环代码 ...
        
        USB_Upgrade_Run();   // 3. 运行升级任务
    }
}
```

**步骤 4：配置平台**

编辑 `bsp/hal_config.h`：

```c
#define PLATFORM_STM32      // 选择平台
#define CHIP_SERIES_F103    // 选择芯片系列
#define USE_BAREMETAL       // 裸机模式
```

**步骤 5：配置引脚**

编辑 `bsp/board_config.h`，修改 SPI Flash 引脚定义（必须）。

**可选配置：**
- 串口调试：取消注释 `#define USB_UPGRADE_USE_UART`
- LED 指示：取消注释 `#define USB_UPGRADE_USE_LED`

> 最小依赖：只需 SPI Flash + USB，串口和 LED 均为可选。

---

## 4. uCOS-II 集成指南

### 4.1 配置切换

编辑 `bsp/hal_config.h`：

```c
// #define USE_BAREMETAL    // 注释掉
#define USE_UCOS2           // 启用 uCOS-II
```

### 4.2 任务创建

```c
#include "os.h"
#include "usb_upgrade.h"

/* 任务配置 */
#define TASK_USB_UPGRADE_PRIO   10      // 任务优先级
#define TASK_USB_UPGRADE_STK    512     // 栈大小（单位：OS_STK）

/* 任务栈 */
OS_STK TaskUsbUpgradeStk[TASK_USB_UPGRADE_STK];

/* 任务函数 */
void TaskUsbUpgrade(void *p_arg)
{
    (void)p_arg;
    
    // 初始化（必须在 OSStart() 之后调用）
    USB_Upgrade_Init();
    
    while (1) {
        USB_Upgrade_Run();
        OSTimeDly(10);  // 10ms 周期，让出 CPU
    }
}

/* main 函数 */
int main(void)
{
    OSInit();
    
    // 创建升级任务
    OSTaskCreate(TaskUsbUpgrade, 
                 NULL,
                 &TaskUsbUpgradeStk[TASK_USB_UPGRADE_STK - 1],
                 TASK_USB_UPGRADE_PRIO);
    
    OSStart();
    return 0;
}
```

### 4.3 uCOS-II 安全机制

| 机制 | 实现 | 说明 |
|------|------|------|
| 互斥锁 | `OSMutexCreate(OS_PRIO_MUTEX_CEIL_DIS)` | 优先级天花板，防止优先级反转 |
| 临界区 | `OS_ENTER_CRITICAL()` | RTOS 原生临界区保护 |
| 静态栈 | `usb_ep0_task_stk[]` | 避免在中断中 malloc |
| 超时 | `OSMutexPend(mutex, ticks, &err)` | 所有阻塞调用都有超时 |

### 4.4 任务优先级建议

| 任务 | 建议优先级 | 说明 |
|------|-----------|------|
| 关键控制任务 | 5 | 最高优先级 |
| USB EP0 | 4 | USB 控制传输 |
| USB MSC | 6 | USB 数据传输 |
| **USB 升级任务** | **10** | 升级检测和处理 |
| LED / 显示 | 15 | 低优先级 |

### 4.5 CherryUSB 线程配置

编辑 `middleware/CherryUSB/usb_config.h`：

```c
/* EP0 线程 */
#define CONFIG_USBDEV_EP0_PRIO          4
#define CONFIG_USBDEV_EP0_STACKSIZE     2048    // 字节

/* MSC 线程 */
#define CONFIG_USBDEV_MSC_PRIO          6
#define CONFIG_USBDEV_MSC_STACKSIZE     2048    // 字节

/* uCOS-II 静态栈大小（单位：OS_STK 个数，1个 = 4字节） */
#define USB_UCOS_EP0_STK_SIZE   512     // 2KB
#define USB_UCOS_MSC_STK_SIZE   512     // 2KB
```

### 4.6 注意事项

1. **初始化顺序**：`USB_Upgrade_Init()` 必须在 `OSStart()` 之后调用
2. **栈大小**：升级任务栈建议 512 字节以上
3. **任务周期**：建议 10ms，太大会影响 USB 响应
4. **优先级**：不要设置太高，避免阻塞其他关键任务

---

## 5. 分区配置修改

所有分区配置都在 `src/upgrade_config.h` 中，**三个工程必须保持一致**：
- `usb_bootloader/Source/Dev/upgrade_config.h`
- `usb_upgrade_platform/usb_upgrade/src/upgrade_config.h`
- `usb_app_try/usb_upgrade/src/upgrade_config.h`

### 5.1 当前默认配置

```
STM32F103 内部 Flash (512KB):
┌────────────────────────────────────────────────────────────┐
│ 0x08000000 ──────────────── 0x0800FFFF │ Bootloader 64KB  │
│ 0x08010000 ──────────────── 0x0807FFFF │ APP 448KB        │
└────────────────────────────────────────────────────────────┘

W25Q128 外部 Flash (16MB):
┌────────────────────────────────────────────────────────────┐
│ 0x000000 ── 0x5DFFFF │ 资源区 5.875MB                     │
│ 0x5E0000 ── 0x8DFFFF │ Slot A 3MB (固件)                  │
│ 0x8E0000 ── 0xBDFFFF │ Slot B 3MB (备份)                  │
│ 0xBE0000 ── 0xFDFFFF │ 数据区 4MB (FatFS)                 │
│ 0xFE0000 ── 0xFFFFFF │ 配置区 128KB                       │
└────────────────────────────────────────────────────────────┘
```

### 5.2 修改 Bootloader 大小

**场景**：Bootloader 从 64KB 扩大到 128KB

```c
/* 原配置 */
#define BOOTLOADER_SIZE         0x00010000  // 64KB
#define APP_ADDR                0x08010000
#define APP_SIZE                0x00070000  // 448KB

/* 新配置 */
#define BOOTLOADER_SIZE         0x00020000  // 128KB
#define APP_ADDR                0x08020000  // 地址也要改！
#define APP_SIZE                0x00060000  // 384KB
```

**同时需要修改**：
1. Keil 工程的 Scatter File（链接脚本）
2. Bootloader 工程的起始地址设置
3. APP 工程的起始地址设置

### 5.3 修改固件大小

**场景**：固件从 3MB 扩大到 4MB

```c
/* 原配置 */
#define FIRMWARE_SLOT_A_ADDR    0x5E0000
#define FIRMWARE_SLOT_B_ADDR    0x8E0000
#define FIRMWARE_SLOT_SIZE      0x300000    // 3MB
#define FIRMWARE_MAX_SIZE       (3 * 1024 * 1024)

/* 新配置 */
#define FIRMWARE_SLOT_A_ADDR    0x5E0000
#define FIRMWARE_SLOT_B_ADDR    0x9E0000    // A + 4MB
#define FIRMWARE_SLOT_SIZE      0x400000    // 4MB
#define FIRMWARE_MAX_SIZE       (4 * 1024 * 1024)
```

### 5.4 修改 FatFS 数据区大小

**场景**：数据区从 4MB 缩小到 2MB（给固件腾空间）

```c
/* 原配置 */
#define FATFS_BASE_ADDR         0xBE0000
#define FATFS_SIZE              0x400000    // 4MB
#define FATFS_SECTOR_COUNT      8192

/* 新配置 */
#define FATFS_BASE_ADDR         0xBE0000
#define FATFS_SIZE              0x200000    // 2MB
#define FATFS_SECTOR_COUNT      4096
```

### 5.5 完整重分区示例

**需求**：Bootloader 128KB + APP 384KB + 固件 4MB + 数据区 2MB

```c
/* ──── W25Q128 分区地址 ──── */
#define ASSET_BASE_ADDR         0x000000
#define ASSET_SIZE              0x5E0000    // 5.875MB

#define FIRMWARE_SLOT_A_ADDR    0x5E0000
#define FIRMWARE_SLOT_B_ADDR    0x9E0000    // A + 4MB
#define FIRMWARE_SLOT_SIZE      0x400000    // 4MB
#define FIRMWARE_BASE_ADDR      FIRMWARE_SLOT_A_ADDR

#define FATFS_BASE_ADDR         0xBE0000
#define FATFS_SIZE              0x200000    // 2MB
#define FATFS_SECTOR_COUNT      4096

#define CONFIG_AREA_ADDR        0xFE0000
#define CONFIG_AREA_SIZE        0x020000    // 128KB

/* ──── 配置区偏移（一般不用改）──── */
#define UPGRADE_FLAG_ADDR       (CONFIG_AREA_ADDR + 0x0000)
#define UPGRADE_STATE_ADDR      (CONFIG_AREA_ADDR + 0x0004)
#define FIRMWARE_SIZE_ADDR      (CONFIG_AREA_ADDR + 0x0008)
#define FIRMWARE_CRC_ADDR       (CONFIG_AREA_ADDR + 0x000C)
#define ACTIVE_SLOT_ADDR        (CONFIG_AREA_ADDR + 0x0010)

/* ──── 升级标志值（一般不用改）──── */
#define UPGRADE_FLAG_YES        0x12345678
#define UPGRADE_FLAG_NO         0x00000000

/* ──── 升级状态（一般不用改）──── */
#define UPGRADE_STATE_NONE      0x00000000
#define UPGRADE_STATE_BURNING   0x11111111
#define UPGRADE_STATE_DONE      0x22222222
#define UPGRADE_STATE_CONFIRMED 0x33333333

/* ──── 内部 Flash 分区 ──── */
#define BOOTLOADER_ADDR         0x08000000
#define BOOTLOADER_SIZE         0x00020000  // 128KB
#define APP_ADDR                0x08020000
#define APP_SIZE                0x00060000  // 384KB

/* ──── 固件配置 ──── */
#define FIRMWARE_FILENAME_MAX   64
#define FIRMWARE_BUF_SIZE       (4 * 1024)  // 4KB
#define FIRMWARE_MAX_SIZE       (4 * 1024 * 1024)  // 4MB
```

### 5.6 验证配置

在 main.c 中添加验证代码：

```c
printf("=== 分区配置验证 ===\r\n");
printf("Bootloader: 0x%08X, Size: %dKB\r\n", BOOTLOADER_ADDR, BOOTLOADER_SIZE/1024);
printf("APP:        0x%08X, Size: %dKB\r\n", APP_ADDR, APP_SIZE/1024);
printf("Slot A:     0x%08X, Size: %dKB\r\n", FIRMWARE_SLOT_A_ADDR, FIRMWARE_SLOT_SIZE/1024);
printf("Slot B:     0x%08X, Size: %dKB\r\n", FIRMWARE_SLOT_B_ADDR, FIRMWARE_SLOT_SIZE/1024);
printf("FatFS:      0x%08X, Size: %dKB\r\n", FATFS_BASE_ADDR, FATFS_SIZE/1024);
printf("Config:     0x%08X, Size: %dKB\r\n", CONFIG_AREA_ADDR, CONFIG_AREA_SIZE/1024);
```

---

## 6. 硬件资源配置

### 6.1 定时器选择（三种模式）

所有定时器配置都在 `bsp/board_config.h` 中。

#### 模式 A：使用模块自带定时器（默认 TIM2）

新工程推荐，开箱即用，什么都不用改。

#### 模式 B：换定时器（如 TIM2 已被占用）

```c
// board_config.h — 改这几行即可
#define TICK_TIM                TIM3
#define TICK_TIM_IRQn           TIM3_IRQn
#define TICK_TIM_IRQHandler     TIM3_IRQHandler
#define TICK_TIM_RCC            RCC_APB1Periph_TIM3
```

可选定时器：TIM2, TIM3, TIM4, TIM5 等。

#### 模式 C：外部 Tick 注入（不占用额外定时器）

老工程已有 SysTick/TIM 驱动时使用。

```c
// board_config.h — 取消注释这行
#define TICK_EXTERNAL
```

然后在你自己的定时器中断里加一行：

```c
void SysTick_Handler(void)  // 或 TIMx_IRQHandler
{
    // ... 你原来的代码 ...
    USB_Upgrade_TickInc();  // ← 加这一行，1ms 调一次
}
```

### 6.2 串口（可选）

串口用于调试输出，默认**关闭**，不占用任何 USART 资源。

**启用串口：**

```c
// board_config.h — 取消注释这行
#define USB_UPGRADE_USE_UART
```

**切换串口（如 USART1 已被占用）：**

```c
// board_config.h
#define DEBUG_UART              USART2      // 改成你空闲的串口
#define DEBUG_UART_IRQn         USART2_IRQn
#define DEBUG_UART_IRQHandler   USART2_IRQHandler
#define DEBUG_UART_RCC          RCC_APB1Periph_USART2
#define DEBUG_UART_RCC_GPIO     RCC_APB2Periph_GPIOA
#define DEBUG_UART_TX_PORT      GPIOA
#define DEBUG_UART_TX_PIN       GPIO_Pin_2
#define DEBUG_UART_RX_PORT      GPIOA
#define DEBUG_UART_RX_PIN       GPIO_Pin_3
#define USART_BAUDRATE          115200
```

**printf 重定向：** 如果老工程已有 `fputc` 定义，注释掉 `USB_UPGRADE_IMPLEMENT_FPUTC`。

### 6.3 LED 指示灯（可选）

LED 用于状态指示，默认**关闭**，不占用任何 GPIO 资源。

```c
// board_config.h — 取消注释这行
#define USB_UPGRADE_USE_LED
```

启用后可修改 LED 引脚：

```c
#define LED0_PORT           GPIOB
#define LED0_PIN            GPIO_Pin_5
#define LED1_PORT           GPIOE
#define LED1_PIN            GPIO_Pin_5
```

### 6.4 兼容旧接口

如果不需要 `Delay_*`, `W25QXX_*` 等兼容函数：

```c
// board_config.h
// #define USB_UPGRADE_COMPAT_FUNCTIONS   // 注释掉
```

### 6.5 主频配置

```c
// board_config.h
#define SYSTEM_CORE_CLOCK       72      // STM32F103: 72MHz
// #define SYSTEM_CORE_CLOCK       168   // STM32F407: 168MHz
```

---

## 7. 移植指南

### 7.1 移植到新 STM32 芯片

**只需修改两个文件**：

#### 7.1.1 修改 `bsp/hal_config.h`

```c
/* 芯片系列 */
// #define CHIP_SERIES_F103
#define CHIP_SERIES_F407        // 改为新芯片系列
```

#### 7.1.2 修改 `bsp/board_config.h`

```c
/* SPI Flash 引脚（根据新板子修改） */
#define FLASH_CS_PORT       GPIOB
#define FLASH_CS_PIN        GPIO_Pin_12
// ... 其他引脚 ...
```

### 7.2 移植到新平台（如 GD32）

#### 7.2.1 创建平台目录

```
bsp/
└── gd/                     # 新建 GD32 平台目录
    ├── gd_platform.h       # 平台头文件
    ├── gd_flash.c          # Flash 驱动
    ├── gd_gpio.c           # GPIO 驱动
    ├── gd_pwr.c            # 电源管理
    ├── gd_tick.c           # 定时器
    ├── gd_uart.c           # 串口
    └── gd_usb.c            # USB
```

#### 6.2.2 实现 HAL 接口

每个驱动都需要实现对应的 `HAL_xxx_Drv_t` 结构体：

```c
// gd_flash.c
#include "hal_config.h"

#ifdef PLATFORM_GD32

static void gd_flash_init(void) { /* GPIO/SPI 初始化 */ }
static uint16_t gd_flash_read_id(void) { /* 读取 ID */ }
static void gd_flash_read(uint8_t *buf, uint32_t addr, uint32_t len) { /* 读取 */ }
static void gd_flash_write_nocheck(const uint8_t *buf, uint32_t addr, uint32_t len) { /* 写入 */ }
static void gd_flash_erase_sector(uint32_t addr) { /* 扇区擦除 */ }
static void gd_flash_erase_block(uint32_t addr) { /* 块擦除 */ }
static uint8_t gd_flash_is_busy(void) { /* 忙检测 */ }

const HAL_Flash_Drv_t GD_Flash_Drv = {
    .name           = "W25Q128",
    .capacity       = 16 * 1024 * 1024,
    .sector_size    = 4096,
    .block_size     = 65536,
    .init           = gd_flash_init,
    .read_id        = gd_flash_read_id,
    .read           = gd_flash_read,
    .write_nocheck  = gd_flash_write_nocheck,
    .erase_sector   = gd_flash_erase_sector,
    .erase_block    = gd_flash_erase_block,
    .is_busy        = gd_flash_is_busy,
};

const HAL_Flash_Drv_t *HAL_Flash = &GD_Flash_Drv;

#endif /* PLATFORM_GD32 */
```

#### 7.2.2 修改配置

```c
// bsp/hal_config.h
// #define PLATFORM_STM32
#define PLATFORM_GD32           // 启用 GD32

// #define CHIP_SERIES_F103
#define CHIP_SERIES_GD32F450    // GD32 芯片系列
```

### 7.3 移植 Checklist

| 项目 | 文件 | 说明 | 必须？ |
|------|------|------|--------|
| ✅ 平台选择 | `hal_config.h` | 设置 PLATFORM_xxx | 必须 |
| ✅ 芯片系列 | `hal_config.h` | 设置 CHIP_SERIES_xxx | 必须 |
| ✅ RTOS 模式 | `hal_config.h` | 设置 USE_BAREMETAL 或 USE_UCOS2 | 必须 |
| ✅ Flash 引脚 | `board_config.h` | SPI Flash 引脚定义 | 必须 |
| ✅ 主频配置 | `board_config.h` | SYSTEM_CORE_CLOCK | 必须 |
| ✅ Flash 驱动 | `bsp/xxx/xxx_flash.c` | 实现 HAL_Flash_Drv_t | 必须 |
| ✅ Tick 驱动 | `bsp/xxx/xxx_tick.c` | 实现 HAL_Tick_Drv_t 或用外部注入 | 必须 |
| ✅ USB 驱动 | `bsp/xxx/xxx_usb.c` | 实现 HAL_Usb_Drv_t | 必须 |
| ✅ PWR 驱动 | `bsp/xxx/xxx_pwr.c` | 实现 HAL_Pwr_Drv_t | 必须 |
| ✅ 分区配置 | `upgrade_config.h` | 三个工程保持一致 | 必须 |
| ⬜ 串口 | `board_config.h` | USB_UPGRADE_USE_UART | 可选 |
| ⬜ LED | `board_config.h` | USB_UPGRADE_USE_LED | 可选 |
| ⬜ 兼容函数 | `board_config.h` | USB_UPGRADE_COMPAT_FUNCTIONS | 可选 |

---

## 8. 支持的芯片

### 8.1 已支持

| 芯片系列 | 平台宏 | 状态 |
|----------|--------|------|
| STM32F103 | `CHIP_SERIES_F103` | ✅ 完全支持 |
| STM32F407 | `CHIP_SERIES_F407` | ⚠️ 需要适配 |

### 8.2 待支持

| 芯片系列 | 平台宏 | 状态 |
|----------|--------|------|
| GD32F450 | `CHIP_SERIES_GD32F450` | 🔲 需要移植 |
| GD32F303 | `CHIP_SERIES_GD32F303` | 🔲 需要移植 |
| AT32F403A | `CHIP_SERIES_AT32F403A` | 🔲 需要移植 |
| CH32V307 | `CHIP_SERIES_CH32V307` | 🔲 需要移植 |

### 8.3 添加新芯片支持

1. 在 `bsp/` 下创建平台目录
2. 实现所有 HAL 驱动
3. 在 `hal_config.h` 添加平台宏
4. 在 `st_platform.h` 或新平台头文件中包含芯片 SDK

---

## 9. 安全机制

### 9.1 掉电保护

```
┌─────────────────────────────────────────────────────────────────┐
│                    升级流程（掉电安全）                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  APP 阶段（写入 Slot A，不碰内部 Flash）                        │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ 1. 擦除 Slot A                                           │   │
│  │ 2. 写入固件到 Slot A                                     │   │
│  │ 3. FlushCache() ← 确保数据落盘                          │   │
│  │ 4. 校验固件头（MSP 检查）                                │   │
│  │ 5. 设置 UPGRADE_FLAG_YES + 固件大小                      │   │
│  │ 6. 设置 UPGRADE_STATE_DONE                               │   │
│  │ 7. 重启                                                  │   │
│  └──────────────────────────────────────────────────────────┘   │
│           ↓ 掉电任何一步都安全（下次重启重来）                   │
│                                                                 │
│  Bootloader 阶段（搬运固件）                                    │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ 1. 读取 UPGRADE_FLAG_ADDR                                │   │
│  │ 2. flag == YES → 继续                                    │   │
│  │ 3. 读取固件大小，合法性检查                               │   │
│  │ 4. 擦除内部 Flash APP 区                                 │   │
│  │ 5. 从 Slot A 搬运到内部 Flash                            │   │
│  │ 6. 清除升级标志 (FLAG_NO)                                │   │
│  │ 7. 写入 UPGRADE_STATE_DONE                               │   │
│  │ 8. 跳转 APP                                              │   │
│  └──────────────────────────────────────────────────────────┘   │
│           ↓ 掉电：flag 还在，下次重启重新搬运                    │
│                                                                 │
│  APP 启动阶段（确认存活）                                       │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ 1. Upgrade_StateCheck() 读取状态                         │   │
│  │ 2. state == DONE → 清 flag，写 CONFIRMED                 │   │
│  │ 3. state == CONFIRMED → 清 flag，写 NONE                 │   │
│  └──────────────────────────────────────────────────────────┘   │
│           ↓ 防止死循环（PC 重新同步 firmware.bin）               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 9.2 安全校验清单

| 校验项 | 位置 | 说明 |
|--------|------|------|
| 固件头校验 | `upgrade.c` | MSP 是否在 SRAM 范围 |
| 固件大小校验 | `source_usb_drag.c` | 256B < size <= MAX_SIZE |
| Bootloader 大小校验 | `bootloader` | fw_size <= APP_SIZE |
| 栈顶校验 | `bootloader` | MSP 在 0x20000000~0x20020000 |
| 状态机保护 | `upgrade.c` | 防止重复执行 |
| FlushCache | `upgrade.c` | 校验前确保数据落盘 |
| cleanup | `usb_task.c` | 升级后删除固件防重复 |

### 9.3 掉电场景分析

| 掉电时机 | 恢复策略 |
|----------|----------|
| 写入 Slot A 中途 | 下次重启，flag 还是 NO，不触发升级 |
| 设置 flag 后掉电 | Bootloader 检测到 flag=YES，重新搬运 |
| Bootloader 搬运中掉电 | flag 还在，下次重启重新搬运 |
| Bootloader 写 DONE 后掉电 | APP 启动检测到 DONE，清除 flag |
| APP 启动后掉电 | 下次启动重新检查状态机 |

---

## 10. 常见问题

### 10.1 USB 无法识别

**可能原因**：
1. USB 时钟未使能
2. USB 引脚配置错误
3. CherryUSB 描述符配置错误

**解决方法**：
1. 检查 `RCC_APB1Periph_USB` 是否使能
2. 检查 USB 引脚（PA11/PA12）配置
3. 检查 `usb_msc_device.c` 中的 VID/PID

### 10.2 固件写入失败

**可能原因**：
1. SPI Flash 通信失败
2. Flash 未初始化
3. 地址配置错误

**解决方法**：
1. 检查 SPI 引脚配置
2. 确保 `HAL_Flash->init()` 已调用
3. 检查 `upgrade_config.h` 中的地址配置

### 10.3 升级后无法启动

**可能原因**：
1. 固件烧录到错误地址
2. 固件损坏
3. 向量表偏移错误

**解决方法**：
1. 检查 `APP_ADDR` 配置
2. 重新编译固件
3. 检查 APP 工程的向量表偏移设置

### 10.4 重复升级

**可能原因**：
1. cleanup 未执行
2. 固件文件未删除

**解决方法**：
1. 检查 `usb_task.c` 中的 cleanup 逻辑
2. 添加调试打印观察 cleanup 结果

### 10.5 uCOS-II 下死锁

**可能原因**：
1. 任务优先级配置不当
2. 互斥锁超时设置过短

**解决方法**：
1. 调整任务优先级
2. 增加互斥锁超时时间

---

## 11. API 参考

### 11.1 对外接口

```c
/**
 * @brief 初始化升级模块
 * 
 * 内部自动完成：Flash 初始化、FatFS 挂载、USB MSC 初始化
 * 调用一次即可，放在初始化阶段
 */
void USB_Upgrade_Init(void);

/**
 * @brief 运行升级任务
 * 
 * 每次主循环调用一次，内部处理：
 * - USB 连接/断开检测
 * - 固件文件检测
 * - 固件搬运到 Slot A
 * - 设升级标志并重启
 */
void USB_Upgrade_Run(void);
```

### 11.2 内部接口（高级用户）

```c
/* 升级状态机 */
void          Upgrade_Init(void);
UpgradeResult Upgrade_Check(void);
UpgradeResult Upgrade_Execute(void);
void          Upgrade_SetFlag(uint32_t flag, uint32_t size);
uint32_t      Upgrade_GetFlag(void);
UpgradeState  Upgrade_GetState(void);
UpgradeResult Upgrade_GetLastError(void);
uint32_t      Upgrade_GetFirmwareSize(void);

/* Flash 服务 */
void     FlashService_Init(void);
void     FlashService_Read(uint8_t *buf, uint32_t addr, uint32_t len);
void     FlashService_Write(const uint8_t *buf, uint32_t addr, uint32_t len);
void     FlashService_EraseSector(uint32_t addr);
void     FlashService_EraseBlock(uint32_t addr);
void     FlashService_FlushCache(void);
void     FlashService_InvalidateCache(void);

/* FatFS */
FRESULT FatFs_Mount(void);
void    FatFs_Unmount(void);
FRESULT FatFs_Format(void);
```

### 11.3 错误码

```c
typedef enum {
    UPGRADE_OK = 0,             // 成功
    UPGRADE_ERR_NO_FILE,        // 无固件文件
    UPGRADE_ERR_CHECK,          // 校验失败
    UPGRADE_ERR_BURN,           // 烧录失败
    UPGRADE_ERR_STATE           // 状态错误
} UpgradeResult;
```

### 11.4 升级状态

```c
typedef enum {
    UPGRADE_IDLE = 0,           // 空闲
    UPGRADE_CHECKING,           // 检查中
    UPGRADE_READY,              // 就绪
    UPGRADE_BURNING,            // 烧录中
    UPGRADE_DONE,               // 完成
    UPGRADE_ERROR               // 错误
} UpgradeState;
```
