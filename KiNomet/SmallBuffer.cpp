#include "SmallBuffer.h"

SmallBuffer::SmallBuffer(unsigned char* src, int len)
{
	start = src;
	pos = 0;;
	max = len;
}
unsigned char* SmallBuffer::GetCurrentBuffer()
{
	return &start[pos];
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
int SmallBuffer::Pos()
{
	return pos;
}