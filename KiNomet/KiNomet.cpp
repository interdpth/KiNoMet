// KiNomet.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "KiNoMet.h"
#include "../KiNomet/Cinepak.h"
#include "../KiNomet/TextEngine.h"
#include "../KiNomet/vera.h"
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

static_assert(sizeof(int)==4, "int size is wrong");
static_assert(sizeof(char)==1, "char size is wrong");
static_assert(sizeof(short)==2, "short size is wrong");
static_assert(sizeof(long)==4, "long size is wrong");
static_assert(sizeof(unsigned int)==4, "unsigned int in size is wrong");
static_assert(sizeof(unsigned char)==1, "unsigned char size is wrong");
static_assert(sizeof(unsigned short)==2, "unsigned short size is wrong");
static_assert(sizeof(unsigned long)==4, "unsigned long size is wrong");
static_assert(sizeof(unsigned char*)==4, "unsigned char* size is wrong");
static_assert(sizeof(unsigned short*)==4, "unsigned short* size is wrong");
static_assert(sizeof(cvid_codebook)==18, "cvid_codebook size is wrong");
//static_assert(sizeof(cinepak_info)==260, "cinepak_info size is wrong");
static_assert(sizeof(BITMAPINFOHEADER)==40, "BITMAPINFOHEADER size is wrong");
static_assert(sizeof(MainAVIHeader)==56, "MainAVIHeader size is wrong");
static_assert(sizeof(_avioldindex_entry)==16, "_avioldindex_entry size is wrong");
static_assert(sizeof(AVIStreamHeader)==56, "AVIStreamHeader size is wrong");


void bmp16_line(int x1, int y1, int x2, int y2, unsigned long clr,
	void* dstBase, int dstPitch)
{
	int ii, dx, dy, xstep, ystep, dd;
	unsigned short* dst = (unsigned short*)(dstBase) + y1 * dstPitch + x1 * 2;
	dstPitch /= 2;

	// --- Normalization ---
	if (x1 > x2)
	{
		xstep = -1;  dx = x1 - x2;
	}
	else
	{
		xstep = +1;  dx = x2 - x1;
	}

	if (y1 > y2)
	{
		ystep = -dstPitch;   dy = y1 - y2;
	}
	else
	{
		ystep = +dstPitch;   dy = y2 - y1;
	}


	// --- Drawing ---

	if (dy == 0)         // Horizontal
	{
		for (ii = 0; ii <= dx; ii++)
			dst[ii * xstep] = clr;
	}
	else if (dx == 0)    // Vertical
	{
		for (ii = 0; ii <= dy; ii++)
			dst[ii * ystep] = clr;
	}
	else if (dx >= dy)     // Diagonal, slope <= 1
	{
		dd = 2 * dy - dx;

		for (ii = 0; ii <= dx; ii++)
		{
			*dst = clr;
			if (dd >= 0)
			{
				dd -= 2 * dx; dst += ystep;
			}

			dd += 2 * dy;
			dst += xstep;
		}
	}
	else                // Diagonal, slope > 1
	{
		dd = 2 * dx - dy;

		for (ii = 0; ii <= dy; ii++)
		{
			*dst = clr;
			if (dd >= 0)
			{
				dd -= 2 * dy; dst += xstep;
			}

			dd += 2 * dx;
			dst += ystep;
		}
	}
}
unsigned short* vid_mem;
//! Draw a rectangle on a 16bpp canvas
void bmp16_rect(int left, int top, int right, int bottom, unsigned long clr,
	void* dstBase, int dstPitch)
{
	int ix, iy;

	int width = right - left, height = bottom - top;
	unsigned short* dst = (unsigned short*)dstBase + top * dstPitch + left * 2;
	dstPitch /= 2;

	// --- Draw ---
	for (iy = 0; iy < height; iy++)
		for (ix = 0; ix < width; ix++)
			dst[iy * dstPitch + ix] = clr;
}

