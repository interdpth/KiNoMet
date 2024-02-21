
#ifndef KINOMET_H
#define KINOMET_H

#include <stdlib.h>

#include "Packets.h"
#include "SmallBuffer.h"
#include "Cinepak.h"
enum KinoFileFormat {
	Cinepak,
	Kinopack
};
#include "AudioKinometPacket.h"
#include "VideoKinometPacket.h"
#define 	AVIIF_LIST   0x00000001
#define 	AVIIF_KEYFRAME   0x00000010
#define 	AVIIF_NO_TIME   0x00000100
//https://cdn.hackaday.io/files/274271173436768/avi.pdf
typedef struct KinometAviControl
{
	bool (*videoCallBack)(VideoKinometPacket*);
	bool (*audiocallback)(AudioKinometPacket*);
	int(*GetSize)();
	unsigned int(*GetTicks)();
	void (*init)();
	KinoFileFormat Format;
};
void LoadAVI(unsigned char* file,
		int size,
		unsigned char* audiofile,
		int audiofsize,
	KinometAviControl* options);
#endif // ! KINOMET_H
