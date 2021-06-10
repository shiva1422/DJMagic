//
// Created by shiva1422 on 5/29/2021.
//

#include <android/asset_manager.h>
#include "MediaPlayer.h"
#include "FileManager.h"
#include "VideoView.h"
#define MediaLogE(...)((void)__android_log_print(ANDROID_LOG_ERROR,"MEDIA_LOG:",__VA_ARGS__))
#define MediaLogI(...)((void)__android_log_print(ANDROID_LOG_INFO,"MEDIA_LOG:",__VA_ARGS__))

#define VIDEO_BUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#define FF_INPUT_BUFFER_PADDING_SIZE 64
struct KIOContext
{

    uint8 *ptr;//opeque internal for ffmpeg
    size_t size ;
    AVIOContext *kioContext;
    AAsset *asset = nullptr;
};
static int readPacket(void *opaque, uint8 *buf, int bufSize)
{
    AAsset* src=(AAsset *)opaque;//this opaque thing can be any thing that is source of data.
    bufSize=FFMIN(bufSize, VIDEO_BUF_SIZE);//check

    if(!bufSize)
        return AVERROR_EOF;
    //MediaLogI("ptr %%p size:%zu\n",bufSize);

    //fill buf buffer data to buf from AAsset or anything src
   int readSize = AAsset_read(src,buf,bufSize);
   if(readSize == 0)
   {
       return AVERROR_EOF;//also < o on error;
   }
   // src->ptr +=bufSize;//if reading from memeory that is through a pointer change current pointer location and size remaining;
   // src->size -=bufSize;

    return readSize;

}
int64_t kseek(void *opaque,int64_t offset , int whence)
{

    AAsset *asset = (AAsset *)opaque;
    return AAsset_seek(asset,offset,whence);
}
static void avDumpFormatCallback(void *ptr,int level,const char *fmt,va_list v1)//clalback av_dump_format;
{
    if(level>av_log_get_level())
        return;
    va_list v12;
    char line[1024];
    static int print_prefix=1;
    va_copy(v12,v1);
    av_log_format_line(ptr,level,fmt,v12,line,sizeof(line),&print_prefix);//level LOG_LEVEL?
    va_end((v12));
    MediaLogI("File Format %s",line);
}
MediaPlayer::MediaPlayer()
{
    //MediaLogI("the header version LIBAVFORMAT_VERSION is %d and libversion is %s",avformat_version());
    av_log_set_level(32);//do globally and static for av_dump_format with below;
    av_log_set_callback(avDumpFormatCallback);

}
MediaPlayer::~MediaPlayer()
{
    close(fd);//error check?
    delete audioCodec;
    audioCodec= nullptr;
    delete audioTrack;
    audioCodec= nullptr;
}
void MediaPlayer::setError(const char *error)
{
    this->errorString += error;
    MediaLogE("%s",error);
}
status MediaPlayer::setFile(const char *fileLoc)
{
    fileName=fileLoc;
    asset= FileManager::getAsset(fileLoc);
    if(!asset)
    {
       setError("could't open Asset");
       return STATUS_KO_FATAL;
    }

    uint8 *vidBuf=(uint8 *)av_malloc(VIDEO_BUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);//move to header


    //based on type of files set ioContext now for asset;
    ioContext = avio_alloc_context(vidBuf ,VIDEO_BUF_SIZE,0,(void *)asset,readPacket,NULL,kseek);
    if(!ioContext)
    {
        //close AAset ,clear vid buffer
        setError("IOContext alloc failed");
        return STATUS_KO_FATAL;
    }
    //everthing below can be move to the respective MeDiaType THeads;
    if(!initFrameAndPacketQsClocks()<0)
    {
        MediaLogE("INITFPQsClocks success");
    }

    return openFileAndFindStreamInfo();


}
status MediaPlayer::openFileAndFindStreamInfo()//Demuxing
{
    formatContext = avformat_alloc_context();
    if(!formatContext)
    {
        //cllose ioContext;
        setError("FormatContext alloc failed");
        return STATUS_KO_FATAL;
    }
    formatContext->pb=ioContext;
    //must be closed;
    //formatContext->iformat=av_probe_input_format(&probeData,1);
    formatContext->flags=AVFMT_FLAG_CUSTOM_IO;

    if(avformat_open_input(&formatContext, "",NULL,NULL) < 0)
    {
        //clear formatContex;
        setError("OpenInput failed");
        return STATUS_KO_FATAL;
    }
    //check right time for below two
    formatContext->max_analyze_duration=INT64_MAX;
    formatContext->max_ts_probe=INT32_MAX;
    if(avformat_find_stream_info(formatContext,NULL) < 0)
    {
        //fill fmt context->nb streams if the openinput function could not recoginze some streams without headers
        setError("Find Stream Info Failed");
        return STATUS_KO_FATAL;
    }
    totNumStreams=formatContext->nb_streams;
    av_dump_format(formatContext, ANDROID_LOG_INFO, "", 0);//index?
    if(! findStreamsAndOpenCodecs() == STATUS_KO_FATAL)
    {
        return initAndstartCodecs();
    }
     return STATUS_KO_FATAL;

}
status MediaPlayer::findStreamsAndOpenCodecs()
{

    int res=-1;
    status tempStatus=STATUS_OK;
    AVStream *tempStream;
    AVMediaType mediaType=AVMEDIA_TYPE_VIDEO;
    //following should be moved to loop and switch

    res=av_find_best_stream(formatContext,mediaType,-1,-1,&vDecoder,0);
    //check all fun args can open get more codec for streams
    if(res<0)
    {
        if(res==AVERROR_STREAM_NOT_FOUND)
        {
            MediaLogE("could not find %s stream in file %s",av_get_media_type_string(mediaType));

        }
        else if(res==AVERROR_DECODER_NOT_FOUND)
        {
            MediaLogE("found %s stream in file %s but could not find decoder",av_get_media_type_string(mediaType));
        }
        tempStatus=STATUS_KO;
    }
    else
    {//if decoder cant be found should decoderFail flag;todo
        videoStrInd=res;//or can  be passed to above func as well
        tempStream=formatContext->streams[videoStrInd];

        vStream = formatContext->streams[videoStrInd];
        sampleAspectRatio = av_guess_sample_aspect_ratio(formatContext,vStream,NULL);
        MediaLogI("SampleAspectRation : %d/%d",sampleAspectRatio.num,sampleAspectRatio.den);
        if(!vDecoder)
        {
            //can also find by name;
            vDecoder=avcodec_find_decoder(formatContext->streams[videoStrInd]->codecpar->codec_id);
        }
        if(vDecoder)
        {
            //cannot use codeccontext directly from stream so create context and get it from codecparameters.
            vDecodeContext=avcodec_alloc_context3(vDecoder);
          //  vDecodeContext->workaround_bugs=1;
            // avcodec_parameters_copy(vDecodeContext->c)
            res=avcodec_parameters_to_context(vDecodeContext,formatContext->streams[videoStrInd]->codecpar);

            av_log((void *)vDecodeContext, AV_LOG_INFO, "SDFDS");
            if(res<0)
            {
                MediaLogE("OpenCodecs:vid Params to context failed");
            }

            AVDictionary *opts = NULL;

            if((res=avcodec_open2(vDecodeContext,vDecoder,NULL)) < 0)
            {//last option for setting private option for decoder
                MediaLogE("Open %s codec failed",av_get_media_type_string(mediaType));
            }
            else
            {
                bVideoInit=true;
                MediaLogE("video codec opend for stream index %d",videoStrInd);
            }

        }

    }

   /* mediaType=AVMEDIA_TYPE_AUDIO;
    res=av_find_best_stream(formatContext,mediaType,-1,-1,&aDecoder,0);
    if(res<0)
    {
        if(res==AVERROR_STREAM_NOT_FOUND)
        {
            MediaLogE("could not find %s stream in file %s",av_get_media_type_string(mediaType));

        }
        else if(res==AVERROR_DECODER_NOT_FOUND)
        {
            MediaLogE("found %s stream in file %s but could not find decoder",av_get_media_type_string(mediaType));
        }
        tempStatus=STATUS_KO;
    }
    else
    {//if decoder cant be found should decoderFail flag;todo
        audioStrInd=res;//or can  be passed to above func as well
        tempStream=formatContext->streams[audioStrInd];
        if(!aDecoder)
        {
            //can also find by name;
            aDecoder=avcodec_find_decoder(tempStream->codecpar->codec_id);
        }
        if(aDecoder)
        {
            aDecodeContext=avcodec_alloc_context3(aDecoder);
            res=avcodec_parameters_to_context(aDecodeContext,tempStream->codecpar);
            if(res<0)
            {
                MediaLogE("OpenCodecs:aud Params to context failed");
            }
            if((res=avcodec_open2(aDecodeContext,aDecoder,NULL)) < 0)
            {//last option for setting private option for decoder
                MediaLogE("Open %s codec failed",av_get_media_type_string(mediaType));
            }
            else
            {
                bAudioInit=true;
                MediaLogE("audio codec opend");
            }

        }

    }

    mediaType=AVMEDIA_TYPE_SUBTITLE;
    res=av_find_best_stream(formatContext,mediaType,-1,-1,NULL,0);
    if(res<0)
    {
        if(res==AVERROR_STREAM_NOT_FOUND)
        {
            MediaLogE("could not find %s stream in file %s",av_get_media_type_string(mediaType));

        }
        else if(res==AVERROR_DECODER_NOT_FOUND)
        {
            MediaLogE("found %s stream in file %s but could not find decoder",av_get_media_type_string(mediaType));
        }
        tempStatus=STATUS_KO;//STATUS_OK
    }
*/


    return tempStatus;
}
void MediaPlayer::playVideo()
{

    videoFrame=av_frame_alloc();//free
    if(!videoFrame)
        return;
    packet=av_packet_alloc();

    packet->size=0;
    packet->data=NULL;
    if(!packet)
    {
        return;
    }

    int res=0;

  //  res=av_read_frame(formatContext,packet);
    MediaLogI("Play Startd %d error %s",res,av_err2str(res));

    while(av_read_frame(formatContext,packet)>=0)
    {
        MediaLogI("Read packe","succes");
        if(packet->stream_index != videoStrInd)
        {
            continue;//cuz now only decoding video
        }
        res=avcodec_send_packet(vDecodeContext,packet);
        if(res<0)
        {
            MediaLogE("Error sending packet");
            return;
        }
        while(res  >= 0)
        {
            res=avcodec_receive_frame(vDecodeContext,videoFrame);
            if(res == AVERROR(EAGAIN) || res == AVERROR_EOF)
            {
                break;
            }

            else if(res<0)
            {
                MediaLogE("Decode paket failed %s",av_err2str(res));
                return;
            }
            MediaLogE("FrameReeived");
            break;

        }
        av_packet_unref(packet);
        break;


    //    break;
      //  av_packet_unref(packet);
        //got single frame just break loop and display single image for now;

    }
    MediaLogI("Play Startd %d error %s",res,av_err2str(res));
 /*   //use SWS for coverting frame to our requirement for which intermediate frame is req for conversion
    AVFrame *rgbFrame=av_frame_alloc();
    if(!rgbFrame)
        return;

    //specify output for raw data
    uint8 *buffer=NULL;
    int numBytes=getImageBufferSize();
    MediaLogI("the image buffer size is %d",numBytes);
    buffer=(uint8 *)av_malloc(numBytes*sizeof(uint8));//not protected by ffmpeg clear req

    //associate the frame with the allocated buffer;
    //av_image_fill_arrays(rgbFrame->data,rgbFrame->linesize)
    */


}

