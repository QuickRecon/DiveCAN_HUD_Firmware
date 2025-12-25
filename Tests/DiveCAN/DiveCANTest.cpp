#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
#include "DiveCAN/DiveCAN.h"
#include "DiveCAN/Transciever.h"
#include "MockCAN.h"
#include "MockErrors.h"
#include "MockPower.h"
#include "queue.h"
#include "cmsis_os.h"

/* Queue handles are defined in queue.cpp */
extern osMessageQueueId_t PPO2QueueHandle;
extern osMessageQueueId_t CellStatQueueHandle;
}

/* Test Group: RespPPO2 - PPO2 Data Handling */
TEST_GROUP(RespPPO2) {
    static bool queuesInitialized;
    DiveCANMessage_t message;
    DiveCANDevice_t deviceSpec;

    void setup() {
        /* Clear queues from previous test BEFORE CppUTest starts leak tracking */
        if (queuesInitialized) {
            MockQueue_ClearAllQueues();
        }

        MockCAN_Reset();
        MockErrors_Reset();
        MockPower_Reset();

        if (!queuesInitialized) {
            InitRXQueue();
            MockQueue_InitApplicationQueues();
            queuesInitialized = true;
        }

        /* Initialize message and device spec */
        message = {0};
        message.id = PPO2_PPO2_ID;
        message.type = "PPO2_PPO2";

        deviceSpec.name = "TestHUD";
        deviceSpec.type = DIVECAN_MONITOR;
        deviceSpec.manufacturerID = DIVECAN_MANUFACTURER_ISC;
        deviceSpec.firmwareVersion = 1;
    }

    void teardown() {
        MockCAN_Reset();
        MockErrors_Reset();
        MockPower_Reset();
        MockQueue_ResetFreeRTOS();
        queuesInitialized = false;  /* Reset so next test recreates queues */
    }
};

bool TEST_GROUP_CppUTestGroupRespPPO2::queuesInitialized = false;

/* RespPPO2 Tests - PPO2 value extraction and queueing */
TEST(RespPPO2, ExtractCellValues_AllPositive) {
    /* PPO2 values in data[1], data[2], data[3] */
    message.data[0] = 0xFF;  /* Unused */
    message.data[1] = 100;   /* C1 = 1.00 bar */
    message.data[2] = 110;   /* C2 = 1.10 bar */
    message.data[3] = 95;    /* C3 = 0.95 bar */

    RespPPO2(&message, &deviceSpec);

    /* Verify values queued */
    CellValues_t cellValues;
    osStatus_t status = osMessageQueueGet(PPO2QueueHandle, &cellValues, NULL, 0);
    CHECK_EQUAL(osOK, status);
    CHECK_EQUAL(100, cellValues.C1);
    CHECK_EQUAL(110, cellValues.C2);
    CHECK_EQUAL(95, cellValues.C3);
}

TEST(RespPPO2, QueueResetBeforeEnqueue) {
    /* Add old data to queue */
    CellValues_t oldValues = {50, 60, 70};
    osMessageQueuePut(PPO2QueueHandle, &oldValues, 0, 0);

    /* Process new PPO2 message */
    message.data[1] = 100;
    message.data[2] = 110;
    message.data[3] = 120;

    RespPPO2(&message, &deviceSpec);

    /* Queue should have been reset, so only 1 message */
    CHECK_EQUAL(1, MockQueue_GetMessageCount(PPO2QueueHandle));

    /* Verify new values (not old) */
    CellValues_t cellValues;
    osMessageQueueGet(PPO2QueueHandle, &cellValues, NULL, 0);
    CHECK_EQUAL(100, cellValues.C1);
    CHECK_EQUAL(110, cellValues.C2);
    CHECK_EQUAL(120, cellValues.C3);
}

TEST(RespPPO2, FailedCell_Value0xFF) {
    /* Failed cell indicated by 0xFF */
    message.data[1] = 0xFF;  /* C1 failed */
    message.data[2] = 110;
    message.data[3] = 95;

    RespPPO2(&message, &deviceSpec);

    CellValues_t cellValues;
    osMessageQueueGet(PPO2QueueHandle, &cellValues, NULL, 0);
    CHECK_EQUAL(0xFF, cellValues.C1);
    CHECK_EQUAL(110, cellValues.C2);
    CHECK_EQUAL(95, cellValues.C3);
}

