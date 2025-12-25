#pragma once

/* Mock wrapper for Transciever.h that exposes private functions for testing */
#include "../../Core/Src/DiveCAN/Transciever.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Expose private sendCANMessage for testing */
#ifdef TESTING
void sendCANMessage(const DiveCANMessage_t message);
#endif

#ifdef __cplusplus
}
#endif
