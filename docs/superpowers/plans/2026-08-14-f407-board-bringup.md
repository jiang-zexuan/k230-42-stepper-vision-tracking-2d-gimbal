# P01 F407 板级诊断台实施计划

> **供执行代理使用：** 必须按任务逐项执行，并在每项之间复核。每一步使用复选框跟踪。

**目标：** 为探索者 V3.2 建立并实测 STM32F407 的 SWD 下载调试、DS0 状态指示、KEY0 按键事件和 USART1 诊断日志。

**架构：** App/board_diagnostics.c 只负责与硬件无关的 LED/计数状态转换，并用本机 gcc 测试。CubeIDE 负责 ioc、HAL、时钟、GPIO、USART1 和 SWD 配置。Core/Src/main.c 只在 CubeMX 的 USER CODE 区域把纯逻辑接到 HAL。

**技术栈：** STM32CubeIDE 1.19.0、STM32CubeMX、STM32 HAL、ST-Link SWD、经 P10 跳线实测确认的 CH340C、Windows gcc、PowerShell。

---

## 文件边界

| 文件 | 责任 |
| --- | --- |
| docs/01-requirements/system-requirements.md | 增加 REQ-010。 |
| docs/07-test/traceability-matrix.md | 将 REQ-010 追踪到配置、测试和原始证据。 |
| firmware/stm32f407/PanViewF407.ioc | F407ZGT6、SWD、GPIO 和已验证 USART1 的 CubeMX 真值源。 |
| firmware/stm32f407/App/board_diagnostics.[ch] | 与硬件无关的诊断状态转换。 |
| firmware/stm32f407/Tests/board_diagnostics_test.c | 本机 C 状态测试。 |
| firmware/stm32f407/Core/Src/main.c | DS0、KEY0、USART1 的 HAL 适配。 |
| docs/06-experiments/EXP-0002-f407-board-bringup.md | 完整的 P01 实验记录。 |
| docs/07-test/TEST-0001-f407-board-bringup.md | REQ-010 板级测试报告。 |
| evidence/serial-logs/exp-0002-usart1.log | 原始 USART1 日志。 |

### 任务 1：完成文字版实物证据门禁

本任务不要求拍照。请在上电或接线前逐项文字确认；任何一项不能确认，就记录为“待验证”，不得用猜测替代。

- [ ] **步骤 1：确认板卡身份**

确认实物丝印与芯片顶标：探索者 V3.2、STM32F407ZGT6。当前这两项已由用户确认。

- [ ] **步骤 2：确认安全连接范围**

使用开发板逻辑 USB 供电，使用外置 ST-Link 连接 20 针接口的 SWD 信号。不得连接 K230、电机、12 V 或任何未经核验的外部信号线。

- [ ] **步骤 3：确认 ST-Link 的 SWD 接线和枚举**

根据 ST-Link 和开发板手册确认 SWDIO、SWCLK、GND 以及所需供电脚/复位脚在双方连接器上的实际名称或位置；随后用文字记录设备管理器中的 ST-Link 条目。设备缺失是实测结果，不得因此试换 GPIO。

- [ ] **步骤 4：执行 USART1 门禁**

文字确认 P10 的两个跳线帽均已安装：1-2 短接 PA10/USART1_RX 与 CH340C TXD，3-4 短接 PA9/USART1_TX 与 CH340C RXD；随后记录 USB_UART 连接后设备管理器中的 CH340C COM 号。只有两项都确认，才允许启用 USART1；任一条件不满足时，保持 USART1 禁用，在 EXP-0002 中记录原因，REQ-010 保持待验证。

- [ ] **步骤 5：确认 BOOT 和复位条件**

按照手册确认当前 BOOT 跳线处于正常用户 Flash 启动位置，并确认 RESET 按键可用。此处只记录“正常 Flash 启动/无法确认”，不自行移动跳线试验。

### 任务 2：增加 P01 需求追踪

**文件：**
- 修改：docs/01-requirements/system-requirements.md
- 修改：docs/07-test/traceability-matrix.md

- [ ] **步骤 1：增加 REQ-010**

在需求表追加：

~~~markdown
| REQ-010 | F407 板级诊断应通过经资料核验并经实物确认的 SWD、状态 LED、按键和调试串口，提供可复现的下载、状态指示、按键事件和启动日志证据。 |
~~~

- [ ] **步骤 2：增加初始追踪行**

追加以下内容，状态保持“待板级实测”：

