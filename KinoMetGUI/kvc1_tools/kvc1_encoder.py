#!/usr/bin/env python3
"""
KVC1 video encoder (format version 2) for KiNoMet. Decoder: KiNomet/kvc1_decode.c
(kvc1_reader_*) driven by KiNomet/kvc1_load.cpp; Python reference decoder:
kvc1_decode_ref.py.

Pipeline: ffmpeg extracts RGB24 frames at the target size/fps -> RGB555 ->
per-block candidates (FLAT / BTC / VQ against a trained whole-video codebook)
-> per frame, each block's mode is chosen by rate-distortion (D + lambda*R)
against the reconstructed previous frame -> frames are packed into chunks of
at most --chunk-cap decoded bytes, each chunk GBA-BIOS-LZ77 compressed (or
stored raw if that's smaller). With --target-bytes, lambda is searched so the
file lands just under that size.

File layout (little-endian; every chunk payload starts 4-byte aligned, which
the GBA BIOS LZ77 call requires):
  0   magic        "KVC1"
  4   version      u8   = 2
  5   fps          u8
  6   width        u16
  8   height       u16
  10  n_codebook   u16  (entries; each is 16 pixels of u16 RGB555 = 32 bytes)
  12  n_frames     u32
  16  max_declen   u32  (largest decoded chunk; decoder buffer must be >= this)
  20  reserved     u32  = 0
  24  codebook     n_codebook * 32 bytes
  then chunks until n_frames frames are covered:
    u8  tag        0 = stored, 1 = BIOS LZ77
    u8  n_frames   frames in this chunk (1..255)
    u16 reserved   = 0
    u32 comp_len   payload bytes (payload is followed by 0-3 pad bytes to 4-align)
    u32 dec_len    decoded size
    payload

Decoded chunk:
  u16 ops_len, u16 n_flat, u16 n_btc, u16 n_raw    (VQ count = whatever is left)
  ops[ops_len]       run opcodes, block-scan order, frame after frame;
                     a run never crosses a frame boundary:
                       0x00-0x7F  SKIP n = op+1 (1..128)
                       0x80-0x9F  FLAT n = (op&31)+1
                       0xA0-0xBF  VQ   n = (op&31)+1
                       0xC0-0xDF  BTC  n = (op&31)+1
                       0xE0-0xFF  RAW  n = (op&31)+1
  flat[n_flat]       u16 colour
  btc[n_btc]         u16 c0, u16 c1, u16 mask (bit k set -> pixel k uses c1)
  raw[n_raw]         16 x u16 pixels, raster order within the block
  vq[...]            u8 codebook index
Pixels within a block are raster order (row 0 left-to-right, then row 1 ...).
"""
import argparse
import math
import os
import struct
import subprocess
import sys
import time

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gba_lz77  # noqa: E402
from kvc1_codec import (  # noqa: E402
    MODE_SKIP, MODE_FLAT, MODE_VQ, MODE_BTC, MODE_RAW,
    frame_to_blocks, rgb888_to_555, pack555, wsse, analyze_blocks,
    btc_reconstruct, train_codebook, VQSearcher, choose_modes,
)

MAGIC = b"KVC1"
VERSION = 2
HEADER_SIZE = 24
CHUNK_HDR_SIZE = 12
CHUNK_INNER_HDR = 8
OP_BASE = {MODE_FLAT: 0x80, MODE_VQ: 0xA0, MODE_BTC: 0xC0, MODE_RAW: 0xE0}


def log(msg):
    print(f"[kvc1] {msg}", flush=True)


def extract_frames(video_path, w, h, fps):
    """Yields (h,w,3) uint8 RGB frames from ffmpeg at the given size/fps."""
    cmd = ["ffmpeg", "-v", "error", "-i", video_path,
           "-vf", f"fps={fps},scale={w}:{h}:flags=area",
           "-pix_fmt", "rgb24", "-f", "rawvideo", "-"]
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    fb = w * h * 3
    try:
        while True:
            buf = p.stdout.read(fb)
            if len(buf) < fb:
                break
            yield np.frombuffer(buf, dtype=np.uint8).reshape(h, w, 3)
    finally:
        p.stdout.close()
        if p.wait() != 0:
            raise RuntimeError(f"ffmpeg failed reading {video_path}")


