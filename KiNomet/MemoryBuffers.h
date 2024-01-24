#pragma once


class inmemorybuffer
{
public:

	unsigned char* src;
	unsigned char* dst;
	int len;

	inmemorybuffer(unsigned char* s, unsigned char* d, int l);
};

class MemoryBuffers
{
public:
#define ScreenSize (240 * 160 * 2)


#ifdef GBA
	static unsigned char* AudioDecompBuffer;
	static unsigned char* VideoDecompBuffer;
#else
	static unsigned char AudioDecompBuffer[0x1000];
	static unsigned char VideoDecompBuffer[0x1000];
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

extern "C"
{
	extern int safe_memcpy(inmemorybuffer* buffer);
	extern int Valid8BitDest(int dst);
	//returns bytes adjusted.
	extern int Safeunsane(inmemorybuffer* buffer);

	extern int LongCopy(inmemorybuffer* buffer);

	extern int UnsignedShortCopy(inmemorybuffer* buffer);

	extern int Copy8Bit(inmemorybuffer* buffern);
}