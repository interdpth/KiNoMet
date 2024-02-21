#include "SmallBuffer.h"
#ifndef GBA
#include <stdio.h>
#endif
SmallBuffer::SmallBuffer(unsigned char* src, int len)
{
	SelfDelete = false;
	unsigned char arr[4] = { 0x56 , 0x34, 0x12, 0x08 };
	unsigned long tmp = 0x8123456;
	
	volatile unsigned char endianCheck = *(unsigned char*)&tmp;
	if (endianCheck == 0x56)
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

SmallBuffer::~SmallBuffer()
{
	if (SelfDelete)
	{
		delete[] start;
	}
}
SmallBuffer::SmallBuffer( int len)
{
	SelfDelete = true;
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
	start = new unsigned char[len];
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

int SmallBuffer::GetRemaining()
{
	return max - pos;
}



unsigned char SmallBuffer::GetByte()
{
	if (pos + 1 > max) {
		printf("awww yes");
	}
	return start[pos++];
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
	if (pos + offset > max) {
		printf("awww yes");
	}
	switch (type)
	{
	case SEEK_CUR: pos += offset; break;
	case SEEK_END: pos = max; break;
	case SEEK_SET: pos = offset;  break;
	}
}
int SmallBuffer::Read(void* dst, int len)
{
	if (pos + len > max) {
		printf("awww yes");
	}
	int i = 0;
	for (i = 0; i < len; i++) ((unsigned char*)dst)[i] = start[pos + i];
	pos += i;
	return i;
}


int SmallBuffer::Read32()
{
	int i = 0;

	char sl[1024] = { 0 };
	if (pos + 4 > max ) {
		printf("awww yes");
	}


	unsigned long val2 = *(unsigned long*)&start[pos];// (start[pos + 0] << 24 | start[pos + 1] << 16 | start[pos + 2] << 8 | start[pos + 3]);

	pos += 4;
	return val2;
}

int SmallBuffer::Read16()
{
	unsigned short val2 = 0;// (start[pos + 0] << 24 | start[pos + 1] << 16 | start[pos + 2] << 8 | start[pos + 3]);

	if (pos + 2 > max) {
		printf("awww yes");
	}
	if (endian == LE)
	{
		val2 = (start[pos + 0] << 8 | start[pos + 1]);
	}
	else {
		val2 = (start[pos + 1] << 8 | start[pos + 0]);
	}
	pos += 2;
	return val2;
}


void SmallBuffer::WriteByte(unsigned char b)
{
	if (max < pos + 1)
	{
		printf("oh jeeze");
	}
	start[pos] = b;
	pos += 1;
}
void SmallBuffer::Write16(unsigned short val)
{
	if (max < pos + 2)
	{
		printf("oh jeeze");
	}
	//if (endian == LE)
	//{
	//	start[pos + 0] = val & 0xFF;
	//	start[pos + 1] = (val >> 8) & 0xFF;
	//}
	//else
	//{
	//	start[pos + 0] = (val >> 8) & 0xFF;
	//	start[pos + 1] = val & 0xFF;
	//}
	*((unsigned short*)(&start[pos])) = val;
	pos += 2;
}
unsigned char* SmallBuffer::GetBuffer()
{
	return start;
}

void SmallBuffer::Write32(unsigned long value)
{

	if (max < pos + 4)
	{
		printf("oh jeeze");
	}

	*((unsigned long*)(&start[pos])) = value;/*
	if (endian == LE)
	{
		start[pos + 0] = (value >> 24) & 0xFF;
		start[pos + 1] = (value >> 16) & 0xFF;
		start[pos + 2] = (value >> 8) & 0xFF;
		start[pos + 3] = value & 0xFF;
	}
	else
	{
		start[pos + 3] = (value >> 24) & 0xFF;
		start[pos + 2] = (value >> 16) & 0xFF;
		start[pos + 1] = (value >> 8) & 0xFF;
		start[pos + 0] = value & 0xFF;
	}*/
	pos += 4;

	return;
}


int SmallBuffer::Pos()
{
	return pos;
}