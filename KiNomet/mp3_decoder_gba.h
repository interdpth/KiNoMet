#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Decodes up to `maxSamples` mono 8-bit signed PCM samples from the MP3 stream in `src`
// (length `srcLen` bytes), starting at `*srcPos`. Implementations should advance `*srcPos`
// past whatever compressed bytes were consumed and write decoded samples to `dst`.
// Returns the number of samples actually written; 0 means end of stream (or no decoder
// attached -- see below), which AudioV3 treats as "nothing left to play".
//
// ---------------------------------------------------------------------------------------
// STATUS: STUB (see mp3_decoder_gba_stub.c). It always returns 0, so an AudioV3 (MP3)
// track is silent on GBA today, but the GBA build still links and everything else
// (ffmpeg pipeline, container format, AudioManager dispatch, Windows/preview playback via
// dr_mp3) works end to end. This is the one piece that still needs the real decoder.
//
// The real decoder is Martin Korth's ("Nocash") GBA/NDS/DSi/3DS MP3+FLAC player, supplied
// as MP3.A22 (the ROM built from it identifies itself as "NOCASH MP3"). Its core routines
// are exactly what this function needs to wrap:
//
//   mp3_decode_frame:              MP3.A22 ~line 32954   in: r6=dst, [mp3ctx_stream_pos]=src
//   mp3_decode_mpeg_audio_frame:   MP3.A22 ~line 21382   in: r6=dst, [mp3ctx_stream_pos]=src
//   mp3_check_1st_frame:           MP3.A22 ~line 33113   io: [mp3ctx_stream_pos],[mp3ctx_stream_end]
//
// That source is written for Nocash's own assembler dialect (equ/.errif directives, ";"
// end-of-line comments, "@@" local labels, .pack_crc16/.hexdat/.rept) -- devkitARM's GNU
// `as` cannot assemble it unmodified. Turning it into something linkable here means either:
//
//   (a) hand-translating mp3_decode_frame and everything it touches (the bitstream reader,
//       huffman tables/decode, requantization, stereo processing, IMDCT + polyphase
//       synthesis -- realistically several thousand lines) into GNU `as` syntax, or
//       re-hosting the same algorithm as a small .c file built with -mthumb like the rest
//       of this project, or
//   (b) building mp3.a22 with Nocash's own tool and linking the resulting object file in,
//       if that tool is available to whoever finishes this.
//
// Either way this needs a real GBA or an emulator (mGBA, no$gba) to verify against --
// MP3 decoding is bit-exact-sensitive, so a subtle bug produces noise, not a crash. Neither
// devkitARM nor a GBA emulator was available in the environment this stub was written in,
// which is why the port wasn't attempted blind. The included MP3.GBA ("NOCASH MP3") ROM is
// a good reference to compare decoded output against once a toolchain is in place.
//
// To finish: replace the body of mp3_decode_frame_gba() in mp3_decoder_gba_stub.c with a
// call into the ported/assembled routine, keeping this exact signature so AudioV3 doesn't
// need to change.
// ---------------------------------------------------------------------------------------
int mp3_decode_frame_gba(const unsigned char* src, unsigned long srcLen, unsigned long* srcPos, signed char* dst, int maxSamples);

#ifdef __cplusplus
}
#endif
