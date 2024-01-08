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
    unsigned char* decompBuffer;
    int frame; 
    unsigned char* dataSource;
    unsigned long* dataPointers;
    unsigned char* dataOffsetTable;
public:
    AudioV1(unsigned char* src, int len, int fps, int frames, int rsize,int (*func)());

    virtual int Copy(AudioPacket* curPack, unsigned char* dstBuf, int size);

    /// <summary>
/// Dump audio to buffer.
/// </summary>
    int ProcessAudio();

};

