#include "MockPower.h"
#include <cstring>
#include <cstdlib>

/* GPIO port instances */
static GPIO_TypeDef GPIOA_instance;
static GPIO_TypeDef GPIOB_instance;
static GPIO_TypeDef GPIOC_instance;
static GPIO_TypeDef GPIOD_instance;
static GPIO_TypeDef GPIOE_instance;
static GPIO_TypeDef GPIOF_instance;
static GPIO_TypeDef GPIOG_instance;
static GPIO_TypeDef GPIOH_instance;

GPIO_TypeDef* GPIOA = &GPIOA_instance;
GPIO_TypeDef* GPIOB = &GPIOB_instance;
GPIO_TypeDef* GPIOC = &GPIOC_instance;
GPIO_TypeDef* GPIOD = &GPIOD_instance;
GPIO_TypeDef* GPIOE = &GPIOE_instance;
GPIO_TypeDef* GPIOF = &GPIOF_instance;
GPIO_TypeDef* GPIOG = &GPIOG_instance;
GPIO_TypeDef* GPIOH = &GPIOH_instance;

/* CAN_EN is on GPIOC Pin 14 */
GPIO_TypeDef* CAN_EN_GPIO_Port = &GPIOC_instance;

/* CAN handle instance */
/* When testing pwr_management, MockCAN is not linked, so define hcan1 here */
/* Otherwise, hcan1 is defined in MockCAN.cpp */
#ifdef TESTING_PWR_MANAGEMENT
static CAN_TypeDef can1_instance = {0};
CAN_HandleTypeDef hcan1 = {&can1_instance, 0, 0};  /* Instance, State, ErrorCode */
#else
extern CAN_HandleTypeDef hcan1;  /* Defined in MockCAN.cpp */
#endif

/* Pull-up/down configuration tracking using simple arrays */
#define MAX_PIN_CONFIGS 32

struct PinConfig {
    uint32_t gpio;
    uint32_t pin;
    bool valid;
};

/* Mock state */
static bool pullUpDownConfigEnabled = false;
static bool standbyEntered = false;
static HAL_StatusTypeDef pullDownReturnValue = HAL_OK;
static HAL_StatusTypeDef pullUpReturnValue = HAL_OK;
static uint32_t pullDownCallCount = 0;
static uint32_t pullUpCallCount = 0;
static PinConfig pullDownPins[MAX_PIN_CONFIGS];
static PinConfig pullUpPins[MAX_PIN_CONFIGS];

/* GPIO Init tracking using simple array */
#define MAX_GPIO_INITS 32

struct GPIOInitRecord {
    GPIO_TypeDef *GPIOx;
    uint16_t pin;
    uint32_t mode;
    uint32_t pull;
    bool valid;
};

static GPIOInitRecord gpioInitCalls[MAX_GPIO_INITS];
static uint32_t gpioInitCount = 0;

/* GPIO Read Pin values using simple array */
#define MAX_GPIO_PINS 32

struct GPIOPinValue {
    GPIO_TypeDef *GPIOx;
    uint16_t pin;
    GPIO_PinState value;
    bool valid;
};

static GPIOPinValue gpioPinValues[MAX_GPIO_PINS];

extern "C" {

/* HAL Power API */
void HAL_PWREx_EnablePullUpPullDownConfig(void) {
    pullUpDownConfigEnabled = true;
}

HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber) {
    pullDownCallCount++;

    if (pullDownReturnValue == HAL_OK) {
        /* Find empty slot or existing entry */
        for (uint32_t i = 0; i < MAX_PIN_CONFIGS; i++) {
            if (!pullDownPins[i].valid ||
                (pullDownPins[i].gpio == GPIO && pullDownPins[i].pin == GPIONumber)) {
                pullDownPins[i].gpio = GPIO;
                pullDownPins[i].pin = GPIONumber;
                pullDownPins[i].valid = true;
                break;
            }
        }
    }

    return pullDownReturnValue;
}

HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber) {
    pullUpCallCount++;

    if (pullUpReturnValue == HAL_OK) {
        /* Find empty slot or existing entry */
        for (uint32_t i = 0; i < MAX_PIN_CONFIGS; i++) {
            if (!pullUpPins[i].valid ||
                (pullUpPins[i].gpio == GPIO && pullUpPins[i].pin == GPIONumber)) {
                pullUpPins[i].gpio = GPIO;
                pullUpPins[i].pin = GPIONumber;
                pullUpPins[i].valid = true;
                break;
            }
        }
    }

    return pullUpReturnValue;
}

void HAL_PWR_EnterSTANDBYMode(void) {
    standbyEntered = true;
    /* In real hardware, this would not return. For testing, we just set flag */
}

