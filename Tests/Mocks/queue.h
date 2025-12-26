#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* FreeRTOS queue types */
    typedef void *QueueHandle_t;
    typedef void *StaticQueue_t;

/* FreeRTOS constants */
#define pdPASS ((BaseType_t)1)
#define pdFAIL ((BaseType_t)0)
    /* pdTRUE and pdFALSE are defined in cmsis_os.h */

    /* FreeRTOS queue API */
    QueueHandle_t xQueueCreateStatic(uint32_t uxQueueLength,
                                     uint32_t uxItemSize,
                                     uint8_t *pucQueueStorageBuffer,
                                     StaticQueue_t *pxQueueBuffer);

    BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
    BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);
    BaseType_t xQueuePeek(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
    BaseType_t xQueueReset(QueueHandle_t xQueue);

    /* ISR-safe variants */
    BaseType_t xQueueOverwriteFromISR(QueueHandle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken);
    BaseType_t xQueueSendToBackFromISR(QueueHandle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken);

    /* Mock control functions */
    void MockQueue_ResetFreeRTOS(void);
    void MockQueue_ClearAllQueues(void); /* Clears messages from all queues without destroying them */
    uint32_t MockQueue_GetMessageCount(QueueHandle_t xQueue);
    bool MockQueue_GetLastMessage(QueueHandle_t xQueue, void *pvBuffer);
    void MockQueue_SetReceiveBehavior(BaseType_t returnValue, uint32_t delayTicks);
    void MockQueue_SetISRBehavior(BaseType_t returnValue);
    uint32_t MockQueue_GetDelayCallCount(void);
    uint32_t MockQueue_GetTotalDelayTicks(void);

    /* Application-specific queue handles (from main.c) */
    extern QueueHandle_t PPO2QueueHandle;
    extern QueueHandle_t CellStatQueueHandle;

    /* Initialize application queues for testing */
    void MockQueue_InitApplicationQueues(void);

#ifdef __cplusplus
}
#endif
