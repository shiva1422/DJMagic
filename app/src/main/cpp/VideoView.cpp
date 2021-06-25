//
// Created by shiva1422 on 5/31/2021.
//

#include <cstring>
#include "VideoView.h"
#include "MediaPlayer.h"
#include "Codec.h"
VideoView::VideoView() {}

VideoView::VideoView(int imageWidth, int imageHeight, int numFrames)
{
    setTexture(imageWidth,imageHeight,numFrames);
    this->numFrames = numFrames;
}
void VideoView::setTexture(int imageWidth, int imageHeight,int numFrames)
{
    // bitmapStride=image->stride;//check for gettting from ffmpeg
    bitmapWidth =imageWidth,bitmapHeight=imageHeight;

    for(int i=0;i<numFrames;i++)
    {
        textures.push_back(Texture::createTexture(imageWidth,imageHeight));
    }


    texId=textures[0].tex;//active for drawing;//check texture size is not zero;
    texBufId=textures[0].buf;
    return;
}
void VideoView::setFile(const char *assetLoc)
{
    mediaPlayer = new MediaPlayer;
    mediaPlayer->setFile(assetLoc);
    mediaPlayer->outputView=this;
    //temp
    setTexture(100,100,1);

    //uncomment all accordingly;
   // Bitmap bitmapParams=mediaPlayer->getImageParams();
   // Logi("setFile video","height is %d and width is %d",bitmapParams.width,bitmapParams.height);
   // setTexture(bitmapParams.width,bitmapParams.height,2);
   // mediaPlayer->start();

}
void VideoView::draw()
{
    refreshVideo();
    texId = textures[0].tex;
    texBufId = textures[0].buf;
    ImageView::draw();
    /* Logi("VIdeo View :","draw");
     static bool filling = false,swapReq = false,draw = false;

         if(pthread_mutex_lock(&mediaPlayer->mutex) == 0)
         {
             if(texUpdate)
             {
                 buf = nullptr;
                 texUpdate = false;
                 textures[1].unmapBuf();
                 Texture::swapTextures(textures[0],textures[1]);
                 filling = false;

             }
             if(!filling)
             {
                 buf =textures[1].mapBuf();
                 filling = true;
                 pthread_cond_signal(&mediaPlayer->condContReadThread);

             }

             // while(!texUpdate)
              //   pthread_cond_wait(&mediaPlayer->condContReadThread,&mediaPlayer->mutex);

             pthread_mutex_unlock(&mediaPlayer->mutex);
         }


       /* if(pthread_mutex_trylock(&mediaPlayer->mutex) == 0)
        {
            if(texUpdate && filling)
            {
                buf = nullptr;
                texUpdate = false;
                textures[1].unmapBuf();
                filling =false;
            }
            pthread_mutex_unlock(&mediaPlayer->mutex);
        }*/





/*
    if(true)
    {
       // texId = texToDraw->tex;
       // texBufId = texToDraw->buf;
       texId = textures[0].tex;
       texBufId = textures[0].buf;
        ImageView::draw();
    }
*/

}
kforceinline void VideoView::checkUpdate()
{
    static int updateInd=0,preUpdateInd=1;
    static bool updated=false;
    pthread_mutex_lock(&mediaPlayer->mutex);

    if(frameRequired && updateInd!=preUpdateInd && updated)
    {
       // Logi("req frame","true");
        pthread_cond_signal(&mediaPlayer->condContReadThread);

        buf = textures[updateInd].mapBuf();

            Logi("req frame","true");
            pthread_cond_signal(&mediaPlayer->condContReadThread);
            preUpdateInd=updateInd;
            updateInd++;
            if(updateInd>1)
                updateInd=0;
            updated=false;

    }
    pthread_mutex_unlock(&mediaPlayer->mutex);

    pthread_mutex_lock(&mediaPlayer->mutex);
    if(texUpdate)
    {
      textures[preUpdateInd].unmapBuf();
      texToDraw = &textures[preUpdateInd];
      buf = nullptr;
      texUpdate=false;
      updated=true;
    }
    pthread_mutex_unlock(&mediaPlayer->mutex);

}

void VideoView::refreshVideo()
{
    double remTime = 0.01;
    mediaPlayer->onRefresh(&remTime);
}
void VideoView::updateTexture(Frame *frame)
{
    if(textures[0].bitmapWidth != frame->width || textures[0].bitmapHeight != frame->height)
    {
        textures[0] .reallocTexture(frame->width,frame->height);
    }
    uint8 *out =textures[0].mapBuf();
 if(out)
    {
        for(int x = 0;x <frame->width;++x)
        {
            for(int y=0 ; y < frame->height ; ++y)
            {
                // MediaLogE("the width adn height %d and %d",videoFrame->width,videoFrame->height);
                out[y * frame->width * 4 + x * 4 ]     =  frame->frame->data[0][ y *frame->frame->linesize[0] + x];
                out[y *  frame->width * 4 + x * 4 + 1 ] = frame->frame->data[0][ y *frame->frame->linesize[0] + x];
                out[y * frame->width * 4 + x * 4 + 2]  = 255;//player->videoFrame->data[0][ y * player->videoFrame->linesize[0] + x];
                out[y *  frame->width * 4 + x * 4 + 3]  = 255;
            }
        }
        textures[0].unmapBuf();

    }
}