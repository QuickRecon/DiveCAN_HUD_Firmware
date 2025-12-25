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

/* Mock GPIO port definitions */
extern GPIO_TypeDef* LED_0_GPIO_Port;
extern GPIO_TypeDef* LED_1_GPIO_Port;
extern GPIO_TypeDef* LED_2_GPIO_Port;
extern GPIO_TypeDef* LED_3_GPIO_Port;

#define LED_0_Pin GPIO_PIN_0
#define LED_1_Pin GPIO_PIN_1
#define LED_2_Pin GPIO_PIN_2
#define LED_3_Pin GPIO_PIN_3

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
