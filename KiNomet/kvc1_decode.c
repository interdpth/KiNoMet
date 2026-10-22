#include "kvc1_decode.h"
#include "Gba.h"

#ifdef GBA
// Real hardware BIOS call (KiNomet/cinepak_asm.s: "swi 0x11; bx lr"). Takes
// r0=src, r1=dst; the BIOS itself finds the decompressed length from the
// 4-byte header at src (byte0=0x10, bytes1-3=24-bit LE length) and writes
// exactly that many bytes to dst. No meaningful return value (same
// convention as cinepak's existing use of this symbol). Signature matches
// MirrorIO/CinePakio.h's existing extern "C" declaration of this exact
// symbol exactly (int, non-const void*) -- this is a plain .c file so it
// can't include that C++ header, but the declaration must still agree.
extern int GBA_LZDECOMP(void *src, void *dst);
#else
// Software port of the real BIOS LZ77UnComp algorithm for the non-GBA
// (Windows/preview) build. NOTE: this is deliberately a fresh, independent
// implementation rather than a reuse of Compression.cpp's existing
// GBA_LZDECOMP() -- that function decodes into an internal scratch buffer
// and never actually copies the result into its own `dst` parameter (a
// real, pre-existing bug found while building this decoder; see the
// comment on Compression::LZDecomp for what it means for AudioV1/V2's LZ
// option on the Windows build). This was verified against the project's
// own GBA_LZDECOMP's *read* side (header/flag/back-reference layout is
// identical, confirmed by cross-decoding the same compressed blobs through
// both), it just actually writes its output.
static void kvc1_lz_decompress(const uint8_t *src, uint8_t *dst) {
    uint32_t header = (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
    uint32_t declen = header >> 8;
    const uint8_t *sp = src + 4;
    uint32_t written = 0;
    while (written < declen) {
        uint8_t flags = *sp++;
        for (int i = 0; i < 8 && written < declen; i++) {
            int is_ref = flags & (0x80 >> i);
            if (!is_ref) {
                dst[written] = *sp++;
                written++;
            } else {
                uint8_t b0 = sp[0], b1 = sp[1];
                sp += 2;
                int length = (b0 >> 4) + 3;
                int disp = ((b0 & 0xF) << 8 | b1) + 1;
                for (int j = 0; j < length; j++) {
                    dst[written] = dst[written - disp];
                    written++;
                }
            }
        }
    }
}
#endif

void kvc1_decompress_stream(uint8_t tag, const uint8_t *src, uint8_t *dst, uint32_t declen) {
    if (declen == 0) return;
    if (tag == 0) {
        for (uint32_t i = 0; i < declen; i++) dst[i] = src[i];
        return;
    }
#ifdef GBA
    GBA_LZDECOMP((void *)src, (void *)dst);
#else
    kvc1_lz_decompress(src, dst);
#endif
}

void kvc1_write_block(uint16_t *framebuffer16, int width_px, int blocks_w,
                       int bidx, const uint16_t pixels[16]) {
    int bx = bidx % blocks_w;
    int by = bidx / blocks_w;
    int px0 = bx * 4;
    int py0 = by * 4;
    for (int row = 0; row < 4; row++) {
        uint16_t *dst = framebuffer16 + (py0 + row) * width_px + px0;
        const uint16_t *src = pixels + row * 4;
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
    }
}
