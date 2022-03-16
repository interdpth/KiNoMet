// KiNomet.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "KiNoMet.h"
#include "../KiNomet/Cinepak.h"
#include "AviApi.h"
#include <stdio.h>
#include "AudioHandler.h"
void error(char* str)
{
	//printf("%s", str);
	exit(-1);
}



unsigned char* Kinomet_FrameBuffer;
int frameCount;
unsigned char* audioBuffer;

void CreateFrameBuffer(int w, int h)
{
	//Let's do setup
	int sizescr = w * h * 2;//Rgb //hdrz->dwWidth * hdrz->dwHeight * 3;

//#ifdef GBA
//	Kinomet_FrameBuffer = (unsigned char*)0x6000000;
//#else
	Kinomet_FrameBuffer = (unsigned char*)malloc(sizescr);
	//	#endif
	for (int i = 0; i < sizescr / 4; i++)//future iterations should size check but black out the screen
	{
		((unsigned long*)Kinomet_FrameBuffer)[i] = 0;
	}
}

void SetAudioPacket(KinometPacket* pack)
{

}

void LoadAVI(unsigned char* file,
	int size,
	unsigned char* audiofile,
	int audiofsize,
	void (*callback)(KinometPacket*),
	void (*audiocallback)(KinometPacket*),
	int(*GetSize)())
{
	rectangle screen;
	AudioHandler* audio;
	//audioSystem = new AllSight(audiofile, audiofsize);
	//audioSystem->Parse();
	SmallBuffer* buf = new SmallBuffer(file, size);
	//memset(&hdr, 0, sizeof(MainAVIHeader));
	bool bValid = false;
	int pos = 0;


	//Do we make it here? 
	int debug = 0xFFFF1Daa;
	BITMAPINFOHEADER* bmpinf = nullptr;

	AVIStreamHeader* sthread = nullptr;
	MainAVIHeader* hdrz = nullptr;
	unsigned char* moviPointer = nullptr;

	LoadAviInfo(buf, &hdrz, &sthread, &bmpinf, &moviPointer);
	if (bmpinf == nullptr || sthread == nullptr || hdrz == nullptr || moviPointer == nullptr) exit(-1);
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

	//init on our side
	audio = new AudioHandler(audiofile, audiofsize, GetSize);
	//Start buffering
	//What are we doing???

	pack.isAudio = true;
	pack.frame = audio->GetBuffer();
	pack.screen = (rectangle*)audio->GetSampleFreq();
	pack.type = audio->GetType();
	pack.rect = (rectangle*)0x4000;
	audio->Processs();//Get audio in buffer
	audiocallback(&pack);

	pack.isAudio = false;//only sending it once rn.
	//Send a faux packet over to init our consumer. 
//Let's do setup



	int sizescr = screen.w * screen.h * 2;//Rgb //hdrz->dwWidth * hdrz->dwHeight * 3;
	CreateFrameBuffer(screen.w, screen.h);

	int numFrames = size / sizeof(_avioldindex_entry);
	frameCount = audiofsize / numFrames;


	cinepak_info* ci = decode_cinepak_init();
	//It's frame time.
	_avioldindex_entry* idxList = (_avioldindex_entry*)buf->GetCurrentBuffer();
	int fps = sthread->dwRate;

	for (int i = 0; i < numFrames; i++)
	{	//so we will point to
		_avioldindex_entry* cur = &idxList[i];

		//Make sure we are a frame.
		if (cur->FourCC != TAG_00DC) continue;

		//sanity stuff.
#ifdef  DEBUG
		unsigned char* frame = moviPointer + cur->dwOffset - 4;
		unsigned int fourcc = *(unsigned long*)frame; frame += 4;

		if (fourcc != cur->FourCC) continue;

		int framesize = *(unsigned long*)frame; frame += 4;
#else
		unsigned char* frame = moviPointer + cur->dwOffset;
		int framesize = *(unsigned long*)frame; frame += 4;
#endif //  DEBUG
		//handle audio

		audio->Processs();
	
		decode_cinepak(ci, frame, cur->dwSize, Kinomet_FrameBuffer, hdrz->dwWidth, hdrz->dwHeight);
		//Hello we have a full framedata 
		pack.frame = Kinomet_FrameBuffer;
		pack.frameid = i;
		pack.screen = &screen;
		pack.rect = &screen;
		callback(&pack);
	}
#ifndef  GBA
	free(Kinomet_FrameBuffer);
#endif
	free_cvinfo(ci);
}

