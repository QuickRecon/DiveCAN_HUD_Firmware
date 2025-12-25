/**
 * @file HUDControlTest.cpp
 * @brief Unit tests for HUDControl PPO2 display logic
 *
 * Tests the critical safety logic that converts PPO2 sensor data into
 * visual LED blink patterns for the diver. These tests verify:
 * - PPO2 deviation calculation and rounding
 * - Alert detection for dangerous PPO2 levels
 * - Cell failure detection (0xFF values)
 * - Status mask and fail mask handling
 * - Queue empty handling
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestPlugin.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "MockQueue.h"
#include "MockLEDs.h"

extern "C" {
    #include "HUDControl.h"
    #include "DiveCAN/DiveCAN.h"
    #include "common.h"
}

/* Static flag to track queue initialization across all tests */
static bool queuesInitialized = false;

TEST_GROUP(HUDControl)
{
    CellValues_t cellValues;
    bool alerting;

    void setup()
    {
        /* Initialize queues only once to avoid memory leak detection issues */
        if (!queuesInitialized) {
            MockQueue_Init();
            queuesInitialized = true;
        }

        MockQueue_Reset();
        MockLEDs_Reset();

        cellValues.C1 = 0;
        cellValues.C2 = 0;
        cellValues.C3 = 0;
        alerting = false;
    }

    void teardown()
    {
        MockQueue_Reset();
        MockLEDs_Reset();
    }

    /* Helper to put PPO2 data into the queue */
    void enqueuePPO2(int16_t c1, int16_t c2, int16_t c3)
    {
        CellValues_t values;
        values.C1 = c1;
        values.C2 = c2;
        values.C3 = c3;
        osMessageQueuePut(PPO2QueueHandle, &values, 0, 0);
    }

    /* Helper to put cell status into the queue */
    void enqueueCellStatus(uint8_t status)
    {
        osMessageQueuePut(CellStatQueueHandle, &status, 0, 0);
    }
};

/**
 * Test Group: div10_round() - Rounding Function
 *
 * This function is critical for converting PPO2 deviations to blink counts.
 * Incorrect rounding could show wrong PPO2 values to the diver.
 */
TEST_GROUP(DivisionRounding)
{
    void setup() {}
    void teardown() {}
};

TEST(DivisionRounding, ZeroReturnsZero)
{
    CHECK_EQUAL(0, div10_round(0));
}

TEST(DivisionRounding, PositiveRoundsDown)
{
    CHECK_EQUAL(1, div10_round(10));
    CHECK_EQUAL(1, div10_round(11));
    CHECK_EQUAL(1, div10_round(12));
    CHECK_EQUAL(1, div10_round(13));
    CHECK_EQUAL(1, div10_round(14));
}

TEST(DivisionRounding, PositiveRoundsUp)
{
    CHECK_EQUAL(2, div10_round(15));
    CHECK_EQUAL(2, div10_round(16));
    CHECK_EQUAL(2, div10_round(17));
    CHECK_EQUAL(2, div10_round(18));
    CHECK_EQUAL(2, div10_round(19));
    CHECK_EQUAL(2, div10_round(20));
}

TEST(DivisionRounding, NegativeRoundsDown)
{
    CHECK_EQUAL(-1, div10_round(-10));
    CHECK_EQUAL(-1, div10_round(-11));
    CHECK_EQUAL(-1, div10_round(-12));
    CHECK_EQUAL(-1, div10_round(-13));
    CHECK_EQUAL(-1, div10_round(-14));
}

TEST(DivisionRounding, NegativeRoundsUp)
{
    CHECK_EQUAL(-2, div10_round(-15));
    CHECK_EQUAL(-2, div10_round(-16));
    CHECK_EQUAL(-2, div10_round(-17));
    CHECK_EQUAL(-2, div10_round(-18));
    CHECK_EQUAL(-2, div10_round(-19));
    CHECK_EQUAL(-2, div10_round(-20));
}

TEST(DivisionRounding, LargePositiveValues)
{
    CHECK_EQUAL(10, div10_round(100));
    CHECK_EQUAL(25, div10_round(250));
    CHECK_EQUAL(26, div10_round(255));
}

TEST(DivisionRounding, LargeNegativeValues)
{
    CHECK_EQUAL(-10, div10_round(-100));
    CHECK_EQUAL(-25, div10_round(-250));
}

