//
// Created by pspr1 on 6/3/2020.
//
#include<android_native_app_glue.h>
#include <assert.h>
#include "android/log.h"
#include "Commons.h"
#include "AndroidEvents.h"
#include "JavaCall.h"
#include "Context.h"
#include "Graphics.h"
#define TAG "main"
#define LOGI(...)((void)__android_log_print(ANDROID_LOG_INFO,__VA_ARGS__,__VA_ARGS__));
#define LOGE(...)((void)__android_log_print(ANDROID_LOG_ERROR,__VA_ARGS__,__VA_ARGS__));
void initialSetup(android_app *app);
void android_main(struct android_app* app)
{
    initialSetup(app);
    app->onAppCmd=AndroidEvents::onAppCmd;
    app->onInputEvent=AndroidEvents::onInputEvent;

    DisplayMetrics displayMetrics;
    assert(JavaCall::getDisplayMetrics(&displayMetrics)==STATUS_OK);
    displayMetrics.print();

    Context context;
    app->userData=(void *)&context;

    int32_t eventId,events,fdesc;
    android_poll_source* source;
   do
    {
        while((eventId=ALooper_pollAll(0,&fdesc,&events,(void **) &source))>=0)
       {
           if(source!=NULL)
           {
               source->process(app,source);
           }
       }
        glClearColor(1.0,0.0,0.0,1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(context.eglDisplay,context.eglSurface);

    }while(app->destroyRequested==0);
}
void initialSetup(android_app *app)
{
    Context::setApp(app);

}

