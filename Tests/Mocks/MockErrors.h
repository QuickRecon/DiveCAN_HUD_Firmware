#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declare error types - actual definitions come from real errors.h */
#ifndef _ERRORS_H_DEFINED
#define _ERRORS_H_DEFINED

typedef enum
{
    NONE_FERR = 0,
    STACK_OVERFLOW_FERR = 1,
    MALLOC_FAIL_FERR = 2,
    HARD_FAULT_FERR = 3,
    NMI_TRIGGERED_FERR = 4,
    MEM_FAULT_FERR = 5,
    BUS_FAULT_FERR = 6,
    USAGE_FAULT_FERR = 7,
    ASSERT_FAIL_FERR = 8,
    BUFFER_OVERRUN_FERR = 9,
    UNDEFINED_STATE_FERR = 10,
    STACK_GUARD_FERR = 11,
    EXIT_TRIGGERED_FERR = 12,
    MAX_FERR = EXIT_TRIGGERED_FERR
} FatalError_t;

typedef enum
{
    NONE_ERR = 0,
    FLASH_LOCK_ERR = 1,
    EEPROM_ERR = 2,
    OUT_OF_DATE_ERR = 3,
    I2C_BUS_ERR = 4,
    UART_ERR = 5,
    UNREACHABLE_ERR = 6,
    FLAG_ERR = 7,
    CRITICAL_ERR = 8,
    TIMEOUT_ERR = 9,
    QUEUEING_ERR = 10,
    CAN_OVERFLOW_ERR = 11,
    CAN_TX_ERR = 12,
    UNDEFINED_CAL_METHOD_ERR = 13,
    CAL_METHOD_ERR = 14,
    CAL_MISMATCH_ERR = 15,
    INVALID_CELL_NUMBER_ERR = 17,
    INVALID_ADC_NUMBER_ERR = 19,
    NULL_PTR_ERR = 20,
    LOGGING_ERR = 21,
    MENU_ERR = 22,
    CONFIG_ERR = 23,
    INT_ADC_ERR = 24,
    UNKNOWN_ERROR_ERR = 25,
    CELL_OVERRANGE_ERR = 26,
    FS_ERR = 27,
    VBUS_UNDER_VOLTAGE_ERR = 28,
    VCC_UNDER_VOLTAGE_ERR = 29,
    SOLENOID_DISABLED_ERR = 30,
    TSC_ERR = 31,
    MAX_ERR = TSC_ERR
} NonFatalError_t;

#endif /* _ERRORS_H_DEFINED */

/* Error reporting functions that will be mocked */
void NonFatalError_Detail(NonFatalError_t error, uint32_t additionalInfo, uint32_t lineNumber, const char *fileName);
void NonFatalErrorISR_Detail(NonFatalError_t error, uint32_t additionalInfo, uint32_t lineNumber, const char *fileName);
void NonFatalError(NonFatalError_t error, uint32_t lineNumber, const char *fileName);
void NonFatalErrorISR(NonFatalError_t error, uint32_t lineNumber, const char *fileName);
void FatalError(FatalError_t error, uint32_t lineNumber, const char *fileName);

/* Convenience macros matching production code */
#define NON_FATAL_ERROR(x) (NonFatalError(x, __LINE__, __FILE__))
#define NON_FATAL_ERROR_DETAIL(x, y) (NonFatalError_Detail(x, y, __LINE__, __FILE__))
#define NON_FATAL_ERROR_ISR(x) (NonFatalErrorISR(x, __LINE__, __FILE__))
#define NON_FATAL_ERROR_ISR_DETAIL(x, y) (NonFatalErrorISR_Detail(x, y, __LINE__, __FILE__))
#define FATAL_ERROR(x) (FatalError(x, __LINE__, __FILE__))

/* Mock control functions for testing */
void MockErrors_Reset(void);
uint32_t MockErrors_GetNonFatalCount(NonFatalError_t error);
uint32_t MockErrors_GetNonFatalISRCount(NonFatalError_t error);
uint32_t MockErrors_GetTotalNonFatalCount(void);
uint32_t MockErrors_GetTotalNonFatalISRCount(void);
bool MockErrors_GetLastNonFatalDetail(NonFatalError_t *error, uint32_t *detail);
bool MockErrors_GetLastNonFatalISRDetail(NonFatalError_t *error, uint32_t *detail);
bool MockErrors_FatalErrorOccurred(FatalError_t *error);
uint32_t MockErrors_GetLastErrorLine(void);
const char* MockErrors_GetLastErrorFile(void);

#ifdef __cplusplus
}
#endif
