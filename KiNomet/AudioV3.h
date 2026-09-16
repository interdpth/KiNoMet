#pragma once
#include "AudioHandler.h"
#include "Gba.h"

// AudioV3 stores a real MP3 file (MPEG-1/2/2.5 Layer III, mono) verbatim in the ROM/EXE
// instead of raw or lightly RLE/LZ-compressed 8-bit PCM (AudioV0/V1/V2). MP3 typically
// gives 8-15x more compression than those schemes for the same audio, which is what
// actually solves "videos are huge due to audio" -- see the ffmpeg call in Kinomet.cs
// and RenderAudio.RenderAudioV3() on the encoder side.
//
// Decoding is platform-specific:
//  - Non-GBA (Windows/preview build): decoded with dr_mp3 (thirdparty/dr_mp3.h), a
//    portable public-domain single-header decoder. Trivial for a desktop CPU to keep up
//    with in realtime.
//  - GBA: a plain C/float decoder is too slow for the ARM7TDMI to keep up with in
//    realtime, which is why real GBA MP3 players use hand-tuned ARM assembly. This class
//    calls out to mp3_decode_frame_gba() (see mp3_decoder_gba.h) for that reason -- see
//    that header for the current status and what's left to wire in.
class AudioV3 :
	public AudioHandler
{
private:
	unsigned char* mp3Data;
	unsigned long mp3Length;
	unsigned long mp3ReadPos;
	int frame;

#ifndef GBA
	void* drmp3Ctx; // drmp3*, kept opaque here so dr_mp3.h only has to be included in AudioV3.cpp.
#endif

	int DecodeMore(signed char* dst, int maxSamples);

public:
	AudioV3(AudioHeader* src, int frames, int (*func)());
	~AudioV3();
	virtual AudioKinometPacket* GetNextFrame();
};
