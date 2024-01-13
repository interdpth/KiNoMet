#pragma once
#include "AudioHandler.h"
#include "AudioV0.h"
#include "AudioV1.h"
#include "AudioV2.h"


class AudioManager
{

private:
	int ver;
	AudioHandler* hndlr;

public:
	void Init(AudioHeader* src, int len, int fps, int frames, int (*func)());

	int Copy(AudioDataPacket* curPack, unsigned char* dstBuf, int size);
	int FillBuffers(unsigned int bytesLeft, AudioDataPacket* curPack);
	int ProcessAudio();

	AudioDataPacket* GetNextFrame();
	unsigned char* GetBuffer();

	int GetSampleFreq();
	AudioDataPacket* GetCurrPacket();
	int GetType();
	int GetBytesLeft(AudioDataPacket* curPack);
	void Queue(AudioDataPacket* packet);
};

