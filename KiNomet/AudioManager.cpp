#include "AudioManager.h"
#include <cstddef>
AudioManager::AudioManager(unsigned char* src, int len, int fps, int frames, int (*func)())
{

	int hdr = *(unsigned long*)src;
	if (hdr == 0x41555632)//AUV2
	{
		hndlr = new AudioV2(src, len, fps, frames, fps * ((len - sizeof(AudioHeader)) / frames), func);
		ver = V2;
	}
	if (hdr == 0x41555631)//AUV1
	{
		hndlr = new AudioV1(src, len, fps, frames, fps * ((len - sizeof(AudioHeader)) / frames), func);
		ver = V1;
	}
	else if (hdr == 0x41555630)//AUV0
	{
		hndlr = new AudioV0(src, len, fps, frames, (len / frames) * fps, func);
		ver = V0;
	}
}


int AudioManager::Copy(AudioPacket* curPack, unsigned char* dstBuf, int size)
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

AudioPacket* AudioManager::GetNextFrame()
{
	return	hndlr->GetNextFrame();
}


void AudioManager::Queue(AudioPacket* packet)
{
	return hndlr->QueueAudio(packet);
}

AudioPacket* AudioManager::GetCurrPacket()
{
	return hndlr->GetCurrentPacket();
}

int AudioManager::GetBytesLeft(AudioPacket* curPack)
{

	int bytesLeft = curPack->len - (curPack->tracked);


	return bytesLeft;
}