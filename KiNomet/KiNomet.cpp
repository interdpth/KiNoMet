// KiNomet.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "KiNoMet.h"
#include "../KiNomet/Cinepak.h"
#include "AviApi.h"
#include <stdio.h>
#include "AudioManager.h"
#ifdef GBA
#include <tgmath.h>
#endif
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

#ifdef GBA
	Kinomet_FrameBuffer = (unsigned char*)0x6000000;
#else
	Kinomet_FrameBuffer = (unsigned char*)malloc(sizescr);
#endif
	for (int i = 0; i < sizescr / 4; i++)//future iterations should size check but black out the screen
	{
		((unsigned long*)Kinomet_FrameBuffer)[i] = 0;
	}
}
//Every FPS we will update audio buffers. 
void SetAudioPacket(KinometPacket* pack)
{

}
#ifdef GBA

#endif
bool canRender = false;
#ifdef GBA 
IWRAM
#endif

void QuickCopy(unsigned char* src, unsigned char* dst, int l)
{
	for (int i = 0; i < l; i++) dst[i] = src[i];
}
void LoadAVI(unsigned char* file,
	int size,
	unsigned char* audiofile,
	int audiofsize,
	aviLoader* options)
{
	rectangle screen;
	AudioManager* audio = nullptr;

	SmallBuffer* buf = new SmallBuffer(file, size);
	//memset(&hdr, 0, sizeof(MainAVIHeader));
	bool bValid = false;
	int pos = 0;

	KinometPacket pack;
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

	pack.frame = nullptr;
	pack.frameid = -1;
	pack.rect = (rectangle*)(sthread->dwRate);//packing hack
	pack.screen = &screen;
	int fps = sthread->dwRate;
	int audiofps = fps - 8;
	options->videoCallBack(&pack);
	int readSize = 0;
	//init on our side
	if (audiofile != nullptr)
	{
		//determien

		audio = new AudioManager(audiofile, audiofsize, fps, hdrz->dwTotalFrames, options->GetSize);

		pack.isAudio = true;
		pack.frame = NULL;
		pack.screen = (rectangle*)audio->GetSampleFreq();
		pack.type = audio->GetType();

		pack.rect = (rectangle*)NULL;

		options->audiocallback(&pack);
	}

	//Start buffering
	//What are we doing???


	pack.isAudio = false;//only sending it once rn.
	//Send a faux packet over to init our consumer. 
//Let's do setup


	int width = screen.w;
	int height = screen.h;
	int sizescr = width * height * 2;//Rgb //hdrz->dwWidth * hdrz->dwHeight * 3;
	CreateFrameBuffer(width, height);



	KinometPacket audioPacket;

	QuickCopy((unsigned char*)&pack, (unsigned char*)&audioPacket, sizeof(pack));
	cinepak_info* ci = decode_cinepak_init(width, height);
	//It's frame time.
	_avioldindex_entry* idxList = (_avioldindex_entry*)buf->GetCurrentBuffer();


	canRender = true;
	int numFrames = hdrz->dwTotalFrames;
	frameCount = audiofsize / numFrames;
	unsigned long  last = options->GetTicks();
	unsigned long  current = options->GetTicks();

	int curFrame = 0;
	int lastDrawn = -1;
	//Process AUDIO before processing frames.
	int audioFrames = audiofsize / 0x4000;
	int audioUpdate = 0;
	while (curFrame < numFrames)
	{

		_avioldindex_entry* cur = &idxList[curFrame];

		//Make sure we are a frame.
		if (cur->FourCC != TAG_00DC) continue;
		unsigned char* frame = moviPointer + cur->dwOffset;
		//sanity stuff.
#ifdef  DEBUG
		frame -= 4;
		unsigned int fourcc = *(unsigned long*)frame; frame += 4;

		if (fourcc != cur->FourCC) continue;

		int framesize = *(unsigned long*)frame; frame += 4;
#else

		int framesize = *(unsigned long*)frame; frame += 4;
#endif //  DEBUG
		//handle audio


		pack.frameid = curFrame;



		decode_cinepak(ci, frame, cur->dwSize, Kinomet_FrameBuffer);
		//Hello we have a full framedata 


		pack.frame = Kinomet_FrameBuffer;
		pack.screen = &screen;
		pack.rect = &screen;
		KinometPacket videoPacket;
		//HOW DOES QUEUING WORK
		//V0 ABUSES WHOLE FILE I BELIEVE
		//THANKS MATT
		QuickCopy((unsigned char*)&pack, (unsigned char*)&audioPacket, sizeof(pack));
		QuickCopy((unsigned char*)&pack, (unsigned char*)&videoPacket, sizeof(pack));
		if (audio != nullptr)
		{

			//if (audio->GetType() == V0) 
			//{
			//	//0 doesn't do anything but we're here.
			//}
			//else if (audio->GetType() == V1 || audio->GetType() == V2)
			//{
				//we need to form the packet. 

			//Ask for current frame


			AudioPacket* tmp = audio->GetCurrPacket();
			if (tmp != nullptr && audio->GetBytesLeft(tmp) ==0 || tmp == nullptr) 
			{
			
				AudioPacket* pkt = audio->GetNextFrame();
				if (pkt != nullptr)
				{
					audio->Queue(pkt);
				}
				//}

				readSize = audio->ProcessAudio();
				if (readSize)
				{
					audioPacket.type = 1;
					audioPacket.frame = audio->GetBuffer();
					audioPacket.screen = (rectangle*)audio->GetSampleFreq();
					audioPacket.type = audio->GetType();
					audioPacket.rect = (rectangle*)readSize;

					options->audiocallback(&audioPacket);
				}
			}
		}

		if (options->videoCallBack(&videoPacket))
		{

			curFrame++;

		}



		//The caller should handle framerate.

		//Capture timestamp after draw.
		last = options->GetTicks();

	}

	delete audio;
#ifndef  GBA
	free(Kinomet_FrameBuffer);
#endif
	free_cvinfo(ci);
	return;
}

