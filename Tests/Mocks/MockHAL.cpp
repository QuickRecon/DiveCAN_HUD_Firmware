#include "MockHAL.h"
#include <cstring>

/* GPIO port instances - shared across all compilation units */
GPIO_TypeDef LED_0_Port_Static;
GPIO_TypeDef LED_1_Port_Static;
GPIO_TypeDef LED_2_Port_Static;
GPIO_TypeDef LED_3_Port_Static;
GPIO_TypeDef R_Port_Static;
GPIO_TypeDef G_Port_Static;
GPIO_TypeDef B_Port_Static;
GPIO_TypeDef ASC_EN_Port_Static;

/* GPIO pin state tracking using simple array */
#define MAX_GPIO_PINS 64

struct GPIOPinStateRecord {
    GPIO_TypeDef* port;
    uint16_t pin;
    GPIO_PinState state;
    bool valid;
};

/* Mock state */
static uint32_t currentTick = 0;
static GPIOPinStateRecord pinStates[MAX_GPIO_PINS];

extern "C" {

/* Weak stub for inCalibration variable from menu_state_machine.c
 * This allows the real definition to override when menu_state_machine.o is linked */
bool inCalibration __attribute__((weak)) = false;

uint32_t HAL_GetTick(void) {
    return currentTick;
}

void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    /* Find existing entry or empty slot */
    for (uint32_t i = 0; i < MAX_GPIO_PINS; i++) {
        if (!pinStates[i].valid ||
            (pinStates[i].port == GPIOx && pinStates[i].pin == GPIO_Pin)) {
            pinStates[i].port = GPIOx;
            pinStates[i].pin = GPIO_Pin;
            pinStates[i].state = PinState;
            pinStates[i].valid = true;
            break;
        }
    }
}

void MockHAL_SetTick(uint32_t tick) {
    currentTick = tick;
}

void MockHAL_IncrementTick(uint32_t increment) {
    currentTick += increment;
}

void MockHAL_Reset(void) {
    currentTick = 1; /* Start at 1 to avoid buttonPressTimestamp == 0 issue */
    memset(pinStates, 0, sizeof(pinStates));
}

GPIO_PinState MockHAL_GetPinState(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    /* Search for pin state */
    for (uint32_t i = 0; i < MAX_GPIO_PINS; i++) {
        if (pinStates[i].valid &&
            pinStates[i].port == GPIOx &&
            pinStates[i].pin == GPIO_Pin) {
            return pinStates[i].state;
        }
    }
    return GPIO_PIN_RESET; /* Default to reset if not set */
}

} /* extern "C" */
