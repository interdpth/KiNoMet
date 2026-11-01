#!/usr/bin/env python3
"""
KVC1-C video encoder: packs a video into the custom block-based codec KiNomet
plays on GBA/Windows in place of Cinepak (see KiNomet/kvc1_load.cpp for the
decoder, KiNomet/kvc1_decode.c for the shared block-expansion logic).

Two encoders share this file format and most of the pipeline, picking the
one that suits the source content:

  encode() -- general color video. ffmpeg extracts raw RGB24 frames at the
    target fps/resolution -> each frame is ordered-dithered and quantized to
    RGB555 -> a whole-video codebook is trained once (MiniBatchKMeans over
    sampled blocks, see kvc1_codec.train_codebook) -> each frame is
    classified block-by-block (SKIP/FLAT/VQ/RAW, see kvc1_codec.FrameClassifier).

  encode_grayscale() -- genuinely monochrome source (R=G=B, e.g. a black and
    white cel-animation short -- detect with is_grayscale_source() below, or
    just look at it). Instead of RGB555 + dithering, builds a small (16-32
    entry) Lloyd-Max-optimal gray palette from the video's own luma
    histogram (kvc1_codec.build_gray_palette) and classifies blocks of
    PALETTE INDICES against it (kvc1_codec.GrayFrameClassifier) -- no
    dithering at all. This exists because applying the general RGB555+dither
    path to monochrome content wastes 2/3 of RGB555's bit depth on redundant
    color channels and then has to dither away banding that a properly
    Lloyd-Max-placed palette mostly avoids in the first place; dithering
    noise swamping flat/low-contrast regions (background walls, water,
    smoke) was the actual, confirmed root cause of a real quality complaint
    on real grayscale footage that motivated adding this path.

Either way, frames are grouped into GOPs, and each of the four per-GOP
streams (mode/flat/vq/raw) is independently LZ77-compressed (GBA BIOS
LZ77UnComp format, see gba_lz77.py) or stored raw, whichever is smaller.

File layout (all integers little-endian):
  magic        4 bytes  "KVC1"
  version      u8        2
  width        u16
  height       u16
  fps          u8
  gop_size     u8
  n_clusters   u16
  n_frames     u32
  color_mode   u8        0 = RGB555 (encode()), 1 = gray palette (encode_grayscale())
  -- only present when color_mode == 1 --
  palette_len  u8
  palette      palette_len bytes (0..31 RGB555-domain gray levels, ascending)
  -- resumes for both color_modes --
  codebook_len u32        = n_clusters * 48 (color_mode 0) or n_clusters * 16 (color_mode 1)
  codebook     codebook_len bytes
    color_mode 0: n_clusters * 16 pixels * 3 (R,G,B), 0..31 each
    color_mode 1: n_clusters * 16 pixels, each a palette index 0..palette_len-1
  then, repeated once per GOP chunk until n_frames frames have been emitted:
    for each of (mode, flat, vq, raw) in that order:
      tag        u8        0 = stored raw, 1 = LZ77-compressed
      comp_len   u32       bytes that follow for this stream
      dec_len    u32       decompressed size of this stream
    mode_bytes, flat_bytes, vq_bytes, raw_bytes (comp_len bytes each, in order)

  mode stream (decompressed): run-length tokens, NOT 1 byte/block. Each token
    byte is (kind<<5)|(run-1): top 3 bits = mode (0=SKIP,1=FLAT,2=VQ,3=RAW),
    bottom 5 bits = run_length-1, so one token covers 1..32 consecutive blocks
    of the same mode (raster order). Runs never cross a frame boundary (each
    frame's block-mode sequence starts a fresh token at the current stream
    position; see rle_encode_modes/rle_decode_modes below). This matches the
    real GBA decoder's actual on-disk mode-token format (reverse-engineered
    from a shipped build's DWARF debug info + disassembly -- kvc1_decode.c's
    kvc1_reader_next_frame reads exactly this: `n = (byte&0x1F)+1; kind =
    byte>>5`) rather than our earlier guess of 1 raw byte/block. It's a large
    win on real content: long static SKIP runs (titles, backgrounds) collapse
    to ~1 byte per 32 blocks before LZ77 even sees the stream.
  vq stream:   1 byte (codebook index) per VQ block, in mode-stream order -- same for both color_modes.
  color_mode 0 (RGB555):
    flat stream: 3 bytes (R,G,B, 0..31) per FLAT block, in mode-stream order.
    raw stream:  48 bytes (16 px * R,G,B) per RAW block, in mode-stream order.
  color_mode 1 (gray palette):
    flat stream: 1 byte (palette index) per FLAT block, in mode-stream order.
    raw stream:  16 bytes (16 px * palette index) per RAW block, in mode-stream order.
"""
import argparse
import struct
import subprocess
import sys
import os

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gba_lz77
from kvc1_codec import (frame_to_blocks, dither_offsets, rgb888_to_555_dithered, train_codebook, FrameClassifier,
                         frame_to_blocks_mono, build_gray_palette, gray_palette_to_rgb555, quantize_to_palette,
                         train_gray_codebook, GrayFrameClassifier)

