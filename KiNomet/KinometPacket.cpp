#include "KinometPacket.h"



void KinometPacket::Init() {
	SampleSize = 0;
	BasePacket = nullptr;
}

KinometPacket::KinometPacket(PacketType type, DataPacket* packet):DataPacket(packet) {
	packetType = type;
}
KinometPacket* KinometPacket::GetBasePacket() { return BasePacket; }

PacketType KinometPacket::GetPacketType() { return packetType; }
KinometPacket::KinometPacket()
{

}