#include "queue.h"
#include <cstring>
#include <cstdlib>

/* Simple circular buffer implementation to replace std::queue */
#define MAX_QUEUE_ITEMS 16

struct MessageBuffer {
    uint8_t* items[MAX_QUEUE_ITEMS];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
};

/* Queue structure for mocking */
struct MockFreeRTOSQueue {
    MessageBuffer buffer;
    uint32_t itemSize;
    uint32_t maxLength;
};

/* Storage for created queues using fixed-size array */
#define MAX_QUEUES 8

struct QueueRecord {
    QueueHandle_t handle;
    MockFreeRTOSQueue* queue;
    bool valid;
};

static QueueRecord queues[MAX_QUEUES];
static uint32_t nextQueueHandle = 1;

/* Mock behavior control */
static BaseType_t receiveReturnValue = pdPASS;
static uint32_t receiveDelayTicks = 0;
static BaseType_t sendReturnValue = pdPASS;
static BaseType_t isrReturnValue = pdPASS;

/* osDelay tracking */
static uint32_t delayCallCount = 0;
static uint32_t totalDelayTicks = 0;

/* Application-specific queue handles */
QueueHandle_t PPO2QueueHandle = nullptr;
QueueHandle_t CellStatQueueHandle = nullptr;
QueueHandle_t CalStateQueueHandle = nullptr;

extern "C" {

/* Helper function to find queue by handle */
static MockFreeRTOSQueue* findQueue(QueueHandle_t handle) {
    for (uint32_t i = 0; i < MAX_QUEUES; i++) {
        if (queues[i].valid && queues[i].handle == handle) {
            return queues[i].queue;
        }
    }
    return nullptr;
}

QueueHandle_t xQueueCreateStatic(uint32_t uxQueueLength,
                                  uint32_t uxItemSize,
                                  uint8_t *pucQueueStorageBuffer,
                                  StaticQueue_t *pxQueueBuffer) {
    (void)pucQueueStorageBuffer;
    (void)pxQueueBuffer;

    /* Find empty slot */
    for (uint32_t i = 0; i < MAX_QUEUES; i++) {
        if (!queues[i].valid) {
            MockFreeRTOSQueue *queue = (MockFreeRTOSQueue*)malloc(sizeof(MockFreeRTOSQueue));
            memset(queue, 0, sizeof(MockFreeRTOSQueue));
            queue->itemSize = uxItemSize;
            queue->maxLength = uxQueueLength;

            QueueHandle_t handle = (QueueHandle_t)(uintptr_t)nextQueueHandle++;
            queues[i].handle = handle;
            queues[i].queue = queue;
            queues[i].valid = true;

            return handle;
        }
    }

    return nullptr;  /* No space for more queues */
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) {
    (void)xTicksToWait;

    if (receiveDelayTicks > 0) {
        osDelay(receiveDelayTicks);
    }

    if (receiveReturnValue != pdPASS) {
        return receiveReturnValue;
    }

    MockFreeRTOSQueue *queue = findQueue(xQueue);
    if (queue == nullptr || pvBuffer == nullptr) {
        return pdFAIL;
    }

    /* Check if queue is empty */
    if (queue->buffer.count == 0) {
        return pdFAIL;
    }

    /* Copy data from head and remove from queue */
    uint8_t *data = queue->buffer.items[queue->buffer.head];
    memcpy(pvBuffer, data, queue->itemSize);
    free(data);

    /* Advance head */
    queue->buffer.items[queue->buffer.head] = nullptr;
    queue->buffer.head = (queue->buffer.head + 1) % MAX_QUEUE_ITEMS;
    queue->buffer.count--;

    return pdPASS;
}

BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait) {
    (void)xTicksToWait;

    if (sendReturnValue != pdPASS) {
        return sendReturnValue;
    }

    MockFreeRTOSQueue *queue = findQueue(xQueue);
    if (queue == nullptr || pvItemToQueue == nullptr) {
        return pdFAIL;
    }

    /* Check if queue is full */
    if (queue->buffer.count >= queue->maxLength || queue->buffer.count >= MAX_QUEUE_ITEMS) {
        return pdFAIL;
    }

    /* Add message to back of queue */
    uint8_t *data = (uint8_t*)malloc(queue->itemSize);
    memcpy(data, pvItemToQueue, queue->itemSize);
    queue->buffer.items[queue->buffer.tail] = data;
    queue->buffer.tail = (queue->buffer.tail + 1) % MAX_QUEUE_ITEMS;
    queue->buffer.count++;

    return pdPASS;
}

