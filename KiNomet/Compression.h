#pragma once
class Compression
{
public:
	Compression();
	int LZDecomp(unsigned char* src, unsigned char* dst, int size);
	int RLEDecomp(unsigned char* src, unsigned char* dst, int size);
	int RawCopy(unsigned char* src, unsigned char* dst, int size);
};

