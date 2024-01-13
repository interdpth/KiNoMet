#pragma once
#include "AudioHandler.h"
#include "Compression.h"
#include "Gba.h"

class AudioV2 :
	public AudioHandler
{
private:
	int offsetCount;
	int frame;
	//unsigned char* dataSource;
	unsigned long* dataPointers;
	unsigned char* dataOffsetTable;
public:
	AudioV2(AudioHeader* src,  int frames,  int (*func)());

	//int Copy(AudioKinometPacket* curPack, unsigned char* dstBuf, int len);

  //unsigned char* GetBuffer();
  // // int FillBuffers(unsigned int bytesLeft, AudioKinometPacket* curPack);

	/// <summary>
/// Dump audio to buffer.
/// </summary>
	int ProcessAudio();

};

