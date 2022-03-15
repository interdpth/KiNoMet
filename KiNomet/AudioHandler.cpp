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

void AudioHandler::Init(int t, int fp)
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
}
AudioHandler::AudioHandler(int t, int fp)
{
	Init( t,  fp);
}

//Ring buffer is 0x4000 bytes.
AudioHandler::AudioHandler(int t, int fp, unsigned char* src, int len)
{
	Init(t, fp);
	QueueAudio(src, len);
}
void AudioHandler::QueueAudio(unsigned char* src, int len)
{


		
	
}

bool  AudioHandler::SeekAudio(int frame)
{
	return false;
}

bool  AudioHandler::Exhausted()
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
