#include "..\KiNomet\KiNoMet.h"
//#include "gbavideo.h"

int frameHandled;
void handleFrame(unsigned char* framePointer)
{
	//we are gba so frame is always 240*160*2;
    while(1);
}


int main()
{
	//this will be on gba, so we're just gonna load the whole thing in and work with pointers.
	frameHandled = 0;
///	LoadAVI((unsigned char*)Video, Video_size, &handleFrame);	
	return 0;
}

