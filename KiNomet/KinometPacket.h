#pragma once
#include "DataPacket.h"

/// <summary>
/// If type is not raw, audio or video it is corrupted. 
/// </summary>
enum PacketType: unsigned char
{
	UNKOWN = 0xFF,
	RAW = 1, 
	VIDEO = 2,
	AUDIO = 3
};
class KinometPacket :public DataPacket
{
protected:
	void Init();
	KinometPacket* BasePacket;
public:
	//Kinomet packets contain at least...
	int SampleSize;
	PacketType packetType;
	PacketType GetPacketType();
	KinometPacket* GetBasePacket();
	KinometPacket();
	KinometPacket(PacketType type, DataPacket* packet);

};