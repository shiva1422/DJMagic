//
// Created by pspr1 on 5/27/2021.
//


#include <stdint.h>
#include <android_native_app_glue.h>
#include <assert.h>
#include "Commons.h"
#include "AndroidEvents.h"
#include "Context.h"
#include "Graphics.h"
#include "JavaCall.h"

int32_t AndroidEvents::onInputEvent(android_app* papp, AInputEvent* event)
{
    void *data = papp->userData;
    if (data == nullptr) {}
    Loge("INPUT EVENT", "got");
    int32_t eventType = AInputEvent_getType(event);
    switch (eventType)
    {
        case AINPUT_EVENT_TYPE_MOTION:
            if (AInputEvent_getSource(event) == AINPUT_SOURCE_TOUCHSCREEN)
                Logi("TOUCHED TOUCH SCREEN", "done");
            break;
    }
    return 1;
}
 void AndroidEvents::onAppCmd(android_app* app, int32_t cmd)
{
    Logi("command handling","done");
    switch(cmd)
    {
        case APP_CMD_SAVE_STATE:
            // the OS asked us to save the state of the app
            break;
        case APP_CMD_INIT_WINDOW:
            {

            Context *context = static_cast<Context *>(app->userData);
            JavaCall::hideSystemUI();
            if (context->bAppFirstOpen)
            {
                assert(Graphics::init_display(context) == STATUS_OK);
                context->bAppFirstOpen = false;
                context->bWindowInit = true;
                context->bGlInit = true;
                return;
            }
            if (Graphics::onAppReopen(context) == STATUS_OK)
            {
                context->bWindowInit = true;
            }
            }
            break;
        case APP_CMD_TERM_WINDOW:
        {
            Context *context = static_cast<Context *>(app->userData);
            context->bWindowInit=false;
        }
            break;
        case APP_CMD_LOST_FOCUS:
            // if the app lost focus, avoid unnecessary processing
            //(like monitoring the accelerometer)
            break;
        case APP_CMD_GAINED_FOCUS:
            // bring back a certain functionality, like monitoring the accelerometer
            break;
    }
}

