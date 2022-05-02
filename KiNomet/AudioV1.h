#pragma once
#include "AudioHandler.h"
class AudioV1 :
    public AudioHandler
{
private:
    unsigned long* offsets;
    unsigned long offsetBase;
    int offsetCount;
public:
    AudioV1(unsigned char* src, int len, int fps, int frames, int (*func)());
    int SeekIndex();
    int Copy(AudioPacket* curPack, unsigned char* dstBuf, int len);


    int Fillbuffers(unsigned int bytesLeft, AudioPacket* curPack);


};

