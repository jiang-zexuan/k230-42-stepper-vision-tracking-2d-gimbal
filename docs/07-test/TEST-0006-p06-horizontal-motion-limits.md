# TEST-0006：P06 水平轴软件限位测试报告

- 测试编号：TEST-0006
- 日期：2026-08-21
- 对应阶段：P06 单轴运动控制器
- 分支：`feature/f407-uart-dma-lab`

## 自动化测试

| 测试项 | 实际结果 | 状态 |
| --- | --- | --- |
| 未人工回零时移动 | `MOTION_LIMITS_REJECT_POSITION_INVALID` | 通过 |
| `0 -> +800` | 接受，目标 `+800` | 通过 |
| `+800 -> +800` | `MOTION_LIMITS_REJECT_OUT_OF_RANGE` | 通过 |
| `+800 -> 0` | 接受，目标 `0` | 通过 |
| `0 -> -800` | 接受，目标 `-800` | 通过 |
| `-800 -> -1600` | `MOTION_LIMITS_REJECT_OUT_OF_RANGE` | 通过 |
| 板上人工回零 | `POSITION theory=+0.0deg pulses=0 reason=manual_zero` | 通过 |
| 板上 `+90°` 边界拒绝外向运动 | `MOTOR profile move rejected: horizontal_limit` | 通过 |
| 板上 `-90°` 边界拒绝外向运动 | `MOTOR profile move rejected: horizontal_limit` | 通过 |
| 板上边界反向返回 | 两侧均完成反向 800 脉冲运动 | 通过 |
| X42S 运动反馈 | 误差最大约 `0.68°`，停止后 `0.00°` | 通过 |
| 主机编译运行 | `motion_limits_test: PASS` | 通过 |
| STM32 ARM 工程编译 | `0 errors, 0 warnings` | 通过 |

自动化原始输出：`evidence/serial-logs/exp-0007-motion-limits-unit-tests.log`。

## 已知限制

- `±800` 脉冲来自当前 16 细分配置；细分改变后必须重新计算。
- 软件限位不能替代机械限位开关或编码器反馈。
- 俯仰轴数值范围尚未核验。
- 水平 `±90°` 的完整机械行程和线束余量尚未用结构测量确认。
