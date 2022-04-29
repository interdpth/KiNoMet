#include "SmallBuffer.h"

SmallBuffer::SmallBuffer(unsigned char* src, int len)
{
	unsigned char arr[4] = { 0x56 , 0x34, 0x12, 0x08 };
	unsigned long endianCheck = *(unsigned long*)arr;
	if (endianCheck == 0x8123456)
	{
		endian = LE;

	}
	else
	{
		endian = BE;
	}
	//Determien endian
	start = src;
	pos = 0;;
	max = len;
}
unsigned char* SmallBuffer::GetCurrentBuffer()
{
	return &start[pos];
}

int SmallBuffer::GetLen()
{
	return max;
}

int SmallBuffer :: GetRemaining()
{
	return max - pos;
}



unsigned char SmallBuffer::GetByte()
{
	return start[pos];
}
void SmallBuffer::SkipByte()
{
	if (pos + 1 > max)
	{
		return;
	}

	pos++;
}
void SmallBuffer::Seek(int offset, int type)
{
	switch (type)
	{
	case SEEK_CUR: pos += offset; break;
	case SEEK_END: pos = max; break;
	case SEEK_SET: pos = offset;  break;
	}
}
int SmallBuffer::Read(void* dst, int len)
{
	int i = 0;
	for (i = 0; i < len; i++) ((unsigned char*)dst)[i] = start[pos + i];
	pos += i;
	return i;
}


int SmallBuffer::Read32()
{
	int i = 0;


	unsigned short val2 = 0;// (start[pos + 0] << 24 | start[pos + 1] << 16 | start[pos + 2] << 8 | start[pos + 3]);

	if (endian == LE)
	{
		val2 = (start[pos + 0] << 24 | start[pos + 1] << 16 | start[pos + 2] << 8 | start[pos + 3]);
	}
	else {
		val2 = (start[pos + 3] << 24 | start[pos + 2] << 16 | start[pos + 1] << 8 | start[pos + 0]);
	}
	pos += 4;
	return val2;
}

int SmallBuffer::Read16()
{
	unsigned short val2 = 0;// (start[pos + 0] << 24 | start[pos + 1] << 16 | start[pos + 2] << 8 | start[pos + 3]);

	if (endian == LE)
	{
		val2 = (start[pos + 0] << 8 | start[pos + 1]);
	}
	else {
		val2 = ( start[pos + 1] << 8 | start[pos + 0]);
	}
	pos += 2;
	return val2;
}

int SmallBuffer::Pos()
{
	return pos;
}