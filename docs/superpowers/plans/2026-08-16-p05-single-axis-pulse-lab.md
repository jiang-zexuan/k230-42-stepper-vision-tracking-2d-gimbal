# P05 Single-Axis Pulse Lab Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Use KEY0 to safely toggle a 100 Hz TIM4 STEP output for one X42S motor, with explicit direction, enable, stop, and serial state logs.

**Architecture:** Keep the two-state `stopped`/`running` decision in a HAL-independent module so it has a host unit test. `main.c` is the hardware adapter: it writes `MOTOR_DIR` and `MOTOR_EN`, starts or stops `TIM4_CH1`, and reports the result through existing USART1.

**Tech Stack:** STM32CubeIDE, STM32 HAL, TIM4 PWM on PB6, C host tests using MinGW GCC.

---

### Task 1: Test the run-state transition

**Files:**
- Create: `firmware/stm32f407/PanViewF407/Core/Inc/motor_pulse_lab.h`
- Create: `firmware/stm32f407/PanViewF407/Core/Src/motor_pulse_lab.c`
- Create: `firmware/stm32f407/Tests/motor_pulse_lab_test.c`

- [ ] **Step 1: Write the failing test**

```c
MotorPulseLab lab;

MotorPulseLab_Init(&lab);
assert(MotorPulseLab_GetState(&lab) == MOTOR_PULSE_LAB_STOPPED);
assert(MotorPulseLab_Toggle(&lab) == MOTOR_PULSE_LAB_RUNNING);
assert(MotorPulseLab_Toggle(&lab) == MOTOR_PULSE_LAB_STOPPED);
```

- [ ] **Step 2: Run the test to verify it fails**

Run:

```text
gcc -std=c11 -Wall -Wextra -Werror firmware/stm32f407/Tests/motor_pulse_lab_test.c firmware/stm32f407/PanViewF407/Core/Src/motor_pulse_lab.c -o firmware/stm32f407/Tests/motor_pulse_lab_test.exe
```

Expected: compilation fails because `motor_pulse_lab.h` and its implementation do not exist.

- [ ] **Step 3: Add the minimal run-state module**

```c
typedef enum {
  MOTOR_PULSE_LAB_STOPPED = 0,
  MOTOR_PULSE_LAB_RUNNING
} MotorPulseLabState;

typedef struct {
  MotorPulseLabState state;
} MotorPulseLab;

void MotorPulseLab_Init(MotorPulseLab *lab);
MotorPulseLabState MotorPulseLab_Toggle(MotorPulseLab *lab);
MotorPulseLabState MotorPulseLab_GetState(const MotorPulseLab *lab);
```

- [ ] **Step 4: Run the host test to verify it passes**

Run:

```text
firmware/stm32f407/Tests/motor_pulse_lab_test.exe
```

Expected: `motor_pulse_lab_test: PASS`.

### Task 2: Apply the state to TIM4 and the motor control pins

**Files:**
- Modify: `firmware/stm32f407/PanViewF407/Core/Src/main.c`
- Modify: `firmware/stm32f407/PanViewF407/PanViewF407.ioc`

- [ ] **Step 1: Include generated TIM4 and the state module**

Add `#include "tim.h"` and `#include "motor_pulse_lab.h"` in `main.c`. Confirm `MX_TIM4_Init()` runs with the generated peripheral initialization.

- [ ] **Step 2: Add explicit start and stop adapters**

`MotorPulseLab_StartHardware()` sets `MOTOR_DIR` low, sets `MOTOR_EN` high, then calls `HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1)`. `MotorPulseLab_StopHardware()` calls `HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1)` before setting `MOTOR_EN` low. HAL failures call `Error_Handler()`.

- [ ] **Step 3: Keep the boot state stopped**

Immediately after initialization, stop TIM4 channel 1 and write `MOTOR_EN` low. Do not start PWM from initialization or timer configuration.

- [ ] **Step 4: Toggle only on the existing debounced KEY0 press event**

On each accepted KEY0 press event, toggle the run-state, invoke exactly one hardware adapter, and publish either `MOTOR state=running dir=forward step_hz=100` or `MOTOR state=stopped` using USART1.

- [ ] **Step 5: Build the STM32 project**

Run:

```text
make -C firmware/stm32f407/PanViewF407/Debug -j4 all
```

Expected: `PanViewF407.elf` builds with exit code 0.

### Task 3: Verify the acceptance path and record evidence

**Files:**
- Modify: `docs/05-learning-log/p05-single-axis-closed-loop-motor-review.md`
- Modify: `docs/06-experiments/EXP-0006-p05-single-axis-pulse-lab.md`
- Modify: `docs/07-test/TEST-0005-p05-single-axis-pulse-lab.md`
- Create: `evidence/serial-logs/exp-0006-p05-single-axis-pulse-lab.log`

- [ ] **Step 1: Confirm the motor menu before connecting motor power**

Set `P_Pul` to `PUL_ENA` and set `En` to `H`; record the displayed configuration. Keep the shaft unloaded and clear of fingers and cable interference.

- [ ] **Step 2: Apply independently verified motor power**

Connect the motor supply only after confirming its voltage is inside the actual X42S version range. Do not connect motor `V+` or its supply to F407 `5V` or `3.3V`.

- [ ] **Step 3: Observe one start-stop cycle**

With F407 firmware running, press KEY0 once to verify slow forward movement and the running log. Press KEY0 again to verify no further STEP output, motor disable, and the stopped log.

- [ ] **Step 4: Commit only after evidence is available**

Run `git diff --check`, add only P05 code, test, documents, and evidence, commit with a Chinese Conventional Commit message, then push the current feature branch.
