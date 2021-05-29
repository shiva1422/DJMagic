//
// Created by pspr1 on 5/27/2021.
//

#include "JavaCall.h"
#include "Context.h"
#include "android/bitmap.h"
#include "ImageView.h"

#define JniLog(...)((void)__android_log_print(ANDROID_LOG_ERROR,"JNILOG",__VA_ARGS__))

JavaVM* JavaCall::vm=nullptr;
JNIEnv* JavaCall::env= nullptr;
jclass JavaCall::cls=nullptr;
android_app* JavaCall::app= nullptr;

status JavaCall::setImageViewTexture(ImageView *imageView, const char *assetLoc)
{
    Bitmap pixaMap;
    if(attachThreadAndFindClass()==STATUS_OK)
    {
        jmethodID mid = env->GetMethodID(cls, "importImageFromAssets", "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
        if (mid == 0)
        {
            JniLog("error obtaining the method id");
            return STATUS_KO;
        }
        jstring fileNameJava=env->NewStringUTF(assetLoc);
        jobject image = env->CallObjectMethod(app->activity->clazz, mid, fileNameJava);
        if (image != NULL)
        {
            JniLog("successfully obtained the bitmap");
            AndroidBitmapInfo bitmapInfo;
            if (AndroidBitmap_getInfo(env, reinterpret_cast<jobject>(image), &bitmapInfo) < 0) {
                JniLog("coulnd not obtain bitmap info");
                return STATUS_KO;
            }
            if (bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
                JniLog("THE BITMAP FORMATNOT NOT RGBA 8888");/////?improve to support others
                return STATUS_KO;
            }
            pixaMap.width = bitmapInfo.width;
            pixaMap.height = bitmapInfo.height;
            pixaMap.stride = bitmapInfo.stride;
            Loge(assetLoc,"image width-%d and height -%d",pixaMap.width,pixaMap.height);
            if (AndroidBitmap_lockPixels(env, image, (void **) &pixaMap.pixels) < 0) {
                JniLog("the bitmap could not be locked");
                return STATUS_KO;
            }
            imageView->setTexture(&pixaMap);
            AndroidBitmap_unlockPixels(env, image);
            env->DeleteLocalRef(image);
            vm->DetachCurrentThread();
            JniLog("%s Image Obtained succesfully ",assetLoc);
            return STATUS_OK;
        }
    }

        return STATUS_KO;
}
status JavaCall::hideSystemUI()
{
    if(attachThreadAndFindClass()==STATUS_OK)
    {
        jmethodID mid = env->GetMethodID(cls, "hideSystemUI", "()V");
        if (mid == 0)
        {
            JniLog("error obtaining the method id");
            return STATUS_KO;
        }
        env->CallVoidMethod(app->activity->clazz,mid);
        vm->DetachCurrentThread();/////d
        return STATUS_OK;
    }
    return STATUS_KO;
}
status JavaCall::getDisplayMetrics(DisplayMetrics *displayMetrics)
{
    if(attachThreadAndFindClass()==STATUS_OK)
    {
        jmethodID mid = env->GetMethodID(cls, "getDisplayParams", "()[F");
        if (mid == 0)
        {
            JniLog("error obtaining the method id get DisplayMetrics");
            return STATUS_KO ;
        }
        jfloatArray  displayParamsArray=(jfloatArray) env->CallObjectMethod(app->activity->clazz,mid);
        // jsize length=env->GetArrayLength(displayParamsArray);
        jfloat  *params=env->GetFloatArrayElements(displayParamsArray,0);
        displayMetrics->screenWidth=params[0];
        displayMetrics->screenHeight=params[1];
        displayMetrics->density=params[2];
        displayMetrics->densityDpi=params[3];
        displayMetrics->deviceStableDensity=params[4];
        displayMetrics->scaledDensity=params[5];
        displayMetrics->xdpi=params[6];
        displayMetrics->ydpi=params[7];
        env->ReleaseFloatArrayElements(displayParamsArray,params,0);
        vm->DetachCurrentThread();/////dssdfsdfs
        return STATUS_OK;
    }
    return STATUS_KO;
}
status JavaCall::attachThreadAndFindClass()
{

    if(!app)
    {
        app=Context::getApp();
    }
    if(app)
    {
        vm = app->activity->vm;
        vm->AttachCurrentThread(&env, NULL);
        if (env == NULL)
        {
            JniLog("coulf not attach/obtain current thread/get java environment");
            return STATUS_KO;
        }
        cls = (env)->GetObjectClass(app->activity->clazz);
        if(cls==NULL)
        {
            JniLog("coulf not get java object class");
            return STATUS_KO;
        }
        return STATUS_OK ;
    }
    else
    {
        JniLog("attachThreadAndFindClass","could not get app*");
        return STATUS_KO;
    }

}