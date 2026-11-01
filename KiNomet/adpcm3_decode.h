#pragma once
#include <stdint.h>

// AudioV5: 3-bit IMA-style ADPCM (sign + 2-bit magnitude), 8-bit PCM domain.
// See AudioV4.h/adpcm_decode.h for the 4-bit sibling this is modeled on --
// same predictor shape (step-table lookup, adaptive index), just a narrower
// code so each sample costs 3 bits instead of 4: 2.5:1 compression over raw
// 8-bit PCM (vs AudioV4's 2:1), a real file-size win in exchange for more
// quantization noise (measured ~1.4dB lower SNR than AudioV4 on the
// project's own test clip -- see KVC1 session notes/HANDOFF for the
// measurement; not a free upgrade, a deliberate size/quality tradeoff the
// caller can pick instead of AudioV4, not a replacement for it).
//
// Wire format -- the part relevant to "no huge CPU hit": codes are packed
// 10-per-32-bit-little-endian-word, 3 bits each, filling the low 30 bits;
// the top 2 bits of every word are always zero. Every word is fully
// self-contained -- no code ever straddles a word boundary and no bit state
// carries across words -- specifically so the decoder can pull a whole
// group of 10 codes from a single aligned 32-bit load instead of a
// byte-at-a-time bit-reader (cheap on ARM7TDMI: barrel-shifter extracts
// each 3-bit field for free alongside the mask, no per-sample memory
// access). Output is batched the same way on the far side: decoded bytes
// are packed 4-at-a-time into a register and flushed with a single aligned
// 32-bit store, independent of the input's 10-per-word grouping (10 isn't a
// multiple of 4, so reusing that grouping for output stores would drift
// off 4-byte alignment after the first group -- see adpcm3_decode.c's
// comment). Net effect: 1 word load per 10 samples decoded, 1 word store
// per 4 samples written, instead of one load+store per sample.
//
// Alignment contract (ARM7TDMI does NOT fault on a misaligned LDR/STR, it
// silently reads/writes the wrong bytes -- there is no safety net):
//   - `src` must be 4-byte aligned.
//   - `dst` must be 4-byte aligned, and must be 4-byte aligned again on
//     every subsequent call that continues the same stream (i.e. the
//     caller's own per-chunk sample count need not be a multiple of 4, but
//     whatever buffer offset it writes to on each call must be).
//   - The byte offset into the compressed stream that each call starts
//     from must be a multiple of 4. The matching encoder
//     (KinoMetGUI/RenderAudio.cs's AudioV5 path) guarantees this by
//     zero-padding every chunk's code stream out to a whole number of
//     words, so any chunk boundary the encoder actually emitted is a safe
//     place to resume decoding from -- an arbitrary byte offset is not.
#define ADPCM3_N_STEPS 32
#define ADPCM3_CODES_PER_WORD 10

typedef struct {
    int8_t  valpred;
    uint8_t index;
} adpcm3_state_t;

// Decodes `nsamples` 3-bit ADPCM codes starting at `src` into `dst` (signed
// 8-bit PCM), carrying predictor state in `*state` across calls. `nsamples`
// need not be a multiple of ADPCM3_CODES_PER_WORD; a final partial group of
// decoded codes within the last word that aren't requested are computed
// (already in a register, essentially free) but not written out.
void adpcm3_decode(const uint32_t *src, int nsamples, int8_t *dst, adpcm3_state_t *state);
