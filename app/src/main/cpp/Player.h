//
// Created by shiva1422 on 6/6/2021.
//

#ifndef DJMAGIC_PLAYER_H
#define DJMAGIC_PLAYER_H


#include "Codec.h"
#include "string"
using namespace std;
class Player {
private:
    string fileName;
    AAsset *asset;
    AVIOContext  *ioContext;
    AVInputFormat *fileFormat = nullptr;
    VideoState *videoState;
    bool bInit=false;
    int avSyncType = AV_SYNC_AUDIO_MASTER;//default;
    string errString;
    int openStreams();
    int closeStreams();
    int initFrameQueues(FrameQueue *frameQueue , PacketQueue *packetQueue ,int maxSize ,int keepLast);
    int initPacketQueues(PacketQueue *queue);
    void initClock(Clock *c,int *queueSerial);
    void setClock(Clock *c, double pts, int serial);
    void setClockAt(Clock *c,double pts,int serial,double time);
    static void *readThread(void *vidState);

public:
    Player(){}
    Player(const char *fileName);



};


#endif //DJMAGIC_PLAYER_H
