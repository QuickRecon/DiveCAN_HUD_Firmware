#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declare HAL status type */
#ifndef HAL_STATUSDEF
#define HAL_STATUSDEF
typedef enum
{
    HAL_OK = 0x00,
    HAL_ERROR = 0x01,
    HAL_BUSY = 0x02,
    HAL_TIMEOUT = 0x03
} HAL_StatusTypeDef;
#endif

/* CAN identifier type */
#define CAN_ID_STD 0x00000000U  /* Standard Id */
#define CAN_ID_EXT 0x00000004U  /* Extended Id */

/* CAN remote transmission request */
#define CAN_RTR_DATA   0x00000000U  /* Data frame */
#define CAN_RTR_REMOTE 0x00000002U  /* Remote frame */

/* Functional state */
typedef enum
{
    DISABLE = 0,
    ENABLE = !DISABLE
} FunctionalState;

/* CAN TypeDef (opaque pointer in tests) */
#ifndef CAN_TYPEDEF
#define CAN_TYPEDEF
typedef struct {
    uint32_t dummy;
} CAN_TypeDef;
#endif

/* CAN Tx Header structure */
typedef struct
{
    uint32_t StdId;  /* Standard identifier (0-0x7FF) */
    uint32_t ExtId;  /* Extended identifier (0-0x1FFFFFFF) */
    uint32_t IDE;    /* Identifier type (CAN_ID_STD or CAN_ID_EXT) */
    uint32_t RTR;    /* Frame type (CAN_RTR_DATA or CAN_RTR_REMOTE) */
    uint32_t DLC;    /* Data length code (0-8) */
    FunctionalState TransmitGlobalTime;  /* Timestamp transmission */
} CAN_TxHeaderTypeDef;

/* CAN Rx Header structure */
typedef struct
{
    uint32_t StdId;   /* Standard identifier */
    uint32_t ExtId;   /* Extended identifier */
    uint32_t IDE;     /* Identifier type */
    uint32_t RTR;     /* Frame type */
    uint32_t DLC;     /* Data length code */
    uint32_t Timestamp;        /* Timestamp counter value */
    uint32_t FilterMatchIndex; /* Filter match index */
} CAN_RxHeaderTypeDef;

/* CAN Handle structure (simplified for mocking) */
#ifndef CAN_HANDLETYPEDEF
#define CAN_HANDLETYPEDEF
typedef struct
{
    CAN_TypeDef *Instance;
    uint32_t State;
    uint32_t ErrorCode;
} CAN_HandleTypeDef;
#endif

/* CAN handle instance */
extern CAN_HandleTypeDef hcan1;

/* HAL CAN API functions to be mocked */
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan,
                                       CAN_TxHeaderTypeDef *pHeader,
                                       const uint8_t aData[],
                                       uint32_t *pTxMailbox);
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *hcan);

/* Mock control functions */
void MockCAN_Reset(void);
void MockCAN_SetTxBehavior(HAL_StatusTypeDef status, uint32_t freeMailboxes);
void MockCAN_SetTxFailOnCall(uint32_t failCallNumber);

/* Mock query functions */
uint32_t MockCAN_GetTxMessageCount(void);
bool MockCAN_GetLastTxMessage(uint32_t *id, uint8_t *length, uint8_t data[8]);
bool MockCAN_GetTxMessageAt(uint32_t index, uint32_t *id, uint8_t *length, uint8_t data[8]);

/* Verification helpers */
bool MockCAN_VerifyTxMessage(uint32_t index, uint32_t expectedId, uint8_t expectedLength);
bool MockCAN_VerifyTxData(uint32_t index, const uint8_t *expectedData, uint8_t length);

#ifdef __cplusplus
}
#endif
