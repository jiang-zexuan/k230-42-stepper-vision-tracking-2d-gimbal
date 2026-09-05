# T05 VisionRxTask 栈溢出定位记录

日期：2026-09-04

## 现象

接入 PV04 文本组帧和解析后，系统曾出现板载 LED 停止闪烁、任务看起来卡住的现象。K230 的视觉数据仍能到达 F407，说明问题不在发送端和 USART2 DMA 接收链路。

## 定位过程

1. 先通过接收统计和 `PV05` 日志确认字节仍在进入 F407。
2. 检查 `VisionRxTask` 的执行路径，发现任务同时使用接收数组、文本行数组、视觉结果结构体，并调用 `sscanf()`、`snprintf()` 和串口发送函数。
3. 原始任务栈配置为 512 Words，即 2048 字节。
4. 临时将栈改为 1024 Words 后系统恢复稳定，这说明栈空间与故障有关，但这一步本身只是验证性实验。
5. 打开 FreeRTOS 栈溢出检测：

   ```c
   #define configCHECK_FOR_STACK_OVERFLOW 2
   ```

6. 实现 `vApplicationStackOverflowHook()`，在调试器中观察 `task_name`。
7. 调试器实际停在 `vApplicationStackOverflowHook()`，并显示：

   ```text
   task_name = "VisionRxTask"
   ```

   因此确认发生栈溢出的任务是 `VisionRxTask`。

## 栈余量监测

FreeRTOS 接口：

```c
uxTaskGetStackHighWaterMark(task_handle)
```

返回任务运行以来历史最低剩余栈空间，单位是 Words。STM32F407 上 1 Word 等于 4 字节。

当前统一由 `TelemetryTask` 调用 `PanView_TaskHeartbeat_CollectRuntimeInfo()`，再保存到心跳记录中的 `stack_high_water_mark_words`。临时定位时也可在 `VisionRxTask` 中直接观察该值。

例如任务栈为 1024 Words，最高水位剩余 489 Words：

```text
历史最大使用量 = 1024 - 489 = 535 Words
``` 

这说明原来的 512 Words 小于当前实测峰值，因此不足。

## HardFault 与栈溢出的区别

- `vApplicationStackOverflowHook()`：FreeRTOS 检测到任务栈越界，能直接给出任务名。
- `HardFault_Handler()`：Cortex-M 底层严重异常处理函数，不一定能直接说明根因。

本次调试中优先命中的是 `vApplicationStackOverflowHook()`，而不是 HardFault，因此可以直接定位到任务级别。

## 最终处理

当前 `VisionRxTask` 使用：

```c
.stack_size = 1024 * 4,
```

即 1024 Words、4096 字节。保留栈溢出检测和最高水位监测，便于后续长期运行观察。

## 经验

增加栈只能解决固定的瞬时峰值。若最高水位随运行时间持续下降，还要继续检查数组越界、递归、异常输入和内存破坏，不能只无限增大栈空间。
