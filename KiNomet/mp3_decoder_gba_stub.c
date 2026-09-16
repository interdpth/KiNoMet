#include "mp3_decoder_gba.h"

// See mp3_decoder_gba.h -- this is the integration point for Nocash's GBA MP3 decoder
// (MP3.A22). Until that's ported/linked in, AudioV3 (MP3 audio) is silent on GBA; the
// ffmpeg pipeline, container format, and Windows/preview playback all work already.
int mp3_decode_frame_gba(const unsigned char* src, unsigned long srcLen, unsigned long* srcPos, signed char* dst, int maxSamples)
{
	(void)src;
	(void)srcLen;
	(void)srcPos;
	(void)dst;
	(void)maxSamples;
	return 0;
}
