#include "MemoryBuffers.h"


#ifdef GBA

unsigned char* MemoryBuffers::Kinomet_FrameBuffer = (unsigned char*)0x6000000;
unsigned char* MemoryBuffers::startBuf = Kinomet_FrameBuffer + (240 * 160 * 2);//(unsigned char*)malloc(ringSize);
unsigned char* MemoryBuffers::AudioDecompBuffer = startBuf + 0x1000;
unsigned char* MemoryBuffers::VideoDecompBuffer = AudioDecompBuffer + 0x1000;
#else
unsigned char MemoryBuffers::AudioDecompBuffer[0x1000];
unsigned char MemoryBuffers::VideoDecompBuffer[0x1000];
unsigned char MemoryBuffers::Kinomet_FrameBuffer[ScreenSize];
unsigned char MemoryBuffers::startBuf[ScreenSize];
#endif

inmemorybuffer::inmemorybuffer(unsigned char* s, unsigned char* d, int l)
{
	src = s;
	dst = d;
	len = l;
}

/*
  OAM           32    8/16/32   16/32     1/1/1 *
Work RAM 256K 16    8/16/32   8/16/32   3/3/6 **
Palette RAM   16    8/16/32   16/32     1/1/2 *
VRAM          16    8/16/32   16/32     1/1/2 *
  */
//
//int Valid8BitDest(int dst)
//{
//	int dst2 = dst >> 0x18;
//	if (dst == 0x7) return 0;
//	if (dst == 0x6) return 0;
//	if (dst == 0x5) return 0;
//	return 1;
//}


void SafeByteCopy(unsigned char* dst, unsigned char byte)
{

	//Placement means 1234  if placement is 0, then we are refering to 12, 1 we refer to 
	unsigned char* realAddr = dst;
	int onesPlace = (int)dst & 1;

	if (onesPlace)
	{
		realAddr -= 1;//get us even.
	}

	//read val

	unsigned short dat = *(unsigned short*)realAddr;

	if (onesPlace) {
		dat = (dat & 0xFF) | (byte << 8);
	}
	else {
		dat = (dat & 0xFF00) | byte;
	}

	*(unsigned short*)realAddr = dat;
}



//returns bytes adjusted.
//This function is to be used for 2byte writes.
int Safeunsane(inmemorybuffer* buffer, int decider)
{
	//dest 00 10 20 30, where 00010 and 2030 are bytes 
	//src = X
	// if dst & 1 we then get the short at DST -1
	// otherwise we just get the short. 
	//
	//then apply the byte to the short and continue on .
	if (nullptr == buffer->dst || buffer->src == nullptr) return 0;
	int dstAddr = (int)buffer->dst;
	int srcAddr = (int)buffer->src;
	unsigned short realVal = 0;
	unsigned short theByte = 0;

	theByte = (unsigned short)((*buffer->src));

	unsigned char* tmpAddr = buffer->dst;

	SafeByteCopy(buffer->dst, *buffer->src);

	buffer->len--;
	buffer->dst++;
	buffer->src++;
	return 1;
}

int LongCopy(inmemorybuffer* buffer)
{
	int forcedCopy = 0;
	unsigned long* dstp = (unsigned long*)buffer->dst;
	unsigned long* srcp = (unsigned long*)buffer->src;
//
//#ifdef GBA
//	forcedCopy = buffer->len;
//	memcpy32_dma(dstp, srcp, buffer->len >> 2);
//#else 
	for (int counter = 0; counter < (buffer->len >> 2); counter++)
	{
		*dstp++ = *srcp++;
		forcedCopy += 4;
	}
//#endif

	buffer->len -= forcedCopy;
	buffer->dst += forcedCopy;
	buffer->src += forcedCopy;
	return forcedCopy;
}

int UnsignedShortCopy(inmemorybuffer* buffer)
{
	unsigned short* dstp = (unsigned short*)buffer->dst;
	unsigned short* srcp = (unsigned short*)buffer->src;
	int forcedCopy = 0;
	int copyMount = buffer->len >> 1;
//#ifdef GBA
//	forcedCopy = buffer->len;
//	memcpy16_dma(dstp, srcp, buffer->len >> 2);	
//#else 

	for (int counter = 0; counter < (copyMount); counter++)
	{
		*dstp++ = *srcp++;
		forcedCopy += 2;
	}
//#endif

	buffer->len -= forcedCopy;
	buffer->dst += forcedCopy;
	buffer->src += forcedCopy;
	return forcedCopy;
}

//int Copy8Bit(memorybuffer* buffe)
//{
//	int forcedCopy;
//	unsigned char* dstp = (unsigned char*)dst;
//	unsigned char* srcp = (unsigned char*)src;
//	for (forcedCopy = 0; forcedCopy < (len);)
//	{
//		*dstp++ = *srcp++;
//		forcedCopy += 1;
//	}
//	return forcedCopy;
//}

//Only one type of copy happens per loop.
int safe_memcpy(inmemorybuffer* buffer)
{
	int newLen = buffer->len;
	//#ifdef GBA
	int forcedCopy = 0;
	int rem = newLen & 1;

	int even = false;
	int debug = 0xDEADAEEF;

	int retval = 0;
	if (buffer->len == 0) { return 0; }
	while (newLen > 0)
	{

		//write the big stuff. 

		//there was code that handled if dst &1 
		//Capture odd bytes.
		if (((int)buffer->dst & 1) == 1)//For for weird dst
		{
			forcedCopy += Safeunsane(buffer, rem);
		}
		if (((newLen - rem) % 4) == 0)//Write longs.
		{
			debug = 0xDEADDEAD;
			even = true;
			forcedCopy += LongCopy(buffer);
		}
		else if (((newLen - rem) % 2) == 0)//Write shorts
		{
			debug = 0xDEAD;
			even = true;
			forcedCopy += UnsignedShortCopy(buffer);
		}

		if ((buffer->len > 0))
		{
			//should have 1 byte left?
			forcedCopy = Safeunsane(buffer, rem);
		}


		newLen -= forcedCopy;
		retval += forcedCopy;
	}

	//*(unsigned long*)0x200000 = forcedCopy;
	return retval;
}


