# PanView FreeRTOS 架构重构任务规划书

- 项目：基于 K230 视觉模块与 42 步进电机的视觉跟踪二维云台（PanView）
- 规划日期：2026-09-02
- 开发分支：`feature/freertos-rearchitecture`
- 新工程目录：`E:\Fluushbonading\FreeRTOS\Project\PanView`
- 旧工程目录：`firmware/stm32f407/PanViewF407`
- 规划状态：设计草案，待逐节实施和验收

## 1. 工作方式

本次重构采用“一个小任务、一轮验收”的教学式流程：

1. 每节开始前说明目标、原理、关键 FreeRTOS API、风险和验收标准。
2. 由操作者在 CubeMX、CLion、编译器、下载器和实机上完成本节操作。
3. 我负责检查修改范围、接口设计、构建日志、串口证据和实机结果。
4. 只有当前节验收通过，才进入下一节。
5. 每节保留独立提交和回退点，不直接修改 `main` 分支。

旧工程只作为行为参考和回退基线；新工程位于 E 盘独立目录，不把 `main.c` 大段搬入新工程。

## 2. 工程边界

### 旧工程：只读参考

`firmware/stm32f407/PanViewF407` 保留当前已经实现的：

- 外设初始化和引脚映射
- K230 UART 文本/二进制接收代码
- 视觉误差、PID、死区、锁定滞回和速度斜率
- X42S STEP/DIR/EN 与 TIM3/TIM4 脉冲路径
- TFT、触摸、ES8388、PF10 指示和安全逻辑

旧工程不作为新架构的最终代码来源；每次只提取经过理解和验收的最小接口。

### 新工程：逐步重建（E 盘独立工程）

新工程路径为 `E:\Fluushbonading\FreeRTOS\Project\PanView`，由 CubeMX 生成基础 HAL/FreeRTOS 工程，CLion 使用 CMake 构建。应用代码按以下边界组织：

```text
Core/                 CubeMX 启动代码和底层入口
BSP/                  GPIO、UART、DMA、定时器、看门狗适配
Drivers/              K230 协议、X42S、TFT、触摸、音频驱动接口
Services/             视觉解析、目标跟踪、运动规划、限位、安全服务
App/                  FreeRTOS 任务、消息、状态快照和控制编排
Tests/                主机侧单元测试和测试向量
docs/                 架构、实验、验收和决策记录
```

## 3. 目标数据流

```text
USART2 DMA/IDLE ISR
  -> 接收片段环形缓冲区
  -> VisionRxTask
  -> 长度/版本/CRC/序号检查
  -> 最新视觉结果缓存
  -> MotionTask
  -> PID/死区/滞回/斜率/软件限位
  -> 最新双轴运动命令
  -> StepperTask
  -> TIM3/TIM4 硬件 STEP 脉冲
```

并行链路：

```text
InputTask -> InputEventQueue -> AppControlTask
UiTask -> 读取只读状态快照 -> TFT 局部刷新
AudioIndicatorTask <- 音效/指示事件
TelemetryTask -> 日志、统计、栈余量
SafetyTask -> 事件标志、直接停机、健康时刷新 IWDG
```

## 4. 统一约束

- ISR 只做必要的状态记录、缓冲搬运、DMA 重启和任务通知。
- ISR 不做字符串格式化、协议解析、TFT、音频或阻塞式 UART 发送。
- `MotionTask` 不直接写 GPIO 或定时器寄存器。
- `StepperTask` 是 STEP/DIR/EN 和理论位置的唯一业务所有者。
- `UiTask` 不直接操作电机。
- `SafetyTask` 的停机路径不依赖 UI、音频或遥测任务。
- 精确 STEP 脉冲由 TIM3/TIM4 产生，不使用 `osDelay()` 造脉冲。
- 视觉目标采用最新值语义，不累计处理陈旧目标。
- USART1 发送统一通过发送服务和互斥保护；控制任务不得等待低优先级日志任务。
- 所有数值参数记录单位、来源和验证状态。

## 5. 小任务清单

### T00：建立新工程与基线

目标：创建新分支、新目录和可编译的 CubeMX + CLion 工程。

工作内容：

- 在 `feature/freertos-rearchitecture` 分支工作。
- 在 `E:\Fluushbonading\FreeRTOS\Project\PanView` 生成新的 `.ioc`、CMake 和启动工程。
- 复制或配置必要的 HAL/FreeRTOS 依赖，但不复制旧 `main.c` 业务逻辑。
- 记录芯片、时钟、UART、DMA、TIM、IWDG 和调试下载配置。

验收：新工程空应用可编译、下载、运行；串口输出一次启动日志；旧工程行为不变。

回退点：删除新目录或回到本节提交，不触碰旧工程。

