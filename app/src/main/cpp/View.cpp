//
// Created by pspr1 on 5/28/2021.
//

#include <string.h>
#include "View.h"
#include "Graphics.h"

GLuint View::vertexBufId=0;
GLuint View::indexBufId=0;
GLuint View::uvBufId=0;
float View::vertices[]={-1.0f,-1.0f,1.0f,-1.0f,1.0f,1.0f,-1.0f,1.0f};
DisplayMetrics* View::disMet =nullptr;

View::View() {}

View::View(int startX, int startY, int width, int height)
{
    setBounds(startX,startY,width,height);
}
kforceinline void View::setBounds(int startX, int startY, int width, int height)
{
    scaleX = float(width)/float(disMet->screenWidth);
    scaleY = float(height)/float(disMet->screenHeight);
    translateX = ( (startX+width/2.0)- disMet->screenWidth/2.0 )* 2.0/ disMet->screenWidth;
    translateY = ( -(startY+height/2.0) + disMet->screenHeight/2.0 ) *2.0 / disMet->screenHeight;
}

void View ::draw()
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glUniform2f(0,scaleX,scaleY);
    glUniform2f(2,translateX,translateY);

    glBindBuffer(GL_ARRAY_BUFFER,vertexBufId);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void *)0);//make one array for vertices and color and make throgh stride

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indexBufId);
    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,(void *)0);

    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

}

void View::initializeUI()
{
    //improve anything fails return statusKo and assert;

    GLuint bufferIds[3];
    glGenBuffers(3,bufferIds);
    View::indexBufId=bufferIds[0], View::uvBufId=bufferIds[1], View::vertexBufId=bufferIds[2];
    //for(int i=0;i<3;i++)UILogE("%d, vertexBufId %d",bufferIds[i],vertexBufId);

    glBindBuffer(GL_ARRAY_BUFFER,View::vertexBufId);
    glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,(void *)0,GL_STATIC_DRAW);
    float *verts=(float *)glMapBufferRange(GL_ARRAY_BUFFER,0,sizeof(float)*8,GL_MAP_WRITE_BIT);
    if(verts)
    {
        Logi("initialize ui","vets");
        memcpy(verts,View::vertices,8*sizeof(float));

    }
    else
        Logi("initialize ui","vertex upload failed");
    glUnmapBuffer(GL_ARRAY_BUFFER);//return GL_false if error


    glBindBuffer(GL_ARRAY_BUFFER,View::uvBufId);
    glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,(void *)0,GL_STATIC_DRAW);
    float *textCoords=(float *)glMapBufferRange(GL_ARRAY_BUFFER,0,sizeof(float)*8,GL_MAP_WRITE_BIT);
    if(textCoords)
    {

        //0.0f,1.0f,1.0f,1.0f,1.0f,0.0f,0.0f,0.0f
        textCoords[0]=0.0f,textCoords[1]=0.0f,textCoords[2]=1.0f,textCoords[3]=0.0f,textCoords[4]=1.0f,textCoords[5]=1.0f,textCoords[6]=0.0f,textCoords[7]=1.0f;

    }
    else
    {////lazy draw on
        ///uploading vertices everydrawcall
        Logi("failed","UIINITIalize()-error uploading textCoods");
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);//return GL_false if error
    //
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,View::indexBufId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short )*6, (void *)0, GL_STATIC_DRAW);
    GLushort *indices=(GLushort *)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER,0,sizeof(short )*6,GL_MAP_WRITE_BIT);
    if(indices)
    {
        indices[0]=0, indices[1]=1, indices[2]=2, indices[3]=2, indices[4]=3, indices[5]=0;

    }
    else
    {////lazy draw on
        ///uploading vertices everydrawcall
        Loge("failed","UIINITIALIZe()-error uploading indices");
    }
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}