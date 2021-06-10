//
// Created by shiva1422 on 6/6/2021.
//

#include <pthread.h>
#include <regex>
#include "Codec.h"
int FrameQueue::init(PacketQueue *pq, int maxSize, int keepLast)
{
    //can be init in class itself or can move to constructor;
    memset(this, 0 ,sizeof(FrameQueue));
    if(pthread_mutex_init(&mutex,NULL))
    {
        Loge("FrameQ init","could not create mutex %s",strerror(errno));
        return -1;
    }
    if(pthread_cond_init(&cond,NULL))
    {
        Loge("FrameQ init","could not create conditional %s",strerror(errno));
        return -1;
    }

    packetQueue = pq;
    maxSize = FFMIN(maxSize,FRAME_QUEUE_SIZE);
    keepLast = !!keepLast;//?
    for(int i=0 ; i<this->maxSize; ++i)
    {
        if(!(frameQueue[i].frame = av_frame_alloc()))
        {
            Loge("FrameQ init ","frameAlloc failed");
            return AVERROR(ENOMEM);
        }
    }
    return 0;
}
int PacketQueue ::init()
{
    memset(this,0,sizeof(PacketQueue));
    packetList = av_fifo_alloc(sizeof(MyAVPacketList));
    if(!packetList)
    {
        Loge("PacketQ:init","mem error1");
        return AVERROR(ENOMEM);
    }
    if(pthread_mutex_init(&mutex,NULL))
    {
        Loge("initpackets","could not create mutex %s",strerror(errno));
        return -1;
    }
    if(pthread_cond_init(&cond,NULL)!=0)
    {
        Loge("initpackets","could not create cond %s",strerror(errno));
        return -1;
    }
    abortReq = 1;
    return 0;

}
void Clock::init(int *qSerial)
{
    speed = 1.0;
    paused = 0;
    queueSerial = qSerial;
    setClock(NAN,-1);
}
void Clock::setClock(double pts, int serial)
{
    double time = av_gettime_relative()/1000000.0;
    setClockAt(pts,serial,time);
}
void Clock::setClockAt(double pts, int serial, double time)
{
    this->pts = pts;
    lastUpdated = time;
    ptsDrift = this->pts - time;
    this->serial = serial;
}
int Codec::init(AVCodecContext *codecContext, PacketQueue *packetQueue,pthread_cond_t *emptyQueueCond)
{
    memset(this,0 ,sizeof(Codec));//no need defaulted in class;
    if(!(packet = av_packet_alloc()))
    {
        Loge("CodecInit:","packet alloc failed");
        return AVERROR(ENOMEM);
    }
    this->codecContext = codecContext;
    this->packetQueue = packetQueue;
    this->condEmptyQ = emptyQueueCond;
    startPts = AV_NOPTS_VALUE;
    packetSerial = -1;
    return 0;
}