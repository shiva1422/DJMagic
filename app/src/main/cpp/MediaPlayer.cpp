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
        if(infiniteBuf<1 && (audPktQ.size + vidPktQ.size + subPktQ.size) > MAX_QUEUE_SIZE ||(vidPktQ.hasEnoughtPackets(vStream,videoStrInd)) && audPktQ.hasEnoughtPackets(aStream,audioStrInd))// subtitle as well;
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
                if(audioStrInd >= 0)
                {
                    audPktQ.putNullPacket(pkt , audioStrInd);
                }
                // sub streams as well;
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
        else if(pkt->stream_index == audioStrInd && pktInPlayRange )
        {
            audPktQ.put(pkt);
        }
        //else if subtitle
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

            AVDictionary *opts = nullptr;

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

    //Audio ??this whole function can be reduced do loop or swithch or separe audio and vid to separate functions;
     mediaType = AVMEDIA_TYPE_AUDIO;
     res = av_find_best_stream(formatContext,mediaType,-1,-1,&aDecoder,0);//options check;return withing range nb_streams;
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
        audDisable = false;
        audioStrInd=res;//or can  be passed to above func as well
        aStream = formatContext->streams[audioStrInd];
        if(!aDecoder)
        {
            //can also find by name;
            aDecoder=avcodec_find_decoder(aStream->codecpar->codec_id);
        }
        if(aDecoder)
        {
            //cannot use codeccontext directly from stream so create context and get it from codecparameters.
            aDecodeContext=avcodec_alloc_context3(aDecoder);
            if(!aDecodeContext)
            {
                res = AVERROR(ENOMEM);
                goto fail;
            }
            //  vDecodeContext->workaround_bugs=1;
            // avcodec_parameters_copy(vDecodeContext->c)
            res=avcodec_parameters_to_context(aDecodeContext,aStream->codecpar);

            av_log((void *)aDecodeContext, AV_LOG_INFO, "SDFDS");
            if(res<0)
            {
                MediaLogE("OpenCodecs:audio Params to context failed");
                goto fail;
            }

            AVDictionary *opts = nullptr;

            if((res=avcodec_open2(aDecodeContext,aDecoder,NULL)) < 0)
            {//last option for setting private option for decoder
                MediaLogE("Open %s codec failed",av_get_media_type_string(mediaType));
            }
            else
            {
                sampleRate = aDecodeContext->sample_rate;
                numChannels = aDecodeContext->channels;
                channelLayout = aDecodeContext->channel_layout;
                MediaLogE("audioCodec opened for stream index %d ...Creating audio Track sampleRate = %d ,numChannels = %d ,channelLayout = %lu , sampleSize",audioStrInd , sampleRate,numChannels,channelLayout);
                audioTrack = new AudioTrack(numChannels,sampleRate,MediaPlayer::getAudioFormatFromAVSampleFormat(aDecodeContext->sample_fmt),(void *)this);//clear;
                if(audioTrack)
                {
                    bAudioInit=true;
                    audDisable = false;
                    //the below should be the final params accepted or set on the audioStream;//if the params from stream are not available on device
                    //for now same as set on audioTrack;move below to class meth.
                    audioTargetParams.freq = sampleRate;
                    audioTargetParams.format = aDecodeContext->sample_fmt;
                    audioTargetParams.channels = numChannels;
                    audioTargetParams.channelLayout = channelLayout;
                    audioTargetParams.frameSize = av_samples_get_buffer_size(NULL,audioTargetParams.channels,1,audioTargetParams.format,1);
                    audioTargetParams.bytesPerSec = av_samples_get_buffer_size(NULL,audioTargetParams.channels,audioTargetParams.freq,audioTargetParams.format,1);
                    ////audioTargetParams.framesize || bytesPerSecc<=0 fail;
                    audioBufSizeHW = audioTrack->getBufSize();//check
                    audioSrcParams = audioTargetParams;
                    //init averaging filter;
                    audioDiffAvgCoef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);//?
                    audioDiffAvgCount = 0;

                    ///since we do not have a precise enough audio FIFO fullness,we correct audio sync only if larger than this threshold
                    audDiffThreshold = (double)(audioBufSizeHW)/audioTargetParams.bytesPerSec;
                    //pause track?
                    //setCallBack......................?for the track;


                    MediaLogI("Audio Track created");
                    //else try other formats if due to configure err;
                }

            }

        }
        else
        {
            MediaLogE("ERROR DECODER NOT FOUND Audio");//fatal;
            return STATUS_KO_FATAL;
        }


    }
