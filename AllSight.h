#pragma once
#ifndef ALLIGHT_SH
#define ALLIGHT_SH
#define BARGHEADER 0x47415242 //BRAG backwards
#include "SmallBuffer.h"
#include <stdio.h>

enum ChariotWheels;
class AllSight
{
private: 
	SmallBuffer* buff;
	unsigned long framePointers;
	unsigned long frameStart;
	int frameCount;
	bool init;
	void* audioRam; 
	int fps;
	SmallBuffer* audioBuffer; 
	void* GetFrame(int index);
public:
	AllSight(void* buffer, int buffsize);
	~AllSight();
	int FrameCount();
	void Parse();
	bool IsValid();
	int DecodeFrame(int index);
	void* GetBuffer();
};

#endif