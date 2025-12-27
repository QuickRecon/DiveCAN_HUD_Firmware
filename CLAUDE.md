# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an O2 HUD (Heads-Up Display) for CCR (Closed-Circuit Rebreather) diving equipment, built on an STM32L431 microcontroller. The device displays PPO2 (Partial Pressure of O2) values from diving oxygen sensors using RGB LEDs and communicates via DiveCAN protocol over CAN bus.

**Target Hardware**: STM32L431xx (ARM Cortex-M4F with FPU)

## Build System

### Building the Project

```bash
# Full build (uses 20 parallel jobs by default)
make -j20

# Clean build
make clean
make -j20

# Build artifacts location
# - build/STM32.elf (executable for debugging)
# - build/STM32.hex (hex file for flashing)
# - build/STM32.bin (binary file)
```

### Running Unit Tests

```bash
cd Tests

# Build and run all tests
make test

# Build tests only
make

# Run tests with verbose output
make verbose_test

# List available tests
make list_tests

# Clean test build artifacts
make clean
```

Tests are built using CppUTest framework (included as git submodule). See `Tests/README.md` for detailed testing documentation.

### Toolchain Requirements

- `arm-none-eabi-gcc` (ARM GCC toolchain)
- GNU Make
- For debugging: ST-Link debugger and Cortex-Debug VS Code extension

### VS Code Integration

- **Build task**: Ctrl+Shift+B or "Tasks: Run Build Task"
- **Debug**: F5 (requires ST-Link hardware debugger connected)
  - Pre-launch builds automatically
  - Uses SVD file at `/opt/stm32cubeide/plugins/com.st.stm32cube.ide.mcu.productdb.debug_2.2.300.202510101821/resources/cmsis/STMicroelectronics_CMSIS_SVD/STM32L431.svd`

### Compiler Flags

The build uses **aggressive warning flags and static analysis** (`-Werror`, `-fanalyzer`, `-pedantic`, etc.) with stack protection enabled. The codebase follows **C23 standard** (`-std=gnu23`).

## Coding Standards

### NASA's 10 Rules for Safety-Critical Code

This project follows NASA's "Power of Ten" coding rules for developing safety-critical embedded systems. These rules are designed to eliminate common programming errors and improve code verifiability:

1. **Avoid complex flow constructs**: No `goto` statements or recursion. Use simple control flow that is easy to verify.

2. **All loops must have fixed bounds**: Every loop must have a statically determinable upper bound to prevent runaway code and ensure predictable worst-case execution time.

3. **Avoid heap memory allocation**: No dynamic memory allocation (`malloc`, `calloc`, `realloc`) after initialization. This prevents memory leaks, fragmentation, and allocation failures during runtime.

4. **Restrict functions to a single printed page**: Functions should be no longer than ~60 lines (one printed page). This improves readability and testability.

5. **Use a minimum of two runtime assertions per function**: Include assertions to verify preconditions, postconditions, and invariants. This catches errors early during testing.

6. **Restrict the scope of data to the smallest possible**: Declare data at the smallest scope (function-local preferred over file-scope, file-scope over global). Use `static` keyword to limit visibility.

7. **Check return values of all non-void functions**: Always check return values or explicitly cast to `(void)` if intentionally ignored. This ensures error conditions are not silently dropped.

8. **Use the preprocessor sparingly**: Limit preprocessor use to file inclusion and simple conditional compilation. Avoid complex macros that obscure code flow.

9. **Limit pointer use to single dereference**: Avoid multi-level pointers (e.g., `**ptr`). Do not use function pointers where possible, as they complicate static analysis.

10. **Compile with all possible warnings**: Enable maximum warning levels and treat warnings as errors (`-Werror`). Address all warnings before release.

**Current Compliance Status**: The codebase is being audited for compliance. See audit reports for details on conformance and any justified deviations.

**Known Deviations**:
- FreeRTOS uses heap allocation (heap_5) - required by RTOS architecture
- Function pointers used in HAL callbacks - required by STM32 HAL design
- Some generated code may not fully comply - see STM32CubeMX sections

