
#ifndef KINOMET_H
#define KINOMET_H

#include <stdlib.h>

#include "Gba.h"
#ifdef GBA
typedef unsigned int       DWORD;
typedef unsigned int       LONG;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
#else 
#include <Windows.h>
#endif
#pragma pack(push,2)
#ifdef GBA
typedef struct __attribute__((__packed__))
tagBITMAPINFOHEADER {
	
		DWORD biSize;
	LONG  biWidth;
	LONG  biHeight;
	WORD  biPlanes;
	WORD  biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG  biXPelsPerMeter;
	LONG  biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER, * LPBITMAPINFOHEADER, * PBITMAPINFOHEADER;
#else
typedef struct tagBITMAPINFOHEADER;
#endif

struct rectangle
{
	unsigned char x, y, w, h;
};

struct KinometPacket
{
	unsigned char* frame;
	rectangle* rect;
	rectangle* screen;
	int frameid;
};

#include "SmallBuffer.h"
#include "Cinepak.h"

#define 	AVIIF_LIST   0x00000001
#define 	AVIIF_KEYFRAME   0x00000010
#define 	AVIIF_NO_TIME   0x00000100
//https://cdn.hackaday.io/files/274271173436768/avi.pdf
/*
A chunk containing video, audio or subtitle data uses a dwFourCC containing 2 hexadecimal
digits specifying the stream number and 2 letters specifying the data type (dc = video, wb
= audio, tx = text). The values dwFourCC and dwSize have the same meaning in both of
the structures:
dwFourCC describes the type of the chunk (for example 'hdrl' for 'header list'), and dwSize
contains the size of the chunk or list, including the rst byte after the dwSize value. In the
case of Lists, this includes the 4 bytes taken by dwFourCC!
The value of dwList can be 'RIFF' ('RIFF-List') or 'LIST' ('List').


*/
/*a. The data is always padded to nearest WORD boundary. ckSize gives the size of the valid data in the chunk; it does not include the padding, the size of ckID, or the size of ckSize.*/

/*
* We do not use these structures, but they are here for reference
typedef struct {

	DWORD dwFourCC;
	DWORD dwSize;
	BYTE* data;// [dwSize] ; // contains headers or video/audio data
} RIFF, CHUNK;

typedef struct {
	DWORD dwList;
	DWORD dwSize;
	DWORD dwFourCC;
	BYTE* data;// [dwSize - 4] ; // contains Lists and Chunks
} LIST;
*/
#ifdef GBA
typedef struct tagRECT
{
	unsigned long    left;
	unsigned long     top;
	unsigned long     right;
	unsigned long     bottom;
} RECT;

#endif
#ifdef GBA
typedef struct __attribute__((__packed__))
{
	unsigned int fccType;
	unsigned int fccHandler;
	DWORD  dwFlags;
	WORD   wPriority;
	WORD   wLanguage;
	DWORD  dwInitialFrames;
	DWORD  dwScale;
	DWORD  dwRate;
	DWORD  dwStart;
	DWORD  dwLength;
	DWORD  dwSuggestedBufferSize;
	DWORD  dwQuality;
	DWORD  dwSampleSize;

	short int left;
	short int top;
	short int right;
	short int bottom;

} AVIStreamHeader;
#else
typedef struct {
	unsigned int fccType;
	unsigned int fccHandler;
	DWORD  dwFlags;
	WORD   wPriority;
	WORD   wLanguage;
	DWORD  dwInitialFrames;
	DWORD  dwScale;
	DWORD  dwRate;
	DWORD  dwStart;
	DWORD  dwLength;
	DWORD  dwSuggestedBufferSize;
	DWORD  dwQuality;
	DWORD  dwSampleSize;

	short int left;
	short int top;
	short int right;
	short int bottom;

} AVIStreamHeader;
#endif

#ifdef GBA
typedef struct __attribute__((__packed__))
{
	DWORD dwMicroSecPerFrame; // frame display rate (or 0)
	DWORD dwMaxBytesPerSec; // max. transfer rate
	DWORD dwPaddingGranularity; // pad to multiples of this
	// size;
	DWORD dwFlags; // the ever-present flags
	DWORD dwTotalFrames; // # frames in file
	DWORD dwInitialFrames;
	DWORD dwStreams;
	DWORD dwSuggestedBufferSize;
	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwReserved[4];
} MainAVIHeader;

#else
typedef struct
{
	DWORD dwMicroSecPerFrame; // frame display rate (or 0)
	DWORD dwMaxBytesPerSec; // max. transfer rate
	DWORD dwPaddingGranularity; // pad to multiples of this
	// size;
	DWORD dwFlags; // the ever-present flags
	DWORD dwTotalFrames; // # frames in file
	DWORD dwInitialFrames;
	DWORD dwStreams;
	DWORD dwSuggestedBufferSize;
	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwReserved[4];
} MainAVIHeader;

#endif

typedef struct {
	unsigned int FourCC;
	unsigned int dwFlags;
	unsigned int dwOffset;
	unsigned int dwSize;   
}_avioldindex_entry;
#pragma pack(pop)
void LoadAVI(unsigned char* file, int size, void (*callback)(KinometPacket*));
extern unsigned char* Kinomet_FrameBuffer;
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount);
#endif // ! KINOMET_H
