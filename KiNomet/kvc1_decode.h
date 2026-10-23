#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// KVC1 (format version 3) decoder core, plain C, shared verbatim by the GBA
// and Windows builds. File format: KinoMetGUI/kvc1_tools/kvc1_encoder.py's
// module docstring. Python reference decoder: kvc1_decode_ref.py.
//
// Usage:
//   kvc1_reader_t rd;
//   if (kvc1_reader_init(&rd, file, size, buf, sizeof buf) != KVC1_OK) ...;
//   while (kvc1_reader_next_frame(&rd, framebuffer16) == 1) { show frame }
//
// framebuffer16 is width*height RGB555 pixels and must persist between calls:
// SKIP blocks leave whatever the previous frame put there.
//
// buf receives one decoded chunk at a time. KVC1_CHUNK_CAP is the encoder's
// default --chunk-cap; init fails if the file needs more than bufsize.

#define KVC1_CHUNK_CAP 16384u

#define KVC1_OK            0
#define KVC1_ERR_MAGIC    -1
#define KVC1_ERR_VERSION  -2
#define KVC1_ERR_BUFFER   -3
#define KVC1_ERR_CORRUPT  -4

typedef struct {
    const uint8_t *file;
    uint32_t       size;
    const uint8_t *p;          // next chunk header
    const uint8_t *codebook;   // n_codebook * 16 little-endian u16 pixels
    uint16_t width, height, blocks_w, nblocks;
    uint8_t  fps;
    uint32_t n_frames;
    uint32_t frame;            // frames decoded so far
    uint8_t *buf;
    uint32_t bufsize;
    uint32_t chunk_frames_left;
    const uint8_t *ops, *ops_end, *flat, *btc, *raw, *paint, *pal4, *vq;
} kvc1_reader_t;

// Parses the file header. buf must be 4-byte aligned (the GBA BIOS LZ77 call
// writes words). Returns KVC1_OK or a KVC1_ERR_* code.
int kvc1_reader_init(kvc1_reader_t *rd, const uint8_t *file, uint32_t size,
                     uint8_t *buf, uint32_t bufsize);

// Decodes the next frame into framebuffer16 (width*height u16, row-major).
// Returns 1 if a frame was decoded, 0 after the last frame, or a KVC1_ERR_* code.
int kvc1_reader_next_frame(kvc1_reader_t *rd, uint16_t *framebuffer16);

// Decompresses one stored (tag 0) or BIOS-LZ77 (tag 1) payload into dst.
void kvc1_decompress_stream(uint8_t tag, const uint8_t *src, uint8_t *dst, uint32_t declen);

#ifdef __cplusplus
}
#endif
