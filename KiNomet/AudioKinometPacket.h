#pragma once
#include "DataPacket.h"
#include "KinometPacket.h"
#include "AudioHandler.h"

enum AudioVersion:unsigned char;
class AudioKinometPacket :public KinometPacket
{
protected:
	void Init();
public:
	AudioVersion Version;
	AudioKinometPacket(AudioVersion v, KinometPacket* kp);
	AudioKinometPacket(AudioVersion v,DataPacket* p);
	AudioKinometPacket(AudioVersion v);
};