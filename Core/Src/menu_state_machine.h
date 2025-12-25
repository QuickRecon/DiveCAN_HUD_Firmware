#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif


bool menuActive(void);
void onButtonPress(void);
void onButtonRelease(void);
void menuStateMachineTick(void);
void resetMenuStateMachine(void);

#ifdef __cplusplus
}
#endif