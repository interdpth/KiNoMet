#include "VideoKinometPacket.h"
#include <cstring>
VideoKinometPacket::VideoKinometPacket(rectangle& r, int fps) :KinometPacket(PacketType::VIDEO, nullptr)
{
	this->Init();
	inmemorybuffer tmp((unsigned char*)&r,  (unsigned char*)&this->DisplaySize, sizeof(rectangle) );
	safe_memcpy(&tmp);
	this->FramesPerSecond = fps;
}
VideoKinometPacket::VideoKinometPacket(rectangle& r, int fps, DataPacket* p) :KinometPacket(PacketType::VIDEO, p) 
{
	this->Init();
	inmemorybuffer tmp((unsigned char*)&r, (unsigned char*)&this->DisplaySize, sizeof(rectangle));
	safe_memcpy(&tmp);
	this->FramesPerSecond = fps;
}
VideoKinometPacket::VideoKinometPacket(rectangle& r, int fps, int FrameId, unsigned char* dat, int len) :KinometPacket(PacketType::VIDEO, nullptr)
{
	this->Init();
	inmemorybuffer tmp((unsigned char*)&r, (unsigned char*)&this->DisplaySize, sizeof(rectangle));
	safe_memcpy(&tmp);
	this->FramesPerSecond = fps;
	this->FrameId = FrameId;
	this->data = dat;
	this->len = len;
}


void VideoKinometPacket::Init()
{
	memset(&DisplaySize, 0, sizeof(DisplaySize));
	this->FramesPerSecond = 0;
}


