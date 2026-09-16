#include "AudioV3.h"
#include "MemoryBuffers.h"
#include <cstdlib>

#ifndef GBA
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "thirdparty/dr_mp3.h"
#else
#include "mp3_decoder_gba.h"
#endif

// How many decoded PCM bytes GetNextFrame() hands back per call. Matches the 0x500 chunk
// size AudioV0 already streams in, so AudioV3 packets flow through AudioHandler/AudioManager
// (ring buffer sizing, per-frame cadence) exactly the same way the existing formats do.
#define MP3_OUT_CHUNK 0x500

AudioV3::AudioV3(AudioHeader* src, int frames, int (*func)()) :
	AudioHandler(src, frames, func)
{
	mp3Data = (unsigned char*)src->data;
	mp3Length = src->compressedlength;
	mp3ReadPos = 0;
	frame = 0;

#ifndef GBA
	drmp3Ctx = malloc(sizeof(drmp3));
	if (drmp3Ctx != nullptr && !drmp3_init_memory((drmp3*)drmp3Ctx, mp3Data, mp3Length, NULL))
	{
		free(drmp3Ctx);
		drmp3Ctx = nullptr;
	}
#endif
}

AudioV3::~AudioV3()
{
#ifndef GBA
	if (drmp3Ctx != nullptr)
	{
		drmp3_uninit((drmp3*)drmp3Ctx);
		free(drmp3Ctx);
		drmp3Ctx = nullptr;
	}
#endif
}

// Decodes up to maxSamples mono 8-bit signed PCM samples into dst. Returns how many were
// actually produced; 0 means the stream is exhausted.
int AudioV3::DecodeMore(signed char* dst, int maxSamples)
{
#ifndef GBA
	if (drmp3Ctx == nullptr) return 0;
	drmp3* mp3 = (drmp3*)drmp3Ctx;

	drmp3_int16 tmp[1152]; // one MPEG audio frame's worth of mono samples, max.
	int produced = 0;
	while (produced < maxSamples)
	{
		int want = maxSamples - produced;
		if (want > 1152) want = 1152;

		drmp3_uint64 got = drmp3_read_pcm_frames_s16(mp3, (drmp3_uint64)want, tmp);
		if (got == 0) break; // end of stream

		for (drmp3_uint64 i = 0; i < got; i++)
		{
			// 16-bit signed -> 8-bit signed: matches the format the rest of the
			// pipeline expects (ring buffer -> GBA Direct Sound DMA / SDL AUDIO_S8).
			dst[produced + i] = (signed char)(tmp[i] >> 8);
		}
		produced += (int)got;
	}
	return produced;
#else
	return mp3_decode_frame_gba(mp3Data, mp3Length, &mp3ReadPos, dst, maxSamples);
#endif
}

#ifdef GBA
IWRAM
#endif
AudioKinometPacket* AudioV3::GetNextFrame()
{
	signed char* outBuf = (signed char*)MemoryBuffers::AudioDecompBuffer;
	int outLen = DecodeMore(outBuf, MP3_OUT_CHUNK);

	if (outLen == 0) return nullptr;

	AudioKinometPacket* curPack = new AudioKinometPacket(V3, NULL, frame, outLen, (unsigned char*)outBuf);
	frame++;
	return curPack;
}
