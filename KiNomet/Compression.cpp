#include "Compression.h"

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
	if (decompressedLength > 0 && destBuffer) {
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
	}
	return bytesWritten;
}

int GBA_RLEDECOMP(inmemorybuffer* buffer)
{
	if (!buffer->dst || !buffer->src)return 0;

	unsigned int ii, size = 0;
	unsigned short* srcL = (unsigned short*)buffer->src, * dstD = (unsigned short*)buffer->dst;

	for (ii = 0; ii < buffer->len; ii += size)
	{
		// Get header byte
		unsigned int header = *(srcL)++;

		if (header & 0x80)		// compressed stint
		{
			size = min((header & ~0x80) + 3, buffer->len - ii);

			int j; for (j = 0; j < size; j++)dstD[ii + j] = *srcL; //can't used memset for 16bit
			srcL++;
		}
		else				// noncompressed stint
		{
			size = min(header + 1, buffer->len - ii);
			inmemorybuffer tmp ((unsigned char*) & dstD[ii] , (unsigned char*)srcL, size * 2);
			safe_memcpy(&tmp);

			srcL += size;
		}
	}

	return buffer->len;
}
#endif

//WHY ARE THESE NOT USING SWIS
int Compression::LZDecomp(inmemorybuffer* buffer)
{
	unsigned char* realDst = buffer->dst;
	 unsigned char* oldDst = buffer->dst;


	realDst += GBA_LZDECOMP(buffer->src, buffer->dst);


	return realDst - oldDst;
}
int Compression::RLEDecomp(inmemorybuffer* buffer)
{
	unsigned char* realDst = buffer->dst;


#ifdef GBA

	realDst += GBA_RLEDECOMP(buffer->src, buffer->dst);
#else
	realDst += GBA_RLEDECOMP(buffer);

#endif
	return realDst - buffer->dst;
}
int Compression::RawCopy(inmemorybuffer* buffer)
{
	unsigned char* realDst = buffer->dst;
	unsigned char* oldDst = buffer->dst;
	int size = buffer->len;
#ifdef GBA
	memcpy16_dma((unsigned short*)realDst, (unsigned short*)buffer->src, size>>1); realDst += size;
#else
	safe_memcpy(buffer); realDst += size;
#endif

	return realDst - oldDst;
}
