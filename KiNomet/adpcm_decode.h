#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Portable 8-bit-domain, 4-bit, IMA-style ADPCM decoder. Shared verbatim
// between GBA and the Windows/preview build -- the whole decode loop is a
// handful of integer ops (table lookups, adds, shifts), well inside the
// ARM7TDMI's realtime budget, so no platform split and no hand-tuned ARM
// assembly are needed (unlike a real MP3 decoder, which is why AudioV3's
// GBA-side mp3_decode_frame_gba() is still a stub -- see
// mp3_decoder_gba.h). See AudioV4.h for why this codec exists, and
// RenderAudio.RenderAudioV4() (KinoMetGUI/RenderAudio.cs) for the matching
// encoder, which must stay bit-for-bit in lockstep with the arithmetic
// below -- both were derived from, and checked bit-exact against, the same
// Python reference prototype before either was written here.
//
// This is a from-scratch step table and index table sized for the 8-bit
// sample domain directly (1..127, geometric growth), not the standard
// 16-bit-domain IMA-ADPCM table (7..32767) scaled down by 256 -- naively
// dividing that table collapses its first 42 of 89 entries to a single
// step value of 1, throwing away exactly the fine-grained adaptation that
// matters most for quiet passages.

#define ADPCM_N_STEPS 49

typedef struct {
    int8_t valpred;  // last reconstructed sample, carried continuously across calls
    uint8_t index;   // current position in the step table, carried continuously across calls
} adpcm_state_t;

// Decodes `nsamples` signed 8-bit PCM samples from a 4-bit-packed nibble
// stream (2 samples per byte, high nibble decoded first) into dst. Reads
// ceil(nsamples/2) bytes from src. *state is read on entry and updated on
// exit, so back-to-back calls (one per video frame, see
// AudioV4::GetNextFrame()) continue the same adaptive stream correctly.
void adpcm_decode(const uint8_t *src, int nsamples, int8_t *dst, adpcm_state_t *state);

#ifdef __cplusplus
}
#endif
