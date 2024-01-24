#include "AudioKinometPacket.h"

AudioKinometPacket::AudioKinometPacket(AudioVersion v, DataPacket* p) :KinometPacket(PacketType::AUDIO, this)
{
	this->Init();
	Version = v;
}
AudioKinometPacket::AudioKinometPacket(AudioVersion v, int sampleSize) :KinometPacket(PacketType::AUDIO, this)
{
	this->Init();
	Version = v;
	this->SampleSize = sampleSize;
}

AudioKinometPacket::AudioKinometPacket(AudioVersion v, KinometPacket* kp) :KinometPacket(PacketType::AUDIO, this)
{
	this->Init();
	Version = v;
}

AudioKinometPacket::AudioKinometPacket(AudioVersion v, int SampleSSize, int frameId, int len, unsigned char* buf)
{
	this->Init();
	Version = v;
	this->SampleSize = SampleSSize;
	this->len = len;
	this->data = buf;
	this->FrameId = frameId;
}

AudioKinometPacket::AudioKinometPacket(AudioVersion v, int SampleSSize, int len, unsigned char* buf) :KinometPacket(PacketType::AUDIO, this)
{
	this->Init();
	Version = v;
	this->SampleSize = SampleSSize;
	this->len = len;
	this->data = buf;
}
AudioKinometPacket::AudioKinometPacket(AudioKinometPacket* srcp) :KinometPacket(PacketType::AUDIO, this)
{
	//assign to srcp

	this->data = srcp->data;
	this->FrameId = srcp->FrameId;
	this->len = srcp->len;
	this->olen = srcp->olen;
	this->tracked = srcp->tracked;
}
void AudioKinometPacket::Init()
{
	Version = AudioVersion::UNK;
	olen = 0;
	tracked = 0;
}