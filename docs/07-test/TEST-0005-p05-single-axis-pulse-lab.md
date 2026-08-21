# TEST-0005：P05 单轴闭环电机验收台测试报告

- 测试编号：TEST-0005
- 日期：2026-08-21
- 对应阶段：P05
- 分支：`feature/f407-uart-dma-lab`

## 自动化测试

| 测试项 | 实际结果 | 状态 |
| --- | --- | --- |
| 启停状态切换 | `motor_pulse_lab_test: PASS` | 通过 |
| 停止时换向、运行时拒绝换向 | `motor_pulse_lab_test: PASS` | 通过 |
| 800 脉冲加减速阶段边界 | `motor_pulse_lab_test: PASS` | 通过 |
| 人工零点与理论位置失效 | `relative_position_tracker_test: PASS` | 通过 |
| ARM 源文件编译 | `Profile-motion STM32 source compile: PASS`、`Relative-position STM32 source compile: PASS` | 通过 |

证据文件：`evidence/serial-logs/exp-0006-p05-automated-tests.log`。

## 板上验收

| 测试项 | 实际结果 | 状态 |
| --- | --- | --- |
| 上电不自动运动 | 已观察 | 通过 |
| 500 Hz 连续启停 | 已实测 | 通过 |
| Low/High 方向定义 | Low=逆时针，High=顺时针（输出轴正面观察） | 通过 |
| 运行中禁止换向 | 已实测，电机不反转 | 通过 |
| 800 脉冲约 90 度后停止 | 已实测 | 通过 |
| 加速、匀速、减速 | 已实测，未见突然启停 | 通过 |
| 30 秒持续运行 | 未见异常运动、报警或明显温升 | 通过；温度数值待测 |
| STEP 频率/占空比仪器测量 | 尚未测量 | 待验证 |

## 已知限制

- 800 脉冲换算出的 90 度是理论指令角度，不是外部仪器或编码器测得的绝对角度。
- 没有保存 P05 专用 XCOM 原始日志，因此不把串口输出声称为已归档原始证据。
- 没有限位开关时，软件位置失效后必须人工回中。
