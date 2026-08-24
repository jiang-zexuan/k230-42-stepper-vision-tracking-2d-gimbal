# TEST-0004：P04 K230 视觉结果发布器测试报告

- 测试编号：TEST-0004
- 对应需求：REQ-013
- 日期：2026-08-16
- 代码版本：`014e6ef`

## 自动化测试

| 测试项 | 实际结果 | 状态 |
| --- | --- | --- |
| 结果选择、无目标文本和坐标格式化 | `7` 项相关断言通过 | 通过 |
| UART3 FPIOA 映射 | 验证 IO32/TX、IO33/RX、UART3、115200 | 通过 |
| UART 文本边界 | 验证 `CRLF` | 通过 |
| 底层发送 API | 验证调用 `write()` 并发送 ASCII 字节 | 通过 |
| Python 语法检查 | `py_compile` 退出码 0 | 通过 |
| 空白检查 | `git diff --check` 无错误 | 通过 |

运行命令：

```text
python -m unittest firmware/k230/tests/test_p04_face_result_publisher.py -v
```

实际结果：`Ran 8 tests ... OK`。

## 硬件验收

| 测试项 | 实际结果 | 状态 |
| --- | --- | --- |
| K230 人脸检测运行 | LCD 与 IDE 终端持续输出 | 通过 |
| UART 单向接收 | XCOM 收到连续 `PV04` 行 | 通过 |
| 无目标状态 | `target=0 count=0`，251 行 | 通过 |
| 有目标状态 | `target=1 count=1`，135 行 | 通过 |
| 坐标字段 | `x/y/w/h/cx/cy` 均出现在有目标行 | 通过 |
| FPS 字段 | 主要约 27～30 FPS | 通过 |
| 置信度字段 | `confidence=unavailable`；无伪造数值 | 通过（已知限制） |

## 证据

- UART 原始日志：`evidence/serial-logs/exp-0005-p04-k230-uart-result.log`
- SHA-256：`336A8DE5781927ADA432292B38DD4F1BF01C14A5CD5EBDD38F44B9F41EF6A99B`
- 实验记录：`docs/06-experiments/EXP-0005-p04-k230-vision-result-publisher.md`
- K230 视觉前置证据：`evidence/serial-logs/exp-0005-p04-k230-lcd-result.log`、`evidence/serial-logs/exp-0005-p04-k230-confidence-check.log`

## 结论

P04 的模型运行、结果格式化、UART3 单向文本发布和 XCOM 接收均已通过本轮验收。该报告不覆盖 P08 的正式二进制协议、CRC 或 F407 视觉帧解析。
