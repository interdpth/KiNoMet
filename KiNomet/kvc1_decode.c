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

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
/* Grey level 0..31 -> RGB555 with R=G=B. */
#define KVC1_GREY(g) ((uint16_t)((g) * 0x421u))
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int kvc1_reader_init(kvc1_reader_t *rd, const uint8_t *file, uint32_t size,
                     uint8_t *buf, uint32_t bufsize) {
    if (size < 24 || file[0] != 'K' || file[1] != 'V' || file[2] != 'C' || file[3] != '1')
        return KVC1_ERR_MAGIC;
    if (file[4] != 3) return KVC1_ERR_VERSION;
    rd->file = file;
    rd->size = size;
    rd->fps = file[5];
    rd->width = rd16(file + 6);
    rd->height = rd16(file + 8);
    uint16_t n_cb = rd16(file + 10);
    rd->n_frames = rd32(file + 12);
    uint32_t max_declen = rd32(file + 16);
    rd->gray = (uint8_t)(rd32(file + 20) & 1u);
    if (max_declen > bufsize) return KVC1_ERR_BUFFER;
    rd->codebook = file + 24;
    rd->p = file + 24 + (uint32_t)n_cb * (rd->gray ? 16u : 32u);
    rd->blocks_w = rd->width / 4;
    rd->nblocks = (uint16_t)(rd->blocks_w * (rd->height / 4));
    rd->frame = 0;
    rd->buf = buf;
    rd->bufsize = bufsize;
    rd->chunk_frames_left = 0;
    return KVC1_OK;
}

static int load_chunk(kvc1_reader_t *rd) {
    const uint8_t *h = rd->p;
    if ((uint32_t)(h - rd->file) + 12 > rd->size) return KVC1_ERR_CORRUPT;
    uint8_t tag = h[0];
    uint8_t nfr = h[1];
    uint32_t clen = rd32(h + 4);
    uint32_t dlen = rd32(h + 8);
    if (nfr == 0 || dlen < 12 || dlen > rd->bufsize) return KVC1_ERR_CORRUPT;
    kvc1_decompress_stream(tag, h + 12, rd->buf, dlen);
    rd->p = h + 12 + ((clen + 3u) & ~3u);

    const uint8_t *b = rd->buf;
    uint16_t ops_len = rd16(b), n_flat = rd16(b + 2), n_btc = rd16(b + 4), n_raw = rd16(b + 6);
    uint16_t n_paint = rd16(b + 8), n_pal4 = rd16(b + 10);
    rd->ops = b + 12;
    rd->ops_end = rd->ops + ops_len;
    /* Record sizes: colour / greyscale. */
    const uint32_t g = rd->gray;
    rd->flat = rd->ops_end;
    rd->btc = rd->flat + (g ? 1u : 2u) * n_flat;
    rd->raw = rd->btc + (g ? 4u : 6u) * n_btc;
    rd->paint = rd->raw + (g ? 16u : 32u) * n_raw;
    rd->pal4 = rd->paint + (g ? 3u : 4u) * n_paint;
    rd->vq = rd->pal4 + (g ? 8u : 12u) * n_pal4;
    if (rd->vq > b + dlen) return KVC1_ERR_CORRUPT;
    rd->chunk_frames_left = nfr;
    return KVC1_OK;
}

static void fill_block(uint16_t *fb, int stride, uint16_t c) {
    for (int r = 0; r < 4; r++, fb += stride) { fb[0] = c; fb[1] = c; fb[2] = c; fb[3] = c; }
}

