#pragma once
#include "KinometPacket.h"
#include "BaseTypes.h"
class VideoKinometPacket :public KinometPacket
{
protected:
	void Init();
public:
	rectangle DisplaySize;
	int FramesPerSecond;
	int SampleSize;
	VideoKinometPacket(rectangle& r, int fps);
};
