#pragma once

#ifndef STM32L4xx_HAL_H
#define STM32L4xx_HAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Include mock HAL component headers */
#include "stm32l4xx_hal_def.h"    /* HAL status types */
#include "stm32l4xx_hal_gpio.h"   /* GPIO types, ports, and pins */
#include "stm32l4xx_hal_can.h"    /* CAN types */
#include "stm32l4xx_hal_pwr_ex.h" /* Power management */

/* Include core HAL function mocks */
#include "MockHAL.h"              /* HAL_GetTick, HAL_GPIO_WritePin */
#include "MockDelay.h"            /* HAL_Delay, IWDG, IRQ control */

#ifdef __cplusplus
}
#endif

#endif /* STM32L4xx_HAL_H */
