# K230 固件与模型

本目录存放 K230 的应用脚本、模型元数据、部署工具和测试。原始模型文件通常较大，默认不提交；文档应记录来源、版本、量化方式、输入尺寸和验证证据。

## P04：视觉结果发布器

`app/p04_face_result_publisher.py` 是独立的 PanView 脚本，不修改厂商的 `face_detection.py`。它复用厂商例程相同的人脸检测模型、预处理和显示流程，把每帧检测结果打印到 CanMV IDE 终端，并显式将 12Pin 的 IO32 复用为 UART3_TXD，以 115200 bit/s 输出相同文本。

已确认：厂商例程把每个检测结果的前四个字段作为 `x/y/w/h`，并以 `rgb888p_size=[1920, 1080]` 的相机源图像坐标绘制检测框。因此本脚本输出的 `x/y/w/h/cx/cy` 同样是 1920x1080 源图像坐标，而不是显示缩放后的坐标。

根据实测固件标识 `k230_canmv_yahboom` 和报错中的 `ST7701`，当前运行入口使用实体 LCD `800x480`；1920x1080 只保留给摄像头源图像。知识库基础例程也展示了 ST7701 使用 800x480（`K230_02.基础例程_精选合集`）。

已实测：`raw_dets_nonempty` 为 `array([[x, y, w, h]], dtype=float32)`，每个检测结果只有四个框坐标，没有第 5 个置信度字段。因此结果行使用 `confidence=unavailable`，不伪造数值；`confidence_threshold=0.5` 仍由官方 `aidemo.face_det_post_process` 在内部参与筛选，但当前结果结构没有把分数返回给应用层。

### 模型资源路径

不同 CanMV K230 资料包使用过不同的资源目录。依据资料中出现的示例，脚本按以下顺序检查完整的“模型 + 锚点”组合：

```text
/sdcard/examples/kmodel/face_detection_320.kmodel
/sdcard/examples/utils/prior_data_320.bin

/sdcard/kmodel/face_detection_320.kmodel
/sdcard/utils/prior_data_320.bin

/sdcard/app/tests/kmodel/face_detection_320.kmodel
/sdcard/app/tests/utils/prior_data_320.bin
```

只有同一组中的两个文件都存在时才使用该组；三组都不存在时，脚本会列出检查过的路径并停止。这样不会把某个 CanMV 固件版本的目录结构误当成所有版本的固定路径。

运行步骤：

1. 保持 K230 的官方人脸检测例程可运行。
2. 在 CanMV IDE 打开 `app/p04_face_result_publisher.py` 并运行。
3. 在终端观察 `PV04 start ... display=800x480`、最多五行 `PV04 raw_dets=...` 和连续 `PV04 ...` 结果行。
4. 分别观察有人脸和无人脸时的 `target=1`、`target=0`。
5. UART 接线前，先测量 K230 的 IO32 对 GND 空闲电压。K230 标准版资料只确认第 5 脚为 `IO32 / UART3_TXD`，未在当前资料中给出其逻辑电平，因此未测量前不得接到外部接收端。
6. 脚本会在 K230 内部同时配置 IO32 为 UART3_TXD、IO33 为 UART3_RXD，这是 CanMV 创建 UART3 的驱动条件；本阶段外部仍只使用发送方向。电平确认后，拔掉探索者 V3 P10 的两个跳帽。仅接 K230 第 5 脚 IO32 到 P10 第 4 脚 `RXD`（CH340 接收），再接 K230 GND（第 4 或第 7 脚）到 F407 GND；不接 IO33，也不互接两个板子的 5 V 或 3.3 V。XCOM 使用 `COM8`、115200 bit/s、8 数据位、无校验、1 停止位、无流控，观察连续 `PV04` 行。

结果行契约：

```text
PV04 t_ms=1234 target=1 count=2 x=720 y=180 w=300 h=300 cx=870.0 cy=330.0 confidence=unavailable fps=24.6
PV04 t_ms=1300 target=0 count=0 confidence=unavailable fps=25.0
```

`count` 是本帧检测到的人脸数。存在多张脸时，P04 暂按检测框面积选择最大的一个作为主目标；这是验证发布链路的确定性规则，不是最终跟踪策略。

依据：知识库 `K230_02.基础例程_精选合集` 的“YAHBOOM K230 引脚”（IO32 可复用为 UART3_TXD）与“通讯接口”（`YbUart` 默认配置的是 IO9/UART1_TXD，不能用于本次 12Pin IO32 输出）；厂商源码 `K230视觉模块资料/程序源码/02.Basic/05.uart.py`（115200 bit/s 与文本 `send`）；CanMV IDE 官方示例 `04-AI-Demo/face_detection.py`；探索者 V3 手册的“2.1.3 USB 串口/串口 1 选择接口”；现场证据 `evidence/serial-logs/exp-0005-p04-k230-lcd-result.log` 与 `p402.txt`。
