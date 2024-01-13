#pragma once
#include "DataPacket.h"

//enum AUDIO_PACKET_FLAGS :unsigned char
//{
//	START = 1,
//	PLAYING = 1 << 1,
//	DATA = 1 << 2,
//	END = 0xFF,
//};



class AudioDataPacket :public DataPacket
{
private:
	void Init();
public:
	unsigned long olen;//5
	unsigned long tracked;//6
	AudioDataPacket(DataPacket* p);
	AudioDataPacket(int frame, int length, void* data);
	AudioDataPacket(AudioDataPacket* srcp);
};

