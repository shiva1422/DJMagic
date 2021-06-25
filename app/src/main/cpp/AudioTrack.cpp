//
// Created by shiva1422 on 5/29/2021.
//

#include "AudioTrack.h"
#include "oboe/AudioStreamBuilder.h"
#include "MediaPlayer.h"

using namespace oboe;
AudioTrack::AudioTrack()
{

}
bool AudioTrack::configure()
{
    AudioStreamBuilder audioStreamBuilder;
    audioStreamBuilder.setSampleRate(sampleRate);
    audioStreamBuilder.setChannelCount(numChannels);
    audioStreamBuilder.setFormat(audioFormat);
    audioStreamBuilder.setCallback(this);
    audioStreamBuilder.setContentType(ContentType::Music);
    audioStreamBuilder.setPerformanceMode(PerformanceMode::LowLatency);
    audioStreamBuilder.setSharingMode(SharingMode::Exclusive);
    Result result=audioStreamBuilder.openStream(&stream);
    if(result!=Result::OK)
    {
        Loge("AudioTrack","Cound not create stream");
        return false;
    }
    //checkDoc below for runtime tuning of latency
    auto res=stream->setBufferSizeInFrames(stream->getFramesPerBurst()*2);
    if(res)
    {
        audioBufSizeHW = res.value() * stream->getBytesPerFrame();
        Logi("AudioTrack","buffers size is %d ",audioBufSizeHW );
    }
    result=stream->requestStart();//keep result;
    //Close stream
    bConfigured = true;//above if might false this
    return true;
}
AudioTrack::~AudioTrack()
{
    stream->close();//check error ///pausing etc
    delete stream;
    stream= nullptr;
}
AudioTrack::AudioTrack(int numChannels, int sampleRate, AudioFormat audioFormat,void *dataSrc)
{
    this->numChannels = numChannels;
    this->sampleRate = sampleRate;
    this->audioSrc = dataSrc;
    this->audioFormat =audioFormat;
    configure();

}
DataCallbackResult AudioTrack::onAudioReady(AudioStream *audioStream, void *audioData,int32_t numFrames)
{
   // memset(audioData,255,numFrames*2);
    MediaPlayer *player = (MediaPlayer *)audioSrc;
    if(player)
    {
        Loge("onAudioReady ","requries %d frame",numFrames);
        player->audioCallback(audioData,numFrames);
    }
    else memset(audioData,0,numFrames*audioStream->getBytesPerFrame());
    return DataCallbackResult::Continue;
}
int AudioTrack::getBufSize()
{
    return audioBufSizeHW;
}
void AudioTrack::submit(void *bufferData, int numFrames)
{
    stream->write(bufferData,numFrames,1000);//timeout in nano secs;
}
/*
 * Methods to add audio track
 * Cofigure methode instead of in constructor;
 * setContent type
 * setAudioFormat
 * setSharingMode;
 */