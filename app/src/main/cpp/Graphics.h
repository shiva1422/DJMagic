//
// Created by pspr1 on 5/27/2021.
//

#ifndef DHWANI_AUDIOSTUDIO_GRAPHICS_H
#define DHWANI_AUDIOSTUDIO_GRAPHICS_H

#include "Commons.h"
#include "Context.h"
#include "EGL/egl.h"
#if __ANDROID_API__ >= 24
#include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21
#include <GLES3/gl31.h>
#else
#include <GLES3/gl3.h>
#endif

class Graphics {
public:
    static DisplayMetrics displayMetrics;
    static EGLConfig config;
    static status init_display(Context* appContext);
    static status onAppReopen(Context* appContext);
    static void destroyGL(Context *appContext);
    static GLenum printGlError(const char *tag);

};


#endif //DHWANI_AUDIOSTUDIO_GRAPHICS_H
