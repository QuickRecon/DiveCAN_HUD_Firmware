#include "MockCAN.h"
#include <cstring>
#include <cstdlib>

/* CAN message storage using simple array */
#define MAX_CAN_MESSAGES 64

struct StoredCANMessage {
    uint32_t id;
    uint8_t length;
    uint8_t data[8];
    CAN_TxHeaderTypeDef header;
};

/* Mock state */
static HAL_StatusTypeDef txStatus = HAL_OK;
static uint32_t freeTxMailboxes = 3;  /* Default: all 3 mailboxes free */
static StoredCANMessage txMessages[MAX_CAN_MESSAGES];
static uint32_t txMessageCount = 0;
static uint32_t failOnCallNumber = 0;  /* 0 = don't fail */
static uint32_t txCallCount = 0;

/* CAN handle instance */
static CAN_TypeDef can1_instance;
CAN_HandleTypeDef hcan1 = {&can1_instance, 0, 0};

extern "C" {

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan,
                                       CAN_TxHeaderTypeDef *pHeader,
                                       const uint8_t aData[],
                                       uint32_t *pTxMailbox) {
    (void)hcan;

    txCallCount++;

    /* Check if we should fail on this call */
    if (failOnCallNumber > 0 && txCallCount == failOnCallNumber) {
        return HAL_ERROR;
    }

    /* Store the transmitted message */
    if (pHeader != nullptr && aData != nullptr && txStatus == HAL_OK && txMessageCount < MAX_CAN_MESSAGES) {
        StoredCANMessage *msg = &txMessages[txMessageCount];
        msg->header = *pHeader;

        /* Extract ID (prefer ExtId for DiveCAN) */
        if (pHeader->IDE == CAN_ID_EXT) {
            msg->id = pHeader->ExtId;
        } else {
            msg->id = pHeader->StdId;
        }

        msg->length = pHeader->DLC;

        /* Copy data (up to DLC bytes) */
        uint8_t copyLen = (msg->length > 8) ? 8 : msg->length;
        memcpy(msg->data, aData, copyLen);

        /* Fill remaining bytes with zero */
        for (uint8_t i = copyLen; i < 8; i++) {
            msg->data[i] = 0;
        }

        txMessageCount++;

        /* Set mailbox number if requested */
        if (pTxMailbox != nullptr) {
            *pTxMailbox = 0;  /* Always return mailbox 0 for simplicity */
        }
    }

    return txStatus;
}

uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *hcan) {
    (void)hcan;
    return freeTxMailboxes;
}

/* Mock control functions */
void MockCAN_Reset(void) {
    txStatus = HAL_OK;
    freeTxMailboxes = 3;
    txMessageCount = 0;
    failOnCallNumber = 0;
    txCallCount = 0;

    /* Clear message array */
    memset(txMessages, 0, sizeof(txMessages));
}

void MockCAN_SetTxBehavior(HAL_StatusTypeDef status, uint32_t freeMailboxes) {
    txStatus = status;
    freeTxMailboxes = freeMailboxes;
}

void MockCAN_SetTxFailOnCall(uint32_t failCallNumber_) {
    failOnCallNumber = failCallNumber_;
}

/* Mock query functions */
uint32_t MockCAN_GetTxMessageCount(void) {
    return txMessageCount;
}

bool MockCAN_GetLastTxMessage(uint32_t *id, uint8_t *length, uint8_t data[8]) {
    if (txMessageCount == 0) {
        return false;
    }

    const StoredCANMessage *msg = &txMessages[txMessageCount - 1];

    if (id != nullptr) {
        *id = msg->id;
    }

    if (length != nullptr) {
        *length = msg->length;
    }

    if (data != nullptr) {
        memcpy(data, msg->data, 8);
    }

    return true;
}

bool MockCAN_GetTxMessageAt(uint32_t index, uint32_t *id, uint8_t *length, uint8_t data[8]) {
    if (index >= txMessageCount) {
        return false;
    }

    const StoredCANMessage *msg = &txMessages[index];

    if (id != nullptr) {
        *id = msg->id;
    }

    if (length != nullptr) {
        *length = msg->length;
    }

    if (data != nullptr) {
        memcpy(data, msg->data, 8);
    }

    return true;
}

/* Verification helpers */
bool MockCAN_VerifyTxMessage(uint32_t index, uint32_t expectedId, uint8_t expectedLength) {
    if (index >= txMessageCount) {
        return false;
    }

    const StoredCANMessage *msg = &txMessages[index];
    return (msg->id == expectedId && msg->length == expectedLength);
}

bool MockCAN_VerifyTxData(uint32_t index, const uint8_t *expectedData, uint8_t length) {
    if (index >= txMessageCount || expectedData == nullptr) {
        return false;
    }

    const StoredCANMessage *msg = &txMessages[index];

    /* Verify length matches */
    if (length > msg->length) {
        return false;
    }

    /* Compare data bytes */
    return (memcmp(msg->data, expectedData, length) == 0);
}

} /* extern "C" */
