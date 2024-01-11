#pragma once
class MemoryBuffers
{
public:
#define ScreenSize (240 * 160 * 2)
#ifdef GBA
	static unsigned char* DecompBuffer;
#else
	static unsigned char DecompBuffer[0x1000];
#endif
#ifdef GBA
	static unsigned char* Kinomet_FrameBuffer;
#else
	static unsigned char Kinomet_FrameBuffer[ScreenSize];
#endif
#ifdef GBA
	static unsigned char* startBuf;
#else
		static unsigned char startBuf[ScreenSize];
#endif
};

