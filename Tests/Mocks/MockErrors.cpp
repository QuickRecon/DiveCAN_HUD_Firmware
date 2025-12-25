#include "MockErrors.h"
#include <map>
#include <cstring>

/* Mock state tracking */
static std::map<NonFatalError_t, uint32_t> nonFatalErrorCounts;
static std::map<NonFatalError_t, uint32_t> nonFatalISRErrorCounts;
static bool fatalErrorOccurred = false;
static FatalError_t lastFatalError = NONE_FERR;
static NonFatalError_t lastNonFatalError = NONE_ERR;
static uint32_t lastNonFatalDetail = 0;
static bool lastNonFatalHasDetail = false;
static NonFatalError_t lastNonFatalISRError = NONE_ERR;
static uint32_t lastNonFatalISRDetail = 0;
static bool lastNonFatalISRHasDetail = false;
static uint32_t lastErrorLine = 0;
static char lastErrorFile[256] = {0};

extern "C" {

void NonFatalError_Detail(NonFatalError_t error, uint32_t additionalInfo, uint32_t lineNumber, const char *fileName) {
    nonFatalErrorCounts[error]++;
    lastNonFatalError = error;
    lastNonFatalDetail = additionalInfo;
    lastNonFatalHasDetail = true;
    lastErrorLine = lineNumber;
    if (fileName != nullptr) {
        strncpy(lastErrorFile, fileName, sizeof(lastErrorFile) - 1);
        lastErrorFile[sizeof(lastErrorFile) - 1] = '\0';
    }
}

void NonFatalErrorISR_Detail(NonFatalError_t error, uint32_t additionalInfo, uint32_t lineNumber, const char *fileName) {
    nonFatalISRErrorCounts[error]++;
    lastNonFatalISRError = error;
    lastNonFatalISRDetail = additionalInfo;
    lastNonFatalISRHasDetail = true;
    lastErrorLine = lineNumber;
    if (fileName != nullptr) {
        strncpy(lastErrorFile, fileName, sizeof(lastErrorFile) - 1);
        lastErrorFile[sizeof(lastErrorFile) - 1] = '\0';
    }
}

void NonFatalError(NonFatalError_t error, uint32_t lineNumber, const char *fileName) {
    nonFatalErrorCounts[error]++;
    lastNonFatalError = error;
    lastNonFatalHasDetail = false;
    lastErrorLine = lineNumber;
    if (fileName != nullptr) {
        strncpy(lastErrorFile, fileName, sizeof(lastErrorFile) - 1);
        lastErrorFile[sizeof(lastErrorFile) - 1] = '\0';
    }
}

void NonFatalErrorISR(NonFatalError_t error, uint32_t lineNumber, const char *fileName) {
    nonFatalISRErrorCounts[error]++;
    lastNonFatalISRError = error;
    lastNonFatalISRHasDetail = false;
    lastErrorLine = lineNumber;
    if (fileName != nullptr) {
        strncpy(lastErrorFile, fileName, sizeof(lastErrorFile) - 1);
        lastErrorFile[sizeof(lastErrorFile) - 1] = '\0';
    }
}

void FatalError(FatalError_t error, uint32_t lineNumber, const char *fileName) {
    fatalErrorOccurred = true;
    lastFatalError = error;
    lastErrorLine = lineNumber;
    if (fileName != nullptr) {
        strncpy(lastErrorFile, fileName, sizeof(lastErrorFile) - 1);
        lastErrorFile[sizeof(lastErrorFile) - 1] = '\0';
    }
}

void MockErrors_Reset(void) {
    nonFatalErrorCounts.clear();
    nonFatalISRErrorCounts.clear();
    fatalErrorOccurred = false;
    lastFatalError = NONE_FERR;
    lastNonFatalError = NONE_ERR;
    lastNonFatalDetail = 0;
    lastNonFatalHasDetail = false;
    lastNonFatalISRError = NONE_ERR;
    lastNonFatalISRDetail = 0;
    lastNonFatalISRHasDetail = false;
    lastErrorLine = 0;
    memset(lastErrorFile, 0, sizeof(lastErrorFile));
}

uint32_t MockErrors_GetNonFatalCount(NonFatalError_t error) {
    auto it = nonFatalErrorCounts.find(error);
    if (it != nonFatalErrorCounts.end()) {
        return it->second;
    }
    return 0;
}

uint32_t MockErrors_GetNonFatalISRCount(NonFatalError_t error) {
    auto it = nonFatalISRErrorCounts.find(error);
    if (it != nonFatalISRErrorCounts.end()) {
        return it->second;
    }
    return 0;
}

uint32_t MockErrors_GetTotalNonFatalCount(void) {
    uint32_t total = 0;
    for (const auto& pair : nonFatalErrorCounts) {
        total += pair.second;
    }
    return total;
}

uint32_t MockErrors_GetTotalNonFatalISRCount(void) {
    uint32_t total = 0;
    for (const auto& pair : nonFatalISRErrorCounts) {
        total += pair.second;
    }
    return total;
}

bool MockErrors_GetLastNonFatalDetail(NonFatalError_t *error, uint32_t *detail) {
    if (error != nullptr) {
        *error = lastNonFatalError;
    }
    if (detail != nullptr && lastNonFatalHasDetail) {
        *detail = lastNonFatalDetail;
    }
    return lastNonFatalHasDetail;
}

bool MockErrors_GetLastNonFatalISRDetail(NonFatalError_t *error, uint32_t *detail) {
    if (error != nullptr) {
        *error = lastNonFatalISRError;
    }
    if (detail != nullptr && lastNonFatalISRHasDetail) {
        *detail = lastNonFatalISRDetail;
    }
    return lastNonFatalISRHasDetail;
}

bool MockErrors_FatalErrorOccurred(FatalError_t *error) {
    if (error != nullptr && fatalErrorOccurred) {
        *error = lastFatalError;
    }
    return fatalErrorOccurred;
}

uint32_t MockErrors_GetLastErrorLine(void) {
    return lastErrorLine;
}

const char* MockErrors_GetLastErrorFile(void) {
    return lastErrorFile;
}

} /* extern "C" */
