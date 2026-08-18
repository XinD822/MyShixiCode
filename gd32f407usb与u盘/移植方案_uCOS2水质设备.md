# GD32F407 U盘升级模块移植方案（uCOS-II 水质检测设备）

> 版本：v1.0（2026-08-13）
> 目标：把当前 GD32F407 的 U 盘升级 / 拖拽升级模块，以**强可移植性**方式移植到公司成熟的 uCOS-II 水质检测设备上，同时支持裸机 / uCOS 双环境编译切换。

---

## 一、项目背景与移植目标

### 1.1 现状

- 当前工程：`f407_upgrade_gd`（GD32F407，裸机），已实现 **U 盘升级（Host）+ 拖拽升级（Device）** 双模式；
- 外部 W25Q128（16MB）分区：Slot A / Slot B / 数据区(FatFS) / 配置区；
- 升级链路：固件写 Slot A → MD5 校验（搬运前 + 搬运后）→ Bootloader 备份旧固件到 Slot B → 搬运内部 Flash → 失败回滚；
- 内部 Flash：Bootloader（64KB）+ APP（448KB）。

### 1.2 移植目标

| 目标 | 说明 |
|---|---|
| 目标 MCU | GD32F407（与现工程相同，平台层可复用） |
| 目标 OS | **uCOS-II**（设备已有） |
| 运行模式 | **uCOS / 裸机 编译期一键切换** |
| USB Host 栈 | 与当前 GD USB Host 栈一起移植 |
| 升级占用策略 | 可接受升级期间暂停业务任务 |
| 外部 Flash | 同型号 W25Q128，**分区可能按 OTA 融合重新分配** |
| FatFS | 设备已有 FatFS，**融合而非携带** |

### 1.3 核心原则

1. **L1 升级核心零平台依赖**：upgrade/md5/firmware_check 不 include 任何芯片/OS 头；
2. **平台差异全部收敛到 L2 抽象层 + L3 实现层**；
3. **一个宏切换 uCOS/裸机**（`PLATFORM_UCOS2`）；
4. **FatFS 单例原则**：全工程只保留一份 ff.c，融合到设备已有实例；
5. **配置单源**：分区、固件格式、标志语义只定义一处（common/）。

---

## 二、现状评估：耦合点清单

### 2.1 与 GD32 硬件强耦合（必须抽象）

| 文件 | 耦合点 | 处理 |
|---|---|---|
| `bsp/platform.c` | `RCU_AHB2RST`、`USBFS_IRQn`、`nvic_irq_*`、跳 Bootloader | 下沉 L3 |
| `usb_app/usbh_usr.c` | `usbh_host`/`usb_core_driver`/`usbh_msc_*` | 最大耦合块，包 `plat_usbhost` |
| `usb_app/usb_hw.c` | `usb_rcu_config`/`usb_gpio_config` | 下沉 L3 |
| `bsp/tick_drv.c` | TIMER2 + SysTick | 替换为 `plat_tick`（见第五章） |
| `bsp/w25q128_drv.c` | 软件 SPI 直操 GPIO | 包 `plat_flash` |
| `src/usb_host_task.c` | `usbh_init`/`usbh_class_register`/`usbh_core_task` | 随 USB 栈下沉 |

### 2.2 与裸机调度强耦合（uCOS 适配点）

- `flash_service.c` 已有 `CONFIG_USB_USE_UCOS2` 双分支互斥（可复用为 `plat_mutex`）；
- `Tick_*` 遍布全工程（超时判断 / 延时），需替换为 `plat_tick`；
- 升级流程是秒级长阻塞（擦除 150ms~2s），uCOS 下需独立任务运行。

### 2.3 零平台依赖（原样带走）

- `src/upgrade.c`：通过 `UpgradeSource_t` + `FlashService_*` 抽象，零硬件依赖；
- `src/upgrade_source.h`：来源抽象接口；
- `src/md5.c/h`：纯 C；
- `src/firmware_check.c`：纯 C。

### 2.4 可压缩 / 删除项

| 项 | 理由 |
|---|---|
| `Upgrade_ExecuteBuf` | 已无调用方，删除 |
| `source_usb_drag.c` + `source_usb_host.c` | 仅卷号不同，合并为参数化实现 |
| 引导扇区 dump 等诊断 | 收敛到 `plat_log` 分级日志 |
| FatFS `ffunicode.c` | `FF_USE_LFN=0`，裁剪 |
| Bootloader/App 重复的 `upgrade_config.h` + `md5` | 抽 `common/`，单一来源 |

---

## 三、目标架构：四层

