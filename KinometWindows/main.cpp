#include "KiNoMet.h"
#include "../KinoMet/Cinepak.h"
#include "VideoFile.h"
#include <stdio.h>

int frameHandled;
int codeBookSize();
int height = 0;
int width = 0;

void handleFrame(KinometPacket* pack)
{
	//IF this is called nad packet is null, we are just setting up. 

	if (pack->frame == nullptr && !height && !width)
	{
		height = pack->screen->h;
		width = pack->screen->w;
		return;
	}


	//we are gba so frame is always 240*160*2;
	char strbuf[256] = { 0 };
	sprintf(strbuf, "F:\\processing\\frame%d.bin", frameHandled++);

	FILE* fp = fopen(strbuf, "wb");
	if (pack->frame && fp)
	{
		fwrite(pack->frame, 2, width * height, fp);
		fclose(fp);
	}
}
extern int maxNum;
extern int codeBooks;
#ifdef __cplusplus
extern "C" {
#endif
int SDL_main(int argc, char* argv[])
{
	printf("sizeof(int) %d\n", (int)sizeof(int));
	printf("sizeof(char) %d\n", (int)sizeof(char));
	printf("sizeof(short) %d\n", (int)sizeof(short));
	printf("sizeof(long) %d\n", (int)sizeof(long));
	printf("sizeof(unsigned int) %d\n", (int)sizeof(unsigned int));
	printf("sizeof(unsigned char) %d\n", (int)sizeof(unsigned char));
	printf("sizeof(unsigned short) %d\n", (int)sizeof(unsigned short));
	printf("sizeof(unsigned long) %d\n", (int)sizeof(unsigned long));
	printf("sizeof(unsigned char*) %d\n", (int)sizeof(unsigned char*));
	printf("sizeof(unsigned short*) %d\n", (int)sizeof(unsigned short*));
	printf("sizeof(cvid_codebook) %d\n", (int)sizeof(cvid_codebook));
	printf("sizeof(cinepak_info) %d\n", (int)sizeof(cinepak_info));
	printf("sizeof(BITMAPINFOHEADER) %d\n", (int)sizeof(BITMAPINFOHEADER));
	printf("sizeof(MainAVIHeader) %d\n", (int)sizeof(MainAVIHeader));
	printf("sizeof(_avioldindex_entry) %d\n", (int)sizeof(_avioldindex_entry));
	printf("sizeof(AVIStreamHeader) %d\n", (int)sizeof(AVIStreamHeader));
	int b = sizeof(arachicoldcvid_codebook);
	int intz = sizeof(int);
	int uintz = sizeof(unsigned long);
	//this will be on gba, so we're just gonna load the whole thing in and work with pointers.
	frameHandled = 0;
	LoadAVI((unsigned char*)VideoFile, VideoFile_size, &handleFrame);
	int overallSize = codeBookSize();
	printf("%d", maxNum);
	printf("%x", overallSize);
	printf("%d", codeBooks);
	return 0;
}
#ifdef __cplusplus
}
#endif
