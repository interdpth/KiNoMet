#include "CinePakio.h"
#ifndef GBA
unsigned short inline MAKECOLOUR16(unsigned char r, unsigned  char g, unsigned char b)
{
	return ((((r >> 3) & 31) | (((g >> 3) & 31) << 5) | (((b >> 3) & 31) << 10)));
}
#endif
CinepakIo::CinepakIo(unsigned char* src, int len) 
{
	selfDelete = false;
	start = src;
	pos = 0;
	max = len;
	endian = 3;
}
CinepakIo::CinepakIo(int len) 
{
	selfDelete = true;
	start = new unsigned char[len];
	for (int i = 0; i < len; i++) start[i] = 0;
	pos = 0;
	max = len;
	endian = 3;
}

unsigned char CinepakIo::GetByte()
{
	return start[pos++];
}

unsigned long CinepakIo::Read32()
{
	unsigned long retVal =(unsigned long)((start[pos+0] << 24 | start[pos + 1] << 16 | start[pos + 2] << 8 | start[pos + 3]));
	pos += 4;
	return retVal;
	
}
unsigned short CinepakIo::Read16()
{

	pos += 2;
  return (unsigned short)( (start[pos-2] << 8 | start[pos-1]));

	
}
CinepakIo::~CinepakIo()
{
	if (selfDelete)
	{
		delete[] start;
	}
}

void CinepakIo::SkipByte()
{
	pos++;
}
void CinepakIo::ReadCodeBook(memoryCodeBook* c, int mode)

/* ---------------------------------------------------------------------- */
{
	signed int uvr, uvg, uvb;
	oldcvid_codebook* curbk = ((oldcvid_codebook*)(&start[pos]));
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