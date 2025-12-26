#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Mock LED functions */
    void setRGB(uint8_t channel, uint8_t r, uint8_t g, uint8_t b);
    void blinkCode(int8_t c1, int8_t c2, int8_t c3, uint8_t statusMask, uint8_t failMask);
    void blinkNoData(void);
    void blinkAlarm(void);

    /* Mock menu state machine function */
    bool menuActive(void);

    /* Test helper functions to verify LED function calls */
    void MockLEDs_Reset(void);

    /* Query functions for verification */
    uint32_t MockLEDs_GetSetRGBCallCount(void);
    void MockLEDs_GetLastSetRGB(uint8_t *channel, uint8_t *r, uint8_t *g, uint8_t *b);

    uint32_t MockLEDs_GetBlinkCodeCallCount(void);
    void MockLEDs_GetLastBlinkCode(int8_t *c1, int8_t *c2, int8_t *c3, uint8_t *statusMask, uint8_t *failMask);

    uint32_t MockLEDs_GetBlinkNoDataCallCount(void);
    uint32_t MockLEDs_GetBlinkAlarmCallCount(void);

    /* Control menu state for testing */
    void MockLEDs_SetMenuActive(bool active);

#ifdef __cplusplus
}
#endif
