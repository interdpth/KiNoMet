#pragma once
#include "DataPacket.h"
#include "KinometPacket.h"
enum AudioVersion :unsigned char
{
	V0,
	V1,
	V2,
	UNK
};


class AudioKinometPacket :public KinometPacket
{
protected:
	void Init();
public:
	unsigned long olen;//5
	unsigned long tracked;//6
	AudioVersion Version;
	AudioKinometPacket(AudioVersion v, KinometPacket* kp);
	AudioKinometPacket(AudioVersion v, DataPacket* p);
	AudioKinometPacket(AudioKinometPacket* srcp);
	AudioKinometPacket(AudioVersion v, int sampleSize);
	AudioKinometPacket(AudioVersion v, int SampleSSize, int len, unsigned char* buf);
	AudioKinometPacket(AudioVersion v, int SampleSSize, int frameId, int len, unsigned char* buf);
};