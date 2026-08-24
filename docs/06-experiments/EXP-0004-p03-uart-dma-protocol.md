# EXP-0004：P03 UART DMA 协议实验器

## 基本信息

- 实验编号：EXP-0004
- 日期：2026-08-16
- 状态：实测通过。
- 目标：用 PC 串口模拟视觉模块，验证 USART1 RX DMA、IDLE 收包、文本命令、二进制帧、CRC、重同步、超时和序号统计。
- 固件提交：`a9889bc`。

## 硬件与接线

- 开发板：探索者 V3.2，STM32F407ZGT6。
- 调试下载：外置 ST-Link，SWD。
- 串口：板载 CH340C，`COM8`，115200 bit/s、8N1、无流控。
- P03 不新增外接信号线；USART1 的 PA9/PA10 与 CH340C 的板载连接沿用 P01 已核对的资料和实测结论。

## 固件与配置

- USART1 RX DMA：DMA2 Stream2、Channel 4、Peripheral-to-Memory、Normal 模式、64 B 接收缓冲区。
- 收包 API：`HAL_UARTEx_ReceiveToIdle_DMA()`；关闭半传输通知，仅在 UART IDLE 或缓冲区满时处理一段数据。
- 二进制协议：`AA 55 | version | payload_len | sequence | payload | CRC16/MODBUS`。
- CRC：初值 `0xFFFF`，多项式 `0xA001`，CRC 覆盖版本至负载，低字节先发送。
- 超时观察值：1000 ms；该值只用于 P03 链路状态观察。

## 操作与结果

1. 构建并下载固件，在 XCOM 打开 COM8。
   - 预期：启动后状态日志为 `link=waiting`。
   - 实际：原始日志多处记录 `link=waiting`。
2. 发送文本 `PING` 和 `STATUS`。
   - 预期：分别返回 `PONG` 和当前统计。
   - 实际：已实测通过；早期调试行仍保留在原始日志中。
3. 发送有效帧、拆开的半帧、连续的两帧和带垃圾前缀的有效帧。
   - 预期：有效帧被解析；半帧等待后续数据；粘包的两帧均被解析；垃圾字节后恢复到下一帧头。
   - 实际：已逐项实测通过。
4. 发送 CRC 被故意改坏的帧，再发送有效帧。
   - 预期：错误计数增加，后续有效帧仍被解析。
   - 实际：日志记录 `FRAME ERR valid=3 err=1`，之后记录 `FRAME OK seq=4 len=1 valid=4 err=1`。
5. 发送有效帧后静默超过 1000 ms。
   - 预期：`waiting -> active -> timeout`。
   - 实际：日志记录有效帧后的 `link=active`，随后记录 `link=timeout`。
6. 依次发送序号 1、3、2，并查询状态。
   - 预期：分别为 `first`、`gap`、`out_of_order`；缺失计数为 1，旧帧计数为 1。
   - 实际：日志依次记录相应结果，最终 `STATUS` 包含 `last=3 miss=1 dup=0 old=1 link=timeout`。

## 原始证据与结论

- 原始串口日志副本：`evidence/serial-logs/exp-0004-p03-uart-dma-protocol.log`。
- SHA-256：`F550DE06D6FC6281FD376E47A0D22A2299C0AB5E9AB1C88082B66BE253DFCDD8`。
- 说明：原始日志保留早期版本和人为误发文本的输出；当前结论只依据末尾带 `link=`、`order=` 的实测段。重复帧已在板端实测通过，但该结果不在本日志末尾的可检索段中。
- 结论：P03 的 UART DMA 流式接收、帧校验、错帧恢复、通信超时和序号异常观察已完成本轮实测；真实 K230 的帧率、字段语义和安全超时阈值仍待 P04/P08 实测。
