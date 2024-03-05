#pragma once
#include "CinePakio.h"
#include <vector>
using namespace std;
//Controls the buffer.
class LargeBuffer 
{
//We are a LARGE BUFFER 
private:
	
	unsigned char* src;

	unsigned char** start;
	int pos;
	int max;
	int endian;
	std::vector<unsigned char> dat;
public:
	void SkipByte();
	LargeBuffer(int len);
	LargeBuffer(unsigned char* src, int len);
	~LargeBuffer();
	LargeBuffer(std::vector<unsigned char>* srcp, int len);
	void SetEndian(int e);
	unsigned char* GetCurrentBuffer();
	void Seek(int offset, int type);
	int Read(void* dst, int len);
	int Pos();
	unsigned char* GetBuffer();
	unsigned char GetByte();
	int Read32();
	int Read16();
	int GetLen();
	int GetRemaining();
	void WriteByte(unsigned char b);
	void Write16(unsigned short val);
	void Write32(unsigned long val);
	void ReadCodeBook(memoryCodeBook* c, int mode);
};

