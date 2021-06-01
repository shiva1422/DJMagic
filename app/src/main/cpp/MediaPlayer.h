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

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#define FF_INPUT_BUFFER_PADDING_SIZE 64

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

    //Codec
    AVIOContext *ioContext=NULL;
    const AVCodec *aDecoder= NULL,*vDecoder=NULL;
    AVCodecContext *aDecodeContext=NULL,*vDecodeContext=NULL;
    AVStream *vStream=NULL,*aStream=NULL;
    AVFrame  *frame=NULL, *videoFrame=NULL;
    AVPacket *packet;
   // SwsContext *sws;for scaling color conversions.
   //Player
   uint8 inbuf[AUDIO_INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];//paddng size - to compensate for sometimes read over the end

    status findStreamsAndOpenCodecs();
    int openCodecsFromFormat(int *streamIndex,AVMediaType type);
    void playAudio();
    void playVideo();


public:
    MediaPlayer();
    ~MediaPlayer();
    status openFileAndFindFormat(const char* audioFileLoc);
    void configureCodec();
    void configureTrack();
    void play();
    void pause();
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
