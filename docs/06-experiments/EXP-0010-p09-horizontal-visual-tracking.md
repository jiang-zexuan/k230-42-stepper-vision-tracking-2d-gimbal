# EXP-0010：P09 单轴视觉跟踪器

## 基本信息

- 实验编号：EXP-0010
- 日期：2026-08-23
- 状态：水平视觉跟随和安全停机已阶段性实测通过；精确时延和错帧根因待补测。
- 分支：`feature/f407-p09-horizontal-visual-tracking`

## 目标

验证 K230 `PV04` 文本结果经过 F407 解析后，能够只控制水平轴，执行死区、比例速度、最大速度、软件限位和目标丢失停机。

## 硬件与接线

沿用 P08/P07 已确认的接线：

```text
K230 IO9 / TX1  -> F407 PA3 / USART2_RX
K230 IO10 / RX1 <- F407 PA2 / USART2_TX
K230 GND        -> F407 GND
水平 X42S STEP  -> F407 PB6 / TIM4_CH1
水平 X42S DIR   -> F407 PB7
水平 X42S ENA   -> F407 PG15
水平 X42S GND   -> F407 GND
```

当前控制参数：死区 40 px，比例 2 脉冲/px，最大速度 800 脉冲/s，视觉结果超时 300 ms。水平软件范围为 `-800~+800` 脉冲；参数来源为当前 P06/P07 设计和 P09 实测，机械真实余量仍需单独核验。

## 操作与结果

1. 下载 Keil 构建产物，构建结果为 `0 Error(s), 0 Warning(s)`：通过。
2. 两轴手动回中，按 `KEY_UP`，日志出现 `POSITION theory=+0.0deg pulses=0 reason=manual_zero`：通过。
3. 目标偏离画面中心，`err_x` 正负与 `pan_speed` 正负按设计变化，水平电机实物方向正确：通过。
4. 目标进入 40 px 死区，`pan_speed=0` 且不持续输出 STEP：通过。
5. 大偏差目标触发 `pan_speed=800` 或 `-800`，`pan_pos` 按脉冲变化：通过。
6. `pan_pos` 到达 `+800/-800` 后，视觉 STEP 停止且不继续向外运动：通过。
7. K230 输出 `target=0` 时，`pan_speed=0`、`visual_run=0`：通过。
8. 停止 K230 发布后，现场确认水平电机停止；串口日志未提供毫秒级时刻：功能通过，精确 300 ms 待逻辑分析仪测量。

## 自动化与构建

```text
Keil: Rebuild 0 errors, 0 warnings
```

P08 已有的 `vision_error_test`、`visual_track_controller_test` 和文本解析测试继续作为控制器输入的回归依据；本实验未修改这些单元测试。

## 结论

已把视觉误差、速度控制、STEP 输出、软件位置和水平限位连接成第一版单轴视觉闭环。当前可以进入 P10 的双轴视觉控制设计，但必须保留 P09 的三个遗留项：精确 300 ms 波形证据、`text_err` 根因和置信度门槛。

## 证据

- 原始日志：`D:\工具资源\XCOM（正点原子推荐）\p09调试日志.txt`
- 原始日志 SHA-256：`91EB77025E1747F366E374C4574BB95ECADE0D248A59AE820A3DF2ABB50ACFEF`
- 仓库副本：`evidence/serial-logs/exp-0010-p09-horizontal-visual-tracking.log`
