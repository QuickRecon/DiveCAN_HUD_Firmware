#include "DiveCAN.h"
#include <string.h>
#include "cmsis_os.h"
#include "../Hardware/pwr_management.h"
#include "../Hardware/printer.h"
#include "../errors.h"

void CANTask(void *arg);
void RespBusInit(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
void RespPing(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
void RespCal(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
void RespMenu(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
void RespSetpoint(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
void RespAtmos(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
void RespShutdown(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
void RespSerialNumber(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
void RespPPO2(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
void RespPPO2Status(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec);
void RespDiving(const DiveCANMessage_t *const message);
void updatePIDPGain(const DiveCANMessage_t *const message);
void updatePIDIGain(const DiveCANMessage_t *const message);
void updatePIDDGain(const DiveCANMessage_t *const message);

static const uint8_t DIVECAN_TYPE_MASK = 0xF;

extern osMessageQueueId_t PPO2QueueHandle;
extern osMessageQueueId_t CellStatQueueHandle;

/* FreeRTOS tasks */

static osThreadId_t *getOSThreadId(void)
{
    static osThreadId_t CANTaskHandle;
    return &CANTaskHandle;
}

typedef struct
{
    DiveCANDevice_t deviceSpec;
} DiveCANTask_params_t;

void InitDiveCAN(const DiveCANDevice_t *const deviceSpec)
{
    InitRXQueue();

    static uint8_t CANTask_buffer[CANTASK_STACK_SIZE];
    static StaticTask_t CANTask_ControlBlock;
    static DiveCANTask_params_t task_params;
    static const osThreadAttr_t CANTask_attributes = {
        .name = "CANTask",
        .attr_bits = osThreadDetached,
        .cb_mem = &CANTask_ControlBlock,
        .cb_size = sizeof(CANTask_ControlBlock),
        .stack_mem = &CANTask_buffer[0],
        .stack_size = sizeof(CANTask_buffer),
        .priority = CAN_RX_PRIORITY,
        .tz_module = 0,
        .reserved = 0};

    osThreadId_t *CANTaskHandle = getOSThreadId();
    task_params.deviceSpec = *deviceSpec;
    *CANTaskHandle = osThreadNew(CANTask, &task_params, &CANTask_attributes);
    txStartDevice(DIVECAN_MONITOR, DIVECAN_CONTROLLER);
}

/** @brief This task is the context in which we handle inbound CAN messages (which sometimes requires a response), dispatch of our other outgoing traffic may occur elsewhere
 * @param arg
 */
void CANTask(void *arg)
{
    DiveCANTask_params_t *task_params = (DiveCANTask_params_t *)arg;
    const DiveCANDevice_t *const deviceSpec = &(task_params->deviceSpec);

    while (true)
    {
        DiveCANMessage_t message = {0};
        if (pdTRUE == GetLatestCAN(TIMEOUT_1S_TICKS, &message))
        {
            uint32_t message_id = message.id & ID_MASK; /* Drop the source/dest stuff, we're listening for anything from anyone */
            switch (message_id)
            {
            case BUS_ID_ID:
                message.type = "BUS_ID";
                /* Respond to pings */
                RespPing(&message, deviceSpec);
                break;
            case BUS_NAME_ID:
                message.type = "BUS_NAME";
                break;
            case BUS_OFF_ID:
                message.type = "BUS_OFF";
                /* Turn off bus */
                RespShutdown(&message, deviceSpec);
                break;
            case PPO2_PPO2_ID:
                message.type = "PPO2_PPO2";
                RespPPO2(&message, deviceSpec);
                break;
            case HUD_STAT_ID:
                message.type = "HUD_STAT";
                break;
            case PPO2_ATMOS_ID:
                message.type = "PPO2_ATMOS";
                break;
            case MENU_ID:
                message.type = "MENU";
                break;
            case TANK_PRESSURE_ID:
                message.type = "TANK_PRESSURE";
                break;
            case PPO2_MILLIS_ID:
                message.type = "PPO2_MILLIS";
                break;
            case CAL_ID:
                message.type = "CAL";
                break;
            case CAL_REQ_ID:
                message.type = "CAL_REQ";
                break;
            case CO2_STATUS_ID:
                message.type = "CO2_STATUS";
                break;
            case CO2_ID:
                message.type = "CO2";
                break;
            case CO2_CAL_ID:
                message.type = "CO2_CAL";
                break;
            case CO2_CAL_REQ_ID:
                message.type = "CO2_CAL_REQ";
                break;
            case BUS_MENU_OPEN_ID:
                message.type = "BUS_MENU_OPEN";
                break;
            case BUS_INIT_ID:
                /* Bus Init */
                message.type = "BUS_INIT";
                break;
            case RMS_TEMP_ID:
                message.type = "RMS_TEMP";
                break;
            case RMS_TEMP_ENABLED_ID:
                message.type = "RMS_TEMP_ENABLED";
                break;
            case PPO2_SETPOINT_ID:
                message.type = "PPO2_SETPOINT";
                break;
            case PPO2_STATUS_ID:
                message.type = "PPO2_STATUS";
                RespPPO2Status(&message, deviceSpec);
                break;
            case BUS_STATUS_ID:
                message.type = "BUS_STATUS";
                break;
            case DIVING_ID:
                message.type = "DIVING";
                break;
            case CAN_SERIAL_NUMBER_ID:
                message.type = "CAN_SERIAL_NUMBER";
                RespSerialNumber(&message, deviceSpec);
                break;
            default:
                message.type = "UNKNOWN";
                serial_printf("Unknown message 0x%x: [0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x]\n\r", message_id,
                              message.data[0], message.data[1], message.data[2], message.data[3], message.data[4], message.data[5], message.data[6], message.data[7]);
            }
        }
        else
        {
            /* We didn't get a message, soldier forth */
        }
    }
}

void RespBusInit(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    /* Do startup stuff and then ping the bus */
    RespPing(message, deviceSpec);
}

void RespPing(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    DiveCANType_t devType = deviceSpec->type;
    /* We only want to reply to a ping from the head */
    if (((message->id & DIVECAN_TYPE_MASK) == DIVECAN_SOLO) || ((message->id & DIVECAN_TYPE_MASK) == DIVECAN_OBOE))
    {
        txID(devType, deviceSpec->manufacturerID, deviceSpec->firmwareVersion);
        txName(devType, deviceSpec->name);
    }
}

void RespPPO2(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    (void)deviceSpec; /* Unused, but we need to match the function signature */
    CellValues_t cell_values;

    cell_values.C1 = message->data[1];
    cell_values.C2 = message->data[2];
    cell_values.C3 = message->data[3];

    /* Send the values to the PPO2 processing queue */
    osMessageQueueReset(PPO2QueueHandle);
    osStatus_t enQueueStatus = osMessageQueuePut(PPO2QueueHandle, &cell_values, 0, 0);
    if (enQueueStatus != osOK)
    {
        NON_FATAL_ERROR_DETAIL(QUEUEING_ERR, enQueueStatus);
    }
}

void RespPPO2Status(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    (void)deviceSpec; /* Unused, but we need to match the function signature */
    uint8_t status = message->data[0];

    /* Send the status to the Cell Status processing queue */
    osMessageQueueReset(CellStatQueueHandle);
    osStatus_t enQueueStatus = osMessageQueuePut(CellStatQueueHandle, &status, 0, 0);
    if (enQueueStatus != osOK)
    {
        NON_FATAL_ERROR_DETAIL(QUEUEING_ERR, enQueueStatus);
    }
}

void RespShutdown(const DiveCANMessage_t *, const DiveCANDevice_t *)
{
    const uint8_t SHUTDOWN_ATTEMPTS = 20;
    for (uint8_t i = 0; i < SHUTDOWN_ATTEMPTS; ++i)
    {
        if (!getBusStatus())
        {
            serial_printf("Performing requested shutdown");
            Shutdown();
        }
        (void)osDelay(TIMEOUT_100MS_TICKS);
    }
    serial_printf("Shutdown attempted but timed out due to missing en signal");
}

void RespSerialNumber(const DiveCANMessage_t *const message, const DiveCANDevice_t *const deviceSpec)
{
    (void)deviceSpec; /* Unused, but we need to match the function signature */
    DiveCANType_t origin = (DiveCANType_t)(0xF & (message->id));
    char serial_number[sizeof(message->data) + 1] = {0};
    (void)memcpy(serial_number, message->data, sizeof(message->data));
    serial_printf("Received Serial Number of device %d: %s", origin, serial_number);
}
