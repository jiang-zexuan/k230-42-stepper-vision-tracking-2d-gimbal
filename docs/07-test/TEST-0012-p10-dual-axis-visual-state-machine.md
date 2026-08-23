# TEST-0012：P10 双轴视觉状态机与 TFT 仪表盘

## 测试目标

验证 P10 的双轴视觉闭环是否具备明确状态、双轴方向映射、500 ms 锁定确认、通信超时停机、软件限位故障和屏幕状态提示。

## 测试前提

- 已使用 ST-Link 下载当前 P10 固件。
- 两轴已人工回中；按 `KEY_UP` 后才允许接管电机。
- K230 按 P08 已验证格式输出 `PV04` 文本结果。
- 电机电源与逻辑电源分开，共地；首次实测保持低速。

## 验收步骤

| 编号 | 操作 | 预期日志/现象 | 实际结果 |
| --- | --- | --- | --- |
| 1 | 上电并打开 USART1 | `PanView P10 boot`；屏幕状态为 `SEARCH` | 待实测 |
| 2 | 不接管或关闭 `MODE` | `VIS state=SEARCH reason=waiting_for_takeover`；两轴无 STEP | 待实测 |
| 3 | 回中后按 `KEY_UP`，保持无目标 | `VIS state=SEARCH reason=target_absent`；两轴停止 | 待实测 |
| 4 | 目标偏离画面中心 | `VIS state=TRACKING`；`err_x/err_y` 和两轴速度有符号变化 | 待实测 |
| 5 | 目标保持在 X/Y 死区内超过 500 ms | `VIS state=LOCKED reason=centered_for_500ms`；屏幕状态块绿色 | 待实测 |
| 6 | 让 K230 发送无目标帧 | 回到 `SEARCH`；两轴停止 | 待实测 |
| 7 | 停止 K230 或断开其 TX | 约 300 ms 后 `VIS state=LOST`；两轴停止 | 待实测 |
| 8 | 目标继续请求越过水平 ±800 或俯仰 ±400 脉冲 | `VIS state=FAULT reason=software_limit`；STEP 停止且 EN 释放 | 待实测 |
| 9 | 触摸 `MODE/TAKE/STOP/INFO` | 按下沿只执行一次，状态屏和串口动作一致 | 待实测 |

## 通过标准

1. `SEARCH/TRACKING/LOCKED/LOST/FAULT` 均能由日志明确区分，不能靠猜测 `visual_run=0` 的原因。
2. 目标稳定居中至少 500 ms 才能进入 `LOCKED`；中心误差目标为画面宽高 5% 以内，需从实测日志记录。
3. 无目标、通信超时、位置无效和软件越界都先停止运动；任何一次都不能沿用上一帧速度。
4. 实测屏幕状态颜色与串口状态一致。

## 证据记录

- 原始串口日志：待保存为 `evidence/serial-logs/exp-0011-p10-visual-state-machine.log`。
- 用户现场日志：待补充到 `D:\工具资源\XCOM（正点原子推荐）\p10调试日志.txt` 的本轮记录。
- 当前结论：代码和文档已准备完成，实物验收未完成前保持“待实测”。
