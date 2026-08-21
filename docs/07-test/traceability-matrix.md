# 需求追踪矩阵

| 需求 | 设计/代码 | 测试 | 证据 | 当前状态 |
| --- | --- | --- | --- | --- |
| REQ-001 | 待建 | 待建 | 待建 | 设计基线 |
| REQ-002 | 待建 | 待建 | 待建 | 设计基线 |
| REQ-003 | 待建 | 待建 | 待建 | 设计基线 |
| REQ-004 | 待建 | 待建 | 待建 | 设计基线 |
| REQ-005 | 待建 | 待建 | 待建 | 设计基线 |
| REQ-006 | 待建 | 待建 | 待建 | 设计基线 |
| REQ-007 | ADR-0001 | 待建 | 待建 | 设计基线 |
| REQ-008 | 待建 | 待建 | 待建 | 设计基线 |
| REQ-009 | 待建 | 待建 | 待建 | 设计基线 |
| REQ-010 | `firmware/stm32f407/PanViewF407/`、`firmware/stm32f407/App/board_diagnostics.*` | `docs/07-test/TEST-0001-f407-board-bringup.md` | `docs/06-experiments/EXP-0002-f407-board-bringup.md`、`evidence/serial-logs/exp-0002-usart1.log` | 实测通过 |
| REQ-011 | `firmware/stm32f407/PanViewF407/Core/Src/periodic_task.c`、`debounced_button.c`、`main.c` | `docs/07-test/TEST-0002-p02-nonblocking-timed-tasks.md` | `docs/06-experiments/EXP-0003-p02-nonblocking-timed-tasks.md`、`evidence/serial-logs/exp-0003-p02-timed-tasks.log` | 实测通过 |
| REQ-012 | `firmware/stm32f407/PanViewF407/Core/Src/uart_rx_frame.c`、`uart_text_command.c`、`vision_frame_parser.c`、`communication_watchdog.c`、`frame_sequence_tracker.c`、`main.c` | `docs/07-test/TEST-0003-p03-uart-dma-protocol.md` | `docs/06-experiments/EXP-0004-p03-uart-dma-protocol.md`、`evidence/serial-logs/exp-0004-p03-uart-dma-protocol.log` | 实测通过 |
| REQ-013 | `firmware/k230/app/p04_face_result_publisher.py`、`firmware/k230/tests/test_p04_face_result_publisher.py` | `docs/07-test/TEST-0004-p04-k230-vision-result-publisher.md` | `docs/06-experiments/EXP-0005-p04-k230-vision-result-publisher.md`、`evidence/serial-logs/exp-0005-p04-k230-uart-result.log` | 实测通过（置信度字段不可用） |
| REQ-014 | `firmware/stm32f407/PanViewF407/Core/Src/motor_pulse_lab.c`、`relative_position_tracker.c`、`main.c` | `docs/07-test/TEST-0005-p05-single-axis-pulse-lab.md` | `docs/06-experiments/EXP-0006-p05-single-axis-pulse-lab.md`、`evidence/serial-logs/exp-0006-p05-automated-tests.log` | 板上实测通过；波形和温度数值待补测 |
