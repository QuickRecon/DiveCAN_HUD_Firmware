#include "Transciever.h"
#include <cstdint>

/* Track txCalReq calls for testing */
static bool txCalReqCalled = false;
static DiveCANType_t lastDeviceType = DIVECAN_CONTROLLER;
static DiveCANType_t lastTargetDeviceType = DIVECAN_CONTROLLER;
static FO2_t lastFO2 = 0;
static uint16_t lastAtmosphericPressure = 0;

extern "C" {

void txCalReq(const DiveCANType_t deviceType, const DiveCANType_t targetDeviceType, const FO2_t FO2, const uint16_t atmosphericPressure) {
    txCalReqCalled = true;
    lastDeviceType = deviceType;
    lastTargetDeviceType = targetDeviceType;
    lastFO2 = FO2;
    lastAtmosphericPressure = atmosphericPressure;
}

/* Test helpers */
bool MockTransciever_WasTxCalReqCalled(void) {
    return txCalReqCalled;
}

void MockTransciever_GetLastCalReq(DiveCANType_t* deviceType, DiveCANType_t* targetDeviceType, FO2_t* FO2, uint16_t* atmosphericPressure) {
    if (deviceType) *deviceType = lastDeviceType;
    if (targetDeviceType) *targetDeviceType = lastTargetDeviceType;
    if (FO2) *FO2 = lastFO2;
    if (atmosphericPressure) *atmosphericPressure = lastAtmosphericPressure;
}

void MockTransciever_Reset(void) {
    txCalReqCalled = false;
    lastDeviceType = DIVECAN_CONTROLLER;
    lastTargetDeviceType = DIVECAN_CONTROLLER;
    lastFO2 = 0;
    lastAtmosphericPressure = 0;
}

} /* extern "C" */