//! Draw a frame on a 16bpp canvas
void bmp16_frame(int left, int top, int right, int bottom, unsigned long clr,
	void* dstBase, int dstPitch)
{
	// Frame is RB exclusive
	right--;
	bottom--;

	bmp16_line(left, top, right, top, clr, dstBase, dstPitch);
	bmp16_line(left, bottom, right, bottom, clr, dstBase, dstPitch);

	bmp16_line(left, top, left, bottom, clr, dstBase, dstPitch);
	bmp16_line(right, top, right, bottom, clr, dstBase, dstPitch);
}

//! Plot a single \a clr colored pixel in mode 3 at (\a x, \a y).
void m3_plot(int x, int y, unsigned short clr)
{
	vid_mem[y * 240 + x] = clr;
}

//! Draw a \a clr colored line in mode 3.
void m3_line(int x1, int y1, int x2, int y2, unsigned short clr)
{
	bmp16_line(x1, y1, x2, y2, clr, vid_mem, 240 * 2);
}

//! Draw a \a clr colored rectangle in mode 3.
 void m3_rect(int left, int top, int right, int bottom, unsigned short clr)
{
	bmp16_rect(left, top, right, bottom, clr, vid_mem, 240 * 2);
}

//! Draw a \a clr colored frame in mode 3.
void m3_frame(int left, int top, int right, int bottom, unsigned short clr)
{
	bmp16_frame(left, top, right, bottom, clr, vid_mem, 240 * 2);
}
void LoadAVI(unsigned char* file, int size, void (*callback)(unsigned char*))
{	
	int sizescr = 240 * 2 * 160;//Rgb //hdrz->dwWidth * hdrz->dwHeight * 3;


	unsigned char* rgb = (unsigned char*)malloc(sizescr);
	//Init engine
#ifdef GBA

#endif


	
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
		diff=--size;
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

	int numFrames = size / sizeof(_avioldindex_entry);

//#else 
//	unsigned char* rgb = (unsigned char*)0x6000000;
//#endif // ! GBA

	cinepak_info* ci = decode_cinepak_init();
	//It's frame time.
	_avioldindex_entry* idxList = (_avioldindex_entry*)buf->GetCurrentBuffer();

	//Wait till a frame is drawn before we adance, there for. 


	for (int i = 0; i < numFrames; i++)
	{
		_avioldindex_entry* cur = &idxList[i];
		//so we will point to
		//hello what are we
		if (cur->FourCC != TAG_00DC)
		{
			int a = 1;
			int b = 0;
			a = b;
		}

		//sanity stuff.
		unsigned char* frame = moviPointer + cur->dwOffset - 4;
		//Buffer around the frame.

		int fourcc = *(unsigned long*)frame; frame += 4;

		int framesize = *(unsigned long*)frame; frame += 4;

		if (fourcc != cur->FourCC) continue;
		if (cur->dwSize == 0) {//This frame is empty, don't modify the current one just send it.
		
			continue;
		}
		if (framesize != cur->dwSize) continue;

		/*
		#define 	AVIIF_LIST   0x00000001
#define 	AVIIF_KEYFRAME   0x00000010
#define 	AVIIF_NO_TIME   0x00000100
		*/
		//we are for now rgb16 :(

		decode_cinepak(ci, frame, cur->dwSize, rgb, hdrz->dwWidth, hdrz->dwHeight, 16);
		vid_mem = (unsigned short*)rgb;
#ifdef  GBA

#endif //  GBA

		//Hello we have a full framedata 
		callback(rgb);


	}
//#ifndef  GBA
	free(rgb);
//#endif
	//#endif // ! GBA
	free_cvinfo(ci);
	//fseek(fp,riffHeader->dwSize, SEEK_SET);
	//fread(&hdr, 0, sizeof(MainAVIHeader), fp);
	//printf("lol");
}

