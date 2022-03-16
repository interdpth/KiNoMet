#include "audio.h"
#include <SDL_audio.h>


void stopPlaying()
{
    SDL_CloseAudio();
}
SDL_AudioSpec desiredSpec;
SDL_AudioDeviceID deviceid;
void StartPlaying(unsigned char* buff, int len)
{

    int err = SDL_QueueAudio(deviceid, buff, len);
    auto b = SDL_GetError();
    if (err > 0)
    {
        exit(-1);
    }
     SDL_PauseAudioDevice(deviceid, 0);
    b = SDL_GetError();
    if (err > 0)
    {
        exit(-1);
    }
}
static SDL_AudioCallback myAudioCallback(void* userdata, Uint8* stream, int Glen)
{

    return NULL;

    //if (audio->length == 0)
    //    return;

    //Uint32 length = (Uint32)len;
    //length = (length > audio->length ? audio->length : length); // if length is more than the audio length, then set length to be the audio.length, if not, set it to be the length passed to the function

    //SDL_memcpy(stream, audio->position, length);

    //audio->position += length;
    //audio->length -= length;
}
int InitAudioPlayer(int sampleSize)
{
    SDL_zero(desiredSpec);
    desiredSpec.freq = sampleSize;
    desiredSpec.format = AUDIO_S8;
    desiredSpec.channels = 1;
    desiredSpec.samples = 1;
    desiredSpec.callback = NULL;
   
    desiredSpec.userdata = NULL;;
    SDL_AudioSpec os;
    deviceid = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &os, NULL);
    if (deviceid < 0)
    {

        return 1;
    }

   
    return 0;
}
