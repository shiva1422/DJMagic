//
// Created by pspr1 on 5/27/2021.
//

#ifndef DHWANI_AUDIOSTUDIO_JAVACALL_H
#define DHWANI_AUDIOSTUDIO_JAVACALL_H


#include <android_native_app_glue.h>
#include "jni.h"
#include "Commons.h"

class ImageView;
class DisplayMetrics;
//struct android_app;

class JavaCall {
    static jclass cls;
    static JavaVM *vm;
    static JNIEnv *env;
    static android_app *app;
    static status attachThreadAndFindClass();
public:
   // static status importImageFromAssets(const char*,Bitmap *pixaMap);
   // static status setImageViewTexture(ImageView *imageView,const char *assetLoc);
    //static status setImageViewStackTexture(ImageViewStack *,int viewNo,const char* assetLoc);
    static void init(android_app *app)
    {
       JavaCall::app=app;
    }
    static status getDisplayMetrics(DisplayMetrics *displayMetrics);
    static status hideSystemUI();
    static status setImageViewTexture(ImageView *imageView, const char *assetLoc);
};


#endif //DHWANI_AUDIOSTUDIO_JAVACALL_H