Bitmap MediaPlayer::getImageParams()
{
   Bitmap bitmap;
   bitmap.height=vDecodeContext->height;
   bitmap.width=vDecodeContext->width;
   Logi("getPar","the width of pic is %d and height is %d",bitmap.height,bitmap.width);
    return bitmap;
}
void* MediaPlayer::readThread(void *mediaPlayer)
{

    MediaPlayer *player=static_cast<MediaPlayer *>(mediaPlayer);//if !mediaPlayer Exit;
    int res=-1,decodeRes=0;
    while(av_read_frame(player->formatContext,player->packet) >= 0)
    {
        //decode audio and video are same byt change in decode_context
        MediaLogE("decoding thread");
        if(player->packet->stream_index == player->videoStrInd)
        {
            //decode video
            res = avcodec_send_packet(player->vDecodeContext,player->packet);
            if(res<0)
            {
                std::string  err="Decoding : Error submit packet " + std::string(av_err2str(res));
                player->setError(err.c_str());
                break;
            }

            //get all the available frames from the decoder
            decodeRes = player->receiveFrame(player);


        }
        else if(player->packet->stream_index == player->audioStrInd)
        {
            //decode audio same as video
        }
        av_packet_unref(player->packet);
        if(decodeRes < 0)
            break;
    }
    MediaLogE("End decoding");
    //av_read_frame()the returned packet is valid until the next av_read_frame() or until av_close_input_file() and must be freed with av_free_packet


}
kforceinline int MediaPlayer::receiveFrame(MediaPlayer * player)
{

    int decodeRes = 0 ;
    while(decodeRes >= 0)
    {

        decodeRes = avcodec_receive_frame(player->vDecodeContext , player->videoFrame);
        if(decodeRes < 0)
        {
            MediaLogE("The Frame is not available");
            if(decodeRes == AVERROR_EOF || decodeRes == AVERROR(EAGAIN))
            {
                //no Errors also no frame output available
                decodeRes = 0;
                return decodeRes;

            }
            //else player will break
            player->setError((std::string("Error during decoding ") + std::string(av_err2str(decodeRes))).c_str());
            return decodeRes;

        }
        MediaLogE("decoding thread Check");


        //write frame data to destination;based on type of codec context audio or video;
        //player->outputView->updateFromDecoded();
        uint8 *out = nullptr;

        pthread_mutex_lock(&player->mutex);
        //player->ptsSecs = av_q2d(player->vStream->time_base) * player->videoFrame->pts;
        //player->vidFrameReady = true;
       // MediaLogE("decoding thread Check");
        while(!(out = player->outputView->buf))
        {
          //  MediaLogE("waiting");
            pthread_cond_wait(&player->condContReadThread, &player->mutex);
          //  MediaLogE("Got signal and buffer");

        }
        pthread_mutex_unlock(&player->mutex);


        if(player->fillOutput(player,out))
        av_frame_unref(player->videoFrame);



    }
    return decodeRes;

}
kforceinline bool MediaPlayer::fillOutput(MediaPlayer *player, uint8* out)
{

    if(out)
    {
        for(int x = 0;x <player->videoFrame->width;++x)
        {
            for(int y=0 ; y < player->videoFrame->height ; ++y)
            {
                // MediaLogE("the width adn height %d and %d",videoFrame->width,videoFrame->height);
                out[y * player->videoFrame->width * 4 + x * 4 ]     = player->videoFrame->data[0][ y * player->videoFrame->linesize[0] + x];
                out[y * player->videoFrame->width * 4 + x * 4 + 1 ] = player->videoFrame->data[0][ y * player->videoFrame->linesize[0] + x];
                out[y * player->videoFrame->width * 4 + x * 4 + 2]  = 255;//player->videoFrame->data[0][ y * player->videoFrame->linesize[0] + x];
                out[y * player->videoFrame->width * 4 + x * 4 + 3]  = 255;
            }
        }

    }
    else
        return false;

    out= nullptr;
  //  player->printFrameInfo(player->vDecodeContext ,player->videoFrame);
    pthread_mutex_lock(&player->mutex);
    MediaLogE("Texture filled");
  //  player->vidFrameReady=false;
    player->outputView->texUpdate=true;
   // player->outputView->pts = player->videoFrame->pts;
  //  player->outputView->ptsSecs = player->videoFrame->pts * (double) player->vDecodeContext->time_base.num/(double)player->vDecodeContext->time_base.den;
    //MediaLogE("Presentation time is secs is %lf timebaseNum %d and timebase den %d",player->outputView->ptsSecs,player->vDecodeContext->time_base.num,player->vDecodeContext->time_base.den);
  //  pthread_cond_signal(&player->condContReadThread);
    pthread_mutex_unlock(&player->mutex);

return true;

}
status MediaPlayer::start()
{
    videoFrame=av_frame_alloc();//free
    if(!videoFrame)
        return STATUS_KO_FATAL;
    packet=av_packet_alloc();
    if(!packet)
    {
        return STATUS_KO_FATAL;
    }

    packet->size=0;
    packet->data=NULL;

    if(pthread_create(&thread, NULL, readThread, (void *) this) != 0)
        return STATUS_KO;
    return STATUS_OK;

}


