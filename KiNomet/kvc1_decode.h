#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Decompresses one KVC1-C stream (mode/flat/vq/raw -- see kvc1_encoder.py's
// module docstring for the container format) into dst.
//   tag 0: stored raw -- dst gets a straight copy of declen bytes from src.
//   tag 1: GBA BIOS LZ77UnComp format -- on GBA this is the real hardware
//          BIOS call (via the GBA_LZDECOMP wrapper in cinepak_asm.s, swi
//          0x11); off GBA it's kvc1_lz_decompress()'s own software port.
// src must point at the 4-byte LZ77 header (magic+length) when tag==1, or
// directly at the raw bytes when tag==0 -- the caller (kvc1_load.cpp) reads
// those straight out of the per-stream chunk header and doesn't duplicate
// src's length itself here; declen is trusted (it's self-describing in the
// file, written by the same encoder that wrote the LZ77 header's length
// field, so the two always agree).
void kvc1_decompress_stream(uint8_t tag, const uint8_t *src, uint8_t *dst, uint32_t declen);

// Packs an RGB555 triple (each 0..31) into the GBA's native 15-bit color
// (bit0-4=R, bit5-9=G, bit10-14=B -- same layout as MAKECOLOUR16 in
// KiNomet/cinepak_asm.s, reused here rather than re-deriving it).
static inline uint16_t kvc1_pack555(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)((r & 31u) | ((g & 31u) << 5) | ((b & 31u) << 10));
}

// Writes one 4x4 block of already-packed 16-bit pixels (16 of them, raster
// order within the block) into a 16-bit framebuffer of the given pixel
// width, at block index bidx (row-major over a blocks_w x blocks_h grid).
void kvc1_write_block(uint16_t *framebuffer16, int width_px, int blocks_w,
                       int bidx, const uint16_t pixels[16]);

#ifdef __cplusplus
}
#endif
