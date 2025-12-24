#pragma once

#ifdef __cplusplus
extern "C"
{
#endif


bool menuActive();
void onButtonPress();
void onButtonRelease();
void menuStateMachineTick();

#ifdef __cplusplus
}
#endif