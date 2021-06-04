//
// Created by pspr1 on 5/27/2021.
//

#ifndef DHWANI_AUDIOSTUDIO_CONTEXT_H
#define DHWANI_AUDIOSTUDIO_CONTEXT_H

#include <EGL/egl.h>
#include <android_native_app_glue.h>
#include "Commons.h"
#if __ANDROID_API__ >= 24
#include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21
#include <GLES3/gl31.h>
#else
#include <GLES3/gl3.h>
#endif

class DisplayMetrics;
class View;

class Context {
private:
    static android_app *app;


public:
    static DisplayMetrics displayMetrics;
    GLuint uiProgram;
    bool bWindowInit=false,bGlInit=false,bAppFirstOpen=true;
    int32 width=0,height=0;//for graphics init .
    EGLDisplay eglDisplay=EGL_NO_DISPLAY;
    EGLSurface eglSurface=EGL_NO_SURFACE;
    EGLContext eglContext=EGL_NO_CONTEXT;

    View *contentView = nullptr;

    Context();

    static void setApp(android_app *app)
    {
        Context::app=app;
    }
    static android_app* getApp()
    {
        return app;
    }
};
class DisplayMetrics{
public:
    int32 screenWidth=0,screenHeight=0,densityDpi=0,deviceStableDensity=0;
    float density=0.0f,scaledDensity=0.0f,xdpi=0.0f,ydpi=0.0f;
    void print();
};

#endif //DHWANI_AUDIOSTUDIO_CONTEXT_H