BaseType_t xQueuePeek(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) {
    (void)xTicksToWait;

    if (receiveReturnValue != pdPASS) {
        return receiveReturnValue;
    }

    MockFreeRTOSQueue *queue = findQueue(xQueue);
    if (queue == nullptr || pvBuffer == nullptr) {
        return pdFAIL;
    }

    /* Check if queue is empty */
    if (queue->buffer.count == 0) {
        return pdFAIL;
    }

    /* Copy data from head but DON'T remove from queue (peek) */
    uint8_t *data = queue->buffer.items[queue->buffer.head];
    memcpy(pvBuffer, data, queue->itemSize);

    return pdPASS;
}

BaseType_t xQueueReset(QueueHandle_t xQueue) {
    MockFreeRTOSQueue *queue = findQueue(xQueue);
    if (queue == nullptr) {
        return pdFAIL;
    }

    /* Free all items in the buffer */
    for (uint32_t i = 0; i < MAX_QUEUE_ITEMS; i++) {
        if (queue->buffer.items[i] != nullptr) {
            free(queue->buffer.items[i]);
            queue->buffer.items[i] = nullptr;
        }
    }

    /* Reset buffer state */
    queue->buffer.head = 0;
    queue->buffer.tail = 0;
    queue->buffer.count = 0;

    return pdPASS;
}

BaseType_t xQueueOverwriteFromISR(QueueHandle_t xQueue,
                                   const void *pvItemToQueue,
                                   BaseType_t *pxHigherPriorityTaskWoken) {
    (void)pxHigherPriorityTaskWoken;

    if (isrReturnValue != pdPASS) {
        return isrReturnValue;
    }

    MockFreeRTOSQueue *queue = findQueue(xQueue);
    if (queue == nullptr || pvItemToQueue == nullptr) {
        return pdFAIL;
    }

    /* Overwrite: clear queue first */
    for (uint32_t i = 0; i < MAX_QUEUE_ITEMS; i++) {
        if (queue->buffer.items[i] != nullptr) {
            free(queue->buffer.items[i]);
            queue->buffer.items[i] = nullptr;
        }
    }
    queue->buffer.head = 0;
    queue->buffer.tail = 0;
    queue->buffer.count = 0;

    /* Add new message */
    uint8_t *data = (uint8_t*)malloc(queue->itemSize);
    memcpy(data, pvItemToQueue, queue->itemSize);
    queue->buffer.items[queue->buffer.tail] = data;
    queue->buffer.tail = (queue->buffer.tail + 1) % MAX_QUEUE_ITEMS;
    queue->buffer.count = 1;

    return pdPASS;
}

BaseType_t xQueueSendToBackFromISR(QueueHandle_t xQueue,
                                    const void *pvItemToQueue,
                                    BaseType_t *pxHigherPriorityTaskWoken) {
    (void)pxHigherPriorityTaskWoken;

    if (isrReturnValue != pdPASS) {
        return isrReturnValue;
    }

    MockFreeRTOSQueue *queue = findQueue(xQueue);
    if (queue == nullptr || pvItemToQueue == nullptr) {
        return pdFAIL;
    }

    /* Check if queue is full */
    if (queue->buffer.count >= queue->maxLength || queue->buffer.count >= MAX_QUEUE_ITEMS) {
        return pdFAIL;
    }

    /* Add message to back of queue */
    uint8_t *data = (uint8_t*)malloc(queue->itemSize);
    memcpy(data, pvItemToQueue, queue->itemSize);
    queue->buffer.items[queue->buffer.tail] = data;
    queue->buffer.tail = (queue->buffer.tail + 1) % MAX_QUEUE_ITEMS;
    queue->buffer.count++;

    return pdPASS;
}

/* Mock control functions */
void MockQueue_ResetFreeRTOS(void) {
    /* Clean up all queues */
    for (uint32_t i = 0; i < MAX_QUEUES; i++) {
        if (queues[i].valid && queues[i].queue != nullptr) {
            /* Free all items in the queue buffer */
            for (uint32_t j = 0; j < MAX_QUEUE_ITEMS; j++) {
                if (queues[i].queue->buffer.items[j] != nullptr) {
                    free(queues[i].queue->buffer.items[j]);
                }
            }
            /* Free the queue itself */
            free(queues[i].queue);
        }
    }
    memset(queues, 0, sizeof(queues));
    nextQueueHandle = 1;

    /* Reset application queue handles */
    PPO2QueueHandle = nullptr;
    CellStatQueueHandle = nullptr;

    /* Reset behavior */
    receiveReturnValue = pdPASS;
    receiveDelayTicks = 0;
    sendReturnValue = pdPASS;
    isrReturnValue = pdPASS;
    delayCallCount = 0;
    totalDelayTicks = 0;
}

