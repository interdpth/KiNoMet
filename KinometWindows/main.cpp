#include "KiNoMet.h"
#include "../KinoMet/Cinepak.h"
#include "VideoFile.h"
#include "audio_outputmain.h"
#include "audio.h"
#include <stdio.h>
#include "SDL_windows.h"
#include <SDL.h>
#include <string>
// variable declarations
Uint8* audio_pos; // global pointer to the audio buffer to be played
Uint32 audio_len; // remaining length of the sample we have to play

using namespace std;
int frameHandled;
int codeBookSize();
int height = 0;
int width = 0;
//https://www.libsdl.org/release/SDL-1.2.15/docs/html/guidevideoopengl.html
SDL_Window* mainWindow;
SDL_Texture* texture;

long int end;
SDL_Renderer* renderer;
int movieFps;
bool SoundInited;
unsigned int GetTicks(void)
{
	return SDL_GetTicks();
}
void initFrame(KinometPacket* pack)
{
	height = pack->screen->h;
	width = pack->screen->w;
	SDL_Init(SDL_INIT_EVERYTHING);              // Initialize SDL2
	mainWindow = SDL_CreateWindow("Kinometo-Scope",
		SDL_WINDOWPOS_UNDEFINED,           // initial x position
		SDL_WINDOWPOS_UNDEFINED,           // initial y position, 
		width,
		height, SDL_WINDOW_OPENGL);
	if (mainWindow == NULL) {
		// In the case that the window could not be made...
		printf("Could not create window: %s\n", SDL_GetError());
		exit(-1);
	}

	renderer = SDL_CreateRenderer(mainWindow, -1, 0);
	if (!renderer) {
		fprintf(stderr, "Could not create renderer\n");
		exit(-1);
	}
	texture = SDL_CreateTexture
	(
		renderer,
		SDL_PIXELFORMAT_BGR555,
		SDL_TEXTUREACCESS_STREAMING,
		width, height
	);
	movieFps = (int)pack->rect;

	SoundInited = 1;
}
Uint32 totalFrameTicks = 0;
Uint32 totalFrames = 0;
//Audio* mainAudio;
//frame is raw decoded data
//screen rect describes length
int audioType;
bool handleAudio(KinometPacket* pack)
{
	if (pack->frameid == -1)
	{
		InitAudioPlayer((int)pack->screen);
		audioType = (int)pack->type;
		return true;;
	}
	StartPlaying(pack->frame, (int)pack->rect);
	return true;;
}
float last = 0;
extern bool canRender;
bool handleFrame(KinometPacket* pack)
{
	auto start = SDL_GetPerformanceCounter();
	//IF this is called nad packet is null, we are just setting up. 
	if (pack->frame == nullptr && !height && !width)
	{
		initFrame(pack);
		return false;
	}


	SDL_Event event;
	SDL_PollEvent(&event);
	/* handle your event here */
	 //User requests quit
	if (event.type == SDL_QUIT)
	{
		exit(-1);
	}
	Uint32 current = SDL_GetTicks();

	if (pack->frame == nullptr)
	{
		return false;
	}
	
	totalFrames++;
	int height = pack->rect->h;
	int width = pack->rect->w;
	//we are gba so frame is always 240*160*2;
	//char strbuf[512] = { 0 };
	//sprintf(strbuf, "F:\\processing\\frame%d.bin", frameHandled++);
	unsigned char* lockedPixels = nullptr;
	int pitch = 0;
	SDL_LockTexture
	(
		texture,
		NULL,
		reinterpret_cast<void**>(&lockedPixels),
		&pitch
	);

	std::memcpy(lockedPixels, pack->frame, width * height * 2);
	SDL_UnlockTexture(texture);

	SDL_Rect destination = { 0, 0, width, height };
	SDL_RenderCopy(renderer, texture, NULL, &destination);
	SDL_RenderPresent(renderer);
	Uint64 end = SDL_GetPerformanceCounter();

	float elapsedMS = (end - start) /( (float)SDL_GetPerformanceFrequency() * 1000.0f);
	float time = ((1000.0f / movieFps));
	// Cap to 60 FPS
	float dT = (current - last) / 1000.0f;
	float v = (float)floor((float)time - (float)elapsedMS);
	SDL_Delay(abs(v));

	last = current;
	return true;
}
extern unsigned long maxNum;
extern int codeBooks;
#ifdef __cplusplus
extern "C" {
#endif
	int SDL_main(int argc, char* argv[])
	{
		aviLoader l{};
		//this will be on gba, so we're just gonna load the whole thing in and work with pointers.
		frameHandled = 0;

		
		l.audiocallback = &handleAudio;
		l.videoCallBack = &handleFrame;
		l.GetSize = &GetQueuedBytes;
		l.GetTicks = &GetTicks;
		//l.init = &
		
		LoadAVI((unsigned char*)VideoFile, VideoFile_size,(unsigned char*)audio_outputmain, audio_outputmain_size, &l);//  NULL, NULL, &l);
		int overallSize = codeBookSize();
		printf("%d", maxNum);
		printf("%x", overallSize);
		printf("%d", codeBooks);
		SDL_DestroyWindow(mainWindow);

		// Clean up
		SDL_Quit();
		return 0;
	}
#ifdef __cplusplus
}
#endif
