#pragma once
#include "AudioHandler.h"
#include "Compression.h"
#include "Gba.h"

class AudioV1 :
    public AudioHandler
{
private:
    int offsetCount;
    int frame; 
    unsigned char* dataSource;
    unsigned long* dataPointers;
    unsigned char* dataOffsetTable;
public:
    AudioV1(AudioHeader* src,  int frames, int (*func)());
    virtual AudioDataPacket* GetNextFrame();
};

