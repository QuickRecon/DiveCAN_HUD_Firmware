#include "pwr_management.h"
#include "stm32l4xx_hal_pwr_ex.h"
#include "../Hardware/printer.h"
#include "stm32l4xx_ll_rcc.h"
#include "flash.h"

extern IWDG_HandleTypeDef hiwdg;

extern CAN_HandleTypeDef hcan1;

/**
 * @brief Go to our lowest power mode that we can be woken from by the DiveCAN bus
 * @param config The device config, needed to ensure that we don't let the O2S cells go into analog standby mode, we can't get the input impedence above ~118kOhm and we need it above 150kOhm to get it to stay out of analog mode
 */
void Shutdown()
{
    /* We've been asked to shut down cleanly, reset the last fatal reason so we're not logging upsets when we start back up again*/
    bool writeErrOk = SetFatalError(NONE_FERR);
    if (!writeErrOk)
    {
        serial_printf("Failed to reset last fatal error on shutdown");
    }

    /* Pull what we can high to try and get the current consumption down */
    HAL_PWREx_EnablePullUpPullDownConfig();

    /* Silence the CAN transceiver */
    /* CAN_IO_PWR: GPIO C Pin 14*/
    /* CAN_SILENT: GPIO C Pin 15*/
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_14);
    (void)HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_C, PWR_GPIO_BIT_15);

    /* Shut Down VBUS */
    /* BUS_SEL1: GPIO A Pin 6*/
    /* BUS_SEL2: GPIO A Pin 5*/
    (void)HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_A, PWR_GPIO_BIT_6);
    (void)HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_A, PWR_GPIO_BIT_5);

    /* Disable solenoid */
    /* SOL_DIS_BATT: GPIO C Pin 11*/
    /* SOL_DIS_CAN: GPIO B Pin 4*/
    /* SOLENOID: GPIO C Pin 1*/
    (void)HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_C, PWR_GPIO_BIT_11);
    (void)HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_4);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_1);

    /* Pull the enable pin high for a safe high-idle*/
    /* CAN_EN: GPIO C Pin 13*/
    (void)HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_C, PWR_GPIO_BIT_13);

    /* Pull everything else down */
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_1);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_4);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_7);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_8);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_11);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_12);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_13);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_14);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_15);

    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_0);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_1);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_2);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_3);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_5);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_8);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_9);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_10);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_11);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_12);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_13);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_14);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_15);

    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_0);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_2);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_3);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_6);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_7);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_8);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_9);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_10);
    (void)HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_12);

    /* Disable IRQs */
    (void)__disable_irq();
    (void)__disable_fault_irq();

    /* Clear any pending wakeups */
    (void)__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF1);
    (void)__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF2);
    (void)__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF3);
    (void)__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF4);
    (void)__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF5);

    /* Set up the wakeup and shutdown*/
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2_LOW);
    HAL_PWREx_DisableInternalWakeUpLine();
    (void)__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWREx_EnterSHUTDOWNMode();
}

/**
 * @brief Check if the bus is active by first pulling up the pin to avoid being capacitively coupled to a false active, and then checking status, restores pin to no pull after check.
 * @return True if bus active, false otherwise
 */
bool testBusActive(void)
{
    /* Pull up the GPIO pin to avoid capacitance giving a false active*/
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = CAN_EN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    bool busActive = HAL_GPIO_ReadPin(CAN_EN_GPIO_Port, CAN_EN_Pin) == GPIO_PIN_RESET;

    /* Return to no pull to save power */
    GPIO_InitStruct.Pin = CAN_EN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(CAN_EN_GPIO_Port, &GPIO_InitStruct);
    return busActive;
}

/**
 * @brief Get the current bus status
 * @return True if bus on, false if bus off
 */
bool getBusStatus(void)
{
    /* Pin low means bus on, so return true*/
    return HAL_GPIO_ReadPin(CAN_EN_GPIO_Port, CAN_EN_Pin) == GPIO_PIN_RESET;
}
