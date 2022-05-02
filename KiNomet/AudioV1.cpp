#include "AudioV1.h"
AudioV1::AudioV1(unsigned char* src, int len, int fps, int frames, int (*func)()) :
	AudioHandler(src, len, fps, frames, func)
{
#ifdef DEBUG
	unsigned char* filePointer = src;//Skip past header. if debug do a check
	if (*((unsigned long*)filePointer) != 0x41555631)
	{
		throw "fuck";
	}
	filePointer += 4;
#else
	unsigned char* filePointer = src + 4;//Skip past header. if debug do a check
#endif
	int offsetStart1 = (int)(*(unsigned long*)filePointer); filePointer += 4;

	unsigned long* pointerTable = (unsigned long*)&src[offsetStart1];
	int offsetStart2 = (int)(*(unsigned long*)filePointer); filePointer += 4;
	unsigned long* dataOffsetTable = (unsigned long*)&src[offsetStart2];
	int count = ((int)offsetStart2 - offsetStart1) / 4;
	offsetCount = count;
}



int AudioV1::Copy(AudioPacket* curPack, unsigned char* dstBuf, int len)
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


int AudioV1::Fillbuffers(unsigned int bytesLeft, AudioPacket* curPack)
{
	int retVal = 0;
	bool plsSwap = false;

	//PC can ring buffer, gba will play full sample
//#ifndef GBA
//	if (bytesLeft <= TMP_SIZE)
//	{	//Handle transfer buffer write.
//
//		//Buffer check
//
//		bytesLeft = Copy(curPack, tmpBuf, TMP_SIZE);
//
//		
//		retVal = bytesLeft;
//		plsSwap = true;
//	}
//#endif
	//Write to main buffer for swap.

	int newLen = Copy(curPack, GetBuffer(), bytesLeft);

	//#endif
	return retVal;
}
