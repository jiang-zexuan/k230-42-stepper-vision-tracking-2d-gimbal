# TEST-0003：P03 UART DMA 协议实验器测试报告

- 测试编号：TEST-0003
- 对应需求：REQ-012
- 日期：2026-08-16
- 代码版本：`a9889bc`。

| 测试项 | 实际结果 | 状态 |
| --- | --- | --- |
| DMA 邮箱单元测试 | `uart_rx_frame_test: PASS`。 | 通过 |
| 文本命令单元测试 | `uart_text_command_test: PASS`。 | 通过 |
| CRC16/MODBUS 与流式解析单元测试 | `vision_frame_parser_test: PASS`；覆盖标准 CRC 向量、半帧、粘包、垃圾字节和 CRC 错误后恢复。 | 通过 |
| 通信状态单元测试 | `communication_watchdog_test: PASS`；覆盖 waiting、active、timeout 与恢复。 | 通过 |
| 序号统计单元测试 | `frame_sequence_tracker_test: PASS`；覆盖首帧、连续、跳号、重复、乱序。 | 通过 |
| 板端 CRC 错帧恢复 | `FRAME ERR` 后出现新的 `FRAME OK`。 | 通过 |
| 板端通信状态 | `waiting`、`active`、`timeout` 均出现在日志中。 | 通过 |
| 板端序号异常 | `first`、`gap`、`out_of_order` 和最终统计均出现在日志中。 | 通过 |
| 半帧、粘包、垃圾重同步、重复帧 | 已逐项实测通过。重复帧结果不在本日志末尾的可检索段中。 | 通过（现场操作） |

## 证据

- 原始串口日志：`evidence/serial-logs/exp-0004-p03-uart-dma-protocol.log`。
- 实验记录：`docs/06-experiments/EXP-0004-p03-uart-dma-protocol.md`。
- 主机测试：`firmware/stm32f407/Tests/*_test.c`。

## 结论

REQ-012 本轮通过。该结论仅覆盖 PC 模拟发送端与 F407 板端；不等同于真实 K230 链路已验证。
