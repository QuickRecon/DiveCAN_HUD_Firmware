#include "MockQueue.h"
#include <queue>
#include <cstring>

/* Mock implementation using std::queue */
struct MockQueue {
    std::queue<uint8_t*> data;
    size_t item_size;

    MockQueue(size_t size) : item_size(size) {}

    ~MockQueue() {
        while (!data.empty()) {
            delete[] data.front();
            data.pop();
        }
    }
};

/* Queue instances */
static MockQueue* ppo2Queue = nullptr;
static MockQueue* cellStatQueue = nullptr;

/* Delay tracking */
static uint32_t delayCallCount = 0;
static uint32_t totalDelayTicks = 0;

/* Queue handles */
osMessageQueueId_t PPO2QueueHandle = nullptr;
osMessageQueueId_t CellStatQueueHandle = nullptr;

extern "C" {

void MockQueue_Init(void) {
    /* Clean up existing queues if they exist */
    if (ppo2Queue) {
        delete ppo2Queue;
    }
    if (cellStatQueue) {
        delete cellStatQueue;
    }

    /* CellValues_t has 3 int16_t values = 6 bytes */
    ppo2Queue = new MockQueue(6);
    /* Cell status is a single uint8_t */
    cellStatQueue = new MockQueue(1);

    PPO2QueueHandle = (osMessageQueueId_t)ppo2Queue;
    CellStatQueueHandle = (osMessageQueueId_t)cellStatQueue;

    delayCallCount = 0;
    totalDelayTicks = 0;
}

void MockQueue_Reset(void) {
    if (ppo2Queue) {
        while (!ppo2Queue->data.empty()) {
            delete[] ppo2Queue->data.front();
            ppo2Queue->data.pop();
        }
    }
    if (cellStatQueue) {
        while (!cellStatQueue->data.empty()) {
            delete[] cellStatQueue->data.front();
            cellStatQueue->data.pop();
        }
    }

    delayCallCount = 0;
    totalDelayTicks = 0;
}

void MockQueue_Cleanup(void) {
    if (ppo2Queue) {
        delete ppo2Queue;
        ppo2Queue = nullptr;
    }
    if (cellStatQueue) {
        delete cellStatQueue;
        cellStatQueue = nullptr;
    }

    PPO2QueueHandle = nullptr;
    CellStatQueueHandle = nullptr;
}

uint32_t MockQueue_GetDelayCallCount(void) {
    return delayCallCount;
}

uint32_t MockQueue_GetTotalDelayTicks(void) {
    return totalDelayTicks;
}

osStatus_t osMessageQueuePut(osMessageQueueId_t queue_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout) {
    (void)msg_prio;
    (void)timeout;

    if (queue_id == nullptr || msg_ptr == nullptr) {
        return osError;
    }

    MockQueue* queue = (MockQueue*)queue_id;

    /* Allocate and copy data */
    uint8_t* data = new uint8_t[queue->item_size];
    memcpy(data, msg_ptr, queue->item_size);
    queue->data.push(data);

    return osOK;
}

osStatus_t osMessageQueueGet(osMessageQueueId_t queue_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout) {
    (void)msg_prio;
    (void)timeout;

    if (queue_id == nullptr || msg_ptr == nullptr) {
        return osError;
    }

    MockQueue* queue = (MockQueue*)queue_id;

    if (queue->data.empty()) {
        return osErrorResource;
    }

    /* Copy data and remove from queue */
    uint8_t* data = queue->data.front();
    memcpy(msg_ptr, data, queue->item_size);
    delete[] data;
    queue->data.pop();

    return osOK;
}

void osDelay(TickType_t ticks) {
    delayCallCount++;
    totalDelayTicks += ticks;
}

} /* extern "C" */
