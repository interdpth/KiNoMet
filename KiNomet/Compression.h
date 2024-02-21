#pragma once
#include "cinepak.h"

typedef std::vector<uint8_t> Buffer;
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
    static int LZDecomp(inmemorybuffer* buffer);
    static int RLEDecomp(inmemorybuffer* buffer);
    static int RawCopy(inmemorybuffer* buffer);
    static Buffer LZCompress(Buffer& b);
};

