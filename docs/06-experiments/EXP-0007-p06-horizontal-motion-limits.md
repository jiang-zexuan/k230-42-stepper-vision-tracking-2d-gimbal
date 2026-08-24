# EXP-0007：P06 水平轴软件限位

## 基本信息

- 实验编号：EXP-0007
- 日期：2026-08-21
- 状态：软件测试、ARM 编译和板上实测通过；机械余量待核验。
- 目标：验证水平轴在人工零点和 ±90° 设计限值下拒绝外向越界运动。
- 固件分支：`feature/f407-uart-dma-lab`

## 依据与参数

- 当前 X42S 已确认安装在水平轴。
- 当前装配约束：水平线束限制设计为 `-90°` 到 `+90°`。
- X42S 当前 16 细分为 `3200 脉冲/圈`，因此 `90° = 800 脉冲`。
- 代码依据：`firmware/stm32f407/PanViewF407/Core/Inc/motion_limits.h`。
- 决策依据：`docs/04-design-decisions/ADR-0002-horizontal-axis-cable-limit.md`。

## 软件操作步骤与结果

1. 在未设置人工零点的状态请求相对移动：已拒绝。
2. 从 `0` 请求 `+800` 脉冲：已接受，目标为 `+800`。
3. 从 `+800` 再请求 `+800`：已拒绝越界。
4. 从 `+800` 请求 `-800`：已接受，目标回到 `0`。
5. 从 `0` 请求 `-800`：已接受，目标为 `-800`。
6. 从 `-800` 再请求 `-800`：已拒绝越界。

## 板上实测步骤与结果

1. 已烧录 Debug 固件，上电未自动输出运动。
2. 停止状态按 KEY_UP 建立人工零点，日志为 `POSITION theory=+0.0deg pulses=0 reason=manual_zero`。
3. 按 KEY2 运行到两侧，分别记录 `POSITION theory=+90.0deg` 和 `POSITION theory=-90.0deg`。
4. 在两侧边界继续向外按 KEY2，均记录 `MOTOR profile move rejected: horizontal_limit`，未产生新的运动完成日志。
5. 按 KEY1 改变方向后，按 KEY2 可从边界返回范围内部。
6. 运动反馈周期采样显示速度约 `-10..+10 RPM`，位置误差最大约 `0.68°`，停止后为 `0.00°`。

## 证据

- 主机单元测试：`evidence/serial-logs/exp-0007-motion-limits-unit-tests.log`。
- ARM 编译结果：CubeIDE Debug 构建 `0 errors, 0 warnings`，生成 `PanViewF407.elf`。
- 板上 XCOM 原始日志：`evidence/serial-logs/exp-0007-p06-board-feedback.log`。

## 结论与下一步

限位算法和板上安全行为均已通过当前实测。P06 阶段完成；下一步进入 P07 双轴云台基础控制器。俯仰轴范围另行核验，不沿用水平轴数值。
