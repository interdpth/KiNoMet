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
#pragma pack(1)
/*
The first 4 bytes are a four-character code'RIFF', indicating that this is a RIFF file; the following 4 bytes are used to indicate the size of the RIFF file;
Then there is a four-character code indicating the specific type of the file (such as AVI, WAVE, etc.); the last is the actual data. Pay attention to the calculation of the file size value
The method is: the actual data length + 4 (the size of the file type field); that is, the value of the file size does not include the'RIFF' field and the "file size" field itself
the size of.
*/
typedef struct RIFF
{
	char riff[4];
	unsigned int size;
	char type[4];


}RIFF;
/*
Note that the calculation method of the listSize value is: the actual list data length + 4 (the size of the listType field); that is, the listSize value does not include the size of the'LIST' field and the listSize field itself.
*/
typedef struct list
{
	char fcc[4];
	unsigned int size;
	char type[4];
}LIST;
/*************avih*********************/
typedef struct _avimainheader
{
	char fcc[4];//'avih'
	int size;//The size of this data structure, not including the first 8 bytes (fcc and cb fields)
	int dwMicroSecPerFrame;//Video frame interval time (in microseconds)
	int dwMaxBytesPerSec;//The maximum data rate of this AVI file
	int dwPaddingGranularity;//The granularity of data padding
	int dwFlags;//Global flags of AVI files, such as whether it contains index blocks, etc.
	int dwTotalFrames;//total number of frames
	int dwInitialFrames;//Specify the initial frame number for interactive format (non-interactive format should be specified as 0)
	int dwStreams;//The number of streams contained in this file
	int dwSuggestedBufferSize;//It is recommended to read the buffer size of this file (should be able to hold the largest block)
	int dwWidth;//The width of the video image (in pixels)
	int dwHeight;//The height of the video image (in pixels)
	int dwReserved[4];//reserved
}AVIMAINHEADER;
/********strh*******/
typedef struct
{
	short int left;
	short int top;
	short int right;
	short int bottom;
}RCFRAME;
typedef struct _avistreamheader
{
	char fcc[4];//Must be'strh'
	int size;
	char fccType[4];//stream type:'auds' (audio stream),'vids' (video stream),
	char fccHandler[4];//The processor of the specified stream is a decoder for audio and video
	int dwFlags;//Mark: Is this stream output allowed? Does the color palette change?
	int wPriority;//The priority of the stream (when there are multiple streams of the same type, the one with the highest priority is the default stream)
	int wLanguage;
	int dwInitialFrames;//Specify the initial number of frames for the interactive format
	int dwScale;//The time scale used by this stream
	int dwRate;
	int dwStart;//The start time of the stream
	int dwLength;//The length of the stream (the unit is related to the definition of dwScale and dwRate)
	int dwQuality;//Quality index of stream data (0 ~ 10,000)
	int dwSampleSize;//Sample size
	RCFRAME rcFrame;//Specify the display position of this stream (video stream or text stream) in the main video window
}AVISTREAMHEADER;
/**************strf_vids********************/
typedef struct tagBITMAPINFOHEADER
{
	char fcc[4];
	int biSize;
	int biWidth;
	int biHeight;
	short int biPlanes;
	short int biBitCount;
	int biCompression;
	int biSizeImage;
	int biXPelsPerMeter;
	int biYPelsPerMeter;
	int biClrUsed;
	int biClrImportant;
}BITMAPINFOHEADER;
typedef struct tagBITMAPINFO
{
	BITMAPINFOHEADER bmiHeader;
	int bimColors[1];
}BITMAPINFO;
/***************strf_auds*********************/
//#define WAVEFORMAT __attribute__((packed))
typedef struct
{
	char fcc[4];
	int size;
	short int wFormatTag;
	short int nChannels;
	int nSamplesPerSec;
	int nAvgBytesPerSec;
	short int nBlockAlign;
	short int wBitsPerSample;
	short int biSize;


}WAVEFORMAT;

typedef struct strl_vids
{
	LIST list;
	AVISTREAMHEADER strh;

	BITMAPINFO strf;
}STRL_VIDS;
typedef struct strl_auds
{
	LIST list;
	AVISTREAMHEADER strh;
	WAVEFORMAT strf;

}STRL_AUDS;
typedef struct junk
{

	char fcc[4];
	int size;
}JUNK;
typedef struct HDRL
{
	AVIMAINHEADER avih;
	STRL_VIDS strl_vids;
	STRL_AUDS strl_auds;
	JUNK junk;
}HDRL;
typedef struct movi
{
	char id[4];
	int length;

}MOVI;
typedef struct data
{
	LIST list;
	MOVI chunk;
}DATA;
typedef struct aindex
{
	char dwChunkId[4];
	int dwFlags;
	int dwOffset;
	int dwSize;
}AINDEX;
typedef struct avi_idxl
{
	char fcc[4];
	AINDEX aindex[10];
}AVI_IDXL;
void print_avihead(RIFF riff, LIST list, HDRL hdrl)
{
	printf("riff.size=%d\n", riff.size);
	printf("list.size=%d\n", list.size);
	printf("hdrl.avih.fcc=%s\n", hdrl.avih.fcc);
	printf("hdrl.avih.dwTotalFrames=%d\n", hdrl.avih.dwTotalFrames);
	printf("hdrl.avih.dwMicroSecPerFrame=%dms\n", hdrl.avih.dwMicroSecPerFrame / 1000);
	printf("hdrl.strl_vids.strh.fcc=%s\n", hdrl.strl_vids.strh.fcc);
	printf("hdrl.strl_auds.strh.fcc=%s\n", hdrl.strl_auds.strh.fcc);
	printf("hdrl.junk.fcc=%c\n", hdrl.junk.fcc[3]);
	printf("hdrl.junk.size=%d\n", hdrl.junk.size);

}

