#include "AudioV1.h"
#include "MemoryBuffers.h"
#include <cstdlib>
AudioV1::AudioV1(AudioHeader* src,   int frames, int (*func)()) :
	AudioHandler(src,  frames, func)
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
	unsigned char* filePointer = (unsigned char*)(src + sizeof(AudioHeader));
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
AudioKinometPacket* AudioV1::GetNextFrame() {

	//Herroo
	unsigned char* block = &dataOffsetTable[dataPointers[frame]];
	unsigned char* data = block;


	unsigned long compDEADBEEF = *(unsigned long*)data; data += 4;
	if (compDEADBEEF != 0xDEADBEEF)
	{
		while (1); {}
	}


	unsigned long frameNo = *(unsigned long*)data; data += 4;

	//unsigned int id = *(unsigned long*)data; data += 4;
	unsigned char cmp = *data; data += 1;
	unsigned short size = *(unsigned short*)data; data += 2;
	inmemorybuffer buff(  data, MemoryBuffers::AudioDecompBuffer, 0x1000 );
	//Pointer is now at data
	int decompSize = 0;
	switch (cmp)
	{
	case Raw:
		decompSize = Compression::RawCopy(&buff);
		break;
	case RLE:
		decompSize = Compression::RLEDecomp(&buff);
		break;
	case LZ:
		decompSize = Compression::LZDecomp(&buff);
		break;

	default:
		while (1);
		break;
	}

	//Malloc the packet
	if (decompSize == 0)
	{

		while (1);
	}
	
	AudioKinometPacket* curPack = new AudioKinometPacket(V1, NULL, frameNo, decompSize, (unsigned char*)(MemoryBuffers::AudioDecompBuffer));

	frame++;
	
	return curPack;
}
