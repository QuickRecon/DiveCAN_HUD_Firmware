#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HAL Status structures definition */
#ifndef HAL_STATUSDEF
#define HAL_STATUSDEF
typedef enum
{
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;
#endif

#ifdef __cplusplus
}
#endif
