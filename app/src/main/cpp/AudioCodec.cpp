//
// Created by shiva1422 on 5/29/2021.
//

#include "AudioCodec.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}
#include "android/log.h"
#define AVLoge(...)((void)__android_log_print(ANDROID_LOG_ERROR,"AV_LOG:",__VA_ARGS__))
#define AVLogi(...)((void)__android_log_print(ANDROID_LOG_INFO,"AV_LOG:",__VA_ARGS__))


AudioCodec::AudioCodec()
{
    packet=av_packet_alloc();

    codec=avcodec_find_decoder(AV_CODEC_ID_MP2);//find mpeg ANY decoder;
    if(!codec)
    {
        AVLoge("AudioCodec : cannot find decoder");////abort app assert?
        return;
    }

    parser=av_parser_init(codec->id);
    if(!parser)
    {
        AVLoge("AudioCodec : Parser not found");
        return;////abort app assert?
    }

    codecContext=avcodec_alloc_context3(codec);
    if(!codecContext)
    {
        AVLoge("AudioCodec : Could not allocate audio codec context");
        return;////abort app assert?
    }

    if(avcodec_open2(codecContext,codec,NULL)<0) //open Codec;
    {
        AVLoge("AudioCodec : Could not openg Codec");
        return;////abort app assert?
    }
    AVLogi("AudioCodec:codec open success");


}

void AudioCodec::registerCodecs()
{
    //now no need;
}
void AudioCodec::showAvailable()
{

   AVCodec *iteration= nullptr;
    AVLogi("CodecNames :: start............");

    while(true)
    {
        AVCodec* presentCodec = const_cast<AVCodec *>(av_codec_iterate((void **) (&iteration)));


        if(presentCodec)//for  video 0;
        {
            if(presentCodec->type==1)
            AVLogi("decoder: %s , encoder:%s ,CodecName : %s ,MediaType is %d ",av_codec_is_decoder(presentCodec)?"YES":"NO",av_codec_is_encoder(presentCodec)?"YES":"NO",presentCodec->name,presentCodec->type);
           // AVCodecDescriptor *descriptor=(AVCodecDescriptor *)avcodec_descriptor_get(presentCodec->id);
            //AVLogi("descriptor name %s and long name : %s",descriptor->name,descriptor->long_name);


        }
        else
            break;


    }
    AVLogi("CodecNames :: end.........................");

}
void AudioCodec::findCodec()
{
    const AVCodec *codec=avcodec_find_decoder(AV_CODEC_ID_MPEG4);
    if(codec)
    {
        if(av_codec_is_decoder(codec))
        {
            AVLogi("aac decoder found and type is %d",codec->type);
        }
        if(av_codec_is_encoder(codec))
        {
            AVLogi("aac encoder found");
        }
    }
    else
    {
        AVLogi("codec not found");
    }



}