```
┌────────────────────────────────────────────────────────────┐
│ L1 升级核心（零平台依赖，不改一行）                          │
│   upgrade.c / upgrade_source.c/h / firmware_check.c/h       │
│   md5.c/h / upgrade.h                                       │
└──────────────┬─────────────────────────────────────────────┘
               │ 只调用 L2 接口
┌──────────────┴─────────────────────────────────────────────┐
│ L2 平台抽象接口（纯头文件 + 桩实现）                         │
│   plat_flash.h / plat_tick.h / plat_mutex.h                │
│   plat_usbhost.h / plat_log.h / plat_common.h              │
└──────────────┬─────────────────────────────────────────────┘
               │
┌──────────────┴─────────────────────────────────────────────┐
│ L3 具体平台实现（GD32F407 + uCOS-II 适配）                   │
│   w25q128_drv / tick_* / platform_gd32                     │
│   usb_app(GD USB Host 栈) / mutex_ucos2 / log_*            │
└──────────────┬─────────────────────────────────────────────┘
               │
┌──────────────┴─────────────────────────────────────────────┐
│ L4 目标设备整合（uCOS-II）                                  │
│   升级任务封装 / 业务暂停机制 / 内存规划                     │
└────────────────────────────────────────────────────────────┘
```

---

## 四、L2 平台抽象接口定义

### 4.1 plat_common.h —— 平台统一开关（移植唯一要改的配置）

```c
/* platform_config.h —— 整个工程唯一的平台开关 */
#define PLATFORM_BARE_METAL    0
#define PLATFORM_UCOS2         1
#define PLATFORM_SELECT        PLATFORM_UCOS2    /* ← 移植时只改这一行 */
```

所有 L3 实现以 `#if (PLATFORM_SELECT == PLATFORM_UCOS2)` 分支选编。
**L1 业务代码（upgrade/md5/firmware_check）完全不知道这个宏存在。**

### 4.2 plat_tick.h —— 时间抽象（详见第五章）

```c
void     plat_tick_init(void);          /* uCOS 模式可为空 */
uint32_t plat_get_tick_ms(void);        /* 单调毫秒（超时判断） */
void     plat_delay_ms(uint32_t ms);    /* 阻塞延时（uCOS 下让出 CPU） */
/* 注意：Tick_Poll() 不再暴露给业务层 */
```

### 4.3 plat_mutex.h —— 互斥抽象

```c
typedef struct plat_mutex_s plat_mutex_t;
void  plat_mutex_init(plat_mutex_t *m);
int   plat_mutex_lock(plat_mutex_t *m, uint32_t timeout_ms);
void  plat_mutex_unlock(plat_mutex_t *m);
/* 裸机实现 = 关中断原子操作（现有 SvcMutex 裸机分支）
 * uCOS 实现 = OSMutexCreate/Pend/Post（现有 SvcMutex uCOS 分支） */
```

### 4.4 plat_flash.h —— SPI NOR 抽象

```c
uint8_t  plat_flash_init(void);
uint16_t plat_flash_read_id(void);
void     plat_flash_read(uint8_t *buf, uint32_t addr, uint32_t len);
void     plat_flash_write_nocheck(const uint8_t *buf, uint32_t addr, uint32_t len);
void     plat_flash_erase_sector(uint32_t addr);   /* 4KB */
void     plat_flash_erase_block(uint32_t addr);    /* 64KB */
```

### 4.5 plat_usbhost.h —— USB Host 读 U 盘抽象

```c
int   plat_udisk_ready(void);
int   plat_udisk_read_sector(uint8_t *buf, uint32_t sector, uint32_t cnt);
int   plat_udisk_get_sector_count(uint64_t *n);
```

### 4.6 plat_log.h —— 分级日志（量产可全关）

```c
typedef enum { PLAT_LOG_DBG, PLAT_LOG_INFO, PLAT_LOG_ERR } plat_log_level_t;
void plat_log(plat_log_level_t lv, const char *tag, const char *fmt, ...);
#define LOGD(...)   /* 调试级，可裁剪 */
#define LOGI(...)   /* 信息级 */
#define LOGE(...)   /* 错误级，量产保留 */
```

---

## 五、TIM 解耦最简方案（plat_tick）

### 5.1 核心洞察：uCOS-II 自带 tick，uCOS 模式零 TIM 占用

uCOS-II 运行**必然**需要系统节拍（`OS_TICKS_PER_SEC`，由 SysTick 或某个定时器中断驱动 `OSTimeTick()`）。
目标水质设备上**必然已经存在一个正在跑的 tick 源**。因此：

