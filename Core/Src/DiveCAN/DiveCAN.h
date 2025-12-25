#pragma once

#include "../common.h"
#include "Transciever.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @struct DiveCANDevice
     * @brief Contains information about a DiveCAN device.
     *
     * A `DiveCANDevice_t` struct contains information about a device that uses the DiveCAN protocol.
     */

#define MAX_NAME_SIZE 9
    typedef struct
    {
        const char *name;
        DiveCANType_t type;
        DiveCANManufacturer_t manufacturerID;
        uint8_t firmwareVersion;
    } DiveCANDevice_t;

    typedef struct
    {
        int16_t C1;
        int16_t C2;
        int16_t C3;
    } CellValues_t;

    void InitDiveCAN(const DiveCANDevice_t *const deviceSpec);

#ifdef TESTING
    /* Exposed for testing */
    void RespPing(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
    void RespShutdown(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
    void RespSerialNumber(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
    void RespPPO2(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
    void RespPPO2Status(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
#endif

#ifdef __cplusplus
}
#endif
