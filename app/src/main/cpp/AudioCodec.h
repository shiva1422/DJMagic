//
// Created by shiva1422 on 5/29/2021.
//

#ifndef DJMAGIC_AUDIOCODEC_H
#define DJMAGIC_AUDIOCODEC_H


#include "Commons.h"

extern "C"
{
#include <libavcodec/codec.h>
#include <libavcodec/avcodec.h>
};
//move below to class?
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESHOLD=4096;
class AudioCodec {
private:

    int len,ret,numChannles=0;
    uint8 inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8 *data;
    size_t dataSize;
    const char *format;
    AVSampleFormat eSampleFormat;
    const AVCodec *codec=nullptr;
    AVCodecContext *codecContext= nullptr;
    AVCodecParserContext *parser= nullptr;
    AVPacket *packet;
    AVFrame *decodedFrame= nullptr;
public:
    AudioCodec();
    static void showAvailable();
    static void registerCodecs();
    static void findCodec();

};


#endif //DJMAGIC_AUDIOCODEC_H