## Architecture Overview

### RTOS Architecture (FreeRTOS)

The application uses **FreeRTOS with CMSIS-RTOS v2 wrapper** and follows a multi-threaded producer-consumer pattern:

**Task Priority Philosophy**:
- Data consumers have **higher priority** than data producers (defined in `Core/Src/common.h`)
- Consumers block on queues waiting for fresh data, preventing priority inversion
- Watchdog task runs just above idle to catch starvation issues

**Key Tasks** (from `Core/Src/main.c`):
- **TSCTask**: Touch sensor processing (osPriorityNormal, 128 words stack)
- **BlinkTask**: RGB LED control for PPO2 display (osPriorityAboveNormal, 512 words stack)
- **AlertTask**: End LED alert flashing (osPriorityNormal, 128 words stack)

**IMPORTANT**: BlinkTask **Exclusively** controls the application logic for the RGB LEDs, the end LEDs are mainly controlled by AlertTask but the menu subsystem may take control when the `menuActive()` function is true.

**FreeRTOS Hooks** (in `Core/Src/freertos.c`):
- `vApplicationIdleHook()`: Refreshes the IWDG watchdog
- `PreSleepProcessing()`: Enters SLEEP mode when idle, stops TIM7
- `PostSleepProcessing()`: Restarts TIM7 after waking
- Stack overflow and malloc failure hooks trigger fatal errors

**Memory Management**: Uses heap_5 (multiple non-contiguous heap regions)

### DiveCAN Protocol

DiveCAN is a CAN-bus protocol for diving equipment communication. Implementation is in `Core/Src/DiveCAN/`:

- **Transciever.h**: Defines all CAN message IDs (e.g., `PPO2_PPO2_ID`, `HUD_STAT_ID`, `BUS_INIT_ID`)
- **DiveCAN.c/h**: Protocol handling and device initialization
- The HUD receives PPO2 values from external sensors via CAN and displays them

**Key Message IDs**:
- `0xD040000`: PPO2 values (main data feed)
- `0xD070000`: HUD status
- `0xD370000`: Bus initialization
- Extensions use `0xF000000+` range for debugging/advanced features

### LED Control System

**RGB LEDs** (`Core/Src/Hardware/leds.c/h`):
- 3 RGB LED channels controlled via `setRGB(channel, r, g, b)`
- Used to display PPO2 values as blink codes
- Functions: `blinkCode()`, `blinkNoData()`, `blinkAlarm()`

**End LEDs** (LED_0 through LED_3):
- 4 discrete LEDs on GPIO pins
- Used for menu state indication and alert flashing
- Controlled directly via HAL GPIO calls

### HUD Control Logic

**PPO2 Display** (`Core/Src/HUDControl.c`):
- RGB LEDs show PPO2 deviations from setpoint (100 = 1.0 bar)
- Values scaled by dividing by 10 and rounding
- Alert state triggers when any cell is <40 or >165
- Cell failure indicated when value = 0xFF

**Menu State Machine** (`Core/Src/menu_state_machine.c`):
- Button press sequence navigation:
  - **4 presses + hold on 4th**: Shutdown
  - **8 presses + hold on 8th**: Calibration mode
  - Other sequences: Return to normal operation
- LED feedback shows current menu state (counts up first 4 LEDs, then down)
- 10-second timeout returns to idle if no input
- Critical: `menuActive()` prevents alerts from interfering with menu

### Error Handling

**Two-tier error system** defined in `Core/Src/errors.h`:

**Fatal Errors** (trigger system reset):
- Stack overflow, malloc failures, hard faults, bus faults, memory faults
- Assert failures, buffer overruns, undefined states
- Use macro: `FATAL_ERROR(error_code)`

**Non-Fatal Errors** (logged and counted):
- Flash/EEPROM errors, I2C/UART failures, timeouts, queueing errors
- CAN errors, calibration errors, ADC errors, etc.
- Use macros: `NON_FATAL_ERROR(code)`, `NON_FATAL_ERROR_DETAIL(code, info)`

