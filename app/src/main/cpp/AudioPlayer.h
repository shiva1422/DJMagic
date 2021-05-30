//
// Created by shiva1422 on 5/29/2021.
//

#ifndef DJMAGIC_AUDIOPLAYER_H
#define DJMAGIC_AUDIOPLAYER_H

extern "C"
{
#include <libavcodec/codec.h>
#include <libavcodec/avcodec.h>
};

#include "AudioTrack.h"
#include "AudioCodec.h"

class AudioPlayer {
private:
    int fd=-1;
    bool bPlaying=false;
    AudioTrack *audioTrack= nullptr;
    AudioCodec *audioCodec= nullptr;//destroy

public:
    AudioPlayer();
    ~AudioPlayer();
    status setAudioFileFromAssets(const char* audioFileLoc);
    void configureCodec();
    void configureTrack();
    void play();
    void pause();




};


#endif //DJMAGIC_AUDIOPLAYER_H