/**
 * Test Group: cell_alert() - Alert Detection
 *
 * Detects dangerous PPO2 levels that require immediate diver attention.
 * PPO2 < 40 (0.4 bar) = hypoxia risk
 * PPO2 > 165 (1.65 bar) = oxygen toxicity risk
 */
TEST_GROUP(CellAlert)
{
    void setup() {}
    void teardown() {}
};

TEST(CellAlert, NormalValuesNotAlerting)
{
    CHECK_FALSE(cell_alert(40));
    CHECK_FALSE(cell_alert(50));
    CHECK_FALSE(cell_alert(100));
    CHECK_FALSE(cell_alert(150));
    CHECK_FALSE(cell_alert(165));
}

TEST(CellAlert, LowValueAlerting)
{
    CHECK_TRUE(cell_alert(0));
    CHECK_TRUE(cell_alert(10));
    CHECK_TRUE(cell_alert(39));
}

TEST(CellAlert, HighValueAlerting)
{
    CHECK_TRUE(cell_alert(166));
    CHECK_TRUE(cell_alert(200));
    CHECK_TRUE(cell_alert(254));
}

TEST(CellAlert, BoundaryConditions)
{
    /* 39 should alert, 40 should not */
    CHECK_TRUE(cell_alert(39));
    CHECK_FALSE(cell_alert(40));

    /* 165 should not alert, 166 should */
    CHECK_FALSE(cell_alert(165));
    CHECK_TRUE(cell_alert(166));
}

TEST(CellAlert, FailureValueAlerting)
{
    /* 0xFF indicates cell failure, which also alerts (255 > 165) */
    CHECK_TRUE(cell_alert(255));
}

/**
 * Test Group: PPO2Blink() - Main PPO2 Display Logic
 *
 * This is the core function that processes PPO2 data and controls the LED display.
 * Tests verify correct behavior for:
 * - Normal operation
 * - Alert conditions
 * - Cell failures
 * - Empty queue handling
 */

TEST(HUDControl, EmptyQueueCallsBlinkNoData)
{
    /* Queue is empty, should call blinkNoData() AND blinkCode() with current cellValues */
    PPO2Blink(&cellValues, &alerting);

    /* Both blinkNoData and blinkCode should be called */
    CHECK_EQUAL(1, MockLEDs_GetBlinkNoDataCallCount());
    CHECK_EQUAL(0, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());

    /* Verify blinkCode was called with current cellValues (initialized to 0) */
    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);
    CHECK_EQUAL(-10, c1);  /* (0-100) = -100, div10_round(-100) = -10 */
    CHECK_EQUAL(-10, c2);
    CHECK_EQUAL(-10, c3);
}

TEST(HUDControl, NormalPPO2AtSetpoint)
{
    /* All cells at setpoint (100 = 1.0 bar) */
    enqueuePPO2(100, 100, 100);

    PPO2Blink(&cellValues, &alerting);

    /* Should call blinkCode with all zeros (no deviation) */
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());

    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);

    CHECK_EQUAL(0, c1);
    CHECK_EQUAL(0, c2);
    CHECK_EQUAL(0, c3);
    CHECK_EQUAL(0b111, statusMask);  /* Default all good */
    CHECK_EQUAL(0b111, failMask);    /* All cells working */

    CHECK_FALSE(alerting);
}

TEST(HUDControl, PositiveDeviationRoundsCorrectly)
{
    /* C1 = 115 -> deviation = +15 -> div10_round(15) = +2 (rounds up) */
    /* C2 = 110 -> deviation = +10 -> div10_round(10) = +1 */
    /* C3 = 104 -> deviation = +4 -> div10_round(4) = 0 (rounds down) */
    enqueuePPO2(115, 110, 104);

    PPO2Blink(&cellValues, &alerting);

    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);

    CHECK_EQUAL(2, c1);  /* (115-100) = 15, div10_round(15) = 2 */
    CHECK_EQUAL(1, c2);  /* (110-100) = 10, div10_round(10) = 1 */
    CHECK_EQUAL(0, c3);  /* (104-100) = 4, div10_round(4) = 0 */
}

