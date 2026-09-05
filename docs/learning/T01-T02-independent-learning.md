# PanView T01/T02 独立学习文档

> 适用对象：希望脱离完整 PanView 工程，单独理解 FreeRTOS 基础任务、心跳监测和资源观察的学习者。
>
> 文档范围：只覆盖 T01 和 T02，不包含视觉接收、PID、电机、TFT、触摸和音频业务。

## 1. 任务需求说明

### 1.1 T01：最小 BSP 与日志出口

#### 任务目标

T01 要解决的问题是：先证明 MCU 的基础运行环境和 FreeRTOS 调度链路是可靠的，再开始编写复杂业务。

需要验证的链路是：

```text
硬件启动
    -> HAL 毫秒时基可用
    -> FreeRTOS 内核启动
    -> 任务被调度
    -> 任务可以延时并再次运行
    -> 状态可以通过 LED 或串口观察
```

#### 输入

- FreeRTOS 系统 tick。
- HAL 提供的毫秒时间函数，例如 `HAL_GetTick()`。
- 可选的状态 LED 和 USART1 日志出口。

#### 输出

- 周期性运行的基础任务。
- LED 翻转或日志输出，用于证明任务仍在运行。
- 当前运行时间，供调试和日志使用。

#### 依赖的硬件/底层模块

- STM32 HAL 初始化。
- SysTick 或其他 HAL 时间基。
- FreeRTOS CMSIS-RTOS2 适配层。
- 一个 GPIO LED。
- 可选 USART1 发送函数。

### 1.2 T02：RTOS 任务骨架与运行监测

#### 任务目标

T02 要解决的问题是：不要让所有业务继续集中在一个任务中，而是先建立职责清晰的任务骨架，并确认每个任务都确实被调度。

T02 建立以下基础能力：

- 创建安全、视觉、运动、步进、输入、应用控制、UI、音频、遥测任务。
- 给每个任务分配优先级和独立栈空间。
- 用统一心跳表记录任务运行次数和最近运行时间。
- 采集任务历史最低剩余栈空间。
- 采集任务当前 RTOS 状态。
- 预留事件标志组，供后续安全故障状态使用。
- 观察 FreeRTOS 剩余 heap。

#### 输入

- 各任务自己的周期或调度事件。
- 各任务句柄。
- FreeRTOS heap、任务栈和任务状态信息。
- 后续阶段将产生的故障事件；T02 只建立事件标志组，不处理真实故障。

#### 输出

- 多个可独立运行的任务。
- 统一心跳记录：运行次数、最近 tick、最低剩余栈、任务状态。
- 剩余 heap 记录。
- 一个空的系统事件标志组。

#### 依赖的硬件/底层模块

- FreeRTOS CMSIS-RTOS2。
- `heap_4` 内存管理。
- 任务创建和任务查询 API。
- HAL 或 RTOS 系统 tick。
- T01 已建立的基础 GPIO/日志观察能力。

## 2. 解耦后的最小可阅读代码

下面的代码是学习版，不是完整 STM32 工程。它保留了 T01/T02 的核心关系，并把硬件相关部分抽象成少量接口。

### 2.1 T01：基础心跳任务

#### t01_basic_task.h

```c
#ifndef T01_BASIC_TASK_H
#define T01_BASIC_TASK_H

#include <stdint.h>

/* T01 需要的硬件适配函数类型。 */
typedef uint32_t (*T01_GetTickFunction)(void);
typedef void (*T01_ToggleLedFunction)(void);

/* T01 运行状态。 */
typedef struct
{
  uint32_t run_count;
  uint32_t last_tick;
} T01_BasicTaskStatus;

/* 保存 T01 使用的硬件适配函数。 */
typedef struct
{
  T01_GetTickFunction get_tick;
  T01_ToggleLedFunction toggle_led;
} T01_BasicTaskPort;

/* 执行一次基础心跳动作。 */
void T01_BasicTask_RunOnce(const T01_BasicTaskPort *port,
                           T01_BasicTaskStatus *status);

#endif
```

#### t01_basic_task.c

```c
#include "t01_basic_task.h"

void T01_BasicTask_RunOnce(const T01_BasicTaskPort *port,
                           T01_BasicTaskStatus *status)
{
  /* 调用方必须提供有效的接口和状态地址。 */
  if ((port == 0) || (status == 0) || (port->get_tick == 0))
  {
    return;
  }

  /* 记录本次任务运行时刻。 */
  status->last_tick = port->get_tick();

  /* 记录任务已经运行的次数。 */
  status->run_count++;

  /* LED 只是观察手段，没有 LED 时可以不提供这个函数。 */
  if (port->toggle_led != 0)
  {
    port->toggle_led();
  }
}
```

### 2.2 T02：统一任务心跳和资源监测

