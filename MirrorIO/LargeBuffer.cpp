#include "LargeBuffer.h"


#include "SmallBuffer.h"
#include "stdio.h"
LargeBuffer::LargeBuffer(unsigned char* srcp, int len)
{
	for (int i = 0; i < len; i++)
	{
		dat.push_back(0);
	}
	for (int i = 0; i < len; i++)
	{
		dat[i] = srcp[i];
	}


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
	start = nullptr;
	src = &dat.front();
	
	pos = 0;;
	max = len;
}
LargeBuffer::~LargeBuffer()
{
	dat.resize(0);
}
unsigned char* LargeBuffer:: GetBuffer()
{
	return src;
}


LargeBuffer::LargeBuffer( int len) {

	for (int i = 0; i < len; i++)
	{
		dat.push_back(0);
	}
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
	src = &dat.front();
	start = &src;
	pos = 0;;
	max = len;
}



LargeBuffer::LargeBuffer(std::vector<unsigned char>* srcp, int len) {

	unsigned char* tmp =&(srcp->front());
	//transfer contents to us 
	for (int i = 0; i < len; i++)
	{
		dat.push_back(tmp[i]);
	}

	srcp->resize(0);//we're in control now.

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
	
	src = &dat.front();
	start = &src;
	pos = 0;;
	max = len;
}



unsigned char* LargeBuffer::GetCurrentBuffer()
{
	return &src[pos];
}

int LargeBuffer::GetLen()
{
	return max;
}

int LargeBuffer::GetRemaining()
{
	return max - pos;
}



unsigned char LargeBuffer::GetByte()
{
	return src[pos++];
}
void LargeBuffer::SkipByte()
{
	if (pos + 1 > max)
	{
		return;
	}

	pos++;
}
void LargeBuffer::Seek(int offset, int type)
{
	switch (type)
	{
	case SEEK_CUR: pos += offset; break;
	case SEEK_END: pos = max; break;
	case SEEK_SET: pos = offset;  break;
	}
}
int LargeBuffer::Read(void* dst, int len)
{
	int i = 0;
	for (i = 0; i < len; i++) ((unsigned char*)dst)[i] = src[pos + i];
	pos += i;
	return i;
}


int LargeBuffer::Read32()
{
	int i = 0;

	char sl[1024] = { 0 };

	if (max < pos + 4)
	{
		printf("oh jeeze");
	}


	unsigned long val2 = 0;// (start[pos + 0] << 24 | start[pos + 1] << 16 | start[pos + 2] << 8 | start[pos + 3]);

	if (endian == LE)
	{
		val2 = ((unsigned long)src[pos + 3] << 24) | ((unsigned long)src[pos + 2] << 16) | ((unsigned long)src[pos + 1] << 8) | src[pos + 0];
	}
	else {
		val2 = ((unsigned long)src[pos + 0] << 24) | ((unsigned long)src[pos + 1] << 16) | ((unsigned long)src[pos + 2] << 8) | src[pos + 3];
	}
	pos += 4;
	printf("%x", val2);
	return val2;
}

int LargeBuffer::Read16()
{
	if (max < pos + 2)
	{
		//printf("oh jeeze");
		dat.push_back(0xFF);	dat.push_back(0xFF);
	}
	unsigned short val2 = 0;// (start[pos + 0] << 24 | start[pos + 1] << 16 | start[pos + 2] << 8 | start[pos + 3]);

	if (endian == LE)
	{

		val2=  ((unsigned short)src[pos + 1] << 8) | src[pos +0];
	}
	else {
		val2 = ((unsigned short)src[pos + 0] << 8) | src[pos + 1];
	}

	pos += 2;
	printf("%x", val2);
	return val2;
}

void LargeBuffer::SetEndian(int e)
{
	endian = e;
}

void LargeBuffer::ReadCodeBook(memoryCodeBook* c, int mode)

/* ---------------------------------------------------------------------- */
{
	signed int uvr, uvg, uvb;
	oldcvid_codebook* curbk = ((oldcvid_codebook*)(&src[pos]));
	int y0 = curbk->y0;
	int y1 = curbk->y1;
	int y2 = curbk->y2;
	int y3 = curbk->y3;
	pos += 4;//y0-y3;
	if (mode)        /* black and white */
	{
		c->rgb[0] = MAKECOLOUR16(y0, y0, y0);
		c->rgb[1] = MAKECOLOUR16(y1, y1, y1);
		c->rgb[2] = MAKECOLOUR16(y2, y2, y2);
		c->rgb[3] = MAKECOLOUR16(y3, y3, y3);
	}
	else            /* colour */
	{
		signed 	int v = curbk->v;
		signed 	int u = curbk->u;
		pos += 2;//we read v and u
		uvr = v << 1;
		uvg = -((u + 1) >> 1) - v;
		uvb = u << 1;

		c->rgb[0] = MAKECOLOUR16(uiclp[y0 + uvr], uiclp[y0 + uvg], uiclp[y0 + uvb]);

		c->rgb[1] = MAKECOLOUR16(uiclp[y1 + uvr], uiclp[y1 + uvg], uiclp[y1 + uvb]);

		c->rgb[2] = MAKECOLOUR16(uiclp[y2 + uvr], uiclp[y2 + uvg], uiclp[y2 + uvb]);

		c->rgb[3] = MAKECOLOUR16(uiclp[y3 + uvr], uiclp[y3 + uvg], uiclp[y3 + uvb]);

	}
}

void LargeBuffer::WriteByte(unsigned char b)
{
	if (max < pos + 1)
	{
		//printf("oh jeeze");
		dat.push_back(0xFF);
	}
	src[pos] = b;
	pos += 1;
}
void LargeBuffer::Write16(unsigned short val)
{

	if (endian == LE)
	{
		src[pos + 0] = val & 0xFF;
		src[pos + 1] = (val >> 8) & 0xFF;
	}
	else
	{

		src[pos + 0] = (val >> 8) & 0xFF;
		src[pos + 1] = val & 0xFF;
	}
	pos += 2;
}


void LargeBuffer::Write32(unsigned long value)
{


	if (endian == LE)
	{
		src[pos + 0] = (value >> 24) & 0xFF;
		src[pos + 1] = (value >> 16) & 0xFF;
		src[pos + 2] = (value >> 8) & 0xFF;
		src[pos + 3] = value & 0xFF;
	}
	else
	{
		src[pos + 3] = (value >> 24) & 0xFF;
		src[pos + 2] = (value >> 16) & 0xFF;
		src[pos + 1] = (value >> 8) & 0xFF;
		src[pos + 0] = value & 0xFF;
	}
	pos += 4;

	return;
}


int LargeBuffer::Pos()
{
	return pos;
}