TEST(HUDControl, NegativeDeviationRoundsCorrectly)
{
    /* C1 = 85 -> deviation = -15 -> div10_round(-15) = -2 (rounds away from zero) */
    /* C2 = 90 -> deviation = -10 -> div10_round(-10) = -1 */
    /* C3 = 96 -> deviation = -4 -> div10_round(-4) = 0 */
    enqueuePPO2(85, 90, 96);

    PPO2Blink(&cellValues, &alerting);

    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);

    CHECK_EQUAL(-2, c1);  /* (85-100) = -15, div10_round(-15) = -2 */
    CHECK_EQUAL(-1, c2);  /* (90-100) = -10, div10_round(-10) = -1 */
    CHECK_EQUAL(0, c3);   /* (96-100) = -4, div10_round(-4) = 0 */
}

TEST(HUDControl, LowPPO2TriggersAlert)
{
    /* C1 = 39 (< 40) should trigger alert, but still call blinkCode() */
    enqueuePPO2(39, 100, 100);

    PPO2Blink(&cellValues, &alerting);

    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */
    CHECK_EQUAL(0, MockQueue_GetDelayCallCount());  /* No delay when alerting */
}

TEST(HUDControl, HighPPO2TriggersAlert)
{
    /* C2 = 166 (> 165) should trigger alert, but still call blinkCode() */
    enqueuePPO2(100, 166, 100);

    PPO2Blink(&cellValues, &alerting);

    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */
}

TEST(HUDControl, MultipleCellsAlertingTriggersAlert)
{
    /* C1 = 30, C3 = 170 - both alerting, but still calls blinkCode() */
    enqueuePPO2(30, 100, 170);

    PPO2Blink(&cellValues, &alerting);

    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */
}

TEST(HUDControl, AlertBoundaryCondition_39ShouldAlert)
{
    enqueuePPO2(39, 100, 100);

    PPO2Blink(&cellValues, &alerting);

    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */
}

TEST(HUDControl, AlertBoundaryCondition_40ShouldNotAlert)
{
    enqueuePPO2(40, 100, 100);

    PPO2Blink(&cellValues, &alerting);

    CHECK_FALSE(alerting);
    CHECK_EQUAL(0, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());
    CHECK_EQUAL(1, MockQueue_GetDelayCallCount());  /* Delay when not alerting */
}

TEST(HUDControl, AlertBoundaryCondition_165ShouldNotAlert)
{
    enqueuePPO2(100, 165, 100);

    PPO2Blink(&cellValues, &alerting);

    CHECK_FALSE(alerting);
    CHECK_EQUAL(0, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());
    CHECK_EQUAL(1, MockQueue_GetDelayCallCount());  /* Delay when not alerting */
}

TEST(HUDControl, AlertBoundaryCondition_166ShouldAlert)
{
    enqueuePPO2(100, 166, 100);

    PPO2Blink(&cellValues, &alerting);

    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */
}

TEST(HUDControl, CellFailureDetection_SingleCell)
{
    /* C1 = 0xFF indicates cell failure
     * Note: 0xFF (255) > 165, so this triggers alert AND shows failMask */
    enqueuePPO2(0xFF, 100, 100);

    PPO2Blink(&cellValues, &alerting);

    /* Should trigger alert because 0xFF > 165 */
    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */

    /* Verify failMask shows C1 failed */
    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);
    CHECK_EQUAL(0b110, failMask);  /* C1 bit cleared (bit 0 = 0) */
}

TEST(HUDControl, CellFailureDetection_MultipleCells)
{
    /* C1 and C3 failed - both 0xFF will trigger alert */
    enqueuePPO2(0xFF, 100, 0xFF);

    PPO2Blink(&cellValues, &alerting);

    /* Should trigger alert because 0xFF > 165 */
    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */

    /* Verify failMask shows C1 and C3 failed */
    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);
    CHECK_EQUAL(0b010, failMask);  /* Only C2 working (bits 0 and 2 = 0) */
}

TEST(HUDControl, CellFailureDetection_AllCells)
{
    /* All cells failed */
    enqueuePPO2(0xFF, 0xFF, 0xFF);

    PPO2Blink(&cellValues, &alerting);

    /* Should trigger alert */
    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */

    /* Verify failMask shows all cells failed */
    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);
    CHECK_EQUAL(0b000, failMask);  /* All cells failed */
}

TEST(HUDControl, StatusMaskFromQueue)
{
    /* Put custom status mask in queue */
    enqueuePPO2(100, 100, 100);
    enqueueCellStatus(0b101);  /* Only C1 and C3 voted */

    PPO2Blink(&cellValues, &alerting);

    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);

    CHECK_EQUAL(0b101, statusMask);
}

