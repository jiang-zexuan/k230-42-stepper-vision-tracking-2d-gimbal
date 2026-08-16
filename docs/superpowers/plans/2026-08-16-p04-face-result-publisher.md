# P04 K230 视觉结果发布器实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**目标：** 在不修改厂商人脸检测例程的前提下，新增一个可直接在 CanMV IDE 运行的 PanView 脚本，把检测框转换为可读的目标结果文本。

**架构：** 脚本顶部提供不依赖 K230 运行库的结果选择、文本格式化和 UART 载荷格式化函数，先用电脑端单元测试验证；脚本运行在 K230 时复用官方 `FaceDetectionApp` 流程，选择面积最大的检测框作为主目标并打印全部检测数量。运行时显式把 IO32 复用为 UART3_TXD，并以 115200 bit/s 向 UART3 发送与 IDE 相同的文本行；只有完成 IO32 电平测量后才接入 CH340。

**技术栈：** CanMV K230 MicroPython、官方 `PipeLine`/`AIBase`/`aidemo`/`machine.FPIOA`/`machine.UART`、Python `unittest`（仅测试纯逻辑）。

---

### 任务 1：定义结果文本契约

**文件：**
- 创建：`firmware/k230/tests/test_p04_face_result_publisher.py`
- 待创建：`firmware/k230/app/p04_face_result_publisher.py`

- [ ] 写测试：空检测结果输出 `target=0`；有多个框时按面积选主目标，并输出 `x/y/w/h/cx/cy/count/fps`。
- [ ] 运行 `python -m unittest firmware/k230/tests/test_p04_face_result_publisher.py -v`，确认因生产模块不存在而失败。
- [ ] 在生产脚本中实现最小纯逻辑函数 `choose_primary_detection` 与 `format_result_line`。
- [ ] 重跑测试，确认通过。

### 任务 2：接入官方人脸检测流程

**文件：**
- 修改：`firmware/k230/app/p04_face_result_publisher.py`

- [ ] 保留官方预处理、推理和显示流程，新增帧计时、检测数量和最多 5 次原始 `det` 结构打印。
- [ ] 输出固定前缀 `PV04` 的文本；不确认的置信度字段不输出为正式字段。
- [ ] 保证 K230 运行库缺失时仍可被电脑端测试导入。

### 任务 3：UART 文本发布

**文件：**
- 修改：`firmware/k230/tests/test_p04_face_result_publisher.py`
- 修改：`firmware/k230/app/p04_face_result_publisher.py`

- [ ] 写测试：`format_uart_payload("PV04 t_ms=42 target=0")` 返回 `"PV04 t_ms=42 target=0\\r\\n"`。
- [ ] 运行 `python -m unittest firmware/k230/tests/test_p04_face_result_publisher.py -v`，确认因 `format_uart_payload` 未定义而失败。
- [ ] 实现最小函数 `format_uart_payload` 和 `create_k230_uart`，将 IO32 显式复用为 UART3_TXD，再创建 115200 bit/s 的 UART3；每帧保留 `print(result_line)`，并发送 `format_uart_payload(result_line)`；退出时调用 `uart.deinit()`。
- [ ] 重跑测试，确认通过。

### 任务 4：本地验证与现场运行准备

**文件：**
- 修改：`firmware/k230/README.md`

- [ ] 运行单元测试和 `git diff --check`。
- [ ] 记录本地测试结果，说明 K230 现场输出仍待实测。
- [ ] 告知运行步骤：先在 K230 单独运行并测量 IO32 对 GND 的空闲电压；未测量前不接线。确认电平安全后，拔掉探索者 P10 的两个跳帽，K230 IO32 接 P10 第 4 脚（CH340 RXD），K230 GND 接 F407 GND，在 XCOM 的 `COM8`、115200、8N1、无流控观察 `PV04` 行。
