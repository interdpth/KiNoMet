#include "AudioV0.h"

AudioV0::AudioV0(unsigned char* src, int len, int fps, int frames, int rsize, int (*func)()):
    AudioHandler(src, len, fps, frames,  rsize, func)
{

}



int AudioV0::Copy(AudioPacket* curPack, unsigned char* dstBuf, int len)
{

	int bytesLeft = len;
	if (curPack->tracked + bytesLeft > curPack->len)
	{
		bytesLeft = curPack->len - (curPack->tracked);
	}

#ifdef  GBA
	memcpy16_dma((unsigned short*)dstBuf, (unsigned short*)&curPack->start[curPack->tracked], bytesLeft >> 1);
#else
	memcpy(dstBuf, &curPack->start[curPack->tracked], bytesLeft);
#endif
	curPack->tracked += bytesLeft;
	return bytesLeft;
}

#ifdef GBA 
IWRAM
#endif
int AudioV0::ProcessAudio()
{
	ProcessPackets();
	AudioPacket* curPack = GetCurrentPacket();
	if (curPack == nullptr) return 0;


	return Fillbuffers(ringSize, curPack);

	return 0;
}
int AudioV0::Fillbuffers(unsigned int bytesLeft, AudioPacket* curPack)
{
	int retVal = 0;

	int newLen = Copy(curPack, GetBuffer(), bytesLeft);

	//#endif
	return retVal;
}