TEST(HUDControl, StatusMaskDefaultWhenQueueEmpty)
{
    /* Don't enqueue status, should default to 0b111 */
    enqueuePPO2(100, 100, 100);

    PPO2Blink(&cellValues, &alerting);

    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);

    CHECK_EQUAL(0b111, statusMask);
}

TEST(HUDControl, DelayCalledOnNormalOperation)
{
    enqueuePPO2(100, 100, 100);

    PPO2Blink(&cellValues, &alerting);

    /* Should call osDelay with 500ms timeout */
    CHECK_EQUAL(1, MockQueue_GetDelayCallCount());
    /* Note: Can't easily check exact tick value without exposing TIMEOUT_500MS_TICKS */
}

TEST(HUDControl, NoDelayOnAlert)
{
    enqueuePPO2(30, 100, 100);

    PPO2Blink(&cellValues, &alerting);

    /* Should NOT call osDelay when alerting */
    CHECK_EQUAL(0, MockQueue_GetDelayCallCount());
}

TEST(HUDControl, NoDelayOnEmptyQueue)
{
    /* Empty queue */
    PPO2Blink(&cellValues, &alerting);

    /* Should NOT call osDelay when queue empty */
    CHECK_EQUAL(0, MockQueue_GetDelayCallCount());
}

TEST(HUDControl, MaximumPositiveDeviation)
{
    /* C1 = 255, deviation = +155, div10_round(155) = +16 */
    enqueuePPO2(255, 100, 100);

    PPO2Blink(&cellValues, &alerting);

    /* This should alert (255 > 165) */
    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */
}

TEST(HUDControl, MaximumNegativeDeviation)
{
    /* C1 = 0, deviation = -100, div10_round(-100) = -10 */
    enqueuePPO2(0, 100, 100);

    PPO2Blink(&cellValues, &alerting);

    /* This should alert (0 < 40) */
    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */
}

TEST(HUDControl, MixedNormalAndFailedCells)
{
    /* C2 = 0xFF would trigger alert since 255 > 165.
     * This test verifies mixed values with status mask and failMask */
    enqueuePPO2(105, 0xFF, 95);
    enqueueCellStatus(0b101);  /* C1 and C3 voted */

    PPO2Blink(&cellValues, &alerting);

    /* 0xFF triggers alert, but blinkCode is still called */
    CHECK_TRUE(alerting);
    CHECK_EQUAL(1, MockLEDs_GetBlinkAlarmCallCount());
    CHECK_EQUAL(1, MockLEDs_GetBlinkCodeCallCount());  /* Always called */

    /* Verify status mask and fail mask */
    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);
    CHECK_EQUAL(0b101, statusMask);  /* C1 and C3 voted */
    CHECK_EQUAL(0b101, failMask);  /* C2 failed (bit 1 = 0) */
}

/**
 * Integration test: Multiple calls to PPO2Blink
 */
TEST(HUDControl, MultipleCallsProcessQueueCorrectly)
{

    /* Enqueue two sets of values */
    enqueuePPO2(100, 100, 100);
    enqueuePPO2(110, 110, 110);

    /* First call should get first value */
    PPO2Blink(&cellValues, &alerting);

    int8_t c1, c2, c3;
    uint8_t statusMask, failMask;
    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);

    CHECK_EQUAL(0, c1);
    CHECK_EQUAL(0, c2);
    CHECK_EQUAL(0, c3);

    /* Second call should get second value */
    PPO2Blink(&cellValues, &alerting);

    MockLEDs_GetLastBlinkCode(&c1, &c2, &c3, &statusMask, &failMask);

    CHECK_EQUAL(1, c1);  /* (110-100) = 10, div10_round(10) = 1 */
    CHECK_EQUAL(1, c2);
    CHECK_EQUAL(1, c3);
}

int main(int argc, char** argv)
{
    /* Disable global memory leak detection for this test suite
     * The MockQueue fixture uses persistent std::queue objects across all tests
     * which CppUTest's per-test leak detector cannot properly track.
     * MockQueue_Cleanup() is called after tests to ensure proper RAII cleanup. */
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();

    int result = CommandLineTestRunner::RunAllTests(argc, argv);
    MockQueue_Cleanup();  // Clean up queues before exit
    return result;
}
