#include "Transciever.h"
#include "string.h"
#include "cmsis_os.h"
#include "queue.h"
#include "main.h"
#include "../errors.h"
#include "../Hardware/printer.h"

#define BUS_NAME_LEN 8

#define MENU_ACK_LEN 6
#define MENU_LEN 8
#define MENU_FIELD_LEN 10
#define MENU_FIELD_END_LEN 4
#define MENU_SAVE_ACK_LEN 3
#define MENU_SAVE_FIELD_ACK_LEN 5

#define TX_WAIT_DELAY 10

extern CAN_HandleTypeDef hcan1;

#define CAN_QUEUE_LEN 10

static QueueHandle_t *getInboundQueue(void)
{
    static QueueHandle_t QInboundCAN = NULL;
    return &QInboundCAN;
}

static QueueHandle_t *getDataAvailQueue(void)
{
    static QueueHandle_t QDataAvail = NULL;
    return &QDataAvail;
}

void InitRXQueue(void)
{
    QueueHandle_t *inbound = getInboundQueue();
    QueueHandle_t *dataAvail = getDataAvailQueue();

    if ((NULL == *inbound) && (NULL == *dataAvail))
    {
        static StaticQueue_t QInboundCAN_QueueStruct = {0};
        static uint8_t QInboundCAN_Storage[CAN_QUEUE_LEN * sizeof(DiveCANMessage_t)];

        *inbound = xQueueCreateStatic(CAN_QUEUE_LEN, sizeof(DiveCANMessage_t), QInboundCAN_Storage, &QInboundCAN_QueueStruct);

        static StaticQueue_t QDataAvail_QueueStruct = {0};
        static uint8_t QDataAvail_Storage[sizeof(bool)];

        *dataAvail = xQueueCreateStatic(1, sizeof(bool), QDataAvail_Storage, &QDataAvail_QueueStruct);
    }
}

void BlockForCAN(void)
{
    QueueHandle_t *dataAvail = getDataAvailQueue();
    if (xQueueReset(*dataAvail)) /* reset always returns pdPASS, so this should always evaluate to true */
    {
        bool data = false;
        bool msgAvailable = xQueuePeek(*dataAvail, &data, TIMEOUT_1S_TICKS);

        if (!msgAvailable)
        {
            /** Data is not available, but the later code is able to handle that,
             * This method mainly exists to rest for a convenient, event-based amount
             */
            NON_FATAL_ERROR(TIMEOUT_ERR);
        }
    }
    else
    {
        NON_FATAL_ERROR(UNREACHABLE_ERR);
    }
}

BaseType_t GetLatestCAN(const Timestamp_t blockTime, DiveCANMessage_t *message)
{
    QueueHandle_t *inbound = getInboundQueue();
    return xQueueReceive(*inbound, message, blockTime);
}

/** @brief !! ISR METHOD !! Called when CAN mailbox receives message
 * @param id message extended ID
 * @param length length of data
 * @param data data pointer
 */
void rxInterrupt(const uint32_t id, const uint8_t length, const uint8_t *const data)
{
    DiveCANMessage_t message = {
        .id = id,
        .length = length,
        .data = {0, 0, 0, 0, 0, 0, 0, 0},
        .type = NULL};

    if (length > MAX_CAN_RX_LENGTH)
    {
        NON_FATAL_ERROR_ISR_DETAIL(CAN_OVERFLOW_ERR, length);
    }
    else
    {
        (void)memcpy(message.data, data, length);
    }

    QueueHandle_t *inbound = getInboundQueue();
    QueueHandle_t *dataAvail = getDataAvailQueue();
    if ((NULL != *inbound) && (NULL != *dataAvail))
    {
        bool dataReady = true;
        BaseType_t err = xQueueOverwriteFromISR(*dataAvail, &dataReady, NULL);
        if (pdPASS != err)
        {
            NON_FATAL_ERROR_ISR_DETAIL(QUEUEING_ERR, err);
        }

        err = xQueueSendToBackFromISR(*inbound, &message, NULL);
        if (pdPASS != err)
        {
            /* err can only ever be 0 if we get here, means we couldn't enqueue*/
            NON_FATAL_ERROR_ISR(QUEUEING_ERR);
        }
    }
}

/** @brief Add message to the next free mailbox, waits until the next mailbox is available.
 * @param Id Message ID (extended)
 * @param data Pointer to the data to send, must be size dataLength
 * @param dataLength Size of the data to send */
void sendCANMessage(const DiveCANMessage_t message)
{
    /* This isn't super time critical so if we're still waiting on stuff to tx then we can quite happily just wait */
    while (0 == HAL_CAN_GetTxMailboxesFreeLevel(&hcan1))
    {
        (void)osDelay(TX_WAIT_DELAY);
    }

    CAN_TxHeaderTypeDef header = {0};
    header.StdId = 0x0;
    header.ExtId = message.id;
    header.RTR = CAN_RTR_DATA;
    header.IDE = CAN_ID_EXT;
    header.DLC = message.length;
    header.TransmitGlobalTime = DISABLE;

    uint32_t mailboxNumber = 0;

    HAL_StatusTypeDef err = HAL_CAN_AddTxMessage(&hcan1, &header, message.data, &mailboxNumber);
    if (HAL_OK != err)
    {
        NON_FATAL_ERROR_DETAIL(CAN_TX_ERR, err);
    }
}

/*-----------------------------------------------------------------------------------*/
/* Device Metadata */

/** @brief Transmit the bus initialization message
 *@param deviceType the device type of this device
 */
void txStartDevice(const DiveCANType_t targetDeviceType, const DiveCANType_t deviceType)
{
    const DiveCANMessage_t message = {
        .id = BUS_INIT_ID | (deviceType << 8) | targetDeviceType,
        .data = {0x8a, 0xf3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .length = 3,
        .type = "BUS_INIT"};
    sendCANMessage(message);
}

/** @brief Transmit the id of this device
 *@param deviceType the device type of this device
 *@param manufacturerID Manufacturer ID
 *@param firmwareVersion Firmware version
 */
void txID(const DiveCANType_t deviceType, const DiveCANManufacturer_t manufacturerID, const uint8_t firmwareVersion)
{
    const DiveCANMessage_t message = {
        .id = BUS_ID_ID | deviceType,
        .data = {(uint8_t)manufacturerID, 0x00, firmwareVersion, 0x00, 0x00, 0x00, 0x00, 0x00},
        .length = 3,
        .type = "BUS_ID"};
    sendCANMessage(message);
}

/** @brief Transmit the name of this device
 *@param deviceType the device type of this device
 *@param name Name of this device (max 8 chars, excluding null terminator)
 */
void txName(const DiveCANType_t deviceType, const char *const name)
{
    uint8_t data[BUS_NAME_LEN + 1] = {0};
    if (NULL == name)
    {
        NON_FATAL_ERROR(NULL_PTR_ERR);
    }
    else
    {
        (void)strncpy((char *)data, name, BUS_NAME_LEN);
        DiveCANMessage_t message = {
            .id = BUS_NAME_ID | deviceType,
            .data = {0},
            .length = 8,
            .type = "BUS_NAME"};
        (void)memcpy(message.data, data, BUS_NAME_LEN);
        sendCANMessage(message);
    }
}