~~~markdown
| REQ-010 | firmware/stm32f407/PanViewF407.ioc、App/board_diagnostics.*、Core/Src/main.c | docs/07-test/TEST-0001-f407-board-bringup.md | EXP-0002 文字记录、evidence/serial-logs/exp-0002-usart1.log | 待板级实测 |
~~~

- [ ] **步骤 3：检查并提交文档基线**

~~~powershell
git diff --check -- docs/01-requirements/system-requirements.md docs/07-test/traceability-matrix.md
git add docs/01-requirements/system-requirements.md docs/07-test/traceability-matrix.md
git commit -m "docs: 增加 P01 板级诊断需求追踪"
~~~

预期：检查无输出，提交只包含这两个文档文件。

### 任务 3：测试并实现纯诊断状态

**文件：**
- 新建：firmware/stm32f407/Tests/board_diagnostics_test.c
- 新建：firmware/stm32f407/App/board_diagnostics.h
- 新建：firmware/stm32f407/App/board_diagnostics.c

- [ ] **步骤 1：先写失败测试**

创建 firmware/stm32f407/Tests/board_diagnostics_test.c：

~~~c
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../App/board_diagnostics.h"

static void test_init_keeps_led_off_and_counter_zero(void)
{
    BoardDiagnosticsState state;
    BoardDiagnostics_Init(&state);
    assert(state.status_led_on == false);
    assert(state.button_press_count == 0U);
}

static void test_each_key0_press_toggles_led_and_increments_counter(void)
{
    BoardDiagnosticsState state;
    BoardDiagnostics_Init(&state);

    BoardDiagnostics_OnKey0Pressed(&state);
    assert(state.status_led_on == true);
    assert(state.button_press_count == 1U);

    BoardDiagnostics_OnKey0Pressed(&state);
    assert(state.status_led_on == false);
    assert(state.button_press_count == 2U);
}

int main(void)
{
    test_init_keeps_led_off_and_counter_zero();
    test_each_key0_press_toggles_led_and_increments_counter();
    puts("board_diagnostics_test: PASS");
    return 0;
}
~~~

- [ ] **步骤 2：运行并确认失败原因**

~~~powershell
New-Item -ItemType Directory -Force "$env:TEMP\panview-tests" | Out-Null
gcc -std=c11 -Wall -Wextra -Werror firmware/stm32f407/App/board_diagnostics.c firmware/stm32f407/Tests/board_diagnostics_test.c -o "$env:TEMP\panview-tests\board_diagnostics_test.exe"
~~~

预期：因生产头文件和源文件尚不存在而编译失败。

- [ ] **步骤 3：实现最小纯逻辑模块**

创建 firmware/stm32f407/App/board_diagnostics.h：

~~~c
#ifndef PANVIEW_BOARD_DIAGNOSTICS_H
#define PANVIEW_BOARD_DIAGNOSTICS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool status_led_on;
    uint32_t button_press_count;
} BoardDiagnosticsState;

void BoardDiagnostics_Init(BoardDiagnosticsState *state);
void BoardDiagnostics_OnKey0Pressed(BoardDiagnosticsState *state);

#endif
~~~

创建 firmware/stm32f407/App/board_diagnostics.c：

~~~c
#include "board_diagnostics.h"

void BoardDiagnostics_Init(BoardDiagnosticsState *state)
{
    state->status_led_on = false;
    state->button_press_count = 0U;
}

void BoardDiagnostics_OnKey0Pressed(BoardDiagnosticsState *state)
{
    state->status_led_on = !state->status_led_on;
    state->button_press_count++;
}
~~~

- [ ] **步骤 4：运行通过测试并提交**

~~~powershell
gcc -std=c11 -Wall -Wextra -Werror firmware/stm32f407/App/board_diagnostics.c firmware/stm32f407/Tests/board_diagnostics_test.c -o "$env:TEMP\panview-tests\board_diagnostics_test.exe"
& "$env:TEMP\panview-tests\board_diagnostics_test.exe"
git add firmware/stm32f407/App/board_diagnostics.h firmware/stm32f407/App/board_diagnostics.c firmware/stm32f407/Tests/board_diagnostics_test.c
git commit -m "test: 增加 P01 诊断状态单元测试"
~~~

预期输出：board_diagnostics_test: PASS。

### 任务 4：生成有证据依据的 CubeIDE 工程

**文件：**
- 新建：firmware/stm32f407/PanViewF407.ioc
- 新建：firmware/stm32f407/Core/Inc/main.h
- 新建：firmware/stm32f407/Core/Src/main.c
- 新建：firmware/stm32f407/Drivers/
- 新建：firmware/stm32f407/STM32F407ZGTX_FLASH.ld

