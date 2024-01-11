#include "AudioV0.h"
AudioV0::AudioV0(AudioHeader* src, int frames,  int (*func)()):
    AudioHandler(src,  frames,  func)
{
    AudioHeader* hdr = (AudioHeader*)src;	
	AudioPacket* p = (AudioPacket*)malloc(sizeof(AudioPacket));
	if (p != nullptr)
	{
		memset(p, 0, sizeof(AudioPacket));
		p->data = (unsigned char*)&hdr->datPtr;
		p->len = hdr->fileLength;
		p->tracked = 0;
		p->frame = 0;
		QueueAudio(p);
	}
}
