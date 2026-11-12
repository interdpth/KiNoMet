#include "adpcm3_decode.h"
#include <string.h>  // memcpy -- see the note on the output store below

// 32-entry step table: climbs by 1 up to 16 (full resolution for quiet
// passages, matching the shape of AudioV4's table), then geometric growth
// up to 124 (headroom for loud passages/transients). Chosen empirically
// (grid search over a real encoded test clip, see KVC1 session notes) to
// maximize SNR for this specific 3-bit/sign+2-bit-magnitude code shape --
// it is NOT simply AudioV4's 49-entry table truncated; a fresh search was
// needed because halving the magnitude resolution (4 levels instead of 8)
// changes which step sizes are actually reachable per adaptation step.
static const uint8_t kAdpcm3StepTable[ADPCM3_N_STEPS] = {
      1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
     11, 12, 13, 14, 15, 16, 18, 20, 23, 26,
     30, 34, 39, 44, 50, 57, 65, 74, 84, 96,
    109,124,
};

// Index deltas for the 4 possible 2-bit magnitudes (0..3): small magnitudes
// converge the step size back down (quiet material), the top magnitude
// pushes it up to adapt to a transient. Same role as AudioV4's 8-entry
// kAdpcmIndexTable, just re-tuned for 4 entries instead of 8 -- also
// empirically chosen (grid search), not a simple downsample of the 4-bit
// table's values.
static const int8_t kAdpcm3IndexTable[4] = { -2, -1, 1, 1 };

static int8_t adpcm3_clamp8(int v) {
    if (v > 127) return 127;
    if (v < -128) return -128;
    return (int8_t)v;
}

static uint8_t adpcm3_clamp_index(int v) {
    if (v < 0) return 0;
    if (v > ADPCM3_N_STEPS - 1) return ADPCM3_N_STEPS - 1;
    return (uint8_t)v;
}

// Decodes exactly one 3-bit code against the running predictor state.
// Shared by the batched main loop and the tail loop below so there is only
// one place implementing the actual predictor math.
static int8_t adpcm3_step_one(unsigned code, int *valpred, int *index) {
    int step = kAdpcm3StepTable[*index];
    *index = (int)adpcm3_clamp_index(*index + kAdpcm3IndexTable[code & 3]);

    int mag = code & 3;
    int sign = code & 4;
    int vpdiff = step >> 2;
    if (mag & 2) vpdiff += step;
    if (mag & 1) vpdiff += (step >> 1);

    *valpred = sign ? (*valpred - vpdiff) : (*valpred + vpdiff);
    *valpred = adpcm3_clamp8(*valpred);
    return (int8_t)*valpred;
}

void adpcm3_decode(const uint32_t *src, int nsamples, int8_t *dst, adpcm3_state_t *state) {
    int valpred = state->valpred;
    int index = state->index;
    const uint32_t *wp = src;
    int8_t *dp = dst;

    uint32_t word = 0;
    int codes_left_in_word = 0;   // codes not yet pulled out of `word`

    uint32_t outw = 0;            // up to 4 decoded bytes, packed low-to-high
    int outw_n = 0;                // how many bytes currently sitting in outw

    for (int i = 0; i < nsamples; i++) {
        if (codes_left_in_word == 0) {
            word = *wp++;                      // ONE native 32-bit load feeds the next 10 samples
            codes_left_in_word = ADPCM3_CODES_PER_WORD;
        }
        unsigned code = word & 7;
        word >>= 3;
        codes_left_in_word--;

        uint8_t s = (uint8_t)adpcm3_step_one(code, &valpred, &index);

        outw |= ((uint32_t)s) << (8 * outw_n);
        outw_n++;
        if (outw_n == 4) {
            // ONE native 32-bit store for every 4 samples decoded. Batched
            // independently of the 10-per-word input grouping on purpose:
            // 10 isn't a multiple of 4, so flushing output in the same
            // groups as input would drift the store address off 4-byte
            // alignment after the very first group (offsets 0, 10, 20, 30…
            // are only 4-aligned on every other group) -- silently wrong
            // on real ARM hardware, not just slow. Decoupling to a strict
            // 4-sample cadence keeps every store aligned unconditionally.
            // memcpy, not `*(uint32_t*)dp = outw`: dp is an int8_t* here,
            // so a raw pointer-cast store would be undefined behaviour
            // under C's strict-aliasing rule (storing a wider type through
            // a pointer whose own type is narrower than the memory's
            // effective type) even though the address itself is aligned.
            // A constant-size memcpy like this is the standards-compliant
            // way to do a "just write these 4 bytes as a unit" store, and
            // every real compiler (GCC/Clang, including arm-none-eabi at
            // -O2 and up) turns it into exactly one 32-bit store
            // instruction when it can see dp is 4-aligned and the size is
            // a compile-time constant -- which it is here. No performance
            // is given up for the safety.
            memcpy(dp, &outw, 4);
            dp += 4;
            outw = 0;
            outw_n = 0;
        }
    }

    if (outw_n > 0) {
        // Tail: end of this call's sample count landed mid-group (fewer
        // than 4 decoded bytes pending). No full word to batch, so these
        // are written individually -- this is at most 3 bytes per call,
        // not a per-sample cost.
        for (int k = 0; k < outw_n; k++) {
            dp[k] = (int8_t)((outw >> (8 * k)) & 0xFF);
        }
    }

    state->valpred = (int8_t)valpred;
    state->index = (uint8_t)index;
}
