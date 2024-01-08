#include "AudioV1.h"
#include "MemoryBuffers.h"
#include <cstdlib>
AudioV1::AudioV1(unsigned char* src, int len, int fps, int frames, int rsize, int (*func)()) :
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

	frame = 0;
}


#ifdef GBA 
IWRAM
#endif
int AudioV1::ProcessAudio()
{

	AudioPacket * curPack = StartProcessing();
	if (curPack == nullptr) return 0;


	//Herroo
	unsigned char* block = &dataOffsetTable[dataPointers[frame]];
	unsigned char* data = block;


	unsigned long compDEADBEEF = *(unsigned long*)data; data += 4;
	if (compDEADBEEF != 0xDEADBEEF)
	{
		while (1);{}
	}
	//unsigned int id = *(unsigned long*)data; data += 4;
	unsigned char cmp = *data; data += 1;
	unsigned short size = *(unsigned short*)data; data += 2;

	//Pointer is now at data
	int decompSize = 0;
	switch (cmp)
	{
	case Raw:
		decompSize = Compression::RawCopy(data, MemoryBuffers::DecompBuffer, 128);
		break;
	case RLE:
		decompSize = Compression::RLEDecomp(data, MemoryBuffers::DecompBuffer, 128);
		break;
	case LZ:
		decompSize = Compression::LZDecomp(data, MemoryBuffers::DecompBuffer, 128);
		break;

	default:
		while (1);
		break;
	}


	if (decompSize == 0)
	{

		while (1);
	}
	curPack->len = decompSize;
	curPack->tracked = 0;

	curPack->start = MemoryBuffers::DecompBuffer;

	int newLen = Copy(curPack, GetBuffer(), decompSize);
	frame++;
	//#endif
	return newLen;


}



