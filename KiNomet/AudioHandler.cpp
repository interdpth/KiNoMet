#include "AudioHandler.h"
#include <stdio.h>
#include <cstring>
#ifndef GBA
#include <corecrt_malloc.h>
#endif
#include "Gba.h"
#include <cstdlib>
#define RING_SIZE 0x4000
#define TMP_SIZE 0x1000
#define GBA_RING_MEM 0x6000000 + 240 * 160 * 2
//Ring buffer is 0x4000 bytes.

void AudioHandler::Init(int t, int fp, int sam)
{
#ifndef GBA
	startBuf = (unsigned char*)malloc(RING_SIZE);
	tmpBuf = (unsigned char*)malloc(TMP_SIZE);
#else
	startBuf = (unsigned char*)(0x6000000 + 240 * 160 * 2);
	
	tmpBuf = startBuf + RING_SIZE;
#endif
	limitBuf = startBuf + RING_SIZE;
	
	
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
void AudioHandler::Swap()
{

	swapped = !swapped;
}

//Next time this is called, Swap is called again
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


	if (swapped)
	{
		Swap();
		return swapsize;
	}


	int size = GetSize();
	unsigned int bytesLeft = RING_SIZE - size;//Gotta love pointers.

	int retVal = RING_SIZE;
	if (bytesLeft != 0) {
		bool plsSwap = false;
		if (bytesLeft <= TMP_SIZE)
		{

			bytesLeft = TMP_SIZE;
			//Handle transfer buffer write.

			//Buffer check

			int bytesCopy = bytesLeft;
			if (curPack->tracked + bytesLeft > curPack->len)
			{
				bytesLeft = curPack->len - (curPack->tracked);
			}


			memcpy(tmpBuf, &curPack->start[curPack->tracked], bytesLeft);

			curPack->tracked += bytesLeft;
			retVal = bytesLeft;
			plsSwap = true;
		}
		//else
		//{
			//Write to main buffer for swap.
		bytesLeft = RING_SIZE;
		if (curPack->tracked + bytesLeft > curPack->len)
		{
			bytesLeft = curPack->len - (curPack->tracked);
		}


		memcpy(startBuf, &curPack->start[curPack->tracked], bytesLeft);

		curPack->tracked += bytesLeft;
		if (plsSwap)
		{
			Swap();
			swapsize = bytesLeft;

		}
		else
		{
			swapsize = 0;
			retVal = bytesLeft;
		}
			
	//	}

		
		return retVal;
	}
	return 0;
}

void AudioHandler::QueueAudio(AudioPacket* packet)
{
	packets.push_back(packet);
}

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


unsigned char* AudioHandler::GetBuffer()
{
	if (swapped)
	{
		return tmpBuf;
	}
	return startBuf;
}
