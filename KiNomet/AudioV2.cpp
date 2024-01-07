#include "AudioV2.h"
#include "AudioV2.h"
#include <cstdlib>
AudioV2::AudioV2(unsigned char* src, int len, int fps, int frames, int rsize, int (*func)()) :
	AudioHandler(src, len, fps, frames, rsize, func)
{

	//	ClearAudio();
#ifdef DEBUG
	unsigned char* filePointer = src;//Skip past header. if debug do a check
	if (*((unsigned long*)filePointer) != 0x41555631)
	{
		throw "fuck";
	}
	filePointer += 4;
#else
	unsigned char* filePointer = (unsigned char*)&((AudioHeader*)src)->datPtr;
#endif
	int frameCount = (int)(*(unsigned long*)filePointer); filePointer += 4;
	int offsetStart1 = (int)(*(unsigned long*)filePointer); filePointer += 4;

	unsigned long* pointerTable = (unsigned long*)&src[offsetStart1];
	int offsetStart2 = (int)(*(unsigned long*)filePointer); filePointer += 4;
	dataOffsetTable = (unsigned char*)&src[offsetStart2];
	int count = ((int)offsetStart2 - offsetStart1) / 4;

	dataPointers = pointerTable;
	offsetCount = count;
#ifdef GBA
	decompBuffer = ((unsigned char*)0x6000000 + (240 * 160 * 2) + 0x1000);
#else 
	decompBuffer = (unsigned char*)malloc(0x1000);
#endif 
	frame = 0;
}
//int AudioV2::Processs()
//{
//	ProcessPackets();
//	AudioPacket* curPack = GetCurrentPacket();
//	if (curPack == nullptr) return 0;
//
//	//Herroo
//	unsigned char* block = &dataOffsetTable[dataPointers[frame]];
//	unsigned char* data = block; 
//
//
//	unsigned long compDEADBEEF = *(unsigned long*)data; data += 4;
//	if (compDEADBEEF != 0xDEADBEEF)
//	{
//		while (1);{}
//	}
//	unsigned int id = *(unsigned long*)data; data += 4;
//	unsigned char cmp = *data; data += 1;
//	unsigned short size = *(unsigned short*)data; data += 2;
//	Compression* comp = new Compression();
//	//Pointer is now at data
//	int decompSize = 0;
//	switch (cmp)
//	{
//	case Raw:
//		comp->RawCopy(data, decompBuffer, size);
//		break;
//
//	case RLE:
//		
//		comp->RLEDecomp(data, decompBuffer, size);
//		break;
//	case LZ:
//		comp->LZDecomp(data, decompBuffer, size);
//		break;
//
//	default: 
//		while (1);
//		break;
//	}
//	
//	frame++;
//
//	//modify pack
//
//	return Fillbuffers(ringSize, curPack);
//
//	return 0;
//}

//int AudioV2::Copy(AudioPacket* curPack, unsigned char* dstBuf, int len)
//{
//
//	int bytesLeft = len;
//	if (curPack->tracked + bytesLeft > curPack->len)
//	{
//		bytesLeft = curPack->len - (curPack->tracked);
//	}
//
//#ifdef  GBA
//	memcpy16_dma((unsigned short*)dstBuf, (unsigned short*)&curPack->start[curPack->tracked], bytesLeft >> 1);
//#else
//	memcpy(dstBuf, &curPack->start[curPack->tracked], bytesLeft);
//#endif
//	curPack->tracked += bytesLeft;
//	return bytesLeft;
//}

#ifdef GBA 
IWRAM
#endif
int AudioV2::Processs()
{
	ProcessPackets();
	AudioPacket* curPack = GetCurrentPacket();
	if (curPack == nullptr) return 0;


	//Herroo
	unsigned char* block = &dataOffsetTable[dataPointers[frame]];
	unsigned char* data = block;


	//unsigned long compDEADBEEF = *(unsigned long*)data; data += 4;
	//if (compDEADBEEF != 0xDEADBEEF)
	//{
	//	while (1);{}
	//}
	//unsigned int id = *(unsigned long*)data; data += 4;
	unsigned char cmp = *data; data += 1;
	unsigned short size = *(unsigned short*)data; data += 2;
	Compression* comp = new Compression();
	//Pointer is now at data
	int decompSize = 0;
	switch (cmp)
	{
	case Raw:
		decompSize = comp->RawCopy(data, decompBuffer, size);
		break;

	case RLE:
		decompSize = comp->RLEDecomp(data, decompBuffer, size);
		break;
	case LZ:
		decompSize = comp->LZDecomp(data, decompBuffer, size);
		break;

	default:
		while (1);
		break;
	}

	//curPack->len = decompSize;
	if (curPack->tracked >= decompSize) curPack->tracked = 0;
#ifdef  GBA
	memcpy16_dma((unsigned short*)GetBuffer(), (unsigned short*)decompBuffer, decompSize >> 1);
#else
	memcpy(GetBuffer(), decompBuffer, decompSize);
#endif
	frame++;
	delete comp;
	//modify pack
//	int ret = Fillbuffers(ringSize, curPack);
	return 	((AudioHandler*)this)->Processs();
	;

}
//int AudioV2::Fillbuffers(unsigned int bytesLeft, AudioPacket* curPack)
//{
//	int retVal = 0;
//
//	int newLen = Copy(curPack, GetBuffer(), bytesLeft);
//
//	//#endif
//	return newLen;
//}


//#ifdef GBA 
//IWRAM
//#endif
//unsigned char* AudioV2::GetBuffer()
//{	
//	return decompBuffer;
//}
