// KiNomet.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

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

CHUNK* getCHUNK(SmallBuffer * fp)
{
	CHUNK* me = new RIFF();
	fp->Read(&me->dwFourCC, 4);
	
	fp->Read(&me->dwSize, 4);
	me->data = fp->GetCurrentBuffer();

	//me->data =(BYTE*) malloc(me->dwSize);
	//fp->Read(me->data, me->dwSize);
	return me;
}
#define TAG_RIFF 0x46464952//  'RIFF' is 52 49 46 46, but we'll read it as long because costs
#define LIST_LIST 0x5453494C//  'LIST' is 4C 49 53 54, but we'll read it as long because costs

#define TAG_AVI 0x20495641 //(unsigned long)'AVI '
#define TAG_HDRL 0x6C726468 //hdrl
#define TAG_AVIH 0x68697661 //avih
#define TAG_STRL 0x6C727473
#define TAG_STRH 0x68727473
#define TAG_STRF 0x66727473
#define TAG_JUNK 0x4B4E554A
#define TAG_VPRP 0x70727076
#define TAG_MOVI 0x69766F6D
#define TAG_00DC 0x63643030
#define TAG_IDX1 0x31786469

void error(char* str)
{
	printf("%s",str);
	exit(-1);
}


void loader(unsigned char* file, int size)
{
	SmallBuffer* buf = new SmallBuffer(file, size);
	MainAVIHeader *hdr;
	//memset(&hdr, 0, sizeof(MainAVIHeader));
	bool bValid = false;
	int pos = 0; 

	unsigned int tag = 0;
	buf->Read(&tag, 4);
	if (tag != TAG_RIFF) error((char*)"Invalid avi");
	buf->Read(&tag, 4);//emptinesss
	//WE should now read a AVI
	unsigned int tagAvi = 0;
	buf->Read(&tagAvi, 4);
	if (tagAvi != TAG_AVI)  error((char*)"Invalid avi");

	//we should now be at a list
	unsigned int tagList = 0;
	buf->Read(&tagList, 4);
	if (tagList != LIST_LIST)  error((char*)"Invalid avi");
	buf->Seek(4, SEEK_CUR);
	unsigned int hdrlTag = 0;
	buf->Read(&hdrlTag, 4);

	if (hdrlTag != TAG_HDRL)  error((char*)"Invalid avi");
	unsigned int avihlTag = 0;
	buf->Read(&avihlTag, 4);
	if (avihlTag != TAG_AVIH)  error((char*)"Invalid avi");
	size = 0;
	buf->Read(&size, 4);

	MainAVIHeader* hdrz = (MainAVIHeader * )buf->GetCurrentBuffer();
	buf->Seek(size, SEEK_CUR);
	tagList = 0;
	buf->Read(&tagList, 4);
	if (tagList != LIST_LIST)  error((char*)"Invalid avi");
	buf->Read(&size, 4);
	unsigned int strlTag = 0;
	buf->Read(&strlTag, 4);
	
	if (strlTag != TAG_STRL)  error((char*)"Invalid avi");

	unsigned int strhTag = 0;
	buf->Read(&strhTag, 4);
	buf->Read(&size, 4);
	if (strhTag != TAG_STRH)  error((char*)"Invalid avi");
	AVIStreamHeader* sthread = (AVIStreamHeader * )buf->GetCurrentBuffer();
	buf->Seek(size, SEEK_CUR);
	unsigned int strfTag = 0;
	buf->Read(&strfTag, 4);
	buf->Read(&size, 4);
	if (strfTag != TAG_STRF)  error((char*)"Invalid avi");
	BITMAPINFOHEADER* bmpinf = (BITMAPINFOHEADER * )buf->GetCurrentBuffer();
	buf->Seek(size, SEEK_CUR);
	//should be junk? 

	unsigned int junkTag = 0;
	buf->Read(&junkTag, 4);
	buf->Read(&size, 4);
	if (junkTag != TAG_JUNK)  error((char*)"Invalid avi");
	buf->Seek(size, SEEK_CUR);
	//okay now we have frame chunks?

	//we may have a prp section, we don't care.

	unsigned int vprpTag = 0;
	buf->Read(&vprpTag, 4);
	buf->Read(&size, 4);
	if (vprpTag != TAG_VPRP)  error((char*)"Invalid avi");
	buf->Seek(size, SEEK_CUR);
	junkTag = 0;
	buf->Read(&junkTag, 4);
	buf->Read(&size, 4);
	if (junkTag != TAG_JUNK)  error((char*)"Invalid avi");
	buf->Seek(size, SEEK_CUR);
	//once again we should be at a list, i hate avi spec
	tagList = 0;
	buf->Read(&tagList, 4);
	buf->Read(&size, 4);
	if (tagList != LIST_LIST)  error((char*)"Invalid avi");
	buf->Seek(size, SEEK_CUR);
	junkTag = 0;
	buf->Read(&junkTag, 4);
	buf->Read(&size, 4);
	if (junkTag != TAG_JUNK)  error((char*)"Invalid avi");
	buf->Seek(size, SEEK_CUR);
	tagList = 0;
	buf->Read(&tagList, 4);
	buf->Read(&size, 4);
	if (tagList != LIST_LIST)  error((char*)"Invalid avi");
	int tagMovi = 0;
	buf->Read(&tagMovi, 4);

	if (tagMovi != TAG_MOVI)  error((char*)"Invalid avi");
	unsigned char* moviPointer = buf->GetCurrentBuffer();
	buf->Seek(size - 4, SEEK_CUR);
	//buf->Seek(size, SEEK_CUR);
	//Once again a list? 
	//hello movi

	//we should be at idx1 
	int tagIdx1 = 0;
	buf->Read(&tagIdx1, 4);

	if (tagIdx1 != TAG_IDX1)  error((char*)"Invalid avi");
	buf->Read(&size, 4);


	int numFrames = size/sizeof(_avioldindex_entry);
	unsigned char* rgb = (unsigned char*)malloc(hdrz->dwWidth * hdrz->dwHeight * 2);
	cinepak_info* ci = decode_cinepak_init();
	//It's frame time.
	_avioldindex_entry* cur = (_avioldindex_entry * )buf->GetCurrentBuffer();
	for (int i = 0; i < numFrames; i++)
	{
		//so we will point to
		//hello what are we
		if (cur->FourCC != TAG_00DC) continue;

		//sanity stuff.
		unsigned char* frame = moviPointer + cur->dwOffset-4;
		int fourcc = *(unsigned long*)frame; frame += 4;

		int framesize = *(unsigned long*)frame; frame += 4;

		

		if (fourcc != cur->FourCC) continue;
		if (framesize != cur->dwSize) continue;
	
		char buff[256];
		//sprintf(buff, "rawfilecinepakc%d", i);
		//FILE* fp = fopen(buff, "wb");
		//
		//fclose(fp);

		sprintf(buff, "rawfilecinepaku%d", i);
		FILE* fp = fopen(buff, "wb");
		//we are for now rgb16 :(
		if (fp) {
			decode_cinepak(ci, frame, size, rgb, hdrz->dwWidth, hdrz->dwHeight, 16);

			fwrite(rgb, 1, hdrz->dwWidth * hdrz->dwHeight * 2, fp);

			fclose(fp);
		}
		cur++;

	}
		free(rgb);
		free_cvinfo(ci);
	//fseek(fp,riffHeader->dwSize, SEEK_SET);
	//fread(&hdr, 0, sizeof(MainAVIHeader), fp);
	printf("lol");
}

void main()
{
	//this will be on gba, so we're just gonna load the whole thing in and work with pointers.

	FILE* fp = fopen((char*)"F:\\Processing\\alie2.avi", "rb");
	fseek(fp, 0, SEEK_END);
	int bufsize = ftell(fp);
	unsigned char* buffer = new unsigned char[bufsize];
	fseek(fp, 0, SEEK_SET);
	fread(buffer, 1, bufsize, fp);
	fclose(fp);
	

	loader(buffer, bufsize);
	delete[] buffer;
}

