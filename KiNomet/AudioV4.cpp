#include "AudioV4.h"
#include "MemoryBuffers.h"

AudioV4::AudioV4(AudioHeader* src, int frames, int (*func)()) :
	AudioHandler(src, frames, func)
{
	adpcmData = (unsigned char*)src->data;
	adpcmLength = src->compressedlength;
	adpcmReadPos = 0;
	frame = 0;
	state.valpred = 0;
	state.index = 0;

	// One video frame's worth of mono 8-bit PCM -- computed from the
	// header's own freq/fps rather than a fixed chunk size (AudioV3 uses a
	// fixed 0x500 bytes regardless of fps), so audio and video can't drift
	// apart if this is ever encoded at something other than the project's
	// usual 15fps.
	int fps = src->fps > 0 ? src->fps : 1;
	chunkSize = (int)(src->freq / fps);
	if (chunkSize < 1) chunkSize = 1;
	// MemoryBuffers::AudioDecompBuffer (shared with AudioV1/V2/V3) is a fixed
	// 0x1000 bytes; clamp so an unusually low fps / high sample rate
	// combination can't overrun it.
	if (chunkSize > 0x1000) chunkSize = 0x1000;
}

// Decodes up to maxSamples mono 8-bit signed PCM samples into dst. Returns
// how many were actually produced; 0 means the stream is exhausted.
int AudioV4::DecodeMore(signed char* dst, int maxSamples)
{
	unsigned long bytesLeft = adpcmLength - adpcmReadPos;
	if (bytesLeft == 0) return 0;

	// Every byte holds 2 samples (high nibble decoded first -- see
	// RenderAudioV4()'s packing) except possibly the very last byte of the
	// whole encoded stream, if the source had an odd total sample count.
	// That only affects the final chunk, since every earlier chunk's
	// sample count is chunkSize = freq/fps, which RenderAudioV4() flushes
	// to a byte boundary after every chunk -- clamp against what's
	// actually left rather than assuming bytesLeft*2 is always available.
	unsigned long samplesAvail = bytesLeft * 2;
	unsigned long want = (unsigned long)maxSamples;
	if (want > samplesAvail) want = samplesAvail;
	if (want == 0) return 0;

	adpcm_decode(adpcmData + adpcmReadPos, (int)want, (int8_t*)dst, &state);
	adpcmReadPos += (want + 1) / 2;
	return (int)want;
}

#ifdef GBA
IWRAM
#endif
AudioKinometPacket* AudioV4::GetNextFrame()
{
	signed char* outBuf = (signed char*)MemoryBuffers::AudioDecompBuffer;
	int outLen = DecodeMore(outBuf, chunkSize);

	if (outLen == 0) return nullptr;

	AudioKinometPacket* curPack = new AudioKinometPacket(V4, NULL, frame, outLen, (unsigned char*)outBuf);
	frame++;
	return curPack;
}
