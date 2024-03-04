#pragma once
class VarBuffer
{

private:
	unsigned char* start;
	int pos;
	int max;
	int endian;
	bool SelfDelete;
public:
	virtual void SkipByte();
	virtual void SetEndian(int e);
	VarBuffer(unsigned char* src, int len);
	VarBuffer(int len);
	~VarBuffer();
	virtual unsigned char* GetCurrentBuffer();
	virtual void Seek(int offset, int type);
	virtual int Read(void* dst, int len);
	virtual int Pos();
	virtual unsigned char GetByte();
	virtual int Read32();
	virtual int Read16();
	virtual int GetLen();
	virtual int GetRemaining();
	virtual unsigned char* GetBuffer();
	virtual void WriteByte(unsigned char b);
	virtual void Write16(unsigned short val);
	virtual void Write32(unsigned long val);

};

