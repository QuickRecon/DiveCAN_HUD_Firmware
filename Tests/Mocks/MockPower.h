#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations for types used by pwr_management.c */
/* CAN TypeDef (opaque in tests) */
#ifndef CAN_TYPEDEF
#define CAN_TYPEDEF
typedef struct {
    uint32_t dummy;
} CAN_TypeDef;
#endif

#ifndef CAN_HANDLETYPEDEF
#define CAN_HANDLETYPEDEF
typedef struct {
    CAN_TypeDef *Instance;
    uint32_t State;
    uint32_t ErrorCode;
} CAN_HandleTypeDef;
#endif

/* External handle declarations */
extern CAN_HandleTypeDef hcan1;

/* PWR GPIO port definitions (from stm32l4xx_hal_pwr_ex.h) */
#define PWR_GPIO_A 0x00000000U
#define PWR_GPIO_B 0x00000001U
#define PWR_GPIO_C 0x00000002U
#define PWR_GPIO_D 0x00000003U
#define PWR_GPIO_E 0x00000004U
#define PWR_GPIO_F 0x00000005U
#define PWR_GPIO_G 0x00000006U
#define PWR_GPIO_H 0x00000007U

/* PWR GPIO bit definitions (from stm32l4xx_hal_pwr_ex.h) */
#define PWR_GPIO_BIT_0  (1U << 0)
#define PWR_GPIO_BIT_1  (1U << 1)
#define PWR_GPIO_BIT_2  (1U << 2)
#define PWR_GPIO_BIT_3  (1U << 3)
#define PWR_GPIO_BIT_4  (1U << 4)
#define PWR_GPIO_BIT_5  (1U << 5)
#define PWR_GPIO_BIT_6  (1U << 6)
#define PWR_GPIO_BIT_7  (1U << 7)
#define PWR_GPIO_BIT_8  (1U << 8)
#define PWR_GPIO_BIT_9  (1U << 9)
#define PWR_GPIO_BIT_10 (1U << 10)
#define PWR_GPIO_BIT_11 (1U << 11)
#define PWR_GPIO_BIT_12 (1U << 12)
#define PWR_GPIO_BIT_13 (1U << 13)
#define PWR_GPIO_BIT_14 (1U << 14)
#define PWR_GPIO_BIT_15 (1U << 15)

/* HAL Power API */
void HAL_PWREx_EnablePullUpPullDownConfig(void);
HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber);
HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber);
void HAL_PWR_EnterSTANDBYMode(void);

/* HAL GPIO API - subset needed for testing */
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/* Mock control functions */
void MockPower_Reset(void);
void MockPower_SetPullDownBehavior(HAL_StatusTypeDef returnValue);
void MockPower_SetPullUpBehavior(HAL_StatusTypeDef returnValue);

/* Mock query functions */
bool MockPower_GetPullUpDownConfigEnabled(void);
bool MockPower_GetStandbyEntered(void);
uint32_t MockPower_GetPullDownCallCount(void);
uint32_t MockPower_GetPullUpCallCount(void);
bool MockPower_IsPinPulledDown(uint32_t GPIO, uint32_t GPIONumber);
bool MockPower_IsPinPulledUp(uint32_t GPIO, uint32_t GPIONumber);

/* GPIO mock control */
void MockPower_SetPinReadValue(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState value);
uint32_t MockPower_GetGPIOInitCallCount(void);
bool MockPower_VerifyGPIOInit(GPIO_TypeDef *GPIOx, uint16_t pin, uint32_t mode, uint32_t pull);

#ifdef __cplusplus
}
#endif
