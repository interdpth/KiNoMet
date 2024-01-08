#pragma once
#include "AudioHandler.h"
#include "Compression.h"
#include "Gba.h"

class AudioV2 :
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
    AudioV2(unsigned char* src, int len, int fps, int frames, int rsize, int (*func)());

    //int Copy(AudioPacket* curPack, unsigned char* dstBuf, int len);

  //unsigned char* GetBuffer();
  // // int FillBuffers(unsigned int bytesLeft, AudioPacket* curPack);

    /// <summary>
/// Dump audio to buffer.
/// </summary>
    int ProcessAudio();

};

