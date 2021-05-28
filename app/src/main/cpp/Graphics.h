//
// Created by pspr1 on 5/27/2021.
//

#ifndef DHWANI_AUDIOSTUDIO_GRAPHICS_H
#define DHWANI_AUDIOSTUDIO_GRAPHICS_H

#include "Commons.h"
#include "Context.h"
#include <malloc.h>

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

class Shader{
private:
    char *source=NULL;
public:
    GLuint id=0;
    bool getSource(const char *fileName);
    GLuint compile(GLenum shaderType);
    GLuint loadAndCompileShader(const char *fileName,GLenum shaderType)
    {
        bool gotSource=getSource(fileName);
        if(gotSource)
        {
            id = compile(shaderType);//// id =0 error;

        }
        return id;

    }
    void deleteSource()
    {
        if(source)
        {
            free(source);
            source=NULL;
        }
    }
    static GLuint createShaderProgram(const char *vertexShader,const char *fragmentShader);
    static GLuint linkShaders(GLuint vertexShaderId,GLuint fragmentShaderId); ////// if return 0 => error;
    static GLuint createComputeProgram(const char *computeShaderFile);
};


#endif //DHWANI_AUDIOSTUDIO_GRAPHICS_H