| 运行模式 | tick 来源 | 额外 TIM 需求 |
|---|---|---|
| **uCOS-II** | 直接用 OS 自带 tick（`OSTimeGet`/`OSTimeDly`） | **零占用** |
| 裸机（调试/移植期） | SysTick（Cortex-M 内核自带） | **零外设占用** |

**结论**：不存在"为升级模块新开 TIM"的必要，也不存在"往已有 TIM 塞复杂逻辑"的问题。
uCOS 用 OS tick、裸机用 SysTick，两条路都是零外设占用。

### 5.2 实现：一个宏 + 两个函数 + 一个文件

```c
/* plat_tick.c —— 一个文件、两个 #if 分支 */
#if (PLATFORM_SELECT == PLATFORM_UCOS2)
void plat_tick_init(void) { /* 无需初始化，OS 已跑 */ }
uint32_t plat_get_tick_ms(void) { return OSTimeGet() * (1000u / OS_TICKS_PER_SEC); }
void plat_delay_ms(uint32_t ms) { OSTimeDly((ms * OS_TICKS_PER_SEC) / 1000u); }
#else
/* 裸机：SysTick 1ms 中断里 g_tick_ms++（或复用已有 1ms 定时器，仅加一行） */
static volatile uint32_t g_tick_ms;
void plat_tick_init(void) { /* SysTick 1ms 配置 */ }
uint32_t plat_get_tick_ms(void) { return g_tick_ms; }
void plat_delay_ms(uint32_t ms) { while (ms--) plat_delay_us(1000); }
#endif
```

### 5.3 业务代码替换清单（机械替换）

| 旧接口 | 新接口 | 处数 |
|---|---|---|
| `Tick_Init()` | `plat_tick_init()` | 1（usb_upgrade.c） |
| `Tick_GetMs()` | `plat_get_tick_ms()` | ~8（flash_service/usb_task/usb_host_task/usbd_storage_msd） |
| `Tick_DelayMs()` | `plat_delay_ms()` | ~6（usb_host_task/usb_task） |
| `Tick_Poll()` | 删除（uCOS 模式不需要） | 从业务循环移除 |

### 5.4 移植注意点

1. **tick 周期确认**：若设备 uCOS tick 是 10ms，`plat_delay_ms(1)` 会变 10ms，U 盘枚举/读变慢但功能正常（USB 超时通常 500ms~5s 级）；建议设备 tick 为 1ms，或 Host 轮询间隔对齐 tick；
2. **`Tick_Poll` 必须从业务层消失**：uCOS 下无人调轮询，残留会导致 `g_tick_ms` 永远为 0 → 所有超时失效；
3. **互斥超时语义**：uCOS 下 `OSMutexPend` 自带 timeout（tick 数），`plat_mutex` 直接用 OS 语义，不依赖时间戳。

---

## 六、uCOS / 裸机双后端切换

### 6.1 切换矩阵

| 平台层 | 裸机实现 | uCOS 实现 | 切换机制 |
|---|---|---|---|
| tick | SysTick（或复用已有 1ms 定时器） | `OSTimeGet/OSTimeDly` | 宏 |
| mutex | 关中断原子操作 | `OSMutexPend/Post` | 宏（已有双分支，复用） |
| log | 串口直出 | 串口直出 / 邮箱转发 | 宏 |
| 任务调度 | 主循环 `while(1){ Run() }` | `OSTaskCreate` 任务 | 文件级选编 |
| flash 服务 | 裸机直接调 | 同上（已走互斥） | 宏 |
| USB Host | 主循环 `usbh_core_task` | 升级任务内部轮询 | 随任务层 |

### 6.2 任务调度层（差异最大，单独设计）

```
裸机（main）：
  while(1) {
      plat_tick_poll();        /* 喂毫秒计数（仅裸机） */
      USB_Mode_Run();          /* DEVICE/HOST 分派 + 升级检测 */
  }

uCOS：
  OSInit();
  OSTaskCreate(Upgrade_Task, ...);   /* 升级任务：内部含 USB_Mode_Run 等价逻辑 */
  OSStart();
```

业务逻辑（升级状态机、USB 任务推进）抽成**纯函数**（`Upgrade_Step()` / `USB_Mode_Run()`），
两种调度只是"怎么调用它"的区别。设计为两个入口文件：
- `upgrade_bare.c`：`Upgrade_BareMain()`（主循环 + Tick_Poll）；
- `upgrade_ucos.c`：`Upgrade_Task(void *arg)`（OSTimeDly 轮询 + 业务暂停标志）。

---

## 七、FatFS 融合方案（设备已有 FatFS，融合而非携带）

### 7.1 核心矛盾：FatFS 是"单例库"

