#include "main.h"
#include "DiveCAN/Transciever.h"
#include "DiveCAN/DiveCAN.h"
#include "Hardware/printer.h"

extern const uint8_t ADC1_ADDR;
extern const uint8_t ADC2_ADDR;

const uint8_t BOOTLOADER_MSG = 0x79;

void HAL_CAN_RxMsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef pRxHeader = {0};
    uint8_t pData[64] = {0};
    (void)HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &pRxHeader, pData);
    rxInterrupt(pRxHeader.ExtId, (uint8_t)pRxHeader.DLC, pData);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    HAL_CAN_RxMsgPendingCallback(hcan);
}

void HAL_CAN_RxFifo1MsgPendingCallbackxFifo1(CAN_HandleTypeDef *hcan)
{
    HAL_CAN_RxMsgPendingCallback(hcan);
}
