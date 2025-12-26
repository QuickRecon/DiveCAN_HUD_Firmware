#pragma once

/* Mock cmsis_os.h for testing */
/* Provides minimal RTOS type definitions needed for compilation */

#include <stdint.h>

/* Minimal FreeRTOS types that common.h uses */
typedef uint32_t TickType_t;

/* Priority enumeration */
typedef enum
{
    osPriorityNone = 0,
    osPriorityIdle = 1,
    osPriorityLow = 8,
    osPriorityLow1 = 9,
    osPriorityLow2 = 10,
    osPriorityLow3 = 11,
    osPriorityLow4 = 12,
    osPriorityLow5 = 13,
    osPriorityLow6 = 14,
    osPriorityLow7 = 15,
    osPriorityBelowNormal = 16,
    osPriorityBelowNormal1 = 17,
    osPriorityBelowNormal2 = 18,
    osPriorityBelowNormal3 = 19,
    osPriorityBelowNormal4 = 20,
    osPriorityBelowNormal5 = 21,
    osPriorityBelowNormal6 = 22,
    osPriorityBelowNormal7 = 23,
    osPriorityNormal = 24,
    osPriorityNormal1 = 25,
    osPriorityNormal2 = 26,
    osPriorityNormal3 = 27,
    osPriorityNormal4 = 28,
    osPriorityNormal5 = 29,
    osPriorityNormal6 = 30,
    osPriorityNormal7 = 31,
    osPriorityAboveNormal = 32,
    osPriorityAboveNormal1 = 33,
    osPriorityAboveNormal2 = 34,
    osPriorityAboveNormal3 = 35,
    osPriorityAboveNormal4 = 36,
    osPriorityAboveNormal5 = 37,
    osPriorityAboveNormal6 = 38,
    osPriorityAboveNormal7 = 39,
    osPriorityHigh = 40,
    osPriorityHigh1 = 41,
    osPriorityHigh2 = 42,
    osPriorityHigh3 = 43,
    osPriorityHigh4 = 44,
    osPriorityHigh5 = 45,
    osPriorityHigh6 = 46,
    osPriorityHigh7 = 47,
    osPriorityRealtime = 48,
    osPriorityRealtime1 = 49,
    osPriorityRealtime2 = 50,
    osPriorityRealtime3 = 51,
    osPriorityRealtime4 = 52,
    osPriorityRealtime5 = 53,
    osPriorityRealtime6 = 54,
    osPriorityRealtime7 = 55,
    osPriorityISR = 56,
} osPriority_t;

/* pdMS_TO_TICKS macro - in test environment, just pass through the value */
#define pdMS_TO_TICKS(x) (x)

/* FreeRTOS/CMSIS-RTOS v2 types for queue operations */
typedef long BaseType_t;
typedef void *osMessageQueueId_t;
typedef void *osThreadId_t;
typedef void *StaticTask_t;

/* FreeRTOS constants */
#define pdTRUE ((BaseType_t)1)
#define pdFALSE ((BaseType_t)0)

typedef enum
{
    osOK = 0,
    osError = -1,
    osErrorTimeout = -2,
    osErrorResource = -3
} osStatus_t;

/* Thread attributes */
#define osThreadDetached 0x00000000U

typedef struct
{
    const char *name;
    uint32_t attr_bits;
    void *cb_mem;
    uint32_t cb_size;
    void *stack_mem;
    uint32_t stack_size;
    osPriority_t priority;
    uint32_t tz_module;
    uint32_t reserved;
} osThreadAttr_t;

/* Function declarations - implemented in MockQueue.cpp */
#ifdef __cplusplus
extern "C"
{
#endif

    osStatus_t osMessageQueuePut(osMessageQueueId_t queue_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout);
    osStatus_t osMessageQueueGet(osMessageQueueId_t queue_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout);
    osStatus_t osMessageQueueReset(osMessageQueueId_t queue_id);
    void osDelay(TickType_t ticks);

    /* Thread management */
    typedef void (*osThreadFunc_t)(void *argument);
    osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr);

#ifdef __cplusplus
}
#endif
