#pragma once
#include "AudioHandler.h"
#include "Gba.h"
#include "adpcm_decode.h"

// AudioV4: 8-bit-domain, 4-bit, IMA-style ADPCM (see adpcm_decode.h/.c for
// the decoder and RenderAudio.RenderAudioV4(), KinoMetGUI/RenderAudio.cs,
// for the matching encoder).
//
// Why this exists alongside AudioV0-V2 (raw/RLE/LZ 8-bit PCM) and AudioV3
// (MP3 container + dr_mp3 on Windows): AudioV3's real GBA decoder
// (mp3_decode_frame_gba(), mp3_decoder_gba.h) is currently a stub that
// always returns 0 -- real MP3 decoding on GBA needs a hand-tuned ARM
// decode routine that hasn't been ported yet, so an MP3 track is silent on
// real hardware today even though the container format, encoder pipeline
// and Windows playback all work. AudioV4 is a real, working, lightweight
// codec for the GBA target in the meantime: no EWRAM table reservation (its
// whole footprint is a 57-byte ROM table plus 2 bytes of per-stream state),
// no hand-tuned ARM assembly (the decode loop is table lookups, adds and
// shifts, comfortably inside the ARM7TDMI's realtime budget), and it
// actually produces audible output on GBA right now.
//
// Tradeoff: 2:1 compression over raw 8-bit PCM (half of AudioV1/V2's
// best-case, and far short of what a working MP3 decoder would give) --
// see RenderAudioV4()'s logged compression ratio. That's a real quality
// cost, accepted because it's the option that actually plays sound on
// hardware without costing meaningful RAM, which leaves the video codec
// (KVC1-C) free to use the EWRAM budget that would otherwise be spent on
// audio decode tables.
class AudioV4 :
	public AudioHandler
{
private:
	unsigned char* adpcmData;
	unsigned long adpcmLength;
	unsigned long adpcmReadPos;   // bytes already consumed from adpcmData
	int frame;
	int chunkSize;                // samples/frame = freq/fps, avoids audio/video drift
	adpcm_state_t state;           // carried continuously across GetNextFrame() calls

	int DecodeMore(signed char* dst, int maxSamples);

public:
	AudioV4(AudioHeader* src, int frames, int (*func)());
	virtual AudioKinometPacket* GetNextFrame();
};
