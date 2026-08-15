# P01 F407 Board Bring-Up Design

## Status

- Status: design, pending board-level verification.
- Scope: P01 only. Create the reusable F407 diagnostic baseline for LED, button, UART logging, and SWD debugging.
- Out of scope: K230, motors, drivers, external signal wiring, DMA, protocol parsing, FreeRTOS, and motion control.

## Evidence Baseline

- Board: ZhiDianYuanZi Explorer V3.2, STM32F407ZGT6 (LQFP144).
- Physical board identity: confirmed by the operator on 2026-08-14.
- Board manual source: `C:/Users/a7864/Desktop/探索者V3 硬件参考手册_V1.0.pdf`.
- Manual PDF page 10: USB-UART uses header P10. The CH340C data signals connect to USART1 PA9 (TXD) and PA10 (RXD) only when the P10 jumpers are installed.
- Manual PDF page 31: DS0 is PF9 and DS1 is PF10. Both LEDs are connected to 3.3 V through resistors, so driving the corresponding GPIO low turns the LED on. KEY0 is PE4, KEY1 is PE3, and KEY2 is PE2. Those three keys connect to ground when pressed and require an internal pull-up. KEY_UP is PA0 and is high-level active.
- Manual PDF pages 10 and 39: the 20-pin JTAG connector supports SWD; SWD is the preferred debug mode.

The manual establishes the candidate board mapping. P10 jumper state, ST-Link connection, USB device enumeration, LED polarity, button state, UART log output, reset behavior, and download/debug behavior remain pending board-level measurement.

## Candidate Hardware Configuration

| Function | Candidate signal | Electrical behavior | Evidence status |
| --- | --- | --- | --- |
| Status LED | DS0 / PF9 | GPIO output; low turns LED on | Manual verified; pending board test |
| Button | KEY0 / PE4 | GPIO input with internal pull-up; press reads low | Manual verified; pending board test |
| Debug | SWD via 20-pin JTAG header | ST-Link connects in SWD mode | Manual verified; pending board test |
| Log UART | USART1 PA9/PA10 via P10 and CH340C | Requires physical P10 jumper and USB enumeration check | Manual verified; pending board test |

No other pin is selected by this design. Empty entries in the project pin map remain unavailable until their own evidence is recorded.

## Firmware Boundary

Create an STM32CubeIDE project for STM32F407ZGT6 and preserve the generated `.ioc` file. Configure SWD, DS0, and KEY0 from the candidate mapping above. Configure USART1 only after the P10 jumper condition has been confirmed.

The application has one diagnostic flow:

1. Initialize the selected peripherals.
2. Set DS0 to a deterministic initial state of off.
3. Emit a startup identifier and incrementing diagnostic counter when UART is available.
4. Detect a KEY0 press, toggle DS0, and emit a button event when UART is available.
5. Allow reset to repeat the startup sequence.

No inferred timing, clock, or baud-rate constant is committed in this design. Each generated configuration value must cite its STM32CubeIDE or STM32 documentation source and be validated by the matching board-level observation.

## Board-Level Test Design

Perform tests in this order:

1. Record photos of the board front and back, the P10 and BOOT jumper states, the SWD connection, and the USB connection.
2. Record the ST-Link and CH340C entries shown by Windows Device Manager. Absence of a device is a test result, not a reason to select a different pin.
3. Build, download, halt, single-step, inspect a variable, and set a breakpoint through SWD.
4. Observe DS0 after power-on and after each KEY0 press.
5. If P10 and CH340C enumeration are confirmed, capture startup, counter, KEY0-event, and reset logs from USART1.

## Failure Handling

If ST-Link is unavailable, SWD download fails, no serial COM port appears, the LED does not match the expected active-low behavior, the button does not read as expected, or reset does not restart the log, stop that test item. Preserve the reproduction steps and raw evidence, create an experiment or issue record, and return to the board manual, silk screen, jumper state, and physical observations. Do not probe alternative GPIOs by trial.

## Acceptance Evidence

The P01 acceptance package must contain:

- Generated `.ioc` file and source code revision.
- One `EXP-` experiment record with hardware, connection, configuration, expected result, observed result, raw evidence, conclusion, and next step.
- Raw serial log under `evidence/serial-logs/` when UART is verified.
- Board, jumper, and SWD photos under `evidence/images/`.
- Board-level test report and traceability-matrix links.
- A single-purpose Chinese Conventional Commit after the tested evidence is recorded.

## Safety Constraints

- Use only the board's USB logic supply for P01.
- Connect only the ST-Link SWD probe and already documented USB paths.
- Do not connect K230, motor drivers, motors, 12 V supplies, or external signal wiring during P01.
