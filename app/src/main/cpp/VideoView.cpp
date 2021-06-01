//
// Created by shiva1422 on 5/31/2021.
//

#include <cstring>
#include "VideoView.h"
VideoView::VideoView() {}

VideoView::VideoView(int imageWidth, int imageHeight, int numFrames)
{
    bitmapHeight=imageHeight;
    bitmapWidth=imageWidth;
   // bitmapStride=image->stride;//check for gettting from ffmpeg

    if(glIsBuffer(texBufId))
        glDeleteBuffers(1,&texBufId);
    glGenBuffers(1,&texBufId);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,texBufId);
    glBufferData(GL_PIXEL_UNPACK_BUFFER,bitmapWidth*bitmapHeight*4,(void *)0,GL_DYNAMIC_DRAW);///pixels!=Null,check stride
    if(Graphics::printGlError("ImageView::setTexture")==GL_OUT_OF_MEMORY)//prompt
    {
        return;
    }


    if(glIsTexture(texId))
        glDeleteTextures(1,&texId);
    glGenTextures(1,&texId);
    glBindTexture(GL_TEXTURE_2D,texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, bitmapWidth,bitmapHeight);//glTexImage,Mutabletexture
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,bitmapWidth,bitmapHeight,GL_RGBA,GL_UNSIGNED_BYTE,0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    glBindTexture(GL_TEXTURE_2D,0);

    if(Graphics::printGlError("ImageView::setTexture")==GL_NO_ERROR)
    {
        Logi("ImageView::setTexture","success %d ,%d",bitmapWidth,bitmapHeight);
    }
    return;

}
void* VideoView::getBuf()
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,texBufId);
    void *mem = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,0,bitmapHeight*bitmapWidth*sizeof(int32),GL_MAP_WRITE_BIT|GL_MAP_WRITE_BIT);
    if(mem)
    {
       Logi("Videobuf","BufferMap success");
       //memset(mem,1,bitmapWidth*bitmapWidth);
    }
    //should be followd by unmap
    return mem;
}

void VideoView::updateFrame(bool bUpdateTexture)
{
  /*  glBindBuffer(GL_PIXEL_UNPACK_BUFFER,texBufId);
    void *mem = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,0,bitmapHeight*bitmapWidth*sizeof(int32),GL_MAP_WRITE_BIT|GL_MAP_WRITE_BIT);
    if(mem)
    {
        Logi("Videobuf","BufferMap success");
        uint8 *pixels=(uint8 *)mem;
      for(int i=0 ; i<bitmapWidth;++i)
      {
          for(int j=0; j<bitmapHeight;++j)
          {
             // Logi("VIdeoView","fillling %d %d",i,j);
              pixels[j*bitmapWidth *4 + i*4   ]=0;
              pixels[j*bitmapWidth *4 + i*4 +1 ]=0;

              pixels[j*bitmapWidth *4 + i*4  +2]=255;
              pixels[j*bitmapWidth *4 + i*4  +3]=255;



          }
      }
    }*/
    //should be followd by unmap
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    if(bUpdateTexture)
    {
        glBindTexture(GL_TEXTURE_2D,texId);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,texBufId);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,bitmapWidth,bitmapHeight,GL_RGBA,GL_UNSIGNED_BYTE,0);
        Graphics::printGlError("Video View");
        glBindTexture(GL_TEXTURE_2D,0);

    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);

}
