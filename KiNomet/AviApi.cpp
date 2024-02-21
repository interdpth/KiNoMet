#include "AviApi.h"



#define error(s) {printf(s); exit(-1);}

void LoadAviInfo(SmallBuffer* buf, MainAVIHeader** hdrz, AVIStreamHeader** sthread, BITMAPINFOHEADER** bmpinf, unsigned char** moviPointer)
{
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
	int size = 0;
	buf->Read(&size, 4);

	*hdrz = (MainAVIHeader*)buf->GetCurrentBuffer();
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
	*sthread = (AVIStreamHeader*)buf->GetCurrentBuffer();
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
	*bmpinf = (BITMAPINFOHEADER*)buf->GetCurrentBuffer();
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
	*moviPointer = buf->GetCurrentBuffer();
	buf->Seek(size - 4, SEEK_CUR);


	//we should be at idx1 
	int tagIdx1 = 0;
	buf->Read(&tagIdx1, 4);

	if (tagIdx1 != TAG_IDX1)  error((char*)"Invalid avi");
	buf->Read(&size, 4);
	return;
}