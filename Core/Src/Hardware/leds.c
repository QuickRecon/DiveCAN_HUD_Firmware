#include "leds.h"
#include "main.h"
#include <assert.h>
#include "../common.h"

extern TIM_HandleTypeDef htim15;

/* The frequency of the internal clock used to estimate an ~1uS delay */
uint32_t TIMER_MHZ = 8;
uint32_t STARTUP_DELAY_MS = 100;

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
    HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_SET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(G1_GPIO_Port, G1_Pin, GPIO_PIN_SET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(G1_GPIO_Port, G1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(B1_GPIO_Port, B1_Pin, GPIO_PIN_SET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(B1_GPIO_Port, B1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_SET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(G2_GPIO_Port, G2_Pin, GPIO_PIN_SET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(G2_GPIO_Port, G2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(B2_GPIO_Port, B2_Pin, GPIO_PIN_SET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(B2_GPIO_Port, B2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_SET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(G3_GPIO_Port, G3_Pin, GPIO_PIN_SET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(G3_GPIO_Port, G3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(B3_GPIO_Port, B3_Pin, GPIO_PIN_SET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(B3_GPIO_Port, B3_Pin, GPIO_PIN_RESET);

    /* LEDS should be verified, turn off the RED LEDS*/
    HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
    HAL_Delay(STARTUP_DELAY_MS);
    HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);

    /* Test Dimming*/
    // for (int i = 32; i > 0; i--)
    // {
    //     setLEDBrightness(i, R1_GPIO_Port, R1_Pin);
    //     HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_SET);
    //     HAL_Delay(500);
    // }

    // HAL_GPIO_WritePin(R3_GPIO_Port, R1_Pin, GPIO_PIN_RESET);
    // HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Delay for approximately 1 microsecond to get the LED pulse width in the correct regimen
 */
void ledDelay()
{
    HAL_TIM_Base_Stop(&htim15);
    HAL_TIM_Base_Start(&htim15);
    uint32_t start = TIM15->CNT;
    while (TIM15->CNT - start < TIMER_MHZ)
        ; /* Wait ~1uS */
}

void setLEDBrightness(uint8_t level, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    assert(level <= 32);
    uint8_t nPulses = 32 - level;
    if (0 == nPulses)
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
            HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
            // ledDelay();
            HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
            // ledDelay();
        }
        HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
        __enable_irq();
    }
}

void setRGB(uint8_t channel, uint8_t r, uint8_t g, uint8_t b)
{
}

void blinkCode(int8_t c1, int8_t c2, int8_t c3)
{
}

void blinkAlarm(int8_t c1, int8_t c2, int8_t c3)
{
}