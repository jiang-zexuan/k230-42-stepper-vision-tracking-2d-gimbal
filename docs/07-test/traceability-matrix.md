# 需求追踪矩阵

| 需求 | 设计/代码 | 测试 | 证据 | 当前状态 |
| --- | --- | --- | --- | --- |
| REQ-001 | `relative_position_tracker.c`、`main.c` | `docs/07-test/TEST-0006-p06-horizontal-motion-limits.md` | `evidence/serial-logs/exp-0007-p06-board-feedback.log` | 水平轴人工回中和累计位置已实测；俯仰轴待 P07 |
| REQ-002 | `motion_limits.c`、`main.c`、`ADR-0002-horizontal-axis-cable-limit.md` | `docs/07-test/TEST-0006-p06-horizontal-motion-limits.md` | `evidence/serial-logs/exp-0007-motion-limits-unit-tests.log`、`evidence/serial-logs/exp-0007-p06-board-feedback.log` | 水平 ±90° 越界拒绝和反向返回已实测；机械余量、俯仰范围待核验 |
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
| REQ-015 | `firmware/stm32f407/PanViewF407/Core/Src/main.c`、`tim.c`、`uart_text_command.c` | `docs/07-test/TEST-0007-p07-dual-axis-controller.md` | `docs/06-experiments/EXP-0008-p07-dual-axis-controller.md`、用户保存的 P07 串口日志 | 双轴接管、同步运动和俯仰 ±45° 限位已实测通过 |
| REQ-016 | `uart_text_line_accumulator.c`、`vision_text_result_parser.c`、`vision_error.c`、`visual_track_controller.c`、`main.c` | `docs/07-test/TEST-0008-p08-k230-f407-link.md` | `docs/06-experiments/EXP-0009-p08-k230-f407-link.md`、P08 原始串口日志 | 文本过渡链路、误差和水平速度目标已实测；正式二进制协议待完成 |
| REQ-017 | `visual_track_controller.c`、`relative_position_tracker.c`、`main.c` | `docs/07-test/TEST-0009-p09-horizontal-visual-tracking.md` | `docs/06-experiments/EXP-0010-p09-horizontal-visual-tracking.md`、`evidence/serial-logs/exp-0010-p09-horizontal-visual-tracking.log` | 水平视觉跟随、死区、比例、限速、软件限位和功能停机已阶段性实测；精确 300 ms、错帧根因和置信度门槛待补测 |
