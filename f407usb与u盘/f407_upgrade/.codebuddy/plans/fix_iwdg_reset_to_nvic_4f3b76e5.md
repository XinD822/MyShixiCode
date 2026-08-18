---
name: fix_iwdg_reset_to_nvic
overview: 将 APP 中所有 IWDG 硬件看门狗复位替换为 NVIC_SystemReset() 内核软件复位，解决 device↔host 切换、U盘升级跳转时因 LSI 未使能导致 IWDG 复位不生效、程序卡死必须重烧的问题。
todos:
  - id: replace-reset
    content: 重写 platform.c 的 Platform_SystemReset，用 NVIC_SystemReset 替换 IWDG 复位
    status: completed
  - id: verify-callsites
    content: 核对 main.c/usb_host_task.c/usb_task.c 调用点无需改动且标志已 Flush
    status: completed
    dependencies:
      - replace-reset
  - id: build-verify
    content: 编译工程，确认 NVIC_SystemReset 链接无误、无残留 IWDG 引用
    status: completed
    dependencies:
      - replace-reset
---

## 用户需求

升级过程跳转不流畅：在 device 切换到 host 的位置、以及 U 盘升级需要跳转到 bootloader 的位置，每次跳转后板子完全卡死无响应，必须重新烧录 bootloader（等价于强制复位）才能继续。

## 产品概述

STM32F407 的 USB 升级 APP（位于内部 Flash 0x08010000），复位后由 bootloader（0x08000000）校验并跳回 APP，APP 依据 W25Q128 中的 Flash 标志决定以 DEVICE（拖拽升级）或 HOST（U 盘升级）模式运行。所有"模式切换 / 升级跳转"均通过一次系统复位续跑。

## 核心功能

- 将不可靠的 IWDG 硬件看门狗复位替换为 NVIC_SystemReset() 内核软件复位，使 device↔host 模式切换、U 盘升级完成跳转两处流程在复位后自动重启续跑，无需重新烧录。
- 保留 W25Q128 软件复位（Flash_Reset）逻辑，仅替换复位触发方式。
- 复位后由 bootloader 正常重新初始化时钟并跳回 APP，模式由 Flash 标志决定，行为与原设计一致。

## 技术栈

- 芯片：STM32F407ZG（ARM Cortex-M4）
- 工程：Keil MDK（USER/KEY.uvprojx），APP 链接地址 0x08010000
- 库：标准外设库（FWLIB）+ CMSIS（core_cm4.h，已通过 stm32f4xx.h 间接包含，提供 NVIC_SystemReset）
- 语言：C

## 实现方案

### 方法

统一修改复位入口 `Platform_SystemReset()`，将内部基于 IWDG 的复位序列改为调用 CMSIS 的 `NVIC_SystemReset()`。

### 工作原理

`NVIC_SystemReset()` 通过写 SCB->AIRCR 的 SYSRESETREQ 位触发内核级软件复位，不依赖 LSI 低速时钟，复位可靠且即时。原 IWDG 方案依赖 LSI 使能，LSI 未就绪时 `IWDG->KR=0xCCCC` 启动失败，程序卡死在 `while(1)`，导致"必须重烧"。

### 关键技术决策

1. **仅改 `Platform_SystemReset()` 一处**：所有复位调用点（main.c:81、usb_host_task.c:187、usb_task.c:88）均经此函数，集中修改即可覆盖全部跳转场景，符合 DRY 原则，且对外接口不变。
2. **保留 OTG 中断/时钟关闭**：调用 `NVIC_DisableIRQ(OTG_FS_IRQn)` 与关闭 OTG_FS 时钟，清理外设状态，避免复位瞬间产生悬挂中断；bootloader 会重新初始化，无副作用。
3. **不再切 RCC 回 HSI**：NVIC 软件复位为热复位，不重置 RCC，但 bootloader 启动时会从 HSI 重新配置系统时钟，因此无需在 APP 侧预切时钟（原 IWDG 方案因不依赖 AHB 才需要预切）。这反而更简单可靠。
4. **Flash_Reset() 在调用点保留**：W25Q128 软件复位序列（0x66+0x99）与软件复位方式无关，继续复位 SPI Flash 内部状态机，调用点（main.c、usb_host_task.c）不需要改。

### 性能与可靠性

- `NVIC_SystemReset()` 为单条内核写操作 + 总线同步，复位延迟在毫秒级，确定性高，无 LSI 启动超时风险。
- 不引入额外资源占用；原 IWDG 相关代码（platform.c 中 IWDG 寄存器操作、注释）整体移除，减少维护面。
- 经检索，APP 侧从未独立启动过 IWDG（仅在 platform.c 与库头出现），进一步佐证 LSI 可能未使能，IWDG 复位不可靠。

## 实现注意事项

- 必须包含 `core_cm4.h`（已通过 stm32f4xx.h 间接包含，无需新增 include），`NVIC_SystemReset()` 为 CMSIS 标准 API。
- 函数内不要再有任何 DBG_PRINTF 之后的阻塞等待（原注释要求切时钟后禁打印）；改为 NVIC 复位后内核会立即复位，无需等待。
- 复位前确保关键标志（USB 模式标志、升级标志）已写入 W25Q128 并 FlushCache（调用点已做），复位后 bootloader/APP 才能正确读取。
- 保持 `Platform_SystemReset()` 函数签名不变，调用点零改动。

## 架构设计

```
[APP 运行]
   │
   ├─ KEY0 长按切换模式 ──┐
   ├─ U盘升级完成 ────────┼─► Platform_SystemReset() ─► NVIC_SystemReset()
   └─ 拖拽升级完成 ───────┘            (改后)
                                              │
                                       CPU 从 0x08000000 启动
                                              │
                                        bootloader 校验 APP
                                              │
                                        JumpToApp() 跳回 0x08010000
                                              │
                                        APP 读 Flash 标志 → DEVICE/HOST
```

修改仅发生在 `Platform_SystemReset()` 内部，不改变整体架构与数据流。

## 目录结构

```
f407_upgrade/
└── usb_upgrade/
    └── bsp/
        └── platform.c   # [MODIFY] 重写 Platform_SystemReset()：移除 IWDG 复位序列（IWDG->KR 操作、RCC 回 HSI 切换），改为关闭 OTG_FS 中断与时钟后直接调用 NVIC_SystemReset()；更新注释说明不再依赖 LSI。其余函数（Platform_DisableIRQ/SetMSP/SetVTOR 等）不变。
```

其余调用文件（USER/main.c、usb_upgrade/src/usb_host_task.c、usb_upgrade/src/usb_task.c）经统一入口调用，无需修改，但需在验证阶段确认其行为不受影响。

## 关键代码结构

`Platform_SystemReset()` 新实现（接口级，无实现体细节）：

```c
void Platform_SystemReset(void)
{
    /* 1) 关 OTG_FS 中断与时钟，清理外设悬挂状态 */
    NVIC_DisableIRQ(OTG_FS_IRQn);
    RCC->AHB2ENR &= ~RCC_AHB2Periph_OTG_FS;

    /* 2) 内核软件复位（不依赖 LSI，可靠触发） */
    NVIC_SystemReset();   /* 来自 core_cm4.h / CMSIS */
    /* 不会返回 */
}
```