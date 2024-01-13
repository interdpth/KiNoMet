#include "VideoKinometPacket.h"
VideoKinometPacket::VideoKinometPacket(rectangle& r, int fps) :KinometPacket(PacketType::VIDEO, nullptr)
{
	this->Init();
	memcpy(&DisplaySize, &r, sizeof(rectangle));
	FramesPerSecond = fps;
}


void VideoKinometPacket::Init()
{
	memset(&DisplaySize, 0, sizeof(DisplaySize));
	FramesPerSecond = 0;
}
