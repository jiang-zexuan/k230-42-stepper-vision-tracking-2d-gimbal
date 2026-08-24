# TEST-0011：P10 FT6336G 触摸与 UI 交互验证

- 测试编号：TEST-0011
- 日期：2026-08-23
- 分支：`feature/f407-p10-dual-axis-visual-tracking`
- 硬件：F407、MSP2834（ILI9341 + FT6336G）
- 接线：`CTP_SCL->PB8`、`CTP_SDA->PB9`、`CTP_RST->PE5`、`CTP_INT->PE6`、共地

## 测试目标

验证 FT6336G 的 I2C 触摸点读取、180 度显示方向坐标映射、区域识别、按下反馈和安全按钮动作。

## 配置与依据

- I2C1：100 kHz、7 bit addressing。
- FT6336G 从地址代码使用 `0x38 << 1` 作为 HAL 地址参数。
- `0x02`：触摸点数量；`0x03~0x06`：第一个触摸点 X/Y。
- 依据：`D:\datasheet-knowledge\parsed\FT6336G_Register\full.md` 普通模式寄存器表。
- 模块接口和触摸总线分离依据：`D:\datasheet-knowledge\parsed\2.8inch_SPI_Module_MSP2833_MSP2834_User_Manual_CN\full.md` 第 2、4.5、4.6 节。

## 实验步骤与结果

| 步骤 | 操作 | 实际结果 | 状态 |
| --- | --- | --- | --- |
| 1 | 上电并复位 FT6336G | `TOUCH probe=ok` | 已通过 |
| 2 | 启动时读取识别寄存器 | `id_low=00 id_high=00 vendor=00` | 待解释，不作为型号结论 |
| 3 | 单指触摸屏幕 | 输出 `TOUCH point count=1` 和原始 X/Y | 已通过 |
| 4 | 依次点击四个角 | X/Y 方向与显示 180 度相反 | 已通过 |
| 5 | 加入坐标反向 | `screen_x=239-raw_x`、`screen_y=319-raw_y` 与视觉方向一致 | 已通过 |
| 6 | 点击四个测试区域 | `zone=top_left/top_right/bottom_left/bottom_right` 正确 | 已通过 |
| 7 | 按下与松手 | 按钮反色，松手恢复 | 已通过 |
| 8 | 点击交互按钮 | 输出 `TOUCH action=...`，动作按安全边界执行 | 已通过 |

## 验收结论

FT6336G 触摸底层和最小 UI 交互已经实测通过。当前仍有两项明确待验证：识别寄存器全 0 的兼容性原因，以及将轮询改为 `CTP_INT` EXTI 通知后的正式实现。

## 原始证据

- `D:\工具资源\XCOM（正点原子推荐）\p10调试日志.txt`
- 关键日志格式：`TOUCH point count=1 raw=(x,y) screen=(x,y) zone=...`
- 关键动作格式：`TOUCH action=mode_tracking_on/off`、`takeover_on/off`、`stop_all`、`info_status`
