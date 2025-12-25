# STM32 O2 HUD Unit Tests

This directory contains unit tests for the STM32 O2 HUD project using CppUTest framework.

## Prerequisites

### Installing CppUTest

On Ubuntu/Debian:
```bash
sudo apt-get install cpputest
```

On Arch Linux:
```bash
sudo pacman -S cpputest
```

From source:
```bash
git clone https://github.com/cpputest/cpputest.git
cd cpputest
mkdir build && cd build
cmake ..
make
sudo make install
```

## Building and Running Tests

### Build all tests
```bash
cd Tests
make
```

### Run all tests
```bash
make test
```

### Run tests with verbose output
```bash
make verbose_test
```

### List all available tests
```bash
make list_tests
```

### Clean build artifacts
```bash
make clean
```

## Test Structure

### Mocks
- `Mocks/MockHAL.h/cpp`: Mock implementations of STM32 HAL functions (GPIO, tick counter)
- `Mocks/MockCommon.h`: Mock timeout constants

### Test Suites
- `menu_state_machine/MenuStateMachineTest.cpp`: Tests for menu state machine transitions

## Writing New Tests

### Adding a new test suite

1. Create a new directory under `Tests/` for your module
2. Create a test file `<ModuleName>Test.cpp`
3. Include necessary mocks or create new ones in `Mocks/`
4. Add build rules to `Makefile`

### Test structure example

```cpp
#include "CppUTest/TestHarness.h"

extern "C" {
#include "module_under_test.h"
#include "../Mocks/MockHAL.h"
}

TEST_GROUP(ModuleName)
{
    void setup()
    {
        MockHAL_Reset();
        // Additional setup
    }

    void teardown()
    {
        // Cleanup
    }
};

TEST(ModuleName, TestCaseName)
{
    // Test implementation
    CHECK_EQUAL(expected, actual);
}
```

## Current Test Coverage

### menu_state_machine (35 tests)
- State transitions (idle → state1 → ... → state7 → idle)
- Shutdown trigger (4 presses + hold)
- Calibration trigger (8 presses + hold on 8th)
- Timeout behavior (10 second inactivity)
- Button press thresholds (100ms press, 2000ms hold)
- LED patterns for each state
- Persistent states (shutdown and calibration)
- Edge cases (exact threshold timing, rapid presses)

## CI Integration

To integrate with CI, add to your CI configuration:

```yaml
- name: Run Unit Tests
  run: |
    cd Tests
    make clean
    make test
```

## Troubleshooting

### CppUTest not found
If you get "cpputest/TestHarness.h: No such file or directory", set CPPUTEST_HOME:
```bash
export CPPUTEST_HOME=/usr/local  # or wherever CppUTest is installed
make
```

### Linker errors
Ensure CppUTest libraries are in your library path:
```bash
export LD_LIBRARY_PATH=$CPPUTEST_HOME/lib:$LD_LIBRARY_PATH
```
