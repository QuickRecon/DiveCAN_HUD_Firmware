#pragma once

#include "../errors.h"

#ifdef TESTING
/* Include mock HAL for testing */
#include "main.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    void initLEDs(void);

    void setRGB(uint8_t channel, uint8_t r, uint8_t g, uint8_t b);
    void blinkCode(int8_t c1, int8_t c2, int8_t c3, uint8_t statusMask, uint8_t failMask, bool *breakout);
    void blinkNoData(void);
    void blinkAlarm();

#ifdef TESTING
    /* Expose internal function for testing */
    void setLEDBrightness(uint8_t level, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
#endif

#ifdef __cplusplus
}
#endif
