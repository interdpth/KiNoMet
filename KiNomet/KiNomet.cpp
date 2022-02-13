// KiNomet.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "KiNoMet.h"
CHUNK* getCHUNK(SmallBuffer* fp)
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
	//printf("%s", str);
	exit(-1);
}


void LoadAVI(unsigned char* file, int size, void (*callback)(unsigned char*))
{
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
	//buf->Seek(size, SEEK_CUR);
	//Once again a list? 
	//hello movi

	//we should be at idx1 
	int tagIdx1 = 0;
	buf->Read(&tagIdx1, 4);

	if (tagIdx1 != TAG_IDX1)  error((char*)"Invalid avi");
	buf->Read(&size, 4);

	//Do we make it here? 
	int debug = 0xFFFF1Daa;
	
	int numFrames = size / sizeof(_avioldindex_entry);
	int sizescr = hdrz->dwWidth * hdrz->dwHeight * 2;
	unsigned char* rgb = (unsigned char*)malloc(sizescr);
	cinepak_info* ci = decode_cinepak_init();
	//It's frame time.
	_avioldindex_entry* idxList = (_avioldindex_entry*)buf->GetCurrentBuffer();
	for (int i = 0; i < numFrames; i++)
	{
		_avioldindex_entry* cur = &idxList[i];
		//so we will point to
		//hello what are we
		if (cur->FourCC != TAG_00DC) continue;

		//sanity stuff.
		unsigned char* frame = moviPointer + cur->dwOffset - 4;
		int fourcc = *(unsigned long*)frame; frame += 4;

		int framesize = *(unsigned long*)frame; frame += 4;



		if (fourcc != cur->FourCC) continue;
		if (framesize != cur->dwSize) continue;


		//we are for now rgb16 :(

		decode_cinepak(ci, frame, size, rgb, hdrz->dwWidth, hdrz->dwHeight, 16);

		//Hello we have a full framedata 
		callback(rgb);


		cur++;

	}
	free(rgb);
	free_cvinfo(ci);
	//fseek(fp,riffHeader->dwSize, SEEK_SET);
	//fread(&hdr, 0, sizeof(MainAVIHeader), fp);
	//printf("lol");
}