int kvc1_reader_next_frame(kvc1_reader_t *rd, uint16_t *framebuffer16, int stride) {
    if (rd->frame >= rd->n_frames) return 0;
    if (rd->chunk_frames_left == 0) {
        int e = load_chunk(rd);
        if (e != KVC1_OK) return e;
    }
    const int bw = rd->blocks_w;
    const int nb = rd->nblocks;
    const uint8_t *ops = rd->ops, *flat = rd->flat, *btc = rd->btc, *raw = rd->raw, *vq = rd->vq;
    const uint8_t *paint = rd->paint, *pal4 = rd->pal4;
    const int gray = rd->gray;
    int b = 0;
    while (b < nb) {
        if (ops >= rd->ops_end) return KVC1_ERR_CORRUPT;
        uint8_t op = *ops++;
        int n = (op & 31) + 1;
        int kind = op >> 5;
        if (b + n > nb) return KVC1_ERR_CORRUPT;
        if (kind == 0) { b += n; continue; } /* SKIP: keep what's on screen */
        if (kind == 7) return KVC1_ERR_CORRUPT;
        for (; n > 0; n--, b++) {
            uint16_t *dst = framebuffer16 + (b / bw) * 4 * stride + (b % bw) * 4;
            switch (kind) {
            case 1: /* FLAT */
                if (gray) { fill_block(dst, stride, KVC1_GREY(*flat)); flat += 1; }
                else { fill_block(dst, stride, rd16(flat)); flat += 2; }
                break;
            case 2: { /* VQ */
                if (gray) {
                    const uint8_t *e = rd->codebook + (uint32_t)(*vq++) * 16u;
                    for (int r = 0; r < 4; r++, dst += stride, e += 4) {
                        dst[0] = KVC1_GREY(e[0]); dst[1] = KVC1_GREY(e[1]);
                        dst[2] = KVC1_GREY(e[2]); dst[3] = KVC1_GREY(e[3]);
                    }
                    break;
                }
                const uint8_t *e = rd->codebook + (uint32_t)(*vq++) * 32u;
                for (int r = 0; r < 4; r++, dst += stride, e += 8) {
                    dst[0] = rd16(e); dst[1] = rd16(e + 2); dst[2] = rd16(e + 4); dst[3] = rd16(e + 6);
                }
                break;
            }
            case 3: { /* BTC */
                uint16_t c0, c1, m;
                if (gray) { c0 = KVC1_GREY(btc[0]); c1 = KVC1_GREY(btc[1]); m = rd16(btc + 2); btc += 4; }
                else { c0 = rd16(btc); c1 = rd16(btc + 2); m = rd16(btc + 4); btc += 6; }
                for (int r = 0; r < 4; r++, dst += stride, m >>= 4) {
                    dst[0] = (m & 1) ? c1 : c0;
                    dst[1] = (m & 2) ? c1 : c0;
                    dst[2] = (m & 4) ? c1 : c0;
                    dst[3] = (m & 8) ? c1 : c0;
                }
                break;
            }
            case 4: /* RAW */
                if (gray) {
                    for (int r = 0; r < 4; r++, dst += stride, raw += 4) {
                        dst[0] = KVC1_GREY(raw[0]); dst[1] = KVC1_GREY(raw[1]);
                        dst[2] = KVC1_GREY(raw[2]); dst[3] = KVC1_GREY(raw[3]);
                    }
                    break;
                }
                for (int r = 0; r < 4; r++, dst += stride, raw += 8) {
                    dst[0] = rd16(raw); dst[1] = rd16(raw + 2); dst[2] = rd16(raw + 4); dst[3] = rd16(raw + 6);
                }
                break;
            case 5: { /* PAINT: one colour over what's on screen */
                uint16_t c, m;
                if (gray) { c = KVC1_GREY(paint[0]); m = rd16(paint + 1); paint += 3; }
                else { c = rd16(paint); m = rd16(paint + 2); paint += 4; }
                for (int r = 0; r < 4; r++, dst += stride, m >>= 4) {
                    if (m & 1) dst[0] = c;
                    if (m & 2) dst[1] = c;
                    if (m & 4) dst[2] = c;
                    if (m & 8) dst[3] = c;
                }
                break;
            }
            default: { /* PAL4: 4 colours, 2-bit index per pixel */
                uint16_t pal[4];
                uint32_t idx;
                if (gray) {
                    pal[0] = KVC1_GREY(pal4[0]); pal[1] = KVC1_GREY(pal4[1]);
                    pal[2] = KVC1_GREY(pal4[2]); pal[3] = KVC1_GREY(pal4[3]);
                    idx = rd32(pal4 + 4);
                    pal4 += 8;
                } else {
                    pal[0] = rd16(pal4); pal[1] = rd16(pal4 + 2); pal[2] = rd16(pal4 + 4); pal[3] = rd16(pal4 + 6);
                    idx = rd32(pal4 + 8);
                    pal4 += 12;
                }
                for (int r = 0; r < 4; r++, dst += stride, idx >>= 8) {
                    dst[0] = pal[idx & 3];
                    dst[1] = pal[(idx >> 2) & 3];
                    dst[2] = pal[(idx >> 4) & 3];
                    dst[3] = pal[(idx >> 6) & 3];
                }
                break;
            }
            }
        }
    }
    if (b != nb) return KVC1_ERR_CORRUPT;
    rd->ops = ops; rd->flat = flat; rd->btc = btc; rd->raw = raw; rd->vq = vq;
    rd->paint = paint; rd->pal4 = pal4;
    rd->chunk_frames_left--;
    rd->frame++;
    return 1;
}
