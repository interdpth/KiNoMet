#include "KiNoMet.h"
#include "../KinoMet/Cinepak.h"
#include "VideoFile.h"
#include <stdio.h>
#include "SDL_windows.h"
#include <SDL.h>
#include <string>
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
Uint64 start;
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

	 renderer = SDL_CreateRenderer(mainWindow, -1,0);
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
	start = SDL_GetPerformanceCounter();
}
Uint32 totalFrameTicks = 0;
Uint32 totalFrames = 0;

void handleFrame(KinometPacket* pack)
{
	//IF this is called nad packet is null, we are just setting up. 
	if (pack->frame == nullptr && !height && !width)
	{
		initFrame(pack);
		return;
	}
	totalFrames++;
	
	SDL_Event event;
	SDL_PollEvent(&event);
		/* handle your event here */
		 //User requests quit
		if (event.type == SDL_QUIT)
			exit(-1);
	


	if (pack->frame == nullptr)
	{
		return;
	}
	int height = pack->rect->h;
	int width = pack->rect->w;
	//we are gba so frame is always 240*160*2;
	char strbuf[512] = { 0 };
	sprintf(strbuf, "F:\\processing\\frame%d.bin", frameHandled++);
	unsigned char* lockedPixels = nullptr;
	int pitch = 0;
	SDL_LockTexture
	(
		texture,
		NULL,
		reinterpret_cast<void**>(&lockedPixels),
		&pitch
	);

	std::memcpy(lockedPixels, pack->frame, width * height *2);
	SDL_UnlockTexture(texture);

	SDL_Rect destination = { 0, 0, width, height };
	SDL_RenderCopy(renderer, texture, NULL, &destination);
	SDL_RenderPresent(renderer);

	//frames++;
	//const Uint64 end = SDL_GetPerformanceCounter();
	//const static Uint64 freq = SDL_GetPerformanceFrequency();
	//const double seconds = (end - start) / static_cast<double>(freq);
	//if (seconds > 2.0)
	//{
	//	std::cout
	//		<< frames << " frames in "
	//		<< std::setprecision(1) << std::fixed << seconds << " seconds = "
	//		<< std::setprecision(1) << std::fixed << frames / seconds << " FPS ("
	//		<< std::setprecision(3) << std::fixed << (seconds * 1000.0) / frames << " ms/frame)"
	//		<< std::endl;
	//	start = end;
	//	frames = 0;
	//}
	FILE* fp = fopen(strbuf, "wb");
	if (pack->frame && fp)
	{
		fwrite(pack->frame, 2, width * height, fp);
		fclose(fp);
	}
	Uint64 end = SDL_GetPerformanceCounter();

	float elapsedMS = ((end - start) / totalFrames)/ (float)SDL_GetPerformanceFrequency() * 1000.0f;
	float valu = elapsedMS > movieFps ?  movieFps : elapsedMS;
	// Cap to 60 FPS
	SDL_Delay(valu);

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
	start = SDL_GetPerformanceCounter();
	LoadAVI((unsigned char*)VideoFile, VideoFile_size, &handleFrame, nullptr);
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
