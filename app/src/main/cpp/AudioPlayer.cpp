//
// Created by shiva1422 on 5/29/2021.
//

#include "AudioPlayer.h"
#include "FileManager.h"
extern "C"
{
#include "libavformat/avformat.h"
}
AudioPlayer::AudioPlayer()
{

}
AudioPlayer::~AudioPlayer()
{
    close(fd);//error check?
    delete audioCodec;
    audioCodec= nullptr;
    delete audioTrack;
    audioCodec= nullptr;
}
status AudioPlayer::setAudioFileFromAssets(const char *audioFileLoc)
{
    fd=FileManager::getFileDescriptor(audioFileLoc);//get FILE* instead
    if(fd<0)
    {
        ///fatal error prompt
        Loge("AudioPlayer setAudioFile","error cannot get fd");
        return STATUS_KO_FATAL;
    }
    char path[20];

    sprintf(path,"pipe:%d",fd);
    Logi("AudioPlayer","fd path is %s",path);
    AVFormatContext *avFormatContext=NULL;

    //open the file and get info about streams
    if(avformat_open_input(&avFormatContext,path,NULL,NULL)<0)
    {
        //close fd and file
        return STATUS_KO_FATAL;
    }
    if(avformat_find_stream_info(avFormatContext,NULL)<0)
    {
        //close fd and file
        Loge("AudioPlayer::setAudioFile","cannot find stream info");
        return STATUS_KO_FATAL;
    }

    //find streams(A,V) and open decoders.if stream was encoded by unknown decoder=>error //similar process for both vid and audio
    for(uint i=0;i<avFormatContext->nb_streams;++i)
    {
        //audio here has only one stream so loop necessary?
        if(avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)//video for video
        {
            //code
            Logi("setAudioFile","find audio stream count %ud",avFormatContext->nb_streams);
            //To BE CONTINUED.................................
        }
    }

    Logi("AudioPlayer :setAudioFile","success");
    return STATUS_OK;
}