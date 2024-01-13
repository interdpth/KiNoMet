#include "DataPacket.h"

DataPacket::DataPacket(int length, void* dat) {
	len = length;
	data = dat;
	FrameId = -1;
}

DataPacket::DataPacket(int fram, int length, void* dat) {
	FrameId = fram;
	len = length;
	data = dat;
}
DataPacket::DataPacket(DataPacket* p) {

	if (p == nullptr) return;//nothing to set
 	this->FrameId = p->FrameId;
	this->data = p->data;
	this->len = p->len;
}

DataPacket::DataPacket() {
	Init();
}

void DataPacket::Init()
{
	len = 0;
	data = nullptr;
	FrameId = -1;
}
