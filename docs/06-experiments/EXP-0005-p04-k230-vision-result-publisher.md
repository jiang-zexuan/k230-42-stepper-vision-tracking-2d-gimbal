# EXP-0005：P04 K230 视觉结果发布器

## 基本信息

- 实验编号：EXP-0005
- 日期：2026-08-16
- 状态：实测通过；置信度字段保持不可用并已记录。
- 目标：验证 K230 人脸检测结果、坐标、无目标状态、帧率和 UART 文本输出。
- 固件提交：`014e6ef`

## 硬件与接线

- K230：标准版，固件标识 `k230_canmv_yahboom`。
- F407：探索者 V3.2；本实验只使用板载 CH340C 作为 USB-TTL 接收器。
- 接收端：P10 两个跳帽均拔掉，K230 12Pin 第 5 脚 `IO32/UART3_TXD` 接 P10 第 4 脚 `RXD`。
- 共地：K230 12Pin 第 4 或第 7 脚 GND 接探索者板 GND。
- 未接：K230 IO33、两板 5V、两板 3.3V；ST-Link 只用于独立调试，不参与 UART 数据流。

## 固件与参数

- CanMV：`v1.4.3`，`k230_canmv_yahboom`。
- 摄像头源图像：`1920x1080`。
- LCD：`800x480`，ST7701。
- 人脸模型：`face_detection_320.kmodel`；模型输入 `320x320`。
- 置信度筛选阈值：`0.5`；NMS 阈值：`0.2`。
- UART：UART3，115200 bit/s；XCOM 设置 115200、8 数据位、无校验、1 停止位、无流控。

## 操作步骤与结果

1. 在 CanMV IDE 运行 `firmware/k230/app/p04_face_result_publisher.py`。
   - 结果：LCD 正常显示，终端持续输出 `PV04`。
2. 先不接外部串口测量 IO32。
   - 结果：未运行程序时约 `3.3 V`；运行发送时万用表约 `3.0 V`。
3. 拔掉 P10 两个跳帽，完成 IO32、GND 到 CH340 RXD 的单向接线。
   - 结果：XCOM 收到连续可读的 `PV04` 文本，无乱码。
4. 观察无人脸和有人脸状态。
   - 结果：日志共 386 行；`target=0` 251 行，`target=1` 135 行；两种状态均包含 FPS。
5. 观察坐标与 FPS。
   - 结果：有目标行包含 `x/y/w/h/cx/cy`；主要 FPS 约 27～30。

## 现场问题与处理

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| IO32 约 `0.33 V` | `YbUart` 默认是 IO9/IO10 的 UART1，不是 12Pin IO32 | 改为 FPIOA 显式 UART3 映射 |
| `UART(3) rx not configured` | UART3 初始化要求 RX 复用也完成 | 内部配置 IO33 为 UART3_RXD，外部不接线 |
| `UART` 没有 `send` | 底层 `machine.UART` 没有包装类 `send` 方法 | 改用 `write()`，发送 ASCII 字节 |

## 原始证据

- XCOM 原始日志：`evidence/serial-logs/exp-0005-p04-k230-uart-result.log`
- SHA-256：`336A8DE5781927ADA432292B38DD4F1BF01C14A5CD5EBDD38F44B9F41EF6A99B`
- 现场原文件：`D:\工具资源\XCOM（正点原子推荐）\p04调试日志.txt`

## 结论与下一步

K230 已完成检测结果到电脑串口的单向文本发布，P04 本轮通过。置信度未从模型后处理结果暴露，已用 `confidence=unavailable` 诚实标记。下一步进入 P05 单轴闭环电机验收，不提前接入视觉运动控制。
