#include "AudioV0.h"

AudioV0::AudioV0(unsigned char* src, int len, int fps, int frames, int rsize, int (*func)()):
    AudioHandler(src, len, fps, frames,  rsize, func)
{

}
