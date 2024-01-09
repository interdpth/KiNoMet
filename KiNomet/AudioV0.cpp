#include "AudioV0.h"

AudioV0::AudioV0(unsigned char* src, int len, int fps, int frames, int rsize, int (*func)()):
    AudioHandler(src, len, fps, frames,  rsize, func)
{
    AudioHeader* hdr = (AudioHeader*)src;	
	AudioPacket* p = (AudioPacket*)malloc(sizeof(AudioPacket));
	if (p != nullptr)
	{
		memset(p, 0, sizeof(AudioPacket));
		p->start = (unsigned char*)&hdr->datPtr;
		p->len = len;
		p->tracked = 0;
		p->frame = 0;
		QueueAudio(p);
	}
}