def encode_ops(modes):
    """modes: (nb,) uint8 -> op bytes for one frame."""
    out = bytearray()
    cut = np.flatnonzero(np.diff(modes)) + 1
    starts = np.concatenate(([0], cut))
    ends = np.concatenate((cut, [len(modes)]))
    for s, e in zip(starts.tolist(), ends.tolist()):
        m = int(modes[s])
        n = e - s
        if m == MODE_SKIP:
            while n > 0:
                k = min(n, 128)
                out.append(k - 1)
                n -= k
        else:
            base = OP_BASE[m]
            while n > 0:
                k = min(n, 32)
                out.append(base | (k - 1))
                n -= k
    return bytes(out)


class Prepared:
    """Per-frame data that doesn't depend on lambda (computed once)."""

    def __init__(self, blocks, ana, vq_idx, d_vq, codebook, bh, bw, fps):
        self.blocks = blocks      # (F,nb,16,3) uint8 RGB555
        self.ana = ana            # dict of (F,nb,...) arrays
        self.vq_idx = vq_idx      # (F,nb) uint8
        self.d_vq = d_vq          # (F,nb) float32
        self.codebook = codebook  # (C,16,3) uint8
        self.bh, self.bw, self.fps = bh, bw, fps


def prepare(video_path, w, h, fps, n_clusters, dither, seed=0, max_train=400000):
    t0 = time.time()
    log(f"extracting frames via ffmpeg @ {w}x{h} {fps}fps ...")
    blocks = []
    for fr in extract_frames(video_path, w, h, fps):
        b, bh, bw = frame_to_blocks(rgb888_to_555(fr, dither))
        blocks.append(b)
    if not blocks:
        raise RuntimeError("no frames decoded")
    blocks = np.stack(blocks)
    F, nb = blocks.shape[:2]
    log(f"{F} frames ({F / fps:.1f}s), {nb} blocks/frame")

    keys = ("flat_c", "d_flat", "btc_c0", "btc_c1", "btc_mask", "d_btc")
    ana = {k: [] for k in keys}
    for f in range(F):
        a = analyze_blocks(blocks[f])
        for k in keys:
            ana[k].append(a[k])
    ana = {k: np.stack(v) for k, v in ana.items()}

    # Codebook: train on textured blocks that FLAT/BTC don't already handle well.
    textured = np.minimum(ana["d_flat"], ana["d_btc"]) > 40.0
    fidx, bidx = np.nonzero(textured)
    rng = np.random.default_rng(seed)
    if len(fidx) > max_train:
        sel = rng.choice(len(fidx), size=max_train, replace=False)
        fidx, bidx = fidx[sel], bidx[sel]
    log(f"training {n_clusters}-entry codebook on {len(fidx)} textured blocks ...")
    codebook = train_codebook(blocks[fidx, bidx], n_clusters=n_clusters, seed=seed)

    vs = VQSearcher(codebook)
    vq_idx = np.empty((F, nb), dtype=np.uint8)
    d_vq = np.empty((F, nb), dtype=np.float32)
    for f in range(F):
        vq_idx[f], d_vq[f] = vs.search(blocks[f])
    log(f"analysis done in {time.time() - t0:.1f}s")
    return Prepared(blocks, ana, vq_idx, d_vq, codebook, bh, bw, fps)