- [ ] **步骤 1：创建工程并只配置有来源的功能**

在 D:\个人能力补齐\PanView\firmware\stm32f407 创建 PanViewF407，芯片选择 STM32F407ZGT6，SYS > Debug 选择 Serial Wire。时钟先使用 CubeMX 的 HSI 初始配置，并在 EXP-0002 记录生成的时钟树为待板级验证。PF9 配置为 GPIO 输出，用户标签 BOARD_STATUS_LED，初始高电平、无上下拉、推挽、低速。PE4 配置为 GPIO 输入，用户标签 BOARD_KEY0，内部上拉。

- [ ] **步骤 2：只有门禁通过时才配置 UART**

P10 和 CH340C 均已证明后，USART1 配置为异步模式：PA9 TX、PA10 RX、115200 bit/s、8 数据位、无校验、1 停止位、无流控。115200 bit/s 是 P01 设计参数，不是板卡既有事实。门禁未通过时保持 USART1 禁用，不加入 UART 代码。

- [ ] **步骤 3：生成代码并保留用户区**

设置 Project Manager > Code Generator > Keep User Code when re-generating，生成代码并确认存在：

~~~text
firmware/stm32f407/PanViewF407.ioc
firmware/stm32f407/Core/Inc/main.h
firmware/stm32f407/Core/Src/main.c
firmware/stm32f407/Drivers/
~~~

将 App 加入编译器头文件路径，只在 main.c 的 USER CODE 区域编辑。

- [ ] **步骤 4：加入 HAL 适配**

在 main.c 的 USER CODE 区域加入以下代码，保留 CubeMX 生成的初始化和错误处理：

~~~c
/* USER CODE BEGIN Includes */
#include "board_diagnostics.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
static BoardDiagnosticsState board_diagnostics;
static GPIO_PinState previous_key0_state;
static uint32_t diagnostic_counter;
static uint32_t last_counter_log_ms;
enum {
    BOARD_DIAGNOSTIC_SCAN_PERIOD_MS = 10U,
    BOARD_DIAGNOSTIC_COUNTER_PERIOD_MS = 1000U,
    UART_LOG_TX_TIMEOUT_MS = 100U
};
/* 单位：ms。来源：P01 设计；仅适用于本轮轮询诊断。 */
/* USER CODE END PV */

/* USER CODE BEGIN 0 */
static void ApplyStatusLed(void)
{
    HAL_GPIO_WritePin(BOARD_STATUS_LED_GPIO_Port, BOARD_STATUS_LED_Pin,
                      board_diagnostics.status_led_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void EmitLog(const char *message)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)message, (uint16_t)strlen(message),
                      UART_LOG_TX_TIMEOUT_MS);
}

static void EmitDiagnosticCounterIfDue(void)
{
    uint32_t now_ms = HAL_GetTick();

    if ((now_ms - last_counter_log_ms) >= BOARD_DIAGNOSTIC_COUNTER_PERIOD_MS) {
        char message[80];

        diagnostic_counter++;
        (void)snprintf(message, sizeof(message), "counter=%lu\r\n",
                       (unsigned long)diagnostic_counter);
        EmitLog(message);
        last_counter_log_ms = now_ms;
    }
}

static void PollKey0(void)
{
    GPIO_PinState current_key0_state = HAL_GPIO_ReadPin(BOARD_KEY0_GPIO_Port, BOARD_KEY0_Pin);

    if ((previous_key0_state == GPIO_PIN_SET) && (current_key0_state == GPIO_PIN_RESET)) {
        char message[80];

        BoardDiagnostics_OnKey0Pressed(&board_diagnostics);
        ApplyStatusLed();
        (void)snprintf(message, sizeof(message), "KEY0 count=%lu\r\n",
                       (unsigned long)board_diagnostics.button_press_count);
        EmitLog(message);
    }

    previous_key0_state = current_key0_state;
}
/* USER CODE END 0 */
~~~

在所有 MX_*_Init 调用之后加入：

~~~c
BoardDiagnostics_Init(&board_diagnostics);
previous_key0_state = HAL_GPIO_ReadPin(BOARD_KEY0_GPIO_Port, BOARD_KEY0_Pin);
last_counter_log_ms = HAL_GetTick();
ApplyStatusLed();
EmitLog("PanView P01 boot\r\n");
~~~

在 while (1) 中加入：

~~~c
PollKey0();
EmitDiagnosticCounterIfDue();
HAL_Delay(BOARD_DIAGNOSTIC_SCAN_PERIOD_MS);
~~~

