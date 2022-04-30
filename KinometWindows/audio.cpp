#include "audio.h"
#include <SDL_audio.h>
#include <SDL.h>
SDL_AudioSpec os;//What we get 
SDL_AudioSpec desiredSpec;//What we wawnt
SDL_AudioDeviceID deviceid;
int inited = false;


SDL_AudioStream* stream;
void stopPlaying()
{
	SDL_CloseAudio();
}

void StartPlaying(unsigned char* buff, int len)
{
	
	int err = SDL_AudioStreamPut(stream, buff, len); //SDL_QueueAudio(deviceid, buff, len);

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
static void Sound_Callback(void* userdata, Uint8* buffer, int len)
{
 
        int available = SDL_AudioStreamAvailable(stream);

        if (available < len)
        {
            memset(buffer, 0, len);
        }
        else
        {
            int obtained = SDL_AudioStreamGet(stream, buffer, len);
            if (obtained == -1)
            {
            /*    Debug_LogMsgArg("Failed to get converted data: %s",
                    SDL_GetError());*/
            }
            else
            {
                //if (obtained != len)
                //{
                //    //Debug_LogMsgArg("%s: Obtained = %d, Requested = %d",
                //    //    __func__, obtained, len);
                //    //// Clear the rest of the buffer
                //    memset(&(buffer[obtained]), 0, len - obtained);
                //}
            }
        }

        //Debug_LogMsgArg("Available: %d/%d",
        //                SDL_AudioStreamAvailable(stream),
        //                obtained_spec.samples);
    
}
bool inited2 = false;
int InitAudioPlayer(int sampleSize)
{
	if (inited2) return 0;
	SDL_zero(desiredSpec);
	desiredSpec.freq = sampleSize;
	desiredSpec.format = AUDIO_S8;
	desiredSpec.channels = 1;
	desiredSpec.samples = 1;
	desiredSpec.callback = Sound_Callback;

	desiredSpec.userdata = NULL;;

	deviceid = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &os, NULL);
	if (deviceid < 0)
	{

		return 1;
	}
	
	stream = SDL_NewAudioStream(AUDIO_S8, 1, sampleSize,
		os.format, os.channels,
		os.freq);
	if (stream == NULL) {

		return 0;
	}
	SDL_PauseAudioDevice(deviceid, 0);//Start playing right away.
	inited2 = 1;
	return 0;
}
