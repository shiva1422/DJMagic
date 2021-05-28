//
// Created by pspr1 on 5/27/2021.
//

#include "Graphics.h"
#include "initializer_list"
#define GraphicsLog(...)((void)__android_log_print(ANDROID_LOG_ERROR,"GRAPHICS LOG:",__VA_ARGS__))
EGLConfig Graphics::config = nullptr;
status Graphics::onAppReopen(Context *appContext)
{
    EGLint format;
    const EGLint surfaceAttribs[]={EGL_WIDTH,ANativeWindow_getWidth(Context::getApp()->window),EGL_HEIGHT,ANativeWindow_getHeight(Context::getApp()->window),EGL_NONE};
    if(! eglGetConfigAttrib(appContext->eglDisplay, config, EGL_NATIVE_VISUAL_ID, &format)){
        GraphicsLog("egl getConfig attrib failed");
        return STATUS_KO;
    }
    // create surfaace and make current
    ANativeWindow_setBuffersGeometry(Context::getApp()->window,0,0,format);
    appContext->eglSurface = eglCreateWindowSurface(appContext->eglDisplay, config,Context::getApp()->window, nullptr);////this is for visual surface
    //surface=eglCreatePbufferSurface(appContext->eglDisplay,config,surfaceAttribs);/////this is for off screen rendering
    if(  appContext->eglSurface==EGL_NO_SURFACE)
    {
        GraphicsLog("window surface creation failed");
        return STATUS_KO;
    }
    if (eglMakeCurrent(appContext->eglDisplay,appContext->eglSurface,appContext->eglSurface,appContext->eglContext ) == EGL_FALSE) {
        GraphicsLog("Unable to eglMakeCurrent");
        return STATUS_KO;
    }
    GraphicsLog("successfully initialization on app reopeng");
    return STATUS_OK;
}
status Graphics::init_display(Context* appContext)
{
    GraphicsLog("initializing opengl");
    const EGLint attribs[]={ EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_BLUE_SIZE, 8,EGL_GREEN_SIZE, 8,EGL_RED_SIZE, 8, EGL_SURFACE_TYPE, EGL_WINDOW_BIT,EGL_NONE};
    const EGLint context_attribs[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    const EGLint surfaceAttribs[]={EGL_WIDTH,ANativeWindow_getWidth(Context::getApp()->window),EGL_HEIGHT,ANativeWindow_getHeight(Context::getApp()->window),EGL_NONE};
    EGLint format;
    EGLint numConfigs;
    appContext->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(appContext->eglDisplay==EGL_NO_DISPLAY) {
        GraphicsLog("no display");
        return STATUS_KO;
    }

    if( !eglInitialize(appContext->eglDisplay, nullptr, nullptr))//can pass variable to get the result opengl versions
    {        GraphicsLog("egl initialize failed");

        return STATUS_KO;
    }
    if(!eglChooseConfig(appContext->eglDisplay, attribs, &config,1, &numConfigs)||numConfigs<=0)
    {//////chose the first config
        GraphicsLog("eglChooseConfig failed ");
        return STATUS_KO;

    }
    //   std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    //  assert(supportedConfigs);
    // eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);
    //  assert(numConfigs);
    // auto i = 0;
    GraphicsLog("numConfigs %d",numConfigs);
    /*  for (; i < numConfigs; i++) {
           auto& cfg = supportedConfigs[i];
           EGLint r, g, b, d,a;
           if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r)   &&
               eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g) &&
               eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b)  &&
               eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) &&
               r == 8 && g == 8 && b == 8) {//&&d=0 removed
               config = supportedConfigs[i];
               break;
           }
       }*/
    //  if (i == numConfigs) {
    //   GraphicsLog("no configuration match");
    //   config = supportedConfigs[0];
    // }


    if (config == nullptr) {
        GraphicsLog("Unable to initialize EGLConfig");
        return STATUS_KO;
    }
    if(! eglGetConfigAttrib(appContext->eglDisplay, config, EGL_NATIVE_VISUAL_ID, &format))
    {
        GraphicsLog("egl getConfig attrib failed");
        return STATUS_KO;
    }
    appContext->eglContext = eglCreateContext(appContext->eglDisplay, config, nullptr, context_attribs);//context attribs mandatory for creating required context
    if(appContext->eglContext==EGL_NO_CONTEXT){
        GraphicsLog("context creation failed");
        return STATUS_KO;
    }

    // create surfaace and make current
    ANativeWindow_setBuffersGeometry(Context::getApp()->window,0,0,format);
    appContext->eglSurface = eglCreateWindowSurface(appContext->eglDisplay, config, Context::getApp()->window, nullptr);////this is for visual surface
    //surface=eglCreatePbufferSurface(appContext->eglDisplay,config,surfaceAttribs);/////this is for off screen rendering
    if(  appContext->eglSurface==EGL_NO_SURFACE)
    {
        GraphicsLog("window surface creation failed");
        return STATUS_KO;
    }
    if (eglMakeCurrent(appContext->eglDisplay,appContext->eglSurface,appContext->eglSurface,appContext->eglContext ) == EGL_FALSE) {
        GraphicsLog("Unable to eglMakeCurrent");
        return STATUS_KO;
    }
    eglQuerySurface(appContext->eglDisplay,   appContext->eglSurface, EGL_WIDTH, &appContext->width);
    eglQuerySurface(appContext->eglDisplay,  appContext->eglSurface, EGL_HEIGHT, &appContext->height);
    auto opengl_info ={GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS};
    for (auto name : opengl_info) {
        auto info = glGetString(name);
        GraphicsLog("OpenGL Info: %s", info);
    }
    //  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    // glEnable(GL_CULL_FACE);
    //  glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);
    GraphicsLog("Opengl initialized successfully");
    return STATUS_OK;

}

void Graphics::destroyGL(Context *context)
{
    eglMakeCurrent(context->eglDisplay,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(context->eglDisplay,context->eglContext);
    eglDestroySurface(context->eglDisplay,context->eglSurface);
    eglTerminate(context->eglDisplay);
    context->eglDisplay=EGL_NO_DISPLAY;
    context->eglSurface=EGL_NO_SURFACE;
    context->eglContext=EGL_NO_CONTEXT;
}
GLenum Graphics::printGlError(const char *tag)
{
    GLenum glError=glGetError();
    switch (glError)
    {
        case GL_INVALID_ENUM: {
            Loge(tag, "OPENGL ERROR - INVALID ENUM");
        }
            break;
        case GL_INVALID_VALUE: {
            Loge(tag, "OPENGL ERROR - INVALID VALUE");
        }
            break;
        case GL_INVALID_OPERATION:
        {
            Loge(tag, "OPENGL ERROR - INVALID OPERATION");
        }
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:

        {
            Loge(tag,"OPENGL ERROR - INVALIDE FRAMEBUFFER OPERATION - CURRENTLY BOUND FRAMEBUFFER IS NOT FRAME BUFFER COMPLETE");
        }
        case GL_OUT_OF_MEMORY:
        {
            Loge(tag,"OPENGL ERROR-NOT ENOUGH MEMORY");
        }
        case GL_NO_ERROR:
        {
            Loge(tag,"NO OPENGL ERROR");
        }
            break;
        default:
            Loge(tag, "NO OPENGL EROOR OR the opengl error not listed in printglerror");

    }



    //also print any egl errors
    EGLenum eglError=eglGetError();
    switch(eglError)
    {
        case EGL_BAD_DISPLAY :
        {
            Loge(tag,"EGL ERROR - BAD DISPLAY-DISPLAY IS NOT AN EGL DISPLAY CONNECTION");

        }
            break;
        case EGL_BAD_SURFACE :
        {
            Loge(tag,"EGL ERROR - BAD SURFACE -SURFACE NOT AN EGL  DRAWING SURFACE");

        }
            break;
        case EGL_CONTEXT_LOST :
        {
            Loge(tag,"EGL ERROR - EGL CONTEXT LOST -REINITILIZING EVERYTHING AGAIN NEEDED");

        }
            break;
        case EGL_NOT_INITIALIZED:
        {
            Loge(tag,"EGL ERROR - EGL NOT INITIALIZED - DISPLAY HAS NOT BEEN INITIALIZED");

        }
            break;
        default:
            Loge(tag,"NO EGL ERROR OR ERROR IS NOT LISTED IN pringGlError function");

    }
    return glError;


}