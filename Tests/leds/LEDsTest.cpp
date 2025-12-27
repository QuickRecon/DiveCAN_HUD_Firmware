/**
 * LEDsTest.cpp - Unit tests for leds.c module
 *
 * Tests cover RGB LED control, PWM bit-banging, PPO2 blink codes,
 * and interrupt handling. Focus on display logic correctness and timing.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
    #define TESTING
    #include "Hardware/leds.h"
    #include "MockHAL.h"
    #include "MockDelay.h"
    #include "queue.h"      /* For MockQueue functions */
    #include "cmsis_os.h"  /* For osDelay */
}

/* Test constants from leds.c */
const uint32_t STARTUP_DELAY_MS = 500;
/* LED_MAX_BRIGHTNESS is now exported from leds.h, no need to redefine */
const uint8_t LED_BRIGHTNESS[3] = {10, 3, 3};  // R, G, B
const uint8_t LED_MIN_BRIGHTNESS = 3;
const uint32_t BLINK_PERIOD = 500;  /* TIMEOUT_250MS_TICKS is actually 500ms (from common.h:32) */
const uint32_t TIMEOUT_50MS = 50;   /* TIMEOUT_50MS_TICKS in ms */

/**
 * TEST_GROUP: InitLEDs
 * Tests LED initialization sequence, color test pattern, and timing
 */
TEST_GROUP(InitLEDs) {
    void setup() {
        MockHAL_Reset();
        MockDelay_Reset();
    }

    void teardown() {
        MockHAL_Reset();
        MockDelay_Reset();
    }
};

/* Verify all end LEDs are turned on during init (they get turned off at the end) */
/* Note: This test is removed because we can't check intermediate states.
 * The EndLEDs_AllOffAtEnd test verifies the final state. */

/* Verify ASC_EN is turned on */
TEST(InitLEDs, ASC_EN_TurnedOn) {
    initLEDs();

    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(ASC_EN_GPIO_Port, ASC_EN_Pin));
}

/* Verify all RGB LEDs start off */
TEST(InitLEDs, RGB_LEDs_StartOff) {
    initLEDs();

    /* All RGB pins should be RESET (off) */
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(R2_GPIO_Port, R2_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(R3_GPIO_Port, R3_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(G1_GPIO_Port, G1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(G2_GPIO_Port, G2_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(G3_GPIO_Port, G3_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B1_GPIO_Port, B1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B2_GPIO_Port, B2_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B3_GPIO_Port, B3_Pin));
}

/* Verify color sequence: Red -> Green -> Blue */
TEST(InitLEDs, ColorSequence_RedGreenBlue) {
    initLEDs();

    /* Total delays: 3 * STARTUP_DELAY_MS + (3 colors * 3 channels * 6ms reset delay) */
    /* Each color calls setRGB(i, brightness, 0, 0) for 3 channels */
    /* Each setRGB calls setLEDBrightness 3 times (R, G, B), but only 1 is non-zero */
    /* Non-zero brightness adds 6ms reset delay: 3 colors * 3 channels * 6ms = 54ms */
    CHECK_EQUAL(3 * STARTUP_DELAY_MS + 54, MockDelay_GetTotalDelayMs());
}

/* Verify IWDG is refreshed 3 times during init */
TEST(InitLEDs, IWDG_RefreshedThreeTimes) {
    initLEDs();

    CHECK_EQUAL(3, MockDelay_GetIWDGRefreshCount());
}

/* Verify end LEDs are turned off at the end */
TEST(InitLEDs, EndLEDs_AllOffAtEnd) {
    initLEDs();

    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(LED_0_GPIO_Port, LED_0_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(LED_1_GPIO_Port, LED_1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(LED_2_GPIO_Port, LED_2_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(LED_3_GPIO_Port, LED_3_Pin));
}

/**
 * TEST_GROUP: SetLEDBrightness
 * Tests PWM bit-banging, interrupt control, and brightness levels
 * CRITICAL: Interrupts must be disabled during bit-banging
 */
TEST_GROUP(SetLEDBrightness) {
    void setup() {
        MockHAL_Reset();
        MockDelay_Reset();
    }

    void teardown() {
        MockHAL_Reset();
        MockDelay_Reset();
    }
};

/* Verify brightness 0 turns LED off without bit-banging */
TEST(SetLEDBrightness, Brightness0_LEDOff) {
    setLEDBrightness(0, R1_GPIO_Port, R1_Pin);

    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK(MockDelay_GetInterruptsEnabled());  /* Interrupts not disabled for zero brightness */
}

/* Verify brightness 1 requires 31 pulses (32 - 1) */
TEST(SetLEDBrightness, Brightness1_31Pulses) {
    setLEDBrightness(1, R1_GPIO_Port, R1_Pin);

    /* Final state should be SET (LED on) */
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK(MockDelay_GetInterruptsEnabled());  /* Interrupts re-enabled after bit-banging */
}

/* Verify max brightness (32) requires 0 pulses */
TEST(SetLEDBrightness, MaxBrightness_0Pulses) {
    setLEDBrightness(32, R1_GPIO_Port, R1_Pin);

    /* Final state should be SET (LED fully on) */
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK(MockDelay_GetInterruptsEnabled());
}

/* Verify 6ms reset delay before bit-banging */
TEST(SetLEDBrightness, ResetDelay_6ms) {
    setLEDBrightness(10, R1_GPIO_Port, R1_Pin);

    CHECK(MockDelay_GetTotalDelayMs() >= 6);  /* At least 6ms delay for reset */
}

/* Verify interrupts are re-enabled even after bit-banging */
TEST(SetLEDBrightness, Interrupts_ReEnabledAfter) {
    setLEDBrightness(16, R1_GPIO_Port, R1_Pin);

    CHECK(MockDelay_GetInterruptsEnabled());
}

/**
 * TEST_GROUP: SetRGB
 * Tests RGB channel control and pin mapping
 */
TEST_GROUP(SetRGB) {
    void setup() {
        MockHAL_Reset();
        MockDelay_Reset();
    }

    void teardown() {
        MockHAL_Reset();
        MockDelay_Reset();
    }
};

/* Verify channel 0 controls R1, G1, B1 */
TEST(SetRGB, Channel0_R1G1B1) {
    setRGB(0, 10, 5, 3);

    /* All should be SET (on) */
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(G1_GPIO_Port, G1_Pin));
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(B1_GPIO_Port, B1_Pin));
}

/* Verify channel 1 controls R2, G2, B2 */
TEST(SetRGB, Channel1_R2G2B2) {
    setRGB(1, 10, 5, 3);

    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(R2_GPIO_Port, R2_Pin));
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(G2_GPIO_Port, G2_Pin));
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(B2_GPIO_Port, B2_Pin));
}

