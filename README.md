# PanView Tracker

面向具身智能边缘视觉的双轴目标跟踪云台学习项目。K230 负责视觉检测，STM32F407 与 FreeRTOS 负责通信、运动控制和安全保护。

当前状态：项目文档基线已建立，尚未生成 STM32CubeIDE 工程或 K230 应用，所有硬件连接与性能指标均待实测。

## 系统边界

K230 摄像头检测结果经 UART 发送给 F407；F407 以 STEP/DIR 控制两路 TMC2209 与步进电机。水平轴软件范围为 -180 到 +180 度，俯仰轴为 -45 到 +45 度。无编码器和限位开关，因此零点由人工回中建立。

第一版只提供屏幕准星、LED 或蜂鸣器锁定提示，不包含主动光学发射器。

## 导航

- [项目章程](docs/00-project/project-charter.md)
- [16 周路线图](docs/00-project/roadmap.md)
- [项目化学习规划书](docs/00-project/project-based-learning-plan.md)
- [GitHub 协作与同步](docs/00-project/github-collaboration.md)
- [系统架构](docs/02-architecture/system-architecture.md)
- [K230-F407 UART 接口](docs/03-interfaces/k230-f407-uart.md)
- [物料清单](hardware/bom/bom.md)
- [实验记录规则](docs/06-experiments/experiment-template.md)
