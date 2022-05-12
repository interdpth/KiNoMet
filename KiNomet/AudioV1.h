#pragma once
#include "AudioHandler.h"
#include "Compression.h"
#include "Gba.h"
enum CompType
{

    Raw = 0,
    RLE = 1,
    LZ = 2,
    Pointer = 3, //Always chec kfor pointer, then size difference.         
    ENDME = 0xFD


};
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

    //int Copy(AudioPacket* curPack, unsigned char* dstBuf, int len);

  //unsigned char* GetBuffer();
  // // int Fillbuffers(unsigned int bytesLeft, AudioPacket* curPack);

    /// <summary>
/// Dump audio to buffer.
/// </summary>
    int Processs();

};

