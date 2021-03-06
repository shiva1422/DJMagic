//
// Created by shiva1422 on 5/29/2021.
//

#ifndef DJMAGIC_MEDIAPLAYER_H
#define DJMAGIC_MEDIAPLAYER_H

extern "C"
{
#include <libavcodec/codec.h>
#include <libavcodec/avcodec.h>
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include "libavutil/frame.h"
#include <libavutil/common.h>
};

#include "AudioTrack.h"
#include "MediaCodec.h"
#include "Bitmap.h"
#include "string"
#include "Codec.h"
#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#define FF_INPUT_BUFFER_PADDING_SIZE 64

class VideoView;
class MediaPlayer {
private:

    int fd=-1;

    bool bPlaying=false,bAudioInit=false,bVideoInit=true;
    MediaCodec *audioCodec= nullptr;
    int videoFrameCount=0,audioFrameCount=0;

    // IO Format and Stream
    uint totNumStreams=0,totNumAudioStreams=0,totNumVideoStreams=0;
    int audioStrInd=-1;//(type from above);
    int videoStrInd=-1;//later  multiple streams
    AAsset *asset=nullptr;
    uint8 *buffer = NULL, *avioContextBuf = NULL;
    size_t bufSize , avioContextBufSize = 4096;
    AVRational sampleAspectRatio;
    //New
    std::string fileName;
    uint8 *inputBuf = nullptr;
    bool bPlayerInit = false;
    const AVInputFormat *inFormat = nullptr;//need init before open format contex
    AVIOContext *avioContext = NULL;

    AVFormatContext *formatContext=nullptr;

    //MasterRead THread;
    AVPacket *pkt =NULL;

    //Picture Props:
     int width = 0,height =0,xleft =0,ytop=0;
    //Codec
    AVIOContext *ioContext = NULL;
    const AVCodec *aDecoder = NULL,*vDecoder = NULL;
    AVCodecContext *aDecodeContext = NULL,*vDecodeContext = NULL;
    AVStream *vStream = NULL,*aStream = NULL;
    AVFrame  *frame = NULL, *videoFrame = NULL;
   // SwsContext *sws;for scaling color conversions.
   //Player



    Codec audDec,vidDec,subDec;



    FrameQueue picFQ,subPicFQ,sampleFQ;

    PacketQueue audPktQ,vidPktQ,subPktQ;


    //time and state
    int eof = 0 , abortRequest = 0 ,realTime;//abortReq check;
    int audClockSer = -1;
    int queueAttachmentsReq = 0,infiniteBuf =0 ,paused = 0,lastPaused = 0,readPauseReturn;//bool?
    //seek
    int seekReq = 0,seekFlags = 0 , step = 0;//seek flag change based on type seek
    int64 seekPos,seekRel;
    int frameDrop = -1,frameDropsEarly = 0,frameDropsLate = 0 ;//check if ==0
    int64 startTime = AV_NOPTS_VALUE,duration = AV_NOPTS_VALUE;//check relevance;in startReading; may be for  loop start_time is for inital seek 2847
    int decoderReorderPts = -1;//check -1
    int avSyncType = AV_SYNC_AUDIO_MASTER;
    double maxFrameDuration; //above this ,we consider the jump a timestamp discountinuity;
    double frameTimer = 0.0,frameLastReturnedTime =0.0,frameLastFilterDelay = 0.0;//check init
    bool vidDisable = true,audDisable = true ,subDisable = true;
    Clock audClock,vidClock,extClock;

    int getMasterSyncType();//to clock
    double getMasterClock();//to clock current master ClockValue
    void checkExtClkSpeed();
    double getVidPicDuration(Frame *pic,Frame *nexPic);
    double getMasterClkValue();
    double computeTargetDelay(double delay);
    void updateVideoPts(double pts,int64 pos,int serial);
    void syncClockToSlave(Clock *c, Clock * slave);


