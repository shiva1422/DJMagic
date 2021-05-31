//
// Created by shiva1422 on 5/29/2021.
//

#include "MediaPlayer.h"
#include "FileManager.h"
#define MediaLogE(...)((void)__android_log_print(ANDROID_LOG_ERROR,"MEDIA_LOG:",__VA_ARGS__))
#define MediaLogI(...)((void)__android_log_print(ANDROID_LOG_INFO,"MEDIA_LOG:",__VA_ARGS__))
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
status MediaPlayer::setAudioFileFromAssets(const char *audioFileLoc)
{
    //any failure close resources put flags
    fileName=audioFileLoc;
    fd=FileManager::getFileDescriptor(audioFileLoc);//get FILE* instead
    if(fd<0)
    {
        ///fatal error prompt
        MediaLogE("MediaPlayer setAudioFile","error cannot get fd");
        return STATUS_KO_FATAL;
    }
    char filePath[20];

    sprintf(filePath, "pipe:%d", fd);
    MediaLogI("MediaPlayer", "fd filePath is %s", filePath);

    //open the file and get info about streams to formatContet,demuxer info
    if(avformat_open_input(&formatContext, filePath, NULL, NULL) < 0)
    {
        //close fd and file
        MediaLogE("could not find stream info may be file error","check other ways like below or other");//
        return STATUS_KO_FATAL;
    }
    if(avformat_find_stream_info(formatContext, NULL) < 0)
    {
        //close fd and file
        MediaLogE("MediaPlayer::setAudioFile","cannot find stream info");
        return STATUS_KO_FATAL;
    }
    av_dump_format(formatContext, ANDROID_LOG_INFO, filePath, 0);//index? dump after open codecs for all streams;

    //open codecs for all  streams for now only one for each audio and video
    if(openCodecsFromFormat(&videoStrInd,AVMEDIA_TYPE_VIDEO) >= 0)//else?
    {//video codec opened success
        vStream=formatContext->streams[videoStrInd];//can be moved to openCode

        //allocate image where the decoded image will be put from decodeContext struct details//
        MediaLogI("Video Format pictureWidth-%d and height%d and bitRate-%ld",vDecodeContext->width,vDecodeContext->height,vDecodeContext->max_b_frames);

    }
    if(openCodecsFromFormat(&audioStrInd,AVMEDIA_TYPE_AUDIO) >=0)//else?
    {//audioCodec opended success
        aStream=formatContext->streams[audioStrInd];
        MediaLogI("AudioContext -sampleRate %d and bitRate %d and ,bperRawSample -%d",aDecodeContext->sample_rate,aDecodeContext->bit_rate,aDecodeContext->bits_per_raw_sample);
    }
    playAudio();
    return STATUS_OK;
}
int MediaPlayer::openCodecsFromFormat(int *streamIndex,AVMediaType type)//break into 2 for audio and video?
{

    //find streams(A,V) and open decoders.if stream was encoded by unknown decoder=>error //similar process for both vid and audio
    //if multiple streams check all streams
    //2 ways iterate open relevant codec  and avfindbeststream through formatContext->nb_streams(initlaize all in on go);


    //1st; for now assuming only 2 steams later use 2nd way for all streams although can be done in  fmtCtx->nb_streams for each type.
    const AVCodec** decoder;
    AVCodecContext** decodeContext;
    if(type == AVMEDIA_TYPE_AUDIO)
    {
        decoder= (&aDecoder);
        decodeContext=&aDecodeContext;

    }
    else if(type==AVMEDIA_TYPE_VIDEO)
    {
        decoder=(&vDecoder);
        decodeContext=&vDecodeContext;
    }

    else
        return -1;

    int res=-1;

    AVStream *stream;

    res=av_find_best_stream(formatContext,type,-1,-1,NULL,0);//check all fun args can open get more codec for streams
    if(res<0)
    {
        MediaLogE("could not find %s stream in file %s",av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return res;//video stream not found but there may be audio stream so if breakfunctio
    }
    else
    {

        *streamIndex = res;
        stream = formatContext->streams[*streamIndex];

        //find decoder fo the stream
        *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        if(!(*decoder))
        {
            MediaLogE("failed to find %s codec",av_get_media_type_string(type));
            return res;
        }
        *decodeContext=avcodec_alloc_context3(*decoder);///check null and clearing
       // avcodec_open2(*decodeContext,*decoder,NULL);
        res=avcodec_parameters_to_context(*decodeContext,stream->codecpar);
        if(res<0)
        {
            MediaLogE("Get Context from codecParams Failed");
            return res;
        }

        MediaLogI("streamIndex -%d and %d",*streamIndex,res);
        if(res=avcodec_open2(*decodeContext,*decoder,NULL)<0)
        {
            MediaLogE("Open %s codec failed",av_get_media_type_string(type));
            return res;
        }

    }
    MediaLogI("Codec opened successfully");
    return 0;

    //2nd way just to remember
    /* AVMediaType mediaType=AVMEDIA_TYPE_NB;
     for(uint i=0;i<formatContext->nb_streams;++i)
     {
         mediaType=AVMEDIA_TYPE_NB;//check if not any media type _NB means
         mediaType=formatContext->streams[i]->codecpar->codec_type;
         switch (mediaType)
         {
             case AVMEDIA_TYPE_VIDEO:
             {
                 videoStrInd=i;
                 MediaLogI(" AVMEDIA_TYPE_Video -    0 and streamIndex %d",videoStrInd);

             }break;
             case AVMEDIA_TYPE_AUDIO:
             {
                 audioStrInd=i;
                 MediaLogI("AVMEDIA_TYPE_AUDIO - 1 and streamIndex %d",audioStrInd);
             }break;

             case AVMEDIA_TYPE_SUBTITLE:
             {
                 MediaLogI(" AVMEDIA_TYPE_SUBTITLE-  3");

             }break;
             case AVMEDIA_TYPE_DATA:
             {
                 MediaLogI(" AVMEDIA_TYPE_DATA-  2");

             }break;
             case AVMEDIA_TYPE_UNKNOWN:
             {
                 MediaLogI(" AVMEDIA_TYPE_UNKNOW -   -1");//error
             }
             case AVMEDIA_TYPE_ATTACHMENT:
             {
                 MediaLogI(" AVMEDIA_TYPE_ATTACHMENT-    4");
             }
             break;
             case AVMEDIA_TYPE_NB:
             {
                 MediaLogI(" AVMEDIA_TYPE_NB-    5");
             }break;
             default:
             {
                 MediaLogI("Media type default no need actually");
             };

         }

     }*/


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
      res = av_parser_parse2(parser,aDecodeContext,&aPacket->data,&aPacket->size,data,dataSize,AV_NOPTS_VALUE,AV_NOPTS_VALUE,0);
      if(res<0)
      {
          MediaLogE("AudioPlay error parsing");
          return;
      }
      data +=res;
      dataSize -=res;

      if(aPacket->size)
          decodeAudio(aPacket,frame);
      if(dataSize < AUDIO_REFILL_THRESH)
      {
          //move remaining data to start of buf and point data to start again
          memmove(inbuf,data,dataSize);
          data=inbuf;
          //len=read(fd,data+dataSize,AUDIO_INBUF_SIZE-dataSize);//append data to end of inbuf(that is free)
       //   av_read_frame(formatContext,aPacket);
          if(len>0)
              dataSize +=len;
      }
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
       audioTrack->submit(frame->data,dataSize);
       //usleep(1000);

    }
}
//free
//contexts,