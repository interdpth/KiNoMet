#pragma once
#define BARGHEADER 0x47415242 //BRAG backwards
#include "SmallBuffer.h"
#include <stdio.h>

enum ChariotWheels
{
	Raw = 0,
	RLE,
	LZ,
	Pointer,
	END = 0xFD//Always chec kfor pointer, then size difference. 
}ChariotWheels;

class AllSight
{
private: 
	void* rawBuffer; 
	int buffSize;
	unsigned long* framePointers;
	int frameCount;
	bool init;
	void* audioRam; 
	SmallBuffer* audioBuffer; 
public:
	AllSight(void* buffer, int buffsize);
	~AllSight();
	int FrameCount();
	void Parse();
	bool IsValid();
	void DecodeFrame(int index);
};


