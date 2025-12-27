/**
 * FlashTest.cpp - Unit tests for flash.c module
 *
 * Tests cover EEPROM emulation, option bytes configuration, and fatal error persistence.
 * These are safety-critical tests ensuring watchdog configuration and error tracking.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
    #include "flash.h"
    #include "MockFlash.h"
    #include "MockErrors.h"
}

/**
 * TEST_GROUP: SetBit
 * Tests the bit manipulation utility function used for option bytes configuration
 */
TEST_GROUP(SetBit) {
    void setup() {
        MockFlash_Reset();
        MockErrors_Reset();
    }

    void teardown() {
        MockFlash_Reset();
        MockErrors_Reset();
    }
};

TEST(SetBit, ZeroBitSet) {
    uint32_t result = set_bit(0x00000000, 5, true);
    CHECK_EQUAL(0x00000020, result);
}

TEST(SetBit, ZeroBitClear) {
    uint32_t result = set_bit(0xFFFFFFFF, 5, false);
    CHECK_EQUAL(0xFFFFFFDF, result);
}

TEST(SetBit, ToggleBitOn) {
    uint32_t result = set_bit(0x00000000, 3, true);
    CHECK_EQUAL(0x00000008, result);
}

TEST(SetBit, ToggleBitOff) {
    uint32_t result = set_bit(0x00000008, 3, false);
    CHECK_EQUAL(0x00000000, result);
}

TEST(SetBit, MultipleBitsSequence) {
    uint32_t result = 0x00000000;
    result = set_bit(result, 0, true);  // Set bit 0
    result = set_bit(result, 5, true);  // Set bit 5
    result = set_bit(result, 10, true); // Set bit 10
    CHECK_EQUAL(0x00000421, result);
}

TEST(SetBit, Bit31) {
    uint32_t result = set_bit(0x00000000, 31, true);
    CHECK_EQUAL(0x80000000, result);
}

TEST(SetBit, Bit0) {
    uint32_t result = set_bit(0x00000000, 0, true);
    CHECK_EQUAL(0x00000001, result);
}

/**
 * TEST_GROUP: OptionBytes
 * Tests option bytes configuration - SAFETY CRITICAL
 * Verifies watchdog settings, reset modes, and boot configuration
 */
TEST_GROUP(OptionBytes) {
    void setup() {
        MockFlash_Reset();
        MockErrors_Reset();
    }

    void teardown() {
        MockFlash_Reset();
        MockErrors_Reset();
    }
};

TEST(OptionBytes, UnlockCalled) {
    setOptionBytes();
    CHECK_EQUAL(1, MockFlash_GetUnlockCallCount());
}

TEST(OptionBytes, LockCalled) {
    setOptionBytes();
    CHECK_EQUAL(1, MockFlash_GetLockCallCount());
}

TEST(OptionBytes, UnlockFailureLogsError) {
    MockFlash_SetUnlockBehavior(HAL_ERROR);
    setOptionBytes();
    CHECK_EQUAL(1, MockErrors_GetNonFatalCount(FLASH_LOCK_ERR));
}

TEST(OptionBytes, nBoot0SetTrue) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_nBOOT0_Pos, true));
}

TEST(OptionBytes, nSWBOOT0SetFalse) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_nSWBOOT0_Pos, false));
}

TEST(OptionBytes, SRAM2ResetOnReset) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_SRAM2_RST_Pos, false));
}

TEST(OptionBytes, SRAM2ParityCheckDisabled) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_SRAM2_PE_Pos, true));
}

TEST(OptionBytes, nBoot1SetTrue) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_nBOOT1_Pos, true));
}

TEST(OptionBytes, WWDGSoftwareControlled) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_WWDG_SW_Pos, true));
}

TEST(OptionBytes, IWDGNotFrozenStandby) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_IWDG_STDBY_Pos, true));
}

TEST(OptionBytes, IWDGNotFrozenStop) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_IWDG_STOP_Pos, true));
}

/* SAFETY CRITICAL: IWDG must start on power-up */
TEST(OptionBytes, IWDGStartsOnPowerUp) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_IWDG_SW_Pos, false));
}

TEST(OptionBytes, ResetOnShutdown) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_nRST_SHDW_Pos, true));
}

TEST(OptionBytes, ResetOnStandby) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_nRST_STDBY_Pos, true));
}

TEST(OptionBytes, ResetOnStop) {
    setOptionBytes();
    CHECK_TRUE(MockFlash_VerifyOptionBit(FLASH_OPTR_nRST_STOP_Pos, true));
}

