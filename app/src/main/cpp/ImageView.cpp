//
// Created by shiva1422 on 5/28/2021.
//

#include "ImageView.h"
#include "JavaCall.h"

ImageView::ImageView() {}

ImageView::ImageView(float startX,float startY,float width,float height): View(startX,startY,width,height)
{
    //
}

void ImageView::setTexture(const char *assetLoc)
{
    if(JavaCall::setImageViewTexture(this,assetLoc)==STATUS_OK)
        Logi("setTexture","succes");
}

void ImageView::setTexture(Bitmap *image)
{
    if(image)
    {
        bitmapHeight=image->height;
        bitmapWidth=image->width;
        bitmapStride=image->stride;

        if(glIsBuffer(texBufId))
            glDeleteBuffers(1,&texBufId);
        glGenBuffers(1,&texBufId);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,texBufId);
        glBufferData(GL_PIXEL_UNPACK_BUFFER,bitmapWidth*bitmapHeight*4,image->pixels,GL_STATIC_COPY);///pixels!=Null,check stride
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
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, image->width,image->height);//glTexImage,Mutabletexture
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,image->width,image->height,GL_RGBA,GL_UNSIGNED_BYTE,0);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
        glBindTexture(GL_TEXTURE_2D,0);

        if(Graphics::printGlError("ImageView::setTexture")==GL_NO_ERROR)
        {
            Logi("ImageView::setTexture","success");
        }
        return;
    }
    Loge("setTEx","null bitmap");
}

void ImageView::draw()
{
    //View::draw();//backGround
   // glEnable(GL_BLEND);//check needed wholly(keep globlally enable ) correspond with disable.
   // glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glUniform2f(0,scaleX,scaleY);
    glUniform2f(2,translateX,translateY);

    glBindBuffer(GL_ARRAY_BUFFER,vertexBufId);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void *)0);

    glBindBuffer(GL_ARRAY_BUFFER,uvBufId);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FLOAT,0,(void *)0);

   // glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,texId);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indexBufId);
    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,(void *)0);


    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    glBindTexture(GL_TEXTURE_2D,0);

    glDisable(GL_BLEND);

}