Errors stored in **EEPROM emulation** via `Core/Src/Hardware/flash.c`

### Power Management

**Power Modes** (`Core/Src/Hardware/pwr_management.c/h`):
- FreeRTOS idle hook puts CPU into SLEEP mode automatically
- IWDG (Independent Watchdog) refreshed in idle hook
- Low-power design targets ~1.5mA in "low" power mode (per commit message)

### Peripheral Configuration

**Hardware Abstractions** in `Core/Src/Hardware/`:
- `flash.c/h`: EEPROM emulation, option bytes, error persistence
- `leds.c/h`: RGB and discrete LED control
- `pwr_management.c/h`: Power state management
- `printer.c/h`: Debug output (likely UART-based)

**Key Peripherals**:
- CAN1: DiveCAN communication
- TSC: Touch sensing for button input
- I2C: Potential sensor communication
- TIM7: Runtime statistics counter for FreeRTOS
- IWDG: Independent watchdog
- CRC: Hardware CRC calculation

### STM32CubeMX Integration

This project uses **STM32CubeMX** for peripheral initialization (`STM32.ioc` config file):
- Auto-generated code in sections marked `/* USER CODE BEGIN */` and `/* USER CODE END */`
- **IMPORTANT**: Only edit code within USER CODE sections to preserve changes when regenerating
- Files that do not contain `/* USER CODE BEGIN */` and `/* USER CODE END */` and are within the Core/Src tree are not auto generated at all, and can be freely edited.

- Re-generation updates `main.c`, `stm32l4xx_hal_msp.c`, `stm32l4xx_it.c`, etc.

## Code Organization

```
Core/
├── Inc/              # HAL headers, FreeRTOS config
└── Src/
    ├── main.c        # Main entry, task definitions, peripheral init
    ├── freertos.c    # FreeRTOS hooks and configuration
    ├── common.h      # Type definitions, timeouts, task priorities
    ├── errors.h/c    # Error handling framework
    ├── HUDControl.c/h       # PPO2 display logic
    ├── menu_state_machine.c/h  # Button menu system
    ├── DiveCAN/
    │   ├── DiveCAN.c/h      # Protocol implementation
    │   └── Transciever.c/h  # CAN message definitions
    └── Hardware/
        ├── flash.c/h           # EEPROM emulation
        ├── leds.c/h            # LED control
        ├── pwr_management.c/h  # Power modes
        └── printer.c/h         # Debug output

Middlewares/
├── ST/EEPROM_Emul/          # ST EEPROM emulation library
├── ST/STM32_TouchSensing_Library/  # Touch sensing
└── Third_Party/FreeRTOS/    # FreeRTOS kernel

Drivers/
├── CMSIS/                   # ARM CMSIS headers
└── STM32L4xx_HAL_Driver/    # ST HAL library

TOUCHSENSING/App/            # Touch sensing configuration
```

## Common Development Patterns

### Adding a New Task

1. Define task handle, buffer, control block, and attributes in `main.c`
2. Set stack size constant in `common.h`
3. Set priority level in `common.h` following priority philosophy
4. Create task function with signature: `void TaskFunction(void *argument)`
5. Call `osThreadNew()` in `MX_FREERTOS_Init()`

### Using Queues for Inter-Task Communication

```c
// Producer (from HUDControl.c example)
osStatus_t status = osMessageQueuePut(PPO2QueueHandle, &data, 0, timeout);

// Consumer
CellValues_t cellValues;
osStatus_t status = osMessageQueueGet(PPO2QueueHandle, &cellValues, NULL, timeout);
```

### Error Handling Pattern

```c
// Non-fatal error without detail
if (operation_failed) {
    NON_FATAL_ERROR(TIMEOUT_ERR);
    return false;
}

// Non-fatal error with additional info
if (operation_failed) {
    NON_FATAL_ERROR_DETAIL(I2C_BUS_ERR, peripheral_status);
    return false;
}

// Fatal error (triggers reset)
if (critical_failure) {
    FATAL_ERROR(UNDEFINED_STATE_FERR);
}
```

