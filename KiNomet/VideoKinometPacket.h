#pragma once
#include "KinometPacket.h"
#include "MemoryBuffers.h"
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
	VideoKinometPacket(rectangle& r, int fps, DataPacket* p);

	VideoKinometPacket(rectangle& r, int fps, int FrameId, unsigned char* dat, int len);
};
