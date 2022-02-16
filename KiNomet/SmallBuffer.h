#pragma once
#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0
class SmallBuffer
{
private:
	unsigned char* start;
	int pos;
	int max;
public:
	void SkipByte();
	SmallBuffer(unsigned char* src, int len);
	unsigned char* GetCurrentBuffer();
	void Seek(int offset, int type);
    int Read(void* dst, int len);
	int Pos();
	unsigned char GetByte();
};
