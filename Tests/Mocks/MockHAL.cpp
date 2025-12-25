#include "MockHAL.h"
#include <map>
#include <utility>

/* Mock GPIO port instances */
static GPIO_TypeDef LED_0_Port_Instance;
static GPIO_TypeDef LED_1_Port_Instance;
static GPIO_TypeDef LED_2_Port_Instance;
static GPIO_TypeDef LED_3_Port_Instance;

GPIO_TypeDef* LED_0_GPIO_Port = &LED_0_Port_Instance;
GPIO_TypeDef* LED_1_GPIO_Port = &LED_1_Port_Instance;
GPIO_TypeDef* LED_2_GPIO_Port = &LED_2_Port_Instance;
GPIO_TypeDef* LED_3_GPIO_Port = &LED_3_Port_Instance;

/* Mock state */
static uint32_t currentTick = 0;
static std::map<std::pair<GPIO_TypeDef*, uint16_t>, GPIO_PinState> pinStates;

extern "C" {

uint32_t HAL_GetTick(void) {
    return currentTick;
}

void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    pinStates[std::make_pair(GPIOx, GPIO_Pin)] = PinState;
}

void MockHAL_SetTick(uint32_t tick) {
    currentTick = tick;
}

void MockHAL_IncrementTick(uint32_t increment) {
    currentTick += increment;
}

void MockHAL_Reset(void) {
    currentTick = 1; /* Start at 1 to avoid buttonPressTimestamp == 0 issue */
    pinStates.clear();
}

GPIO_PinState MockHAL_GetPinState(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    auto it = pinStates.find(std::make_pair(GPIOx, GPIO_Pin));
    if (it != pinStates.end()) {
        return it->second;
    }
    return GPIO_PIN_RESET; /* Default to reset if not set */
}

} /* extern "C" */
