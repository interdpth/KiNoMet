#include "AllSight.h"
#include "Compression.h"
Compression* decomp;

enum ChariotWheels
{
	Raw = 0,
	RLE,
	LZ,
	Pointer,
	END = 0xFD//Always chec kfor pointer, then size difference. 
}ChariotWheels;
AllSight::AllSight(void* buffer, int buffsize)
{
	frameCount = 0;
	framePointers = 0;
	frameStart = 0;
	buff = new SmallBuffer((unsigned char*)buffer, buffsize);
	init = 0;
	audioRam = nullptr;
	audioBuffer = nullptr;
	fps = 0;
	decomp = new Compression();
}


void AllSight::Parse()
{
	unsigned long hdr = 0; buff->Read(&hdr, 4);
 	if (hdr != BARGHEADER) return;
	init = true;
	buff->Read(&fps, 1);
//	tmpBuffer += 4;

	framePointers = buff->Read32();
	frameStart = buff->Read32();
	frameCount = buff->Read32();//Num frames
	audioRam = new unsigned char[4096];

}

void* AllSight::GetFrame(int index)
{
	int pointer = framePointers ;
	int mathed = pointer + index * 4;
	int bufzf = frameStart;
	int inBufferMath = mathed + bufzf; 
	buff->Seek(inBufferMath, SEEK_SET);
	return buff->GetCurrentBuffer();
}
void* AllSight::GetBuffer()
{
	return audioBuffer;
}
int AllSight::DecodeFrame(int index)
{
	unsigned char* frame = (unsigned char*)GetFrame(index);
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
	while (*frame != ChariotWheels::END)
	{
		int comp = *frame++;
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

	return (int)&out - (int)&audioRam;
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