//
// Created by pspr1 on 5/27/2021.
//

#include "Context.h"
android_app* Context::app= nullptr;

void DisplayMetrics::print()
{
    Logi("\nDisplayParams:","screenWidth %d\nscreenHeight %d\ndensityDpi %d\ndeviceStableDensity %d\ndensity %f\nscaled density %f\nxdpi %f\nydpi %f",screenWidth,screenHeight,densityDpi,deviceStableDensity,density,scaledDensity,xdpi,ydpi);
}