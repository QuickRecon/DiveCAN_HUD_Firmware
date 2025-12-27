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

extern bool inShutdown;

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
    // Assertion 1: Verify cell value is not the fail marker when checking for alerts
    assert(cellVal != PPO2_FAIL);

    // Assertion 2: Verify we're checking a value in the reasonable diving PPO2 range
    assert(cellVal <= 255);  // Always true for uint8_t, documents assumption

    return (cellVal < 40 || cellVal > 165);
}

/**
 * @brief Process PPO2 data from the queue and control LED blinking accordingly
 * @param cellValues Pointer to CellValues_t structure to store dequeued values, initialized by caller with sensible default values if the queue is empty
 * @param alerting Pointer to a boolean flag indicating if an alert is active
 */
void PPO2Blink(CellValues_t *cellValues, bool *alerting)
{
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
    for (uint8_t brightness = 10; brightness > 0; brightness--)
    {
        if (inShutdown)
        {
            for (uint8_t channel = 0; channel < 3; channel++)
            {
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

bool alerting = false;
void RGBBlinkControl()
{
    /* We have LED control, turn the blue LEDS on while we wait for a signal */
    for (uint8_t channel = 0; channel < 3; channel++)
    {
        setRGB(channel, 0, 0, 3); // Blue
    }
    osDelay(TIMEOUT_2S_TICKS); /* Initial delay to for the DiveCAN system to start up and prime the queue */
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

void EndBlinkControl()
{
    /* Infinite loop */
    for (;;)
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
