//
// Created by shiva1422 on 6/6/2021.
//

#include <pthread.h>
#include <android/asset_manager.h>
#include "Player.h"
#include "Commons.h"
#include "FileManager.h"

#define VIDEO_BUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#define FF_INPUT_BUFFER_PADDING_SIZE 64
static int readPacket(void *opaque, uint8 *buf, int bufSize)
{
    AAsset* src=(AAsset *)opaque;//this opaque thing can be any thing that is source of data.
    bufSize=FFMIN(bufSize, VIDEO_BUF_SIZE);//check

    if(!bufSize)
        return AVERROR_EOF;
    //MediaLogI("ptr %%p size:%zu\n",bufSize);

    //fill buf buffer data to buf from AAsset or anything src
    int readSize = AAsset_read(src,buf,bufSize);
    if(readSize == 0)
    {
        return AVERROR_EOF;//also < o on error;
    }
    // src->ptr +=bufSize;//if reading from memeory that is through a pointer change current pointer location and size remaining;
    // src->size -=bufSize;

    return readSize;

}
static int64_t kseek(void *opaque,int64_t offset , int whence)
{

    AAsset *asset = (AAsset *)opaque;
    return AAsset_seek(asset,offset,whence);
}
void * Player:: readThread(void *VidState)
{
    uint8 *vidBuf=(uint8 *)av_malloc(VIDEO_BUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);//move to header

    Logi("ReadThread","created ,started");
    VideoState *vidState=(VideoState *)VidState;
    AVFormatContext *formatContext = nullptr;
    int err , i , ret;
    int streamInd[3];//AV_MEDIA_TYPENB;///check;
    AVPacket *packet = nullptr;
    int64 streamStartTime;
    int pktInPlayRange = 0;
    AVDictionaryEntry  *t;
    pthread_mutex_t *waitMutex;
    int scanAllPMTSSet = 0;
    int64 packetTS;
    if(pthread_mutex_init(waitMutex,NULL))
    {
        goto fail;
    }
    memset(streamInd,-1,sizeof(streamInd));
    vidState->eof = 0;
    packet = av_packet_alloc();
    if(!packet)
    {
        Loge("ReadThread","couldnt alloc packet");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    formatContext = avformat_alloc_context();
    if(!formatContext)
    {
        Loge("ReadThread","format context alloc fialed");
        ret =AVERROR(ENOMEM);
        goto fail;
    }
   // formatContext->interrupt_callback = decodeInterruptCallBack;//uncom;
   formatContext->interrupt_callback.opaque = vidState;
   /* if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
        av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }*/
   vidState->asset= FileManager::getAsset(vidState->fileName.c_str());
    if(!vidState->asset)
    {
        Loge("readThread","couldnot open asset");
        goto fail;
    }


    //based on type of files set ioContext now for asset;
    vidState->ioContext = avio_alloc_context(vidBuf ,VIDEO_BUF_SIZE,0,(void *)vidState->asset,readPacket,NULL,kseek);
    if(!vidState->ioContext)
    {
        //close AAset ,clear vid buffer
        Loge("readThread","IOContext alloc failed");
        goto fail;
    }
    formatContext->pb =vidState->ioContext;
    formatContext->flags = AVFMT_FLAG_CUSTOM_IO;
    if(avformat_open_input(&formatContext, "",NULL,NULL) < 0)
    {
        //clear formatContex;
        ret = -1;
        Loge("ReadThread","OpenInput failed");//close
        goto fail;
    }

    fail:
    Loge("readThread","Fail");

    Logi("ReadThread","exit");
}
Player::Player(const char* fileName)
{
    this->fileName = string(fileName);
    videoState = new VideoState;//(VideoState *) av_mallocz(sizeof(VideoState));
    if(!videoState)
    {
        Logi("Player","video state Alloc Failed");
        return;
    }
    videoState->fileName = string(fileName);
    //if ! filename goto fial
    videoState->inputFormat =  this->fileFormat;

    if(openStreams()<0)
    {
        return;
    }
    bInit = true;

    //start VideoDisp


}
int Player ::openStreams()
{
    //some can be set in classes itself

    if(initFrameQueues(&videoState->picQ,&videoState->vidPktQ ,VIDEO_PICTURE_QUEUE_SIZE,1) < 0)
    {
        goto fail;
    }
    if(initFrameQueues(&videoState->subPicQ ,&videoState->subPktQ,SUBPICTURE_QUEUE_SIZE,0)<0)
    {
        goto fail;
    }
    if(initFrameQueues(&videoState->sampleQ, &videoState->audioPktQ, SAMPLE_QUEUE_SIZE, 1) < 0)
    {
        goto fail;
    }
    if(initPacketQueues(&videoState->vidPktQ) < 0 || initPacketQueues(&videoState->audioPktQ) < 0 || initPacketQueues(&videoState->subPktQ) < 0)
    {
        goto fail;
    }
    initClock(&videoState->audioClock,&videoState->audioPktQ.serial);
    initClock(&videoState->videoClock,&videoState->vidPktQ.serial);
    initClock(&videoState->extClock,&videoState->extClock.serial);
    videoState->audioClockSerial = -1;
    videoState->muted = 0;
    videoState->avSyncType = avSyncType;
    if(pthread_create(videoState->readThread,NULL,readThread,(void *)videoState))
    {
        Loge("openStream","ReadThread creation failed;");
        goto fail;

    }
    return 0;

    fail:
    closeStreams();//.impelemtnt;
    return -1;
}
int Player::initFrameQueues(FrameQueue *frameQueue, PacketQueue *packetQueue, int maxSize,int keepLast)
{
    int i;
    memset(frameQueue , 0 , sizeof(FrameQueue));
    if(pthread_mutex_init(&frameQueue->mutex,NULL)!=0)//destroy
    {
        Loge("initFrame","could not create mutex");
        errString += string(strerror(errno));
        return -1;
    }
    if(pthread_cond_init(&frameQueue->cond,NULL)!=0)
    {
        Loge("initFrame","could not create conditional");
        errString += string(strerror(errno));
        return -1;
    }
    frameQueue->packetQueue = packetQueue;
    frameQueue->maxSize = FFMIN(maxSize, FRAME_QUEUE_SIZE);
    frameQueue->keepLast = !!keepLast;//?
    for(i = 0; i <frameQueue->maxSize ; i++)
    {
        if(!(frameQueue->frameQueue[i].frame = av_frame_alloc()))
        {
            errString += string("could not alloc frame");
            return AVERROR(ENOMEM);
        }
    }
    return 0;


}
int Player::initPacketQueues(PacketQueue *queue)
{
    memset(queue,0,sizeof(PacketQueue));
    queue->packetList = av_fifo_alloc(sizeof(MyAVPacketList));
    if(!queue->packetList)
    {
        Loge("INitPackets","mem error 1");
        return AVERROR(ENOMEM);
    }
    if(pthread_mutex_init(&queue->mutex,NULL)!=0)
    {
        Loge("initpackets","could not create mutex");
        errString += string(strerror(errno));
        return -1;
    }
    if(pthread_cond_init(&queue->cond,NULL)!=0)
    {
        Loge("initpackets","could not create cond");
        errString += string(strerror(errno));
        return -1;
    }
    queue->abortReq = 1;
    return 0;
}
void Player::initClock(Clock *c, int *queueSerial)
{
    c->speed = 1.0;
    c->paused = 0;
    c->queueSerial = queueSerial;
    setClock(c,NAN,-1);
}
void Player::setClock(Clock *c,double pts, int serial)
{
    double time = av_gettime_relative()/1000000.0;
    setClockAt(c,pts,serial,time);
}
void Player::setClockAt(Clock *c, double pts,int serial, double time)
{
    c->pts = pts;
    c->lastUpdated = time;
    c->ptsDrift = c->pts -time;
    c->serial = serial;
}
int Player::closeStreams()
{

}
