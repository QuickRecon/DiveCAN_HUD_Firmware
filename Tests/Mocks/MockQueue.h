#pragma once

#include <stdint.h>
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Types and function declarations are in cmsis_os.h */
    /* This header just provides the test helper functions */

    /* Mock queue handles - these will be initialized in the mock implementation */
    extern osMessageQueueId_t PPO2QueueHandle;
    extern osMessageQueueId_t CellStatQueueHandle;

    /* Test helper functions */
    void MockQueue_Init(void);
    void MockQueue_Reset(void);
    void MockQueue_Cleanup(void);
    uint32_t MockQueue_GetDelayCallCount(void);
    uint32_t MockQueue_GetTotalDelayTicks(void);

#ifdef __cplusplus
}
#endif
