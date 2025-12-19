#pragma once
#include "main.h"
#include "stdbool.h"
#include "../common.h"

#ifdef __cplusplus
extern "C"
{
#endif
    bool testBusActive(void);
    bool getBusStatus(void);
    void Shutdown();

#ifdef __cplusplus
}
#endif
