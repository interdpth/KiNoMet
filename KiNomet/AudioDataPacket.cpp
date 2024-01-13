#include "AudioDataPacket.h"
void AudioDataPacket::Init()
{
	olen = 0;
	tracked = 0;
}

AudioDataPacket::AudioDataPacket(DataPacket* p) :DataPacket(p)
{
	this->Init();
}

AudioDataPacket::AudioDataPacket(int frame, int length, void* data)
{
	this->Init();
}

AudioDataPacket::AudioDataPacket(AudioDataPacket* srcp)
{
	//assign to srcp

	this->data = srcp->data;
	this->FrameId = srcp->FrameId;
	this->len = srcp->len;
	this->olen = srcp->olen;
	this->tracked = srcp->tracked;
}
