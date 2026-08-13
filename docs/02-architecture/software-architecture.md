# 软件架构

## F407 分层设计

| 层 | 职责 |
| --- | --- |
| BSP | GPIO、UART、DMA、定时器、步进脉冲。 |
| Drivers | TMC2209、K230 协议、日志接口。 |
| Services | 运动规划、位置估算、目标跟踪、故障管理。 |
| App | FreeRTOS 任务与系统状态机。 |

## FreeRTOS 任务设计

| 任务 | 周期或触发 | 职责 |
| --- | --- | --- |
| VisionRxTask | UART DMA 事件 | 接收、组帧与 CRC 校验。 |
| MotionTask | 10 ms | 计算双轴目标速度。 |
| StepperTask | 定时器事件 | 生成脉冲并执行加减速。 |
| SafetyTask | 10 ms | 通信超时、软件限位和任务存活检查。 |
| TelemetryTask | 100 ms | 输出状态和错误统计。 |
| StatusTask | 状态变化 | 显示搜索、跟踪、锁定和故障。 |

任务优先级、栈大小和具体定时器资源均待板级工程创建后实测确定。
