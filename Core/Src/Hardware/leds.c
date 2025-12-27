#include "leds.h"
#include "main.h"
#include <assert.h>
#include "../common.h"

/* The frequency of the internal clock used to estimate an ~1uS delay */
const uint32_t STARTUP_DELAY_MS = 500;
const uint8_t LED_MAX_BRIGHTNESS = 32;
const uint8_t LED_BRIGHTNESS[3] = {10, 3, 3}; // R, G, B brightness levels, gotta push red a bit harder because its a lower voltage
const uint8_t LED_MIN_BRIGHTNESS = 3;
const uint8_t MAX_BLINKS = 25;
const uint32_t BLINK_PERIOD = TIMEOUT_250MS_TICKS;

extern IWDG_HandleTypeDef hiwdg;

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
    // Assertion 1: Verify GPIO ports are initialized
    assert(LED_0_GPIO_Port != NULL);
    assert(R1_GPIO_Port != NULL);
    assert(G1_GPIO_Port != NULL);
    assert(B1_GPIO_Port != NULL);
    assert(ASC_EN_GPIO_Port != NULL);

    // Assertion 2: Verify LED brightness constants are in range
    assert(LED_BRIGHTNESS[0] <= LED_MAX_BRIGHTNESS);
    assert(LED_BRIGHTNESS[1] <= LED_MAX_BRIGHTNESS);
    assert(LED_BRIGHTNESS[2] <= LED_MAX_BRIGHTNESS);

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

    /* Show each color*/
    for (uint8_t i = 0; i < 3; i++)
    {
        setRGB(i, LED_BRIGHTNESS[0], 0, 0); /* Red*/
    }
    HAL_Delay(STARTUP_DELAY_MS);
    (void)HAL_IWDG_Refresh(&hiwdg);
    for (uint8_t i = 0; i < 3; i++)
    {
        setRGB(i, 0, LED_BRIGHTNESS[1], 0); /* Green */
    }
    HAL_Delay(STARTUP_DELAY_MS);
    (void)HAL_IWDG_Refresh(&hiwdg);
    for (uint8_t i = 0; i < 3; i++)
    {
        setRGB(i, 0, 0, LED_BRIGHTNESS[2]); /* Blue */
    }
    HAL_Delay(STARTUP_DELAY_MS);
    (void)HAL_IWDG_Refresh(&hiwdg);
    for (uint8_t i = 0; i < 3; i++)
    {
        setRGB(i, 0, 0, 0); /* Off */
    }

    /* LEDS should be verified, turn off the RED LEDS*/
    HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
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
        HAL_Delay(6); // Reset the LED, 5ms is the maximum specified delay
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
    // Assertion 1: Verify channel is within valid range
    assert(channel < 3);

    // Assertion 2: Verify RGB values are within LED brightness limits
    assert(r <= LED_MAX_BRIGHTNESS);
    assert(g <= LED_MAX_BRIGHTNESS);
    assert(b <= LED_MAX_BRIGHTNESS);

    setLEDBrightness(r, LED_PinMap[channel][0].port, LED_PinMap[channel][0].pin);
    setLEDBrightness(g, LED_PinMap[channel][1].port, LED_PinMap[channel][1].pin);
    setLEDBrightness(b, LED_PinMap[channel][2].port, LED_PinMap[channel][2].pin);
}

/**
 * @brief Blink LEDs in a smithers code, negative values imply red, positive implies green
 * @param c1 Channel 1 blinks
 * @param c2 Channel 2 blinks
 * @param c3 Channel 3 blinks
 * @param statusMask 3 bit wide mask to indicate which cells are voted in, voted out cells get a yellow background, 1 implies cell voted in
 * @param failMask 3 bit wide mask to indicate which cells are in fail state, failed cells get constant red indication, 1 implies cell OK
 * @param break Pointer to a boolean that can be set to true to break out of the blink early
 */
