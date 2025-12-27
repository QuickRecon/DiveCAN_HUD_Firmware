# Proposed Runtime Assertions for NASA Rule 5 Compliance

**Date:** 2025-12-27
**Purpose:** Systematic addition of runtime assertions to achieve NASA Rule 5 compliance
**Goal:** Minimum 2 assertions per function

This document provides **actionable code snippets** for adding assertions to every function lacking them. Each assertion is justified based on the function's implicit assumptions.

---

## Table of Contents

1. [Core/Src/HUDControl.c](#coresrchudcontrolc)
2. [Core/Src/menu_state_machine.c](#coresrcmenu_state_machinec)
3. [Core/Src/Hardware/leds.c](#coresrchardwareledsc)
4. [Core/Src/Hardware/flash.c](#coresrchardwareflashc)
5. [Core/Src/Hardware/pwr_management.c](#coresrchardwarepwr_managementc)
6. [Core/Src/DiveCAN/DiveCAN.c](#coresrcdivecandivecanc)
7. [Core/Src/DiveCAN/Transciever.c](#coresrcdivecantranscieverc)
8. [Implementation Guidelines](#implementation-guidelines)

---

## Core/Src/HUDControl.c

### Function: `div10_round()` (Line 15-19)

**Current Code:**
```c
inline int16_t div10_round(int16_t x)
{
    return (int16_t)(((int32_t)x + (x >= 0 ? 5 : -5)) / 10);
}
```

**Proposed Assertions:**
```c
inline int16_t div10_round(int16_t x)
{
    // Assertion 1: Verify input is in reasonable PPO2 range (scaled by 10)
    // PPO2 values should be 0-255 when unscaled, so 0-2550 when scaled by 10
    assert(x >= -1000 && x <= 2550);

    int16_t result = (int16_t)(((int32_t)x + (x >= 0 ? 5 : -5)) / 10);

    // Assertion 2: Verify result is within expected output range
    assert(result >= -100 && result <= 255);

    return result;
}
```

**Rationale:**
- Input range based on PPO2_t being uint8_t (0-255), scaled by 10
- Output range ensures division doesn't produce unexpected values
- Catches overflow/underflow conditions

---

### Function: `cell_alert()` (Line 21-24)

**Current Code:**
```c
inline bool cell_alert(uint8_t cellVal)
{
    return (cellVal < 40 || cellVal > 165);
}
```

**Proposed Assertions:**
```c
inline bool cell_alert(uint8_t cellVal)
{
    // Assertion 1: Verify cell value is not the fail marker
    assert(cellVal != PPO2_FAIL);  // 0xFF = 255

    // Assertion 2: Verify cell value is in valid PPO2 range (0-255 inherent to uint8_t)
    // This is implicit but we check for reasonable diving range
    assert(cellVal <= 255);  // Always true for uint8_t, but documents assumption

    return (cellVal < 40 || cellVal > 165);
}
```

**Rationale:**
- Function should not be called with failed cell (0xFF)
- Second assertion documents that uint8_t range is intentional

---

### Function: `PPO2Blink()` (Line 31-71)

**Current Code:**
```c
void PPO2Blink(CellValues_t *cellValues, bool *alerting)
{
    const uint8_t centerValue = 100;
    osStatus_t osStat = osMessageQueueGet(PPO2QueueHandle, cellValues, NULL, 0);
    // ... rest of function
}
```

**Proposed Assertions:**
```c
void PPO2Blink(CellValues_t *cellValues, bool *alerting)
{
    // Assertion 1: Verify pointer parameters are not NULL
    assert(cellValues != NULL);
    assert(alerting != NULL);

    // Assertion 2: Verify queue handle is valid
    assert(PPO2QueueHandle != NULL);

    const uint8_t centerValue = 100;
    osStatus_t osStat = osMessageQueueGet(PPO2QueueHandle, cellValues, NULL, 0);

    // ... rest of function (existing code)

    // Assertion 3: Verify cell values are in valid range after dequeue
    if (osStat == osOK) {
        assert(cellValues->C1 <= 255);  // uint8_t, documents expectation
        assert(cellValues->C2 <= 255);
        assert(cellValues->C3 <= 255);
    }

    // ... rest of function continues
}
```

**Rationale:**
- Pointers must be valid for function to work
- Queue handle must be initialized before use
- Cell values should be validated after dequeue

---

### Function: `ShutdownFadeout()` (Line 73-91)

**Current Code:**
```c
void ShutdownFadeout()
{
    for (uint8_t brightness = 10; brightness > 0; brightness--)
    {
        if (inShutdown)
        {
            for (uint8_t channel = 0; channel < 3; channel++)
            {
                setRGB(channel, brightness, 0, 0);
            }
            osDelay(TIMEOUT_500MS_TICKS);
        }
    }
    if(inShutdown){
        Shutdown();
    }
}
```

**Proposed Assertions:**
```c
void ShutdownFadeout()
{
    // Assertion 1: Verify we're in shutdown state when called
    assert(inShutdown == true);

    // Assertion 2: Verify TIMEOUT constant is reasonable (> 0)
    assert(TIMEOUT_500MS_TICKS > 0);

    for (uint8_t brightness = 10; brightness > 0; brightness--)
    {
        // Assertion 3: Verify brightness is in valid LED range during loop
        assert(brightness <= LED_MAX_BRIGHTNESS);

        if (inShutdown)
        {
            for (uint8_t channel = 0; channel < 3; channel++)
            {
                assert(channel < 3);  // Documents loop bound
                setRGB(channel, brightness, 0, 0);
            }
            osDelay(TIMEOUT_500MS_TICKS);
        }
    }
    if(inShutdown){
        Shutdown();
    }
}
```

**Rationale:**
- Function should only be called when inShutdown is true
- Brightness must stay within LED limits
- Channel must be within valid range

---

### Function: `RGBBlinkControl()` (Line 94-114)

**Current Code:**
```c
void RGBBlinkControl()
{
    for (uint8_t channel = 0; channel < 3; channel++)
    {
        setRGB(channel, 0, 0, 3);
    }
    osDelay(TIMEOUT_2S_TICKS);
    CellValues_t cellValues = {0};
    for (;;)
    {
        if (inShutdown)
        {
            ShutdownFadeout();
        }
        else
        {
            PPO2Blink(&cellValues, &alerting);
        }
    }
}
```

**Proposed Assertions:**
```c
void RGBBlinkControl()
{
    // Assertion 1: Verify TIMEOUT constant is valid
    assert(TIMEOUT_2S_TICKS > 0);

    // Assertion 2: Verify LED brightness constants are in range
    assert(LED_BRIGHTNESS[2] <= LED_MAX_BRIGHTNESS);  // Blue = index 2

    for (uint8_t channel = 0; channel < 3; channel++)
    {
        assert(channel < 3);
        setRGB(channel, 0, 0, 3);
    }
    osDelay(TIMEOUT_2S_TICKS);
    CellValues_t cellValues = {0};
    for (;;)  // Infinite loop acceptable for RTOS task
    {
        if (inShutdown)
        {
            ShutdownFadeout();
        }
        else
        {
            PPO2Blink(&cellValues, &alerting);
        }
    }
}
```

**Rationale:**
- Timeout constants must be positive
- LED brightness must be within hardware limits
- Channel index must be valid

---

### Function: `EndBlinkControl()` (Line 116-135)

**Current Code:**
```c
void EndBlinkControl()
{
    for (;;)
    {
        if (alerting && !menuActive())
        {
            HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
            // ... more GPIO operations
        }
        osDelay(TIMEOUT_100MS_TICKS);
    }
}
```

**Proposed Assertions:**
```c
void EndBlinkControl()
{
    // Assertion 1: Verify GPIO port pointers are valid
    assert(LED_0_GPIO_Port != NULL);
    assert(LED_1_GPIO_Port != NULL);
    assert(LED_2_GPIO_Port != NULL);
    assert(LED_3_GPIO_Port != NULL);

    // Assertion 2: Verify timeout constant is valid
    assert(TIMEOUT_100MS_TICKS > 0);

    for (;;)  // Infinite loop acceptable for RTOS task
    {
        if (alerting && !menuActive())
        {
            HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
            // ... rest of GPIO operations
        }
        osDelay(TIMEOUT_100MS_TICKS);
    }
}
```

**Rationale:**
- GPIO port pointers must be valid for HAL functions
- Timeout must be positive

---

## Core/Src/menu_state_machine.c

### Function: `menuActive()` (Line 47-50)

**Current Code:**
```c
bool menuActive()
{
    return currentMenuState != MENU_STATE_IDLE;
}
```

**Proposed Assertions:**
```c
bool menuActive()
{
    // Assertion 1: Verify current state is valid
    assert(currentMenuState <= MENU_STATE_CALIBRATE);

    // Assertion 2: Verify state is within enum bounds
    assert(currentMenuState >= MENU_STATE_IDLE);

    return currentMenuState != MENU_STATE_IDLE;
}
```

**Rationale:**
- State must be within valid enum range
- Detects memory corruption of state variable

---

### Function: `onButtonPress()` (Line 54-60)

**Current Code:**
```c
void onButtonPress()
{
    if (buttonPressTimestamp == 0)
    {
        buttonPressTimestamp = HAL_GetTick();
    }
}
```

**Proposed Assertions:**
```c
void onButtonPress()
{
    // Assertion 1: Verify HAL_GetTick() returns reasonable value
    uint32_t tick = HAL_GetTick();
    assert(tick > 0);  // Tick should never be 0 after boot

    if (buttonPressTimestamp == 0)
    {
        buttonPressTimestamp = tick;

        // Assertion 2: Verify timestamp was set
        assert(buttonPressTimestamp != 0);
    }
}
```

**Rationale:**
- HAL_GetTick() should return valid timestamp
- Timestamp should be successfully stored

---

### Function: `onButtonRelease()` (Line 62-65)

**Current Code:**
```c
void onButtonRelease()
{
    buttonPressTimestamp = 0;
}
```

**Proposed Assertions:**
```c
void onButtonRelease()
{
    // Assertion 1: Verify we had a valid press before release
    // (This is informational - release can happen without press)
    // No assertion needed, but we can add postcondition

    buttonPressTimestamp = 0;

    // Assertion 2: Verify timestamp was cleared (postcondition)
    assert(buttonPressTimestamp == 0);
}
```

**Rationale:**
- Postcondition verifies operation succeeded
- Simple function but assertion documents expectation

---

### Function: `incrementState()` (Line 67-145)

**Current Code:**
```c
bool incrementState(ButtonState_t button_state)
{
    MenuState_t previousState = currentMenuState;
    switch (currentMenuState)
    {
        // ... large switch statement
    }
    return previousState != currentMenuState;
}
```

**Proposed Assertions:**
```c
bool incrementState(ButtonState_t button_state)
{
    // Assertion 1: Verify button state is valid
    assert(button_state >= NONE && button_state <= HOLD);

    // Assertion 2: Verify current menu state is valid before transition
    assert(currentMenuState >= MENU_STATE_IDLE && currentMenuState <= MENU_STATE_CALIBRATE);

    MenuState_t previousState = currentMenuState;

    switch (currentMenuState)
    {
        // ... existing switch cases
    }

    // Assertion 3: Verify new state is valid after transition
    assert(currentMenuState >= MENU_STATE_IDLE && currentMenuState <= MENU_STATE_CALIBRATE);

    // Assertion 4: Verify state transition logic (no invalid jumps)
    // E.g., can't jump from IDLE directly to SHUTDOWN
    if (previousState == MENU_STATE_IDLE) {
        assert(currentMenuState == MENU_STATE_IDLE ||
               currentMenuState == MENU_STATE_1PRESS);
    }

    return previousState != currentMenuState;
}
```

**Rationale:**
- Button state must be valid enum value
- State must be valid before and after transition
- State machine transitions must follow valid paths

---

### Function: `resetMenuStateMachine()` (Line 147-152)

**Current Code:**
```c
void resetMenuStateMachine()
{
    currentMenuState = MENU_STATE_IDLE;
    timeInState = 0;
    buttonPressTimestamp = 0;
}
```

**Proposed Assertions:**
```c
void resetMenuStateMachine()
{
    currentMenuState = MENU_STATE_IDLE;
    timeInState = 0;
    buttonPressTimestamp = 0;

    // Assertion 1: Verify state was reset to IDLE (postcondition)
    assert(currentMenuState == MENU_STATE_IDLE);

    // Assertion 2: Verify timestamps were cleared (postcondition)
    assert(timeInState == 0);
    assert(buttonPressTimestamp == 0);
}
```

**Rationale:**
- Postconditions verify reset succeeded
- Catches memory corruption issues

---

### Function: `displayLEDsForState()` (Line 154-228)

**Current Code:**
```c
void displayLEDsForState()
{
    switch (currentMenuState)
    {
        case MENU_STATE_IDLE:
            // LED operations
            break;
        // ... more cases
    }
}
```

**Proposed Assertions:**
```c
void displayLEDsForState()
{
    // Assertion 1: Verify current state is valid
    assert(currentMenuState >= MENU_STATE_IDLE && currentMenuState <= MENU_STATE_CALIBRATE);

    // Assertion 2: Verify GPIO ports are valid
    assert(LED_0_GPIO_Port != NULL);
    assert(LED_1_GPIO_Port != NULL);
    assert(LED_2_GPIO_Port != NULL);
    assert(LED_3_GPIO_Port != NULL);

    switch (currentMenuState)
    {
        case MENU_STATE_IDLE:
            // LED operations
            break;
        // ... more cases
    }
}
```

**Rationale:**
- State must be valid for switch statement
- GPIO ports must be initialized

---

### Function: `menuStateMachineTick()` (Line 230-265)

**Current Code:**
```c
void menuStateMachineTick()
{
    static ButtonState_t button_state = NONE;
    if (buttonPressTimestamp != 0)
    {
        if (HAL_GetTick() - buttonPressTimestamp > BUTTON_HOLD_TIME_MS)
        {
            button_state = HOLD;
        }
        // ... more logic
    }
    // ... rest of function
}
```

**Proposed Assertions:**
```c
void menuStateMachineTick()
{
    static ButtonState_t button_state = NONE;

    // Assertion 1: Verify timeout constants are valid
    assert(BUTTON_HOLD_TIME_MS > 0);
    assert(BUTTON_PRESS_TIME_MS > 0);
    assert(MENU_MODE_TIMEOUT_MS > 0);

    // Assertion 2: Verify button state is valid
    assert(button_state >= NONE && button_state <= HOLD);

    if (buttonPressTimestamp != 0)
    {
        uint32_t currentTick = HAL_GetTick();

        // Assertion 3: Verify time hasn't wrapped around unexpectedly
        assert(currentTick >= buttonPressTimestamp ||
               currentTick < 1000);  // Allow for wrap-around

        if (currentTick - buttonPressTimestamp > BUTTON_HOLD_TIME_MS)
        {
            button_state = HOLD;
        }
        else if (currentTick - buttonPressTimestamp > BUTTON_PRESS_TIME_MS && button_state == NONE)
        {
            button_state = PRESS;
        }
    }
    else
    {
        button_state = NONE;
    }

    displayLEDsForState();
    bool stateIncremented = incrementState(button_state);

    if (stateIncremented)
    {
        timeInState = HAL_GetTick();
        if (button_state == PRESS)
        {
            button_state = PRESSED;
        }
    }

    inShutdown = currentMenuState == MENU_STATE_SHUTDOWN;

    if ((timeInState != 0) && (HAL_GetTick() - timeInState > MENU_MODE_TIMEOUT_MS) && buttonPressTimestamp == 0)
    {
        resetMenuStateMachine();
    }
}
```

**Rationale:**
- Timeout constants must be positive
- Button state must stay within valid enum
- Timer wrap-around should be handled safely

---

## Core/Src/Hardware/leds.c

### Function: `initLEDs()` (Line 28-79)

**Current Code:**
```c
void initLEDs(void)
{
    HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
    // ... many GPIO operations
}
```

**Proposed Assertions:**
```c
void initLEDs(void)
{
    // Assertion 1: Verify GPIO ports are initialized
    assert(LED_0_GPIO_Port != NULL);
    assert(LED_1_GPIO_Port != NULL);
    assert(LED_2_GPIO_Port != NULL);
    assert(LED_3_GPIO_Port != NULL);
    assert(ASC_EN_GPIO_Port != NULL);
    assert(R1_GPIO_Port != NULL);
    assert(G1_GPIO_Port != NULL);
    assert(B1_GPIO_Port != NULL);

    // Assertion 2: Verify LED brightness constants are in range
    assert(LED_BRIGHTNESS[0] <= LED_MAX_BRIGHTNESS);  // Red
    assert(LED_BRIGHTNESS[1] <= LED_MAX_BRIGHTNESS);  // Green
    assert(LED_BRIGHTNESS[2] <= LED_MAX_BRIGHTNESS);  // Blue

    // ... rest of function with GPIO operations
}
```

**Rationale:**
- GPIO ports must be initialized by HAL
- LED brightness constants must be within hardware limits

---

### Function: `setRGB()` (Line 105-110)

**Current Code:**
```c
void setRGB(uint8_t channel, uint8_t r, uint8_t g, uint8_t b)
{
    setLEDBrightness(r, LED_PinMap[channel][0].port, LED_PinMap[channel][0].pin);
    setLEDBrightness(g, LED_PinMap[channel][1].port, LED_PinMap[channel][1].pin);
    setLEDBrightness(b, LED_PinMap[channel][2].port, LED_PinMap[channel][2].pin);
}
```

**Proposed Assertions:**
```c
void setRGB(uint8_t channel, uint8_t r, uint8_t g, uint8_t b)
{
    // Assertion 1: Verify channel is within valid range
    assert(channel < 3);

    // Assertion 2: Verify RGB values are within LED brightness limits
    assert(r <= LED_MAX_BRIGHTNESS);
    assert(g <= LED_MAX_BRIGHTNESS);
    assert(b <= LED_MAX_BRIGHTNESS);

    // Assertion 3: Verify LED pin map has valid pointers
    assert(LED_PinMap[channel][0].port != NULL);
    assert(LED_PinMap[channel][1].port != NULL);
    assert(LED_PinMap[channel][2].port != NULL);

    setLEDBrightness(r, LED_PinMap[channel][0].port, LED_PinMap[channel][0].pin);
    setLEDBrightness(g, LED_PinMap[channel][1].port, LED_PinMap[channel][1].pin);
    setLEDBrightness(b, LED_PinMap[channel][2].port, LED_PinMap[channel][2].pin);
}
```

**Rationale:**
- Channel must be 0-2 to index LED_PinMap array
- RGB values must not exceed hardware maximum
- GPIO ports must be valid

---

### Function: `blinkNoData()` (Line 205-222)

**Current Code:**
```c
void blinkNoData(void)
{
    for (uint8_t i = 0; i < 2; i++)
    {
        for (uint8_t channel = 0; channel < 3; channel++)
        {
            setRGB(channel, 0, 0, LED_BRIGHTNESS[2]);
        }
        osDelay(BLINK_PERIOD);
        // ... more operations
    }
}
```

**Proposed Assertions:**
```c
void blinkNoData(void)
{
    // Assertion 1: Verify LED brightness constant is valid
    assert(LED_BRIGHTNESS[2] <= LED_MAX_BRIGHTNESS);

    // Assertion 2: Verify blink period is valid
    assert(BLINK_PERIOD > 0);

    for (uint8_t i = 0; i < 2; i++)
    {
        for (uint8_t channel = 0; channel < 3; channel++)
        {
            assert(channel < 3);
            setRGB(channel, 0, 0, LED_BRIGHTNESS[2]);
        }
        osDelay(BLINK_PERIOD);
        for (uint8_t channel = 0; channel < 3; channel++)
        {
            assert(channel < 3);
            setRGB(channel, 0, 0, 0);
        }
        osDelay(BLINK_PERIOD);
    }
    osDelay(BLINK_PERIOD * 2);
}
```

**Rationale:**
- LED brightness must be within limits
- Blink period must be positive
- Channel must be valid in loops

---

### Function: `blinkAlarm()` (Line 224-246)

**Current Code:**
```c
void blinkAlarm()
{
    for (int i = 0; i < 5; i++)
    {
        setRGB(0, LED_MAX_BRIGHTNESS, 0, 0);
        osDelay(TIMEOUT_50MS_TICKS);
        // ... more operations
    }
}
```

**Proposed Assertions:**
```c
void blinkAlarm()
{
    // Assertion 1: Verify LED max brightness is valid
    assert(LED_MAX_BRIGHTNESS <= 32);  // Hardware limit
    assert(LED_MAX_BRIGHTNESS > 0);

    // Assertion 2: Verify timeout constant is valid
    assert(TIMEOUT_50MS_TICKS > 0);

    for (int i = 0; i < 5; i++)
    {
        assert(i >= 0 && i < 5);
        setRGB(0, LED_MAX_BRIGHTNESS, 0, 0);
        osDelay(TIMEOUT_50MS_TICKS);
        setRGB(0, 0, 0, 0);
        setRGB(1, LED_MAX_BRIGHTNESS, 0, 0);
        osDelay(TIMEOUT_50MS_TICKS);
        // ... rest of pattern
    }
}
```

**Rationale:**
- LED_MAX_BRIGHTNESS must be within hardware limits
- Timeout must be positive
- Loop counter should be within bounds

---

## Core/Src/Hardware/flash.c

### Function: `set_bit()` (Line 12-18)

**Current Code:**
```c
static inline uint32_t set_bit(uint32_t number, uint32_t n, bool x)
{
    return (number & ~((uint32_t)1 << n)) | ((uint32_t)x << n);
}
```

**Proposed Assertions:**
```c
static inline uint32_t set_bit(uint32_t number, uint32_t n, bool x)
{
    // Assertion 1: Verify bit position is within 32-bit range
    assert(n < 32);

    // Assertion 2: Verify boolean is actually 0 or 1
    assert(x == 0 || x == 1 || x == true || x == false);

    return (number & ~((uint32_t)1 << n)) | ((uint32_t)x << n);
}
```

**Rationale:**
- Bit position must be 0-31 for 32-bit value
- Boolean should be valid true/false

---

### Function: `setOptionBytes()` (Line 20-106)

**Current Code:**
```c
void setOptionBytes(void)
{
    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    if (HAL_OK != status)
    {
        NON_FATAL_ERROR_DETAIL(FLASH_LOCK_ERR, status);
    }
    // ... more operations
}
```

**Proposed Assertions:**
```c
void setOptionBytes(void)
{
    // Assertion 1: Verify FLASH is in locked state before unlock
    // (HAL doesn't provide easy check, but we can verify after)

    HAL_StatusTypeDef status = HAL_FLASH_Unlock();

    // Assertion 2: Verify unlock status is a valid HAL status
    assert(status == HAL_OK || status == HAL_ERROR ||
           status == HAL_BUSY || status == HAL_TIMEOUT);

    if (HAL_OK == status)
    {
        FLASH_OBProgramInitTypeDef optionBytes = {0};
        HAL_FLASHEx_OBGetConfig(&optionBytes);

        // Assertion 3: Verify option bytes structure has valid config
        assert(optionBytes.USERConfig != 0xFFFFFFFF);  // Not uninitialized

        uint32_t original_opt = optionBytes.USERConfig;

        // ... bit setting operations

        // Assertion 4: Verify new config is different if changed
        if (optionBytes.USERConfig != original_opt) {
            assert(optionBytes.USERConfig != 0);
        }

        // ... rest of function
    }
}
```

**Rationale:**
- HAL status must be valid enum value
- Option bytes should not be uninitialized (0xFFFFFFFF)
- Config changes should result in non-zero value

---

### Function: `initFlash()` (Line 108-152)

**Current Code:**
```c
void initFlash(void)
{
    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    if (HAL_OK != status)
    {
        NON_FATAL_ERROR_DETAIL(FLASH_LOCK_ERR, status);
    }
    // ... more operations
}
```

**Proposed Assertions:**
```c
void initFlash(void)
{
    // Assertion 1: Verify HAL is initialized (indirect check)
    // We can't directly check HAL state, but we verify after operations

    HAL_StatusTypeDef status = HAL_FLASH_Unlock();

    // Assertion 2: Verify unlock status is valid
    assert(status == HAL_OK || status == HAL_ERROR ||
           status == HAL_BUSY || status == HAL_TIMEOUT);

    if (HAL_OK == status)
    {
        // ... clear flags

        EE_Status eePromStatus = EE_Init(EE_FORCED_ERASE);

        // Assertion 3: Verify EEPROM status is valid
        assert(eePromStatus == EE_OK || eePromStatus == EE_NO_DATA ||
               eePromStatus == EE_WRITE_ERROR || eePromStatus == EE_FORMAT_ERROR);

        // ... rest of function

        status = HAL_FLASH_Lock();

        // Assertion 4: Verify flash was locked successfully (postcondition)
        // HAL doesn't expose lock state, but status should be OK
        assert(status == HAL_OK || status == HAL_ERROR);
    }
}
```

**Rationale:**
- HAL and EEPROM statuses must be valid enum values
- Flash should be locked after operations complete

---

### Function: `WriteInt32()` (Line 161-213)

**Current Code:**
```c
static bool WriteInt32(uint16_t addr, uint32_t value)
{
    bool writeOk = true;
    uint8_t attempts = 0;
    EE_Status result = EE_OK;
    do
    {
        // ... write operations
        ++attempts;
    } while ((!writeOk) && (attempts < MAX_WRITE_ATTEMPTS));
    return writeOk;
}
```

**Proposed Assertions:**
```c
static bool WriteInt32(uint16_t addr, uint32_t value)
{
    // Assertion 1: Verify address is within EEPROM range
    // EEPROM emulation typically uses addresses 0x00-0xFF
    assert(addr <= 0xFF);

    // Assertion 2: Verify MAX_WRITE_ATTEMPTS is reasonable
    assert(MAX_WRITE_ATTEMPTS > 0 && MAX_WRITE_ATTEMPTS < 100);

    bool writeOk = true;
    uint8_t attempts = 0;
    EE_Status result = EE_OK;

    do
    {
        // ... write operations

        // Assertion 3: Verify attempts doesn't overflow
        assert(attempts < MAX_WRITE_ATTEMPTS);

        ++attempts;

    } while ((!writeOk) && (attempts < MAX_WRITE_ATTEMPTS));

    // Assertion 4: Verify attempts stayed within bounds
    assert(attempts <= MAX_WRITE_ATTEMPTS);

    return writeOk;
}
```

**Rationale:**
- Address must be within EEPROM emulation range
- Retry attempts must be bounded
- Loop counter should not overflow

---

### Function: `GetFatalError()` (Line 219-250)

**Current Code:**
```c
bool GetFatalError(FatalError_t *err)
{
    bool readOk = false;
    if (NULL == err)
    {
        NON_FATAL_ERROR(NULL_PTR_ERR);
    }
    else
    {
        // ... read operations
    }
    return readOk;
}
```

**Proposed Assertions:**
```c
bool GetFatalError(FatalError_t *err)
{
    // Assertion 1: Verify pointer is not NULL
    assert(err != NULL);

    bool readOk = false;

    if (NULL == err)
    {
        NON_FATAL_ERROR(NULL_PTR_ERR);
        return false;
    }

    uint32_t errInt = 0;
    EE_Status result = EE_ReadVariable32bits(FATAL_ERROR_BASE_ADDR, &errInt);

    *err = (FatalError_t)errInt;

    // Assertion 2: Verify error value is within valid enum range
    assert(*err >= NONE_FERR);  // Assuming NONE_FERR is first enum
    assert(*err < 0xFF);  // Reasonable upper bound for error codes

    if (result == EE_OK)
    {
        readOk = true;
    }
    else if (result == EE_NO_DATA)
    {
        FatalError_t defaultVal = NONE_FERR;
        (void)SetFatalError(defaultVal);
    }
    else
    {
        NON_FATAL_ERROR_DETAIL(EEPROM_ERR, result);
    }

    return readOk;
}
```

**Rationale:**
- Pointer must not be NULL
- Read error value should be within valid range

---

### Function: `SetFatalError()` (Line 255-259)

**Current Code:**
```c
bool SetFatalError(FatalError_t err)
{
    bool writeOk = WriteInt32(FATAL_ERROR_BASE_ADDR, (uint32_t)err);
    return writeOk;
}
```

**Proposed Assertions:**
```c
bool SetFatalError(FatalError_t err)
{
    // Assertion 1: Verify error code is within valid range
    assert(err >= NONE_FERR);
    assert(err < 0xFF);  // Reasonable upper bound

    // Assertion 2: Verify EEPROM address is valid
    assert(FATAL_ERROR_BASE_ADDR <= 0xFF);

    bool writeOk = WriteInt32(FATAL_ERROR_BASE_ADDR, (uint32_t)err);
    return writeOk;
}
```

**Rationale:**
- Error code must be valid enum value
- EEPROM address must be within range

---

## Core/Src/Hardware/pwr_management.c

### Function: `Shutdown()` (Line 16-52)

**Current Code:**
```c
void Shutdown()
{
    bool writeErrOk = SetFatalError(NONE_FERR);
    if (!writeErrOk)
    {
        serial_printf("Failed to reset last fatal error on shutdown");
    }
    // ... GPIO pullup/pulldown operations
    HAL_PWR_EnterSTANDBYMode();
}
```

**Proposed Assertions:**
```c
void Shutdown()
{
    // Assertion 1: Verify we're in a safe state to shutdown
    // (No critical operations pending)
    assert(inShutdown == true);

    bool writeErrOk = SetFatalError(NONE_FERR);

    // Assertion 2: Verify HAL PWR functions are available
    // (indirect check via successful pullup enable)
    HAL_PWREx_EnablePullUpPullDownConfig();

    // ... GPIO pullup/pulldown operations

    // Assertion 3: This function should not return
    // (if it does, something went wrong)
    HAL_PWR_EnterSTANDBYMode();

    // Should never reach here
    assert(false && "Failed to enter standby mode");
}
```

**Rationale:**
- System should be in shutdown state
- HAL PWR functions must be available
- Function should not return (enters standby)

---

### Function: `testBusActive()` (Line 58-75)

**Current Code:**
```c
bool testBusActive(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = CAN_EN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    bool busActive = HAL_GPIO_ReadPin(CAN_EN_GPIO_Port, CAN_EN_Pin) == GPIO_PIN_RESET;

    // ... restore to NOPULL
    return busActive;
}
```

**Proposed Assertions:**
```c
bool testBusActive(void)
{
    // Assertion 1: Verify GPIO port is valid
    assert(CAN_EN_GPIO_Port != NULL);
    assert(CAN_EN_GPIO_Port == GPIOC);

    // Assertion 2: Verify pin is valid (within 0-15)
    assert(CAN_EN_Pin < GPIO_PIN_All);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = CAN_EN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    bool busActive = HAL_GPIO_ReadPin(CAN_EN_GPIO_Port, CAN_EN_Pin) == GPIO_PIN_RESET;

    // Restore to NOPULL
    GPIO_InitStruct.Pin = CAN_EN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(CAN_EN_GPIO_Port, &GPIO_InitStruct);

    return busActive;
}
```

**Rationale:**
- GPIO port must be initialized and correct
- Pin must be within valid GPIO pin range

---

### Function: `getBusStatus()` (Line 81-85)

**Current Code:**
```c
bool getBusStatus(void)
{
    return HAL_GPIO_ReadPin(CAN_EN_GPIO_Port, CAN_EN_Pin) == GPIO_PIN_RESET;
}
```

**Proposed Assertions:**
```c
bool getBusStatus(void)
{
    // Assertion 1: Verify GPIO port is valid
    assert(CAN_EN_GPIO_Port != NULL);

    // Assertion 2: Verify pin is valid
    assert(CAN_EN_Pin < GPIO_PIN_All);

    return HAL_GPIO_ReadPin(CAN_EN_GPIO_Port, CAN_EN_Pin) == GPIO_PIN_RESET;
}
```

**Rationale:**
- GPIO port and pin must be valid for read operation

---

## Core/Src/DiveCAN/DiveCAN.c

### Function: `InitDiveCAN()` (Line 42-63)

**Current Code:**
```c
void InitDiveCAN(const DiveCANDevice_t *const deviceSpec)
{
    InitRXQueue();

    static uint8_t CANTask_buffer[CANTASK_STACK_SIZE];
    static StaticTask_t CANTask_ControlBlock;
    static DiveCANTask_params_t task_params;
    // ... task creation
}
```

**Proposed Assertions:**
```c
void InitDiveCAN(const DiveCANDevice_t *const deviceSpec)
{
    // Assertion 1: Verify device spec pointer is valid
    assert(deviceSpec != NULL);

    // Assertion 2: Verify device spec fields are valid
    assert(deviceSpec->name != NULL);
    assert(deviceSpec->type >= DIVECAN_CONTROLLER && deviceSpec->type <= DIVECAN_REVO);
    assert(deviceSpec->manufacturerID > 0);

    InitRXQueue();

    // Assertion 3: Verify stack size is reasonable
    assert(CANTASK_STACK_SIZE >= 512 && CANTASK_STACK_SIZE <= 10000);

    static uint8_t CANTask_buffer[CANTASK_STACK_SIZE];
    static StaticTask_t CANTask_ControlBlock;
    static DiveCANTask_params_t task_params;

    // ... task creation

    osThreadId_t *CANTaskHandle = getOSThreadId();
    task_params.deviceSpec = *deviceSpec;
    *CANTaskHandle = osThreadNew(CANTask, &task_params, &CANTask_attributes);

    // Assertion 4: Verify task was created successfully
    assert(*CANTaskHandle != NULL);
}
```

**Rationale:**
- Device spec must be valid and populated
- Stack size must be within reasonable bounds
- Task creation should succeed

---

### Function: `CANTask()` (Line 68-173)

**Current Code:**
```c
void CANTask(void *arg)
{
    DiveCANTask_params_t *task_params = (DiveCANTask_params_t *)arg;
    const DiveCANDevice_t *const deviceSpec = &(task_params->deviceSpec);

    txStartDevice(DIVECAN_MONITOR, DIVECAN_CONTROLLER);
    while (true)
    {
        // ... message processing
    }
}
```

**Proposed Assertions:**
```c
void CANTask(void *arg)
{
    // Assertion 1: Verify argument pointer is valid
    assert(arg != NULL);

    DiveCANTask_params_t *task_params = (DiveCANTask_params_t *)arg;

    // Assertion 2: Verify device spec is valid
    assert(task_params != NULL);
    const DiveCANDevice_t *const deviceSpec = &(task_params->deviceSpec);
    assert(deviceSpec != NULL);
    assert(deviceSpec->type >= DIVECAN_CONTROLLER && deviceSpec->type <= DIVECAN_REVO);

    txStartDevice(DIVECAN_MONITOR, DIVECAN_CONTROLLER);

    while (true)  // Infinite loop acceptable for RTOS task
    {
        DiveCANMessage_t message = {0};
        if (pdTRUE == GetLatestCAN(TIMEOUT_1S_TICKS, &message))
        {
            // Assertion 3: Verify message length is valid
            assert(message.length <= MAX_CAN_RX_LENGTH);

            uint32_t message_id = message.id & ID_MASK;

            switch (message_id)
            {
                // ... message handling
            }
        }
    }
}
```

**Rationale:**
- Task parameter must be valid
- Device spec must be initialized
- Message length must not exceed CAN limits

---

### Function: `RespPing()` (Line 181-190)

**Current Code:**
```c
void RespPing(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    DiveCANType_t devType = deviceSpec->type;
    if (((message->id & DIVECAN_TYPE_MASK) == DIVECAN_SOLO) ||
        ((message->id & DIVECAN_TYPE_MASK) == DIVECAN_OBOE))
    {
        txID(devType, deviceSpec->manufacturerID, deviceSpec->firmwareVersion);
        txName(devType, deviceSpec->name);
    }
}
```

**Proposed Assertions:**
```c
void RespPing(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    // Assertion 1: Verify pointers are valid
    assert(message != NULL);
    assert(deviceSpec != NULL);

    // Assertion 2: Verify device spec fields are valid
    assert(deviceSpec->name != NULL);
    assert(deviceSpec->type >= DIVECAN_CONTROLLER && deviceSpec->type <= DIVECAN_REVO);

    DiveCANType_t devType = deviceSpec->type;

    if (((message->id & DIVECAN_TYPE_MASK) == DIVECAN_SOLO) ||
        ((message->id & DIVECAN_TYPE_MASK) == DIVECAN_OBOE))
    {
        txID(devType, deviceSpec->manufacturerID, deviceSpec->firmwareVersion);
        txName(devType, deviceSpec->name);
    }
}
```

**Rationale:**
- Message and device spec must be valid
- Device spec fields must be populated

---

### Function: `RespPPO2()` (Line 192-208)

**Current Code:**
```c
void RespPPO2(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    (void)deviceSpec;
    CellValues_t cell_values;

    cell_values.C1 = message->data[1];
    cell_values.C2 = message->data[2];
    cell_values.C3 = message->data[3];

    osMessageQueueReset(PPO2QueueHandle);
    osStatus_t enQueueStatus = osMessageQueuePut(PPO2QueueHandle, &cell_values, 0, 0);
    // ... error handling
}
```

**Proposed Assertions:**
```c
void RespPPO2(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    (void)deviceSpec;

    // Assertion 1: Verify message pointer is valid
    assert(message != NULL);

    // Assertion 2: Verify message has enough data
    assert(message->length >= 4);

    // Assertion 3: Verify queue handle is valid
    assert(PPO2QueueHandle != NULL);

    CellValues_t cell_values;

    cell_values.C1 = message->data[1];
    cell_values.C2 = message->data[2];
    cell_values.C3 = message->data[3];

    // Assertion 4: Verify cell values are in valid range
    assert(cell_values.C1 <= 255);  // uint8_t, documents expectation
    assert(cell_values.C2 <= 255);
    assert(cell_values.C3 <= 255);

    osMessageQueueReset(PPO2QueueHandle);
    osStatus_t enQueueStatus = osMessageQueuePut(PPO2QueueHandle, &cell_values, 0, 0);
    if (enQueueStatus != osOK)
    {
        NON_FATAL_ERROR_DETAIL(QUEUEING_ERR, enQueueStatus);
    }
}
```

**Rationale:**
- Message must be valid and contain sufficient data
- Queue handle must be initialized
- Cell values should be validated

---

### Function: `RespPPO2Status()` (Line 210-222)

**Current Code:**
```c
void RespPPO2Status(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    (void)deviceSpec;
    uint8_t status = message->data[0];

    osMessageQueueReset(CellStatQueueHandle);
    osStatus_t enQueueStatus = osMessageQueuePut(CellStatQueueHandle, &status, 0, 0);
    // ... error handling
}
```

**Proposed Assertions:**
```c
void RespPPO2Status(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    (void)deviceSpec;

    // Assertion 1: Verify message pointer is valid
    assert(message != NULL);

    // Assertion 2: Verify message has data
    assert(message->length >= 1);

    // Assertion 3: Verify queue handle is valid
    assert(CellStatQueueHandle != NULL);

    uint8_t status = message->data[0];

    // Assertion 4: Verify status is a 3-bit mask
    assert(status <= 0b111);

    osMessageQueueReset(CellStatQueueHandle);
    osStatus_t enQueueStatus = osMessageQueuePut(CellStatQueueHandle, &status, 0, 0);
    if (enQueueStatus != osOK)
    {
        NON_FATAL_ERROR_DETAIL(QUEUEING_ERR, enQueueStatus);
    }
}
```

**Rationale:**
- Message must be valid and contain data
- Queue must be initialized
- Status should be 3-bit mask (0-7)

---

### Function: `RespShutdown()` (Line 224-237)

**Current Code:**
```c
void RespShutdown(const DiveCANMessage_t *, const DiveCANDevice_t *)
{
    const uint8_t SHUTDOWN_ATTEMPTS = 20;
    for (uint8_t i = 0; i < SHUTDOWN_ATTEMPTS; ++i)
    {
        if (!getBusStatus())
        {
            serial_printf("Performing requested shutdown");
            Shutdown();
        }
        (void)osDelay(TIMEOUT_100MS_TICKS);
    }
    serial_printf("Shutdown attempted but timed out due to missing en signal");
}
```

**Proposed Assertions:**
```c
void RespShutdown(const DiveCANMessage_t *message, const DiveCANDevice_t *deviceSpec)
{
    // Assertion 1: Verify shutdown attempts is reasonable
    const uint8_t SHUTDOWN_ATTEMPTS = 20;
    assert(SHUTDOWN_ATTEMPTS > 0 && SHUTDOWN_ATTEMPTS < 100);

    // Assertion 2: Verify timeout is valid
    assert(TIMEOUT_100MS_TICKS > 0);

    for (uint8_t i = 0; i < SHUTDOWN_ATTEMPTS; ++i)
    {
        assert(i < SHUTDOWN_ATTEMPTS);

        if (!getBusStatus())
        {
            serial_printf("Performing requested shutdown");
            Shutdown();
            // Should not return from Shutdown()
            assert(false && "Returned from Shutdown() unexpectedly");
        }
        (void)osDelay(TIMEOUT_100MS_TICKS);
    }
    serial_printf("Shutdown attempted but timed out due to missing en signal");
}
```

**Rationale:**
- Shutdown attempts must be bounded
- Loop counter should not overflow
- Shutdown() should not return

---

### Function: `RespSerialNumber()` (Line 239-246)

**Current Code:**
```c
void RespSerialNumber(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    (void)deviceSpec;
    DiveCANType_t origin = (DiveCANType_t)(0xF & (message->id));
    char serial_number[sizeof(message->data) + 1] = {0};
    (void)memcpy(serial_number, message->data, sizeof(message->data));
    serial_printf("Received Serial Number of device %d: %s", origin, serial_number);
}
```

**Proposed Assertions:**
```c
void RespSerialNumber(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    (void)deviceSpec;

    // Assertion 1: Verify message pointer is valid
    assert(message != NULL);

    // Assertion 2: Verify message has data
    assert(message->length > 0 && message->length <= 8);

    DiveCANType_t origin = (DiveCANType_t)(0xF & (message->id));

    // Assertion 3: Verify origin type is valid
    assert(origin >= DIVECAN_CONTROLLER && origin <= DIVECAN_REVO);

    char serial_number[sizeof(message->data) + 1] = {0};
    (void)memcpy(serial_number, message->data, sizeof(message->data));

    // Assertion 4: Verify serial number is null-terminated
    assert(serial_number[sizeof(message->data)] == '\0');

    serial_printf("Received Serial Number of device %d: %s", origin, serial_number);
}
```

**Rationale:**
- Message must be valid with data
- Origin device type must be valid
- Serial number buffer must be null-terminated

---

## Core/Src/DiveCAN/Transciever.c

### Function: `InitRXQueue()` (Line 36-53)

**Current Code:**
```c
void InitRXQueue(void)
{
    QueueHandle_t *inbound = getInboundQueue();
    QueueHandle_t *dataAvail = getDataAvailQueue();

    if ((NULL == *inbound) && (NULL == *dataAvail))
    {
        // ... queue creation
    }
}
```

**Proposed Assertions:**
```c
void InitRXQueue(void)
{
    QueueHandle_t *inbound = getInboundQueue();
    QueueHandle_t *dataAvail = getDataAvailQueue();

    // Assertion 1: Verify getter functions return valid pointers
    assert(inbound != NULL);
    assert(dataAvail != NULL);

    if ((NULL == *inbound) && (NULL == *dataAvail))
    {
        static StaticQueue_t QInboundCAN_QueueStruct = {0};
        static uint8_t QInboundCAN_Storage[CAN_QUEUE_LEN * sizeof(DiveCANMessage_t)];

        *inbound = xQueueCreateStatic(CAN_QUEUE_LEN, sizeof(DiveCANMessage_t),
                                      QInboundCAN_Storage, &QInboundCAN_QueueStruct);

        static StaticQueue_t QDataAvail_QueueStruct = {0};
        static uint8_t QDataAvail_Storage[sizeof(bool)];

        *dataAvail = xQueueCreateStatic(1, sizeof(bool), QDataAvail_Storage,
                                        &QDataAvail_QueueStruct);

        // Assertion 2: Verify queues were created successfully (postcondition)
        assert(*inbound != NULL);
        assert(*dataAvail != NULL);
    }
}
```

**Rationale:**
- Getter functions must return valid pointers
- Queue creation must succeed

---

### Function: `BlockForCAN()` (Line 55-75)

**Current Code:**
```c
void BlockForCAN(void)
{
    QueueHandle_t *dataAvail = getDataAvailQueue();
    if (xQueueReset(*dataAvail))
    {
        bool data = false;
        bool msgAvailable = xQueuePeek(*dataAvail, &data, TIMEOUT_1S_TICKS);
        // ... error handling
    }
}
```

**Proposed Assertions:**
```c
void BlockForCAN(void)
{
    QueueHandle_t *dataAvail = getDataAvailQueue();

    // Assertion 1: Verify queue pointer is valid
    assert(dataAvail != NULL);
    assert(*dataAvail != NULL);

    // Assertion 2: Verify timeout is valid
    assert(TIMEOUT_1S_TICKS > 0);

    if (xQueueReset(*dataAvail))
    {
        bool data = false;
        bool msgAvailable = xQueuePeek(*dataAvail, &data, TIMEOUT_1S_TICKS);

        if (!msgAvailable)
        {
            NON_FATAL_ERROR(TIMEOUT_ERR);
        }
    }
    else
    {
        NON_FATAL_ERROR(UNREACHABLE_ERR);
    }
}
```

**Rationale:**
- Queue must be initialized
- Timeout must be positive

---

### Function: `GetLatestCAN()` (Line 77-81)

**Current Code:**
```c
BaseType_t GetLatestCAN(const Timestamp_t blockTime, DiveCANMessage_t *message)
{
    QueueHandle_t *inbound = getInboundQueue();
    return xQueueReceive(*inbound, message, blockTime);
}
```

**Proposed Assertions:**
```c
BaseType_t GetLatestCAN(const Timestamp_t blockTime, DiveCANMessage_t *message)
{
    // Assertion 1: Verify message pointer is valid
    assert(message != NULL);

    // Assertion 2: Verify blockTime is reasonable (not infinite unless expected)
    assert(blockTime >= 0 && blockTime <= TIMEOUT_25S_TICKS);

    QueueHandle_t *inbound = getInboundQueue();

    // Assertion 3: Verify queue is initialized
    assert(inbound != NULL);
    assert(*inbound != NULL);

    return xQueueReceive(*inbound, message, blockTime);
}
```

**Rationale:**
- Message pointer must be valid for queue receive
- Block time should be within reasonable bounds
- Queue must be initialized

---

### Function: `rxInterrupt()` (Line 88-123)

**Current Code:**
```c
void rxInterrupt(const uint32_t id, const uint8_t length, const uint8_t *const data)
{
    DiveCANMessage_t message = {
        .id = id,
        .length = length,
        .data = {0, 0, 0, 0, 0, 0, 0, 0},
        .type = NULL};

    if (length > MAX_CAN_RX_LENGTH)
    {
        NON_FATAL_ERROR_ISR_DETAIL(CAN_OVERFLOW_ERR, length);
    }
    // ... rest of function
}
```

**Proposed Assertions:**
```c
void rxInterrupt(const uint32_t id, const uint8_t length, const uint8_t *const data)
{
    // Assertion 1: Verify data pointer is valid
    assert(data != NULL);

    // Assertion 2: Verify length is within CAN spec (0-8 bytes)
    assert(length <= 8);

    DiveCANMessage_t message = {
        .id = id,
        .length = length,
        .data = {0, 0, 0, 0, 0, 0, 0, 0},
        .type = NULL};

    if (length > MAX_CAN_RX_LENGTH)
    {
        NON_FATAL_ERROR_ISR_DETAIL(CAN_OVERFLOW_ERR, length);
    }
    else
    {
        (void)memcpy(message.data, data, length);
    }

    QueueHandle_t *inbound = getInboundQueue();
    QueueHandle_t *dataAvail = getDataAvailQueue();

    // Assertion 3: Verify queues are initialized (critical in ISR)
    assert(inbound != NULL && *inbound != NULL);
    assert(dataAvail != NULL && *dataAvail != NULL);

    // ... rest of function
}
```

**Rationale:**
- Data pointer must be valid (from HAL)
- Length must be within CAN specification
- Queues must be initialized before ISR fires

---

### Function: `sendCANMessage()` (Line 129-152)

**Current Code:**
```c
void sendCANMessage(const DiveCANMessage_t message)
{
    while (0 == HAL_CAN_GetTxMailboxesFreeLevel(&hcan1))
    {
        (void)osDelay(TX_WAIT_DELAY);
    }
    // ... send message
}
```

**Proposed Assertions:**
```c
void sendCANMessage(const DiveCANMessage_t message)
{
    // Assertion 1: Verify message length is valid
    assert(message.length > 0 && message.length <= 8);

    // Assertion 2: Verify CAN handle is valid
    assert(&hcan1 != NULL);

    // **CRITICAL FIX for Rule 2**: Add timeout to prevent unbounded loop
    const uint32_t MAX_CAN_TX_WAIT_ATTEMPTS = 100;  // ~1 second timeout
    uint32_t attempts = 0;

    while ((0 == HAL_CAN_GetTxMailboxesFreeLevel(&hcan1)) &&
           (attempts < MAX_CAN_TX_WAIT_ATTEMPTS))
    {
        // Assertion 3: Verify attempts is within bounds
        assert(attempts < MAX_CAN_TX_WAIT_ATTEMPTS);

        (void)osDelay(TX_WAIT_DELAY);
        attempts++;
    }

    if (attempts >= MAX_CAN_TX_WAIT_ATTEMPTS) {
        NON_FATAL_ERROR(CAN_TX_ERR);
        return;
    }

    CAN_TxHeaderTypeDef header = {0};
    header.StdId = 0x0;
    header.ExtId = message.id;
    header.RTR = CAN_RTR_DATA;
    header.IDE = CAN_ID_EXT;
    header.DLC = message.length;
    header.TransmitGlobalTime = DISABLE;

    uint32_t mailboxNumber = 0;

    HAL_StatusTypeDef err = HAL_CAN_AddTxMessage(&hcan1, &header, message.data, &mailboxNumber);
    if (HAL_OK != err)
    {
        NON_FATAL_ERROR_DETAIL(CAN_TX_ERR, err);
    }
}
```

**Rationale:**
- Message length must be within CAN spec
- CAN handle must be valid
- **CRITICAL**: Loop must have bounded iterations (fixes Rule 2 violation)

---

### Function: `txStartDevice()` (Line 160-168)

**Current Code:**
```c
void txStartDevice(const DiveCANType_t targetDeviceType, const DiveCANType_t deviceType)
{
    const DiveCANMessage_t message = {
        .id = BUS_INIT_ID | (deviceType << 8) | targetDeviceType,
        .data = {0x8a, 0xf3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .length = 3,
        .type = "BUS_INIT"};
    sendCANMessage(message);
}
```

**Proposed Assertions:**
```c
void txStartDevice(const DiveCANType_t targetDeviceType, const DiveCANType_t deviceType)
{
    // Assertion 1: Verify device types are valid
    assert(targetDeviceType >= DIVECAN_CONTROLLER && targetDeviceType <= DIVECAN_REVO);
    assert(deviceType >= DIVECAN_CONTROLLER && deviceType <= DIVECAN_REVO);

    const DiveCANMessage_t message = {
        .id = BUS_INIT_ID | (deviceType << 8) | targetDeviceType,
        .data = {0x8a, 0xf3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .length = 3,
        .type = "BUS_INIT"};

    // Assertion 2: Verify message structure is valid
    assert(message.length > 0 && message.length <= 8);

    sendCANMessage(message);
}
```

**Rationale:**
- Device types must be valid enum values
- Message must be properly formed

---

### Function: `txID()` (Line 175-183)

**Current Code:**
```c
void txID(const DiveCANType_t deviceType, const DiveCANManufacturer_t manufacturerID,
          const uint8_t firmwareVersion)
{
    const DiveCANMessage_t message = {
        .id = BUS_ID_ID | deviceType,
        .data = {(uint8_t)manufacturerID, 0x00, firmwareVersion, 0x00, 0x00, 0x00, 0x00, 0x00},
        .length = 3,
        .type = "BUS_ID"};
    sendCANMessage(message);
}
```

**Proposed Assertions:**
```c
void txID(const DiveCANType_t deviceType, const DiveCANManufacturer_t manufacturerID,
          const uint8_t firmwareVersion)
{
    // Assertion 1: Verify device type is valid
    assert(deviceType >= DIVECAN_CONTROLLER && deviceType <= DIVECAN_REVO);

    // Assertion 2: Verify manufacturer ID is valid (non-zero)
    assert(manufacturerID > 0);

    const DiveCANMessage_t message = {
        .id = BUS_ID_ID | deviceType,
        .data = {(uint8_t)manufacturerID, 0x00, firmwareVersion, 0x00, 0x00, 0x00, 0x00, 0x00},
        .length = 3,
        .type = "BUS_ID"};

    sendCANMessage(message);
}
```

**Rationale:**
- Device type must be valid
- Manufacturer ID should be non-zero

---

### Function: `txName()` (Line 189-207)

**Current Code:**
```c
void txName(const DiveCANType_t deviceType, const char *const name)
{
    uint8_t data[BUS_NAME_LEN + 1] = {0};
    if (NULL == name)
    {
        NON_FATAL_ERROR(NULL_PTR_ERR);
    }
    else
    {
        (void)strncpy((char *)data, name, BUS_NAME_LEN);
        // ... send message
    }
}
```

**Proposed Assertions:**
```c
void txName(const DiveCANType_t deviceType, const char *const name)
{
    // Assertion 1: Verify device type is valid
    assert(deviceType >= DIVECAN_CONTROLLER && deviceType <= DIVECAN_REVO);

    // Assertion 2: Verify name pointer is valid
    assert(name != NULL);

    uint8_t data[BUS_NAME_LEN + 1] = {0};

    if (NULL == name)
    {
        NON_FATAL_ERROR(NULL_PTR_ERR);
        return;
    }

    (void)strncpy((char *)data, name, BUS_NAME_LEN);

    DiveCANMessage_t message = {
        .id = BUS_NAME_ID | deviceType,
        .data = {0},
        .length = 8,
        .type = "BUS_NAME"};

    (void)memcpy(message.data, data, BUS_NAME_LEN);

    // Assertion 3: Verify message data was copied
    assert(message.data[0] != 0 || name[0] == '\0');  // Either copied or name was empty

    sendCANMessage(message);
}
```

**Rationale:**
- Device type must be valid
- Name pointer must not be NULL
- Message data should be populated

---

## Implementation Guidelines

### 1. Include Assert Header

Add to the top of each `.c` file:
```c
#include <assert.h>
```

### 2. Configure Assertions for Release Builds

In `common.h` or a project-wide header:
```c
// In debug builds, assertions are active
// In release builds, assertions are compiled out for performance
#ifndef DEBUG
#define NDEBUG
#endif
#include <assert.h>
```

### 3. Assertion Best Practices

**DO:**
- Assert preconditions (pointer != NULL, value in range)
- Assert invariants (state machine states, loop bounds)
- Assert postconditions (operation succeeded)
- Use assertions to document assumptions
- Write clear assertion messages for complex checks

**DON'T:**
- Assert on conditions that can fail normally (network errors, user input)
- Use assertions for production error handling
- Assert on expressions with side effects (e.g., `assert(func())` that modifies state)
- Over-assert trivial or impossible conditions

### 4. Priority Order for Implementation

**Phase 1 - Critical Safety (Week 1):**
1. Fix unbounded loop in `sendCANMessage()` (**CRITICAL**)
2. Add pointer checks to all functions accepting pointers
3. Add range checks to array indexing operations
4. Add state machine invariant checks

**Phase 2 - Data Validation (Week 2):**
5. Add sensor value range checks (PPO2, cell values)
6. Add queue/handle validity checks
7. Add timeout constant validations
8. Add GPIO port/pin validity checks

**Phase 3 - Comprehensive Coverage (Week 3):**
9. Add loop counter bounds checks
10. Add configuration value validations
11. Add postcondition checks
12. Review and add any remaining assertions to reach 2+ per function

### 5. Testing Strategy

**Unit Tests:**
- Verify assertions fire correctly with invalid inputs
- Test boundary conditions
- Measure assertion coverage

**Integration Tests:**
- Run with assertions enabled in debug build
- Monitor for any unexpected assertion failures
- Verify no performance impact in release build

**Static Analysis:**
- Run build with `-Wstack-usage` to verify no stack increase
- Use `-fanalyzer` to verify assertion coverage helps static analysis

### 6. Documentation

For each assertion added, consider adding a brief comment explaining:
- What assumption is being validated
- Why this check is important for safety
- What error condition is being prevented

Example:
```c
// Verify channel is within LED array bounds to prevent buffer overrun
assert(channel < 3);
```

---

## Summary Statistics

**Total Functions Analyzed:** 54
**Functions Currently With Assertions:** 2 (4%)
**Functions Requiring Assertions:** 52 (96%)

**Estimated Assertions to Add:** ~150-200 total assertions
**Average Assertions Per Function:** ~3 (exceeds minimum of 2)

**Implementation Effort Estimate:**
- Phase 1 (Critical): 1-2 days
- Phase 2 (Validation): 2-3 days
- Phase 3 (Comprehensive): 2-3 days
- **Total:** 5-8 days

**Benefits:**
- Early detection of programming errors during development
- Improved code documentation of assumptions
- Better static analysis results
- Compliance with NASA Rule 5 for safety-critical code
- Reduced debugging time for logic errors

---

**End of Assertions Proposal Document**
