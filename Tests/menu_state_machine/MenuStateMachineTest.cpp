#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
#include "../../Core/Src/menu_state_machine.h"
#include "../Mocks/MockHAL.h"
}

TEST_GROUP(MenuStateMachine)
{
    void setup()
    {
        MockHAL_Reset();
        resetMenuStateMachine();
    }

    void teardown()
    {
        MockHAL_Reset();
    }

    /* Helper to simulate a button press event */
    void simulatePress(uint32_t duration_ms)
    {
        onButtonPress();
        MockHAL_IncrementTick(duration_ms);
        menuStateMachineTick();
    }

    /* Helper to simulate a button release event */
    void simulateRelease()
    {
        onButtonRelease();
        menuStateMachineTick();
    }

    /* Helper to simulate a short press (between PRESS and HOLD threshold) */
    void simulateShortPress()
    {
        onButtonPress();
        MockHAL_IncrementTick(150); /* Between 100ms and 2000ms */
        menuStateMachineTick();
        simulateRelease();
    }

    /* Helper to simulate a hold (> 2000ms) */
    void simulateHold()
    {
        onButtonPress();
        MockHAL_IncrementTick(2001); /* Just above hold threshold (code uses >) */
        menuStateMachineTick();
        simulateRelease();
    }

    /* Helper to verify LED state */
    void verifyLEDState(GPIO_PinState led0, GPIO_PinState led1, GPIO_PinState led2, GPIO_PinState led3)
    {
        CHECK_EQUAL(led0, MockHAL_GetPinState(LED_0_GPIO_Port, LED_0_Pin));
        CHECK_EQUAL(led1, MockHAL_GetPinState(LED_1_GPIO_Port, LED_1_Pin));
        CHECK_EQUAL(led2, MockHAL_GetPinState(LED_2_GPIO_Port, LED_2_Pin));
        CHECK_EQUAL(led3, MockHAL_GetPinState(LED_3_GPIO_Port, LED_3_Pin));
    }

    /* Helper to advance time and tick without button interaction */
    void advanceTime(uint32_t ms)
    {
        MockHAL_IncrementTick(ms);
        menuStateMachineTick();
    }
};

/*
 * Test: InitialStateIsIdle
 * Setup: Fresh menu state machine after reset
 * Action: Call menuStateMachineTick() with no button input
 * Expected: menuActive() returns false, all LEDs off
 */