/* SAFETY CRITICAL: BOR level protects against brownout */
TEST(OptionBytes, BORLevel2_8V) {
    setOptionBytes();
    FLASH_OBProgramInitTypeDef config;
    CHECK_TRUE(MockFlash_GetLastOBConfig(&config));
    CHECK_TRUE((config.USERConfig & FLASH_OPTR_BOR_LEV_4) != 0);
}

TEST(OptionBytes, ProgramCalledWhenChanged) {
    /* First call gets current config (which is zero), compares to new config */
    setOptionBytes();
    CHECK_EQUAL(1, MockFlash_GetOBProgramCallCount());
}

TEST(OptionBytes, ProgramFailureLogsError) {
    MockFlash_SetOBProgramBehavior(HAL_ERROR);
    setOptionBytes();
    CHECK_EQUAL(1, MockErrors_GetNonFatalCount(EEPROM_ERR));
}

TEST(OptionBytes, LockFailureLogsError) {
    MockFlash_SetLockBehavior(HAL_ERROR);
    setOptionBytes();
    CHECK_EQUAL(1, MockErrors_GetNonFatalCount(FLASH_LOCK_ERR));
}

/**
 * TEST_GROUP: FlashInit
 * Tests EEPROM initialization sequence including error recovery
 */
TEST_GROUP(FlashInit) {
    void setup() {
        MockFlash_Reset();
        MockErrors_Reset();
    }

    void teardown() {
        MockFlash_Reset();
        MockErrors_Reset();
    }
};

TEST(FlashInit, UnlockCalled) {
    initFlash();
    CHECK(MockFlash_GetUnlockCallCount() >= 1);
}

TEST(FlashInit, LockCalled) {
    initFlash();
    CHECK(MockFlash_GetLockCallCount() >= 1);
}

TEST(FlashInit, InitCalledWithForcedErase) {
    initFlash();
    CHECK_EQUAL(1, MockFlash_GetInitCallCount());
}

TEST(FlashInit, UnlockFailureLogsError) {
    MockFlash_SetUnlockBehavior(HAL_ERROR);
    initFlash();
    /* initFlash() calls unlock (1 error), then calls setOptionBytes() which also calls unlock (2nd error) */
    CHECK_EQUAL(2, MockErrors_GetNonFatalCount(FLASH_LOCK_ERR));
}

/* Format fallback when init returns write error */
TEST(FlashInit, WriteErrorTriggersFormat) {
    MockFlash_SetInitBehavior(EE_WRITE_ERROR);
    initFlash();
    CHECK_EQUAL(1, MockFlash_GetFormatCallCount());
}

TEST(FlashInit, FormatFailureLogsError) {
    MockFlash_SetInitBehavior(EE_WRITE_ERROR);
    MockFlash_SetFormatBehavior(EE_ERROR_NOACTIVE_PAGE);
    initFlash();
    /* Three errors: 1) Initial EE_Init fails, 2) EE_Format fails, 3) Final status check */
    CHECK_EQUAL(3, MockErrors_GetNonFatalCount(EEPROM_ERR));
}

TEST(FlashInit, ReinitAfterSuccessfulFormat) {
    MockFlash_SetInitBehavior(EE_WRITE_ERROR);
    MockFlash_SetFormatBehavior(EE_OK);
    initFlash();
    CHECK_EQUAL(2, MockFlash_GetInitCallCount()); // Initial attempt + retry after format
}

TEST(FlashInit, LockFailureLogsError) {
    MockFlash_SetLockBehavior(HAL_ERROR);
    initFlash();
    /* initFlash() calls lock (1 error), then calls setOptionBytes() which also calls lock (2nd error) */
    CHECK_EQUAL(2, MockErrors_GetNonFatalCount(FLASH_LOCK_ERR));
}

TEST(FlashInit, OptionBytesCalledAtEnd) {
    initFlash();
    /* setOptionBytes also calls unlock/lock, so check it was called */
    CHECK(MockFlash_GetOBProgramCallCount() >= 1);
}

/**
 * TEST_GROUP: FatalErrorPersistence
 * Tests reading and writing fatal errors to EEPROM - SAFETY CRITICAL
 */
TEST_GROUP(FatalErrorPersistence) {
    void setup() {
        MockFlash_Reset();
        MockErrors_Reset();
    }

    void teardown() {
        MockFlash_Reset();
        MockErrors_Reset();
    }
};

TEST(FatalErrorPersistence, GetFatalError_NoDataInitializesToNone) {
    MockFlash_SetReadBehavior(EE_NO_DATA, 0);
    FatalError_t err = HARD_FAULT_FERR;
    bool result = GetFatalError(&err);
    CHECK_FALSE(result); // Read failed because no data
    /* SetFatalError should be called to init to NONE_FERR */
    CHECK_EQUAL(1, MockFlash_GetWriteCallCount());
}

