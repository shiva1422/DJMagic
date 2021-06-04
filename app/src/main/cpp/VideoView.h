//
// Created by shiva1422 on 5/31/2021.
//

#ifndef DJMAGIC_VIDEOVIEW_H
#define DJMAGIC_VIDEOVIEW_H


#include "ImageView.h"

class MediaPlayer;
class VideoView : public ImageView{
private:
    int numFrames=0;
    uint8 *buf=nullptr;
    bool frameRequired=true;
    bool textureUpdateRequired=true,drawRequired=true;
    MediaPlayer* mediaPlayer= nullptr;
    void onFrameAvailable(void *buf);
    void *getBuf();

    void onFrameRequired();

public:
    friend class MediaPlayer;
    VideoView();
    VideoView(int imageWidth, int imageHeight, int numFrames);
    virtual void draw() override;
    void setTexture(int imageWidth,int  imageHeight);
    void setFile(const char* assetLoc);
    void updateFrame(bool bUpdateTexture);
};


#endif //DJMAGIC_VIDEOVIEW_H
