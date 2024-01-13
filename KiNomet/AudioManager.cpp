#include "AudioManager.h"
#include <cstddef>

//WE NEED TO STORE RAW FILE AND COMPRESSED FILE SIZE
//AUDOIOFSIZE IS COMPRESSED
//LEN IS UNCOMPRESSED
//COMPRESSED FOR PACKETS
//DECOMPRESSED FOR LENGTH.
void AudioManager::Init(AudioHeader* src, int len, int fps, int frames, int (*func)())
{
	int hdr = src->hdr;
	if (hdr == 0x41555632)//AUV2
	{
		hndlr = new AudioV2(src, frames, func);
		ver = V2;
	}
	if (hdr == 0x41555631)//AUV1
	{
		hndlr = new AudioV1(src, frames, func);
		ver = V1;
	}
	else if (hdr == 0x41555630)//AUV0
	{
		hndlr = new AudioV0(src, frames, func);
		ver = V0;
	}
}


int AudioManager::Copy(AudioDataPacket* curPack, unsigned char* dstBuf, int size)
{
	return	(hndlr)->Copy(curPack, dstBuf, size);
}


int AudioManager::ProcessAudio()
{
	return	hndlr->ProcessAudio();
}


unsigned char* AudioManager::GetBuffer()
{
	return	hndlr->GetBuffer();
}

int AudioManager::GetSampleFreq()
{
	return	hndlr->GetSampleFreq();
}

int AudioManager::GetType()
{
	return	hndlr->GetType();
}

AudioDataPacket* AudioManager::GetNextFrame()
{
	return	hndlr->GetNextFrame();
}


void AudioManager::Queue(AudioDataPacket* packet)
{
	return hndlr->QueueAudio(packet);
}

AudioDataPacket* AudioManager::GetCurrPacket()
{
	return hndlr->GetCurrentPacket();
}

int AudioManager::GetBytesLeft(AudioDataPacket* curPack)
{

	int bytesLeft = curPack->len - (curPack->tracked);


	return bytesLeft;
}