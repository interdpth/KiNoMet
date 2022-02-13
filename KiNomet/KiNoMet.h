#ifndef  KINOMET_H
#define KINOMET_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <Windows.h>
#include "SmallBuffer.h"
#include "Cinepak.h"
#pragma pack(1)
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
typedef struct {
	FOURCC fccType;
	FOURCC fccHandler;
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
	RECT   rcFrame;
} AVIStreamHeader;
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

typedef struct {
	unsigned int FourCC;
	unsigned int dwFlags;
	unsigned int dwOffset;
	unsigned int dwSize;   
}_avioldindex_entry;
void LoadAVI(unsigned char* file, int size, void (*callback)(unsigned char*));
#endif // ! KINOMET_H
