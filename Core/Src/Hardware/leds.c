#include "leds.h"
#include "main.h"
#include <assert.h>
#include "../common.h"

extern TIM_HandleTypeDef htim15;

/* The frequency of the internal clock used to estimate an ~1uS delay */
const uint32_t TIMER_MHZ = 8;
const uint32_t STARTUP_DELAY_MS = 500;
const uint8_t LED_MAX_BRIGHTNESS = 32;
const uint8_t LED_BRIGHTNESS[3] = {10, 3, 3}; // R, G, B brightness levels, gotta push red a bit harder because its a lower voltage
const uint8_t MAX_BLINKS = 10;
const uint32_t BLINK_PERIOD = 250;


struct GPIO_PinMap
{
    GPIO_TypeDef *port;
    uint16_t pin;
};

const struct GPIO_PinMap LED_PinMap[3][3] = {
    {{R1_GPIO_Port, R1_Pin}, {G1_GPIO_Port, G1_Pin}, {B1_GPIO_Port, B1_Pin}},
    {{R2_GPIO_Port, R2_Pin}, {G2_GPIO_Port, G2_Pin}, {B2_GPIO_Port, B2_Pin}},
    {{R3_GPIO_Port, R3_Pin}, {G3_GPIO_Port, G3_Pin}, {B3_GPIO_Port, B3_Pin}}};

void setLEDBrightness(uint8_t level, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void initLEDs(void)
{
    /* Turn all the end LEDs on*/
    HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ASC_EN_GPIO_Port, ASC_EN_Pin, GPIO_PIN_SET);

    /* Turn the main LEDS off*/
    HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(G1_GPIO_Port, G1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(G2_GPIO_Port, G2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(G3_GPIO_Port, G3_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(B1_GPIO_Port, B1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(B2_GPIO_Port, B2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(B3_GPIO_Port, B3_Pin, GPIO_PIN_RESET);

    /* Sequence through them one at a time */
    for (uint8_t i = 0; i < 3; i++)
    {
        setRGB(i, LED_BRIGHTNESS[0], 0, 0); /* Red*/
        HAL_Delay(STARTUP_DELAY_MS);
        setRGB(i, 0, LED_BRIGHTNESS[1], 0); /* Green*/
        HAL_Delay(STARTUP_DELAY_MS);
        setRGB(i, 0, 0, LED_BRIGHTNESS[2]); /* Blue*/
        HAL_Delay(STARTUP_DELAY_MS);
        setRGB(i, 0, 0, 0); /* Off*/
    }

    /* LEDS should be verified, turn off the RED LEDS*/
    HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);

}

void setLEDBrightness(uint8_t level, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    assert(level <= 32);
    uint8_t nPulses = 32 - level;
    if (0 == level)
    {
        HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
    }
    else
    {

        HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
        HAL_Delay(10); // Reset the LED
        __disable_irq();
        for (uint8_t i = 0; i < nPulses; i++) /* This could probably be done as a timer with HW interrupts but lets be lazy until we determine we need the CPU cycles*/
        {
            HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET); /* At 8Mhz we land within the acceptable timing by just toggling in place*/
            HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
        }
        HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
        __enable_irq();
    }
}

void setRGB(uint8_t channel, uint8_t r, uint8_t g, uint8_t b)
{
    setLEDBrightness(r, LED_PinMap[channel][0].port, LED_PinMap[channel][0].pin);
    setLEDBrightness(g, LED_PinMap[channel][1].port, LED_PinMap[channel][1].pin);
    setLEDBrightness(b, LED_PinMap[channel][2].port, LED_PinMap[channel][2].pin);
}

/**
 * @brief Blink LEDs in a smithers code, negative values imply red, positive implies green
 * @param c1 Channel 1 blinks
 * @param c2 Channel 2 blinks
 * @param c3 Channel 3 blinks
 */
void blinkCode(int8_t c1, int8_t c2, int8_t c3)
{
    int8_t channel_values[3] = {c1, c2, c3};
    for (uint8_t i = 0; i < MAX_BLINKS; i++)
    {
        for (uint8_t channel = 0; channel < 3; channel++)
        {
            if (channel_values[channel] != 0)
            {
                if (i < abs(channel_values[channel]))
                {
                    if (channel_values[channel] > 0)
                    {
                        setRGB(channel, 0, LED_BRIGHTNESS[channel], 0); // Green
                    }
                    else
                    {
                        setRGB(channel, LED_BRIGHTNESS[channel], 0, 0); // Red
                    }
                }
                else
                {
                    setRGB(channel, 0, 0, 0); // Off
                }
            }
        }
        HAL_Delay(BLINK_PERIOD); // Let the digits cook for a bit
        for (uint8_t channel = 0; channel < 3; channel++) // Turn everything off
        {
            setRGB(channel, 0, 0, 0); // Off
        }
        HAL_Delay(BLINK_PERIOD);
    }
}

void blinkAlarm(int8_t c1, int8_t c2, int8_t c3)
{
}