#include "Compression.h"

Compression::Compression()
{

}
void Compression::LZDecomp(unsigned char* src, unsigned char** dst, int size)
{
	unsigned char* realDst = *dst; 


	*dst = realDst;
}
void Compression::RLEDecomp(unsigned char* src, unsigned char** dst, int size)
{
	unsigned char* realDst = *dst;


	*dst = realDst;
}
void Compression::RawCopy(unsigned char* src, unsigned char** dst, int size)
{
	unsigned char* realDst = *dst;


	*dst = realDst;
}