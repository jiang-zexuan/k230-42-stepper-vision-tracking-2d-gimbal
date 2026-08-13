# 系统状态机

状态流：BOOT -> MANUAL_CENTER -> SELF_TEST -> SEARCH -> TRACKING -> LOCKED。目标丢失或故障均会进入 LOST 或 FAULT；FAULT 处理后需重新人工回中。

| 状态 | 进入条件 | 行为 | 离开条件 |
| --- | --- | --- | --- |
| BOOT | 上电 | 初始化且禁止运动 | 操作者确认回中 |
| MANUAL_CENTER | 等待人工回中 | 记录零点前不跟踪 | 回中确认 |
| SELF_TEST | 零点建立 | 检查关键任务与接口 | 成功到 SEARCH；异常到 FAULT |
| SEARCH | 无有效目标 | 电机停止或执行后续定义的受限搜索 | 获得有效目标到 TRACKING |
| TRACKING | 有有效目标 | 计算受限双轴运动 | 稳定居中到 LOCKED；目标丢失到 LOST |
| LOCKED | 居中持续 500 ms | 输出锁定提示 | 偏差增大或丢失 |
| LOST | 超时或目标丢失 | 在 300 ms 内停止电机 | 新目标到 TRACKING；故障到 FAULT |
| FAULT | 保护条件触发 | 停止运动并保留故障码 | 人工处理后重新回中 |
