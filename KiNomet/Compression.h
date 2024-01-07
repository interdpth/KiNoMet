#pragma once
enum CompType
{

    Raw = 0,
    RLE = 1,
    LZ = 2,
    Pointer = 3, //Always chec kfor pointer, then size difference.         
    ENDME = 0xFD


};
class Compression
{
public:
	Compression();
	int LZDecomp(unsigned char* src, unsigned char* dst, int size);
	int RLEDecomp(unsigned char* src, unsigned char* dst, int size);
	int RawCopy(unsigned char* src, unsigned char* dst, int size);
};

