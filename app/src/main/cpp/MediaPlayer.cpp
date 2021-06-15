//
// Created by shiva1422 on 5/29/2021.
//

#include <android/asset_manager.h>
#include "MediaPlayer.h"
#include "FileManager.h"
#include "VideoView.h"
#define MediaLogE(...)((void)__android_log_print(ANDROID_LOG_ERROR,"MEDIA_LOG:",__VA_ARGS__))
#define MediaLogI(...)((void)__android_log_print(ANDROID_LOG_INFO,"MEDIA_LOG:",__VA_ARGS__))

#define INBUFSIZE 20480
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
    bufSize=FFMIN(bufSize, INBUFSIZE);//check

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
static int64_t kseek(void *opaque,int64_t offset , int whence)
{

    AAsset *asset = (AAsset *)opaque;
    return AAsset_seek(asset,offset,whence);
}
int MediaPlayer::decodeInterruptCallBack(void *ctx)
{
    MediaPlayer *player=(MediaPlayer *)ctx;
    return player->abortRequest;
}
static int isRealTime(AVFormatContext *fmtCtx)
{
    if( !strcmp(fmtCtx->iformat->name, "rtp")|| !strcmp(fmtCtx->iformat->name, "rtsp")|| !strcmp(fmtCtx->iformat->name, "sdp"))
        return 1;
    if(fmtCtx->pb && (   !strncmp(fmtCtx->url, "rtp:", 4)|| !strncmp(fmtCtx->url, "udp:", 4)))
        return 1;
    return 0;

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
    if(pthread_cond_init(&condContReadThread,NULL))
    {
        setError("Conditional init failed Player ");//,strerror(errno))
        goto fail;
    }

    inputBuf = (uint8 *)av_malloc(INBUFSIZE + FF_INPUT_BUFFER_PADDING_SIZE);//move to header and free;
    if(!inputBuf)
    {
       setError( "could not create inputBuf");
        goto fail;
    }


    if(initFrameAndPacketQsClocks()==STATUS_KO_FATAL)
    {
        setError("init Queues and clocks failed");
        goto fail;
    }
    bPlayerInit = true;
    return;
fail:
    bPlayerInit = false;//clear resources;
    MediaLogE("INIT fialed");

}
status MediaPlayer::initFrameAndPacketQsClocks()
{
    //FrameQ init


    if(picFQ.init(&vidPktQ,VIDEO_PICTURE_QUEUE_SIZE,1) < 0 || sampleFQ.init(&audPktQ , SAMPLE_QUEUE_SIZE , 1) < 0 || subPicFQ.init(&subPktQ, SUBPICTURE_QUEUE_SIZE,0)<0)
    {
        MediaLogE("initFrameAndPacketQsClocks"," frameQ init failed");
        return STATUS_KO_FATAL;
    }
    //packetQ init

    if(vidPktQ.init() < 0 || audPktQ.init() < 0 || subPktQ.init()<0)
    {
        MediaLogE("initFrameAndPacketQsClocks"," packetQ init failed");
        return STATUS_KO_FATAL;
    }

    audClock.init(&audPktQ.serial);
    vidClock.init(&vidPktQ.serial);
    extClock.init(&extClock.serial);
    MediaLogI("initFPQsClocks success;");
    return STATUS_OK;
}