### T01：最小 BSP 与日志出口

目标：只建立 GPIO、毫秒时基和 USART1 日志出口。

API 重点：`osKernelInitialize`、`osThreadNew`、`osDelay`、`HAL_UART_Transmit`。

验收：一个心跳任务周期运行，日志有时间戳；没有电机、视觉和 UI 逻辑。

### T02：RTOS 任务骨架

目标：创建 `SafetyTask`、`VisionRxTask`、`MotionTask`、`StepperTask`、`InputTask`、`UiTask`、`AudioIndicatorTask`、`TelemetryTask`。

新增对象：任务句柄、心跳表、消息队列、互斥锁、事件标志组。

验收：所有任务创建成功；输出剩余 heap、栈余量和任务状态；SafetyTask 健康时才允许启动 IWDG。

### T03：统一消息和状态快照

目标：定义跨任务接口和所有权。

核心类型：`VisionResult`、`MotionCommand`、`InputEvent`、`IndicatorEvent`、`PanViewStateSnapshot`。

验收：主机侧编译测试能够构造、复制和检查这些消息；不允许任务直接访问其他任务的私有变量。

### T04：UART DMA/IDLE 接收适配

目标：将 USART2 接收改造成“ISR 搬运 + 通知任务”的接口，但暂不接入视觉业务。

API 重点：`HAL_UARTEx_ReceiveToIdle_DMA`、`osThreadFlagsSet` 或任务通知、环形缓冲区。

验收：半帧、粘包、DMA 满、UART 错误均能被统计；ISR 无日志和复杂解析。

### T05：VisionRxTask

目标：迁移组帧、长度/版本/CRC/序号检查并发布最新视觉结果。

验收：正常帧、坏 CRC、非法长度、错版本、垃圾字节、重复帧、乱序帧和超时均有可复现实验记录。

### T06：MotionTask 影子运行

目标：迁移视觉误差、PID、死区、锁定滞回、速度斜率和软件限位，但只输出影子命令。

验收：新旧控制器对同一组输入的输出差异在预先定义的容差内；不驱动电机。

### T07：StepperTask 与硬件脉冲

目标：迁移 STEP/DIR/EN、启动/停止、理论位置和人工回中。

验收：TIM3/TIM4 波形正确；正反方向、停止、失能、±90°软件限位和重新回中要求均通过实机验证。

### T08：SafetyTask 独立停机

目标：实现视觉超时、软件越界、任务失活、UART 异常和紧急停止事件。

验收：SafetyTask 可直接停止脉冲并失能；停止 UI、音频或遥测任务不影响安全停机；IWDG 故障注入按预期复位。

### T09：InputTask 与 AppControlTask

目标：按键、触摸只发布事件；启停、人工回中、模式切换由控制任务统一处理。

验收：输入任务不直接写电机；重复触摸、消抖和非法状态下的操作有明确结果。

### T10：UiTask

目标：从状态快照刷新 TFT 局部区域。

验收：显示视觉状态、运行时间、X/Y 误差和故障码；UI 阻塞或刷新异常不阻止运动安全路径。

### T11：AudioIndicatorTask

目标：迁移命中特效音、PF10 指示、音效结束关闭和故障关闭。

验收：命中、停止、故障、复位和音效结束的指示状态符合定义；音频任务不阻塞控制任务。

### T12：TelemetryTask

目标：统一低优先级日志、通信统计、任务状态和栈余量。

验收：USART1 并发发送无交错；遥测拥塞时控制任务仍按周期运行。

### T13：端到端联调

目标：接通 K230 -> VisionRxTask -> MotionTask -> StepperTask 全链路。

验收：人工回中后完成双轴跟踪；目标丢失、坏帧、超时、越界和任务失活均进入安全状态。

### T14：删除旧集中式路径

目标：在新链路完全验收后删除 `PanView_AppStep()` 中的旧业务逻辑和兼容入口。

验收：新工程不依赖旧集中式状态；全功能回归、故障注入、长时间运行和文档追踪矩阵完成。

## 6. 每节固定验收模板

每个小任务都必须记录：

- 任务编号与目标
- 日期、分支和提交哈希
- 修改文件
- 编译命令和完整结果
- 下载固件与硬件接线
- 串口日志、波形、照片或视频证据
- 预期结果与实际结果
- 未通过项、原因假设和下一步

状态只能写：需求、设计、实测、待验证。没有实机证据时不得写成“已完成”或“稳定”。

## 7. 当前下一步

规划书确认后，从 T00 开始：在 `E:\Fluushbonading\FreeRTOS\Project\PanView` 生成新 CubeMX 工程和 CLion/CMake 构建骨架，完成一次空应用编译和下载，不迁移业务代码。
