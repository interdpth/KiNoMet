#pragma once
#include "DataPacket.h"
#include "KinometPacket.h"

class AudioKinometPacket :public KinometPacket
{
protected:
	void Init();
public:
	AudioKinometPacket(KinometPacket* kp);
	AudioKinometPacket(DataPacket* p);
	AudioKinometPacket();
};