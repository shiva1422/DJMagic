//
// Created by pspr1 on 5/28/2021.
//

#ifndef DJMAGIC_VIEW_H
#define DJMAGIC_VIEW_H
#include "Graphics.h"
#include "Commons.h"
class DisplayMetrics;
class View {
protected:
    float translateX=0.0f,translateY=0.0f,rotation=0.0f,scaleX=0.5f,scaleY=0.5f;
    float r=0.5f,g=0.5f,b=0.5f,a=1.0f;
    bool bVisible=true;
    static float vertices[8];
    static GLuint vertexBufId;
    static GLuint indexBufId;
    static GLuint texCoodBufId;

public:
    static DisplayMetrics *disMet;//private?

    View();
    View(int32 startX,int32 startY,int32 width,int32 height);
    static void initializeUI();
    virtual void setBounds(int32 startX,int32 startY,int32 width,int32 height);
    virtual void draw() ;

};


#endif //DJMAGIC_VIEW_H
