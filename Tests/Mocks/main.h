#pragma once

/* Mock main.h for testing */
/* This provides the GPIO port definitions needed by menu_state_machine.c */

#include "MockHAL.h"
#include "stm32l4xx_hal_gpio.h"

/* CAN_EN pin definitions (used by pwr_management.c) */
#define CAN_EN_Pin GPIO_PIN_14
extern GPIO_TypeDef* CAN_EN_GPIO_Port;

#ifdef TESTING_CAN
#include "stm32l4xx_hal_can.h"

/* CAN handle declaration */
extern CAN_HandleTypeDef hcan1;
#endif
