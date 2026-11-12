#include "AudioV5.h"
#include "MemoryBuffers.h"

// Decode target -- same reasoning as AudioV4.cpp: MemoryBuffers::AudioDecompBuffer
// is in VRAM on GBA, which ignores 8-bit stores, and adpcm3_decode finishes a
// chunk's tail with byte stores. EWRAM (".sbss") takes byte writes.
#ifdef GBA
static signed char adpcm3_out[0x1000] __attribute__((section(".sbss"), aligned(4)));
#else
static signed char adpcm3_out[0x1000];
#endif

AudioV5::AudioV5(AudioHeader* src, int frames, int (*func)()) :
	AudioHandler(src, frames, func)
{
	adpcmData = (unsigned char*)src->data;
	adpcmLength = src->compressedlength;
	adpcmReadPos = 0;
	frame = 0;
	state.valpred = 0;
	state.index = 0;

	int fps = src->fps > 0 ? src->fps : 1;
	chunkSize = (int)(src->freq / fps);
	if (chunkSize < 1) chunkSize = 1;
	// MemoryBuffers::AudioDecompBuffer (shared with AudioV1/V2/V3/V4) is a
	// fixed 0x1000 bytes; clamp so an unusually low fps / high sample rate
	// combination can't overrun it.
	if (chunkSize > 0x1000) chunkSize = 0x1000;
}

// Decodes up to maxSamples mono 8-bit signed PCM samples into dst. Returns
// how many were actually produced; 0 means the stream is exhausted.
//
// adpcmData must be 4-byte aligned at its base, and adpcmReadPos must only
// ever land on a multiple of 4 (both hold here: ROM-baked byte arrays are
// placed on at least a 4-byte boundary by the toolchain, and every chunk
// this consumes advances adpcmReadPos by a whole number of 4-byte words --
// see the loop below -- because RenderAudioV5() on the encoder side
// zero-pads every chunk's own code stream out to a whole word before
// moving on to the next chunk's bytes).
int AudioV5::DecodeMore(signed char* dst, int maxSamples)
{
	unsigned long bytesLeft = adpcmLength - adpcmReadPos;
	if (bytesLeft == 0) return 0;

	unsigned long wordsLeft = bytesLeft / 4;
	unsigned long samplesAvail = wordsLeft * ADPCM3_CODES_PER_WORD;
	unsigned long want = (unsigned long)maxSamples;
	if (want > samplesAvail) want = samplesAvail;
	if (want == 0) return 0;

	adpcm3_decode((const uint32_t*)(adpcmData + adpcmReadPos), (int)want, (int8_t*)dst, &state);
	unsigned long wordsConsumed = (want + ADPCM3_CODES_PER_WORD - 1) / ADPCM3_CODES_PER_WORD;
	adpcmReadPos += wordsConsumed * 4;
	return (int)want;
}

#ifdef GBA
IWRAM
#endif
AudioKinometPacket* AudioV5::GetNextFrame()
{
	signed char* outBuf = adpcm3_out;
	int outLen = DecodeMore(outBuf, chunkSize);

	if (outLen == 0) return nullptr;

	AudioKinometPacket* curPack = new AudioKinometPacket(V5, NULL, frame, outLen, (unsigned char*)outBuf);
	frame++;
	return curPack;
}
