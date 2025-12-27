#include "menu_state_machine.h"
#include "main.h"
#include "common.h"
#include <assert.h>
#include "DiveCAN/Transciever.h"

extern osMessageQueueId_t CalStateQueueHandle;

/* The gist of the menu system is as follows:
 *  Pressing the button 4 times, and holding on the 4th time will trigger the hud to shut down
 *  Pressing the button 8 times, holding on the 4th time, will trigger calibration mode
 *  Any other number of presses will just exit the menu back to normal operation
 *  As the number of buttons is pressed we light each of the end LEDS in sequence to indicate how many presses have been registered
 *  First 4 presses count up the LEDS, second 4 count down
 */

typedef enum
{
    MENU_STATE_IDLE = 0,
    MENU_STATE_1PRESS,
    MENU_STATE_2PRESS,
    MENU_STATE_3PRESS,
    MENU_STATE_SHUTDOWN,
    MENU_STATE_4PRESS,
    MENU_STATE_5PRESS,
    MENU_STATE_6PRESS,
    MENU_STATE_7PRESS,
    MENU_STATE_CALIBRATE
} MenuState_t;

typedef enum
{
    NONE,
    PRESS,
    PRESSED,
    HOLD
} ButtonState_t;

static MenuState_t currentMenuState = MENU_STATE_IDLE;
static uint32_t buttonPressTimestamp = 0;
static uint32_t timeInState = 0;

const uint32_t BUTTON_HOLD_TIME_MS = TIMEOUT_2S_TICKS;
const uint32_t BUTTON_PRESS_TIME_MS = TIMEOUT_100MS_TICKS;
const uint32_t MENU_MODE_TIMEOUT_MS = TIMEOUT_10S_TICKS;

/**
 * @brief Allow external code to query if the menu is active, mainly so we aren't trying to fire alarms overtop of the menu
 * @return True if menu is active, false if not
 */
bool menuActive()
{
    // Assertion: Verify current state is valid
    assert(currentMenuState <= MENU_STATE_CALIBRATE);

    return currentMenuState != MENU_STATE_IDLE;
}

bool inShutdown = false;
bool inCalibration = false;

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

void onButtonRelease()
{
    buttonPressTimestamp = 0;

    // Assertion: Verify timestamp was cleared (postcondition)
    assert(buttonPressTimestamp == 0);
}

bool incrementState(ButtonState_t button_state)
{
    // Assertion 1: Verify button state is valid
    assert(button_state <= HOLD);

    // Assertion 2: Verify current menu state is valid before transition
    assert(currentMenuState <= MENU_STATE_CALIBRATE);

    MenuState_t previousState = currentMenuState;
    switch (currentMenuState)
    {
    case MENU_STATE_IDLE:
        if (button_state == PRESS)
        {
            currentMenuState = MENU_STATE_1PRESS;
        }
        break;
    case MENU_STATE_1PRESS:
        if (button_state == PRESS)
        {
            currentMenuState = MENU_STATE_2PRESS;
        }
        break;
    case MENU_STATE_2PRESS:
        if (button_state == PRESS)
        {
            currentMenuState = MENU_STATE_3PRESS;
        }
        break;
    case MENU_STATE_3PRESS:
        if (button_state == PRESS)
        {
            currentMenuState = MENU_STATE_4PRESS;
        }
        break;
    case MENU_STATE_SHUTDOWN:
        // Stay here until reset
        if (button_state == NONE)
        {
            // Go back to idle once button is released
            currentMenuState = MENU_STATE_IDLE;
        }
        break;
    case MENU_STATE_4PRESS:
        if (button_state == HOLD)
        {
            currentMenuState = MENU_STATE_SHUTDOWN;
        }
        else if (button_state == PRESS)
        {
            currentMenuState = MENU_STATE_5PRESS;
        }
        break;
    case MENU_STATE_5PRESS:
        if (button_state == PRESS)
        {
            currentMenuState = MENU_STATE_6PRESS;
        }
        break;
    case MENU_STATE_6PRESS:
        if (button_state == PRESS)
        {
            currentMenuState = MENU_STATE_7PRESS;
        }
        break;
    case MENU_STATE_7PRESS:
        if (button_state == HOLD)
        {
            currentMenuState = MENU_STATE_CALIBRATE;
        }
        else if (button_state == PRESS)
        {
            currentMenuState = MENU_STATE_IDLE;
        }
        break;
    case MENU_STATE_CALIBRATE:
        currentMenuState = MENU_STATE_IDLE;
        break;
    default:
        currentMenuState = MENU_STATE_IDLE;
        break;
    }

    // Assertion 3: Verify new state is valid after transition
    assert(currentMenuState <= MENU_STATE_CALIBRATE);

    return previousState != currentMenuState;
}

