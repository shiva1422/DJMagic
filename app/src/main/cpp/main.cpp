//
// Created by pspr1 on 6/3/2020.
//
#include <assert.h>
#include "android/log.h"
#include "Commons.h"
#include "AndroidEvents.h"
#include "JavaCall.h"
#include "Context.h"
#include "Graphics.h"
#include "View.h"
#include "ImageView.h"
#include "AudioTrack.h"
#include "MediaCodec.h"
#include "FileManager.h"
#include "MediaPlayer.h"

extern "C" {
#include <libavformat/avformat.h>
}

void initialSetup(android_app *app);
void android_main(struct android_app* app)
{
    Logi("FFMpeg"," avcodec configuration %s",avcodec_configuration());
    MediaCodec::findCodec();

    MediaCodec::showAvailableCodecs();

    app->onAppCmd=AndroidEvents::onAppCmd;
    app->onInputEvent=AndroidEvents::onInputEvent;

    Context context;
    app->userData=(void *)&context;
//initialize displayMetric hear and let Context have only pointer;
    initialSetup(app);
    DisplayMetrics *displayMetrics=&Context::displayMetrics;
    ImageView view(displayMetrics->screenWidth*(10/100),displayMetrics->screenHeight/2,displayMetrics->screenWidth/2,displayMetrics->screenHeight/2);
    view.setTexture("icons/test.png");
    MediaPlayer audioPlayer;
    audioPlayer.setAudioFileFromAssets("audio/okeoka.mp3");








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
        view.draw();
        eglSwapBuffers(context.eglDisplay,context.eglSurface);

    }while(app->destroyRequested==0);
}
void initialSetup(android_app *app)
{


    Context::setApp(app);

    int32_t eventId,events,fdesc;
    android_poll_source* source;
    do//just to init GL so that glfuncts dont fail after this;
    {
        while((eventId=ALooper_pollAll(0,&fdesc,&events,(void **) &source))>=0)
        {
            if(source!=NULL)
            {
                source->process(app,source);
            }
        }

    }while(!((Context *)app->userData)->bWindowInit);

    assert(JavaCall::getDisplayMetrics(&Context::displayMetrics)==STATUS_OK);
    Context::displayMetrics.print();
    View::disMet=&Context::displayMetrics;
    View::initializeUI();



}

