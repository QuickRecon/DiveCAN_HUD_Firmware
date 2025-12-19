#pragma once

#include "../errors.h"

#ifdef __cplusplus
extern "C"
{
#endif
    void initFlash(void);
#ifdef TESTING
    void setOptionBytes(void);
    uint32_t set_bit(uint32_t number, uint32_t n, bool x);
#endif
    bool GetFatalError(FatalError_t *err);
    bool SetFatalError(FatalError_t err);

    bool GetNonFatalError(NonFatalError_t err, uint32_t *errCount);
    bool SetNonFatalError(NonFatalError_t err, uint32_t errCount);
#ifdef __cplusplus
}
#endif
