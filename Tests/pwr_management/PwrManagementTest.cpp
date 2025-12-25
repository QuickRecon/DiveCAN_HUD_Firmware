/**
 * PwrManagementTest.cpp - Unit tests for pwr_management.c module
 *
 * Tests cover shutdown sequence, GPIO pull-up/pull-down configuration,
 * bus detection, and STANDBY mode entry.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
    #define TESTING
    #include "Hardware/pwr_management.h"
    #include "MockPower.h"
    #include "flash.h"
    #include "MockFlash.h"
    #include "MockErrors.h"
    #include "printer.h"
}

/* Test constants */
#define FATAL_ERROR_BASE_ADDR 0x04

/**
 * TEST_GROUP: Shutdown
 * Tests shutdown sequence: fatal error reset, GPIO configuration, STANDBY entry
 * CRITICAL: All GPIO pins configured correctly to minimize parasitic current
 * CRITICAL: Fatal error cleared before shutdown (prevents false alarms on restart)
 */
TEST_GROUP(Shutdown) {
    void setup() {
        MockPower_Reset();
        MockFlash_Reset();
        MockErrors_Reset();
    }

    void teardown() {
        MockPower_Reset();
        MockFlash_Reset();
        MockErrors_Reset();
    }
};

/* Verify SetFatalError called with NONE_FERR to clear error state */
TEST(Shutdown, ClearsFatalError) {
    MockFlash_SetWriteBehavior(EE_OK);  /* SetFatalError succeeds */

    Shutdown();

    CHECK_EQUAL(NONE_FERR, MockFlash_GetStoredValue(FATAL_ERROR_BASE_ADDR));
}

/* Verify pull-up/pull-down configuration enabled */
TEST(Shutdown, EnablesPullUpPullDownConfig) {
    MockFlash_SetWriteBehavior(EE_OK);

    Shutdown();

    CHECK(MockPower_GetPullUpDownConfigEnabled());
}

/* Verify all 8 RGB LED pins pulled down (Port A: bits 1,2,3,4,6,7,8,15) */
TEST(Shutdown, RGBLEDsPulledDown) {
    MockFlash_SetWriteBehavior(EE_OK);

    Shutdown();

    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_A, PWR_GPIO_BIT_1));
    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_A, PWR_GPIO_BIT_2));
    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_A, PWR_GPIO_BIT_3));
    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_A, PWR_GPIO_BIT_4));
    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_A, PWR_GPIO_BIT_6));
    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_A, PWR_GPIO_BIT_7));
    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_A, PWR_GPIO_BIT_8));
    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_A, PWR_GPIO_BIT_15));
}

/* Verify all 4 alert LED pins pulled down (Port B: bits 0,1,6,7) */
TEST(Shutdown, AlertLEDsPulledDown) {
    MockFlash_SetWriteBehavior(EE_OK);

    Shutdown();

    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_B, PWR_GPIO_BIT_0));
    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_B, PWR_GPIO_BIT_1));
    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_B, PWR_GPIO_BIT_6));
    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_B, PWR_GPIO_BIT_7));
}

/* Verify CAN_EN pin pulled up for safe high-idle (Port C bit 14) */
TEST(Shutdown, CANEnablePulledUp) {
    MockFlash_SetWriteBehavior(EE_OK);

    Shutdown();

    CHECK(MockPower_IsPinPulledUp(PWR_GPIO_C, PWR_GPIO_BIT_14));
}

/* Verify PWR_BUS pin pulled down (Port C bit 15) */
TEST(Shutdown, PowerBusPulledDown) {
    MockFlash_SetWriteBehavior(EE_OK);

    Shutdown();

    CHECK(MockPower_IsPinPulledDown(PWR_GPIO_C, PWR_GPIO_BIT_15));
}

/* Verify STANDBY mode entered */
TEST(Shutdown, EntersStandbyMode) {
    MockFlash_SetWriteBehavior(EE_OK);

    Shutdown();

    CHECK(MockPower_GetStandbyEntered());
}

