// KiNomet.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "KiNoMet.h"
#include "../KiNomet/Cinepak.h"
#include "AviApi.h"
#include <stdio.h>
#include "AudioManager.h"
#include "MemoryBuffers.h"
#include "Kinomet_Pack.h"
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

	Kinomet_FrameBuffer = MemoryBuffers::Kinomet_FrameBuffer;

	for (int i = 0; i < w * h; i++)//future iterations should size check but black out the screen
	{
		((unsigned short*)Kinomet_FrameBuffer)[i] = 0;
	}
}
//Every FPS we will update audio buffers. 
void SetAudioKinometPacket(KinometPacket* pack)
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

void ProcessAudio(AudioManager* audio, int& readSize, int curFrame, KinometAviControl* options)
{
	if (audio != nullptr)
	{

		//Check if there is a packet playing.
		//Playing is defined as still being copied
		//If there is a track playing(tmp not equal null)
		// is it really playing(tmp is null or 0 bytes left or )

		AudioKinometPacket* tmp = audio->GetCurrPacket();


		if (tmp == nullptr || audio->GetBytesLeft(tmp) == 0)
		{
			tmp = audio->GetNextFrame();
			if (tmp != nullptr)
			{
				audio->Queue(tmp);
			}
		}


		if (tmp != nullptr)
		{
			readSize = audio->ProcessAudio();
			if (readSize)
			{
				AudioKinometPacket kpm(audio->GetType(), audio->GetSampleFreq(), curFrame, readSize, audio->GetBuffer());
				/*	AudioKinometPacket AudioKinometPacket(audio->GetType(), tmp);
				AudioKinometPacket.type = 1;
				AudioKinometPacket.frame = audio->GetBuffer();
				AudioKinometPacket.screen = (rectangle*)audio->GetSampleFreq();
				AudioKinometPacket.type = audio->GetType();
				AudioKinometPacket.rect = (rectangle*)readSize;
				AudioKinometPacket.frameid = curFrame;*/
				options->audiocallback(&kpm);
			}

		}
	}
}


void ProcessVideo(cinepak_info* ci, unsigned char* cineframe, kinoindex_entry* cur, rectangle& screen, int fps, int curFrame, KinometAviControl* options)
{
	int len = 0;
	if (options->Format == KinoFileFormat::Kinopack)
	{
		len = decode_cinepak(ci, cineframe, cur->dwSize, Kinomet_FrameBuffer);
	}
	else 
	{
		len = decode_cinepak(ci, cineframe, cur->dwSize, Kinomet_FrameBuffer);
	}
		VideoKinometPacket videoPacket(screen, fps, curFrame, Kinomet_FrameBuffer, len);

		options->videoCallBack(&videoPacket);
		
		
}


