//
// Created by shiva1422 on 6/5/2021.
//

#ifndef DJMAGIC_TEXTURE_H
#define DJMAGIC_TEXTURE_H
#include "Graphics.h"

class Texture {

public:
    GLuint tex = 0, buf = 0;
    int bitmapWidth=0, bitmapHeight=0;
    bool bDrawReady = false;
    double ptsTime = 0.0;//just for now later move from texture;
    static Texture createTexture(int width,int height);//now only rgba;
    static void swapTextures(Texture &tex1 ,Texture &tex2);
    uint8* mapBuf();
    void unmapBuf();
};


#endif //DJMAGIC_TEXTURE_H
