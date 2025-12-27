#include "HUDControl.h"
#include "Hardware/leds.h"
#include "main.h"
#include "cmsis_os.h"
#include "common.h"
#include "menu_state_machine.h"
#include "DiveCAN/DiveCAN.h"
#include "Hardware/pwr_management.h"
#include <assert.h>

extern osMessageQueueId_t PPO2QueueHandle;
extern osMessageQueueId_t CellStatQueueHandle;
extern osMessageQueueId_t CalStateQueueHandle;

extern bool inShutdown;
extern bool inCalibration;

inline int16_t div10_round(int16_t x)
{
    /* rounds x/10 to nearest integer, handles negatives safely via int64_t */
    // Assertion 1: Verify input is in reasonable PPO2 range (scaled by 10)
    assert(x >= -1000 && x <= 2550);

    int16_t result = (int16_t)(((int32_t)x + (x >= 0 ? 5 : -5)) / 10);

    // Assertion 2: Verify result is within expected output range
    assert(result >= -100 && result <= 255);

    return result;
}

inline bool cell_alert(uint8_t cellVal)
{
    // Assertion 1: Verify alert thresholds are sane
    assert(40 < 165);  // LOW_THRESHOLD < HIGH_THRESHOLD

    // Calculate alert condition
    bool result = (cellVal < 40 || cellVal > 165);

    // Assertion 2: Verify result is boolean
    assert(result == 0 || result == 1);

    return result;
}

/**
 * @brief Process PPO2 data from the queue and control LED blinking accordingly
 * @param cellValues Pointer to CellValues_t structure to store dequeued values, initialized by caller with sensible default values if the queue is empty
 * @param alerting Pointer to a boolean flag indicating if an alert is active
 */
void PPO2Blink(CellValues_t *cellValues, bool *alerting)
{
    // Assertion 1: Verify pointer parameters are not NULL
    assert(cellValues != NULL);
    assert(alerting != NULL);

    // Assertion 2: Verify queue handle is valid
    assert(PPO2QueueHandle != NULL);

    const uint8_t centerValue = 100;
    /* Dequeue the latest PPO2 information */
    osStatus_t osStat = osMessageQueueGet(PPO2QueueHandle, cellValues, NULL, 0);
    if (osStat != osOK)
    {
        blinkNoData();
    }
    else if (cell_alert(cellValues->C1) || cell_alert(cellValues->C2) || cell_alert(cellValues->C3))
    {
        *alerting = true;
        blinkAlarm();
    }
    else
    {
        *alerting = false;
        osDelay(TIMEOUT_500MS_TICKS); /* Use an extra delay to "partition" the segments */
    }

    int16_t c1 = cellValues->C1 - centerValue;
    int16_t c2 = cellValues->C2 - centerValue;
    int16_t c3 = cellValues->C3 - centerValue;

    c1 = div10_round(c1);
    c2 = div10_round(c2);
    c3 = div10_round(c3);

    uint8_t failMask = ((cellValues->C1 == 0xFF ? 0 : 1) << 0) |
                       ((cellValues->C2 == 0xFF ? 0 : 1) << 1) |
                       ((cellValues->C3 == 0xFF ? 0 : 1) << 2);

    uint8_t statusMask = 0b111;
    osStat = osMessageQueueGet(CellStatQueueHandle, &statusMask, NULL, 0);
    if (osStat != osOK)
    {
        statusMask = 0b111; // Default to all good if no status available
    }

    blinkCode((int8_t)c1, (int8_t)c2, (int8_t)c3, statusMask, failMask, &inShutdown);
}

void ShutdownFadeout()
{
    // Assertion 1: Verify we're in shutdown state when called
    assert(inShutdown == true);

    // Assertion 2: Verify TIMEOUT constant is reasonable
    assert(TIMEOUT_500MS_TICKS > 0);

    for (uint8_t brightness = 10; brightness > 0; brightness--)
    {
        // Assertion 3: Verify brightness is in valid LED range
        assert(brightness <= LED_MAX_BRIGHTNESS);

        if (inShutdown)
        {
            for (uint8_t channel = 0; channel < 3; channel++)
            {
                assert(channel < 3);
                setRGB(channel, brightness, 0, 0); // Red fading
            }
            osDelay(TIMEOUT_500MS_TICKS);
        } else {
            /* We'll zip through the rest of the loop and end */
        }
    }
    if(inShutdown){
        Shutdown();
    }
}