#### t02_task_monitor.h

```c
#ifndef T02_TASK_MONITOR_H
#define T02_TASK_MONITOR_H

#include <stdint.h>

/* 学习版 RTOS 任务状态，实际工程可直接使用 osThreadState_t。 */
typedef enum
{
  T02_TASK_READY = 1,
  T02_TASK_RUNNING,
  T02_TASK_BLOCKED,
  T02_TASK_TERMINATED,
  T02_TASK_ERROR
} T02_TaskState;

/* 一个任务的全部监测数据。 */
typedef struct
{
  volatile uint32_t run_count;
  volatile uint32_t last_tick;
  volatile uint32_t stack_high_water_mark_words;
  volatile T02_TaskState state;
} T02_TaskRecord;

/* 任务在心跳表中的统一编号。 */
typedef enum
{
  T02_TASK_SAFETY = 0,
  T02_TASK_VISION_RX,
  T02_TASK_MOTION,
  T02_TASK_STEPPER,
  T02_TASK_INPUT,
  T02_TASK_APP_CONTROL,
  T02_TASK_UI,
  T02_TASK_AUDIO,
  T02_TASK_TELEMETRY,
  T02_TASK_COUNT
} T02_TaskId;

/* 更新一次任务心跳。 */
void T02_TaskMonitor_UpdateHeartbeat(T02_TaskId task_id,
                                     uint32_t current_tick);

/* 更新任务的栈余量和状态。 */
void T02_TaskMonitor_UpdateRuntime(T02_TaskId task_id,
                                   uint32_t stack_words,
                                   T02_TaskState state);

/* 读取任务监测记录。 */
const volatile T02_TaskRecord *T02_TaskMonitor_Get(T02_TaskId task_id);

/* 读取整个监测表，供调试器或遥测任务观察。 */
const volatile T02_TaskRecord *T02_TaskMonitor_GetAll(void);

#endif
```

#### t02_task_monitor.c

```c
#include "t02_task_monitor.h"

/* 只有本模块可以直接修改这张表。 */
static volatile T02_TaskRecord task_records[T02_TASK_COUNT];

void T02_TaskMonitor_UpdateHeartbeat(T02_TaskId task_id,
                                     uint32_t current_tick)
{
  if (task_id >= T02_TASK_COUNT)
  {
    return;
  }

  task_records[task_id].last_tick = current_tick;
  task_records[task_id].run_count++;
}

void T02_TaskMonitor_UpdateRuntime(T02_TaskId task_id,
                                   uint32_t stack_words,
                                   T02_TaskState state)
{
  if (task_id >= T02_TASK_COUNT)
  {
    return;
  }

  task_records[task_id].stack_high_water_mark_words = stack_words;
  task_records[task_id].state = state;
}

const volatile T02_TaskRecord *T02_TaskMonitor_Get(T02_TaskId task_id)
{
  if (task_id >= T02_TASK_COUNT)
  {
    return 0;
  }

  return &task_records[task_id];
}

const volatile T02_TaskRecord *T02_TaskMonitor_GetAll(void)
{
  return task_records;
}
```

#### t02_event_flags.h

```c
#ifndef T02_EVENT_FLAGS_H
#define T02_EVENT_FLAGS_H

#include <stdint.h>

/* 每一位代表一种独立的系统状态。 */
#define T02_EVENT_VISION_TIMEOUT (1UL << 0)
#define T02_EVENT_SOFTWARE_LIMIT (1UL << 1)
#define T02_EVENT_TASK_INACTIVE  (1UL << 2)
#define T02_EVENT_UART_ERROR     (1UL << 3)
#define T02_EVENT_EMERGENCY_STOP (1UL << 4)

/* 设置指定事件位。 */
void T02_EventFlags_Set(uint32_t flags);

/* 清除指定事件位。 */
void T02_EventFlags_Clear(uint32_t flags);

/* 读取当前全部事件位。 */
uint32_t T02_EventFlags_Get(void);

#endif
```

#### t02_event_flags.c

```c
#include "t02_event_flags.h"

/* 学习版用一个整数保存所有事件位。 */
static volatile uint32_t system_event_flags;

void T02_EventFlags_Set(uint32_t flags)
{
  system_event_flags |= flags;
}

void T02_EventFlags_Clear(uint32_t flags)
{
  system_event_flags &= ~flags;
}

uint32_t T02_EventFlags_Get(void)
{
  return system_event_flags;
}
```

> 在真实 FreeRTOS 工程中，`t02_event_flags.c` 的三个函数应由 `osEventFlagsNew()`、`osEventFlagsSet()`、`osEventFlagsClear()` 和 `osEventFlagsGet()` 实现。上面保留整数位操作，是为了先理解“多个独立状态共用一个 bit 集合”的思想。