MAGIC = b"KVC1"
VERSION = 2
# color_mode values in the v2 header (see encode()'s and encode_grayscale()'s
# docstrings for the exact byte layout each one implies):
COLOR_RGB555 = 0
COLOR_GRAY_PALETTE = 1


def extract_frames(video_path, w, h, fps, tmp_raw):
    """Runs ffmpeg once to produce a raw rgb24 stream at (w,h)@fps, returns an
    (N,h,w,3) uint8 array."""
    cmd = [
        "ffmpeg", "-y", "-i", video_path,
        "-vf", f"fps={fps},scale={w}:{h}:flags=bicubic",
        "-pix_fmt", "rgb24", "-f", "rawvideo", tmp_raw,
    ]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    raw = np.fromfile(tmp_raw, dtype=np.uint8)
    frame_bytes = w * h * 3
    n = len(raw) // frame_bytes
    if n * frame_bytes != len(raw):
        raw = raw[:n * frame_bytes]
    return raw.reshape(n, h, w, 3)


def best_compress(data: bytes):
    """Returns (tag, payload): tag 1 + LZ77 payload if that's smaller than
    storing raw, else tag 0 + the raw bytes themselves."""
    if len(data) == 0:
        return 0, b""
    comp = gba_lz77.compress(bytes(data))
    if len(comp) < len(data):
        return 1, comp
    return 0, bytes(data)


def pack_stream_header(tag, comp_len, dec_len):
    return struct.pack("<BII", tag, comp_len, dec_len)


def rle_encode_modes(modes):
    """Run-length-encodes one frame's per-block mode array (values 0..7, one
    entry per block in raster order) into the real decoder's token format:
    each output byte is (kind<<5)|(run-1), run in [1,32]. See the module
    docstring's "mode stream" section."""
    modes = np.asarray(modes, dtype=np.uint8)
    out = bytearray()
    n = len(modes)
    i = 0
    while i < n:
        kind = int(modes[i])
        j = i + 1
        limit = min(n, i + 32)
        while j < limit and modes[j] == kind:
            j += 1
        run = j - i
        out.append((kind << 5) | (run - 1))
        i = j
    return bytes(out)


