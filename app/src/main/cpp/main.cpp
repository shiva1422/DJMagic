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
#include "VideoView.h"
#include "Time.h"
#include "Player.h"

extern "C" {
#include <libavformat/avformat.h>
}
void initialSetup(android_app *app);
void android_main(struct android_app* app)
{
    Logi("FFMpeg"," avcodec configuration %s",avcodec_configuration());
    MediaCodec::findCodec();

  //  MediaCodec::showAvailableCodecs();

    app->onAppCmd=AndroidEvents::onAppCmd;
    app->onInputEvent=AndroidEvents::onInputEvent;

    Context context;
    app->userData=(void *)&context;
//initialize displayMetric hear and let Context have only pointer;
    initialSetup(app);
    DisplayMetrics *displayMetrics=&Context::displayMetrics;
    ImageView view(displayMetrics->screenWidth*(10/100),displayMetrics->screenHeight/2,displayMetrics->screenWidth/2,displayMetrics->screenHeight/2);
    view.setTexture("icons/test.png");
    context.contentView=&view;

   //Bitmap imageParams=mediaPlayer.getImageParams();
  // VideoView videoView(imageParams.width,imageParams.height,1);
   VideoView videoView;
   videoView.setBounds(100,100,1000,500);
   videoView.setFile("video/test.mp4");
   // Player player("video/test.mp4");
    int32_t eventId,events,fdesc;
    android_poll_source* source;
    TimeDiff frameTime;
   do
    {
        while((eventId=ALooper_pollAll(0,&fdesc,&events,(void **) &source))>=0)
        {
            if (source != NULL)
            {
                source->process(app, source);

            }
            break;

        }
        frameTime.start();
        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);


      //  videoView.updateFrame(mediaPlayer.getFrame(videoView.getBuf(), imageParams));
        // usleep(1000);

        view.draw();
        videoView.draw();
       // Graphics::printGlError("Video View");
        eglSwapBuffers(context.eglDisplay, context.eglSurface);
        frameTime.end();

    }while(app->destroyRequested==0);
}
void initialSetup(android_app *app)//move to context::init;
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

