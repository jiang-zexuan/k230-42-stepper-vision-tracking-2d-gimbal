# 基于 K230 视觉模块与 42 步进电机的视觉跟踪二维云台

PanView：基于 K230 视觉模块与 42 步进电机的视觉跟踪二维云台学习项目。K230 负责视觉检测，STM32F407 负责通信、运动控制和安全保护。

当前状态：P01、P02、P03、P04、P05 已完成阶段性实测并留存记录；P05 的 STEP 波形和温度数值仍待仪器补测。其余硬件连接与性能指标仍须以手册和实测为准。

## 系统边界

K230 摄像头检测结果经 UART 发送给 F407；F407 以 STEP/DIR 控制两路 TMC2209 与步进电机。当前装配约束下，水平轴软件范围设计为 -90 到 +90 度；俯仰轴数值范围尚待机械行程核验，不能据此假设无限旋转。无编码器和限位开关，因此零点由人工回中建立。

第一版只提供屏幕准星、LED 或蜂鸣器锁定提示，不包含主动光学发射器。

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
