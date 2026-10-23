/*
 * KVC1-C container loader/player. See kvc1_load.h for the calling contract
 * and KinoMetGUI/kvc1_tools/kvc1_encoder.py's module docstring for the exact
 * file format this parses.
 *
 * Reuses the existing Cinepak-pipeline plumbing that isn't actually about
 * Cinepak: audio pacing (ProcessAudio, defined in KiNomet.cpp) just drains
 * AudioManager on each video frame tick regardless of which video codec is
 * playing, and CreateFrameBuffer/Kinomet_FrameBuffer just set up the shared
 * 240x160 RGB555 output surface -- this calls those exactly as LoadAVI does
 * rather than duplicating them.
 */
#include "KiNoMet.h"
#include "AudioManager.h"
#include "kvc1_decode.h"
#include "kvc1_load.h"

/* Defined in KiNomet.cpp; not exposed via KiNoMet.h because they're
 * Cinepak-pipeline internals, but they're genuinely codec-agnostic. */
extern unsigned char* Kinomet_FrameBuffer;
extern void CreateFrameBuffer(int w, int h);
extern void ProcessAudio(AudioManager* audio, int& readSize, int curFrame, KinometAviControl* options);

static inline unsigned int kvc1_rd_u32(const unsigned char* p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}
static inline unsigned short kvc1_rd_u16(const unsigned char* p) {
    return (unsigned short)(p[0] | (p[1] << 8));
}

/* ---- Fixed decode-time scratch buffers --------------------------------
 * Sized for this project's fixed 240x160 output (60x40 = 2400 4x4 blocks)
 * at the encoder's default gop_size=15:
 *   MODE: exactly 1 byte/block -- gop_size*nblocks = 15*2400 = 36000 is a
 *         hard ceiling (every block in the chunk, worst case), not a
 *         measured-with-margin guess.
 *   FLAT: up to 3 bytes/block (R,G,B) if every block in the chunk were
 *         FLAT -- 15*2400*3 = 108000 is likewise the true worst case.
 *   VQ:   up to 1 byte/block (a codebook index) if every block were VQ --
 *         15*2400 = 36000, same reasoning as MODE.
 *   RAW:  up to 48 bytes/block (16px * R,G,B) if every block were RAW,
 *         which would be 15*2400*48 = 1,728,000 bytes -- not something to
 *         provision for outright (RAW is the encoder's rare last-resort
 *         fallback for blocks that are neither flat nor a good codebook
 *         match). KVC1_RAW_CAP below is a measured-with-margin number from
 *         real encodes instead; if a future encode's content ever
 *         legitimately needs more, the defensive halt in kvc1_load_chunk()
 *         says so loudly instead of silently corrupting memory.
 *         Measured at the encoder's current default thresholds (skip_t=60,
 *         flat_t=800, vq_t=8000 -- loosened from the original 30/300/3000
 *         to hit a 3MB/3min size budget; see kvc1_encoder.py), the real
 *         per-chunk RAW byte count was 0 across every test clip tried,
 *         including a synthetic worst case (pure per-pixel random noise,
 *         which cannot SKIP or FLAT and should defeat VQ too): the looser
 *         vq_t threshold is forgiving enough that VQ absorbs almost
 *         everything RAW used to catch. 32768 therefore has enormous margin
 *         over anything observed, kept as-is (not shrunk) as a hedge against
 *         real-world footage this was never tested on.
 * Total at the values below: 36000 + 108000 + 36000 + KVC1_RAW_CAP.
 * With 256KB (262144 bytes) of real EWRAM and nothing else of significant
 * size statically claiming it in this project (checked: Cinepak's own
 * static footprint is ~1KB, and the ported MP3 decoder is a no-op stub that
 * reserves nothing -- see mp3_decoder_gba_stub.c), this comfortably fits
 * with tens of KB of headroom left for the stack, heap, and everything
 * else. This has NOT been checked against a real devkitARM build's linker
 * map (no ARM cross-compiler is available in the environment this was
 * written in) -- that remains the one way to get an authoritative yes/no
 * on real hardware.
 */
#define KVC1_MODE_CAP 36000u
#define KVC1_FLAT_CAP 108000u
#define KVC1_VQ_CAP   36000u
#define KVC1_RAW_CAP  32768u

// On GBA these must live in EWRAM: plain globals land in .bss, which devkitARM
// links into the 32KB IWRAM, and ~208KB of buffers overflow it. ".sbss" is
// devkitARM's zero-initialised, NOLOAD EWRAM section (libgba's EWRAM_BSS), so
// they cost no ROM space either.
#ifdef GBA
#define KVC1_EWRAM_BSS __attribute__((section(".sbss"), aligned(4)))
#else
#define KVC1_EWRAM_BSS
#endif
static unsigned char kvc1_mode_buf[KVC1_MODE_CAP] KVC1_EWRAM_BSS;
static unsigned char kvc1_flat_buf[KVC1_FLAT_CAP] KVC1_EWRAM_BSS;
static unsigned char kvc1_vq_buf[KVC1_VQ_CAP] KVC1_EWRAM_BSS;
static unsigned char kvc1_raw_buf[KVC1_RAW_CAP] KVC1_EWRAM_BSS;

static inline unsigned short kvc1_pack_at(const unsigned char* rgb) {
    return kvc1_pack555(rgb[0], rgb[1], rgb[2]);
}