void MediaPlayer::printFrameInfo(AVCodecContext* codecContext,AVFrame *frame)
{
    double  timebase = av_q2d(vStream->time_base);

    MediaLogI("\\n timeUnit is %lf",frame->pts*timebase);
    MediaLogI("Frame - %c frameNo -%d , , pts - %lu , dts - %d , keyFrame %d ,repeatPicCount %d, codedPictureNum %d , displayPictureNum %d ",av_get_picture_type_char(frame->pict_type),
              codecContext->frame_number,frame->pts,frame->pkt_dts,frame->key_frame,frame->repeat_pict,frame->coded_picture_number,frame->display_picture_number);

}
int MediaPlayer::initFrameAndPacketQsClocks()
{
    //FrameQ init


    if(picFQ.init(&vidPktQ,VIDEO_PICTURE_QUEUE_SIZE,1) < 0 || sampleFQ.init(&audPktQ , SAMPLE_QUEUE_SIZE , 1) < 0 || subPicFQ.init(&subPktQ, SUBPICTURE_QUEUE_SIZE,0)<0)
    {
        MediaLogE("initFrameAndPacketQsClocks"," frameQ init failed");
        goto fail;
    }
    //packetQ init

    if(vidPktQ.init() < 0 || audPktQ.init() < 0 || subPktQ.init()<0)
    {
        MediaLogE("initFrameAndPacketQsClocks"," packetQ init failed");
        goto fail;
    }

    audClock.init(&audPktQ.serial);
    vidClock.init(&vidPktQ.serial);
    extClock.init(&extClock.serial);
    MediaLogI("initFPQsClocks success;");
    return 0;
    fail:
    clearResources();
    return -1;
}
status MediaPlayer::initAndstartCodecs()
{
    //for now just vid;//if fail cclear contextxts;
    if(vidDec.init(vDecodeContext,&vidPktQ,&condContReadThread))
        return STATUS_KO_FATAL;
    MediaLogI("init and StartCodecs success");
    return STATUS_OK;

}
void *MediaPlayer ::videoThread(void *mediaPlayer)
{
    MediaPlayer *player = (MediaPlayer *)mediaPlayer;
    AVFrame *frame = av_frame_alloc();
    double pts;
    double duration;
    int ret;
    AVRational tb = player->vStream->time_base;
    AVRational frameRate = av_guess_frame_rate(player ->formatContext , player->vStream,NULL);
    if(!frame)
    {
        MediaLogE("Videothread frame alloc faield");
        ret = AVERROR(ENOMEM);
        goto theEnd;
    }

    while(true)
    {

        ret = player->getVideoFrame(frame);
        if(ret < 0)
            goto theEnd;
        if(!ret)
            continue;

        duration = (frameRate.num && frameRate.den ? av_q2d(((AVRational){frameRate.den,frameRate.num})) : 0);
        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        ret = player->queuePicture(frame , pts ,duration,frame->pkt_pos,player ->vidDec.packetSerial);
        av_frame_unref(frame);
        if(ret < 0)
        {
            goto theEnd;
        }

    }

    theEnd:
    av_frame_free(&frame);
}
int MediaPlayer::getVideoFrame(AVFrame *frame)
{
    int gotPicture;
    if((gotPicture = decoderDecodeFrame(&this->vidDec,frame,NULL)) < 0)
        return -1;

    if(gotPicture)
    {
        double dpts = NAN;
        if(frame->pts != AV_NOPTS_VALUE)
            dpts =av_q2d(vStream->time_base) * frame->pts;
        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(formatContext,vStream,frame);

        if(frameDrop > 0 || (frameDrop && ))
    }
}
int MediaPlayer::decoderDecodeFrame(Codec *codec, AVFrame *frame, AVSubtitle *sub)
{

}
int MediaPlayer::queuePicture(AVFrame *srcFrame, double pts, double duration, int64 pos,int serial)
{

}
void MediaPlayer::clearResources()
{
    close(fd);
    avcodec_close(vDecodeContext);
    avcodec_close(aDecodeContext);
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    av_free(ioContext);

    //free contexts ,packet,
}
//free
//contexts,