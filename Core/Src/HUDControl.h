#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "DiveCAN/DiveCAN.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Main task functions */
    void RGBBlinkControl();
    void EndBlinkControl();

    /* Exported for testing */
    int16_t div10_round(int16_t x);
    bool cell_alert(uint8_t cellVal);
    void PPO2Blink(CellValues_t *cellValues, bool *alerting);

#ifdef __cplusplus
}
#endif