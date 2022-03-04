#include "AllSight.h"
#include "Compression.h"
Compression* decomp;
AllSight::AllSight(void* buffer, int buffsize)
{
	framePointers = nullptr;
	rawBuffer = buffer;
	buffSize = buffsize;
	init = 0;
	audioRam = nullptr;
	audioBuffer = nullptr;
	decomp = new Compression();
}


void AllSight::Parse()
{
	unsigned char* tmpBuffer = (unsigned char*)rawBuffer;
	unsigned long hdr = *(unsigned long*)tmpBuffer; tmpBuffer += 4;
	if (hdr != BARGHEADER) return;
	init = true;
	tmpBuffer += 4;
	frameCount = *(unsigned long*)tmpBuffer; tmpBuffer += 4;//Num frames
	framePointers = (unsigned long*)tmpBuffer; tmpBuffer += 4;
	audioRam = new unsigned char[4096];

}

void AllSight::DecodeFrame(int index)
{
	//Our frame is here. 
	unsigned char* frame = (unsigned char*)&framePointers[index];
	unsigned char* out = (unsigned char*)audioRam;
	//See if we're just a thing
	unsigned char type = *frame; frame++;
	unsigned char* decodingFrame = frame;

	if (type == ChariotWheels::Pointer)
	{
		frame = (unsigned char*)(*(unsigned char*)frame); frame += 4;
	}
	//Read frame size.
	unsigned long frameSize = *(unsigned long*)frame;  frame += 4;
	while (*frame++ != ChariotWheels::END)
	{
		int size = *(int*)frame; frame += 4;
		switch (*frame++)
		{
		case ChariotWheels::LZ:
			decomp->LZDecomp(frame, &out, size);
			break;
		case ChariotWheels::RLE:
			decomp->RLEDecomp(frame, &out, size);
			break;
		case ChariotWheels::Raw:
			decomp->RawCopy(frame, &out, size);
			break;
		case ChariotWheels::Pointer:
		{
			unsigned char* pointedFrame = (unsigned char*)(*(unsigned char*)frame);//mght be a bug here
			frame += 4;
			switch (*pointedFrame++)
			{
			case ChariotWheels::LZ:
				decomp->LZDecomp(pointedFrame, &out, size);
				break;
			case ChariotWheels::RLE:
				decomp->RLEDecomp(pointedFrame, &out, size);
				break;
			case ChariotWheels::Raw:
				decomp->RawCopy(pointedFrame, &out, size);
				break;
			}
		}
		break;
		}
	}

}
int AllSight::FrameCount()
{
	return frameCount;
}

AllSight::~AllSight()
{
	delete[] audioRam;
	delete audioBuffer;
}