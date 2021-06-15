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
    this->maxSize = FFMIN(maxSize,FRAME_QUEUE_SIZE);
    this->keepLast = !!keepLast;//?
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
void FrameQueue::push()
{
    if(++windex == maxSize)
        windex = 0;
    pthread_mutex_lock(&mutex);
    size++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
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
double Clock::getValue()
{
    if(*queueSerial != serial)
        return NAN;
    if(paused)
        return pts;
    else
    {
        double time = av_gettime_relative()/1000000.0;
        return ptsDrift + time - (time - lastUpdated) * (1.0 - speed);
    }

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
int Codec::start(void *(*fn)(void *), MediaPlayer *player)
{
    packetQueue->start();
    if(pthread_create(&thrId,NULL,fn,player))
    {
        Loge("CodecStart:","Could not create thread");
        return -1;//NOMEM
    }
    return 0;
}

void PacketQueue::start()
{
    pthread_mutex_lock(&mutex);//check succ
    abortReq = 0;
    serial++;
    pthread_mutex_unlock(&mutex);
}
void PacketQueue::flush()
{
    MyAVPacketList pkt1;
    pthread_mutex_lock(&mutex);

    while(av_fifo_size(packetList) >= sizeof(pkt1))
    {
        av_fifo_generic_read(packetList,&pkt1,sizeof(pkt1),NULL);
        av_packet_free(&pkt1.pkt);
    }
    numPackets = 0;
    size = 0;
    duration = 0;
    serial++;

    pthread_mutex_unlock(&mutex);
}
int PacketQueue::put(AVPacket *pkt)
{
    AVPacket *pkt1;
    int ret;

    pkt1 = av_packet_alloc();
    if(!pkt1)
    {
        av_packet_unref(pkt);
        return -1;
    }
    av_packet_move_ref(pkt1, pkt);

    pthread_mutex_lock(&mutex);
    ret = putPrivate(pkt1);
    pthread_mutex_unlock(&mutex);
    if(ret < 0)
        av_packet_free(&pkt1);

    return ret;
}
int PacketQueue::putPrivate(AVPacket *pkt)
{
    MyAVPacketList pkt1;

    if(abortReq)
        return -1;

    if(av_fifo_space(packetList) < sizeof(pkt1))
    {
        if(av_fifo_grow(packetList , sizeof(pkt1)) < 0)
            return -1;
    }

    pkt1.pkt = pkt;
    pkt1.serial = serial;

    av_fifo_generic_write(packetList , &pkt1 ,sizeof(pkt1),NULL);
    numPackets++;
    size += pkt1.pkt->size + sizeof(pkt1);
    duration += pkt1.pkt->duration;
    //should duplicate packet data in DV case 445

    pthread_cond_signal(&cond); //init?

    return 0;
}
int PacketQueue::putNullPacket(AVPacket *pkt, int streamInd)
{
    pkt->stream_index = streamInd;
    return put(pkt);
}
int PacketQueue::hasEnoughtPackets(AVStream *stream, int streadId)
{
    return streadId < 0 || abortReq || (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) || numPackets>MIN_FRAMES && (!duration || av_q2d(stream->time_base) * duration > 1.0);
}