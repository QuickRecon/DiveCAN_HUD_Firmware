#pragma once

/* Mock main.h for testing */
#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32l4xx_hal.h"
#include "MockCAN.h"

/* GPIO Pin and Port definitions are provided by MockHAL.h (via stm32l4xx_hal.h) */

/* CAN_EN pin definitions (used by pwr_management.c) */
extern GPIO_TypeDef *CAN_EN_GPIO_Port;  /* Defined in MockPower.cpp */
#define CAN_EN_Pin GPIO_PIN_14

#ifdef __cplusplus
}
#endif
