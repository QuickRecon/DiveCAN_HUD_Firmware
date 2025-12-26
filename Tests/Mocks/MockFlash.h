#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* HAL Status Type (shared with MockDelay.h and MockCAN.h) */
#ifndef HAL_STATUSDEF
#define HAL_STATUSDEF
    typedef enum
    {
        HAL_OK = 0x00,
        HAL_ERROR = 0x01,
        HAL_BUSY = 0x02,
        HAL_TIMEOUT = 0x03
    } HAL_StatusTypeDef;
#endif

    /* EE Status enum - from ST EEPROM emulation library */
    typedef enum
    {
        EE_OK = 0U,
        EE_ERASE_ERROR,
        EE_WRITE_ERROR,
        EE_ERROR_NOACTIVE_PAGE,
        EE_ERROR_NOERASE_PAGE,
        EE_ERROR_NOERASING_PAGE,
        EE_ERROR_NOACTIVE_NORECEIVE_NOVALID_PAGE,
        EE_NO_DATA,
        EE_INVALID_VIRTUALADDRESS,
        EE_INVALID_PAGE,
        EE_INVALID_PAGE_SEQUENCE,
        EE_INVALID_ELEMENT,
        EE_TRANSFER_ERROR,
        EE_DELETE_ERROR,
        EE_INVALID_BANK_CFG,
        EE_NO_PAGE_FOUND,
        EE_PAGE_NOTERASED,
        EE_PAGE_ERASED,
        EE_PAGE_FULL,
        EE_CLEANUP_REQUIRED = 0x100U
    } EE_Status;

    typedef enum
    {
        EE_FORCED_ERASE,
        EE_CONDITIONAL_ERASE
    } EE_Erase_type;

    /* FLASH Option Bytes structure */
    typedef struct
    {
        uint32_t OptionType;     /*!< Option byte to be configured */
        uint32_t WRPArea;        /*!< Write protection area */
        uint32_t WRPStartOffset; /*!< Write protection start offset */
        uint32_t WRPEndOffset;   /*!< Write protection end offset */
        uint32_t RDPLevel;       /*!< Read protection level */
        uint32_t USERType;       /*!< User option byte(s) to be configured */
        uint32_t USERConfig;     /*!< Value of the user option byte */
        uint32_t PCROPConfig;    /*!< Configuration of the PCROP */
        uint32_t PCROPStartAddr; /*!< PCROP Start address */
        uint32_t PCROPEndAddr;   /*!< PCROP End address */
    } FLASH_OBProgramInitTypeDef;

/* FLASH OPTR bit positions */
#define FLASH_OPTR_nBOOT0_Pos (27U)
#define FLASH_OPTR_nSWBOOT0_Pos (26U)
#define FLASH_OPTR_SRAM2_RST_Pos (25U)
#define FLASH_OPTR_SRAM2_PE_Pos (24U)
#define FLASH_OPTR_nBOOT1_Pos (23U)
#define FLASH_OPTR_WWDG_SW_Pos (19U)
#define FLASH_OPTR_IWDG_STDBY_Pos (18U)
#define FLASH_OPTR_IWDG_STOP_Pos (17U)
#define FLASH_OPTR_IWDG_SW_Pos (16U)
#define FLASH_OPTR_nRST_SHDW_Pos (14U)
#define FLASH_OPTR_nRST_STDBY_Pos (13U)
#define FLASH_OPTR_nRST_STOP_Pos (12U)
#define FLASH_OPTR_BOR_LEV_Pos (8U)
#define FLASH_OPTR_BOR_LEV_4 (0x4UL << FLASH_OPTR_BOR_LEV_Pos)

/* OB_USER Type flags */
#define OB_USER_BOR_LEV ((uint32_t)0x0001)
#define OB_USER_nRST_STOP ((uint32_t)0x0002)
#define OB_USER_nRST_STDBY ((uint32_t)0x0004)
#define OB_USER_IWDG_SW ((uint32_t)0x0008)
#define OB_USER_IWDG_STOP ((uint32_t)0x0010)
#define OB_USER_IWDG_STDBY ((uint32_t)0x0020)
#define OB_USER_WWDG_SW ((uint32_t)0x0040)
#define OB_USER_nBOOT1 ((uint32_t)0x0200)
#define OB_USER_SRAM2_PE ((uint32_t)0x0400)
#define OB_USER_SRAM2_RST ((uint32_t)0x0800)
#define OB_USER_nRST_SHDW ((uint32_t)0x1000)
#define OB_USER_nSWBOOT0 ((uint32_t)0x2000)
#define OB_USER_nBOOT0 ((uint32_t)0x4000)

/* FLASH Flag clear macro - in tests this is a no-op */
#define __HAL_FLASH_CLEAR_FLAG(flags) ((void)0)

    /* HAL Flash API functions to be mocked */
    HAL_StatusTypeDef HAL_FLASH_Unlock(void);
    HAL_StatusTypeDef HAL_FLASH_Lock(void);
    HAL_StatusTypeDef HAL_FLASHEx_OBProgram(FLASH_OBProgramInitTypeDef *pOBInit);
    void HAL_FLASHEx_OBGetConfig(FLASH_OBProgramInitTypeDef *pOBInit);

    /* EEPROM Emulation API functions to be mocked */
    EE_Status EE_Init(EE_Erase_type EraseType);
    EE_Status EE_Format(EE_Erase_type EraseType);
    EE_Status EE_WriteVariable32bits(uint16_t VirtAddress, uint32_t Data);
    EE_Status EE_ReadVariable32bits(uint16_t VirtAddress, uint32_t *Data);
    EE_Status EE_CleanUp(void);

    /* Mock control functions for testing */
    void MockFlash_Reset(void);
    void MockFlash_SetUnlockBehavior(HAL_StatusTypeDef status);
    void MockFlash_SetLockBehavior(HAL_StatusTypeDef status);
    void MockFlash_SetOBProgramBehavior(HAL_StatusTypeDef status);
    void MockFlash_SetInitBehavior(EE_Status status);
    void MockFlash_SetFormatBehavior(EE_Status status);
    void MockFlash_SetWriteBehavior(EE_Status status);
    void MockFlash_SetReadBehavior(EE_Status status, uint32_t value);
    void MockFlash_SetCleanupBehavior(EE_Status status);

    /* Mock query functions for verification */
    uint32_t MockFlash_GetUnlockCallCount(void);
    uint32_t MockFlash_GetLockCallCount(void);
    uint32_t MockFlash_GetOBProgramCallCount(void);
    uint32_t MockFlash_GetInitCallCount(void);
    uint32_t MockFlash_GetFormatCallCount(void);
    uint32_t MockFlash_GetWriteCallCount(void);
    uint32_t MockFlash_GetReadCallCount(void);
    uint32_t MockFlash_GetCleanupCallCount(void);

    /* Get last written/read data */
    bool MockFlash_GetLastWrite(uint16_t *addr, uint32_t *data);
    bool MockFlash_GetLastRead(uint16_t *addr);

    /* Option bytes verification */
    bool MockFlash_GetLastOBConfig(FLASH_OBProgramInitTypeDef *config);
    bool MockFlash_VerifyOptionBit(uint32_t bitPosition, bool expectedValue);

    /* Simulated EEPROM storage */
    void MockFlash_SetStoredValue(uint16_t addr, uint32_t value);
    uint32_t MockFlash_GetStoredValue(uint16_t addr);

#ifdef __cplusplus
}
#endif