void MockQueue_ClearAllQueues(void) {
    /* Clear messages from all queues without destroying queue handles */
    for (uint32_t i = 0; i < MAX_QUEUES; i++) {
        if (queues[i].valid && queues[i].queue != nullptr) {
            MockFreeRTOSQueue *queue = queues[i].queue;
            /* Free all message data */
            for (uint32_t j = 0; j < MAX_QUEUE_ITEMS; j++) {
                if (queue->buffer.items[j] != nullptr) {
                    free(queue->buffer.items[j]);
                    queue->buffer.items[j] = nullptr;
                }
            }
            /* Reset buffer state */
            queue->buffer.head = 0;
            queue->buffer.tail = 0;
            queue->buffer.count = 0;
        }
    }
}

uint32_t MockQueue_GetMessageCount(QueueHandle_t xQueue) {
    MockFreeRTOSQueue *queue = findQueue(xQueue);
    if (queue == nullptr) {
        return 0;
    }

    return queue->buffer.count;
}

bool MockQueue_GetLastMessage(QueueHandle_t xQueue, void *pvBuffer) {
    MockFreeRTOSQueue *queue = findQueue(xQueue);
    if (queue == nullptr || pvBuffer == nullptr) {
        return false;
    }

    if (queue->buffer.count == 0) {
        return false;
    }

    /* Get the last message (most recently added, which is tail - 1) */
    uint32_t lastIndex = (queue->buffer.tail == 0) ? (MAX_QUEUE_ITEMS - 1) : (queue->buffer.tail - 1);
    uint8_t *lastData = queue->buffer.items[lastIndex];

    if (lastData != nullptr) {
        memcpy(pvBuffer, lastData, queue->itemSize);
        return true;
    }

    return false;
}

void MockQueue_SetReceiveBehavior(BaseType_t returnValue, uint32_t delayTicks) {
    receiveReturnValue = returnValue;
    receiveDelayTicks = delayTicks;
}

void MockQueue_SetISRBehavior(BaseType_t returnValue) {
    isrReturnValue = returnValue;
}

uint32_t MockQueue_GetDelayCallCount(void) {
    return delayCallCount;
}

uint32_t MockQueue_GetTotalDelayTicks(void) {
    return totalDelayTicks;
}

/* osDelay implementation */
void osDelay(TickType_t ticks) {
    delayCallCount++;
    totalDelayTicks += ticks;
}

/* Initialize application-specific queues for testing */
void MockQueue_InitApplicationQueues(void) {
    /* Create PPO2 queue (length 1, item size for CellValues_t which is 3 int16_t = 6 bytes) */
    static uint8_t ppo2QueueStorage[1 * 8];  /* Use 8 bytes to be safe */
    static StaticQueue_t ppo2QueueBuffer;
    if (PPO2QueueHandle == nullptr) {
        PPO2QueueHandle = xQueueCreateStatic(1, 8, ppo2QueueStorage, &ppo2QueueBuffer);
    }

    /* Create CellStat queue (length 1, item size uint8_t = 1 byte) */
    static uint8_t cellStatQueueStorage[1 * 4];  /* Use 4 bytes to be safe */
    static StaticQueue_t cellStatQueueBuffer;
    if (CellStatQueueHandle == nullptr) {
        CellStatQueueHandle = xQueueCreateStatic(1, 4, cellStatQueueStorage, &cellStatQueueBuffer);
    }
}

/* CMSIS-RTOS v2 wrappers */
osStatus_t osMessageQueuePut(osMessageQueueId_t queue_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout) {
    (void)msg_prio;
    (void)timeout;

    if (queue_id == nullptr || msg_ptr == nullptr) {
        return osError;
    }

    /* Use ISR-safe send to back */
    BaseType_t result = xQueueSendToBackFromISR((QueueHandle_t)queue_id, msg_ptr, nullptr);
    return (result == pdPASS) ? osOK : osError;
}

osStatus_t osMessageQueueGet(osMessageQueueId_t queue_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout) {
    (void)msg_prio;
    (void)timeout;

    if (queue_id == nullptr || msg_ptr == nullptr) {
        return osError;
    }

    BaseType_t result = xQueueReceive((QueueHandle_t)queue_id, msg_ptr, 0);
    return (result == pdPASS) ? osOK : osErrorTimeout;
}

osStatus_t osMessageQueueReset(osMessageQueueId_t queue_id) {
    if (queue_id == nullptr) {
        return osError;
    }

    BaseType_t result = xQueueReset((QueueHandle_t)queue_id);
    return (result == pdPASS) ? osOK : osError;
}

osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr) {
    /* For testing, we don't actually create threads, just return a dummy handle */
    (void)func;
    (void)argument;
    (void)attr;

    static uint32_t dummyThreadHandle = 0x12345678;
    return (osThreadId_t)&dummyThreadHandle;
}

} /* extern "C" */
