#pragma once
#include "AudioHandler.h"
#include "AudioV0.h"
#include "AudioV1.h"
#include "AudioV2.h"
enum version
{
	UNK,
	V0,
	V1,
	V2
};


class AudioManager
{

private:
	int ver;
	AudioHandler* hndlr;

public:
	AudioManager(unsigned char* src, int len, int fps, int frames, int (*func)());

	int Copy(AudioPacket* curPack, unsigned char* dstBuf, int size);
	int FillBuffers(unsigned int bytesLeft, AudioPacket* curPack);
	int ProcessAudio();

	AudioPacket* GetNextFrame();
	unsigned char* GetBuffer();

	int GetSampleFreq();
	AudioPacket* GetCurrPacket();
	int GetType();
	int GetBytesLeft(AudioPacket* curPack);
	void Queue(AudioPacket* packet);
};

