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
#include <libavutil/common.h>
};

#include "AudioTrack.h"
#include "MediaCodec.h"

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#define FF_INPUT_BUFFER_PADDING_SIZE 64

class MediaPlayer {
private:

    int fd=-1;
    bool bPlaying=false;
    const char *fileName=nullptr;
    AudioTrack *audioTrack= nullptr;
    MediaCodec *audioCodec= nullptr;
    //Codec
    int audioStrInd=-1;
    int videoStrInd=-1;//later  multiple streams
    const AVCodec *aDecoder= nullptr,*vDecoder=nullptr;
    AVFormatContext *formatContext=nullptr;
    AVCodecContext *aDecodeContext=nullptr,*vDecodeContext=nullptr;
    AVStream *vStream=nullptr,*aStream=nullptr;
    AVFrame* frame;
    AVPacket packet;
   // SwsContext *sws;for scaling color conversions.
   //Player
   uint8 inbuf[AUDIO_INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];//paddng size - even if some time read over the end


    int openCodecsFromFormat(int *streamIndex,AVMediaType type);
    void playAudio();


public:
    MediaPlayer();
    ~MediaPlayer();
    status setAudioFileFromAssets(const char* audioFileLoc);
    void configureCodec();
    void configureTrack();
    void play();
    void pause();
    void decodeAudio(AVPacket *packet,AVFrame *frame);
    /*
     * meths
     *confiure codecs,
     *
     */




};


#endif //DJMAGIC_MEDIAPLAYER_H