void CalibrationCountdown()
{
    /* Total countdown time ~20 seconds across 3 channels */
    /* Each channel gets approximately 6.67 seconds */
    const uint8_t STEPS_PER_CHANNEL = 20; /* 20 steps per channel */
    const TickType_t DELAY_PER_STEP = pdMS_TO_TICKS(333); /* 333ms per step = ~6.67s per channel */
    const uint8_t BRIGHTNESS = 5;

    CalibrationState_t calState = CAL_STATE_REQUESTED;

    /* Countdown from left to right (channels 0, 1, 2) */
    for (uint8_t channel = 0; channel < 3 && inCalibration; channel++)
    {
        for (uint8_t step = 0; step < STEPS_PER_CHANNEL && inCalibration; step++)
        {
            /* Set current channel to blue to indicate waiting */
            setRGB(channel, 0, 0, BRIGHTNESS);

            /* Check for calibration response */
            osStatus_t status = osMessageQueueGet(CalStateQueueHandle, &calState, NULL, DELAY_PER_STEP);

            if (status == osOK && (calState == CAL_STATE_SUCCESS || calState == CAL_STATE_FAILURE))
            {
                /* We got a response, break out of countdown */
                goto calibration_result;
            }
        }
        /* Turn off the current channel after it's done */
        setRGB(channel, 0, 0, 0);
    }

    /* If we get here, calibration timed out */
    calState = CAL_STATE_TIMEOUT;

calibration_result:
    /* Display result */
    if (calState == CAL_STATE_SUCCESS)
    {
        /* Flash green 3 times */
        for (uint8_t i = 0; i < 3 && inCalibration; i++)
        {
            for (uint8_t channel = 0; channel < 3; channel++)
            {
                setRGB(channel, 0, 10, 0); /* Green */
            }
            osDelay(TIMEOUT_500MS_TICKS);
            for (uint8_t channel = 0; channel < 3; channel++)
            {
                setRGB(channel, 0, 0, 0); /* Off */
            }
            osDelay(TIMEOUT_500MS_TICKS);
        }
    }
    else
    {
        /* Flash red 3 times for failure or timeout */
        for (uint8_t i = 0; i < 3 && inCalibration; i++)
        {
            for (uint8_t channel = 0; channel < 3; channel++)
            {
                setRGB(channel, 10, 0, 0); /* Red */
            }
            osDelay(TIMEOUT_500MS_TICKS);
            for (uint8_t channel = 0; channel < 3; channel++)
            {
                setRGB(channel, 0, 0, 0); /* Off */
            }
            osDelay(TIMEOUT_500MS_TICKS);
        }
    }

    /* Clear all LEDs */
    for (uint8_t channel = 0; channel < 3; channel++)
    {
        setRGB(channel, 0, 0, 0);
    }
}

bool alerting = false;
void RGBBlinkControl()
{
    // Assertion 1: Verify TIMEOUT constant is valid
    assert(TIMEOUT_2S_TICKS > 0);

    // Assertion 2: Verify LED brightness constant is in range
    assert(3 <= LED_MAX_BRIGHTNESS);

    /* We have LED control, turn the blue LEDS on while we wait for a signal */
    for (uint8_t channel = 0; channel < 3; channel++)
    {
        assert(channel < 3);
        setRGB(channel, 0, 0, 3); // Blue
    }
    osDelay(TIMEOUT_2S_TICKS); /* Initial delay to for the DiveCAN system to start up and prime the queue */
    CellValues_t cellValues = {0};
    for (;;)  // Infinite loop acceptable for RTOS task
    {
        if (inShutdown)
        {
            ShutdownFadeout();
        }
        else if (inCalibration)
        {
            CalibrationCountdown();
        }
        else
        {
            PPO2Blink(&cellValues, &alerting);
        }
    }
}

void EndBlinkControl()
{
    // Assertion 1: Verify GPIO port pointers are valid
    assert(LED_0_GPIO_Port != NULL);
    assert(LED_1_GPIO_Port != NULL);
    assert(LED_2_GPIO_Port != NULL);
    assert(LED_3_GPIO_Port != NULL);

    // Assertion 2: Verify timeout constant is valid
    assert(TIMEOUT_100MS_TICKS > 0);

    /* Infinite loop */
    for (;;)  // Infinite loop acceptable for RTOS task
    {
        if (alerting && !menuActive())
        {
            HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_SET);
            osDelay(TIMEOUT_100MS_TICKS);
            HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
        }
        osDelay(TIMEOUT_100MS_TICKS);
    }
}
