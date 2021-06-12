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
    const char *fileName=nullptr;
    AudioTrack *audioTrack= nullptr;
    MediaCodec *audioCodec= nullptr;
    int videoFrameCount=0,audioFrameCount=0;

    // IO Format and Stream
    uint totNumStreams=0,totNumAudioStreams=0,totNumVideoStreams=0;
    int audioStrInd=-1;//(type from above);
    int videoStrInd=-1;//later  multiple streams
    AAsset *asset=nullptr;
    AVFormatContext *formatContext=NULL;
    AVIOContext *avioContext = NULL;
    uint8 *buffer = NULL, *avioContextBuf = NULL;
    size_t bufSize , avioContextBufSize = 4096;
    AVRational sampleAspectRatio;

    //Codec
    AVIOContext *ioContext = NULL;
    const AVCodec *aDecoder = NULL,*vDecoder = NULL;
    AVCodecContext *aDecodeContext = NULL,*vDecodeContext = NULL;
    AVStream *vStream = NULL,*aStream = NULL;
    AVFrame  *frame = NULL, *videoFrame = NULL;
    AVPacket *packet;
   // SwsContext *sws;for scaling color conversions.
   //Player
   uint8 inbuf[AUDIO_INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];//paddng size - to compensate for sometimes read over the end
   //time and state
   int frameDrop = -1,frameDropsEarly = 0,frameDropsLate = 0 ;double frameLastFilterDelay = 0.0;//check if ==0
   int decoderReorderPts = -1;//check -1
   int avSyncType = AV_SYNC_AUDIO_MASTER;
   int getMasterSyncType();//to clock
   double getMasterClock();//to clock current master ClockValue


    Codec audDec,vidDec,subDec;

    Clock audClock,vidClock,extClock;

    FrameQueue picFQ,subPicFQ,sampleFQ;

    PacketQueue audPktQ,vidPktQ,subPktQ;


    int initFrameAndPacketQsClocks();
    status initAndstartCodecs();
    int packetQget(PacketQueue *packetQueue, AVPacket *packet,int block ,int *serial);//to packetQ
    int packetQPutPrivate(PacketQueue *packetQueue,AVPacket *packet);
    //Audio


   //Threading && OutPut
   friend class VideoView;
   VideoView *outputView = nullptr;
   bool vidFrameReady=false;
   double ptsSecs=0.0;
   pthread_t thread;
   pthread_attr_t *threadAttribs;
   pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER,mutReadWait = PTHREAD_MUTEX_INITIALIZER;//destroy all thread using this done;
   pthread_cond_t condContReadThread = PTHREAD_COND_INITIALIZER;
   static void *readThread(void *mediaPlayer);
   void printFrameInfo(AVCodecContext* codecContext , AVFrame* frame);

    //control
    bool isStarted = false;//till first frame
    bool isEnded = false;//after lastFrame;

    status openFileAndFindStreamInfo();
    status findStreamsAndOpenCodecs();
    status start();
    void playAudio();
    void playVideo();
    int  receiveFrame(MediaPlayer *player);
    bool fillOutput(MediaPlayer *player, uint8* out);

    void setError(const char *error);
    static void *videoThread(void *mediaPlayer);
    static void *audioThread(MediaPlayer *player);
    static void *subtitleThread(MediaPlayer *player);
    int getVideoFrame(AVFrame *frame);
    int queuePicture(AVFrame *srcFrame , double pts , double duration ,int64 pos, int serial);
    int decoderDecodeFrame(Codec *codec , AVFrame *frame ,AVSubtitle *sub);//to codec
    Frame* frameQueuePeekReadable(FrameQueue *f);//to frame
    Frame* frameQueuePeekWritable(FrameQueue *f);//to frame




public:
    std::string errorString;
    MediaPlayer();
    ~MediaPlayer();
    void configureCodec();
    void configureTrack();
    void play();
    void pause();
    status setFile(const char* fileLoc);
    void decodeAudio(AVPacket *packet,AVFrame *frame);
    void clearResources();
    Bitmap getImageParams(); //later can be used based on image format;just gives image params not p
    bool getFrame(void *dest, Bitmap bitmapParams);


    /*
     * meths
     *confiure codecs,
     *
     */




};


#endif //DJMAGIC_MEDIAPLAYER_H