/* HAL GPIO API */
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) {
    if (GPIO_Init != nullptr && gpioInitCount < MAX_GPIO_INITS) {
        gpioInitCalls[gpioInitCount].GPIOx = GPIOx;
        gpioInitCalls[gpioInitCount].pin = GPIO_Init->Pin;
        gpioInitCalls[gpioInitCount].mode = GPIO_Init->Mode;
        gpioInitCalls[gpioInitCount].pull = GPIO_Init->Pull;
        gpioInitCalls[gpioInitCount].valid = true;
        gpioInitCount++;
    }
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    /* Search for pin value */
    for (uint32_t i = 0; i < MAX_GPIO_PINS; i++) {
        if (gpioPinValues[i].valid &&
            gpioPinValues[i].GPIOx == GPIOx &&
            gpioPinValues[i].pin == GPIO_Pin) {
            return gpioPinValues[i].value;
        }
    }

    /* Default to GPIO_PIN_RESET if not set */
    return GPIO_PIN_RESET;
}

/* Mock control functions */
void MockPower_Reset(void) {
    pullUpDownConfigEnabled = false;
    standbyEntered = false;
    pullDownReturnValue = HAL_OK;
    pullUpReturnValue = HAL_OK;
    pullDownCallCount = 0;
    pullUpCallCount = 0;
    gpioInitCount = 0;

    /* Clear all arrays */
    memset(pullDownPins, 0, sizeof(pullDownPins));
    memset(pullUpPins, 0, sizeof(pullUpPins));
    memset(gpioInitCalls, 0, sizeof(gpioInitCalls));
    memset(gpioPinValues, 0, sizeof(gpioPinValues));
}

void MockPower_SetPullDownBehavior(HAL_StatusTypeDef returnValue) {
    pullDownReturnValue = returnValue;
}

void MockPower_SetPullUpBehavior(HAL_StatusTypeDef returnValue) {
    pullUpReturnValue = returnValue;
}

/* Mock query functions */
bool MockPower_GetPullUpDownConfigEnabled(void) {
    return pullUpDownConfigEnabled;
}

bool MockPower_GetStandbyEntered(void) {
    return standbyEntered;
}

uint32_t MockPower_GetPullDownCallCount(void) {
    return pullDownCallCount;
}

uint32_t MockPower_GetPullUpCallCount(void) {
    return pullUpCallCount;
}

bool MockPower_IsPinPulledDown(uint32_t GPIO, uint32_t GPIONumber) {
    for (uint32_t i = 0; i < MAX_PIN_CONFIGS; i++) {
        if (pullDownPins[i].valid &&
            pullDownPins[i].gpio == GPIO &&
            pullDownPins[i].pin == GPIONumber) {
            return true;
        }
    }
    return false;
}

bool MockPower_IsPinPulledUp(uint32_t GPIO, uint32_t GPIONumber) {
    for (uint32_t i = 0; i < MAX_PIN_CONFIGS; i++) {
        if (pullUpPins[i].valid &&
            pullUpPins[i].gpio == GPIO &&
            pullUpPins[i].pin == GPIONumber) {
            return true;
        }
    }
    return false;
}

/* GPIO mock control */
void MockPower_SetPinReadValue(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState value) {
    /* Find existing entry or empty slot */
    for (uint32_t i = 0; i < MAX_GPIO_PINS; i++) {
        if (!gpioPinValues[i].valid ||
            (gpioPinValues[i].GPIOx == GPIOx && gpioPinValues[i].pin == GPIO_Pin)) {
            gpioPinValues[i].GPIOx = GPIOx;
            gpioPinValues[i].pin = GPIO_Pin;
            gpioPinValues[i].value = value;
            gpioPinValues[i].valid = true;
            break;
        }
    }
}

uint32_t MockPower_GetGPIOInitCallCount(void) {
    return gpioInitCount;
}

bool MockPower_VerifyGPIOInit(GPIO_TypeDef *GPIOx, uint16_t pin, uint32_t mode, uint32_t pull) {
    for (uint32_t i = 0; i < gpioInitCount; i++) {
        if (gpioInitCalls[i].valid &&
            gpioInitCalls[i].GPIOx == GPIOx &&
            gpioInitCalls[i].pin == pin &&
            gpioInitCalls[i].mode == mode &&
            gpioInitCalls[i].pull == pull) {
            return true;
        }
    }
    return false;
}

/* Power management API wrappers (from pwr_management.h) */
/* Only include these when NOT testing pwr_management.c itself */
#ifndef TESTING_PWR_MANAGEMENT

bool getBusStatus(void) {
    /* Default implementation: CAN_EN is on GPIOC Pin 14 */
    extern GPIO_TypeDef *CAN_EN_GPIO_Port;
    return HAL_GPIO_ReadPin(CAN_EN_GPIO_Port, GPIO_PIN_14) == GPIO_PIN_RESET;
}

void Shutdown(void) {
    /* Enable pull-up/down configuration */
    HAL_PWREx_EnablePullUpPullDownConfig();

    /* Pull down RGB LED pins on GPIO A */
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_1);
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_2);
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_3);
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_4);
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_6);
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_7);
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_8);
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_15);

    /* Pull down alert LEDs on GPIO B */
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_0);
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_1);
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_6);
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_7);

    /* Pull up CAN_EN on GPIO C */
    HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_C, PWR_GPIO_BIT_14);

    /* Pull down PWR_BUS on GPIO C */
    HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_15);

    /* Enter STANDBY mode */
    HAL_PWR_EnterSTANDBYMode();
}

#endif /* !TESTING_PWR_MANAGEMENT */

} /* extern "C" */
