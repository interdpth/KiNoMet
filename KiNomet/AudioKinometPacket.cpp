#include "AudioKinometPacket.h"

AudioKinometPacket::AudioKinometPacket(AudioVersion v,DataPacket* p) :KinometPacket()
{
	Version = v;
}
AudioKinometPacket::AudioKinometPacket(AudioVersion v)
{
	Version = v;
}

AudioKinometPacket::AudioKinometPacket(AudioVersion v,KinometPacket* kp):KinometPacket() {
	Version = v;
}

void AudioKinometPacket::Init()
{
	Version = AudioVersion::UNK;
}