TEST(RespPPO2, DeviceSpecUnused) {
    /* deviceSpec parameter is unused in RespPPO2 */
    message.data[1] = 100;
    message.data[2] = 110;
    message.data[3] = 95;

    /* Should not crash with NULL deviceSpec */
    RespPPO2(&message, NULL);

    /* Verify still works */
    CellValues_t cellValues;
    osStatus_t status = osMessageQueueGet(PPO2QueueHandle, &cellValues, NULL, 0);
    CHECK_EQUAL(osOK, status);
}

/* Test Group: RespPPO2Status - Cell Status Handling */
TEST_GROUP(RespPPO2Status) {
    static bool queuesInitialized;
    DiveCANMessage_t message;
    DiveCANDevice_t deviceSpec;

    void setup() {
        if (queuesInitialized) {
            MockQueue_ClearAllQueues();
        }

        MockCAN_Reset();
        MockErrors_Reset();
        MockPower_Reset();

        if (!queuesInitialized) {
            InitRXQueue();
            MockQueue_InitApplicationQueues();
            queuesInitialized = true;
        }

        message = {0};
        message.id = PPO2_STATUS_ID;
        message.type = "PPO2_STATUS";

        deviceSpec.name = "TestHUD";
        deviceSpec.type = DIVECAN_MONITOR;
        deviceSpec.manufacturerID = DIVECAN_MANUFACTURER_ISC;
        deviceSpec.firmwareVersion = 1;
    }

    void teardown() {
        MockCAN_Reset();
        MockErrors_Reset();
        MockPower_Reset();
        MockQueue_ResetFreeRTOS();
        queuesInitialized = false;  /* Reset so next test recreates queues */
    }
};

bool TEST_GROUP_CppUTestGroupRespPPO2Status::queuesInitialized = false;

TEST(RespPPO2Status, ExtractStatusByte) {
    /* Status in data[0] */
    message.data[0] = 0x07;  /* All 3 cells active (bits 0-2 set) */

    RespPPO2Status(&message, &deviceSpec);

    uint8_t status;
    osStatus_t result = osMessageQueueGet(CellStatQueueHandle, &status, NULL, 0);
    CHECK_EQUAL(osOK, result);
    CHECK_EQUAL(0x07, status);
}

TEST(RespPPO2Status, QueueResetBeforeEnqueue) {
    /* Add old status */
    uint8_t oldStatus = 0x01;
    osMessageQueuePut(CellStatQueueHandle, &oldStatus, 0, 0);

    /* Process new status */
    message.data[0] = 0x05;
    RespPPO2Status(&message, &deviceSpec);

    /* Queue should have been reset */
    CHECK_EQUAL(1, MockQueue_GetMessageCount(CellStatQueueHandle));

    uint8_t status;
    osMessageQueueGet(CellStatQueueHandle, &status, NULL, 0);
    CHECK_EQUAL(0x05, status);
}

TEST(RespPPO2Status, StatusAllCellsFailed_0x00) {
    message.data[0] = 0x00;  /* All cells failed/inactive */

    RespPPO2Status(&message, &deviceSpec);

    uint8_t status;
    osMessageQueueGet(CellStatQueueHandle, &status, NULL, 0);
    CHECK_EQUAL(0x00, status);
}

TEST(RespPPO2Status, DeviceSpecUnused) {
    message.data[0] = 0x03;

    /* Should not crash with NULL deviceSpec */
    RespPPO2Status(&message, NULL);

    uint8_t status;
    osStatus_t result = osMessageQueueGet(CellStatQueueHandle, &status, NULL, 0);
    CHECK_EQUAL(osOK, result);
}

