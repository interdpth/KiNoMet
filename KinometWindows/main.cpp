#include "KiNoMet.h"
#include "gbavideo.h"

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
	//this will be on gba, so we're just gonna load the whole thing in and work with pointers.
	frameHandled = 0;
	LoadAVI((unsigned char*)Video, Video_size, &handleFrame);
	
}

