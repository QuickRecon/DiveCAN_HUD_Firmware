#include "MockFlash.h"
#include <map>
#include <cstring>

/* Mock state */
static HAL_StatusTypeDef unlockStatus = HAL_OK;
static HAL_StatusTypeDef lockStatus = HAL_OK;
static HAL_StatusTypeDef obProgramStatus = HAL_OK;
static EE_Status initStatus = EE_OK;
static EE_Status formatStatus = EE_OK;
static EE_Status writeStatus = EE_OK;
static EE_Status readStatus = EE_OK;
static uint32_t readValue = 0;
static EE_Status cleanupStatus = EE_OK;

/* Call counters */
static uint32_t unlockCallCount = 0;
static uint32_t lockCallCount = 0;
static uint32_t obProgramCallCount = 0;
static uint32_t initCallCount = 0;
static uint32_t formatCallCount = 0;
static uint32_t writeCallCount = 0;
static uint32_t readCallCount = 0;
static uint32_t cleanupCallCount = 0;

/* Last operation parameters */
static uint16_t lastWriteAddr = 0;
static uint32_t lastWriteData = 0;
static bool writeOccurred = false;
static uint16_t lastReadAddr = 0;
static bool readOccurred = false;

/* Option bytes storage */
static FLASH_OBProgramInitTypeDef lastOBConfig = {0};
static bool obConfigSet = false;
static FLASH_OBProgramInitTypeDef currentOBConfig = {0};

/* Simulated EEPROM storage */
static std::map<uint16_t, uint32_t> eepromStorage;

