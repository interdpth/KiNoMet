#include "AudioV0.h"
AudioV0::AudioV0(AudioHeader* src, int frames,  int (*func)()):
    AudioHandler(src,  frames,  func)
{
 
	AudioPacket* p = (AudioPacket*)malloc(sizeof(AudioPacket));
	if (p != nullptr)
	{
		p->len = src->fileLength;
		p->tracked = 0;
		p->frame = 0;
		p->data = src->data;
		QueueAudio(p);
	}
}
