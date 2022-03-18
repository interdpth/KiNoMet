#include "audio.h"
#include <SDL_audio.h>
#include <SDL.h>

SDL_AudioSpec desiredSpec;
SDL_AudioDeviceID deviceid;
int inited = false;

SDL_AudioSpec os;

void stopPlaying()
{
	SDL_CloseAudio();
}

void StartPlaying(unsigned char* buff, int len)
{

	int err = SDL_QueueAudio(deviceid, buff, len);
	auto b = SDL_GetError();
	if (err > 0)
	{
		exit(-1);
	}
}

int GetQueuedBytes()
{
	return SDL_GetQueuedAudioSize(deviceid);
}

int InitAudioPlayer(int sampleSize)
{
	SDL_zero(desiredSpec);
	desiredSpec.freq = sampleSize;
	desiredSpec.format = AUDIO_S8;
	desiredSpec.channels = 1;
	desiredSpec.samples =4096;
	desiredSpec.callback = NULL;

	desiredSpec.userdata = NULL;;
	
	deviceid = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &os, NULL);
	if (deviceid < 0)
	{

		return 1;
	}

	SDL_PauseAudioDevice(deviceid, 0);
	return 0;
}
