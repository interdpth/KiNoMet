#pragma once
#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0
#define LE 0
#define BE 1
class SmallBuffer
{
private:
	unsigned char* start;
	int pos;
	int max;
	int endian;
	bool SelfDelete;
public:
	void SkipByte();
	void SetEndian(int e);
	SmallBuffer(unsigned char* src, int len);
	SmallBuffer(int len);
	~SmallBuffer();
	unsigned char* GetCurrentBuffer();
	void Seek(int offset, int type);
	int Read(void* dst, int len);
	int Pos();
	unsigned char GetByte();
	int Read32();
	int Read16();
	int GetLen();
	int GetRemaining();
	unsigned char* GetBuffer();
	void WriteByte(unsigned char b);
	void Write16(unsigned short val);
	void Write32(unsigned long val);
};
