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
    return findStreamsAndOpenCodecs();

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
void* MediaPlayer::threadFunc(void *mediaPlayer)
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
                        break;
                    }
                    //else player will break
                    player->setError((std::string("Error during decoding ") + std::string(av_err2str(res))).c_str());
                    break;

                }

                //write frame data to destination;based on type of codec context audio or video;
                //player->outputView->updateFromDecoded();

                pthread_mutex_lock(&player->mutex);
                pthread_cond_wait(&player->frameSignalCond,&player->mutex);

                while(!player->outputView->frameRequired)
                {
                    pthread_cond_wait(&player->frameSignalCond,&player->mutex);

                }
                pthread_mutex_unlock(&player->mutex);

                uint8 *out=(uint8 *) player->outputView->buf;

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

                out= nullptr;

                pthread_mutex_lock(&player->mutex);
                player->outputView->frameRequired=false;
                player->outputView->textureUpdateRequired=true;
                pthread_mutex_unlock(&player->mutex);



                av_frame_unref(player->videoFrame);



            }

        }
        else if(player->packet->stream_index == player->audioStrInd)
        {
            //decode audio same as video
        }
        av_packet_unref(player->packet);
        if(decodeRes < 0)
            break;
    }
    //av_read_frame()the returned packet is valid until the next av_read_frame() or until av_close_input_file() and must be freed with av_free_packet


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

    if(pthread_create(&thread,NULL,threadFunc,(void *)this) != 0)
        return STATUS_KO;
    return STATUS_OK;

}
bool MediaPlayer::getFrame(void *dest, Bitmap bitmapParams)
{
    int res;
    av_read_frame(formatContext,packet);
        MediaLogI("Read packe","succes");
        if(packet->stream_index != videoStrInd)
        {
            av_packet_unref(packet);
        }
        else
        res=avcodec_send_packet(vDecodeContext,packet);
        if(res<0)
        {
            MediaLogE("Error sending packet");

        }
        res=avcodec_receive_frame(vDecodeContext,videoFrame);
        if(res!=0)
        {
            av_packet_unref(packet);
            return false;
        }






        //    break;
        //  av_packet_unref(packet);
        //got single frame just break loop and display single image for now;


 //   MediaLogE("update frame","success");
    uint8 *out=(uint8 *)dest;


   for(int x = 0;x < videoFrame->width;++x)
   {
       for(int y=0 ; y < videoFrame->height ; ++y)
       {
          // MediaLogE("the width adn height %d and %d",videoFrame->width,videoFrame->height);
           out[y * videoFrame->width * 4 + x * 4 ]     = videoFrame->data[0][ y * videoFrame->linesize[0] + x];
           out[y * videoFrame->width * 4 + x * 4 + 1 ] = videoFrame->data[0][ y * videoFrame->linesize[0] + x];
           out[y * videoFrame->width * 4 + x * 4 + 2]  = videoFrame->data[0][ y * videoFrame->linesize[0] + x];
           out[y * videoFrame->width * 4 + x * 4 + 3]  = 255;
       }
   }
   static int frameCount=0;
  //  Logi("update frame","success %d",frameCount++);
    return true;
}

void MediaPlayer::playAudio()
{
    audioTrack=new AudioTrack;

    AVCodecParserContext *parser = NULL;
    AVSampleFormat sampleFormat;
    int numChannels;
    const char *fmt;
    AVPacket *aPacket=av_packet_alloc();//errorcheck

    parser=av_parser_init(aDecoder->id);
    if(!parser)
    {
        MediaLogE("AudioPlay parser Alloc failed");
        return;
    }
    //decode until eof
   // aPacket->data=
    ssize_t dataSize;
  uint8 *data=inbuf;
  dataSize=read(fd,inbuf,AUDIO_INBUF_SIZE);
  int res,len;
  while(dataSize>0)
  {
      if(!frame)
      {
            if(!(frame =av_frame_alloc()))
            {
                MediaLogE("Could not allocate audio frame");
                return;
            }
      }
   /*  res = av_parser_parse2(parser,aDecodeContext,&aPacket->data,&aPacket->size,data,dataSize,AV_NOPTS_VALUE,AV_NOPTS_VALUE,0);
      if(res<0)
      {
          MediaLogE("AudioPlay error parsing");
          return;
      }
      data +=res;
      dataSize -=res;*/
   MediaLogE("Audio","playing");
      av_read_frame(formatContext,aPacket);

      if(aPacket->stream_index==audioStrInd)
          decodeAudio(aPacket,frame);
    //  else continue;
    /*  if(dataSize < AUDIO_REFILL_THRESH)
      {
          //move remaining data to start of buf and point data to start again
          memmove(inbuf,data,dataSize);
          data=inbuf;
          len=read(fd,data+dataSize,AUDIO_INBUF_SIZE-dataSize);//append data to end of inbuf(that is free)
       //   av_read_frame(formatContext,aPacket);
          if(len>0)
              dataSize +=len;
      }*/
  }
  //flush decoder
  aPacket->data=NULL;
  aPacket->size=0;
  decodeAudio(aPacket,frame);
}
void MediaPlayer::decodeAudio(AVPacket *packet, AVFrame *frame)
{
    //send packet with compressed data to decoder
    int res,dataSize;
    res=avcodec_send_packet(aDecodeContext,packet);
    if(res<0)
    {
        MediaLogE("decodeAudio Error submit packet to decoder");
        return;//should exit()
    }

    //read all output frames (in general there may be any number of them)
    while(res>=0)
    {
        res = avcodec_receive_frame(aDecodeContext,frame);
        if(res==AVERROR(EAGAIN) || res==AVERROR_EOF)
        {
            return;
        }
        else if(res<0)
        {
            MediaLogE("Error decoding audio");
            return;//exit;
        }
        dataSize=av_get_bytes_per_sample(aDecodeContext->sample_fmt);
        if(dataSize<0)
        {
            //shouldnot happen failed calculate data size;
            return;//exit;
        }

       /*write the data to ouput
        * for(int i = 0; i < frame->nb_samples; i++)
        {
            for (int ch = 0; ch < aDecodeContext->channels; ch++);
            //AudioTrack.write(frame->data[ch] + dataSize*i,dataSize);
        }
        */
      // frame.id
       audioTrack->submit(frame->data,dataSize);
       //usleep(1000);

    }
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