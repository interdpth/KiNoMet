#pragma once


#include "Gba.h"
#ifndef GBA
#include <Windows.h>
#endif
#include "AudioHandler.h"

class AudioV0 :
    public AudioHandler
{
public:
    AudioV0(AudioHeader* src,  int frames, int (*func)());
};