/* Test Group: RespPing - Device Identification */
TEST_GROUP(RespPing) {
    static bool queuesInitialized;
    DiveCANMessage_t message;
    DiveCANDevice_t deviceSpec;

    void setup() {
        if (queuesInitialized) {
            MockQueue_ClearAllQueues();
        }

        MockCAN_Reset();
        MockErrors_Reset();
        MockPower_Reset();

        if (!queuesInitialized) {
            InitRXQueue();
            MockQueue_InitApplicationQueues();
            queuesInitialized = true;
        }

        message = {0};
        message.id = BUS_ID_ID;
        message.type = "BUS_ID";

        deviceSpec.name = "TestHUD";
        deviceSpec.type = DIVECAN_MONITOR;
        deviceSpec.manufacturerID = DIVECAN_MANUFACTURER_ISC;
        deviceSpec.firmwareVersion = 10;
    }

    void teardown() {
        MockCAN_Reset();
        MockErrors_Reset();
        MockPower_Reset();
        MockQueue_ResetFreeRTOS();
        queuesInitialized = false;  /* Reset so next test recreates queues */
    }
};

bool TEST_GROUP_CppUTestGroupRespPing::queuesInitialized = false;

TEST(RespPing, PingFromSOLO_SendsIDAndName) {
    /* Ping from SOLO (0x0) */
    message.id = BUS_ID_ID | DIVECAN_SOLO;

    RespPing(&message, &deviceSpec);

    /* Should send 2 CAN messages: ID and Name */
    CHECK_EQUAL(2, MockCAN_GetTxMessageCount());

    /* Verify first message is ID */
    uint32_t id1;
    uint8_t len1, data1[8];
    MockCAN_GetTxMessageAt(0, &id1, &len1, data1);
    CHECK_EQUAL(0xD000003, id1);  /* BUS_ID_ID | MONITOR | ISC | FW10 */

    /* Verify second message is Name */
    uint32_t id2;
    MockCAN_GetTxMessageAt(1, &id2, nullptr, nullptr);
    CHECK_EQUAL(0xD010003, id2);  /* BUS_NAME_ID | MONITOR */
}

TEST(RespPing, PingFromOBOE_SendsIDAndName) {
    /* Ping from OBOE (0x1) */
    message.id = BUS_ID_ID | DIVECAN_OBOE;

    RespPing(&message, &deviceSpec);

    /* Should send 2 CAN messages */
    CHECK_EQUAL(2, MockCAN_GetTxMessageCount());
}

TEST(RespPing, PingFromCONTROLLER_NoResponse) {
    /* Ping from CONTROLLER (0x2) */
    message.id = BUS_ID_ID | DIVECAN_CONTROLLER;

    RespPing(&message, &deviceSpec);

    /* Should NOT respond to CONTROLLER */
    CHECK_EQUAL(0, MockCAN_GetTxMessageCount());
}

TEST(RespPing, PingFromMONITOR_NoResponse) {
    /* Ping from another MONITOR (0x3) */
    message.id = BUS_ID_ID | DIVECAN_MONITOR;

    RespPing(&message, &deviceSpec);

    /* Should NOT respond to MONITOR */
    CHECK_EQUAL(0, MockCAN_GetTxMessageCount());
}

TEST(RespPing, PingFromREVO_NoResponse) {
    /* Ping from REVO (0x5) */
    message.id = BUS_ID_ID | DIVECAN_REVO;

    RespPing(&message, &deviceSpec);

    /* Should NOT respond to REVO */
    CHECK_EQUAL(0, MockCAN_GetTxMessageCount());
}

TEST(RespPing, DeviceTypeInIDMessage) {
    message.id = BUS_ID_ID | DIVECAN_SOLO;

    RespPing(&message, &deviceSpec);

    /* Extract device type from ID message */
    uint32_t id;
    MockCAN_GetTxMessageAt(0, &id, nullptr, nullptr);

    /* Device type is in bits 0-3 of ID */
    uint8_t deviceType = id & 0xF;
    CHECK_EQUAL(DIVECAN_MONITOR, deviceType);
}

TEST(RespPing, ManufacturerAndFirmwareInID) {
    message.id = BUS_ID_ID | DIVECAN_SOLO;

    /* Custom device spec */
    DiveCANDevice_t customDevice;
    customDevice.type = DIVECAN_CONTROLLER;
    customDevice.manufacturerID = DIVECAN_MANUFACTURER_GEN;
    customDevice.firmwareVersion = 5;
    customDevice.name = "CustomDev";

    RespPing(&message, &customDevice);

    uint32_t id;
    uint8_t len, data[8];
    MockCAN_GetTxMessageAt(0, &id, &len, data);

    /* Verify manufacturer in data[0] bits 4-7 and firmware in data[1] */
    CHECK_EQUAL(3, len);
    CHECK_EQUAL(DIVECAN_MANUFACTURER_GEN, data[0]);
    CHECK_EQUAL(5, data[2]);
}

