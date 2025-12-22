/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

    /* Private includes ----------------------------------------------------------*/
    /* USER CODE BEGIN Includes */

    /* USER CODE END Includes */

    /* Exported types ------------------------------------------------------------*/
    /* USER CODE BEGIN ET */

    /* USER CODE END ET */

    /* Exported constants --------------------------------------------------------*/
    /* USER CODE BEGIN EC */

    /* USER CODE END EC */

    /* Exported macro ------------------------------------------------------------*/
    /* USER CODE BEGIN EM */

    /* USER CODE END EM */

    /* Exported functions prototypes ---------------------------------------------*/
    void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CAN_EN_Pin GPIO_PIN_14
#define CAN_EN_GPIO_Port GPIOC
#define CAN_EN_EXTI_IRQn EXTI15_10_IRQn
#define ASC_EN_Pin GPIO_PIN_15
#define ASC_EN_GPIO_Port GPIOC
#define B1_Pin GPIO_PIN_1
#define B1_GPIO_Port GPIOA
#define G1_Pin GPIO_PIN_2
#define G1_GPIO_Port GPIOA
#define R1_Pin GPIO_PIN_3
#define R1_GPIO_Port GPIOA
#define B2_Pin GPIO_PIN_4
#define B2_GPIO_Port GPIOA
#define G2_Pin GPIO_PIN_5
#define G2_GPIO_Port GPIOA
#define R2_Pin GPIO_PIN_6
#define R2_GPIO_Port GPIOA
#define R3_Pin GPIO_PIN_7
#define R3_GPIO_Port GPIOA
#define LED_0_Pin GPIO_PIN_0
#define LED_0_GPIO_Port GPIOB
#define LED_1_Pin GPIO_PIN_1
#define LED_1_GPIO_Port GPIOB
#define G3_Pin GPIO_PIN_8
#define G3_GPIO_Port GPIOA
#define B3_Pin GPIO_PIN_15
#define B3_GPIO_Port GPIOA
#define LED_2_Pin GPIO_PIN_6
#define LED_2_GPIO_Port GPIOB
#define LED_3_Pin GPIO_PIN_7
#define LED_3_GPIO_Port GPIOB

    /* USER CODE BEGIN Private defines */

    /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