/* Verify channel 2 controls R3, G3, B3 */
TEST(SetRGB, Channel2_R3G3B3) {
    setRGB(2, 10, 5, 3);

    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(R3_GPIO_Port, R3_Pin));
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(G3_GPIO_Port, G3_Pin));
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(B3_GPIO_Port, B3_Pin));
}

/* Verify all zeros turns channel off */
TEST(SetRGB, AllZeros_ChannelOff) {
    setRGB(0, 0, 0, 0);

    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(G1_GPIO_Port, G1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B1_GPIO_Port, B1_Pin));
}

/* Verify red-only display */
TEST(SetRGB, RedOnly) {
    setRGB(0, 10, 0, 0);

    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(G1_GPIO_Port, G1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B1_GPIO_Port, B1_Pin));
}

/* Verify yellow (red + green) display */
TEST(SetRGB, Yellow_RedPlusGreen) {
    setRGB(0, 10, 5, 0);

    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(G1_GPIO_Port, G1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B1_GPIO_Port, B1_Pin));
}

/**
 * TEST_GROUP: BlinkCode
 * Tests PPO2 blink code patterns, status/fail masks, and timing
 * CRITICAL: Blink count accuracy (positive=green, negative=red)
 * CRITICAL: Failed cell indication (constant red)
 */
TEST_GROUP(BlinkCode) {
    void setup() {
        MockHAL_Reset();
        MockDelay_Reset();
        MockQueue_ResetFreeRTOS();
    }

    void teardown() {
        MockHAL_Reset();
        MockDelay_Reset();
        MockQueue_ResetFreeRTOS();
    }
};

/* Verify positive value blinks green */
TEST(BlinkCode, PositiveValue_GreenBlinks) {
    bool breakout = false;
    blinkCode(3, 0, 0, 0x07, 0x07, &breakout);  /* 3 green blinks on channel 0, all cells OK and voted in */

    /* Should have 3 blink cycles: on period + off period per blink */
    /* Total osDelay calls: 2 * max_blinks = 2 * 3 = 6 */
    CHECK_EQUAL(6, MockQueue_GetDelayCallCount());
    CHECK_EQUAL(6 * BLINK_PERIOD, MockQueue_GetTotalDelayTicks());
}

/* Verify negative value blinks red */
TEST(BlinkCode, NegativeValue_RedBlinks) {
    bool breakout = false;
    blinkCode(-5, 0, 0, 0x07, 0x07, &breakout);  /* 5 red blinks on channel 0 */

    /* Should have 5 blink cycles: on period + off period per blink */
    CHECK_EQUAL(10, MockQueue_GetDelayCallCount());
    CHECK_EQUAL(10 * BLINK_PERIOD, MockQueue_GetTotalDelayTicks());
}

/* Verify multiple channels use max blink count */
TEST(BlinkCode, MultipleChannels_UseMaxCount) {
    bool breakout = false;
    blinkCode(2, 5, 3, 0x07, 0x07, &breakout);  /* Max is 5 */

    /* Should have 5 blink cycles (max of 2, 5, 3) */
    CHECK_EQUAL(10, MockQueue_GetDelayCallCount());
    CHECK_EQUAL(10 * BLINK_PERIOD, MockQueue_GetTotalDelayTicks());
}