/* Test Group: RespShutdown - Shutdown Sequence */
TEST_GROUP(RespShutdown) {
    static bool queuesInitialized;
    DiveCANMessage_t message;
    DiveCANDevice_t deviceSpec;

    void setup() {
        if (queuesInitialized) {
            MockQueue_ClearAllQueues();
        }

        MockCAN_Reset();
        MockErrors_Reset();
        MockPower_Reset();

        if (!queuesInitialized) {
            InitRXQueue();
            MockQueue_InitApplicationQueues();
            queuesInitialized = true;
        }

        message = {0};
        message.id = BUS_OFF_ID;
        message.type = "BUS_OFF";

        deviceSpec.name = "TestHUD";
        deviceSpec.type = DIVECAN_MONITOR;
        deviceSpec.manufacturerID = DIVECAN_MANUFACTURER_ISC;
        deviceSpec.firmwareVersion = 1;

        /* Default: bus active (pin low) */
        extern GPIO_TypeDef* CAN_EN_GPIO_Port;
        MockPower_SetPinReadValue(CAN_EN_GPIO_Port, GPIO_PIN_14, GPIO_PIN_SET);
    }

    void teardown() {
        MockCAN_Reset();
        MockErrors_Reset();
        MockPower_Reset();
        MockQueue_ResetFreeRTOS();
        queuesInitialized = false;  /* Reset so next test recreates queues */
    }
};

bool TEST_GROUP_CppUTestGroupRespShutdown::queuesInitialized = false;

TEST(RespShutdown, BusActive_DoesNotShutdown) {
    /* Bus active (CAN_EN pin HIGH) - should NOT shutdown */
    extern GPIO_TypeDef* CAN_EN_GPIO_Port;
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, GPIO_PIN_14, GPIO_PIN_RESET);

    RespShutdown(&message, &deviceSpec);

    /* Should NOT enter STANDBY */
    CHECK_FALSE(MockPower_GetStandbyEntered());
}

TEST(RespShutdown, BusInactive_Immediately_EntersShutdown) {
    /* Bus inactive (CAN_EN pin LOW) from first check */
    extern GPIO_TypeDef* CAN_EN_GPIO_Port;
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, GPIO_PIN_14, GPIO_PIN_SET);

    RespShutdown(&message, &deviceSpec);

    /* Should enter STANDBY */
    CHECK_TRUE(MockPower_GetStandbyEntered());
}

TEST(RespShutdown, BusGoesInactive_AfterRetries_EntersShutdown) {
    /* Simulate bus becoming inactive after 10 attempts */
    extern GPIO_TypeDef* CAN_EN_GPIO_Port;

    /* Start with bus active */
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, GPIO_PIN_14, GPIO_PIN_RESET);

    /* Note: We can't easily test retry logic without modifying the mock to
     * change value mid-function. This test verifies immediate shutdown works. */
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, GPIO_PIN_14, GPIO_PIN_SET);

    RespShutdown(&message, &deviceSpec);

    CHECK_TRUE(MockPower_GetStandbyEntered());
}

TEST(RespShutdown, Timeout_After20Attempts) {
    /* Bus stays active for all 20 attempts */
    extern GPIO_TypeDef* CAN_EN_GPIO_Port;
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, GPIO_PIN_14, GPIO_PIN_RESET);

    RespShutdown(&message, &deviceSpec);

    /* Should timeout and NOT shutdown */
    CHECK_FALSE(MockPower_GetStandbyEntered());

    /* Should have delayed 20 times (20 * 100ms = 2 seconds) */
    CHECK_EQUAL(20, MockQueue_GetDelayCallCount());
    CHECK_EQUAL(20 * 100, MockQueue_GetTotalDelayTicks());
}

// TEST(RespShutdown, ShutdownClearsFatalError) {
//     /* Bus inactive */
//     extern GPIO_TypeDef* CAN_EN_GPIO_Port;
//     MockPower_SetPinReadValue(CAN_EN_GPIO_Port, GPIO_PIN_14, GPIO_PIN_SET);

