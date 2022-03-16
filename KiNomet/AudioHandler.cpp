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


AudioHandler::AudioHandler(int t, int fp, int sam)
{
	Init(t, fp, sam);
}

//Ring buffer is 0x4000 bytes.
AudioHandler::AudioHandler(unsigned char* src, int len)
{
	AudioHeader* hdr = (AudioHeader*)src;
	Init(hdr->type, hdr->fps, hdr->freq);
	AudioPacket* p;
	memset(&p, 0, sizeof(AudioPacket));
	p.start = hdr->datPtr;
	p.len = len;
	QueueAudio(&p);
}


void AudioHandler::Processs()
{

}

void AudioHandler::QueueAudio(AudioPacket* packet)
{
	  
}

AudioPacket* AudioHandler::GetCurrentPacket()
{
	return &packets[0];

}
bool  AudioHandler::SeekAudio(int frame)
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
