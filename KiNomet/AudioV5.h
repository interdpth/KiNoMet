#pragma once
#include "AudioHandler.h"
#include "Gba.h"
#include "adpcm3_decode.h"

// AudioV5: 3-bit IMA-style ADPCM (sign + 2-bit magnitude), 8-bit PCM domain.
// Sits alongside AudioV4 (4-bit ADPCM, 2:1) rather than replacing it: this
// is a real quality-for-size trade, not a strict upgrade, so both options
// stay available (see AudioManager::Init()'s hdr dispatch -- "AUV5" /
// 0x41555635) and the GUI picks which one to bake.
//
// Compression: 2.5:1 over raw 8-bit PCM (vs AudioV4's 2:1) -- 25% smaller
// audio for the same source than AudioV4, on top of AudioV4's own 2x over
// raw PCM. Cost: ~1.4dB lower SNR than AudioV4, measured on the project's
// own 180s test clip (27.5dB vs AudioV4's 29.0dB) -- audibly a bit noisier,
// not catastrophic, on hardware that's already playing lo-fi mono 8-bit
// audio through a small speaker.
//
// Where the size savings is NOT paid for in decode-side CPU: see
// adpcm3_decode.h's module comment for the actual mechanism (codes packed
// 10-per-32-bit-word so the decoder does 1 word load per 10 samples and 1
// word store per 4 samples, instead of a load+store per sample). The
// per-sample predictor math itself (table lookup, add, clamp) is identical
// in shape to AudioV4's -- the CPU cost per sample is the same ballpark,
// only the memory-traffic side got cheaper.
class AudioV5 :
	public AudioHandler
{
private:
	unsigned char* adpcmData;
	unsigned long adpcmLength;     // bytes (always a whole number of 4-byte words)
	unsigned long adpcmReadPos;    // bytes already consumed from adpcmData (always a multiple of 4)
	int frame;
	int chunkSize;                 // samples/frame = freq/fps, avoids audio/video drift
	adpcm3_state_t state;          // carried continuously across GetNextFrame() calls

	int DecodeMore(signed char* dst, int maxSamples);

public:
	AudioV5(AudioHeader* src, int frames, int (*func)());
	virtual AudioKinometPacket* GetNextFrame();
};