## 3. 代码拆解讲解

### 3.1 变量说明

#### T01 变量

| 变量 | 类型 | 作用 |
|---|---|---|
| `port` | `const T01_BasicTaskPort *` | 指向硬件适配函数，不让学习模块直接依赖 HAL。 |
| `status` | `T01_BasicTaskStatus *` | 指向任务运行记录，由调用方提供存储。 |
| `get_tick` | 函数指针 | 获取当前毫秒 tick。真实工程可连接 `HAL_GetTick`。 |
| `toggle_led` | 函数指针 | 翻转 LED；只是观察手段，不是控制逻辑。 |
| `run_count` | `uint32_t` | 任务累计运行次数。 |
| `last_tick` | `uint32_t` | 最近一次运行时的时间戳。 |

#### T02 变量

| 变量 | 类型 | 作用 |
|---|---|---|
| `task_records[]` | `static volatile T02_TaskRecord[]` | 模块私有的统一任务监测表。 |
| `task_id` | `T02_TaskId` | 指定要更新或读取哪一个任务。 |
| `run_count` | `uint32_t` | 任务累计心跳次数。 |
| `last_tick` | `uint32_t` | 任务最近一次心跳时间。 |
| `stack_high_water_mark_words` | `uint32_t` | 历史最低剩余栈空间，单位是 word。F407 上 1 word 通常为 4 字节。 |
| `state` | `T02_TaskState` | 最近采集到的任务状态。 |
| `system_event_flags` | `volatile uint32_t` | 学习版事件位集合；真实工程由 FreeRTOS 事件标志组对象管理。 |

`static` 表示数据只在当前 `.c` 文件内可见，防止其他模块直接修改内部状态。`volatile` 表示数据可能被任务、中断或调试器观察，编译器不能随意缓存它的读写；它不等于互斥锁，也不保证复合操作的原子性。

### 3.2 对外公开接口

| 函数 | 调用方 | 入参 | 返回值 |
|---|---|---|---|
| `T01_BasicTask_RunOnce` | T01 任务入口或测试程序 | 硬件适配函数表、状态结构体地址 | 无；参数无效时直接返回 |
| `T02_TaskMonitor_UpdateHeartbeat` | 每个任务自身 | 任务编号、当前 tick | 无 |
| `T02_TaskMonitor_UpdateRuntime` | 遥测任务或监控任务 | 任务编号、剩余栈 word、任务状态 | 无 |
| `T02_TaskMonitor_Get` | SafetyTask、TelemetryTask、调试代码 | 任务编号 | 返回记录地址；编号非法返回空指针 |
| `T02_TaskMonitor_GetAll` | 遥测或主机测试 | 无 | 返回整张记录表地址 |
| `T02_EventFlags_Set` | 发现故障的任务或中断适配层 | 要置位的 bit 掩码 | 无 |
| `T02_EventFlags_Clear` | 故障恢复逻辑 | 要清除的 bit 掩码 | 无 |
| `T02_EventFlags_Get` | SafetyTask | 无 | 返回当前所有事件 bit |

### 3.3 内部私有函数/数据功能

T01 示例没有额外私有函数，重点是通过函数指针隔离硬件。

T02 的 `task_records[]` 和 `system_event_flags` 是私有数据。外部只能通过接口访问，不能直接写数组。这体现了模块封装：监测模块负责数据所有权，任务只报告或读取结果。

真实工程中，心跳模块还会保存 `osThreadId_t` 任务句柄，并在一个统一采集函数中调用：

```c
uxTaskGetStackHighWaterMark(...);
osThreadGetState(...);
```

这样 `freertos.c` 只负责创建任务、注册句柄和调用采集函数。

### 3.4 完整执行逻辑

#### T01 流程

```text
main 完成 HAL 初始化
    -> 初始化 FreeRTOS
    -> 创建基础任务
    -> 启动调度器
    -> 任务调用 T01_BasicTask_RunOnce
    -> 读取当前 tick
    -> 更新 run_count 和 last_tick
    -> 翻转 LED 或发送日志
    -> 延时
    -> 重复
```

#### T02 流程

```text
系统创建多个职责任务
    -> 每个任务独立获得栈和优先级
    -> 任务每完成一轮工作就更新自己的心跳
    -> TelemetryTask 周期性采集 heap、栈余量和任务状态
    -> 监测数据写入统一 task_records[]
    -> SafetyTask 后续读取这些数据
    -> 若发现任务失活，后续阶段再设置故障事件并停机
```

注意：T02 阶段只建立监测通道，不应该提前把视觉超时、急停、电机失能等真实安全业务塞进骨架任务。

### 3.5 嵌入式开发坑点与注意事项

