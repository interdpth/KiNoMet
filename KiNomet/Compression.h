#pragma once
class Compression
{
public:
	Compression();
	void LZDecomp(unsigned char* src, unsigned char** dst, int size);
	void RLEDecomp(unsigned char* src, unsigned char** dst, int size);
	void RawCopy(unsigned char* src, unsigned char** dst, int size);
};

