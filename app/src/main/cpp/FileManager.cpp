//
// Created by shiva1422 on 5/29/2021.
//

#include <android/asset_manager.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include "FileManager.h"
#include "Context.h"
#include "android/log.h"

AAsset * FileManager::getFileDescriptor(const char *assetLoc)
{

    AAssetManager *assetManager = Context::getApp()->activity->assetManager;
    AAsset *asset =AAssetManager_open(assetManager,assetLoc,AASSET_MODE_RANDOM);

   // AAsset_close(asset);//CLose after done
    return asset;

}