# 基于 K230 视觉模块与 42 步进电机的视觉跟踪二维云台

PanView 是一套基于 K230 视觉模块与 42 步进电机的视觉跟踪二维云台。K230 负责自定义目标检测，STM32F407 负责通信、双轴运动控制、界面交互与安全保护。

当前版本已完成自定义目标检测、UART 通信、双轴视觉跟踪、TFT 触摸控制台、音效与命中指示等功能，并保留各阶段的设计、实验和调试记录。

## 实物展示

| 整机与控制台 | 云台与目标跟踪 |
| --- | --- |
| ![PanView 整机总览](https://raw.githubusercontent.com/jiang-zexuan/k230-42-stepper-vision-tracking-2d-gimbal/main/docs/assets/panview-assembly-overview.jpg) | ![PanView 目标跟踪](https://raw.githubusercontent.com/jiang-zexuan/k230-42-stepper-vision-tracking-2d-gimbal/main/docs/assets/panview-target-tracking.jpg) |
| ![PanView 控制板与显示界面](https://raw.githubusercontent.com/jiang-zexuan/k230-42-stepper-vision-tracking-2d-gimbal/main/docs/assets/panview-controller-and-display.jpg) | ![PanView 命中状态界面](https://raw.githubusercontent.com/jiang-zexuan/k230-42-stepper-vision-tracking-2d-gimbal/main/docs/assets/panview-tft-hit-screen.jpg) |

| 结构细节 | 最终装配 |
| --- | --- |
| ![PanView 云台正面](https://raw.githubusercontent.com/jiang-zexuan/k230-42-stepper-vision-tracking-2d-gimbal/main/docs/assets/panview-gimbal-front.jpg) | ![PanView 最终装配](https://raw.githubusercontent.com/jiang-zexuan/k230-42-stepper-vision-tracking-2d-gimbal/main/docs/assets/panview-final-assembly.jpg) |

[查看整机演示视频（MP4）](docs/assets/panview-demo.mp4)

## 功能概览

- K230 运行自定义训练的目标检测模型，并通过模块专用 UART 接口向 F407 输出目标位置。
- F407 采用 FreeRTOS 分任务处理通信、视觉状态、双轴运动、TFT/触摸和安全看门狗。
- 两路 X42S 闭环步进电机使用 STEP/DIR/EN 控制：水平轴与俯仰轴均采用 ±90° 软件限位。
- 跟踪控制包含 PID、速度斜率限制、死区与锁定滞回，降低目标到位后的往复抖动。
- 2.8 英寸 SPI TFT 提供运行状态、目标 X/Y 误差、运行时间、启动/停止和主题切换。
- 命中状态触发提示音，并以 PF10 控制普通光线指示模块；音效结束后自动关闭指示。

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
