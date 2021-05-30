//
// Created by shiva1422 on 5/29/2021.
//

#include <android/asset_manager.h>
#include <unistd.h>
#include <cstdio>
#include "FileManager.h"
#include "Context.h"
#include "android/log.h"

int FileManager::getFileDescriptor(const char *assetLoc)
{
    int fd = -1;
    AAssetManager *assetManager = Context::getApp()->activity->assetManager;
    AAsset *asset =AAssetManager_open(assetManager,assetLoc,AASSET_MODE_RANDOM);
    off64_t offset,length=0;
    fd=AAsset_openFileDescriptor64(asset,&offset,&length);//<0 fail
    Logi("FileManager","the file descriptor is %d,%d,%d",fd,offset,length);//offset length values look reverseed so do as below;
    FILE *file=fdopen(fd,"rb");
    if(file && fseek(file,offset,SEEK_SET)!=0)//set file to begining cauze when getting fd from asset position is at end;AAsset_seek not working?
    {
        fd=-1;
    }
    AAsset_close(asset);
  // fclose(file);
    return fd;

}