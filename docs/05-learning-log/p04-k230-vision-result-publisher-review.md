# P04：K230 视觉结果发布器复盘

- 日期：2026-08-16
- 阶段状态：实测通过；当前人脸检测结果没有可用的数值置信度字段。
- 代码提交：`014e6ef`
- 现场证据：`evidence/serial-logs/exp-0005-p04-k230-uart-result.log`

## 这一步做了什么

已让 K230 独立完成“摄像头采集 -> 人脸检测 -> 目标框整理 -> UART 文本发布”。F407 没有参与目标解析，探索者 V3 板上的 CH340 只作为 USB-TTL 接收器，把 K230 文本送到 XCOM。

## 已实测结果

| 项目 | 实际结果 | 证据状态 |
| --- | --- | --- |
| 摄像头源图像 | `1920x1080` | K230 终端与现有 P04 记录 |
| LCD 显示 | `800x480`，ST7701 | K230 终端与现有 P04 记录 |
| 无目标 | `target=0 count=0`，日志中 251 条 | UART 原始日志 |
| 有目标 | `target=1 count=1`，日志中 135 条 | UART 原始日志 |
| 目标数据 | `x/y/w/h/cx/cy`，坐标基于 1920x1080 源图像 | UART 原始日志 |
| 发送频率 | 主要约 27～30 FPS，观测范围 19.2～35.7 FPS | UART 原始日志 |
| 置信度 | `confidence=unavailable`；检测结果只有四个框坐标 | 实测 `raw_dets_nonempty` 与 UART 日志 |
| UART 接收 | XCOM 收到连续、可读的 `PV04` 行 | UART 原始日志 |

现场日志共 386 条 `PV04` 结果行，时间从 `t_ms=31399` 到 `t_ms=45024`，跨度约 13.6 秒。示例：

```text
PV04 t_ms=38087 target=1 count=1 x=857 y=430 w=329 h=464 cx=1021.5 cy=662.0 confidence=unavailable fps=28.6
PV04 t_ms=38299 target=0 count=0 confidence=unavailable fps=29.4
```

## 代码动作 -> 工程意图

| 代码动作 | 工程意图 | 大白话描述给 AI |
| --- | --- | --- |
| `create_k230_uart()` 将 IO32 映射为 `UART3_TXD`、IO33 映射为 `UART3_RXD` | 把 UART3 接到标准版 12Pin 的真实信号脚；RX 仅是驱动初始化条件，本阶段不接外部线 | “请按标准版 12Pin，把 UART3 的发送脚明确接到 IO32，接收脚配置到 IO33，但只做单向发送。” |
| `format_result_line()` | 固定每帧的可读字段和无目标状态 | “每帧输出目标有无、数量、框坐标、中心点和 FPS；没有字段就明确写 unavailable。” |
| `format_uart_payload()` | 用 `CRLF` 给文本接收端提供行边界 | “每条结果末尾加回车换行，让串口助手逐行显示。” |
| `send_uart_payload()` | 将 ASCII 文本编码成字节后调用底层 `UART.write()` | “不要调用包装类 send；底层 UART 用 write 发送字节。” |
| 多目标按面积选最大框 | 为后续单目标跟踪提供确定的主目标 | “检测到多张脸时先选面积最大的一个，规则固定且可复现。” |

## 本轮问题与根因

1. 首次测 IO32 约 `0.33 V`：厂商 `YbUart` 默认把 UART1 映射到 IO9/IO10，并没有驱动 12Pin 的 IO32；不是 IO32 的最终逻辑电平结论。
2. 改用 UART3 后出现 `UART(3) rx not configured`：CanMV 创建 UART3 要求 TX/RX 两个 FPIOA 功能都完成配置；外部仍可以只接 TX。
3. 发送时出现 `UART object has no attribute 'send'`：`send()` 属于 `YbUart` 包装类，底层 `machine.UART` 使用 `write()`。
4. P10 跳帽必须拔掉：避免 F407 PA9 的 USART1 输出与 K230 TX 同时驱动 CH340 RX；ST-Link 使用 SWD，和 P10 无关。

## 复习要点

1. “引脚编号”不等于“当前外设已接到该引脚”；K230 需要通过 FPIOA 显式完成复用。
2. 外部只用 UART TX 时，UART 驱动仍可能要求 RX 功能完成内部配置。
3. 包装类 API 与底层硬件 API 不能混用；看到对象类型，先确认它到底是 `YbUart` 还是 `machine.UART`。
4. 万用表能帮助判断静态/平均电平，但不能替代逻辑分析仪确认每一个 UART 比特。
5. 当前结果只有 `x/y/w/h`，不能凭阈值 `0.5` 反推出一个不存在的分数。

## 用大白话向 AI 描述本项目

“请保留官方 K230 人脸检测流程，把每帧检测结果整理成 `PV04` 文本。标准版 12Pin 第 5 脚是 IO32/UART3_TXD，第 3 脚是 IO33/UART3_RXD；用 FPIOA 显式配置 UART3，115200 bit/s。每帧同时打印到 IDE，并用底层 `UART.write()` 发送 ASCII 字节，行尾使用 `CRLF`。无目标输出 `target=0`，有目标输出框和中心坐标；当前检测结果没有置信度字段，输出 `confidence=unavailable`，不要伪造数值。测试纯逻辑，不改 F407。”

## 下一步

P04 的视觉结果和文本 UART 行为已经有真实串口证据。下一步按路线进入 P05 单轴闭环电机验收台；P08 的 K230-F407 二进制协议仍要等电机前的通信设计与现场条件成熟后再做。

## 证据

- UART 原始日志副本：`evidence/serial-logs/exp-0005-p04-k230-uart-result.log`
- SHA-256：`336A8DE5781927ADA432292B38DD4F1BF01C14A5CD5EBDD38F44B9F41EF6A99B`
- K230 视觉结果证据：`evidence/serial-logs/exp-0005-p04-k230-lcd-result.log`、`evidence/serial-logs/exp-0005-p04-k230-confidence-check.log`
- 资料依据：`D:\datasheet-knowledge\parsed\K230视觉模块-标准版\full.md` 的“12Pin GPIO 介绍”；`D:\datasheet-knowledge\parsed\K230_02.基础例程_精选合集\full.md` 的“YAHBOOM K230 引脚”和“通讯接口”；探索者 V3 手册的“2.1.3 USB 串口/串口 1 选择接口”。