/* Verify correct number of pull-down calls (13 pins total: 8 RGB + 4 alert + 1 PWR_BUS) */
TEST(Shutdown, ThirteenPullDownCalls) {
    MockFlash_SetWriteBehavior(EE_OK);

    Shutdown();

    CHECK_EQUAL(13, MockPower_GetPullDownCallCount());
}

/* Verify correct number of pull-up calls (1 pin: CAN_EN) */
TEST(Shutdown, OnePullUpCall) {
    MockFlash_SetWriteBehavior(EE_OK);

    Shutdown();

    CHECK_EQUAL(1, MockPower_GetPullUpCallCount());
}

/* Verify shutdown proceeds even if SetFatalError fails */
TEST(Shutdown, ContinuesIfSetFatalErrorFails) {
    MockFlash_SetWriteBehavior(EE_WRITE_ERROR);  /* SetFatalError fails */

    Shutdown();

    /* Should still enter standby despite error */
    CHECK(MockPower_GetStandbyEntered());
}

/**
 * TEST_GROUP: TestBusActive
 * Tests bus detection with pull-up/release technique to avoid capacitive coupling
 */
TEST_GROUP(TestBusActive) {
    void setup() {
        MockPower_Reset();
    }

    void teardown() {
        MockPower_Reset();
    }
};

/* Verify GPIO_Init called twice (pull-up, then no-pull) */
TEST(TestBusActive, ConfiguresGPIOTwice) {
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, CAN_EN_Pin, GPIO_PIN_RESET);

    bool result = testBusActive();
    (void)result;  /* Suppress unused warning */

    CHECK_EQUAL(2, MockPower_GetGPIOInitCallCount());
}

/* Verify first GPIO_Init configures pull-up */
TEST(TestBusActive, FirstConfiguresPullUp) {
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, CAN_EN_Pin, GPIO_PIN_RESET);

    testBusActive();

    CHECK(MockPower_VerifyGPIOInit(CAN_EN_GPIO_Port, CAN_EN_Pin, GPIO_MODE_INPUT, GPIO_PULLUP));
}

/* Verify second GPIO_Init restores NOPULL */
TEST(TestBusActive, SecondConfiguresNoPull) {
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, CAN_EN_Pin, GPIO_PIN_RESET);

    testBusActive();

    /* Note: This check verifies the LAST call had NOPULL */
    CHECK(MockPower_VerifyGPIOInit(CAN_EN_GPIO_Port, CAN_EN_Pin, GPIO_MODE_INPUT, GPIO_NOPULL));
}

/* Verify returns true when pin is RESET (bus active) */
TEST(TestBusActive, ReturnsTrueWhenBusActive) {
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, CAN_EN_Pin, GPIO_PIN_RESET);

    bool result = testBusActive();

    CHECK(result);
}

/* Verify returns false when pin is SET (bus inactive) */
TEST(TestBusActive, ReturnsFalseWhenBusInactive) {
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, CAN_EN_Pin, GPIO_PIN_SET);

    bool result = testBusActive();

    CHECK_FALSE(result);
}

/**
 * TEST_GROUP: GetBusStatus
 * Tests simple bus status read (no configuration changes)
 */
TEST_GROUP(GetBusStatus) {
    void setup() {
        MockPower_Reset();
    }

    void teardown() {
        MockPower_Reset();
    }
};

/* Verify returns true when pin is RESET (bus on) */
TEST(GetBusStatus, ReturnsTrueWhenBusOn) {
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, CAN_EN_Pin, GPIO_PIN_RESET);

    bool result = getBusStatus();

    CHECK(result);
}

/* Verify returns false when pin is SET (bus off) */
TEST(GetBusStatus, ReturnsFalseWhenBusOff) {
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, CAN_EN_Pin, GPIO_PIN_SET);

    bool result = getBusStatus();

    CHECK_FALSE(result);
}

/* Verify does not call GPIO_Init (unlike testBusActive) */
TEST(GetBusStatus, DoesNotConfigureGPIO) {
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, CAN_EN_Pin, GPIO_PIN_RESET);

    getBusStatus();

    CHECK_EQUAL(0, MockPower_GetGPIOInitCallCount());
}

int main(int ac, char** av) {
    return CommandLineTestRunner::RunAllTests(ac, av);
}