void LoadKino(unsigned char* file,
	int size,
	unsigned char* audiofile,
	int audiofsize,
	KinometAviControl* options)
{
	rectangle screen;
	AudioManager audiomgr;
	AudioManager* audio = nullptr;
	SmallBuffer* buf = new SmallBuffer(file, size);
	//memset(&hdr, 0, sizeof(MainAVIHeader));
	bool bValid = false;
	int pos = 0;

	kinofile* k;
	unsigned char* moviPointer = nullptr;
	k = (kinofile*)buf->GetBuffer();
	//should be K,I,N,O
	unsigned long header = buf->Read32();
	
	if (header != 0x4f4e494b) return;


	unsigned char v = buf->GetByte();;

	
	screen.x = 0;
	screen.y = 0;
	screen.w = buf->GetByte();;
	screen.h = buf->GetByte();
	unsigned char fps = buf->GetByte();
	unsigned long totalFrames = buf->Read32();
	unsigned long hdrCount = buf->Read32();
	VideoKinometPacket videoPack(screen, fps, -1, nullptr, 0);
	options->videoCallBack(&videoPack);
	int readSize = 0;
	//init on our side
	if (audiofile != nullptr)
	{
		//determien

		audiomgr.Init((AudioHeader*)audiofile, audiofsize, fps, totalFrames, options->GetSize);
		audio = &audiomgr;

		AudioKinometPacket akp(audio->GetType(), audio->GetSampleFreq());
		akp.FrameId = -1;


		options->audiocallback(&akp);
	}

	int width = screen.w;
	int height = screen.h;
	int sizescr = width * height * 2;//Rgb //hdrz->dwWidth * hdrz->dwHeight * 3;
	CreateFrameBuffer(width, height);



	cinepak_info* ci = decode_cinepak_init(width, height);
	//It's frame time.
	kinoindex_entry* idxList = (kinoindex_entry*)(buf->GetCurrentBuffer()+4);
	buf->Seek(sizeof(kinoindex_entry) * hdrCount, SEEK_CUR);
	moviPointer = buf->GetCurrentBuffer()+8;
	canRender = true;
	int numFrames = totalFrames;
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

		kinoindex_entry* cur = &idxList[curFrame];

		//Make sure we are a frame.
		if (cur->dwSize == 0) continue;
		unsigned char* cineframe = moviPointer + cur->dwOffset;
		//sanity stuff.
#ifdef  DEBUG
		frame -= 4;
		unsigned int fourcc = *(unsigned long*)frame; frame += 4;

		if (fourcc != cur->FourCC) continue;

		int framesize = *(unsigned long*)frame; frame += 4;
#else

		int framesize = *(unsigned long*)cineframe; cineframe += 4;
#endif //  DEBUG
		//handle audio




		//ProcessAudio(audio, readSize, curFrame, options);


		//
		ProcessVideo(ci, cineframe, cur, screen, fps, curFrame, options);
		curFrame++;
		//Capture timestamp after draw.
		last = options->GetTicks();
	}




	free_cvinfo(ci);
	return;
}



void LoadAVI(unsigned char* file,
	int size,
	unsigned char* audiofile,
	int audiofsize,
	KinometAviControl* options)
{
	rectangle screen;
	AudioManager audiomgr;
	AudioManager* audio = nullptr;
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
	screen.w = (unsigned char)bmpinf->biWidth;
	screen.h = (unsigned char)bmpinf->biHeight;
	VideoKinometPacket videoPack(screen, sthread->dwRate, -1 , nullptr, 0);
	options->videoCallBack(&videoPack);
	int readSize = 0;
	//init on our side
	if (audiofile != nullptr)
	{
		//determien

		audiomgr.Init((AudioHeader*)audiofile, audiofsize, sthread->dwRate, hdrz->dwTotalFrames, options->GetSize);
		audio = &audiomgr;

		AudioKinometPacket akp(audio->GetType(), audio->GetSampleFreq());
		akp.FrameId = -1;


		options->audiocallback(&akp);
	}

	int width = screen.w;
	int height = screen.h;
	int sizescr = width * height * 2;//Rgb //hdrz->dwWidth * hdrz->dwHeight * 3;
	CreateFrameBuffer(width, height);



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
		unsigned char* cineframe = moviPointer + cur->inf.dwOffset;
		//sanity stuff.
#ifdef  DEBUG
		frame -= 4;
		unsigned int fourcc = *(unsigned long*)frame; frame += 4;

		if (fourcc != cur->FourCC) continue;

		int framesize = *(unsigned long*)frame; frame += 4;
#else

		int framesize = *(unsigned long*)cineframe; cineframe += 4;
#endif //  DEBUG
		//handle audio




		ProcessAudio(audio, readSize, curFrame, options);


		//
		ProcessVideo(ci, cineframe, &cur->inf, screen, sthread->dwRate, curFrame, options);
		curFrame++;
		//Capture timestamp after draw.
		last = options->GetTicks();
	}




	free_cvinfo(ci);
	return;
}
