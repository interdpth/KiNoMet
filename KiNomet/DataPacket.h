#pragma once
class DataPacket
{
protected:
	void Init();
public:
	int len;
	void* data;
	int FrameId;
	DataPacket(int frame, int length, void* dat);
	DataPacket(int length, void* dat);
	DataPacket(DataPacket* p);
	DataPacket();
};