def run_pass(P, lam, chunk_cap):
    """Encodes every frame at this lambda. Returns (frames, stats) where frames
    is a list of per-frame (ops, flat, btc, raw, vq) byte strings."""
    F, nb = P.blocks.shape[:2]
    a = P.ana
    cb = P.codebook
    ref = np.zeros((nb, 16, 3), dtype=np.uint8)
    frames = []
    counts = np.zeros(5, dtype=np.int64)
    sse = 0.0
    bumped = 0
    frame_budget = chunk_cap - CHUNK_INNER_HDR
    for f in range(F):
        cur = P.blocks[f]
        d_skip = wsse(cur, ref)
        lf = lam
        while True:
            modes = choose_modes(d_skip, a["d_flat"][f], P.d_vq[f], a["d_btc"][f], lf, allow_skip=f > 0)
            m_flat = modes == MODE_FLAT
            m_vq = modes == MODE_VQ
            m_btc = modes == MODE_BTC
            m_raw = modes == MODE_RAW
            ops = encode_ops(modes)
            size = (len(ops) + 2 * int(m_flat.sum()) + int(m_vq.sum())
                    + 6 * int(m_btc.sum()) + 32 * int(m_raw.sum()))
            if size <= frame_budget:
                break
            lf = lf * 2.0 if lf > 0 else 1.0
            bumped += 1

        flat_c = a["flat_c"][f][m_flat]
        c0 = a["btc_c0"][f][m_btc]
        c1 = a["btc_c1"][f][m_btc]
        mask = a["btc_mask"][f][m_btc]
        raw_b = cur[m_raw]
        vqi = P.vq_idx[f][m_vq]

        flat_bytes = pack555(flat_c).astype("<u2").tobytes()
        btc_bytes = np.stack([pack555(c0), pack555(c1), mask], axis=1).astype("<u2").tobytes()
        raw_bytes = pack555(raw_b).astype("<u2").tobytes()
        vq_bytes = vqi.tobytes()
        frames.append((ops, flat_bytes, btc_bytes, raw_bytes, vq_bytes))

        # Reconstruct exactly what the decoder will show.
        ref = ref.copy()
        ref[m_flat] = flat_c[:, None, :]
        ref[m_vq] = cb[vqi]
        ref[m_btc] = btc_reconstruct(c0, c1, mask)
        ref[m_raw] = raw_b
        d = cur.astype(np.int32) - ref.astype(np.int32)
        sse += float((d * d).sum())
        counts += np.bincount(modes, minlength=5)

    mse = sse / (F * nb * 48)
    psnr = 10 * math.log10(31.0 ** 2 / mse) if mse > 0 else 99.0
    return frames, {"counts": counts, "psnr": psnr, "bumped": bumped}


