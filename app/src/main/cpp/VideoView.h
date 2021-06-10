//
// Created by shiva1422 on 5/31/2021.
//

#ifndef DJMAGIC_VIDEOVIEW_H
#define DJMAGIC_VIDEOVIEW_H

#include "vector"
#include "queue"
#include "ImageView.h"
#include "Texture.h"
using namespace std;
class MediaPlayer;
class VideoView : public ImageView{
private:
    int numFrames=0,activeTexNo=0,nextUpdateInd = 0,boolFirstFrame = true;
    int64_t pts=0;
    double ptsSecs=0;
    uint8 *buf=nullptr;
    bool frameRequired=true;
    bool texUpdate=false;
    bool textureUpdateRequired=true,drawFrameAvailable=false;
    MediaPlayer* mediaPlayer= nullptr;
    vector<Texture> textures;
    Texture *texToUpdate = nullptr;
    Texture *texToDraw = nullptr;

    void checkUpdate();

public:
    friend class MediaPlayer;
    VideoView();
    VideoView(int imageWidth, int imageHeight, int numFrames);
    virtual void draw() override;
    void setTexture(int imageWidth,int  imageHeight,int numFrames);
    void setFile(const char* assetLoc);
};


#endif //DJMAGIC_VIDEOVIEW_H