### Timeout Constants

Use predefined timeout constants from `common.h`:
```c
osDelay(TIMEOUT_100MS_TICKS);  // 100ms delay
osDelay(TIMEOUT_2S_TICKS);     // 2 second delay
```

Available: 5ms, 10ms, 50ms, 100ms, 250ms, 500ms, 1s, 2s, 4s, 5s, 10s, 25s

### Code Style and Best Practices

**Avoid `goto` Statements**

Do not use `goto` statements in new code. Instead, use structured programming constructs:
- **Flags/state variables**: Use boolean flags to break out of nested loops
- **Helper functions**: Extract complex logic into separate functions
- **Early returns**: Return early from functions when conditions are met

Example of breaking out of nested loops without `goto`:
```c
// Instead of using goto
bool found = false;
for (int i = 0; i < rows && !found; i++) {
    for (int j = 0; j < cols && !found; j++) {
        if (condition) {
            found = true;
        }
    }
}
if (found) {
    // Handle result
}
```

This keeps the control flow clear and easier to maintain.

## Important Constraints

### Stack Usage

The build monitors stack usage with `-Wstack-usage=1305` and `-fstack-protector-strong`. Stay within allocated stack sizes defined in `common.h`. Stack analysis dumps are generated during build.

### Watchdog Timing

The IWDG watchdog is refreshed only in the FreeRTOS idle hook. Ensure tasks don't starve the idle task for extended periods.

### CAN Message Length

DiveCAN messages are limited to **8 bytes maximum** (`MAX_CAN_RX_LENGTH`). Longer messages trigger `CAN_OVERFLOW_ERR`.

### FreeRTOS Tickless Idle

The system uses tickless idle with custom pre/post sleep processing. Avoid blocking operations that prevent idle task execution.

## Debugging and Analysis

### Static Analysis Artifacts

Build generates analyzer output in `build/`:
- `*.callgraph`: Function call graphs from `-fdump-analyzer-callgraph`
- `*.supergraph`: Control flow graphs from `-fdump-analyzer-supergraph`
- `*.su`: Stack usage files from `-fstack-usage`
- `*.optimized`: Optimized tree dumps from `-fdump-tree-optimized`

### Memory Map

Check `build/STM32.map` for:
- Memory region usage
- Symbol addresses
- Link-time memory statistics

### Runtime Statistics

FreeRTOS runtime stats use TIM7 as high-frequency counter (configured via `configureTimerForRunTimeStats()` and `getRunTimeCounterValue()`).

## Testing

### Unit Test Infrastructure

Unit tests are located in `Tests/` directory and use CppUTest framework:
- **CppUTest**: Included as git submodule at `Tests/cpputest`
- **Mocks**: Hardware abstraction mocks in `Tests/Mocks/` (HAL functions, GPIO, timers)
- **Test Suites**: Each module has its own test directory (e.g., `Tests/menu_state_machine/`)

### Writing Tests

When adding new functionality:
1. Create mocks for any new HAL dependencies in `Tests/Mocks/`
2. Write tests in a new subdirectory under `Tests/`
3. Update `Tests/Makefile` to include new test targets
4. Ensure tests pass before committing

**Important Testing Notes**:
- Mock timer starts at tick=1 (not 0) to avoid timestamp issues
- Button/timeout thresholds use `>` not `>=` in code
- `displayLEDsForState()` is called before `incrementState()`, so call `menuStateMachineTick()` again after state changes to update LED display
- State timeout resets on each state transition (counted from `timeInState`, not menu entry)

## Git Workflow

Recent commits show focus on:
- LED control refactoring
- Menu state machine implementation
- EEPROM emulation and option byte writing
- Power optimization (targeting lower sleep current)

When making changes:
- **Write unit tests** for new state machine logic or hardware abstractions
- Run `make test` in Tests/ directory before committing
- Keep commits focused on single functional changes
- Test power consumption impacts for changes affecting sleep modes
- Verify IWDG refresh behavior if modifying task priorities or timing