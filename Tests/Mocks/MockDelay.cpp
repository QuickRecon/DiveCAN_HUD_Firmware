#include "MockDelay.h"

/* Mock state */
static uint32_t delayCallCount = 0;
static uint32_t totalDelayMs = 0;
static uint32_t iwdgRefreshCount = 0;
static bool interruptsEnabled = true;

/* Mock IWDG instance */
static IWDG_HandleTypeDef iwdgInstance = {0};
IWDG_HandleTypeDef hiwdg = iwdgInstance;

extern "C" {

void HAL_Delay(uint32_t Delay) {
    delayCallCount++;
    totalDelayMs += Delay;
}

HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef *hiwdg) {
    (void)hiwdg;
    iwdgRefreshCount++;
    return HAL_OK;
}

void __disable_irq(void) {
    interruptsEnabled = false;
}

void __enable_irq(void) {
    interruptsEnabled = true;
}

/* Mock control functions */
void MockDelay_Reset(void) {
    delayCallCount = 0;
    totalDelayMs = 0;
    iwdgRefreshCount = 0;
    interruptsEnabled = true;
}

uint32_t MockDelay_GetCallCount(void) {
    return delayCallCount;
}

uint32_t MockDelay_GetTotalDelayMs(void) {
    return totalDelayMs;
}

uint32_t MockDelay_GetIWDGRefreshCount(void) {
    return iwdgRefreshCount;
}

bool MockDelay_GetInterruptsEnabled(void) {
    return interruptsEnabled;
}

} /* extern "C" */
