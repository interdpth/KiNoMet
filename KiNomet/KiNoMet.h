
#ifndef KINOMET_H
#define KINOMET_H

#include <stdlib.h>

#include "Gba.h"

struct rectangle
{
	unsigned char x, y, w, h;
};

struct KinometPacket
{
	unsigned char type;
	unsigned char isAudio;
	unsigned char* frame;
	rectangle* rect;
	rectangle* screen;
	int frameid;
};

#include "SmallBuffer.h"
#include "Cinepak.h"

#define 	AVIIF_LIST   0x00000001
#define 	AVIIF_KEYFRAME   0x00000010
#define 	AVIIF_NO_TIME   0x00000100
//https://cdn.hackaday.io/files/274271173436768/avi.pdf
struct aviLoader
{
	void (*videoCallBack)(KinometPacket*);
		void (*audiocallback)(KinometPacket*);
		int(*GetSize)();
		unsigned int(*GetTicks)();
};
void LoadAVI(unsigned char* file,
		int size,
		unsigned char* audiofile,
		int audiofsize,
	aviLoader* options);
	
	extern unsigned char* Kinomet_FrameBuffer;
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount);
#endif // ! KINOMET_H
