#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Mock GPIO types and definitions */
typedef struct {
    uint32_t dummy;
} GPIO_TypeDef;

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
} GPIO_PinState;

#define GPIO_PIN_0  ((uint16_t)0x0001)
#define GPIO_PIN_1  ((uint16_t)0x0002)
#define GPIO_PIN_2  ((uint16_t)0x0004)
#define GPIO_PIN_3  ((uint16_t)0x0008)

/* Extern port instances for compile-time constant addresses */
extern GPIO_TypeDef LED_0_Port_Static;
extern GPIO_TypeDef LED_1_Port_Static;
extern GPIO_TypeDef LED_2_Port_Static;
extern GPIO_TypeDef LED_3_Port_Static;
extern GPIO_TypeDef R_Port_Static;
extern GPIO_TypeDef G_Port_Static;
extern GPIO_TypeDef B_Port_Static;
extern GPIO_TypeDef ASC_EN_Port_Static;

/* Mock GPIO port definitions - End LEDs (as compile-time constant macros) */
#define LED_0_GPIO_Port (&LED_0_Port_Static)
#define LED_1_GPIO_Port (&LED_1_Port_Static)
#define LED_2_GPIO_Port (&LED_2_Port_Static)
#define LED_3_GPIO_Port (&LED_3_Port_Static)

#define LED_0_Pin GPIO_PIN_0
#define LED_1_Pin GPIO_PIN_1
#define LED_2_Pin GPIO_PIN_2
#define LED_3_Pin GPIO_PIN_3

/* Mock GPIO port definitions - RGB LEDs (as compile-time constant macros) */
#define R1_GPIO_Port (&R_Port_Static)
#define R2_GPIO_Port (&R_Port_Static)
#define R3_GPIO_Port (&R_Port_Static)
#define G1_GPIO_Port (&G_Port_Static)
#define G2_GPIO_Port (&G_Port_Static)
#define G3_GPIO_Port (&G_Port_Static)
#define B1_GPIO_Port (&B_Port_Static)
#define B2_GPIO_Port (&B_Port_Static)
#define B3_GPIO_Port (&B_Port_Static)
#define ASC_EN_GPIO_Port (&ASC_EN_Port_Static)

#define R1_Pin GPIO_PIN_0
#define R2_Pin GPIO_PIN_1
#define R3_Pin GPIO_PIN_2
#define G1_Pin GPIO_PIN_0
#define G2_Pin GPIO_PIN_1
#define G3_Pin GPIO_PIN_2
#define B1_Pin GPIO_PIN_0
#define B2_Pin GPIO_PIN_1
#define B3_Pin GPIO_PIN_2
#define ASC_EN_Pin GPIO_PIN_0
#define GPIO_PIN_14 ((uint16_t)0x4000)

/* Mock HAL functions */
uint32_t HAL_GetTick(void);
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);

/* Test helper functions to control mock behavior */
void MockHAL_SetTick(uint32_t tick);
void MockHAL_IncrementTick(uint32_t increment);
void MockHAL_Reset(void);

/* GPIO state query for verification */
GPIO_PinState MockHAL_GetPinState(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

#ifdef __cplusplus
}
#endif
