#include "Compression.h"

#define u32 unsigned long

#define u16 unsigned short
#define u8 unsigned char

u32 LZCompWrite(u8* srcp, u32 size, u8* dstp, u8 lzSearchOffset=0)
{
#ifdef DEBUG_PRINT
	fprintf(stderr, "LZCompWrite\tsize=%d\tlzSearchOffset=%d\n", size, lzSearchOffset);
#endif

	u32  LZDstCount;    // Compression data size in bytes
	u32  LZSrcCount;    // Size of data that is the target of compression, after processing is complete (bytes)
	u8   LZCompFlags;   // Flag sequence that indicates whether compression or not.
	u8* LZCompFlagsp;  // Points to the memory region that stores LZCompFlags
	u8* compressingDataHeadp;  // Beginning address of the current data region that is targetted for compression.
	u8* searchp;       // Beginning address of the matching data search region
	u16  tmpOffset;     // Offset to the matching data (mid-search data)
	u8   tmpLength;     // Matching data length (mid-search data)
	u16  lastOffset;    // Offset to matching data (longest matching data at that time)
	u8   lastLength;    // Matching data length (longest matching data at that time)
	u8   i;

	*(u32*)dstp = size << 8 | 0x10;                 // Data header
	LZDstCount = 4;

	LZSrcCount = 0;
	while (LZSrcCount < size)
	{
		LZCompFlags = 0;
		LZCompFlagsp = &dstp[LZDstCount++];          // Flag sequence storage destination

		// Loop 8 times because the flag sequence is stored as 8-bit data.
		for (i = 0; i < 8; i++)
		{
			LZCompFlags <<= 1;  // First loop (i=0) has no meaning in particular
			lastOffset = 0;
			lastLength = 0;
			compressingDataHeadp = &srcp[LZSrcCount];

			// VRAM access is in 2-byte units (because there are times when we read out data from VRAM).
			// Therefore, you must search from data that is at least 2 bytes previous.
			//
			// Because offset is stored as 12-bit, no more than 4096
			for (tmpOffset = lzSearchOffset; tmpOffset <= LZSrcCount && tmpOffset <= 4096; tmpOffset++)
			{
				if (LZSrcCount >= size) break;
				searchp = &srcp[LZSrcCount - tmpOffset];    // Settings for search data region
				if (*searchp == *compressingDataHeadp)      // Matching data found
				{
					// Because data length is stored as 4-bit, max 18 (3 added from the bottom)
					for (tmpLength = 1; tmpLength <= 16 + 2; tmpLength++)
						if (searchp[tmpLength] != compressingDataHeadp[tmpLength]) break;
					if (tmpLength > 16 + 2)               tmpLength = 18; // So that it does not become 19
					if (LZSrcCount + tmpLength >= size) tmpLength = (u8)(size - LZSrcCount);

					// Update longest matching data
					if (tmpLength >= 3 && tmpLength > lastLength) {
						lastOffset = tmpOffset;
						lastLength = tmpLength;
						LZCompFlags |= 0x01;
					}
				}
			}
			// LZCompFlags Store one unit of compression data
			if (LZSrcCount < size) {
				if ((LZCompFlags & 0x01) == 0)  // No compression
					dstp[LZDstCount++] = srcp[LZSrcCount++];
				else {
					// Split offset to the top 4 bits and the bottom 8 bits, and store.
					dstp[LZDstCount++] = (lastLength - 3) << 4 | (lastOffset - 1) >> 8;
					dstp[LZDstCount++] = (lastOffset - 1) & 0xff;
					LZSrcCount += lastLength;
				}
			}
		}   // End of 8 loops
		*LZCompFlagsp = LZCompFlags;                  // Store flag sequence
	}

	// 4-byte border alignment
	// 0 data for alignment is not included in the data size.
	i = 0;
	while ((LZDstCount + i) & 0x3) {
		dstp[LZDstCount + i] = 0;
		i++;
	}

	return LZDstCount;
}


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

Buffer Compression::LZCompress(Buffer& b)
{
	Buffer n;
	n.resize(b.size());


	return b;
	//int realLen = LZCompWrite((unsigned char*)&b.front(), b.size(), (unsigned char*)&n.front());

	//Buffer n2;
	//for (int i = 0; i < realLen; i++)
	//{
	//	n2.push_back(n[i]);
	//}
	//return n2;
}