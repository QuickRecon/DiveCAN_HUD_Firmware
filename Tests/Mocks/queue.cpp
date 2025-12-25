#include "queue.h"
#include <queue>
#include <map>
#include <cstring>
#include <cstdlib>

/* Queue structure for mocking */
struct MockFreeRTOSQueue {
    std::queue<uint8_t*> messages;
    uint32_t itemSize;
    uint32_t maxLength;

    MockFreeRTOSQueue(uint32_t length, uint32_t size)
        : itemSize(size), maxLength(length) {}

    ~MockFreeRTOSQueue() {
        while (!messages.empty()) {
            free(messages.front());  /* Use free() instead of delete[] */
            messages.pop();
        }
    }
};

/* Storage for created queues */
static std::map<QueueHandle_t, MockFreeRTOSQueue*> queues;
static uint32_t nextQueueHandle = 1;

/* Mock behavior control */
static BaseType_t receiveReturnValue = pdPASS;
static uint32_t receiveDelayTicks = 0;
static BaseType_t isrReturnValue = pdPASS;

/* osDelay tracking */
static uint32_t delayCallCount = 0;
static uint32_t totalDelayTicks = 0;

/* Application-specific queue handles */
QueueHandle_t PPO2QueueHandle = nullptr;
QueueHandle_t CellStatQueueHandle = nullptr;

extern "C" {

QueueHandle_t xQueueCreateStatic(uint32_t uxQueueLength,
                                  uint32_t uxItemSize,
                                  uint8_t *pucQueueStorageBuffer,
                                  StaticQueue_t *pxQueueBuffer) {
    (void)pucQueueStorageBuffer;
    (void)pxQueueBuffer;

    MockFreeRTOSQueue *queue = new MockFreeRTOSQueue(uxQueueLength, uxItemSize);
    QueueHandle_t handle = (QueueHandle_t)(uintptr_t)nextQueueHandle++;
    queues[handle] = queue;

    return handle;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) {
    (void)xTicksToWait;

    if (receiveDelayTicks > 0) {
        osDelay(receiveDelayTicks);
    }

    if (receiveReturnValue != pdPASS) {
        return receiveReturnValue;
    }

    auto it = queues.find(xQueue);
    if (it == queues.end() || pvBuffer == nullptr) {
        return pdFAIL;
    }

    MockFreeRTOSQueue *queue = it->second;
    if (queue->messages.empty()) {
        return pdFAIL;
    }

    /* Copy data and remove from queue */
    uint8_t *data = queue->messages.front();
    memcpy(pvBuffer, data, queue->itemSize);
    free(data);  /* Use free() instead of delete[] */
    queue->messages.pop();

    return pdPASS;
}

BaseType_t xQueuePeek(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) {
    (void)xTicksToWait;

    if (receiveReturnValue != pdPASS) {
        return receiveReturnValue;
    }

    auto it = queues.find(xQueue);
    if (it == queues.end() || pvBuffer == nullptr) {
        return pdFAIL;
    }

    MockFreeRTOSQueue *queue = it->second;
    if (queue->messages.empty()) {
        return pdFAIL;
    }

    /* Copy data but DON'T remove from queue (peek) */
    uint8_t *data = queue->messages.front();
    memcpy(pvBuffer, data, queue->itemSize);

    return pdPASS;
}

BaseType_t xQueueReset(QueueHandle_t xQueue) {
    auto it = queues.find(xQueue);
    if (it == queues.end()) {
        return pdFAIL;
    }

    MockFreeRTOSQueue *queue = it->second;
    while (!queue->messages.empty()) {
        free(queue->messages.front());  /* Use free() instead of delete[] */
        queue->messages.pop();
    }

    return pdPASS;
}

BaseType_t xQueueOverwriteFromISR(QueueHandle_t xQueue,
                                   const void *pvItemToQueue,
                                   BaseType_t *pxHigherPriorityTaskWoken) {
    (void)pxHigherPriorityTaskWoken;

    if (isrReturnValue != pdPASS) {
        return isrReturnValue;
    }

    auto it = queues.find(xQueue);
    if (it == queues.end() || pvItemToQueue == nullptr) {
        return pdFAIL;
    }

    MockFreeRTOSQueue *queue = it->second;

    /* Overwrite: clear queue first, then add new item */
    while (!queue->messages.empty()) {
        free(queue->messages.front());  /* Use free() instead of delete[] */
        queue->messages.pop();
    }

    /* Add new message - use malloc() instead of new */
    uint8_t *data = (uint8_t*)malloc(queue->itemSize);
    memcpy(data, pvItemToQueue, queue->itemSize);
    queue->messages.push(data);

    return pdPASS;
}

BaseType_t xQueueSendToBackFromISR(QueueHandle_t xQueue,
                                    const void *pvItemToQueue,
                                    BaseType_t *pxHigherPriorityTaskWoken) {
    (void)pxHigherPriorityTaskWoken;

    if (isrReturnValue != pdPASS) {
        return isrReturnValue;
    }

    auto it = queues.find(xQueue);
    if (it == queues.end() || pvItemToQueue == nullptr) {
        return pdFAIL;
    }

    MockFreeRTOSQueue *queue = it->second;

    /* Check if queue is full */
    if (queue->messages.size() >= queue->maxLength) {
        return pdFAIL;
    }

    /* Add message to back of queue - use malloc() instead of new */
    uint8_t *data = (uint8_t*)malloc(queue->itemSize);
    memcpy(data, pvItemToQueue, queue->itemSize);
    queue->messages.push(data);

    return pdPASS;
}

/* Mock control functions */
void MockQueue_ResetFreeRTOS(void) {
    /* Clean up all queues */
    for (auto &pair : queues) {
        delete pair.second;
    }
    queues.clear();
    nextQueueHandle = 1;

    /* Reset behavior */
    receiveReturnValue = pdPASS;
    receiveDelayTicks = 0;
    isrReturnValue = pdPASS;
    delayCallCount = 0;
    totalDelayTicks = 0;
}

void MockQueue_ClearAllQueues(void) {
    /* Clear messages from all queues without destroying queue handles */
    for (auto &pair : queues) {
        MockFreeRTOSQueue *queue = pair.second;
        /* Free all message data - use free() instead of delete[] */
        while (!queue->messages.empty()) {
            free(queue->messages.front());
            queue->messages.pop();
        }
        /* Force std::queue to release internal std::deque buffers */
        queue->messages = std::queue<uint8_t*>();
    }
}

uint32_t MockQueue_GetMessageCount(QueueHandle_t xQueue) {
    auto it = queues.find(xQueue);
    if (it == queues.end()) {
        return 0;
    }

    return static_cast<uint32_t>(it->second->messages.size());
}

bool MockQueue_GetLastMessage(QueueHandle_t xQueue, void *pvBuffer) {
    auto it = queues.find(xQueue);
    if (it == queues.end() || pvBuffer == nullptr) {
        return false;
    }

    MockFreeRTOSQueue *queue = it->second;
    if (queue->messages.empty()) {
        return false;
    }

    /* Get the last message (back of queue) */
    /* Note: std::queue doesn't provide back() access, so we need to copy all and get last */
    std::queue<uint8_t*> temp = queue->messages;
    uint8_t *lastData = nullptr;
    while (!temp.empty()) {
        lastData = temp.front();
        temp.pop();
    }

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
