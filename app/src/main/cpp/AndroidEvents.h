//
// Created by pspr1 on 5/27/2021.
//

#ifndef DHWANI_AUDIOSTUDIO_ANDROIDEVENTS_H
#define DHWANI_AUDIOSTUDIO_ANDROIDEVENTS_H

#include "Commons.h"
struct android_app;
struct AInputEvent;

int32_t onInputEvent(android_app* papp, AInputEvent* event);
void onAppCmd(android_app* app, int32_t cmd);

class AndroidEvents{
public:
    static int32_t onInputEvent(android_app* papp, AInputEvent* event);
    static void onAppCmd(android_app* app, int32_t cmd);
};
#endif //DHWANI_AUDIOSTUDIO_ANDROIDEVENTS_H
