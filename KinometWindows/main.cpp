#include "KiNoMet.h"
#include "../KinoMet/Cinepak.h"
#include "gbavideo.h"
#include <stdio.h>
int frameHandled;
void handleFrame(unsigned char* framePointer)
{
	//we are gba so frame is always 240*160*2;
	char strbuf[256] = { 0 };
	sprintf(strbuf, "frame%d.bin", frameHandled++);
	
	FILE* fp = fopen(strbuf, "wb");
	if (fp)
	{
		fwrite(framePointer, 2, 240 * 160, fp);
		fclose(fp);
	}
}

int main(int arc, char* argv[])
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

	int intz = sizeof(int);
	int uintz = sizeof(unsigned long);
	//this will be on gba, so we're just gonna load the whole thing in and work with pointers.
	frameHandled = 0;
	LoadAVI((unsigned char*)Video, Video_size, &handleFrame);	
}

