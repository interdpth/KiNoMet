#include "adpcm_decode.h"

// 49-entry step-size table for the 8-bit-domain ADPCM variant (see
// adpcm_decode.h's module comment). Generated as round(ratio^i) for i in
// 0..48, ratio chosen so the table spans 1..127 (the full magnitude range
// of a signed 8-bit sample), with each entry bumped to at least
// (previous + 1) so no two consecutive entries collapse to the same value.
// Values 0..35 climb by exactly 1 per step (full resolution for quiet
// passages); from there growth is geometric up to 127 (headroom for loud
// passages/transients).
static const uint8_t kAdpcmStepTable[ADPCM_N_STEPS] = {
      1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
     11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
     21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
     31, 32, 33, 34, 35, 36, 38, 42, 46, 51,
     57, 63, 69, 77, 85, 94,104,115,127,
};

// Same shape as the standard IMA-ADPCM index table: the low 3 bits of a
// decoded nibble select how far/fast the step-table index moves. Small
// magnitudes (0-3) nudge the index down by 1 (converge toward finer
// steps); large magnitudes (4-7) push it up fast (adapt quickly to a loud
// transient).
static const int8_t kAdpcmIndexTable[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static int8_t adpcm_clamp8(int v) {
    if (v > 127) return 127;
    if (v < -128) return -128;
    return (int8_t)v;
}

static uint8_t adpcm_clamp_index(int v) {
    if (v < 0) return 0;
    if (v > ADPCM_N_STEPS - 1) return ADPCM_N_STEPS - 1;
    return (uint8_t)v;
}

void adpcm_decode(const uint8_t *src, int nsamples, int8_t *dst, adpcm_state_t *state) {
    int valpred = state->valpred;
    int index = state->index;
    int step = kAdpcmStepTable[index];
    int hi_pending = 0;
    uint8_t cur_byte = 0;
    const uint8_t *sp = src;

    for (int i = 0; i < nsamples; i++) {
        int delta;
        if (!hi_pending) {
            cur_byte = *sp++;
            delta = (cur_byte >> 4) & 0xF;
            hi_pending = 1;
        } else {
            delta = cur_byte & 0xF;
            hi_pending = 0;
        }

        index = (int)adpcm_clamp_index(index + kAdpcmIndexTable[delta & 7]);

        int sign = delta & 8;
        int mag = delta & 7;
        int vpdiff = step >> 3;
        if (mag & 4) vpdiff += step;
        if (mag & 2) vpdiff += (step >> 1);
        if (mag & 1) vpdiff += (step >> 2);

        if (sign) valpred -= vpdiff;
        else valpred += vpdiff;
        valpred = adpcm_clamp8(valpred);

        step = kAdpcmStepTable[index];
        dst[i] = (int8_t)valpred;
    }

    state->valpred = (int8_t)valpred;
    state->index = (uint8_t)index;
}
