//
// Created by shiva1422 on 5/29/2021.
//

#include "AudioTrack.h"
#include "oboe/AudioStreamBuilder.h"
using namespace oboe;
AudioTrack::AudioTrack()
{
    AudioStreamBuilder audioStreamBuilder;
    audioStreamBuilder.setSampleRate(sampleRate);
    audioStreamBuilder.setChannelCount(numChannels);
    audioStreamBuilder.setFormat(audioFormat);
    //audioStreamBuilder.setCallback(this);//
    audioStreamBuilder.setContentType(ContentType::Music);
    audioStreamBuilder.setPerformanceMode(PerformanceMode::LowLatency);
    audioStreamBuilder.setSharingMode(SharingMode::Exclusive);
    Result result=audioStreamBuilder.openStream(&stream);
    if(result!=Result::OK)
    {
        Loge("AudioTrack","Cound not create stream");
        return;
    }
    auto res=stream->setBufferSizeInFrames(stream->getFramesPerBurst()*2);
    if(res)
    {
        Logi("AudioTrack","buffers size is %d",res.value());
    }
    result=stream->requestStart();//keep result;
    //Close stream

}
AudioTrack::~AudioTrack()
{
    stream->close();//check error ///pausing etc
    delete stream;
    stream= nullptr;
}
AudioTrack::AudioTrack(short numChannels, AudioFormat audioFormat, short sampleRate)
{


}
/*
 * Methods to add audio track
 * setContent type
 * setAudioFormat
 * setSharingMode;
 */