TEST(MenuStateMachine, InitialStateIsIdle)
{
    CHECK_FALSE(menuActive());
    menuStateMachineTick();
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

/*
 * Test: SingleShortPressEntersState1
 * Setup: Menu starts in idle state
 * Action: Simulate one short button press (>100ms, <2000ms)
 * Expected: Menu becomes active (state 1), LED_0 on, others off
 */
TEST(MenuStateMachine, SingleShortPressEntersState1)
{
    CHECK_FALSE(menuActive());

    simulateShortPress();

    CHECK_TRUE(menuActive());
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

/*
 * Test: TwoShortPressesEnterState2
 * Setup: Menu starts in idle state
 * Action: Simulate two short button presses in sequence
 * Expected: Menu active (state 2), LED_0 and LED_1 on, others off
 */
TEST(MenuStateMachine, TwoShortPressesEnterState2)
{
    simulateShortPress();
    simulateShortPress();

    CHECK_TRUE(menuActive());
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

/*
 * Test: ThreeShortPressesEnterState3
 * Setup: Menu starts in idle state
 * Action: Simulate three short button presses in sequence
 * Expected: Menu active (state 3), LED_0, LED_1, LED_2 on, LED_3 off
 */
TEST(MenuStateMachine, ThreeShortPressesEnterState3)
{
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();

    CHECK_TRUE(menuActive());
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET);
}

/*
 * Test: FourShortPressesEnterState4
 * Setup: Menu starts in idle state
 * Action: Simulate four short button presses in sequence
 * Expected: Menu active (state 4), all LEDs on (peak of count-up sequence)
 */
TEST(MenuStateMachine, FourShortPressesEnterState4)
{
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();

    CHECK_TRUE(menuActive());
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: FiveShortPressesEnterState5
 * Setup: Menu starts in idle state
 * Action: Simulate five short button presses in sequence
 * Expected: Menu active (state 5), LED_0 off, LED_1-3 on (count-down begins)
 */
TEST(MenuStateMachine, FiveShortPressesEnterState5)
{
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();

    CHECK_TRUE(menuActive());
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: SixShortPressesEnterState6
 * Setup: Menu starts in idle state
 * Action: Simulate six short button presses in sequence
 * Expected: Menu active (state 6), LED_0-1 off, LED_2-3 on
 */
TEST(MenuStateMachine, SixShortPressesEnterState6)
{
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();

    CHECK_TRUE(menuActive());
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: SevenShortPressesEnterState7
 * Setup: Menu starts in idle state
 * Action: Simulate seven short button presses in sequence
 * Expected: Menu active (state 7), only LED_3 on (end of count-down)
 */
TEST(MenuStateMachine, SevenShortPressesEnterState7)
{
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();

    CHECK_TRUE(menuActive());
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET);
}

/*
 * Test: EightShortPressesReturnToIdle
 * Setup: Menu starts in idle state
 * Action: Simulate eight short button presses (completing the cycle without hold)
 * Expected: Menu returns to idle (inactive), all LEDs off
 */
TEST(MenuStateMachine, EightShortPressesReturnToIdle)
{
    /* 8 presses without hold should return to idle */
    for (int i = 0; i < 8; i++) {
        simulateShortPress();
    }

    CHECK_FALSE(menuActive());
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

/*
 * Test: HoldOn4thPressEntersShutdown
 * Setup: Menu starts in idle state
 * Action: Three short presses, then hold button on 4th press (>2000ms)
 * Expected: Menu enters shutdown state (active), all LEDs on
 */
TEST(MenuStateMachine, HoldOn4thPressEntersShutdown)
{
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateHold();

    CHECK_TRUE(menuActive());
    /* Shutdown state lights all LEDs */
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: HoldOn8thPressEntersCalibration
 * Setup: Menu starts in idle state
 * Action: Seven short presses, then hold button on 8th press (>2000ms)
 * Expected: Menu enters calibration state (active)
 */
TEST(MenuStateMachine, HoldOn8thPressEntersCalibration)
{
    for (int i = 0; i < 7; i++) {
        simulateShortPress();
    }
    simulateHold();

    CHECK_TRUE(menuActive());
}

/*
 * Test: ShutdownStateIsPersistent
 * Setup: Menu starts in idle state
 * Action: Enter shutdown state (3 presses + hold), then attempt more button presses
 * Expected: Menu stays in shutdown state (active), all LEDs remain on (state is terminal)
 */
TEST(MenuStateMachine, ShutdownStateIsPersistent)
{
    /* Enter shutdown */
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateHold();

    /* Try to exit with more presses */
    simulateShortPress();
    simulateShortPress();

    /* Should still be in shutdown (menuActive) */
    CHECK_TRUE(menuActive());
    /* LEDs should still be all on */
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: CalibrationStateIsPersistent
 * Setup: Menu starts in idle state
 * Action: Enter calibration state (7 presses + hold), then attempt more button presses
 * Expected: Menu stays in calibration state (active), state is terminal
 */
TEST(MenuStateMachine, CalibrationStateIsPersistent)
{
    /* Enter calibration */
    for (int i = 0; i < 7; i++) {
        simulateShortPress();
    }
    simulateHold();

    /* Try to exit with more presses */
    simulateShortPress();
    simulateShortPress();

    /* Should still be in calibration (menuActive) */
    CHECK_TRUE(menuActive());
}

/*
 * Test: TimeoutResetsToIdle
 * Setup: Menu in state 1 (one short press)
 * Action: Wait >10 seconds without any button activity
 * Expected: Menu times out and returns to idle (inactive), all LEDs off
 */
TEST(MenuStateMachine, TimeoutResetsToIdle)
{
    /* Enter menu state */
    simulateShortPress();
    CHECK_TRUE(menuActive());

    /* Advance time beyond timeout (10 seconds) */
    advanceTime(10001);

    /* Should be back to idle */
    CHECK_FALSE(menuActive());
    /* Call tick once more to update LED display after timeout */
    menuStateMachineTick();
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

/*
 * Test: TimeoutDoesNotOccurWhileButtonPressed
 * Setup: Menu in state 1 (one short press)
 * Action: Press and hold button, then advance time >10 seconds while button is still pressed
 * Expected: Menu stays active (timeout is blocked while button_state != NONE)
 */
TEST(MenuStateMachine, TimeoutDoesNotOccurWhileButtonPressed)
{
    /* Enter menu state by doing full short press */
    simulateShortPress();

    /* Press and hold button */
    onButtonPress();

    /* Advance time beyond timeout while button is pressed */
    MockHAL_IncrementTick(10001);
    menuStateMachineTick();

    /* Should still be active because button is pressed (button_state != NONE prevents timeout) */
    CHECK_TRUE(menuActive());
}

/*
 * Test: TimeoutResetsFromAnyState
 * Setup: Menu in state 3 (three short presses)
 * Action: Wait >10 seconds without any button activity
 * Expected: Menu times out from state 3 and returns to idle, all LEDs off
 */
TEST(MenuStateMachine, TimeoutResetsFromAnyState)
{
    /* Go to state 3 */
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();

    CHECK_TRUE(menuActive());

    /* Wait for timeout */
    advanceTime(10001);

    CHECK_FALSE(menuActive());
    /* Call tick once more to update LED display after timeout */
    menuStateMachineTick();
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

/*
 * Test: ResetMenuStateMachineResetsToIdle
 * Setup: Menu in state 2 (two short presses)
 * Action: Call resetMenuStateMachine() function
 * Expected: Menu immediately returns to idle (inactive), all LEDs off
 */
TEST(MenuStateMachine, ResetMenuStateMachineResetsToIdle)
{
    /* Enter some state */
    simulateShortPress();
    simulateShortPress();
    CHECK_TRUE(menuActive());

    resetMenuStateMachine();

    CHECK_FALSE(menuActive());
    menuStateMachineTick();
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

/*
 * Test: ButtonPressDurationBelowThresholdDoesNotTrigger
 * Setup: Menu starts in idle state
 * Action: Press button for only 50ms (below 100ms threshold), then release
 * Expected: Menu stays idle (no state transition), button press not registered
 */
TEST(MenuStateMachine, ButtonPressDurationBelowThresholdDoesNotTrigger)
{
    /* Press button but release before threshold */
    onButtonPress();
    MockHAL_IncrementTick(50); /* Below 100ms threshold */
    menuStateMachineTick();
    simulateRelease();

    /* Should still be in idle */
    CHECK_FALSE(menuActive());
}

/*
 * Test: ButtonPressExactlyAtThresholdTriggersPress
 * Setup: Menu starts in idle state
 * Action: Press button for 101ms (just above 100ms threshold)
 * Expected: Menu enters state 1 (button press registered, code uses > not >=)
 */
TEST(MenuStateMachine, ButtonPressExactlyAtThresholdTriggersPress)
{
    onButtonPress();
    MockHAL_IncrementTick(101); /* Just above press threshold (code uses >) */
    menuStateMachineTick();

    /* Should have moved to state 1 */
    CHECK_TRUE(menuActive());
}

/*
 * Test: ButtonHoldExactlyAtThresholdTriggersHold
 * Setup: Menu in state 3 (three short presses)
 * Action: Press and hold button for 2001ms (just above 2000ms hold threshold)
 * Expected: Menu enters shutdown state, all LEDs on (code uses > not >=)
 */
TEST(MenuStateMachine, ButtonHoldExactlyAtThresholdTriggersHold)
{
    /* Get to state 3 */
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();

    /* Now hold on the 4th */
    onButtonPress();
    MockHAL_IncrementTick(2001); /* Just above hold threshold (code uses >) */
    menuStateMachineTick();

    /* Should be in shutdown */
    CHECK_TRUE(menuActive());
    /* Release and tick to update LED display */
    simulateRelease();
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: LEDPatternState1
 * Setup: Menu starts in idle state
 * Action: One short press to enter state 1
 * Expected: LED pattern shows state 1: LED_0=on, LED_1-3=off
 */
TEST(MenuStateMachine, LEDPatternState1)
{
    simulateShortPress();
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

/*
 * Test: LEDPatternState2
 * Setup: Menu starts in idle state
 * Action: Two short presses to enter state 2
 * Expected: LED pattern shows state 2: LED_0-1=on, LED_2-3=off
 */
TEST(MenuStateMachine, LEDPatternState2)
{
    simulateShortPress();
    simulateShortPress();
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

/*
 * Test: LEDPatternState3
 * Setup: Menu starts in idle state
 * Action: Three short presses to enter state 3
 * Expected: LED pattern shows state 3: LED_0-2=on, LED_3=off
 */
TEST(MenuStateMachine, LEDPatternState3)
{
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET);
}

/*
 * Test: LEDPatternState4
 * Setup: Menu starts in idle state
 * Action: Four short presses to enter state 4
 * Expected: LED pattern shows state 4: all LEDs on (peak of count-up)
 */
TEST(MenuStateMachine, LEDPatternState4)
{
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: LEDPatternState5
 * Setup: Menu starts in idle state
 * Action: Five short presses to enter state 5
 * Expected: LED pattern shows state 5: LED_0=off, LED_1-3=on (count-down begins)
 */
TEST(MenuStateMachine, LEDPatternState5)
{
    for (int i = 0; i < 5; i++) {
        simulateShortPress();
    }
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: LEDPatternState6
 * Setup: Menu starts in idle state
 * Action: Six short presses to enter state 6
 * Expected: LED pattern shows state 6: LED_0-1=off, LED_2-3=on
 */
TEST(MenuStateMachine, LEDPatternState6)
{
    for (int i = 0; i < 6; i++) {
        simulateShortPress();
    }
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: LEDPatternState7
 * Setup: Menu starts in idle state
 * Action: Seven short presses to enter state 7
 * Expected: LED pattern shows state 7: LED_0-2=off, LED_3=on (end of count-down)
 */
TEST(MenuStateMachine, LEDPatternState7)
{
    for (int i = 0; i < 7; i++) {
        simulateShortPress();
    }
    verifyLEDState(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET);
}

/*
 * Test: LEDPatternShutdown
 * Setup: Menu starts in idle state
 * Action: Enter shutdown state (3 short presses + hold)
 * Expected: LED pattern for shutdown: all LEDs on
 */
TEST(MenuStateMachine, LEDPatternShutdown)
{
    simulateShortPress();
    simulateShortPress();
    simulateShortPress();
    simulateHold();
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: MultipleButtonPressesInQuickSuccession
 * Setup: Menu starts in idle state
 * Action: Four rapid button presses (110ms press + 10ms gap between each)
 * Expected: Menu reaches state 4, all LEDs on (rapid input handling works correctly)
 */
TEST(MenuStateMachine, MultipleButtonPressesInQuickSuccession)
{
    /* Rapid fire presses */
    for (int i = 0; i < 4; i++) {
        onButtonPress();
        MockHAL_IncrementTick(110); /* Just above press threshold */
        menuStateMachineTick();
        simulateRelease();
        MockHAL_IncrementTick(10); /* Small delay between presses */
    }

    CHECK_TRUE(menuActive());
    verifyLEDState(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_SET);
}

/*
 * Test: TimeoutCountsFromFirstPress
 * Setup: Menu in state 1 (one short press)
 * Action: Wait 9s (still active), then wait another 1001ms (total >10s)
 * Expected: Timeout triggers after total of >10s from entering state, menu returns to idle
 */
TEST(MenuStateMachine, TimeoutCountsFromFirstPress)
{
    /* Enter state 1 */
    simulateShortPress();

    /* Wait 9 seconds */
    advanceTime(9000);

    /* Still active */
    CHECK_TRUE(menuActive());

    /* Wait another 1001ms to exceed 10s timeout from first press */
    advanceTime(1001);

    /* Should timeout now (>10s from first press, no state changes) */
    CHECK_FALSE(menuActive());
}

int main(int argc, char** argv)
{
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
