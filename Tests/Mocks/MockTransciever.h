#pragma once

#include "Transciever.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Test helper functions */
bool MockTransciever_WasTxCalReqCalled(void);
void MockTransciever_GetLastCalReq(DiveCANType_t* deviceType, DiveCANType_t* targetDeviceType, FO2_t* FO2, uint16_t* atmosphericPressure);
void MockTransciever_Reset(void);

#ifdef __cplusplus
}
#endif
