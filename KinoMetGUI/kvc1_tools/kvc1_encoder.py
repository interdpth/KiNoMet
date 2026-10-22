#!/usr/bin/env python3
"""
KVC1-C video encoder: packs a video into the custom block-based codec KiNomet
plays on GBA/Windows in place of Cinepak (see KiNomet/kvc1_load.cpp for the
decoder, KiNomet/kvc1_decode.c for the shared block-expansion logic).

Pipeline: ffmpeg extracts raw RGB24 frames at the target fps/resolution ->
each frame is ordered-dithered and quantized to RGB555 -> a whole-video
codebook is trained once (MiniBatchKMeans over sampled blocks, see
kvc1_codec.train_codebook) -> each frame is classified block-by-block against
the previous frame and the codebook (SKIP/FLAT/VQ/RAW, see
kvc1_codec.FrameClassifier) -> frames are grouped into GOPs, each of the four
per-GOP streams (mode/flat/vq/raw) is independently LZ77-compressed (GBA BIOS
LZ77UnComp format, see gba_lz77.py) or stored raw, whichever is smaller.

File layout (all integers little-endian):
  magic        4 bytes  "KVC1"
  version      u8        1
  width        u16
  height       u16
  fps          u8
  gop_size     u8
  n_clusters   u16
  n_frames     u32
  codebook_len u32        = n_clusters * 48
  codebook     codebook_len bytes  (n_clusters * 16 pixels * 3 (R,G,B), 0..31 each)
  then, repeated once per GOP chunk until n_frames frames have been emitted:
    for each of (mode, flat, vq, raw) in that order:
      tag        u8        0 = stored raw, 1 = LZ77-compressed
      comp_len   u32       bytes that follow for this stream
      dec_len    u32       decompressed size of this stream
    mode_bytes, flat_bytes, vq_bytes, raw_bytes (comp_len bytes each, in order)

  mode stream (decompressed): 1 byte/block, (frames_in_chunk * nblocks) bytes,
    block-scan order within each frame, frames in order. 0=SKIP,1=FLAT,2=VQ,3=RAW.
  flat stream: 3 bytes (R,G,B, 0..31) per FLAT block, in mode-stream order.
  vq stream:   1 byte (codebook index) per VQ block, in mode-stream order.
  raw stream:  48 bytes (16 px * R,G,B) per RAW block, in mode-stream order.
"""
import argparse
import struct
import subprocess
import sys
import os

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gba_lz77
from kvc1_codec import frame_to_blocks, dither_offsets, rgb888_to_555_dithered, train_codebook, FrameClassifier

MAGIC = b"KVC1"
VERSION = 1


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

    header = MAGIC + struct.pack("<BHHBBHI", VERSION, w, h, fps, gop, n_clusters, n_frames)
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
            mode_stream += modes.tobytes()
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


def main():
    ap = argparse.ArgumentParser(description="Encode a video to KVC1-C")
    ap.add_argument("video")
    ap.add_argument("out")
    ap.add_argument("--width", type=int, default=240)
    ap.add_argument("--height", type=int, default=160)
    ap.add_argument("--fps", type=int, default=15)
    ap.add_argument("--clusters", type=int, default=256)
    ap.add_argument("--gop", type=int, default=15)
    ap.add_argument("--skip-t", type=float, default=60)
    ap.add_argument("--flat-t", type=float, default=800)
    ap.add_argument("--vq-t", type=float, default=8000)
    ap.add_argument("--no-dither", dest="dither", action="store_false")
    args = ap.parse_args()

    encode(args.video, args.out, w=args.width, h=args.height, fps=args.fps,
           n_clusters=args.clusters, gop=args.gop, skip_t=args.skip_t,
           flat_t=args.flat_t, vq_t=args.vq_t, dither=args.dither)


if __name__ == "__main__":
    main()
