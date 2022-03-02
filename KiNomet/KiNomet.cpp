// KiNomet.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "KiNoMet.h"
#include "../KiNomet/Cinepak.h"
#include <stdio.h>
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
	//printf("%s", str);
	exit(-1);
}

static_assert(sizeof(int) == 4, "int size is wrong");
static_assert(sizeof(char) == 1, "char size is wrong");
static_assert(sizeof(short) == 2, "short size is wrong");
static_assert(sizeof(long) == 4, "long size is wrong");
static_assert(sizeof(unsigned int) == 4, "unsigned int in size is wrong");
static_assert(sizeof(unsigned char) == 1, "unsigned char size is wrong");
static_assert(sizeof(unsigned short) == 2, "unsigned short size is wrong");
static_assert(sizeof(unsigned long) == 4, "unsigned long size is wrong");
static_assert(sizeof(unsigned char*) == 4, "unsigned char* size is wrong");
static_assert(sizeof(unsigned short*) == 4, "unsigned short* size is wrong");
//static_assert(sizeof(cvid_codebook)==18, "cvid_codebook size is wrong");
//static_assert(sizeof(cinepak_info)==260, "cinepak_info size is wrong");
static_assert(sizeof(BITMAPINFOHEADER) == 40, "BITMAPINFOHEADER size is wrong");
static_assert(sizeof(MainAVIHeader) == 56, "MainAVIHeader size is wrong");
static_assert(sizeof(_avioldindex_entry) == 16, "_avioldindex_entry size is wrong");
static_assert(sizeof(AVIStreamHeader) == 56, "AVIStreamHeader size is wrong");

unsigned char* Kinomet_FrameBuffer;

void LoadAVI(unsigned char* file, int size, void (*callback)(KinometPacket*), void (*audiocallback)(KinometPacket*))
{

	rectangle screen;


	SmallBuffer* buf = new SmallBuffer(file, size);
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

	MainAVIHeader* hdrz = (MainAVIHeader*)buf->GetCurrentBuffer();
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
	AVIStreamHeader* sthread = (AVIStreamHeader*)buf->GetCurrentBuffer();
	buf->Seek(size, SEEK_CUR);
	if (size != sizeof(AVIStreamHeader))
	{
		int diff = sizeof(AVIStreamHeader);
		diff = --size;
	}
	unsigned int strfTag = 0;
	buf->Read(&strfTag, 4);
	buf->Read(&size, 4);
	if (strfTag != TAG_STRF)  error((char*)"Invalid avi");
	BITMAPINFOHEADER* bmpinf = (BITMAPINFOHEADER*)buf->GetCurrentBuffer();
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


	//we should be at idx1 
	int tagIdx1 = 0;
	buf->Read(&tagIdx1, 4);

	if (tagIdx1 != TAG_IDX1)  error((char*)"Invalid avi");
	buf->Read(&size, 4);

	//Do we make it here? 
	int debug = 0xFFFF1Daa;

	screen.x = 0;
	screen.y = 0;
	screen.w = bmpinf->biWidth;
	screen.h = bmpinf->biHeight;
	KinometPacket pack;
	pack.frame = nullptr;
	pack.frameid = -1;
	pack.rect = (rectangle*)(sthread->dwRate);//packing hack
	pack.screen = &screen;

	callback(&pack);
	//Send a faux packet over to init our consumer. 
//Let's do setup
	int sizescr = screen.w * screen.h * 2;//Rgb //hdrz->dwWidth * hdrz->dwHeight * 3;

//#ifdef GBA
//	Kinomet_FrameBuffer = (unsigned char*)0x6000000;
//#else
	Kinomet_FrameBuffer = (unsigned char*)malloc(sizescr);
	//#endif
	for (int i = 0; i < sizescr / 4; i++)//future iterations should size check but black out the screen
	{
		((unsigned long*)Kinomet_FrameBuffer)[i] = 0;
	}
	int numFrames = size / sizeof(_avioldindex_entry);
	cinepak_info* ci = decode_cinepak_init();
	//It's frame time.
	_avioldindex_entry* idxList = (_avioldindex_entry*)buf->GetCurrentBuffer();

	for (int i = 0; i < numFrames; i++)
	{	//so we will point to
		_avioldindex_entry* cur = &idxList[i];

		//hello what are we
		if (cur->FourCC != TAG_00DC) {
			printf("Hello");
		}

		//sanity stuff.
		unsigned char* frame = moviPointer + cur->dwOffset - 4;
		//Buffer around the frame.

		unsigned int fourcc = *(unsigned long*)frame; frame += 4;

		int framesize = *(unsigned long*)frame; frame += 4;

		if (fourcc != cur->FourCC) continue;

		decode_cinepak(ci, frame, cur->dwSize, Kinomet_FrameBuffer, hdrz->dwWidth, hdrz->dwHeight);
		
		//Hello we have a full framedata 
		pack.frame = Kinomet_FrameBuffer;
		pack.frameid = i;
		pack.screen = &screen;

		callback(&pack);
	}
#ifndef  GBA
	free(Kinomet_FrameBuffer);
#endif
	free_cvinfo(ci);
}