1. **任务栈单位不是字节**：CubeMX 的 `Stack Size (Words)` 在 F407 上通常是 4 字节一个 word。`384` 表示约 `1536 B`，不是 `384 B`。
2. **高水位不是当前剩余栈**：`uxTaskGetStackHighWaterMark()` 返回历史上最危险时刻的最低剩余量。值越小越危险。
3. **`volatile` 不是同步机制**：它不能替代队列、互斥锁、事件标志组或临界区。
4. **任务状态为 Blocked 通常是正常的**：任务调用 `osDelayUntil()` 等待时就会进入阻塞态。
5. **不要用 `osDelay()` 产生 STEP 脉冲**：精确脉冲必须交给定时器硬件，任务只提交运动命令。
6. **监控代码不能阻塞控制链路**：heap、栈、状态采集放在低优先级遥测任务中，不要让运动任务等待日志。
7. **事件标志组只表达状态**：它适合“视觉超时/急停/串口错误”等 bit 状态，不适合传输坐标、速度或整帧数据。
8. **句柄必须先创建再查询**：任务句柄为空时不能调用栈查询或状态查询 API。
9. **调试器看到的是暂停瞬间**：Watch 窗口不是实时曲线，程序必须暂停后变量才会刷新。
10. **CubeMX 重新生成可能覆盖代码**：自定义声明和逻辑必须放在 `USER CODE BEGIN/END` 区域，独立模块文件不应依赖自动生成区。
11. **心跳更新本身要短小**：不要在心跳函数中打印日志、格式化字符串或执行复杂计算。
12. **事件位读写要考虑并发**：真实 FreeRTOS 事件标志组由内核保护；不能简单照搬普通整数的 `|=` 和 `&=~` 到多任务生产代码。

## 4. main 简易测试 demo

下面示例演示上层如何调用两个独立模块。它不是 STM32 完整 `main.c`，只展示调用关系。

```c
#include <stdio.h>
#include "t01_basic_task.h"
#include "t02_task_monitor.h"
#include "t02_event_flags.h"

static uint32_t demo_tick;

static uint32_t Demo_GetTick(void)
{
  return demo_tick;
}

static void Demo_ToggleLed(void)
{
  printf("LED toggle\n");
}

int main(void)
{
  T01_BasicTaskPort port = {
    .get_tick = Demo_GetTick,
    .toggle_led = Demo_ToggleLed
  };

  T01_BasicTaskStatus basic_status = {0};

  /* 模拟 T01 任务运行三次。 */
  T01_BasicTask_RunOnce(&port, &basic_status);
  demo_tick += 500U;
  T01_BasicTask_RunOnce(&port, &basic_status);
  demo_tick += 500U;
  T01_BasicTask_RunOnce(&port, &basic_status);

  /* 模拟 T02 的 SafetyTask 更新心跳。 */
  T02_TaskMonitor_UpdateHeartbeat(T02_TASK_SAFETY, demo_tick);

  /* 模拟遥测任务写入栈余量和任务状态。 */
  T02_TaskMonitor_UpdateRuntime(T02_TASK_SAFETY,
                                300U,
                                T02_TASK_BLOCKED);

  /* 模拟另一个任务报告视觉超时。 */
  T02_EventFlags_Set(T02_EVENT_VISION_TIMEOUT);

  const volatile T02_TaskRecord *safety_record =
      T02_TaskMonitor_Get(T02_TASK_SAFETY);

  printf("T01 count = %lu\n",
         (unsigned long)basic_status.run_count);
  printf("Safety count = %lu\n",
         (unsigned long)safety_record->run_count);
  printf("Safety stack left = %lu words\n",
         (unsigned long)safety_record->stack_high_water_mark_words);
  printf("Event flags = 0x%08lx\n",
         (unsigned long)T02_EventFlags_Get());

  return 0;
}
```

这个 demo 的调用关系是：

```text
main/test
    -> T01_BasicTask_RunOnce
    -> T02_TaskMonitor_UpdateHeartbeat
    -> T02_TaskMonitor_UpdateRuntime
    -> T02_EventFlags_Set
    -> T02_TaskMonitor_Get
    -> T02_EventFlags_Get
```

在真实工程中，`main()` 不应该直接承担这些周期工作；它只负责底层初始化和启动 RTOS。周期逻辑应放入各自任务，监测采集应放入 `TelemetryTask`，故障决策应由 `SafetyTask` 完成。

## T01/T02 学习结论

T01 是“证明系统能运行”；T02 是“把运行中的任务组织起来并能观察它们”。

这一阶段的重点不是实现业务，而是建立三个工程基础：

```text
任务职责边界
    + 数据所有权
    + 可观察、可验证、可回退的运行监测
```

完成 T01/T02 后，才适合进入 T03，定义视觉结果、运动命令、输入事件和状态快照等真正的跨任务接口。