def encode(video_path, out_path, w=240, h=160, fps=15, n_clusters=256, gop=15,
           skip_t=60, flat_t=800, vq_t=8000, dither=True,
           sample_stride=2, blocks_per_frame_sample=900, seed=0, tmp_dir=None):
    tmp_dir = tmp_dir or os.path.dirname(os.path.abspath(out_path)) or "."
    tmp_raw = os.path.join(tmp_dir, "_kvc1_tmp_raw.rgb24")

    print(f"[kvc1] extracting frames via ffmpeg @ {w}x{h} {fps}fps ...")
    frames888 = extract_frames(video_path, w, h, fps, tmp_raw)
    try:
        os.remove(tmp_raw)
    except OSError:
        pass
    n_frames = frames888.shape[0]
    print(f"[kvc1] {n_frames} frames extracted")

    off = dither_offsets(h, w) if dither else np.zeros((h, w), dtype=np.float32)

    print("[kvc1] quantizing to RGB555 (dithered)" if dither else "[kvc1] quantizing to RGB555")
    frames555 = np.empty((n_frames, h, w, 3), dtype=np.uint8)
    for i in range(n_frames):
        frames555[i] = rgb888_to_555_dithered(frames888[i], off)
    del frames888

    print("[kvc1] sampling blocks for codebook training ...")
    rng = np.random.default_rng(seed)
    sample_blocks = []
    for i in range(0, n_frames, sample_stride):
        blocks, bh, bw = frame_to_blocks(frames555[i])
        nblocks = blocks.shape[0]
        k = min(blocks_per_frame_sample, nblocks)
        idx = rng.choice(nblocks, size=k, replace=False)
        sample_blocks.append(blocks[idx])
    sample_blocks = np.concatenate(sample_blocks, axis=0)
    print(f"[kvc1] training codebook: {sample_blocks.shape[0]} sample blocks -> {n_clusters} clusters")
    codebook = train_codebook(sample_blocks, n_clusters=n_clusters, seed=seed)

    classifier = FrameClassifier(codebook, skip_t=skip_t, flat_t=flat_t, vq_t=vq_t)

    _, bh, bw = frame_to_blocks(frames555[0])
    nblocks = bh * bw

    header = MAGIC + struct.pack("<BHHBBHIB", VERSION, w, h, fps, gop, n_clusters, n_frames, COLOR_RGB555)
    codebook_bytes = codebook.astype(np.uint8).tobytes()
    header += struct.pack("<I", len(codebook_bytes)) + codebook_bytes

    out_chunks = [header]
    mode_counts = {0: 0, 1: 0, 2: 0, 3: 0}
    max_raw_len = {"mode": 0, "flat": 0, "vq": 0, "raw": 0}
    total_comp = {"mode": 0, "flat": 0, "vq": 0, "raw": 0}

    prev_blocks = None
    i = 0
    while i < n_frames:
        chunk_end = min(i + gop, n_frames)
        mode_stream = bytearray()
        flat_stream = bytearray()
        vq_stream = bytearray()
        raw_stream = bytearray()

        for fidx in range(i, chunk_end):
            cur_blocks, _, _ = frame_to_blocks(frames555[fidx])
            modes, flat_vals, vq_idx, raw_blocks = classifier.encode_frame(cur_blocks, prev_blocks)
            for v in modes:
                mode_counts[int(v)] += 1
            mode_stream += rle_encode_modes(modes)
            flat_stream += flat_vals.tobytes()
            vq_stream += vq_idx.astype(np.uint8).tobytes()
            raw_stream += raw_blocks.astype(np.uint8).tobytes()
            prev_blocks = cur_blocks

        for name, stream in (("mode", mode_stream), ("flat", flat_stream), ("vq", vq_stream), ("raw", raw_stream)):
            max_raw_len[name] = max(max_raw_len[name], len(stream))

        for name, stream in (("mode", mode_stream), ("flat", flat_stream), ("vq", vq_stream), ("raw", raw_stream)):
            tag, payload = best_compress(stream)
            total_comp[name] += len(payload)
            out_chunks.append(pack_stream_header(tag, len(payload), len(stream)))
            out_chunks.append(bytes(payload))

        i = chunk_end
        if (i // gop) % 20 == 0:
            print(f"[kvc1]   encoded frame {i}/{n_frames}")

    data = b"".join(out_chunks)
    with open(out_path, "wb") as f:
        f.write(data)

    total_blocks = sum(mode_counts.values())
    print(f"[kvc1] mode mix: SKIP={mode_counts[0]/total_blocks:.1%} FLAT={mode_counts[1]/total_blocks:.1%} "
          f"VQ={mode_counts[2]/total_blocks:.1%} RAW={mode_counts[3]/total_blocks:.1%}")
    print(f"[kvc1] real per-chunk MAX decompressed size (mode,flat,vq,raw) = "
          f"({max_raw_len['mode']},{max_raw_len['flat']},{max_raw_len['vq']},{max_raw_len['raw']})")
    print(f"[kvc1] compressed stream totals (mode,flat,vq,raw) = "
          f"({total_comp['mode']},{total_comp['flat']},{total_comp['vq']},{total_comp['raw']})")
    print(f"[kvc1] final file size: {len(data)} bytes ({len(data)/1024/1024:.3f} MB) for {n_frames} frames "
          f"({n_frames/fps:.1f}s @ {fps}fps)")
    return len(data)


def is_grayscale_source(frames888, sample_frames=10, max_mean_chan_spread=3.0):
    """Cheap heuristic: sample a few frames, check how far R/G/B actually
    spread apart per pixel on average. A true monochrome source (even after
    lossy h264 compression, which adds a little chroma noise) sits a few
    units apart at most; real color content is routinely 10s-100s of units
    apart. Used to pick encode() vs encode_grayscale() automatically when
    the caller doesn't already know."""
    n = frames888.shape[0]
    idx = np.linspace(0, n - 1, min(sample_frames, n)).astype(int)
    sample = frames888[idx].astype(np.int16)
    spread = (sample.max(axis=-1) - sample.min(axis=-1)).mean()
    return spread <= max_mean_chan_spread


def encode_grayscale(video_path, out_path, w=240, h=160, fps=15, n_clusters=256, gop=15,
                      n_levels=24, skip_t=0, flat_t=48, vq_t=400,
                      sample_stride=5, blocks_per_frame_sample=400, seed=0, tmp_dir=None):
    """See this module's docstring for the file format and the reasoning
    for using this instead of encode() on monochrome source. No dithering
    step -- quantize_to_palette() is a direct nearest-level lookup."""
    tmp_dir = tmp_dir or os.path.dirname(os.path.abspath(out_path)) or "."
    tmp_raw = os.path.join(tmp_dir, "_kvc1_tmp_raw.rgb24")

    print(f"[kvc1-gray] extracting frames via ffmpeg @ {w}x{h} {fps}fps ...")
    frames888 = extract_frames(video_path, w, h, fps, tmp_raw)
    try:
        os.remove(tmp_raw)
    except OSError:
        pass
    n_frames = frames888.shape[0]
    print(f"[kvc1-gray] {n_frames} frames extracted")

    print(f"[kvc1-gray] building {n_levels}-level Lloyd-Max gray palette ...")
    palette255 = build_gray_palette(frames888, n_levels=n_levels)
    palette31 = gray_palette_to_rgb555(palette255)
    print(f"[kvc1-gray] palette (0..255 domain): {np.round(palette255, 1)}")

    print("[kvc1-gray] quantizing to palette indices (no dithering)")
    qframes = quantize_to_palette(frames888, palette31)
    del frames888

    print("[kvc1-gray] training codebook ...")
    codebook = train_gray_codebook(qframes, n_clusters=n_clusters, sample_stride=sample_stride,
                                    blocks_per_frame_sample=blocks_per_frame_sample, seed=seed)
    print(f"[kvc1-gray] codebook trained, shape={codebook.shape}")

    classifier = GrayFrameClassifier(codebook, skip_t=skip_t, flat_t=flat_t, vq_t=vq_t)

    _, bh, bw = frame_to_blocks_mono(qframes[0])
    nblocks = bh * bw

    header = MAGIC + struct.pack("<BHHBBHIB", VERSION, w, h, fps, gop, n_clusters, n_frames, COLOR_GRAY_PALETTE)
    header += struct.pack("<B", len(palette31)) + palette31.astype(np.uint8).tobytes()
    codebook_bytes = codebook.astype(np.uint8).tobytes()
    header += struct.pack("<I", len(codebook_bytes)) + codebook_bytes

    out_chunks = [header]
    mode_counts = {0: 0, 1: 0, 2: 0, 3: 0}
    max_raw_len = {"mode": 0, "flat": 0, "vq": 0, "raw": 0}
    total_comp = {"mode": 0, "flat": 0, "vq": 0, "raw": 0}

    prev_blocks = None
    i = 0
    while i < n_frames:
        chunk_end = min(i + gop, n_frames)
        mode_stream = bytearray()
        flat_stream = bytearray()
        vq_stream = bytearray()
        raw_stream = bytearray()

        for fidx in range(i, chunk_end):
            cur_blocks, _, _ = frame_to_blocks_mono(qframes[fidx])
            modes, flat_vals, vq_idx, raw_blocks = classifier.encode_frame(cur_blocks, prev_blocks)
            for v in modes:
                mode_counts[int(v)] += 1
            mode_stream += rle_encode_modes(modes)
            flat_stream += flat_vals.tobytes()
            vq_stream += vq_idx.astype(np.uint8).tobytes()
            raw_stream += raw_blocks.astype(np.uint8).tobytes()
            prev_blocks = cur_blocks

        for name, stream in (("mode", mode_stream), ("flat", flat_stream), ("vq", vq_stream), ("raw", raw_stream)):
            max_raw_len[name] = max(max_raw_len[name], len(stream))

        for name, stream in (("mode", mode_stream), ("flat", flat_stream), ("vq", vq_stream), ("raw", raw_stream)):
            tag, payload = best_compress(stream)
            total_comp[name] += len(payload)
            out_chunks.append(pack_stream_header(tag, len(payload), len(stream)))
            out_chunks.append(bytes(payload))

        i = chunk_end
        if (i // gop) % 20 == 0:
            print(f"[kvc1-gray]   encoded frame {i}/{n_frames}")

    data = b"".join(out_chunks)
    with open(out_path, "wb") as f:
        f.write(data)

    total_blocks = sum(mode_counts.values())
    print(f"[kvc1-gray] mode mix: SKIP={mode_counts[0]/total_blocks:.1%} FLAT={mode_counts[1]/total_blocks:.1%} "
          f"VQ={mode_counts[2]/total_blocks:.1%} RAW={mode_counts[3]/total_blocks:.1%}")
    print(f"[kvc1-gray] real per-chunk MAX decompressed size (mode,flat,vq,raw) = "
          f"({max_raw_len['mode']},{max_raw_len['flat']},{max_raw_len['vq']},{max_raw_len['raw']})")
    print(f"[kvc1-gray] compressed stream totals (mode,flat,vq,raw) = "
          f"({total_comp['mode']},{total_comp['flat']},{total_comp['vq']},{total_comp['raw']})")
    print(f"[kvc1-gray] final file size: {len(data)} bytes ({len(data)/1024/1024:.3f} MB) for {n_frames} frames "
          f"({n_frames/fps:.1f}s @ {fps}fps)")
    return len(data)


def main():
    ap = argparse.ArgumentParser(description="Encode a video to KVC1-C")
    ap.add_argument("video")
    ap.add_argument("out")
    ap.add_argument("--width", type=int, default=240)
    ap.add_argument("--height", type=int, default=160)
    ap.add_argument("--fps", type=int, default=15)
    ap.add_argument("--clusters", type=int, default=256)
    ap.add_argument("--gop", type=int, default=15)
    ap.add_argument("--skip-t", type=float, default=None,
                     help="default 60 for color, 0 for --grayscale")
    ap.add_argument("--flat-t", type=float, default=None,
                     help="default 800 for color, 48 for --grayscale")
    ap.add_argument("--vq-t", type=float, default=None,
                     help="default 8000 for color, 400 for --grayscale")
    ap.add_argument("--no-dither", dest="dither", action="store_false")
    ap.add_argument("--grayscale", dest="grayscale", action="store_true", default=None,
                     help="force the Lloyd-Max gray-palette path (see module docstring); "
                          "default is auto-detect from the source")
    ap.add_argument("--color", dest="grayscale", action="store_false",
                     help="force the RGB555 path even if the source looks monochrome")
    ap.add_argument("--levels", type=int, default=24, help="gray palette size, --grayscale only")
    args = ap.parse_args()

    grayscale = args.grayscale
    if grayscale is None:
        tmp_dir = os.path.dirname(os.path.abspath(args.out)) or "."
        tmp_raw = os.path.join(tmp_dir, "_kvc1_tmp_probe.rgb24")
        probe = extract_frames(args.video, args.width, args.height, args.fps, tmp_raw)
        try:
            os.remove(tmp_raw)
        except OSError:
            pass
        grayscale = is_grayscale_source(probe)
        del probe
        print(f"[kvc1] auto-detected source as {'grayscale' if grayscale else 'color'}; "
              f"pass --grayscale/--color to override")

    if grayscale:
        encode_grayscale(args.video, args.out, w=args.width, h=args.height, fps=args.fps,
                          n_clusters=args.clusters, gop=args.gop, n_levels=args.levels,
                          skip_t=args.skip_t if args.skip_t is not None else 0,
                          flat_t=args.flat_t if args.flat_t is not None else 48,
                          vq_t=args.vq_t if args.vq_t is not None else 400)
    else:
        encode(args.video, args.out, w=args.width, h=args.height, fps=args.fps,
               n_clusters=args.clusters, gop=args.gop,
               skip_t=args.skip_t if args.skip_t is not None else 60,
               flat_t=args.flat_t if args.flat_t is not None else 800,
               vq_t=args.vq_t if args.vq_t is not None else 8000,
               dither=args.dither)


if __name__ == "__main__":
    main()