void blinkCode(int8_t c1, int8_t c2, int8_t c3, uint8_t statusMask, uint8_t failMask, bool *breakout)
{
    int8_t channel_values[3] = {c1, c2, c3};

    /* Work out the max of the absolute values */
    uint8_t max_blinks = 0;
    for (uint8_t channel = 0; channel < 3; channel++)
    {
        /* Only count non-failed cells towards the blinking */
        if ((abs(channel_values[channel]) > max_blinks) && ((failMask & (1 << channel)) != 0))
        {
            max_blinks = abs(channel_values[channel]);
        }
    }

    assert(max_blinks <= MAX_BLINKS);

    /* Do the loop and blink the blink*/
    for (uint8_t i = 0; i < max_blinks; i++)
    {
        for (uint8_t channel = 0; channel < 3; channel++)
        {
            if (channel_values[channel] != 0)
            {
                if (i < abs(channel_values[channel]))
                {
                    if (channel_values[channel] > 0 && ((failMask & (1 << channel)) != 0))
                    {
                        setRGB(channel, 0, LED_BRIGHTNESS[1], 0); // Green
                    }
                    else
                    {
                        setRGB(channel, LED_BRIGHTNESS[0], 0, 0); // Red
                    }
                }
                else
                {
                    if ((failMask & (1 << channel)) == 0)
                    {
                        setRGB(channel, LED_MIN_BRIGHTNESS, 0, 0); // Red background for failed cells
                    }
                    else if ((statusMask & (1 << channel)) == 0)
                    {
                        setRGB(channel, LED_MIN_BRIGHTNESS, LED_MIN_BRIGHTNESS, 0); // Yellow background for voted out cells
                    }
                    else
                    {
                        setRGB(channel, 0, 0, 0); // Off
                    }
                }
            }
        }
        if (breakout != NULL && *breakout)
        {
            return;
        }
        osDelay(BLINK_PERIOD);                            // Let the digits cook for a bit
        for (uint8_t channel = 0; channel < 3; channel++) // Turn everything off
        {
            if ((failMask & (1 << channel)) == 0)
            {
                setRGB(channel, LED_MIN_BRIGHTNESS, 0, 0); // Red background for failed cells
            }
            else if ((statusMask & (1 << channel)) == 0)
            {
                setRGB(channel, 15, 3, 0); // Yellow background for voted out cells
            }
            else
            {
                setRGB(channel, 0, 0, 0); // Off
            }
        }
        if (breakout != NULL && *breakout)
        {
            return;
        }
        osDelay(BLINK_PERIOD);
    }
}

/**
 * @brief Do a blue blink to indicate that the displayed data is stale
 * @param
 */
void blinkNoData(void)
{
    // Assertion 1: Verify LED brightness constant is valid
    assert(LED_BRIGHTNESS[2] <= LED_MAX_BRIGHTNESS);

    // Assertion 2: Verify blink period is valid
    assert(BLINK_PERIOD > 0);

    /* Do the loop and blink the blink*/
    for (uint8_t i = 0; i < 2; i++)
    {
        for (uint8_t channel = 0; channel < 3; channel++)
        {
            assert(channel < 3);
            setRGB(channel, 0, 0, LED_BRIGHTNESS[2]); // Blue
        }
        osDelay(BLINK_PERIOD);                            // Let the digits cook for a bit
        for (uint8_t channel = 0; channel < 3; channel++) // Turn everything off
        {
            assert(channel < 3);
            setRGB(channel, 0, 0, 0); // Off
        }
        osDelay(BLINK_PERIOD);
    }
    osDelay(BLINK_PERIOD * 2); // Extra delay at the end
}

void blinkAlarm()
{
    // Assertion 1: Verify LED max brightness is valid
    assert(LED_MAX_BRIGHTNESS <= 32);
    assert(LED_MAX_BRIGHTNESS > 0);

    // Assertion 2: Verify timeout constant is valid
    assert(TIMEOUT_50MS_TICKS > 0);

    /* We go do a "nightrider" sweep to the left and back to the right 3 times (100ms per LED) */
    for (int i = 0; i < 5; i++)
    {
        assert(i >= 0 && i < 5);
        setRGB(0, LED_MAX_BRIGHTNESS, 0, 0);
        osDelay(TIMEOUT_50MS_TICKS);
        setRGB(0, 0, 0, 0);
        setRGB(1, LED_MAX_BRIGHTNESS, 0, 0);
        osDelay(TIMEOUT_50MS_TICKS);
        setRGB(1, 0, 0, 0);
        setRGB(2, LED_MAX_BRIGHTNESS, 0, 0);
        osDelay(TIMEOUT_50MS_TICKS);
        setRGB(2, 0, 0, 0);
        setRGB(1, LED_MAX_BRIGHTNESS, 0, 0);
        osDelay(TIMEOUT_50MS_TICKS);
        setRGB(1, 0, 0, 0);
        setRGB(0, LED_MAX_BRIGHTNESS, 0, 0);
        osDelay(TIMEOUT_50MS_TICKS);
        setRGB(0, 0, 0, 0);
        osDelay(TIMEOUT_50MS_TICKS);
    }
}