def build_chunks(frames, chunk_cap):
    """Groups consecutive frames into chunks whose decoded size <= chunk_cap.
    Returns list of (n_frames, decoded_bytes)."""
    chunks = []
    cur = []
    cur_size = CHUNK_INNER_HDR

    def flush():
        ops = b"".join(fr[0] for fr in cur)
        flat = b"".join(fr[1] for fr in cur)
        btc = b"".join(fr[2] for fr in cur)
        raw = b"".join(fr[3] for fr in cur)
        vq = b"".join(fr[4] for fr in cur)
        hdr = struct.pack("<HHHH", len(ops), len(flat) // 2, len(btc) // 6, len(raw) // 32)
        chunks.append((len(cur), hdr + ops + flat + btc + raw + vq))

    for fr in frames:
        sz = sum(len(x) for x in fr)
        if cur and (cur_size + sz > chunk_cap or len(cur) == 255):
            flush()
            cur, cur_size = [], CHUNK_INNER_HDR
        cur.append(fr)
        cur_size += sz
    if cur:
        flush()
    return chunks


def _compress_one(data):
    comp = gba_lz77.compress(data)
    if len(comp) < len(data):
        return 1, comp
    return 0, data


def compress_chunks(chunks, pool=None):
    datas = [c[1] for c in chunks]
    if pool is not None:
        results = pool.map(_compress_one, datas, chunksize=8)
    else:
        results = [_compress_one(d) for d in datas]
    return [(c[0], c[1], tag, payload) for c, (tag, payload) in zip(chunks, results)]


def file_size(cchunks, n_cb):
    total = HEADER_SIZE + n_cb * 32
    for _, _, _, payload in cchunks:
        total += CHUNK_HDR_SIZE + ((len(payload) + 3) & ~3)
    return total


def write_file(path, P, cchunks):
    F, nb = P.blocks.shape[:2]
    h = P.bh * 4
    w = P.bw * 4
    n_cb = P.codebook.shape[0]
    max_declen = max(len(dec) for _, dec, _, _ in cchunks)
    out = bytearray()
    out += MAGIC + struct.pack("<BBHHHIII", VERSION, P.fps, w, h, n_cb, F, max_declen, 0)
    assert len(out) == HEADER_SIZE
    out += pack555(P.codebook).astype("<u2").tobytes()
    for nfr, dec, tag, payload in cchunks:
        out += struct.pack("<BBHII", tag, nfr, 0, len(payload), len(dec))
        out += payload
        out += b"\0" * ((-len(payload)) % 4)
    with open(path, "wb") as fh:
        fh.write(out)
    return len(out), max_declen


def report(stats, size, F, fps, lam):
    c = stats["counts"]
    tot = c.sum()
    names = ["SKIP", "FLAT", "VQ", "BTC", "RAW"]
    mix = " ".join(f"{n}={c[i] / tot:.1%}" for i, n in enumerate(names))
    log(f"lambda={lam:.3g}: {size} bytes ({size * 8 / (F / fps) / 1000:.1f} kbit/s), "
        f"PSNR {stats['psnr']:.2f} dB, {mix}"
        + (f", {stats['bumped']} frame(s) coarsened to fit chunk cap" if stats["bumped"] else ""))


def encode(video_path, out_path, w=240, h=160, fps=15, n_clusters=256, dither=False,
           lam=None, target_bytes=0, chunk_cap=16384, jobs=0):
    P = prepare(video_path, w, h, fps, n_clusters, dither)
    F = P.blocks.shape[0]
    n_cb = P.codebook.shape[0]

    pool = None
    if jobs != 1:
        import multiprocessing as mp
        pool = mp.Pool(jobs if jobs > 0 else None)

    def attempt(l):
        frames, stats = run_pass(P, l, chunk_cap)
        cc = compress_chunks(build_chunks(frames, chunk_cap), pool)
        size = file_size(cc, n_cb)
        report(stats, size, F, fps, l)
        return size, cc, stats

    try:
        if not target_bytes:
            l = lam if lam is not None else 200.0
            size, cc, stats = attempt(l)
        else:
            log(f"searching for lambda to fit {target_bytes} bytes ...")
            lo_ok = None   # (lam, size, cc, stats) best fitting attempt
            hi_bad = None  # (lam, size) smallest-lambda... tracking bracket
            pts = []
            l = lam if lam is not None else 200.0
            for _ in range(12):
                size, cc, stats = attempt(l)
                pts.append((l, size))
                if size <= target_bytes:
                    if lo_ok is None or l < lo_ok[0]:
                        lo_ok = (l, size, cc, stats)
                    if size >= target_bytes * 0.975:
                        break
                else:
                    if hi_bad is None or l > hi_bad[0]:
                        hi_bad = (l, size)
                # next guess: secant on log(size) vs log(lambda), kept inside the bracket
                goal = target_bytes * 0.99
                if len(pts) >= 2 and pts[-1][1] != pts[-2][1] and pts[-1][0] != pts[-2][0]:
                    (l1, s1), (l2, s2) = pts[-2], pts[-1]
                    slope = (math.log(s2) - math.log(s1)) / (math.log(l2) - math.log(l1))
                    if slope >= -1e-3:
                        slope = -0.5
                    nl = math.exp(math.log(l2) + (math.log(goal) - math.log(s2)) / slope)
                else:
                    nl = l * (4.0 if size > target_bytes else 0.25)
                if lo_ok is not None and hi_bad is not None:
                    a, b = math.log(hi_bad[0]), math.log(lo_ok[0])
                    x = math.log(nl)
                    if not (min(a, b) < x < max(a, b)):
                        nl = math.exp((a + b) / 2)
                l = max(nl, 1e-3)
            if lo_ok is None:
                raise RuntimeError(f"could not get under {target_bytes} bytes")
            l, size, cc, stats = lo_ok
    finally:
        if pool is not None:
            pool.close()

    written, max_declen = write_file(out_path, P, cc)
    log(f"wrote {out_path}: {written} bytes, {len(cc)} chunks, largest decoded chunk {max_declen} bytes")
    report(stats, written, F, fps, l)
    return written


def main():
    ap = argparse.ArgumentParser(description="Encode a video to KVC1 (v2)")
    ap.add_argument("video")
    ap.add_argument("out")
    ap.add_argument("--width", type=int, default=240)
    ap.add_argument("--height", type=int, default=160)
    ap.add_argument("--fps", type=int, default=15)
    ap.add_argument("--clusters", type=int, default=256)
    ap.add_argument("--target-bytes", type=int, default=0,
                    help="search lambda so the output is just under this many bytes")
    ap.add_argument("--lambda", dest="lam", type=float, default=None,
                    help="fixed rate/quality tradeoff (higher = smaller, lower quality); "
                         "starting guess when --target-bytes is set")
    ap.add_argument("--chunk-cap", type=int, default=16384,
                    help="max decoded bytes per chunk (= decoder buffer size, KVC1_CHUNK_CAP)")
    ap.add_argument("--dither", action="store_true")
    ap.add_argument("--jobs", type=int, default=0, help="LZ worker processes (0 = all cores)")
    args = ap.parse_args()
    encode(args.video, args.out, w=args.width, h=args.height, fps=args.fps,
           n_clusters=args.clusters, dither=args.dither, lam=args.lam,
           target_bytes=args.target_bytes, chunk_cap=args.chunk_cap, jobs=args.jobs)


if __name__ == "__main__":
    main()