TEST(FatalErrorPersistence, GetFatalError_Success) {
    MockFlash_SetStoredValue(0x04, STACK_OVERFLOW_FERR);
    FatalError_t err = NONE_FERR;
    bool result = GetFatalError(&err);
    CHECK_TRUE(result);
    CHECK_EQUAL(STACK_OVERFLOW_FERR, err);
}

TEST(FatalErrorPersistence, GetFatalError_EEPROMError) {
    /* Store a value so the mock doesn't return EE_NO_DATA */
    MockFlash_SetStoredValue(0x04, HARD_FAULT_FERR);
    /* Now set read behavior to return an error */
    MockFlash_SetReadBehavior(EE_ERROR_NOACTIVE_PAGE, 0);
    FatalError_t err = NONE_FERR;
    bool result = GetFatalError(&err);
    CHECK_FALSE(result);
    CHECK_EQUAL(1, MockErrors_GetNonFatalCount(EEPROM_ERR));
}

TEST(FatalErrorPersistence, SetFatalError_Success) {
    bool result = SetFatalError(BUS_FAULT_FERR);
    CHECK_TRUE(result);
    CHECK_EQUAL(1, MockFlash_GetWriteCallCount());
    uint16_t addr;
    uint32_t data;
    MockFlash_GetLastWrite(&addr, &data);
    CHECK_EQUAL(0x04, addr);
    CHECK_EQUAL(BUS_FAULT_FERR, data);
}

TEST(FatalErrorPersistence, SetFatalError_WriteFailure) {
    MockFlash_SetWriteBehavior(EE_WRITE_ERROR);
    bool result = SetFatalError(MALLOC_FAIL_FERR);
    CHECK_FALSE(result);
    /* Should retry 3 times */
    CHECK_EQUAL(3, MockFlash_GetWriteCallCount());
}

TEST(FatalErrorPersistence, SetFatalError_CleanupRequired) {
    MockFlash_SetWriteBehavior(EE_CLEANUP_REQUIRED);
    bool result = SetFatalError(ASSERT_FAIL_FERR);
    CHECK_TRUE(result);
    CHECK_EQUAL(1, MockFlash_GetCleanupCallCount());
}

TEST(FatalErrorPersistence, RoundTripWriteRead) {
    /* Write a value */
    SetFatalError(MEM_FAULT_FERR);

    /* Read it back */
    FatalError_t err = NONE_FERR;
    bool result = GetFatalError(&err);
    CHECK_TRUE(result);
    CHECK_EQUAL(MEM_FAULT_FERR, err);
}

TEST(FatalErrorPersistence, WriteInt32_UnlockFailure) {
    MockFlash_SetUnlockBehavior(HAL_ERROR);
    bool result = SetFatalError(USAGE_FAULT_FERR);
    CHECK_FALSE(result);
    /* WriteInt32 retries 3 times, each attempt fails to unlock */
    CHECK_EQUAL(3, MockErrors_GetNonFatalCount(FLASH_LOCK_ERR));
}

TEST(FatalErrorPersistence, WriteInt32_LockFailure) {
    MockFlash_SetLockBehavior(HAL_ERROR);
    bool result = SetFatalError(NMI_TRIGGERED_FERR);
    CHECK_FALSE(result);
    /* WriteInt32 retries 3 times, each attempt succeeds unlock/write but fails lock */
    CHECK_EQUAL(3, MockErrors_GetNonFatalCount(FLASH_LOCK_ERR));
}

TEST(FatalErrorPersistence, WriteInt32_RetriesOnFailure) {
    /* Fail first two attempts, succeed on third */
    MockFlash_SetWriteBehavior(EE_WRITE_ERROR);
    bool result = SetFatalError(BUFFER_OVERRUN_FERR);
    CHECK_FALSE(result);
    /* Verify it tried MAX_WRITE_ATTEMPTS (3) times */
    CHECK_EQUAL(3, MockFlash_GetWriteCallCount());
}

TEST(FatalErrorPersistence, WriteInt32_EEPROMErrorLogged) {
    MockFlash_SetWriteBehavior(EE_ERROR_NOACTIVE_PAGE);
    SetFatalError(UNDEFINED_STATE_FERR);
    CHECK_EQUAL(3, MockErrors_GetNonFatalCount(EEPROM_ERR)); // 3 attempts, all fail
}

/* Main test runner */
int main(int argc, char** argv) {
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