`ff.c` 内部的卷表、目录缓冲、窗口缓冲全是文件内静态全局
（`FATFS FatFs[FF_VOLUMES]`、`win[]`、`dir[]` 等）。两个工程各带一份 ff.c 链接到一起
会导致两个文件系统实例各自管卷表，而 diskio 底层是单例——**必然冲突**。

> **铁律：全工程（设备 + 升级模块）只保留一份 ff.c/ff.h/ffconf.h，
> diskio.c 合并成一份，卷号统一分配。**

### 7.2 融合点清单

| 融合点 | 我的现状 | 融合动作 |
|---|---|---|
| FatFS 版本 | 接口较新（R0.14+，有 `MKFS_PARM`/`LBA_t`） | 与设备统一到高版本 |
| ffconf.h | `FF_VOLUMES=2`、`FF_USE_MKFS=1`、`FF_USE_LFN=0` | 求并集合并；`FF_VOLUMES` 按最终卷数调 |
| 卷号分配 | pdrv0=本地数据区、pdrv1=U盘 | 重新排卷号表（见 7.4） |
| diskio.c | 双卷实现（Flash + USB） | **合并成一份**，按 pdrv 分发 |
| fatfs_system.c 封装 | 带"无文件系统自动格式化" | **必须降级**：绝不自动格式化设备卷，只报错 |
| 线程安全 | `FF_FS_LOCK=0` | 开 `FF_FS_REENTRANT=1` + 4 钩子（见 7.3） |
| 暂存策略 | 固件先拷入本地数据区再写 Slot A | 与设备协商划"升级暂存目录"（如 `/upgrade/firmware.bin`） |

### 7.3 FatFS 官方 RTOS 集成机制（标准答案，零自研）

FatFS **官方自带** RTOS 支持（本机 ffconf.h 已确认），不需要自己发明：

```c
#define FF_FS_REENTRANT  1
#define FF_FS_TIMEOUT    1000
#define FF_SYNC_t        OS_EVENT *      /* uCOS-II 用 OSMutex 做同步对象 */
```

开启后只需实现 4 个钩子（官方样例在 FatFS 的 `option/syscall.c`，uCOS 版直接照抄）：

| 钩子 | uCOS-II 实现 |
|---|---|
| `ff_cre_syncobj()` | `OSMutexCreate` |
| `ff_req_grant()` | `OSMutexPend` |
| `ff_rel_grant()` | `OSMutexPost` |
| `ff_del_syncobj()` | `OSMutexDel` |

### 7.4 卷号分配建议（需设备确认）

| pdrv | 用途 | 归属 |
|---|---|---|
| 0 | 设备现有存储（SD/内部 Flash，不动） | 设备 |
| 1 | 本地升级数据区（SPI Flash，若允许占用） | 升级模块 |
| 2 | USB U 盘 | 升级模块 |

`FF_VOLUMES` 对应调到 3。设备原有代码零改动，升级模块只申请 1、2 号卷。

### 7.5 融合实施步骤

1. **盘点设备 FatFS**：版本、ffconf、卷号使用、diskio 底层、是否已线程安全；
2. **配置合并**：定 ffconf 最终值 + 卷号表 + 暂存目录约定 → 出《FatFS 融合协议》；
3. **合 diskio**：U 盘卷的 `disk_read/write/ioctl/status/initialize`（约 40 行）并入设备 diskio.c；
4. **改封装**：`fatfs_system.c` 消费设备挂载体系，移除自动格式化；
5. **线程安全**：`FF_FS_REENTRANT=1` 4 钩子对接 uCOS 信号量；
6. **验证**：设备原有文件功能不回归 + 升级全流程通过。

---

## 八、压缩简化清单

| 动作 | 对象 | 说明 |
|---|---|---|
| 删除 | `Upgrade_ExecuteBuf` | 已无调用方 |
| 合并 | `source_usb_drag.c` + `source_usb_host.c` | 仅卷号不同 → 参数化 `upgrade_source_file.c` |
| 抽公共 | Bootloader/App 重复的 `upgrade_config.h` + `md5.c/h` | 放入 `common/`，两边同一份 |
| 裁剪 | FatFS `ffunicode.c` | `FF_USE_LFN=0`，不需要 |
| 收敛 | 引导扇区 dump、`[SCAN]` 类诊断 | 并入 `plat_log`，量产可全关 |
| 下沉 | `usb_app/`（GD USB 栈）、`bsp/`、`tick_drv` | 归 L3，不进 L1 |
| 精简 | `board_config.h` | 拆分：芯片/引脚留 L3，升级策略留 `upgrade_config.h` |

