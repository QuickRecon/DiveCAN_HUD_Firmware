#pragma once

#include <stdint.h>
#include "MockHAL.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GPIO mode definitions */
#define GPIO_MODE_INPUT                 0x00000000U
#define GPIO_MODE_OUTPUT_PP             0x00000001U
#define GPIO_MODE_OUTPUT_OD             0x00000011U
#define GPIO_MODE_AF_PP                 0x00000002U
#define GPIO_MODE_AF_OD                 0x00000012U
#define GPIO_MODE_ANALOG                0x00000003U
#define GPIO_MODE_IT_RISING             0x10110000U
#define GPIO_MODE_IT_FALLING            0x10210000U
#define GPIO_MODE_IT_RISING_FALLING     0x10310000U
#define GPIO_MODE_EVT_RISING            0x10120000U
#define GPIO_MODE_EVT_FALLING           0x10220000U
#define GPIO_MODE_EVT_RISING_FALLING    0x10320000U

/* GPIO pull-up/pull-down definitions */
#define GPIO_NOPULL                     0x00000000U
#define GPIO_PULLUP                     0x00000001U
#define GPIO_PULLDOWN                   0x00000002U

/* GPIO speed definitions */
#define GPIO_SPEED_FREQ_LOW             0x00000000U
#define GPIO_SPEED_FREQ_MEDIUM          0x00000001U
#define GPIO_SPEED_FREQ_HIGH            0x00000002U
#define GPIO_SPEED_FREQ_VERY_HIGH       0x00000003U

/* GPIO alternate function definitions (subset) */
#define GPIO_AF0_RTC_50Hz               ((uint8_t)0x00)
#define GPIO_AF1_TIM1                   ((uint8_t)0x01)
#define GPIO_AF2_TIM2                   ((uint8_t)0x02)
#define GPIO_AF7_USART1                 ((uint8_t)0x07)

/* GPIO port definitions (need to match MockHAL) */
extern GPIO_TypeDef* GPIOA;
extern GPIO_TypeDef* GPIOB;
extern GPIO_TypeDef* GPIOC;
extern GPIO_TypeDef* GPIOD;
extern GPIO_TypeDef* GPIOE;
extern GPIO_TypeDef* GPIOF;
extern GPIO_TypeDef* GPIOG;
extern GPIO_TypeDef* GPIOH;

/* GPIO pin definitions (extended from MockHAL) */
#define GPIO_PIN_4  ((uint16_t)0x0010)
#define GPIO_PIN_5  ((uint16_t)0x0020)
#define GPIO_PIN_6  ((uint16_t)0x0040)
#define GPIO_PIN_7  ((uint16_t)0x0080)
#define GPIO_PIN_8  ((uint16_t)0x0100)
#define GPIO_PIN_9  ((uint16_t)0x0200)
#define GPIO_PIN_10 ((uint16_t)0x0400)
#define GPIO_PIN_11 ((uint16_t)0x0800)
#define GPIO_PIN_12 ((uint16_t)0x1000)
#define GPIO_PIN_13 ((uint16_t)0x2000)
#define GPIO_PIN_14 ((uint16_t)0x4000)
#define GPIO_PIN_15 ((uint16_t)0x8000)
#define GPIO_PIN_All ((uint16_t)0xFFFF)

/**
 * @brief GPIO Init structure definition
 */
typedef struct {
    uint32_t Pin;       /*!< Specifies the GPIO pins to be configured.
                             This parameter can be any value of @ref GPIO_pins */

    uint32_t Mode;      /*!< Specifies the operating mode for the selected pins.
                             This parameter can be a value of @ref GPIO_mode */

    uint32_t Pull;      /*!< Specifies the Pull-up or Pull-Down activation for the selected pins.
                             This parameter can be a value of @ref GPIO_pull */

    uint32_t Speed;     /*!< Specifies the speed for the selected pins.
                             This parameter can be a value of @ref GPIO_speed */

    uint32_t Alternate; /*!< Peripheral to be connected to the selected pins
                             This parameter can be a value of @ref GPIOEx_Alternate_function_selection */
} GPIO_InitTypeDef;

#ifdef __cplusplus
}
#endif
