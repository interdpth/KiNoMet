#pragma once
#include "AudioHandler.h"
#include "AudioV0.h"
#include "AudioV1.h"
#include "AudioV2.h"


class AudioManager
{

private:
	AudioVersion ver;
	AudioHandler* hndlr;

public:
	void Init(AudioHeader* src, int len, int fps, int frames, int (*func)());

	int Copy(AudioKinometPacket* curPack, unsigned char* dstBuf, int size);
	int FillBuffers(unsigned int bytesLeft, AudioKinometPacket* curPack);
	int ProcessAudio();

	AudioKinometPacket* GetNextFrame();
	unsigned char* GetBuffer();

	int GetSampleFreq();
	AudioKinometPacket* GetCurrPacket();
	AudioVersion GetType();
	int GetBytesLeft(AudioKinometPacket* curPack);
	void Queue(AudioKinometPacket* packet);
};

