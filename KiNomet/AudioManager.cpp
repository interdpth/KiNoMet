#include "AudioManager.h"
AudioManager::AudioManager(unsigned char* src, int len, int fps, int frames, int (*func)())
{
	
	int hdr = *(unsigned long*)src;
	if (hdr == 0x41555631)
	{
		hndlr = new AudioV1(src, len, fps, frames, func);
		ver = V1;
	}
	else
	{
		hndlr = new AudioHandler(src, len, fps, frames, func);
		ver = V0;
	}
}



int AudioManager::Copy(AudioPacket* curPack, unsigned char* dstBuf, int size)
{
	if (ver == V1)
	{
	  return	((AudioV1*)hndlr)->Copy(curPack, dstBuf, size);
	}
	else {
		return	((AudioHandler*)hndlr)->Copy(curPack, dstBuf, size);
	}
	return -1;
}
int AudioManager::Fillbuffers(unsigned int bytesLeft, AudioPacket* curPack)
{
	if (ver == V1)
	{
		return	((AudioV1*)hndlr)->Fillbuffers(bytesLeft, curPack);
	}
	else {
		return	((AudioHandler*)hndlr)->Fillbuffers(bytesLeft, curPack);
	}
	return -1;
}
int AudioManager::Processs()
{
	if (ver == V1)
	{
		return	((AudioV1*)hndlr)->Processs();
	}
	else {
		return	((AudioHandler*)hndlr)->Processs();
	}
	return -1;
}


unsigned char* AudioManager::GetBuffer()
{
	if (ver == V1)
	{
		return	((AudioV1*)hndlr)->GetBuffer();
	}
	else {
		return	((AudioHandler*)hndlr)->GetBuffer();
	}
	return NULL;
}




int AudioManager::GetSampleFreq()
{
	//if (ver == V1)
	//{
	//	return	((AudioV1*)hndlr)->GetSampleFreq();
	//}
	//else {
		return	((AudioHandler*)hndlr)->GetSampleFreq();
	//}
	//return -1;
}

int AudioManager::GetType()
{
	if (ver == V1)
	{
		return	((AudioV1*)hndlr)->GetType();
	}
	else {
		return	((AudioHandler*)hndlr)->GetType();
	}
	return -1;
}