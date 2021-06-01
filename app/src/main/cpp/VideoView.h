//
// Created by shiva1422 on 5/31/2021.
//

#ifndef DJMAGIC_VIDEOVIEW_H
#define DJMAGIC_VIDEOVIEW_H


#include "ImageView.h"

class VideoView : public ImageView{
private:
    int numFrames=0;
public:
    VideoView();
    VideoView(int imageWidth, int imageHeight, int numFrames);
    void *getBuf();

    void updateFrame(bool bUpdateTexture);
};


#endif //DJMAGIC_VIDEOVIEW_H
