#include "Compression.h"
#include "cinepak.h"
#include "Gba.h"
Compression::Compression()
{

}
#ifndef GBA
int GBA_LZDECOMP(void* src, void* dst) {
	unsigned char* srcp = (unsigned char*)src;
	unsigned char* dstp = (unsigned char*)dst;


	uint32_t header = *(uint32_t*)srcp; srcp += 4;
	uint8_t magic = (header & 0xff);
	uint32_t decompressedLength = (header >> 8);

	if (magic != 0x10) {
		printf("Incorrect magic number, expected 0x10, got 0x%02x\n", magic);
		return 1;
	}

	uint8_t* destBuffer = new uint8_t[decompressedLength];
	size_t bytesWritten = 0;
	while (bytesWritten < decompressedLength)
	{
		uint8_t flags = *(uint8_t*)srcp; srcp += 1;
		for (int i = 0; i < 8 && bytesWritten < decompressedLength; ++i)
		{
			bool type = (flags & (0x80 >> i));
			if (!type)
			{
				uint8_t value = *(uint8_t*)srcp; srcp += 1;
				destBuffer[bytesWritten] = value;
				bytesWritten++;
			}
			else
			{
				uint16_t value = *(uint16_t*)srcp; srcp += 2;
				uint16_t disp = ((value & 0xf) << 8) | (value >> 8);
				uint8_t n = ((value >> 4) & 0xf);
				//	printf("pos %x, src %x, length %d\n", bytesWritten, bytesWritten-disp-1, n+3);
				for (int j = 0; j < n + 3; ++j)
				{
					destBuffer[bytesWritten] = destBuffer[bytesWritten - disp - 1];
					bytesWritten++;
				}
			}
		}
	}
	return bytesWritten;
}
int GBA_RLEDECOMP(void* src, void* dst, int dstS)
{
	if (!dst || !src)return 0;

	unsigned int ii, size = 0;
	unsigned short* srcL = (unsigned short*)src, * dstD = (unsigned short*)dst;

	for (ii = 0; ii < dstS; ii += size)
	{
		// Get header byte
		unsigned int header = *srcL++;

		if (header & 0x80)		// compressed stint
		{
			size = min((header & ~0x80) + 3, dstS - ii);

			int j; for (j = 0; j < size; j++)dstD[ii + j] = *srcL; //can't used memset for 16bit
			srcL++;
		}
		else				// noncompressed stint
		{
			size = min(header + 1, dstS - ii);
			memcpy(&dstD[ii], srcL, size * 2);

			srcL += size;
		}
	}

	return dstS;
}
#endif


int Compression::LZDecomp(unsigned char* src, unsigned char* dst, int size)
{
	unsigned char* realDst = dst;



	realDst += GBA_LZDECOMP(src, dst); 


	return realDst - dst;
}
int Compression::RLEDecomp(unsigned char* src, unsigned char* dst, int size)
{
	unsigned char* realDst = dst;


#ifdef GBA

	realDst += GBA_RLEDECOMP(src, dst);
#else
#endif
	return realDst - dst;
}
int Compression::RawCopy(unsigned char* src, unsigned char* dst, int size)
{
	unsigned char* realDst = dst;

#ifdef GBA
	memcpy16_dma((unsigned short*)realDst, (unsigned short*)src, size>>1); realDst += size;
#else
	memcpy(realDst, src, size); realDst += size;
#endif

	return realDst - dst;
}
