#include "AudioHandler.h"
#include <stdio.h>
#include <cstring>
#ifndef GBA
#include <corecrt_malloc.h>
#endif
#include "Gba.h"
#include <cstdlib>
//#define RING_SIZE 0x4000
//#define TMP_SIZE 0x1000
#define GBA_RING_MEM 0x6000000 + 240 * 160 * 2
//Ring buffer is 0x4000 bytes.

void AudioHandler::Init(int t, int fp, int sam)
{
	startBuf = (unsigned char*)malloc(ringSize);
	/*tmpBuf = (unsigned char*)malloc(TMP_SIZE);*/
	limitBuf = startBuf + ringSize;


	endBuf = startBuf;
	currentBuf = startBuf;
	type = t;
	fps = fp;
	sample_rate = sam;
	swapped = false;
}



void AudioHandler::Init(AudioHeader* p, int len)
{
	Init(p->type, p->fps, p->freq);


}


AudioHandler::AudioHandler(int type, int fp, int sam, int frames, int (*func)())
{
	while (1)
	{
		;
	}
	Init(type, fp, sam);
	GetSize = func;
}

//Ring buffer is 0x4000 bytes.
#ifdef GBA 
IWRAM
#endif
AudioHandler::AudioHandler(unsigned char* src, int len, int fps, int frames, int (*func)())
{
	AudioHeader* hdr = (AudioHeader*)src;
	ringSize = (len / frames) * fps;
	Init(hdr->type, hdr->fps, hdr->freq);

	GetSize = func;
	AudioPacket* p = (AudioPacket*)malloc(sizeof(AudioPacket));
	if (p != nullptr)
	{
		memset(p, 0, sizeof(AudioPacket));
		p->start = (unsigned char*)&hdr->datPtr;
		p->len = len;
		p->tracked = 0;
		QueueAudio(p);
	}
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

int AudioHandler::Copy(AudioPacket* curPack, unsigned char* dstBuf, int len)
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


int AudioHandler::Fillbuffers(unsigned int bytesLeft, AudioPacket* curPack)
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

	int newLen = Copy(curPack, startBuf, bytesLeft);
	//#ifndef GBA
	if (plsSwap)
	{
		Swap();
		swapsize = bytesLeft;
	}
	else
	{
		swapsize = 0;
		retVal = newLen;
	}
	//#endif
	return retVal;
}

/// <summary>
/// Check if we need to be on a new packet.
/// </summary>
void AudioHandler::ProcessPackets()
{
	AudioPacket* curPack = GetCurrentPacket();
	if (curPack != nullptr && curPack->tracked >= curPack->len)
	{
		free(&packets[0]);
		for (int i = 1; i < packets.size(); i++)
		{
			memcpy(&packets[i - 1], &packets[i], sizeof(AudioPacket));
		}

		if (packets.size())packets.pop_back();//bringing all entries forward.
		curPack = GetCurrentPacket();
		if (curPack == nullptr) return;
		curPack->tracked = 0;
	}

}

//Next time this is called, Swap is called again

#ifdef GBA 
IWRAM
#endif
int AudioHandler::Processs()
{
	ProcessPackets();
	AudioPacket* curPack = GetCurrentPacket();
	if (curPack == nullptr) return 0;


	return Fillbuffers(ringSize, curPack);

	return 0;
}

void AudioHandler::QueueAudio(AudioPacket* packet)
{
	packets.push_back(packet);
}
#ifdef GBA 
IWRAM
#endif
AudioPacket* AudioHandler::GetCurrentPacket()
{
	if (packets.size() == 0) return nullptr;
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
	return (int)(&limitBuf - &endBuf);;
}

int AudioHandler::GetBytesUsed()
{
	return (int)(&endBuf - &startBuf);;
}

int AudioHandler::GetSampleFreq()
{
	return sample_rate;
}

int AudioHandler::GetType()
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
	return startBuf;
}
