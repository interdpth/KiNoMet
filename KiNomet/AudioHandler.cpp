#include "AudioHandler.h"
#include <stdio.h>
#include <cstring>
#ifndef GBA
#include <corecrt_malloc.h>
#endif
#ifdef GBA
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount);
#define IWRAM __attribute__((section(".IWRAM"), target("arm"), noinline))
#endif
#include <cstdlib>
//#define RING_SIZE 0x4000
//#define TMP_SIZE 0x1000
#define GBA_RING_MEM 0x6000000 + 240 * 160 * 2
//Ring buffer is 0x4000 bytes.

#include "MemoryBuffers.h"
void AudioHandler::Init(AudioVersion t, int l, int fp, int sam)
{

	/*tmpBuf = (unsigned char*)malloc(TMP_SIZE);*/
	limitBuf = (unsigned char*)MemoryBuffers::startBuf + RING_SIZE;

	BeginBuffer = (unsigned char*)MemoryBuffers::startBuf;
	currentBuf = (unsigned char*)MemoryBuffers::startBuf;
	type = t;
	fps = fp;
	filesize = l;
	sample_rate = sam;
	swapped = false;
}

AudioKinometPacket* AudioHandler::StartProcessing()
{
	ProcessPackets();
	AudioKinometPacket* curPack = GetCurrentPacket();
	if (curPack == nullptr) return NULL;
	return curPack;
}

#ifdef GBA 
IWRAM
#endif
int AudioHandler::ProcessAudio()
{
	AudioKinometPacket* curPack = StartProcessing();
	if (curPack == nullptr) return 0;

	
	int newLen = Copy(curPack, GetBuffer(), RING_SIZE);

	//#endif
	return newLen;

}



int AudioHandler::Copy(AudioKinometPacket* curPack, unsigned char* dstBuf, int len)
{

	int bytesLeft = len;
	if (curPack->tracked + bytesLeft > curPack->len)
	{
		bytesLeft = curPack->len - (curPack->tracked);
	}

#ifdef GBA
	memcpy16_dma((unsigned short*)dstBuf, &((unsigned short*)curPack->data)[curPack->tracked], bytesLeft >> 1);
#else
	memcpy(dstBuf,   & ((unsigned char*)curPack->data)[curPack->tracked], bytesLeft);
#endif
	curPack->tracked += bytesLeft;
	return bytesLeft;
}

void AudioHandler::InitAudioHandler(AudioHeader* p)
{
	Init(p->type, p->fileLength, p->fps, p->freq);
}

//Ring buffer is 0x4000 bytes.
#ifdef GBA 
IWRAM
#endif
AudioHandler::AudioHandler(AudioHeader* src,  int frames, int (*func)())
{	
	InitAudioHandler(src);

	GetSize = func;	
}


//So process if less than 0x1000 bytes are free, will copy the bytes to tmp buf, 

//Next data is coopied to start buf

//Swap is called
#ifdef GBA 
IWRAM
#endif
void AudioHandler::Swap()
{
	swapped = !swapped;
}

/// <summary>
/// Returns current working packet once ended. 
/// Or returns nullptr
/// </summary>
AudioKinometPacket* AudioHandler::ProcessPackets()
{
	AudioKinometPacket* curPack = GetCurrentPacket();
	if (curPack != nullptr && curPack->tracked >= curPack->len)
	{
		free(packets[0]);
		for (size_t i = 1; i < packets.size(); i++)
		{
			memcpy(&packets[i - 1], &packets[i], sizeof(AudioKinometPacket));
		}

		if (packets.size())packets.pop_back();//bringing all entries forward.
		curPack = GetCurrentPacket();
		if (curPack == nullptr) return nullptr;
		curPack->tracked = 0;
	}

	return curPack;
}

void AudioHandler::ClearAudio()
{
	while (packets.size())
	{
		free(packets[packets.size() - 1]);
		packets.pop_back();
	}
}
void AudioHandler::QueueAudio(AudioKinometPacket* packet)
{
	packets.push_back(packet);
}
#ifdef GBA 
IWRAM
#endif
AudioKinometPacket* AudioHandler::GetCurrentPacket()
{

	if (packets.size() == 0) {
//		while (1);
		return nullptr;

	
	}
	return packets[0];

}
bool AudioHandler::SeekAudio(int frame)
{
	return false;
}

bool AudioHandler::Exhausted()
{
	return GetRemainingBytes() == 0;
}

int AudioHandler::GetRemainingBytes()
{
	return (int)(&limitBuf - &BeginBuffer);;
}

int AudioHandler::GetBytesUsed()
{
	return (int)(BeginBuffer - MemoryBuffers::startBuf);
}

int AudioHandler::GetSampleFreq()
{
	return sample_rate;
}

AudioVersion AudioHandler::GetType()
{
	return type;
}

#ifdef GBA 
IWRAM
#endif
unsigned char* AudioHandler::GetBuffer()
{
	if (swapped)
	{
		return tmpBuf;
	}
	return MemoryBuffers::startBuf;
}
AudioKinometPacket* AudioHandler::GetNextFrame()
{
	return nullptr;

}

