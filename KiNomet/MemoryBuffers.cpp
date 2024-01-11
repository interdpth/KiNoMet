#include "MemoryBuffers.h"

	
#ifdef GBA
unsigned char* MemoryBuffers::DecompBuffer = (unsigned char*)0x6000000 + ScreenSize + 0x1000;
unsigned char* MemoryBuffers::Kinomet_FrameBuffer = (unsigned char*)0x6000000;
unsigned char* MemoryBuffers::startBuf = ((unsigned char*)0x6000000 + (240 * 160 * 2));//(unsigned char*)malloc(ringSize);

#else
	unsigned char MemoryBuffers::DecompBuffer[0x1000]; 
	unsigned char MemoryBuffers::Kinomet_FrameBuffer[ScreenSize];
	unsigned char MemoryBuffers::startBuf[ScreenSize];
#endif