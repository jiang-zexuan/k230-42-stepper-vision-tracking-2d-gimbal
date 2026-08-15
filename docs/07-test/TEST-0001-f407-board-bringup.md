# TEST-0001：P01 F407 板级诊断台测试报告

- 测试编号：TEST-0001
- 对应需求：REQ-010
- 日期：2026-08-15
- 硬件版本与接线：探索者 V3.2、STM32F407ZGT6；外置 ST-Link SWD；板载 CH340C `COM8`。
- 固件：`firmware/stm32f407/PanViewF407/`，工程基线提交 `5339c42`。
- 配置：SYS Serial Wire；PF9 GPIO 输出；PE4 GPIO 输入上拉；USART1 PA9/PA10，115200 bit/s、8N1、无流控。

## 步骤与实际结果

| 测试项 | 预期结果 | 实际结果 | 状态 |
| --- | --- | --- | --- |
| ST-Link 下载与校验 | 工程可下载并校验 | 已确认 CubeIDE 显示下载校验成功。 | 实测 |
| 断点与变量 | CPU 可停在用户代码并查看 KEY0 状态 | 已在断点观察到 KEY0 未按为 `GPIO_PIN_SET`、按下为 `GPIO_PIN_RESET`。 | 实测 |
| DS0 初始状态 | 上电后 DS0 熄灭 | 已观察到熄灭。 | 实测 |
| KEY0 改变 DS0 | 每次有效短按切换 DS0 | 已观察到切换。 | 实测 |
| 启动日志 | 复位后输出 `PanView P01 boot` | 已在 `COM8` 观察到，RESET 后再次出现。 | 实测 |
| KEY0 日志 | 有效按下输出一次 `KEY0 pressed` | 已观察到。 | 实测 |
| 周期日志 | 周期输出固定心跳 | 已观察到 `PanView alive`。 | 实测 |
| 递增计数日志 | 输出连续递增计数 | 原始日志包含 3313 个样本、3 次重新计数；最后一段从 `heartbeat: 1` 连续递增至 `heartbeat: 1300`。 | 实测 |
| 原始串口导出 | 保存未编辑 `.log` 作为证据 | 已保存于 `evidence/serial-logs/exp-0002-usart1.log`。 | 实测 |

## 证据

- 现场观察记录：`docs/06-experiments/EXP-0002-f407-board-bringup.md`。
- 原始串口日志：`evidence/serial-logs/exp-0002-usart1.log`，SHA-256：`4C75308961B7E98995F8FFE2B3B9876DF47E84D7088A08F4E12E4355CA263273`。

## 结论

REQ-010 的下载、调试、LED、按键、启动日志、按键事件、递增心跳与原始串口导出均已完成实测，本测试通过。
