#pragma once
#include "AudioHandler.h"
#include "Compression.h"
#include "Gba.h"

class AudioV1 :
    public AudioHandler
{
private:
    unsigned long* offsets;
    unsigned long offsetBase;
    int offsetCount;
    int frame; 
    unsigned char* dataSource;
    unsigned long* dataPointers;
    unsigned char* dataOffsetTable;
public:
    AudioV1(unsigned char* src, int len, int fps, int frames, int rsize,int (*func)());
    virtual AudioPacket* GetNextFrame();
};