    int packetQget(PacketQueue *packetQueue, AVPacket *packet,int block ,int *serial);//to packetQ
    int packetQPutPrivate(PacketQueue *packetQueue,AVPacket *packet);
    //Audio

    //streamInds

     int vidStr = -1,audStr = -1 ,subStr = -1,lastVidStr = -1,lastAudStr = -1,lastSubStr = -1;
   //Threading && OutPut
   friend class VideoView;
   VideoView *outputView = nullptr;
   bool vidFrameReady=false;
   double ptsSecs=0.0;
   pthread_t thread;
   pthread_attr_t *threadAttribs;
   pthread_t readThr;
   pthread_cond_t condContReadThread ;
   pthread_mutex_t mutWait,mutex = PTHREAD_MUTEX_INITIALIZER,mutReadWait = PTHREAD_MUTEX_INITIALIZER;//destroy all thread using this done;
   void printFrameInfo(AVCodecContext* codecContext , AVFrame* frame);

    //control
    bool isStarted = false;//till first frame
    bool isEnded = false;//after lastFrame;

    //New


    //Audio;
        //from stream
    int sampleRate,numChannels;
    uint64_t channelLayout;

    int audioBufSizeHW;
    uint8 *audioBuf;//channel 1 and below is channel 2
    uint8 *audioBuf2;
    int numSamples = 0;
    unsigned int audioBufSize = 0,audioBuf2Size;
    int audioBufIndex =0;//in bytes
    int audioWriteBufSize;
    int audioVolume;
    bool muted = false;
    struct AudioParams audioSrcParams;
    struct AudioParams audioTargetParams;
    //swrContext format convert;
    int16 sampleArray[SAMPLE_ARRAY_SIZE];
    int sampleArrayIndex;
    int lastIStart;
    int xPos;
    double lastVisTime;
        //out
    int64 audioCallbackTime;
    int audioDiffAvgCount = 0 , audioClockSerial = 0;//ser?
    double audioDiffCum = 0.0,audioDiffAvgCoef = 0 ,audDiffThreshold = 0.0,audioClock;
    friend class AudioTrack;
    AudioTrack *audioTrack;
    static void *audioThread(void *mediaPlayer);
    int decodeAudioFrame();
    void audioCallback(void *audioData, int32 numFrames);
    int synchronizeAudio(int numSamples);








    status initFrameAndPacketQsClocks();
    status openInputFindStreamInfo();
    status openStreamsAndCodecs();
    status initAndStartCodecs();
    int startReading();
    void stepToNextFrame();
    void streamTogglePause();

    //display
    int forceRefresh = 0;
    void onRefresh(double *remainingTime);
    void videoDisplay();



    int  receiveFrame(MediaPlayer *player);
    bool fillOutput(MediaPlayer *player, uint8* out);

    void setError(const char *error);
    static void *videoThread(void *mediaPlayer);
    static void *subtitleThread(MediaPlayer *player);
    int getVideoFrame(AVFrame *frame);
    int decoderDecodeFrame(Codec *codec , AVFrame *frame ,AVSubtitle *sub);//to codec
    static void* masterReadThread(void *mediaPlayer);
    static int decodeInterruptCallBack(void *ctx);









public:
    std::string errorString;
    MediaPlayer();
    ~MediaPlayer();
    void configureCodec();
    void configureTrack();
    void play();
    void pause();
   // friend class Codec;
    status setFile(const char* fileLoc);
    void decodeAudio(AVPacket *packet,AVFrame *frame);
    void clearResources();
    Bitmap getImageParams(); //later can be used based on image format;just gives image params not p
    bool getFrame(void *dest, Bitmap bitmapParams);
    static AudioFormat getAudioFormatFromAVSampleFormat(AVSampleFormat sampleFormat);


    /*
     *meths
     *confiure codecs,
     *
     */




};


#endif //DJMAGIC_MEDIAPLAYER_H