门禁未通过时，删除 EmitLog、EmitDiagnosticCounterIfDue、snprintf、string.h 和所有 huart1 引用，只保留 LED/KEY0 适配，并记录 UART 阻塞原因。

- [ ] **步骤 5：构建并提交工程**

运行任务 3 的主机测试，然后在 CubeIDE 执行 Project > Build All，配置选择 Debug。预期主机测试输出 PASS，CubeIDE 显示 Build Finished 且无错误，并生成本地 Debug/PanViewF407.elf。不要添加 Debug/。

~~~powershell
git add firmware/stm32f407/PanViewF407.ioc firmware/stm32f407/Core firmware/stm32f407/Drivers firmware/stm32f407/STM32F407ZGTX_FLASH.ld
git commit -m "feat: 建立 P01 F407 板级诊断工程"
~~~

### 任务 5：执行 SWD、LED、按键和 UART 实测

**文件：**
- 新建：evidence/serial-logs/exp-0002-usart1.log
- 新建：docs/06-experiments/EXP-0002-f407-board-bringup.md

- [ ] **步骤 1：先验证 SWD**

通过 ST-Link 启动 CubeIDE Debug，在 main 停止，在 PollKey0 设置断点，查看 board_diagnostics.button_press_count，单步一次后继续运行。记录确切结果。连接失败时把 CubeIDE 原始错误写入 EXP-0002 并停止本轮。

- [ ] **步骤 2：验证 LED 和按键**

复位后观察 DS0 熄灭。逐次按下并释放 KEY0，观察 DS0 状态变化，并记录第一次按下后计数从 0 变化。行为矛盾时记录失败，不得替换 PF9 或 PE4。

- [ ] **步骤 3：在门禁允许时捕获 UART**

使用已枚举的 CH340C COM 口，以 115200 bit/s、8N1、无流控打开终端，将未编辑的输出保存为 evidence/serial-logs/exp-0002-usart1.log。日志必须包含 PanView P01 boot、至少两条递增的 counter=、一条 KEY0 count=，以及 RESET 后第二条 PanView P01 boot。

- [ ] **步骤 4：诚实记录 UART 阻塞**

门禁未通过时，在 EXP-0002 中记录缺失的 P10 或 CH340C 证据，将 UART 标为待验证，不创建伪造日志，不将 REQ-010 标为通过。

### 任务 6：闭合实验、测试和追踪

**文件：**
- 修改：docs/06-experiments/index.md
- 新建：docs/07-test/TEST-0001-f407-board-bringup.md
- 修改：docs/07-test/traceability-matrix.md

- [ ] **步骤 1：根据原始证据完成 EXP-0002**

从实验模板创建 EXP-0002，填写编号、目标、日期、硬件/跳线、供电、固件提交、配置、步骤、预期/实际结果、原始证据、问题/原因、结论和下一步。只有有对应文字确认、调试结果或原始日志时才写“实测”，否则写“待验证”或“失败”。

- [ ] **步骤 2：登记 EXP-0002 并编写 TEST-0001**

在实验索引追加一行 EXP-0002，并从测试报告模板创建 TEST-0001。测试步骤和实际结果必须分别覆盖下载、断点/变量检查、DS0 初始状态、KEY0 状态变化、启动日志、递增计数日志、KEY0 日志和复位日志。

- [ ] **步骤 3：更新 REQ-010 状态**

将 REQ-010 的证据字段替换为具体 EXP-0002、TEST-0001 和串口日志路径。所有行为均有原始证据时才写“实测通过”；缺少 SWD、P10、CH340C、LED、KEY0 或 UART 任一项时，保持“待板级实测”并在 TEST-0001 写明缺项。

- [ ] **步骤 4：验证、提交并推送**

~~~powershell
gcc -std=c11 -Wall -Wextra -Werror firmware/stm32f407/App/board_diagnostics.c firmware/stm32f407/Tests/board_diagnostics_test.c -o "$env:TEMP\panview-tests\board_diagnostics_test.exe"
& "$env:TEMP\panview-tests\board_diagnostics_test.exe"
git diff --check
git add docs/06-experiments/EXP-0002-f407-board-bringup.md docs/06-experiments/index.md docs/07-test/TEST-0001-f407-board-bringup.md docs/07-test/traceability-matrix.md evidence/serial-logs
git commit -m "test: 记录 P01 F407 板级诊断实测"
git push origin feature/f407-board-bringup
~~~

预期：主机测试输出 board_diagnostics_test: PASS，git diff --check 无输出，推送成功。若任一要求仍未验证，提交信息改为“test: 记录 P01 F407 板级诊断待验证结果”。