MediaLogI("Open Streams and Codecs Success");
return STATUS_OK;
    fail:
    return STATUS_KO_FATAL;//clear res;above;
}
status MediaPlayer::initAndStartCodecs()
{
    //for now just vid;//if fail cclear contextxts;diable aud ,vid ,inits if failed; do these based on vid init and baudinit flags

    if(vidDec.init(vDecodeContext,&vidPktQ,&condContReadThread))//if fails free this codecContext
        return STATUS_KO_FATAL;
    if(vidDec.start(MediaPlayer::videoThread,this))
        return STATUS_KO_FATAL;
    else
        queueAttachmentsReq = 1;//id vidDec succes
    if(audDec.init(aDecodeContext,&audPktQ,&condContReadThread))
        return STATUS_KO_FATAL;
    if((formatContext->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) &&formatContext->iformat->read_seek)
    {
        audDec.startPts = aStream->start_time;
        audDec.startPtsTB = aStream->time_base;
    }
    if(audDec.start(MediaPlayer::audioThread,this))
        return STATUS_KO_FATAL;
    MediaLogE("init and StartCodecs codec thread started success");
    return STATUS_OK;

}



Bitmap MediaPlayer::getImageParams()
{
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

    Bitmap bitmap;
   bitmap.height=vDecodeContext->height;
   bitmap.width=vDecodeContext->width;
   Logi("getPar","the width of pic is %d and height is %d",bitmap.height,bitmap.width);
    return bitmap;
}
void MediaPlayer::audioCallback(void *audioData,int32 numFrames)
{
    //prepare new aud buff;
    float *stream = (float *)audioData;
    int audioSize , numFrames1,sampleCount;
    audioCallbackTime = av_gettime_relative();
    while(numFrames > 0)
    {

        if(audioBufIndex >= numSamples)
        {
            sampleCount = decodeAudioFrame();
            if(sampleCount < 0)
            {
                //if err output silence
                audioBuf = NULL;
                audioBufSize = OBOE_AUDIO_MIN_BUFFER_SIZE / audioTargetParams.frameSize * audioTargetParams.frameSize;
                numSamples = audioBufSize / 8;//byte to float with 2 channels;
            }
            else
            {
                //update sample display if there;
                //audioBufSize = audioSize;
                numSamples = sampleCount;
            }
            audioBufIndex = 0;

        }
        //audioBuf Index is acturally audioFrameIndex;
        numFrames1 = numSamples - audioBufIndex;
        if(numFrames1 > numFrames)
            numFrames1 = numFrames;
        //fill bufs
        if(!muted && audioBuf)//one cond with volume 2479
        {
          //  memcpy(stream,(uint8 *)audioBuf + audioBufIndex,numFrames1);
            float *bufCh1 = (float *)(audioBuf),*bufCh2 = (float *)(audioBuf2);
            bufCh1 += audioBufIndex,bufCh2 += audioBufIndex;
            for(int i=0; i < numFrames1; i++)
            {
                stream[2*i]=bufCh1[i];
                stream[2*i+1]=bufCh2[i] ;
              //  MediaLogE("sample values %f and %f",bufCh1[i],bufCh2[i]);
            }

        }
        else
        {
            memset(stream, 0, numFrames1);
            if(!muted && audioBuf)
            {
                /////////////MIX audio to dst;s
                MediaLogE("EEEEEEEEEEEEEEEF CALLABAC");
            }
        }
        numFrames -= numFrames1;
        stream += numFrames1 *2;
        audioBufIndex += numFrames1;

    }
    audioWriteBufSize = numSamples * 8 - audioBufIndex * 8;
    //aasming audio driver has two periods
    if(!isnan(audioClock))
    {
        audClock.setClockAt(audioClock - (double)(2 * audioBufSizeHW + audioWriteBufSize)/audioTargetParams.bytesPerSec,audioClockSerial,audioCallbackTime/1000000.0);
        syncClockToSlave(&extClock,&audClock);
    }

}
int MediaPlayer::decodeAudioFrame()
{
    /*
     * decode audio frame and return its uncompressed size;
     * frame deoded ,converted if required and stored in MediaPlayer.audioBuf,return size in bytes of that buf;
     */
    int dataSize ,resampledDataSize;//if resampled
    int64_t decChannelLayout;
    av_unused double audioClock0;
    int wantedNumSamples;
    Frame *af;
    if(paused)
        return -1;

    do{
        if(!(af = sampleFQ.peekReadable()))
            return -1;
        sampleFQ.next();
    }while(af->serial != audPktQ.serial);

    //datasize below gives size of both channels;
    dataSize = av_samples_get_buffer_size(NULL, af->frame->channels, af->frame->nb_samples,static_cast<AVSampleFormat>(af->frame->format), 1);//check cast;
    decChannelLayout = (af->frame->channel_layout && af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ? af->frame->channel_layout : av_get_default_channel_layout(af->frame->channels);
    wantedNumSamples = synchronizeAudio(af->frame->nb_samples);

    //some work if the output format not equal to the current format and needed conversion;//2375

    //else2430
    audioBuf = af->frame->data[0]; //multiple channels
    audioBuf2 =af->frame->data[1];
    resampledDataSize = dataSize;

    audioClock0 = audioClock;
    //update the audio clock with pts;
    if(!isnan(af->pts))
    {
        audioClock = af->pts + (double)af->frame->nb_samples /af->frame->sample_rate;
    }
    else
        audioClock = NAN;
    audioClockSerial = af->serial;


    //printinfo if debug;
    //    static double last_clock;printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",is->audio_clock - last_clock,is->audio_clock, audio_clock0);last_clock = is->audio_clock;

    return wantedNumSamples;//earlier resampledDataSize;

}
int MediaPlayer::synchronizeAudio(int numSamples)
{
    /*
     * return wanted num samples to get better sync if synttype is video or external master clock
     */
    int wantedNumSamples = numSamples;

    //if not master then try to remove or add samples to correct the clock

    if(getMasterSyncType() != AV_SYNC_AUDIO_MASTER)
    {
        double diff ,avgDiff;
        int minNumSamples , maxNumSamples;
        diff = audClock.getValue() - getMasterClkValue();

        if(!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD)
        {
            audioDiffCum = diff + audioDiffAvgCoef * audioDiffCum;
            if(audioDiffAvgCoef < AUDIO_DIFF_AVG_NB)
            {
                //not enough measures t ohave a corresct estimate
                audioDiffAvgCount++;
            }
            else
            {
                //estimate A-v differnece;
                avgDiff = audioDiffCum * (1.0 - audioDiffAvgCoef);
                if(fabs(avgDiff) >= audDiffThreshold)
                {
                    wantedNumSamples = numSamples + (int)(diff * audioSrcParams.freq);
                    minNumSamples = ((numSamples *(100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    maxNumSamples = ((numSamples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) /100));
                    wantedNumSamples = av_clip(wantedNumSamples,minNumSamples,maxNumSamples);
                }
              //  av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",diff, avg_diff, wanted_nb_samples - nb_samples,is->audio_clock, is->audio_diff_threshold);
            }
        }
        else
        {
            //too big diff : may be initial pts errors so reset A-V filter;
            audioDiffAvgCount = 0;
            audioDiffCum = 0;
        }
    }
    return wantedNumSamples;
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
        ret = player->picFQ.queuePicture(frame , pts ,duration,frame->pkt_pos,player ->vidDec.packetSerial);
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
void *MediaPlayer::audioThread(void *mediaPlayer)
{
    MediaPlayer *player  = (MediaPlayer *)mediaPlayer;
    AVFrame *frame = av_frame_alloc();
    Frame *af;
    int gotFrame = 0;
    AVRational tb;
    int ret = 0;
    if(!frame)
    {
        MediaLogE("AudioThread frame not allcoated");
        ret = AVERROR(ENOMEM);//return here
    }

    do
        {
            if((gotFrame = player->decoderDecodeFrame(&player->audDec,frame,NULL)) < 0)
                goto theEnd;

            if(gotFrame)
            {
                tb = (AVRational) {1, frame->sample_rate};
                if (!(af = player->sampleFQ.peekWritable()))
                    goto theEnd;
                af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
                af->pos = frame->pkt_pos;
                af->serial = player->audDec.packetSerial;
                af->duration = av_q2d((AVRational) {frame->nb_samples, frame->sample_rate});
                av_frame_move_ref(af->frame, frame);
                player->sampleFQ.push();
            }
        } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
    theEnd:
    av_frame_free(&frame);
    MediaLogE("the end");
    //return ret;
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
                    case AVMEDIA_TYPE_AUDIO:
                        res = avcodec_receive_frame(dec->codecContext,frame);
                        if(res >=0)
                        {
                            AVRational  tb = (AVRational){1 , frame->sample_rate};
                            if(frame->pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(frame->pts,dec->codecContext->pkt_timebase,tb);
                            else if(dec->nextPts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(dec->nextPts , dec->nextPtsTB,tb);
                            if(frame->pts != AV_NOPTS_VALUE)
                            {
                                dec->nextPts = frame->pts +frame->nb_samples;
                                dec->nextPtsTB = tb;
                            }
                            MediaLogE("audio queued");
                        }
                        break;

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
void MediaPlayer::updateVideoPts(double pts, int64 pos, int serial)
{
    //update current vid pts
    vidClock.setClock(pts,serial);
    syncClockToSlave(&extClock , &vidClock);//move to clock;

}
void MediaPlayer::syncClockToSlave(Clock *c, Clock *slave)
{
    double clock = c->getValue();
    double slaveClock = slave->getValue();
    if(!isnan(slaveClock) && (isnan(clock) || fabs(clock -slaveClock) > AV_NOSYNC_THRESHOLD))
        c->setClock(slaveClock,slave->serial);
}
void MediaPlayer::onRefresh(double *remainingTime)
{
    //display/main Thread
    //double remaininTime;//from mainLoop consider screenRefresh reate;
    double time;

    Frame *sp , *sp2;//subtit pics
    if(!paused && getMasterSyncType() == AV_SYNC_EXTERNAL_CLOCK && realTime) //threadsafe check;
        checkExtClkSpeed();
    if(!vidDisable && aStream) //showmode!video i.e audio and rdft display;1585
    {
        time = av_gettime_relative() / 1000000.0;
      //  videoDisplay();
        //other
    }
    //remaininTime = FFMIN(remaininTime,lastVIstTime +rdftspeed - time);

    if(vStream)
    {
        retry:
        if(picFQ.getRemainingCount() == 0)
        {
            //no pictures available to display;
        }
        else
        {
            double lastDuration,duration,delay;
            Frame *vp , *lastvp ;//video pics

            //dequeue the picture;
            lastvp = picFQ.peekLast();
            vp = picFQ.peek();
            if(vp->serial != vidPktQ.serial)//TS
            {
                picFQ.next();
                goto retry;
            }

            if(lastvp->serial != vp->serial)
            {
                frameTimer = av_gettime_relative() / 1000000.0;
            }
            if(paused)//TSCheck
                goto display;

            //compute nominal lastDuration
            lastDuration = getVidPicDuration(lastvp,vp);
            delay = computeTargetDelay(lastDuration);

            time = av_gettime_relative()/1000000.0;
            if(time < frameTimer + delay)
            {
                *remainingTime = FFMIN(frameTimer + delay -time , *remainingTime);
                goto display;//update not req?
            }

            frameTimer += delay;
            if(delay > 0 && time - frameTimer > AV_SYNC_THRESHOLD_MAX)
            {
                frameTimer = time;
            }

            pthread_mutex_lock(&picFQ.mutex);//checkcond;
            if(!isnan(vp->pts))
                updateVideoPts(vp->pts , vp->pos,vp->serial);
            pthread_mutex_unlock(&picFQ.mutex);

            if(picFQ.getRemainingCount() > 1)
            {
                Frame *nextPic = picFQ.peekNext();
                duration = getVidPicDuration(vp,nextPic);
                if(!step && (frameDrop>0 || (frameDrop && getMasterSyncType()!=AV_SYNC_VIDEO_MASTER) && time > frameTimer+duration))
                {
                    frameDropsLate++;
                    picFQ.next();
                    goto retry;
                }
            }

            //SubTitle as well;1646
            picFQ.next();
            forceRefresh = 1;

            if(step && !paused)
                streamTogglePause();

        }

        display:
        //display picture

        if(!vidDisable && forceRefresh && picFQ.rindexShown)
            videoDisplay();


    }
    forceRefresh = 0;

    //show status//1692

}
void MediaPlayer::videoDisplay()
{
    //if display not init,init
    //if no vid stream display audio - rdft else videoImageDisplay;
    Frame *vp;
    Frame *sp = nullptr;

    vp = picFQ.peekLast();
    //if subtitle disp subtitle
    //calculate output pic dims using vps width ,height ,sar,also startX,startY,width ,height;
    if(!vp->uploaded)
    {
        //uploadTexture;
        //if(upload failed return)
        outputView->updateTexture(vp);

        MediaLogI("VIDEO DISPLAY %d %d %lf",vp->serial,vp->pos,vp->pts);
        vp->uploaded = 1;
    }

}
double MediaPlayer::getMasterClkValue()
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
            break;
    }
    return val;
}
double MediaPlayer::computeTargetDelay(double delay)
{
    double syncThresold , diff = 0;
    //update delay to follow master sync source;
    if(getMasterSyncType() != AV_SYNC_VIDEO_MASTER)
    {
        //if vid is slave ,try to correct big delays by duplacating or deleting a frame

        diff = vidClock.getValue() - getMasterClkValue();
        /* skip or repeat frame .take delayy into account to compute thresholds.still dont know if its the best guess
         *
         */
        syncThresold = FFMAX(AV_SYNC_THRESHOLD_MIN , FFMIN(AV_SYNC_THRESHOLD_MAX,delay));
        if(!isnan(diff) && fabs(diff) < maxFrameDuration)
        {
            if(diff <= -syncThresold)
                delay = FFMAX(0,delay + diff);
            else if(diff >= syncThresold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                delay = delay + diff;
            else if(diff > syncThresold)
                delay = 2 * delay;
        }
    }
//    av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n",delay, -diff);

return delay;

}
double MediaPlayer::getVidPicDuration(Frame *pic, Frame *nexPic)
{
    if(pic->serial == nexPic->serial)
    {
        double duration = nexPic->pts - pic->pts;
        if(isnan(duration) || duration <= 0|| duration> maxFrameDuration)
            return pic->duration;
        else
            return duration;
    }
    else
    {
        return 0.0;
    }

}
void MediaPlayer::checkExtClkSpeed()
{
    if(videoStrInd >= 0 && vidPktQ.numPackets <= EXTERNAL_CLOCK_MIN_FRAMES || audioStrInd >= 0 && audPktQ.numPackets <= EXTERNAL_CLOCK_MIN_FRAMES)
    {
        extClock.setSpeed(FFMAX(EXTERNAL_CLOCK_SPEED_MIN , extClock.speed - EXTERNAL_CLOCK_SPEED_STEP));
    }
    else if((videoStrInd < 0 || vidPktQ.numPackets > EXTERNAL_CLOCK_MAX_FRAMES) && (audioStrInd < 0 || audPktQ.numPackets > EXTERNAL_CLOCK_MAX_FRAMES))
    {
        extClock.setSpeed(FFMIN(EXTERNAL_CLOCK_SPEED_MAX, extClock.speed + EXTERNAL_CLOCK_SPEED_STEP));

    }
    else
    {
        double speed = extClock.speed;
        if(speed != 1.0)
        {
            extClock.setSpeed(speed + EXTERNAL_CLOCK_SPEED_STEP *(1.0 -speed)/fabs(1.0 -speed));
        }
    }
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
AudioFormat MediaPlayer::getAudioFormatFromAVSampleFormat(AVSampleFormat sampleFormat)
{
    if(sampleFormat == AV_SAMPLE_FMT_FLTP)
        return AudioFormat::Float;//float planar;
    return AudioFormat::I32;
}
void MediaPlayer::setError(const char *error)//check thread Safe?
{
    this->errorString += error;
    MediaLogE("%s",error);
}
MediaPlayer::~MediaPlayer()
{
    close(fd);//error check?
    delete audioCodec;
    audioCodec= nullptr;
    delete audioTrack;
    audioCodec= nullptr;
}


//free
//contexts,