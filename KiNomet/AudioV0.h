#pragma once


#include "Gba.h"
#ifndef GBA
#include <Windows.h>
#endif
#include "AudioHandler.h"

class AudioV0 :
    public AudioHandler
{
    int curFrame;
    int len;
    unsigned char* srcData;
public:
    AudioV0(AudioHeader* src,  int frames, int (*func)());
    virtual AudioKinometPacket* GetNextFrame();
};

