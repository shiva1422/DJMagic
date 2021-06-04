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
#include "View.h"
int32 pointerIndex,pointerId,pointerCount;
float touchX ,touchY;
int32_t AndroidEvents::onInputEvent(android_app* papp, AInputEvent* event)
{
    View *contentView = ((Context *)papp->userData)->contentView;
    Loge("INPUT EVENT", "got");
    if(AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)//check eventSource accordinglt
    {
        pointerIndex =(AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        pointerId=AMotionEvent_getPointerId(event,pointerIndex);
        // TouchLog("the pointer index for all is %d and the pointer id is %d",pointerIndex,pointerId);
        touchX = AMotionEvent_getX(event, pointerIndex);
        touchY = AMotionEvent_getY(event, pointerIndex);
      //  Logi("OnInputEvent :","touch coordinates are x - %f and y -%f",touchX , touchY);
        switch (AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK)
        {
            case AMOTION_EVENT_ACTION_DOWN:
            {
                return contentView->onDispatchTouch(touchX,touchY,pointerId,ACTION_DOWN);
            }
            break;

        }




    }

    return 1;//change based on dispatching
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
                context->uiProgram=Shader::createShaderProgram("shaders/ui/vertexShader.glsl","shaders/ui/fragmentShader.glsl");
                glUseProgram(context->uiProgram);
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

