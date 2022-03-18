#include "AudioHandler.h"
#include <stdio.h>
#include <cstring>
#ifndef GBA
#include <corecrt_malloc.h>
#endif
#include "Gba.h"
#define RING_SIZE 0x4000
#define GBA_RING_MEM 0x6000000 + 240 * 160 * 2 + 0x1000
//Ring buffer is 0x4000 bytes.

void AudioHandler::Init(int t, int fp, int sam)
{
#ifndef GBA
	startBuf = (unsigned char*)malloc(RING_SIZE);
#else
	startBuf = (unsigned char*)(0x6000000 + 240 * 160 * 2);
#endif
	limitBuf = startBuf + RING_SIZE;
	endBuf = startBuf;
	currentBuf = startBuf;
	type = t;
	fps = fp;
	sample_rate = sam;
}



void AudioHandler::Init(AudioHeader* p, int len)
{	
	Init(p->type, p->fps, p->freq);	  
}


AudioHandler::AudioHandler(int t, int fp, int sam, int (*func)())
{
	Init(t, fp, sam);
	GetSize = func;
}

//Ring buffer is 0x4000 bytes.
AudioHandler::AudioHandler(unsigned char* src, int len, int (*func)())
{
	AudioHeader* hdr = (AudioHeader*)src;
	Init(hdr->type, hdr->fps, hdr->freq);

	GetSize = func;
	AudioPacket* p = (AudioPacket*)malloc(sizeof(AudioPacket));
	memset(p, 0, sizeof(AudioPacket));
	p->start =(unsigned char*) &hdr->datPtr;
	p->len = len;
	p->tracked = 0;
	QueueAudio(p);
}


int AudioHandler::Processs()
{
	AudioPacket* curPack = GetCurrentPacket();
	if (curPack == nullptr) return 0;
	if (curPack->tracked >= curPack->len)
	{	
		free(&packets[0]);
		for (int i = 1; i < packets.size(); i++)
		{
			memcpy(&packets[i - 1], &packets[i], sizeof(AudioPacket));
		}

		if (packets.size())packets.pop_back();//bringing all entries forward.
		curPack = GetCurrentPacket();
		if (curPack == nullptr) return 0;
		curPack->tracked = 0;
	}

	int bytesLeft = RING_SIZE - GetSize();//Gotta love pointers.
	//if (bytesLeft==0 || bytesLeft == RING_SIZE)
	//{
		int bytesCopy = 0x4000;
		if (curPack->tracked + 0x4000 > curPack->len)
		{
			bytesCopy = curPack->len - (curPack->tracked);
		}
		memcpy(startBuf, &curPack->start[curPack->tracked], bytesCopy);
	
		curPack->tracked += bytesCopy;
		return bytesCopy;
	//}
	//return 0;
}

void AudioHandler::QueueAudio(AudioPacket* packet)
{
	packets.push_back(*packet);
}

AudioPacket* AudioHandler::GetCurrentPacket()
{
	if (packets.size() == 0) return nullptr;
	return &packets[0];

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


unsigned char* AudioHandler::GetBuffer()
{
	return startBuf;
}
