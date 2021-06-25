//
// Created by shiva1422 on 6/5/2021.
//

#include "Texture.h"
Texture Texture::createTexture(int bitmapWidth , int bitmapHeight)
{
    Texture texture;
    texture.bitmapWidth=bitmapWidth;
    texture.bitmapHeight=bitmapHeight;
    if(glIsBuffer(texture.buf))
        glDeleteBuffers(1,&texture.buf);
    glGenBuffers(1,&texture.buf);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,texture.buf);
    glBufferData(GL_PIXEL_UNPACK_BUFFER,bitmapWidth*bitmapHeight*sizeof(int32),(void *)0,GL_DYNAMIC_DRAW);///pixels!=Null,check stride
    if(Graphics::printGlError("ImageView::setTexture")==GL_OUT_OF_MEMORY)//prompt
    {
        return texture;
    }


    if(glIsTexture(texture.tex))
        glDeleteTextures(1,&texture.tex);
    glGenTextures(1,&texture.tex);
    glBindTexture(GL_TEXTURE_2D,texture.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, bitmapWidth,bitmapHeight);//glTexImage,Mutabletexture
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,bitmapWidth,bitmapHeight,GL_RGBA,GL_UNSIGNED_BYTE,0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);//out of loop
    glBindTexture(GL_TEXTURE_2D,0);

    if(Graphics::printGlError("ImageView::setTexture")==GL_NO_ERROR)
    {
        Logi("ImageView::setTexture","success %d ,%d",bitmapWidth,bitmapHeight);
    }
    return texture;
}
void Texture::reallocTexture(int width, int height)
{
    Loge("Texture realloc","test1");
    if(glIsBuffer(buf))
        glDeleteBuffers(1,&buf);
    if(glIsTexture(tex))
        glDeleteTextures(1,&tex);
    *this = createTexture(width,height);
    Loge("Texture realloc","test2");


    //Needs change in createTex
}
uint8* Texture::mapBuf()
{

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,buf);
    void *mem = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,0,bitmapHeight*bitmapWidth*sizeof(int32),GL_MAP_WRITE_BIT|GL_MAP_WRITE_BIT);
    if(mem)
    {

        Logi("Videobuf ","BufferMap success %d",buf);
        // memset(mem,0,bitmapWidth*bitmapWidth*sizeof(int32));
        //texInds.pop();
        //texInds.push(texFillInd);
        // nextUpdateInd = texFillInd;
       // Logi("Get Buf","dequed");

    }
    //should be followd by unmap
    return (uint8 *)mem;
}
void Texture::unmapBuf()
{
    //  glBindBuffer(GL_PIXEL_UNPACK_BUFFER,texBufs[nextUpdateInd]);

    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    glBindTexture(GL_TEXTURE_2D,tex);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,buf);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,bitmapWidth,bitmapHeight,GL_RGBA,GL_UNSIGNED_BYTE,0);
   // Graphics::printGlError("Texture unmap");
    glBindTexture(GL_TEXTURE_2D,0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
}
void Texture::swapTextures(Texture &tex1, Texture &tex2)
{

    Texture temp=tex1;
    tex1 = tex2;
    tex2 = temp;
}