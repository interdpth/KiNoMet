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
    AudioV0(unsigned char* src, int len, int fps, int frames, int (*func)());
	int Copy(AudioPacket* curPack, unsigned char* dstBuf, int len);


	int Fillbuffers(unsigned int bytesLeft, AudioPacket* curPack);


};

