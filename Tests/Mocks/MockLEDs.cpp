#include "MockLEDs.h"

/* Tracking structures for LED function calls */
static uint32_t setRGBCallCount = 0;
static struct {
    uint8_t channel;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} lastSetRGB;

static uint32_t blinkCodeCallCount = 0;
static struct {
    int8_t c1;
    int8_t c2;
    int8_t c3;
    uint8_t statusMask;
    uint8_t failMask;
} lastBlinkCode;

static uint32_t blinkNoDataCallCount = 0;
static uint32_t blinkAlarmCallCount = 0;

static bool menuActiveState = false;

extern "C" {

void MockLEDs_Reset(void) {
    setRGBCallCount = 0;
    lastSetRGB.channel = 0;
    lastSetRGB.r = 0;
    lastSetRGB.g = 0;
    lastSetRGB.b = 0;

    blinkCodeCallCount = 0;
    lastBlinkCode.c1 = 0;
    lastBlinkCode.c2 = 0;
    lastBlinkCode.c3 = 0;
    lastBlinkCode.statusMask = 0;
    lastBlinkCode.failMask = 0;

    blinkNoDataCallCount = 0;
    blinkAlarmCallCount = 0;
    menuActiveState = false;
}

void setRGB(uint8_t channel, uint8_t r, uint8_t g, uint8_t b) {
    setRGBCallCount++;
    lastSetRGB.channel = channel;
    lastSetRGB.r = r;
    lastSetRGB.g = g;
    lastSetRGB.b = b;
}

void blinkCode(int8_t c1, int8_t c2, int8_t c3, uint8_t statusMask, uint8_t failMask) {
    blinkCodeCallCount++;
    lastBlinkCode.c1 = c1;
    lastBlinkCode.c2 = c2;
    lastBlinkCode.c3 = c3;
    lastBlinkCode.statusMask = statusMask;
    lastBlinkCode.failMask = failMask;
}

void blinkNoData(void) {
    blinkNoDataCallCount++;
}

void blinkAlarm(void) {
    blinkAlarmCallCount++;
}

bool menuActive(void) {
    return menuActiveState;
}

uint32_t MockLEDs_GetSetRGBCallCount(void) {
    return setRGBCallCount;
}

void MockLEDs_GetLastSetRGB(uint8_t *channel, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (channel) *channel = lastSetRGB.channel;
    if (r) *r = lastSetRGB.r;
    if (g) *g = lastSetRGB.g;
    if (b) *b = lastSetRGB.b;
}

uint32_t MockLEDs_GetBlinkCodeCallCount(void) {
    return blinkCodeCallCount;
}

void MockLEDs_GetLastBlinkCode(int8_t *c1, int8_t *c2, int8_t *c3, uint8_t *statusMask, uint8_t *failMask) {
    if (c1) *c1 = lastBlinkCode.c1;
    if (c2) *c2 = lastBlinkCode.c2;
    if (c3) *c3 = lastBlinkCode.c3;
    if (statusMask) *statusMask = lastBlinkCode.statusMask;
    if (failMask) *failMask = lastBlinkCode.failMask;
}

uint32_t MockLEDs_GetBlinkNoDataCallCount(void) {
    return blinkNoDataCallCount;
}

uint32_t MockLEDs_GetBlinkAlarmCallCount(void) {
    return blinkAlarmCallCount;
}

void MockLEDs_SetMenuActive(bool active) {
    menuActiveState = active;
}

} /* extern "C" */
