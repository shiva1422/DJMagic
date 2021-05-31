//
// Created by shiva1422 on 5/29/2021.
//

#include "MediaCodec.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}
#include "android/log.h"
#include<map>
#define CodecLogE(...)((void)__android_log_print(ANDROID_LOG_ERROR,"AV_LOG:",__VA_ARGS__))
#define CodecLogI(...)((void)__android_log_print(ANDROID_LOG_INFO,"AV_LOG:",__VA_ARGS__))

MediaCodec::MediaCodec()
{
    packet=av_packet_alloc();

    codec=avcodec_find_decoder(AV_CODEC_ID_MP2);//find mpeg ANY decoder;
    if(!codec)
    {
        CodecLogE("MediaCodec : cannot find decoder");////abort app assert?
        return;
    }

    parser=av_parser_init(codec->id);
    if(!parser)
    {
        CodecLogE("MediaCodec : Parser not found");
        return;////abort app assert?
    }

    codecContext=avcodec_alloc_context3(codec);
    if(!codecContext)
    {
        CodecLogE("MediaCodec : Could not allocate audio codec context");
        return;////abort app assert?
    }

    if(avcodec_open2(codecContext,codec,NULL)<0) //open Codec;
    {
        CodecLogE("MediaCodec : Could not openg Codec");
        return;////abort app assert?
    }
    CodecLogI("MediaCodec:codec open success");


}

void MediaCodec::registerCodecs()
{
    //now no need;
}
void MediaCodec::showAvailableCodecs()
{
    AVCodec *iteration= nullptr;
    CodecLogI("CodecNames :: start............");

    while(true)
    {
        AVCodec* presentCodec = const_cast<AVCodec *>(av_codec_iterate((void **) (&iteration)));


        if(presentCodec)//for  video 0;
        {
            const char* mediaType;
            if(presentCodec->type==AVMEDIA_TYPE_AUDIO) {
                mediaType = "audio";
            }
            else if(presentCodec->type==AVMEDIA_TYPE_VIDEO)
            {
                mediaType="video";
            }
            else
                mediaType="unknown";//text,data,nb

                CodecLogI("decoder: %s , encoder:%s ,CodecName : %s ,MediaType is %s ", av_codec_is_decoder(presentCodec) ? "YES" : "NO", av_codec_is_encoder(presentCodec) ? "YES" : "NO", presentCodec->name,mediaType);
            // AVCodecDescriptor *descriptor=(AVCodecDescriptor *)avcodec_descriptor_get(presentCodec->id);
            //CodecLogI("descriptor name %s and long name : %s",descriptor->name,descriptor->long_name);


        }
        else
            break;


    }
    CodecLogI("CodecNames :: end.........................");
}
void MediaCodec::findCodec()
{
    const AVCodec *codec=avcodec_find_decoder(AV_CODEC_ID_MPEG4);
    if(codec)
    {
        if(av_codec_is_decoder(codec))
        {
            CodecLogI("aac decoder found and type is %d", codec->type);
        }
        if(av_codec_is_encoder(codec))
        {
            CodecLogI("aac encoder found");
        }
    }
    else
    {
        CodecLogI("codec not found");
    }



}