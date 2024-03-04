#pragma once
#ifdef GBA
typedef struct __attribute__((__packed__))
#else
typedef struct
#endif
{
	unsigned char y0, y1, y2, y3;
	signed char u, v;
	/* unsigned char reds[4], greens[4], blues[4];*/
} oldcvid_codebook;

/// <summary>
/// We only store converted colors in this doo hickey
/// </summary>
typedef struct
{
	//unsigned char y0, y1, y2, y3;
	//signed char u, v;

	unsigned short rgb[4];
	//  unsigned char reds[4], greens[4], blues[4];
} memoryCodeBook;
extern unsigned char* uiclp;
#ifndef GBA
unsigned short MAKECOLOUR16(unsigned char r, unsigned char g, unsigned char b);
#else 
extern "C" unsigned short MAKECOLOUR16(unsigned char r, unsigned char g, unsigned char b);
extern "C" int GBA_RLEDECOMP(void* src, void* dst);
extern "C"  int GBA_LZDECOMP(void* src, void* dst);
#endif
class CinepakIo
{
private:
	unsigned char* start;
	int pos;
	int max;
	int endian;
	bool selfDelete;
public:
	void SkipByte();
	void SetEndian(int e);
	CinepakIo(unsigned char* src, int len);
	CinepakIo(int len);
	~CinepakIo();
	unsigned char* GetCurrentBuffer();
	void Seek(int offset, int type);
	int Read(void* dst, int len);
	int Pos();
	unsigned char GetByte();
	unsigned long Read32();
	unsigned short  Read16();
	int GetLen();
	int GetRemaining();
	unsigned char* GetBuffer();
	void WriteByte(unsigned char b);
	void Write16(unsigned short val);
	void Write32(unsigned long val);
	void ReadCodeBook(memoryCodeBook* c, int mode);
};
