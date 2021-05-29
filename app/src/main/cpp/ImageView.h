//
// Created by shiva1422 on 5/28/2021.
//

#ifndef DJMAGIC_IMAGEVIEW_H
#define DJMAGIC_IMAGEVIEW_H


#include "View.h"
#include "Bitmap.h"

class ImageView : public View{
private:
    GLuint texId=0,texBufId=0;
    int32 bitmapHeight=0,bitmapWidth=0,bitmapStride=0;
public:
    ImageView();
    ImageView(int32 startX,int32 startY,int32 width,int32 height);
    void setTexture(Bitmap *image);
    void setTexture(const char *assetLoc);
    virtual void draw() override;

};


#endif //DJMAGIC_IMAGEVIEW_H
