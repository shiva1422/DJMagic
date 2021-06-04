//
// Created by pspr1 on 5/28/2021.
//

#ifndef DJMAGIC_VIEW_H
#define DJMAGIC_VIEW_H
#include "Graphics.h"
#include "Commons.h"
/*
 * bounds <-> floats check accorly
 */
enum TouchAction{ACTION_DOWN,ACTION_POINTER_DOWN,ACTION_MOVE,ACTION_POINTER_UP,ACTION_UP};

class DisplayMetrics;
class View {
    class TouchListener{
    public:
        virtual bool onTouch(int32 touchX,int32 touchY,int pointerId,TouchAction touchAction,View *view);

    };
protected:
    float startX=0,startY=0,width=0,height=0;//just for now later obtain from translate scale and rot or based on latency may be keep it
    float translateX=0.0f,translateY=0.0f,rotation=0.0f,scaleX=0.5f,scaleY=0.5f;
    float r=0.5f,g=0.5f,b=0.5f,a=1.0f;

    bool bVisible=true,bTouchEnabled=false;//modify touchEnabledd based on visibility
    static float vertices[8];
    static GLuint vertexBufId;
    static GLuint indexBufId;
    static GLuint uvBufId;
    TouchListener *touchListener= nullptr;



public:
    static DisplayMetrics *disMet;//private?

    View();
    View(float startX,float startY,float width,float height);
    static void initializeUI();
    virtual void setBounds(float startX,float startY,float width,float height);
    virtual bool onDispatchTouch(float touchX,float touchY,int pointerId,TouchAction touchAction);
    virtual void draw() ;

};


#endif //DJMAGIC_VIEW_H