status MediaPlayer::setFile(const char *fileLoc)
{
    this->fileName=std::string(fileName);
    asset= FileManager::getAsset(fileLoc);
    if(!asset)
    {
        setError("could't open Asset");
        goto fail;
    }

    //after asset opened;
    ioContext = avio_alloc_context(inputBuf , INBUFSIZE, 0, (void *)asset, readPacket, NULL, kseek);
    if(!ioContext)
    {
        setError("IOContext alloc failed");
        goto fail ;
    }

    if(pthread_create(&readThr,NULL,MediaPlayer::masterReadThread,this))
    {
        setError("ReadThread Create Error");//strerr(errno)
        goto fail;
    }

  //  return openInputFindStreamInfo();
    return STATUS_OK;
fail:
    //clear res //stream_close();
    return STATUS_KO_FATAL;
}
void * MediaPlayer::masterReadThread(void *mediaPlayer)
{
    MediaPlayer *player=(MediaPlayer *)mediaPlayer;
    int ret,err,i;
    int streamInds[3];//
    AVDictionaryEntry *t;



    if(pthread_mutex_init(&player->mutWait,NULL))
    {
        ret = AVERROR(ENOMEM);
        player->setError("wait Mutex init failed");
        goto fail;
    }
    memset(streamInds,-1,sizeof(streamInds));

    player->pkt = av_packet_alloc();
    if(!player->pkt)
    {
        player->setError("readThread - pkt alloc failed");
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    //open streams and codecs done in in below;
   if(player->openInputFindStreamInfo() != STATUS_OK)//can be ko?
   {
       ret =-1;
       player->setError("openInputAndFindStreamInfo failed");
       goto fail;
   }
    MediaLogI("Starting Decoding Threads");
    if(player->initAndStartCodecs() != STATUS_OK)
   {
       ret = -1;
       player->setError("InitAndStartCodecs Failed");
       goto fail;
   }
    //if(vidStrInd < 0 && audStrInd <0)//failed open file goto fail; move this and below to openStreamComps
    if(player->infiniteBuf < 0&& player->realTime)//check infiniteBuf init in declaration
        player->infiniteBuf = 1;

    player->startReading();//read Loop until EOF or err
    fail:
    //free pakt;destroy waitmutex;notify mainthread to quit play at the end these are also don
    return (void *)ret;

}
void MediaPlayer::streamTogglePause()
{
    if(paused)
    {
        frameTimer += av_gettime_relative()/1000000.0 - vidClock.lastUpdated;
        if(readPauseReturn != AVERROR(ENOSYS))
        {
            vidClock.paused = 0;
        }
        vidClock.setClock(vidClock.getValue(),vidClock.serial);//check getValue to get clock 1496;
    }

    paused = audClock.paused = vidClock.paused =extClock.paused = !paused;//toggled;

}
void MediaPlayer::stepToNextFrame()
{
    //if stream is paused unpause i t then step
    if(paused)
    {
        streamTogglePause();
    }
    step = 1;
}
kforceinline int MediaPlayer::startReading()
{
    /*
     2935
     */
    int pktInPlayRange = 0;
    int scallAllPmtsSet = 0;
    int64 pktTs,strstarttime;
    int ret;
    while(true)
    {
        if(abortRequest)
            break;
        if(paused != lastPaused)
        {
            lastPaused = paused; //check  both init vals
            if(paused)
            {
                readPauseReturn = av_read_pause(formatContext);
                //stop aud vid (this for network based(RTSP) stream;
            }
            else
                av_read_play(formatContext);//need?
        }
        //few lines for rtsp stream
        if(seekReq)
        {
            int64 seekTarget = seekPos;
            int64 seekMin = seekRel > 0 ? seekTarget -seekRel +2 : INT64_MAX;//smallest acceptable time stam and below is largest if(not accurat timestamp);
            int64 seekMax = seekRel < 0 ? seekTarget -seekRel -2 :INT64_MAX;  //fix req from src 2959;

            //doc say not to use this as its still under const check
            ret = avformat_seek_file(formatContext , -1 ,seekMin,seekTarget ,seekMax,seekFlags);
            if(ret < 0)
            {
                MediaLogE("Reading :error seeking");
            }
            else
            {
                //flush all current packetQueue to start queuing from seeked Pos
                if(audioStrInd >= 0)
                    audPktQ.flush();
                if(videoStrInd >= 0)
                    vidPktQ.flush();
                //subtitle as well
                if(seekFlags & AVSEEK_FLAG_BYTE)
                {
                    extClock.setClock(NAN,0);
                }
                else
                {
                    extClock.setClock(seekTarget/(double)AV_TIME_BASE,0);
                }

            }

            seekReq = 0;
            queueAttachmentsReq = 1;
            eof = 0;
            if(paused)
                stepToNextFrame();
        }

        if(queueAttachmentsReq)
        {
            if(vStream && vStream->disposition & AV_DISPOSITION_ATTACHED_PIC)//check;
            {
                if((ret = av_packet_ref(pkt, &vStream->attached_pic)) < 0)
                {
                    goto fail;
                }
                vidPktQ.put(pkt);
                vidPktQ.putNullPacket(pkt , videoStrInd);

            }
            queueAttachmentsReq = 0;
        }

        //if tthe queue are full ,no need to read more
        if(infiniteBuf<1 && (audPktQ.size + vidPktQ.size + subPktQ.size) > MAX_QUEUE_SIZE ||(vidPktQ.hasEnoughtPackets(vStream,videoStrInd)))//2997 //audio and subtitle as well;
        {
            //wait 10 ms continue;//check all rets;
            timespec absWaitTime;
            clock_gettime(CLOCK_MONOTONIC,&absWaitTime);
            absWaitTime.tv_sec += 0;//0 secs ?
            absWaitTime.tv_nsec = 10000000;//10 ms
            pthread_mutex_lock(&mutWait);//check val;
            pthread_cond_timedwait(&condContReadThread ,&mutWait,&absWaitTime);
            pthread_mutex_unlock(&mutWait);
            continue;
        }

        //few for looping maybe 3007


        //read packets;
        MediaLogI("read Packet");
        ret = av_read_frame(formatContext,pkt);
        if(ret < 0)
        {
            if((ret == AVERROR_EOF || avio_feof(formatContext->pb)) && !eof )
            {
                if(videoStrInd >= 0)
                {
                    vidPktQ.putNullPacket(pkt ,videoStrInd);
                }
                //audio sub streams as well;
            }
            if(formatContext->pb && formatContext->pb->error)
            {
                //autoExit goto fail;elsebreak;
                break;
            }

            //wait 10 ms continue;//check all rets;
            timespec absWaitTime;
            clock_gettime(CLOCK_MONOTONIC,&absWaitTime);
            absWaitTime.tv_sec += 0;//0 secs ?
            absWaitTime.tv_nsec = 10000000;//10 ms
            pthread_mutex_lock(&mutWait);//check val;
            pthread_cond_timedwait(&condContReadThread ,&mutWait,&absWaitTime);
            pthread_mutex_unlock(&mutWait);
            continue;
        }
        else
        {
            eof = 0;
        }

        //queue the packet if its in play range otherwise dicard;
        strstarttime = formatContext->streams[pkt->stream_index]->start_time;//only vid now;
        pktTs = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;//duration = pkt->duration /;
        pktInPlayRange =duration == AV_NOPTS_VALUE || (pktTs - (strstarttime != AV_NOPTS_VALUE ? strstarttime : 0)) *av_q2d(formatContext->streams[pkt->stream_index]->time_base) -(double)(startTime != AV_NOPTS_VALUE ? startTime : 0) / 1000000<= ((double)duration / 1000000);

        if(pkt->stream_index == videoStrInd && pktInPlayRange)
        {
            MediaLogI("Packet in play Range Queued");
            vidPktQ.put(pkt);

        }
        //else if audio subtitle
        else
        {
            av_packet_unref(pkt);
        }
    }
    ret = 0;

    fail://check 3062;
    //closeinput ,free packe ,quit evetn ,,3063;
    av_packet_free(&pkt);
    return ret;
}
status MediaPlayer::openInputFindStreamInfo()//Demuxing
{
    int res;
    formatContext = avformat_alloc_context();
    if(!formatContext)
    {
        //cllose ioContext;
        setError("FormatContext alloc failed");//AvERROR(NOMEM);
        res = AVERROR(ENOMEM);
        goto fail;
    }
    formatContext->interrupt_callback.callback = decodeInterruptCallBack;//check wheret to abort blocking functs ,if call back return 1 blocking operation will be aborted;;
    formatContext->interrupt_callback.opaque = this;
    ///++++++++++++++++++++ 2792,2802 - 2815;later;
    formatContext->pb=ioContext;
    formatContext->pb->eof_reached = 0;//2836 only if pb!NULL
    //must be closed;
    //formatContext->iformat=av_probe_input_format(&probeData,1);//use with open input; check src;also above alater with open input
    formatContext->flags=AVFMT_FLAG_CUSTOM_IO;

    if(avformat_open_input(&formatContext, "",NULL,NULL) < 0)//AVInput format and options later;
    {
        //clear formatContex;
        setError("OpenInput failed");
        goto fail;
    }
    //check right time for below two also check above later;before find
    formatContext->max_analyze_duration=INT64_MAX;
    formatContext->max_ts_probe=INT32_MAX;
    if(avformat_find_stream_info(formatContext,NULL) < 0)
    {
        //fill fmt context->nb streams if the openinput function could not recoginze some streams without headers
        setError("Find Stream Info Failed");
        goto fail;
    }
    totNumStreams=formatContext->nb_streams;
    maxFrameDuration = (formatContext->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;//fue lines above later;and below seeking;
    realTime = isRealTime(formatContext);
    av_dump_format(formatContext, ANDROID_LOG_INFO, "", 0);//index?

    if(openStreamsAndCodecs() == STATUS_KO_FATAL)
    {
        setError("Open Streams and Codecs Failed");
        goto fail;
    }

    return STATUS_OK;
    fail:
    if(!formatContext)
        avformat_close_input(&formatContext);
    return STATUS_KO_FATAL;

}
status MediaPlayer::openStreamsAndCodecs()
{
    /*
     * just vid now ,initially set stream->discard = AvDISCARD_ALL;
     * move to func so that minimized fora all streams;and more work to accurate;
     */
    AVMediaType mediaType = AVMEDIA_TYPE_VIDEO;
    int res = av_find_best_stream(formatContext,mediaType,-1,-1,&vDecoder,0);//options check;return withing range nb_streams;
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
        return STATUS_KO;//also check audio subtitles
    }
    else
    {//if decoder cant be found should decoderFail flag;todo
        vidDisable = false;
        videoStrInd=res;//or can  be passed to above func as well
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
            if(!vDecodeContext)
            {
                res = AVERROR(ENOMEM);
              goto fail;
            }
            //  vDecodeContext->workaround_bugs=1;
            // avcodec_parameters_copy(vDecodeContext->c)
            res=avcodec_parameters_to_context(vDecodeContext,formatContext->streams[videoStrInd]->codecpar);

            av_log((void *)vDecodeContext, AV_LOG_INFO, "SDFDS");
            if(res<0)
            {
                MediaLogE("OpenCodecs:vid Params to context failed");
                goto fail;
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
        else
        {
            MediaLogE("ERROR DECODER NOT FOUND");//fatal;
            return STATUS_KO_FATAL;
        }


    }
    vStream->discard = AVDISCARD_DEFAULT;//discard all to make stream inactive after open? //next and before few lines check 2641
    ///extranted the stream info above now open streamComponenets;(above needs improvement);above needs to be moved to funct;


MediaLogI("Open Streams and Codecs Success");
return STATUS_OK;
    fail:
    return STATUS_KO_FATAL;//clear res;above;
}
status MediaPlayer::initAndStartCodecs()
{
    //for now just vid;//if fail cclear contextxts;

    if(vidDec.init(vDecodeContext,&vidPktQ,&condContReadThread))//if fails free this codecContext
        return STATUS_KO_FATAL;
    if(vidDec.start(MediaPlayer::videoThread,this))
        return STATUS_KO_FATAL;
    else
        queueAttachmentsReq = 1;//id vidDec success
    MediaLogE("init and StartCodecs codec thread started success");
    return STATUS_OK;

}
MediaPlayer::~MediaPlayer()
{
    close(fd);//error check?
    delete audioCodec;
    audioCodec= nullptr;
    delete audioTrack;
    audioCodec= nullptr;
}
void MediaPlayer::setError(const char *error)//check thread Safe?
{
    this->errorString += error;
    MediaLogE("%s",error);
}



void MediaPlayer::playVideo()
{
/*
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



void MediaPlayer::printFrameInfo(AVCodecContext* codecContext,AVFrame *frame)
{
    double  timebase = av_q2d(vStream->time_base);

    MediaLogI("\\n timeUnit is %lf",frame->pts*timebase);
    MediaLogI("Frame - %c frameNo -%d , , pts - %lu , dts - %d , keyFrame %d ,repeatPicCount %d, codedPictureNum %d , displayPictureNum %d ",av_get_picture_type_char(frame->pict_type),
              codecContext->frame_number,frame->pts,frame->pkt_dts,frame->key_frame,frame->repeat_pict,frame->coded_picture_number,frame->display_picture_number);

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
    return 0;
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

        if(frameDrop > 0 || (frameDrop && getMasterSyncType() != AV_SYNC_VIDEO_MASTER))
        {
            if(frame->pts != AV_NOPTS_VALUE)
            {
                double diff = dpts - getMasterClock();
                if(!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD && (diff - frameLastFilterDelay) < 0 && vidDec.packetSerial == vidClock.serial && vidPktQ.numPackets)
                {
                    frameDropsEarly++;
                    av_frame_unref(frame);
                    gotPicture = 0;
                }
            }
        }
    }
    return gotPicture;
}
int MediaPlayer::getMasterSyncType()
{
    if(avSyncType == AV_SYNC_VIDEO_MASTER)
    {
        if(vStream)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_AUDIO_MASTER;
    }
    else if(avSyncType == AV_SYNC_AUDIO_MASTER)
    {
        if(aStream)
            return AV_SYNC_AUDIO_MASTER;
        else return AV_SYNC_EXTERNAL_CLOCK;
    }
    else
        return AV_SYNC_VIDEO_MASTER;
}
double MediaPlayer::getMasterClock()
{
    double val;

    switch(getMasterSyncType())
    {
        case AV_SYNC_VIDEO_MASTER:
            val = vidClock.getValue();
                    break;
        case AV_SYNC_AUDIO_MASTER:
            val = audClock.getValue();
            break;
        default:
            val = extClock.getValue();
    }
    return val;
}
int MediaPlayer::decoderDecodeFrame(Codec *dec, AVFrame *frame, AVSubtitle *sub)
{

    int res = AVERROR(EAGAIN);
    while(true)
    {
        if(dec->packetQueue->serial == dec->packetSerial)
        {
            do{
                if(dec->packetQueue->abortReq)
                    return -1;

                switch(dec->codecContext->codec_type)//Now only Video
                {
                    case AVMEDIA_TYPE_VIDEO:
                        res = avcodec_receive_frame(dec->codecContext,frame);
                        if(res >= 0)
                        {
                            if(decoderReorderPts == -1)
                            {
                                frame->pts = frame->best_effort_timestamp;
                            }
                            else if(!decoderReorderPts)
                            {
                                frame->pts = frame->pkt_dts;
                            }
                        }

                        break;
                        //OtherCases;
                }
                if(res == AVERROR_EOF)
                {
                    dec->finished = dec->packetSerial;
                    avcodec_flush_buffers(dec->codecContext);
                    return 0;
                }
                if(res >= 0)
                    return 1;


            }while(res != AVERROR(EAGAIN));
        }


        do{

            if(dec->packetQueue->numPackets == 0)
                pthread_cond_signal(dec->condEmptyQ);
                if(dec->packetPending)
                    dec->packetPending = 0;

                else
                {
                    int oldSerial = dec->packetSerial;
                    if(packetQget(dec->packetQueue,dec->packet,1,&dec->packetSerial)<0)
                        return -1;
                    if(oldSerial != dec->packetSerial)
                    {
                        avcodec_flush_buffers(dec->codecContext);
                        dec->finished = 0;
                        dec->nextPts = dec->startPts;
                        dec->nextPtsTB =dec->startPtsTB;
                    }

                }
                if(dec->packetQueue->serial ==dec->packetSerial)
                    break;
                av_packet_unref(dec->packet);
            }while(true);

        if(dec->codecContext->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            int gotFrame = 0;
            res =avcodec_decode_subtitle2(dec->codecContext ,sub,&gotFrame,dec->packet);
            if(res < 0)
            {
                res = AVERROR(EAGAIN);
            }
            else
            {
                if(gotFrame && !dec->packet->data)
                {
                    dec->packetPending = 1;
                }
                res =gotFrame ? 0 :(dec->packet->data ? AVERROR(EAGAIN) : AVERROR_EOF);
            }
            av_packet_unref(dec->packet);
        }
        else
        {
            if(avcodec_send_packet(dec->codecContext,dec->packet) == AVERROR(EAGAIN))
            {
                MediaLogE("Decoder decode frame : revieveFrame and sendpacket both returned EAGAIN , which is an API violation;");//check src av_log;
                dec->packetPending = 1;
            }
            else
            {
                av_packet_unref(dec->packet);
            }
        }
    }

    return res;//not in src
}
int MediaPlayer::packetQget(PacketQueue *packetQueue, AVPacket *packet,int block ,int *serial)//move to packetQuw
{
    MyAVPacketList pkt;
    int res;
    pthread_mutex_lock(&packetQueue->mutex);
    while(true)
    {
        if(packetQueue->abortReq)
        {
            res = -1;
            break;
        }
        if(av_fifo_size(packetQueue->packetList) >= sizeof(pkt))
        {
            av_fifo_generic_read(packetQueue->packetList,&pkt,sizeof(pkt),NULL);
            packetQueue->numPackets --;
            packetQueue->size -= pkt.pkt->size + sizeof(pkt);
            packetQueue->duration -= pkt.pkt->duration;
            av_packet_move_ref(packet,pkt.pkt);
            if(serial)
                *serial = pkt.serial;
            av_packet_free(&pkt.pkt);
            res = 1;
            break;
        }
        else if(!block)
        {
            res = 0;
            break;
        }
        else
        {
            pthread_cond_wait(&packetQueue->cond,&packetQueue->mutex);
        }
    }

    pthread_mutex_unlock(&packetQueue->mutex);
    return res;

}

int MediaPlayer::packetQPutPrivate(PacketQueue *packetQueue, AVPacket *packet)
{

}
int MediaPlayer::queuePicture(AVFrame *srcFrame, double pts, double duration, int64 pos,int serial)
{
    Frame *vp;
    #ifndef NDEBUG
    MediaLogI(" FrameType = %c  ,pts = %0.3f",av_get_picture_type_char(srcFrame->pict_type),pts);//print frameInfo;
    #endif

    if(!(vp = frameQueuePeekWritable(&picFQ)))
        return -1;
    vp->sar = srcFrame->sample_aspect_ratio;
    vp->uploaded =0;
    vp->width = srcFrame->width;
    vp->height =srcFrame->height;
    vp->format = srcFrame->format;

    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;
    //setDefaultWindowSize(vp->width,vp->height,vp->sar);

    av_frame_move_ref(vp->frame,srcFrame);//should the unrefbe called? on dest before thsis?
    picFQ.push();
    return 0;


}
Frame* MediaPlayer::frameQueuePeekWritable(FrameQueue *f)
{
    //Move into FrameQueue;
    //wait until there is space to put a new Frame;
    pthread_mutex_lock(&f->mutex);
    while(f->size >= f->maxSize && !f->packetQueue->abortReq)
    {
        pthread_cond_wait(&f->cond,&f->mutex);
        MediaLogI("VideoTHread Check");
    }
    pthread_mutex_unlock(&f->mutex);

    if(f->packetQueue->abortReq)
        return NULL;
    return &f->frameQueue[f->windex];
}
void MediaPlayer::clearResources()
{
    avcodec_close(vDecodeContext);
    avcodec_close(aDecodeContext);
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    AAsset_close(asset);
    free(inputBuf);//nullcheck;
    av_free(ioContext);

    //free contexts ,packet,
}
//free
//contexts,