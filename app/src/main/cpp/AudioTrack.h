//
// Created by shiva1422 on 5/29/2021.
//

#ifndef DJMAGIC_AUDIOTRACK_H
#define DJMAGIC_AUDIOTRACK_H


#include "Commons.h"
#include "oboe/Oboe.h"
#include "oboe/AudioStream.h"
#include "oboe/Definitions.h"
using namespace oboe;

class AudioTrack{
private:
    AudioStream *stream= nullptr;
public:
    short numChannels=2,bitsPerSample=32;//audioFormat=1;//audioFormat=1 for pcm other for compressed;
    //short blockAlign=numChannels *bitsPerSample/8;(byteRate ,(frame));
    int32 sampleRate=44100;//byteRate=sampleRate*blocalign
    AudioFormat audioFormat=AudioFormat::Float;

    AudioTrack();
    ~AudioTrack();
    AudioTrack(short numChannels,AudioFormat audioFormat,short sampleRate);
    void submit(void *bufferData,int buffSize);//test

};


#endif //DJMAGIC_AUDIOTRACK_H