**预计 L1 核心压到 6~8 个源文件**，其余全部在 L2/L3。

---

## 九、分区重排（与同事 OTA 融合）的严谨做法

因为分区"可能按 OTA 融合重新分配"，方案内置抗重排机制：

1. **单一配置源**：`upgrade_config.h` 抽到 `common/`，Bootloader、App、同事的 OTA 工程都 `#include` 同一份——重排只改一处；
2. **分区表版本号**：配置区加 `PARTITION_VERSION` 字段（4B），Bootloader 启动时校验——分区变了能识别并提示"需重新烧录"而非误读旧数据；
3. **与同事对齐协议**（建议单独开一次对齐会）：
   - 分区布局：Slot A/B 地址与大小、数据区、配置区的**绝对约定**；
   - 固件格式：MD5 存放位置（现按配置区方案 A，需与同事确认其 MD5 位置）；
   - 升级标志语义：flag/state/size 的值定义双方一致；
4. **重排迁移**：分区地址变化后，旧设备配置区/固件区数据全部失效，**必须 Bootloader+App 同时烧录**；版本号机制让现场可诊断。

---

## 十、分阶段实施计划

| 阶段 | 内容 | 产出/验证 |
|---|---|---|
| 1 解耦重构 | 抽 `common/`、建 `plat_*` 接口、合并 source、删 ExecuteBuf、裁 FatFS | 裸机版编译通过，**功能回归不变**（U盘/拖拽/回滚各测一遍） |
| 2 FatFS 融合 | 与设备 FatFS 合并（需设备信息，双向协调） | 《FatFS 融合协议》+ 设备编译通过 |
| 3 uCOS 集成 | `plat_mutex/tick/log` uCOS 实现、升级任务封装、业务暂停标志、USB 栈接入 | 目标设备工程编译通过 |
| 4 联调验证 | 真机：U 盘升级、断电重试、回滚、与业务任务共存 | 全流程日志验证 + 稳定性 |

> **阶段 1 是纯重构（行为不变），可在本工程内先做并验证，不影响现有可用状态——最安全的起点。**

---

## 十一、风险与对策

| 风险 | 对策 |
|---|---|
| 设备 USB Host 栈与 GD 库不同 | `plat_usbhost` 隔离，重写 L3 即可，L1 不动 |
| 升级长阻塞饿死业务任务 | 已确认"可接受停业务"→ 升级封装为独占式低优先级任务，业务任务轮询到升级标志即挂起 |
| 定时器资源冲突 | `plat_tick` uCOS 模式用 OS tick、裸机用 SysTick，**零外设占用** |
| 两份配置漂移 | `common/` 单源，编译期单一引用 |
| FatFS 与设备现有文件系统冲突 | 卷号/扇区访问收敛到 `diskio.c`，接入时确认卷映射；禁止自动格式化设备卷 |
| 分区重排导致旧数据错位 | `PARTITION_VERSION` 版本号 + Bootloader/App 同时烧录 |
| `Tick_Poll` 残留 | 从业务层彻底移除，计数由后端保证 |

---

## 十二、待确认清单（实施前需要设备侧提供）

1. FatFS 版本号、完整 `ffconf.h` 配置；
2. 现有卷号分配和底层介质（SD？SPI Flash？多大？）；
3. 是否已开启 `FF_FS_REENTRANT` / `FF_FS_LOCK`；
4. 设备数据区能否划一块给"升级暂存"（firmware.bin 中转）；
5. 设备是否需要保留"拖拽升级（MSC 虚拟 U 盘）"——与设备现有 USB DEVICE 功能是否冲突；
6. 设备 uCOS tick 周期（1ms 还是 10ms，影响 Host 轮询间隔）；
7. 分区表最终方案（与同事 OTA 对齐后的绝对地址）；
8. 设备 uCOS-II 具体版本（接口细节对齐）。

---

## 附：关键决策记录

| 决策点 | 结论 |
|---|---|
| MD5 存放位置 | 配置区方案 A（`FIRMWARE_MD5_A_ADDR`=0xFE2000、`_B`=0xFE3000，独立扇区） |
| 回滚大小 | 统一 APP_SIZE + MD5_B 校验 |
| 校验链 | 搬运前 Slot A vs MD5_A + 搬运后内部 Flash vs MD5_A + MSP 头 |
| uCOS/裸机切换 | `PLATFORM_SELECT` 一个宏 |
| TIM 策略 | uCOS 用 OS tick、裸机用 SysTick，零外设占用 |
| FatFS | 官方 `FF_FS_REENTRANT` + 4 钩子，融合设备实例 |