void LoadKvc1(unsigned char* file, int size, unsigned char* audiofile, int audiofsize, KinometAviControl* options) {
    (void)size;
    const unsigned char* p = file;
    if (p[0] != 'K' || p[1] != 'V' || p[2] != 'C' || p[3] != '1') {
        while (1) { (char*)"kvc1: bad magic"; }
    }
    p += 4;
    unsigned char version = *p++; (void)version;
    unsigned short width = kvc1_rd_u16(p); p += 2;
    unsigned short height = kvc1_rd_u16(p); p += 2;
    unsigned char fps = *p++;
    unsigned char gop_size = *p++;
    unsigned short n_clusters = kvc1_rd_u16(p); p += 2;
    unsigned int n_frames = kvc1_rd_u32(p); p += 4;
    unsigned int codebook_len = kvc1_rd_u32(p); p += 4;
    const unsigned char* codebook = p; /* read in place -- still in ROM/flash, no RAM copy needed */
    p += codebook_len;
    (void)n_clusters;

    int blocks_w = width / 4;
    int blocks_h = height / 4;
    int nblocks = blocks_w * blocks_h;

    rectangle screen;
    screen.x = 0; screen.y = 0;
    screen.w = (unsigned char)width; screen.h = (unsigned char)height;

    AudioManager audiomgr;
    AudioManager* audio = nullptr;
    if (audiofile != nullptr) {
        audiomgr.Init((AudioHeader*)audiofile, audiofsize, fps, n_frames, options->GetSize);
        audio = &audiomgr;
        AudioKinometPacket akp(audio->GetType(), audio->GetSampleFreq());
        akp.FrameId = -1;
        options->audiocallback(&akp);
    }

    VideoKinometPacket videoPack(screen, fps, -1, nullptr, 0);
    options->videoCallBack(&videoPack);

    CreateFrameBuffer(width, height);
    uint16_t* fb16 = (uint16_t*)Kinomet_FrameBuffer;

    int curFrame = 0;
    int readSize = 0;

    while ((unsigned int)curFrame < n_frames) {
        unsigned int frames_this_chunk = n_frames - curFrame;
        if (frames_this_chunk > gop_size) frames_this_chunk = gop_size;

        unsigned char tag[4];
        unsigned int complen[4];
        unsigned int declen[4];
        for (int s = 0; s < 4; s++) {
            tag[s] = *p++;
            complen[s] = kvc1_rd_u32(p); p += 4;
            declen[s] = kvc1_rd_u32(p); p += 4;
        }

        unsigned int caps[4] = { KVC1_MODE_CAP, KVC1_FLAT_CAP, KVC1_VQ_CAP, KVC1_RAW_CAP };
        unsigned char* bufs[4] = { kvc1_mode_buf, kvc1_flat_buf, kvc1_vq_buf, kvc1_raw_buf };
        for (int s = 0; s < 4; s++) {
            if (declen[s] > caps[s]) {
                /* Same defensive-halt convention as InitCodeBook's OOM guard
                 * in Cinepak.cpp -- loud and stuck beats a silent overrun. */
                while (1) { (char*)"kvc1: decode buffer too small for this stream"; }
            }
            kvc1_decompress_stream(tag[s], p, bufs[s], declen[s]);
            p += complen[s];
        }

        const unsigned char* mode_p = kvc1_mode_buf;
        const unsigned char* flat_p = kvc1_flat_buf;
        const unsigned char* vq_p = kvc1_vq_buf;
        const unsigned char* raw_p = kvc1_raw_buf;

        for (unsigned int f = 0; f < frames_this_chunk; f++) {
            ProcessAudio(audio, readSize, curFrame, options);

            for (int b = 0; b < nblocks; b++) {
                unsigned char mode = *mode_p++;
                switch (mode) {
                case 0: /* SKIP: leave the framebuffer's existing pixels alone */
                    break;
                case 1: { /* FLAT: one colour for all 16 pixels */
                    uint16_t px = kvc1_pack_at(flat_p);
                    flat_p += 3;
                    uint16_t block[16];
                    for (int k = 0; k < 16; k++) block[k] = px;
                    kvc1_write_block(fb16, width, blocks_w, b, block);
                    break;
                }
                case 2: { /* VQ: codebook entry */
                    unsigned char idx = *vq_p++;
                    const unsigned char* entry = codebook + (unsigned int)idx * 48u;
                    uint16_t block[16];
                    for (int k = 0; k < 16; k++) block[k] = kvc1_pack_at(entry + k * 3);
                    kvc1_write_block(fb16, width, blocks_w, b, block);
                    break;
                }
                case 3: { /* RAW: 16 explicit pixels */
                    uint16_t block[16];
                    for (int k = 0; k < 16; k++) block[k] = kvc1_pack_at(raw_p + k * 3);
                    raw_p += 48;
                    kvc1_write_block(fb16, width, blocks_w, b, block);
                    break;
                }
                default:
                    while (1) { (char*)"kvc1: corrupt mode byte"; }
                }
            }

            VideoKinometPacket videoPacket(screen, fps, curFrame, Kinomet_FrameBuffer, (int)(width * height * 2));
            options->videoCallBack(&videoPacket);

            curFrame++;
        }
    }
}
