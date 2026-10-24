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

/* One decoded chunk at a time (the encoder caps chunks at KVC1_CHUNK_CAP
 * decoded bytes). On GBA it lives in EWRAM: plain globals go to .bss in the
 * 32KB IWRAM. ".sbss" is devkitARM's zero-initialised NOLOAD EWRAM section
 * (libgba's EWRAM_BSS), so it costs no ROM either. */
#ifdef GBA
#define KVC1_EWRAM_BSS __attribute__((section(".sbss"), aligned(4)))
#else
#define KVC1_EWRAM_BSS
#endif
static unsigned char kvc1_chunk_buf[KVC1_CHUNK_CAP] KVC1_EWRAM_BSS __attribute__((aligned(4)));

/* Stops playback for good. On GBA it first paints the top 16 rows of the
 * screen a solid colour so the reason is visible without a debugger:
 *   red     = VideoFile.bin is the wrong format version (re-encode it)
 *   yellow  = a chunk is bigger than KVC1_CHUNK_CAP (--chunk-cap mismatch)
 *   magenta = corrupt video data
 *   blue    = not a KVC1 file at all */
static void kvc1_halt(const char* why, unsigned short colour) {
    (void)why;
#ifdef GBA
    *(volatile unsigned short*)0x04000000 = 0x0403;           /* mode 3, BG2 on */
    volatile unsigned short* vram = (volatile unsigned short*)0x06000000;
    for (int i = 0; i < 240 * 16; i++) vram[i] = colour;
#else
    (void)colour;
#endif
    while (1) { }
}

void LoadKvc1(unsigned char* file, int size, unsigned char* audiofile, int audiofsize, KinometAviControl* options) {
    kvc1_reader_t rd;
    int err = kvc1_reader_init(&rd, file, (uint32_t)size, kvc1_chunk_buf, sizeof(kvc1_chunk_buf));
    if (err == KVC1_ERR_BUFFER) kvc1_halt("kvc1: chunk larger than KVC1_CHUNK_CAP -- re-encode with a matching --chunk-cap", 0x03FF);
    if (err == KVC1_ERR_VERSION) kvc1_halt("kvc1: wrong KVC1 format version -- re-encode with the current kvc1_encoder.py", 0x001F);
    if (err != KVC1_OK) kvc1_halt("kvc1: VideoFile.bin is not a KVC1 file", 0x7C00);

    rectangle screen;
    screen.x = 0; screen.y = 0;
    screen.w = (unsigned char)rd.width; screen.h = (unsigned char)rd.height;

    AudioManager audiomgr;
    AudioManager* audio = nullptr;
    if (audiofile != nullptr) {
        audiomgr.Init((AudioHeader*)audiofile, audiofsize, rd.fps, rd.n_frames, options->GetSize);
        audio = &audiomgr;
        AudioKinometPacket akp(audio->GetType(), audio->GetSampleFreq());
        akp.FrameId = -1;
        options->audiocallback(&akp);
    }

    VideoKinometPacket videoPack(screen, rd.fps, -1, nullptr, 0);
    options->videoCallBack(&videoPack);

    CreateFrameBuffer(rd.width, rd.height);
    uint16_t* fb16 = (uint16_t*)Kinomet_FrameBuffer;
#ifdef GBA
    /* 240x160 plays in mode 3 (stride 240). Anything smaller plays in mode 5,
     * whose framebuffer is 160 pixels wide; handleFrame's Setup() scales it
     * up to the full screen with the BG2 affine registers. */
    int stride = (rd.width == 240) ? 240 : 160;
    int rows = (rd.width == 240) ? 160 : 128;
#else
    int stride = rd.width;
    int rows = rd.height;
#endif
    /* SKIP blocks keep what's already there, so start from black. */
    for (int i = 0; i < stride * rows; i++) fb16[i] = 0;

    int readSize = 0;
    int curFrame = 0;
    while (rd.frame < rd.n_frames) {
        ProcessAudio(audio, readSize, curFrame, options);
        if (kvc1_reader_next_frame(&rd, fb16, stride) != 1) kvc1_halt("kvc1: corrupt video data", 0x7C1F);

        VideoKinometPacket videoPacket(screen, rd.fps, curFrame, Kinomet_FrameBuffer, (int)(stride * rows * 2));
        options->videoCallBack(&videoPacket);
        curFrame++;
    }
}
