#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* IWDG handle type */
typedef struct {
    void* Instance;
    uint32_t Init;
    uint32_t State;
} IWDG_HandleTypeDef;

/* HAL status type (shared with MockCAN.h) */
#ifndef HAL_STATUSDEF
#define HAL_STATUSDEF
typedef enum {
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;
#endif

/* Mock HAL functions */
void HAL_Delay(uint32_t Delay);
HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef *hiwdg);

/* Interrupt control functions */
void __disable_irq(void);
void __enable_irq(void);

/* Mock control functions */
void MockDelay_Reset(void);
uint32_t MockDelay_GetCallCount(void);
uint32_t MockDelay_GetTotalDelayMs(void);
uint32_t MockDelay_GetIWDGRefreshCount(void);
bool MockDelay_GetInterruptsEnabled(void);

#ifdef __cplusplus
}
#endif
