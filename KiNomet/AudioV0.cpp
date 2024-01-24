#include "AudioV0.h"
//V0 will set tracked to current track
AudioV0::AudioV0(AudioHeader* src, int frames,  int (*func)()):
    AudioHandler(src,  frames,  func)
{
	len = src->fileLength;
	curFrame = 0;
	srcData = src->data;
	
}

AudioKinometPacket* AudioV0::GetNextFrame()
{
	if (curFrame * 128 > len) return nullptr;
	AudioKinometPacket* p = new AudioKinometPacket(V0, NULL, curFrame, 0x500, &srcData[curFrame * 0x500]);
//	if (p != nullptr)
//	{
//////#ifdef GBA
//////		p->len = 128;
//////#else
////		p->len = 0x500;
//////#endif
////		p->tracked = 0;
////		p->frame = curFrame;
////		p->data = &srcData[curFrame*p->len];
////		;
//	}

	curFrame++;
		return p;
}