extern "C" {

/* HAL Flash Functions */
HAL_StatusTypeDef HAL_FLASH_Unlock(void) {
    unlockCallCount++;
    return unlockStatus;
}

HAL_StatusTypeDef HAL_FLASH_Lock(void) {
    lockCallCount++;
    return lockStatus;
}

HAL_StatusTypeDef HAL_FLASHEx_OBProgram(FLASH_OBProgramInitTypeDef *pOBInit) {
    obProgramCallCount++;
    if (pOBInit != nullptr) {
        lastOBConfig = *pOBInit;
        obConfigSet = true;
        /* Also update current config if programming succeeds */
        if (obProgramStatus == HAL_OK) {
            currentOBConfig.USERConfig = pOBInit->USERConfig;
            currentOBConfig.USERType = pOBInit->USERType;
        }
    }
    return obProgramStatus;
}

void HAL_FLASHEx_OBGetConfig(FLASH_OBProgramInitTypeDef *pOBInit) {
    if (pOBInit != nullptr) {
        *pOBInit = currentOBConfig;
    }
}

/* EEPROM Emulation Functions */
EE_Status EE_Init(EE_Erase_type EraseType) {
    (void)EraseType;
    initCallCount++;
    return initStatus;
}

EE_Status EE_Format(EE_Erase_type EraseType) {
    (void)EraseType;
    formatCallCount++;
    /* Format clears storage on success */
    if (formatStatus == EE_OK) {
        eepromStorage.clear();
    }
    return formatStatus;
}

EE_Status EE_WriteVariable32bits(uint16_t VirtAddress, uint32_t Data) {
    writeCallCount++;
    lastWriteAddr = VirtAddress;
    lastWriteData = Data;
    writeOccurred = true;

    /* Store data on successful write */
    if (writeStatus == EE_OK || writeStatus == EE_CLEANUP_REQUIRED) {
        eepromStorage[VirtAddress] = Data;
    }

    return writeStatus;
}

EE_Status EE_ReadVariable32bits(uint16_t VirtAddress, uint32_t *Data) {
    readCallCount++;
    lastReadAddr = VirtAddress;
    readOccurred = true;

    if (Data != nullptr) {
        /* Check if address exists in storage */
        auto it = eepromStorage.find(VirtAddress);
        if (it != eepromStorage.end()) {
            *Data = it->second;
            /* Return configured status, but typically EE_OK if data exists */
            return (readStatus == EE_NO_DATA) ? EE_OK : readStatus;
        } else {
            /* No data at this address */
            *Data = readValue; /* Use configured default value */
            return EE_NO_DATA;
        }
    }

    return readStatus;
}

EE_Status EE_CleanUp(void) {
    cleanupCallCount++;
    return cleanupStatus;
}

/* Mock control functions */
void MockFlash_Reset(void) {
    unlockStatus = HAL_OK;
    lockStatus = HAL_OK;
    obProgramStatus = HAL_OK;
    initStatus = EE_OK;
    formatStatus = EE_OK;
    writeStatus = EE_OK;
    readStatus = EE_OK;
    readValue = 0;
    cleanupStatus = EE_OK;

    unlockCallCount = 0;
    lockCallCount = 0;
    obProgramCallCount = 0;
    initCallCount = 0;
    formatCallCount = 0;
    writeCallCount = 0;
    readCallCount = 0;
    cleanupCallCount = 0;

    lastWriteAddr = 0;
    lastWriteData = 0;
    writeOccurred = false;
    lastReadAddr = 0;
    readOccurred = false;

    std::memset(&lastOBConfig, 0, sizeof(lastOBConfig));
    obConfigSet = false;
    std::memset(&currentOBConfig, 0, sizeof(currentOBConfig));

    eepromStorage.clear();
}

void MockFlash_SetUnlockBehavior(HAL_StatusTypeDef status) {
    unlockStatus = status;
}

void MockFlash_SetLockBehavior(HAL_StatusTypeDef status) {
    lockStatus = status;
}

void MockFlash_SetOBProgramBehavior(HAL_StatusTypeDef status) {
    obProgramStatus = status;
}

void MockFlash_SetInitBehavior(EE_Status status) {
    initStatus = status;
}

void MockFlash_SetFormatBehavior(EE_Status status) {
    formatStatus = status;
}

void MockFlash_SetWriteBehavior(EE_Status status) {
    writeStatus = status;
}

void MockFlash_SetReadBehavior(EE_Status status, uint32_t value) {
    readStatus = status;
    readValue = value;
}

void MockFlash_SetCleanupBehavior(EE_Status status) {
    cleanupStatus = status;
}

/* Mock query functions */
uint32_t MockFlash_GetUnlockCallCount(void) {
    return unlockCallCount;
}

uint32_t MockFlash_GetLockCallCount(void) {
    return lockCallCount;
}

uint32_t MockFlash_GetOBProgramCallCount(void) {
    return obProgramCallCount;
}

uint32_t MockFlash_GetInitCallCount(void) {
    return initCallCount;
}

uint32_t MockFlash_GetFormatCallCount(void) {
    return formatCallCount;
}

uint32_t MockFlash_GetWriteCallCount(void) {
    return writeCallCount;
}

uint32_t MockFlash_GetReadCallCount(void) {
    return readCallCount;
}

uint32_t MockFlash_GetCleanupCallCount(void) {
    return cleanupCallCount;
}

bool MockFlash_GetLastWrite(uint16_t *addr, uint32_t *data) {
    if (writeOccurred) {
        if (addr != nullptr) *addr = lastWriteAddr;
        if (data != nullptr) *data = lastWriteData;
    }
    return writeOccurred;
}

bool MockFlash_GetLastRead(uint16_t *addr) {
    if (readOccurred && addr != nullptr) {
        *addr = lastReadAddr;
    }
    return readOccurred;
}

bool MockFlash_GetLastOBConfig(FLASH_OBProgramInitTypeDef *config) {
    if (obConfigSet && config != nullptr) {
        *config = lastOBConfig;
    }
    return obConfigSet;
}

bool MockFlash_VerifyOptionBit(uint32_t bitPosition, bool expectedValue) {
    if (!obConfigSet) {
        return false;
    }

    bool actualValue = (lastOBConfig.USERConfig >> bitPosition) & 0x01;
    return actualValue == expectedValue;
}

void MockFlash_SetStoredValue(uint16_t addr, uint32_t value) {
    eepromStorage[addr] = value;
}

uint32_t MockFlash_GetStoredValue(uint16_t addr) {
    auto it = eepromStorage.find(addr);
    if (it != eepromStorage.end()) {
        return it->second;
    }
    return 0;
}

} /* extern "C" */
