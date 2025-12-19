#pragma once

#include "../errors.h"

#ifdef __cplusplus
extern "C"
{
#endif
    void initLEDs(void);

    void setRGB(uint8_t channel, uint8_t r, uint8_t g, uint8_t b);

    void blinkCode(int8_t c1, int8_t c2, int8_t c3, uint8_t statusMask);
    void blinkNoData(void);
    void blinkAlarm();

#ifdef __cplusplus
}
#endif
