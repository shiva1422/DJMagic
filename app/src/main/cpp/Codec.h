//
// Created by shiva1422 on 6/6/2021.
//

#ifndef DJMAGIC_CODEC_H
#define DJMAGIC_CODEC_H

#include <android/asset_manager.h>
#include "Commons.h"
#include "string"
extern "C"
{
#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/fifo.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavutil/bprint.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"

}

/* Minimum OBOE audio buffer size, in samples. */
#define OBOE_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define OBOE_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control in dB */
#define OBOE_VOLUME_STEP (0.75)

#define MIN_FRAMES 25
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TOD: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)
#define AV_NOSYNC_THRESHOLD 10.0

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold*/
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001



enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};
typedef struct MyAVPacketList {
    AVPacket *pkt = nullptr;
    int serial;
} MyAVPacketList;

enum ShowMode {
    SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
} ;
typedef int64_t int64;
typedef int16_t int16;
class MediaPlayer;
class PacketQueue{
    AVFifoBuffer *packetList = nullptr;
    int numPackets;
    int size;
    int64 duration;
    int abortReq;
    int serial;
    pthread_cond_t cond;
    pthread_mutex_t mutex;

    int putPrivate(AVPacket *pkt);
public:
    friend class Player;
    friend class MediaPlayer;
    friend class FrameQueue;
    int init();
    void start();
    void flush();
    int put(AVPacket *pkt);
    int putNullPacket(AVPacket *pkt , int streamInd);
    int hasEnoughtPackets(AVStream *stream, int streadId);
};
class AudioParams{
public:
    int freq;
    int channels;
    int64 channelLayout;
    enum AVSampleFormat format;
    int frameSize;
    int bytesPerSec;
public:
    friend class Player;
};
class Clock{
    double pts;//clockBase
    double ptsDrift; //clock base - time at which clock is updated;
    double lastUpdated;
    double speed;
    int serial; //clock is based on a packet with this seriral
    int paused;
    int *queueSerial ;//pointer to current packet queue serial,used for obsolete clock detection ;
public:
    friend class Player;
    friend class MediaPlayer;
    void init(int *qSerial);
    void setClock(double pts, int serial);
    void setClockAt(double pts,int serial,double time);
    void setSpeed(double speed);
    double getValue();
};
/* Common struct for handling all types of decoded data and allocated render buffers. */
class Frame{
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;
    double duration; //estimated duration of frame;
    int64 pos; //byte position of frame in input  file;
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flipV;

public:

    friend class Player;
    friend class MediaPlayer;
    friend class FrameQueue;
    friend class VideoView;
};
class FrameQueue{
    Frame frameQueue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size =0;
    int maxSize;
    int keepLast;
    int rindexShown;
    pthread_mutex_t  mutex;////////init
    pthread_cond_t cond;
    PacketQueue *packetQueue;

public:
    void push();
    int init(PacketQueue *pq,int maxSize,int keepLast);
    friend class Player;
    friend class MediaPlayer;
    Frame *peekWritable();
    Frame *peekReadable();
    Frame* peek();
    Frame* peekLast();
    Frame* peekNext();
    int getRemainingCount();//undisplayed count;
    int queuePicture(AVFrame *srcFrame, double pts, double duration, int64 pos,int serial);
    void next();
    void unrefItem(Frame *frame);//write inline check;
};
class Codec {
private:
    AVPacket *packet = nullptr;
    PacketQueue *packetQueue ;
    AVCodecContext  *codecContext;
    int packetSerial;
    int finished;
    int packetPending;
    //conditional for emptyqueue;
    int64 startPts;
    AVRational startPtsTB;
    int64 nextPts;
    AVRational nextPtsTB;
    pthread_t  thrId;
    pthread_cond_t  *condEmptyQ ;//this is init from player;
    //decoder threadID;
public:
    friend class MediaPlayer;
    int init(AVCodecContext* codecContext,PacketQueue *packetQueue ,pthread_cond_t *emptyQueueCond);
    int start(void* (*fn)(void *),MediaPlayer *player);//return bool check simlar funcs

};

class VideoState{
    //read thread id;
    const AVInputFormat  *inputFormat;
    int abortReq;
    int forceRefresh;
    int paused;
    int lastPaused;
    int queueAttachmentsReq;
    int seekReq;
    int seekFlags;
    int64 seekPos;
    int64 seekRel;
    int readPauseReturn;
    AVFormatContext  *formatContext;
    int realTime;

    Clock audioClock;
    Clock videoClock;
    Clock extClock;

    FrameQueue picQ;
    FrameQueue subPicQ;
    FrameQueue sampleQ;

    Codec audCodec;
    Codec vidCodec;
    Codec subCodec;

    int audioStreamIn = -1;
    int avSyncType;

    double audioClockdb;
    int audioClockSerial;
    double audioDiffCum;//used for av difference average computation
    double audioDiffAvgCoef;
    double audioDiffThreshold;
    int audioDifAvgCount;
    AVStream *audioStream;
    PacketQueue audioPktQ;
    int audioHwBufSize;
    uint8 *audioBuf;
    uint8 *audioBuf1;
    unsigned int audiBufSize;//bytes;
    unsigned int audioBuf1Size;
    int audioBufIndex;//bytes
    int audioWriteBufSize;
    int audioVolume;
    int muted;
    AudioParams audioParamsSrc;//source;//254 AVFiltercheck;
    AudioParams audioParamsTgt;
    SwrContext *swrContext;
    int frameDropsEarly;
    int  frameDropsLate;
    ShowMode showMode;
    int16 sampleArray[SAMPLE_ARRAY_SIZE];
    int sampleArrayIndex;
    int lastIStart;
    RDFTContext *rdftContext;
    int rdftBits;
    FFTSample *rdftData;
    int xPos;
    double lastVisTime;
    //vistextures,subtexture,vidTexture;

    int subtitleStreamIn = -1;
    AVStream *subtitleStream;
    PacketQueue subPktQ;

    double frameTimer;
    double frameLastReturnedTime;
    double frameLastFilterDelay;

    int vidStreaIn = -1;
    AVStream *videoStream;
    PacketQueue vidPktQ;//286
    double maxFrameDuration; //above this ,we consider the jump a timestamp discountinuity;
    SwsContext *imgConvertCtx;
    SwsContext *subConvertCtx;
    int eof;

    char *filename;
    int width ,height,xleft = 0,ytop = 0;
    int step;
    ///AVILTER;296

    int lastVidStream = -1,lastAudStream = -1,lastSubStream = -1;
     pthread_t  *readThread;

     //
     AVIOContext *ioContext;
     AAsset *asset;
     std::string fileName;

public:
    friend class Player;


};


#endif //DJMAGIC_CODEC_H