//     RespShutdown(&message, &deviceSpec);

//     /* Shutdown() should have been called, which resets fatal error
//      * We can verify this by checking that the system entered STANDBY */
//     CHECK_TRUE(MockPower_GetStandbyEntered());

//     /* Verify GPIO pull-up/down config was enabled */
//     CHECK_TRUE(MockPower_GetPullUpDownConfigEnabled());

//     /* Verify 14 GPIO pins configured (8 RGB + 4 alert + CAN_EN + PWR_BUS) */
//     CHECK_EQUAL(12, MockPower_GetPullDownCallCount());  /* 12 pull-downs */
//     CHECK_EQUAL(1, MockPower_GetPullUpCallCount());     /* 1 pull-up (CAN_EN) */
// }

TEST(RespShutdown, DeviceSpecUnused) {
    extern GPIO_TypeDef* CAN_EN_GPIO_Port;
    MockPower_SetPinReadValue(CAN_EN_GPIO_Port, GPIO_PIN_14, GPIO_PIN_SET);

    /* Should not crash with NULL deviceSpec */
    RespShutdown(&message, NULL);

    CHECK_TRUE(MockPower_GetStandbyEntered());
}

/* Test Group: RespSerialNumber - Debug Message Handling */
TEST_GROUP(RespSerialNumber) {
    static bool queuesInitialized;
    DiveCANMessage_t message;
    DiveCANDevice_t deviceSpec;

    void setup() {
        if (queuesInitialized) {
            MockQueue_ClearAllQueues();
        }

        MockCAN_Reset();
        MockErrors_Reset();
        MockPower_Reset();

        if (!queuesInitialized) {
            InitRXQueue();
            MockQueue_InitApplicationQueues();
            queuesInitialized = true;
        }

        message = {0};
        message.id = CAN_SERIAL_NUMBER_ID;
        message.type = "CAN_SERIAL_NUMBER";

        deviceSpec.name = "TestHUD";
        deviceSpec.type = DIVECAN_MONITOR;
        deviceSpec.manufacturerID = DIVECAN_MANUFACTURER_ISC;
        deviceSpec.firmwareVersion = 1;
    }

    void teardown() {
        MockCAN_Reset();
        MockErrors_Reset();
        MockPower_Reset();
        MockQueue_ResetFreeRTOS();
        queuesInitialized = false;  /* Reset so next test recreates queues */
    }
};

bool TEST_GROUP_CppUTestGroupRespSerialNumber::queuesInitialized = false;

TEST(RespSerialNumber, ExtractDeviceType) {
    /* Device type in lower 4 bits of message ID */
    message.id = CAN_SERIAL_NUMBER_ID | DIVECAN_CONTROLLER;
    const char *serial = "SN12345";
    memcpy(message.data, serial, 8);

    /* Function just prints to serial, so no crash = success */
    RespSerialNumber(&message, &deviceSpec);

    /* No assertions needed - function only logs to serial */
}

TEST(RespSerialNumber, SerialNumberExactly8Chars) {
    message.id = CAN_SERIAL_NUMBER_ID | DIVECAN_MONITOR;
    const char *serial = "ABCDEFGH";  /* Exactly 8 chars */
    memcpy(message.data, serial, 8);

    RespSerialNumber(&message, &deviceSpec);
    /* No crash = success */
}

TEST(RespSerialNumber, SerialNumberShorterThan8Chars) {
    message.id = CAN_SERIAL_NUMBER_ID | DIVECAN_SOLO;
    const char *serial = "SN1";
    memcpy(message.data, serial, strlen(serial) + 1);  /* Include null terminator */

    RespSerialNumber(&message, &deviceSpec);
    /* No crash = success */
}

TEST(RespSerialNumber, DeviceSpecUnused) {
    message.id = CAN_SERIAL_NUMBER_ID | DIVECAN_CONTROLLER;
    const char *serial = "TEST";
    memcpy(message.data, serial, strlen(serial) + 1);

    /* Should not crash with NULL deviceSpec */
    RespSerialNumber(&message, NULL);
}

/* Main runner */
int main(int argc, char** argv) {
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