void resetMenuStateMachine()
{
    currentMenuState = MENU_STATE_IDLE;
    timeInState = 0;
    buttonPressTimestamp = 0;

    // Postcondition assertions: Verify reset succeeded
    assert(currentMenuState == MENU_STATE_IDLE);
    assert(timeInState == 0);
    assert(buttonPressTimestamp == 0);
}

void displayLEDsForState()
{
    // Assertion 1: Verify current state is valid
    assert(currentMenuState <= MENU_STATE_CALIBRATE);

    // Assertion 2: Verify GPIO ports are valid
    assert(LED_0_GPIO_Port != NULL);
    assert(LED_1_GPIO_Port != NULL);
    assert(LED_2_GPIO_Port != NULL);
    assert(LED_3_GPIO_Port != NULL);

    static uint32_t lastFlashToggle = 0;
    static bool ledFlashState = false;

    switch (currentMenuState)
    {
    case MENU_STATE_IDLE:
        // Turn off all LEDs
        HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
        break;
    case MENU_STATE_1PRESS:
        // Light first LED
        HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
        break;
    case MENU_STATE_2PRESS:
        // Light first two LEDs
        HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
        break;
    case MENU_STATE_3PRESS:
        // Light first three LEDs
        HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
        break;
    case MENU_STATE_SHUTDOWN:
        // Flash all LEDs to indicate shutdown
        HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_SET);
        break;
    case MENU_STATE_4PRESS:
        // Light last three LEDs
        HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_SET);
        break;
    case MENU_STATE_5PRESS:
        // Light last two LEDs
        HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_SET);
        break;
    case MENU_STATE_6PRESS:
        // Light last LED
        HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_SET);
        break;
    case MENU_STATE_7PRESS:
        // Light all LEDs
        HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_SET);
        break;
    case MENU_STATE_CALIBRATE:
        // Flash all LEDs to indicate calibration mode
        if (HAL_GetTick() - lastFlashToggle > 100) /* Flash every 100ms */
        {
            ledFlashState = !ledFlashState;
            lastFlashToggle = HAL_GetTick();
        }
        GPIO_PinState pinState = ledFlashState ? GPIO_PIN_SET : GPIO_PIN_RESET;
        HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, pinState);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, pinState);
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, pinState);
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, pinState);
        break;
    default:
        // Turn off all LEDs
        break;
    }
}

void menuStateMachineTick()
{
    // Assertion 1: Verify timeout constants are valid
    assert(BUTTON_HOLD_TIME_MS > 0);
    assert(BUTTON_PRESS_TIME_MS > 0);
    assert(MENU_MODE_TIMEOUT_MS > 0);

    ButtonState_t button_state = NONE;
    static bool calibrationRequested = false;

    if (buttonPressTimestamp != 0)
    {
        if (HAL_GetTick() - buttonPressTimestamp > BUTTON_HOLD_TIME_MS)
        {
            button_state = HOLD;
        }
        else if (HAL_GetTick() - buttonPressTimestamp > BUTTON_PRESS_TIME_MS && button_state == NONE)
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

        /* Trigger calibration request when entering calibration state */
        if (currentMenuState == MENU_STATE_CALIBRATE && !calibrationRequested)
        {
            calibrationRequested = true;
            /* Send calibration request with PPO2 of 1.0 (100 = 1.0 bar) and standard atmospheric pressure (1013 mbar) */
            txCalReq(DIVECAN_MONITOR, DIVECAN_OBOE, 100, 1013);
            /* Reset the calibration queue to start fresh */
            osMessageQueueReset(CalStateQueueHandle);
            /* Set initial calibration state to REQUESTED */
            CalibrationState_t calState = CAL_STATE_REQUESTED;
            osMessageQueuePut(CalStateQueueHandle, &calState, 0, 0);
        }
    }

    inShutdown = currentMenuState == MENU_STATE_SHUTDOWN;
    inCalibration = currentMenuState == MENU_STATE_CALIBRATE;

    /* Reset calibration flag when leaving calibration state */
    if (currentMenuState != MENU_STATE_CALIBRATE)
    {
        calibrationRequested = false;
    }

    if ((timeInState != 0) && (HAL_GetTick() - timeInState > MENU_MODE_TIMEOUT_MS) && buttonPressTimestamp == 0)
    {
        resetMenuStateMachine();
    }
}