void get_aviidxl(AVI_IDXL* idxl, char* src_buf, int off_set)
{
	char* p;
	p = src_buf + off_set;
	memcpy(idxl, p, sizeof(AVI_IDXL));
	printf("idxl.fcc:%s\n", idxl->fcc);

}
long get_file_size(const char* path)
{
	unsigned long filesize = -1;
	FILE* fp;
	fp = fopen(path, "r");
	if (fp == NULL)
	{
		return filesize;
	}
	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fclose(fp);
	return filesize;
}
int loader(char* src)
{
	unsigned long size=0;
	int count = 0, i;
	int begin_s = 60;//start time s
	int t_s = 20;//Interception interval s
	FILE* fd_src;
	FILE* fd_dst;
	int off_set;
	char* buf, * readbuf;
	char* src_buf, * tmp;

	RIFF riff;
	LIST lhdrl;
	HDRL hdrl;
	LIST lmovi;
	MOVI movi;
	/*******************Open source file and destination file************************ *****/
	fd_src = fopen(src, "rb");
	fd_dst = fopen("out.avi","wb");
	fseek(fd_dst, 0, SEEK_SET);
	/************Read the content of the source file into the buffer area***********************/
	fseek(fd_src, 0, SEEK_SET);
	fread(&riff, 1, sizeof(riff), fd_src);
	fread(&lhdrl, 1, sizeof(lhdrl), fd_src);
	fread(&hdrl,1, lhdrl.size + 4, fd_src);
	print_avihead(riff,lhdrl,hdrl);

		/*********Create a JUNK buffer to store 0************/
	buf = (char*)malloc(hdrl.junk.size+1);
	memset(buf, 0, hdrl.junk.size);
	/**********Write the file header and JUNK area to the target file **********/
	fseek(fd_dst, sizeof(riff), SEEK_CUR);
	fseek(fd_dst, sizeof(lhdrl), SEEK_CUR);
	fseek(fd_dst, sizeof(hdrl), SEEK_CUR);
	//write(fd_dst,&riff,sizeof(riff));
	//write(fd_dst,&list,sizeof(list));
	//write(fd_dst,&hdrl,sizeof(hdrl));
	fwrite(buf, 1, hdrl.junk.size, fd_dst);
	free(buf);

	///***********Offset to the movi area to get actual data***************/
	//fseek(fd_src, hdrl.junk.size, SEEK_CUR);
	//fread(fd_src, &lmovi, sizeof(lmovi));
	//fwrite(fd_dst, &lmovi, sizeof(lmovi));
	///******Write data*****/
	//while (1)//for(i = 0; i <20; i++)
	//{
	//	fread(fd_src, &movi, sizeof(movi));
	//	if (movi.length % 2 != 0)
	//	{
	//		movi.length += 1;
	//	}
	//	buf = (char*)malloc(movi.length);
	//	fread(fd_src, buf, movi.length);
	//	//printf("%d\tmovi.id=%s\n",count,movi.id); 
	//	if (((strncmp(movi.id + 2, "db", 2) == 0) || (strncmp(movi.id + 2, "dc", 2) == 0)) && (count < begin_s * 25))//25 frame is 1 second
	//	{
	//		free(buf);
	//		count++;
	//		//printf("%d\tmovi.id=%s\n",count,movi.id); 

	//		continue;
	//	}
	//	if (((strncmp(movi.id + 2, "pc", 2) == 0) || (strncmp(movi.id + 2, "wb", 2) == 0)) && ((count - 1) < begin_s * 25))//audio frames are not counted, time is only calculated by video frames
	//	{
	//		free(buf);

	//		continue;
	//	}
	//	if (((strncmp(movi.id + 2, "db", 2) == 0) || (strncmp(movi.id + 2, "dc", 2) == 0)) && (count >= begin_s * 25))
	//	{
	//		count++;
	//		printf("%d\tmovi.id=%s\tmovi.size=%x\n", count, movi.id, movi.length);

	//	}
	//	fwrite(fd_dst, &movi, sizeof(movi));
	//	fwrite(fd_dst, buf, movi.length);
	//	size = size + movi.length + 8;
	//	free(buf);

	//	if (count > (begin_s + t_s) * 25)
	//	{
	//		break;
	//	}
	//	//free(buf);
	//	//size=size+movi.length+8;
	//}

	//riff.size = 12 + 12 + lhdrl.size + 4 + hdrl.junk.size + 12 + size;//Calculate the data size to fill the header
	//hdrl.avih.dwTotalFrames = count;//size bytes
	//fseek(fd_dst, 0, SEEK_SET);
	//fwrite(fd_dst, &riff, sizeof(riff));
	//fwrite(fd_dst, &lhdrl, sizeof(lhdrl));
	//fwrite(fd_dst, &hdrl, sizeof(hdrl));

	////off_set=off_set+8+data.list.size;
	////get_aviidxl(&idxl,src_buf,off_set);
	////write(fd_dst,src_buf,size);

	fclose(fd_src);
	fclose(fd_dst);
	return 0;
}
void main()
{
	loader((char*)"F:\\Processing\\alie2.avi");
}