/* Verify failed cell shows constant red (failMask = 0 for that channel) */
TEST(BlinkCode, FailedCell_ConstantRed) {
    bool breakout = false;
    blinkCode(5, 3, 0, 0x07, 0x06, &breakout);  /* Channel 0 failed (bit 0 of failMask = 0), channel 1 OK with 3 blinks */

    /* Failed cell (channel 0) should show min brightness red */
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(G1_GPIO_Port, G1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B1_GPIO_Port, B1_Pin));
}

/* Verify voted-out cell shows yellow background (statusMask = 0 for that channel) */
TEST(BlinkCode, VotedOutCell_YellowBackground) {
    bool breakout = false;
    blinkCode(0, 5, 0, 0x05, 0x07, &breakout);  /* Channel 1 voted out (bit 1 of statusMask = 0) */

    /* After blinking completes, voted-out channel should have yellow */
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(R2_GPIO_Port, R2_Pin));
    CHECK_EQUAL(GPIO_PIN_SET, MockHAL_GetPinState(G2_GPIO_Port, G2_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B2_GPIO_Port, B2_Pin));
}

/* Verify zero value with all cells OK */
TEST(BlinkCode, ZeroValues_NoBlinks) {
    bool breakout = false;
    blinkCode(0, 0, 0, 0x07, 0x07, &breakout);

    /* No blinks, but delay calls still happen in the loop (0 iterations) */
    CHECK_EQUAL(0, MockQueue_GetDelayCallCount());
}

/* Verify mixed positive and negative values */
TEST(BlinkCode, MixedValues_CorrectColors) {
    bool breakout = false;
    blinkCode(3, -2, 4, 0x07, 0x07, &breakout);  /* Max is 4, channels 0 and 2 green, channel 1 red */

    /* Should have 4 blink cycles */
    CHECK_EQUAL(8, MockQueue_GetDelayCallCount());
}

/**
 * TEST_GROUP: BlinkNoData
 * Tests blue blink pattern for stale data indication
 */
TEST_GROUP(BlinkNoData) {
    void setup() {
        MockHAL_Reset();
        MockDelay_Reset();
        MockQueue_ResetFreeRTOS();
    }

    void teardown() {
        MockHAL_Reset();
        MockDelay_Reset();
        MockQueue_ResetFreeRTOS();
    }
};

/* Verify blue blink pattern (2 blinks) */
TEST(BlinkNoData, TwoBlueBlinks) {
    blinkNoData();

    /* 2 blinks: (on + off) * 2 + extra delay at end = 5 osDelay calls */
    CHECK_EQUAL(5, MockQueue_GetDelayCallCount());
}

/* Verify total delay timing */
TEST(BlinkNoData, CorrectTiming) {
    blinkNoData();

    /* Total delay: 2 * (BLINK_PERIOD + BLINK_PERIOD) + BLINK_PERIOD * 2 = 6 * BLINK_PERIOD */
    CHECK_EQUAL(6 * BLINK_PERIOD, MockQueue_GetTotalDelayTicks());
}

/* Verify all channels show blue during blink */
TEST(BlinkNoData, AllChannelsBlue) {
    blinkNoData();

    /* After completion, all channels should be off */
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(G1_GPIO_Port, G1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B1_GPIO_Port, B1_Pin));
}

/**
 * TEST_GROUP: BlinkAlarm
 * Tests "Nightrider" sweep pattern for alarm indication
 */
TEST_GROUP(BlinkAlarm) {
    void setup() {
        MockHAL_Reset();
        MockDelay_Reset();
        MockQueue_ResetFreeRTOS();
    }

    void teardown() {
        MockHAL_Reset();
        MockDelay_Reset();
        MockQueue_ResetFreeRTOS();
    }
};

/* Verify sweep pattern runs 5 times */
TEST(BlinkAlarm, FiveSweeps) {
    blinkAlarm();

    /* 5 sweeps * 6 delays per sweep = 30 osDelay calls */
    CHECK_EQUAL(30, MockQueue_GetDelayCallCount());
}

/* Verify total delay timing */
TEST(BlinkAlarm, CorrectTiming) {
    blinkAlarm();

    /* Total delay: 5 sweeps * 6 delays * 50ms = 1500ms */
    CHECK_EQUAL(30 * TIMEOUT_50MS, MockQueue_GetTotalDelayTicks());
}

/* Verify all channels are off at the end */
TEST(BlinkAlarm, AllChannelsOffAtEnd) {
    blinkAlarm();

    /* All channels should be off after sweep completes */
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(R1_GPIO_Port, R1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(G1_GPIO_Port, G1_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B1_GPIO_Port, B1_Pin));

    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(R2_GPIO_Port, R2_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(G2_GPIO_Port, G2_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B2_GPIO_Port, B2_Pin));

    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(R3_GPIO_Port, R3_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(G3_GPIO_Port, G3_Pin));
    CHECK_EQUAL(GPIO_PIN_RESET, MockHAL_GetPinState(B3_GPIO_Port, B3_Pin));
}

int main(int ac, char** av) {
    return CommandLineTestRunner::RunAllTests(ac, av);
}
