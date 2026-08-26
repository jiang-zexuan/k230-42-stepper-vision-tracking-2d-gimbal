# 基于 K230 视觉模块与 42 步进电机的视觉跟踪二维云台

PanView：基于 K230 视觉模块与 42 步进电机的视觉跟踪二维云台学习项目。K230 负责视觉检测，STM32F407 负责通信、运动控制和安全保护。

当前状态：P01-P11 已完成阶段性实测并留存记录，P11 已完成 FreeRTOS 任务拆分、故障管理和 IWDG 正常运行验收；P12 已完成自定义目标模型接入、TFT 控制台 UI、主题切换、俯仰 ±90° 软件限位和视觉锁定滞回调参。P12 最新现场日志需单独归档后再作为本轮硬件证据。

## 系统边界

K230 摄像头检测结果经 UART 发送给 F407；F407 以 STEP/DIR 控制两路闭环步进电机。X42S 驱动器内部带磁编码器和 FOC 闭环，当前 F407 已通过 USART3 TTL 读取水平轴反馈用于诊断，控制坐标仍按人工回中后的脉冲累计。当前装配约束下，水平轴软件范围设计为 -90 到 +90 度；俯仰轴数值范围尚待机械行程核验，不能据此假设无限旋转。零点先由人工回中建立，位置可信度丢失后必须重新回中。

第一版只提供屏幕准星、LED 或蜂鸣器锁定提示，不包含主动光学发射器。

## 当前基线

- 主分支：`main`
- P11 收尾提交：`67beb5b`
- 主分支合并提交：`459d08c`
- 当前固件环境：STM32CubeMX + Keil MDK；K230 使用标准版模块。
- 已验证的主要链路：K230 视觉结果 -> UART -> F407 + FreeRTOS -> 双轴 STEP/DIR/EN；F407 同时负责 TFT、触摸、音效、限位和故障保护。

## 导航

- [项目章程](docs/00-project/project-charter.md)
- [16 周路线图](docs/00-project/roadmap.md)
- [项目化学习规划书](docs/00-project/project-based-learning-plan.md)
- [中文学习导航](docs/00-project/中文导航.md)
- [GitHub 协作与同步](docs/00-project/github-collaboration.md)
- [系统架构](docs/02-architecture/system-architecture.md)
- [K230-F407 UART 接口](docs/03-interfaces/k230-f407-uart.md)
- [物料清单](hardware/bom/bom.md)
- [实验记录规则](docs/06-experiments/experiment-template.md)
- [P11 FreeRTOS 与故障管理复盘](docs/05-learning-log/p11-freertos-fault-management-review.md)
- [P12 自定义模型与控制台复盘](docs/05-learning-log/p12-custom-model-and-ui-review.md)
- [网页版 UI 方案预览](ui-demo/index.html)
