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
  4   version      u8   = 3
  5   fps          u8
  6   width        u16
  8   height       u16
  10  n_codebook   u16  (entries; each is 16 pixels of u16 RGB555 = 32 bytes)
  12  n_frames     u32
  16  max_declen   u32  (largest decoded chunk; decoder buffer must be >= this)
  20  flags        u32  bit 0 = greyscale: every colour below is stored as one
                          byte g (0..31) instead of a u16, and means RGB555
                          g | g<<5 | g<<10 (record sizes in brackets)
  24  codebook     n_codebook * 32 bytes [greyscale: 16 bytes per entry]
  then chunks until n_frames frames are covered:
    u8  tag        0 = stored, 1 = BIOS LZ77
    u8  n_frames   frames in this chunk (1..255)
    u16 reserved   = 0
    u32 comp_len   payload bytes (payload is followed by 0-3 pad bytes to 4-align)
    u32 dec_len    decoded size
    payload

Decoded chunk:
  u16 ops_len, n_flat, n_btc, n_raw, n_paint, n_pal4   (VQ count = whatever is left)
  ops[ops_len]       run opcodes, block-scan order, frame after frame; a run
                     never crosses a frame boundary. op = kind<<5 | (n-1),
                     n = 1..32 blocks:
                       kind 0 SKIP  (keep what's on screen)
                       kind 1 FLAT  kind 2 VQ  kind 3 BTC  kind 4 RAW
                       kind 5 PAINT kind 6 PAL4
  flat[n_flat]       u16 colour                                    [u8]
  btc[n_btc]         u16 c0, u16 c1, u16 mask (bit k set -> pixel k uses c1)
                                                                   [u8 c0, u8 c1, u16 mask]
  raw[n_raw]         16 x u16 pixels, raster order within the block [16 x u8]
  paint[n_paint]     u16 colour, u16 mask (bit k set -> pixel k = colour,
                     others keep what's on screen)                 [u8, u16 mask]
  pal4[n_pal4]       4 x u16 colours, u32 indices (2 bits per pixel, pixel k
                     at bits 2k..2k+1)                             [4 x u8, u32]
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
    MODE_BYTES, MODE_BYTES_GRAY, rgb888_to_gray555, grayness,
    MODE_FLAT, MODE_VQ, MODE_BTC, MODE_RAW, MODE_PAINT, MODE_PAL4, N_MODES,
    frame_to_blocks, rgb888_to_555, pack555, wsse, analyze_blocks,
    btc_reconstruct, pal4_reconstruct, paint_candidate, paint_reconstruct,
    persistence, train_codebook, VQSearcher, choose_modes,
)

MAGIC = b"KVC1"
VERSION = 3
HEADER_SIZE = 24
CHUNK_HDR_SIZE = 12
CHUNK_INNER_HDR = 12
# Below this many bits per pixel per second at 240x160, encode at 120x80 and
# let the GBA scale it 2x: tested on Steamboat Willie (0.5 bpps) this scored
# 25.9 dB against 24.1 dB at full res for the same bytes; the ALIE trailer
# (2.0 bpps) stays full res.
HALF_RES_BPPS = 1.0


def log(msg):
    print(f"[kvc1] {msg}", flush=True)


class Progress:
    """One-line percentage display, rewritten in place with \\r."""

    def __init__(self, label, total):
        self.label, self.total, self.last = label, total, -1
        self.t0 = time.time()
        self.update(0)

    def update(self, i):
        if self.total:
            pct = min(100, int(100 * i / self.total))
            if pct != self.last:
                self.last = pct
                print(f"\r[kvc1] {self.label}: {pct:3d}%  ({i}/{self.total})   ", end="", flush=True)
        elif i % 50 == 0:
            print(f"\r[kvc1] {self.label}: {i}   ", end="", flush=True)

    def done(self, n=None):
        n = self.total if n is None else n
        print(f"\r[kvc1] {self.label}: 100%  ({n}) in {time.time() - self.t0:.0f}s   ", flush=True)


def probe_frame_count(video_path, fps):
    """Approximate output frame count from the source duration (for the
    progress display only; None if ffprobe isn't available)."""
    try:
        out = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration",
                              "-of", "csv=p=0", video_path], capture_output=True, text=True, timeout=30)
        return int(round(float(out.stdout.strip()) * fps))
    except Exception:
        return None


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
        base = int(modes[s]) << 5
        n = e - s
        while n > 0:
            k = min(n, 32)
            out.append(base | (k - 1))
            n -= k
    return bytes(out)


class Prepared:
    """Per-frame data that doesn't depend on lambda (computed once)."""

    def __init__(self, blocks, ana, vq_idx, d_vq, codebook, bh, bw, fps, weight, gray=False):
        self.gray = gray          # greyscale: 1-byte colours, R=G=B everywhere
        self.blocks = blocks      # (F,nb,16,3) uint8 RGB555
        self.weight = weight      # (F,nb) float32 persistence weight
        self.ana = ana            # dict of (F,nb,...) arrays
        self.vq_idx = vq_idx      # (F,nb) uint8
        self.d_vq = d_vq          # (F,nb) float32
        self.codebook = codebook  # (C,16,3) uint8
        self.bh, self.bw, self.fps = bh, bw, fps


def probe_gray(video_path, w, h):
    """True if the video is (essentially) black and white: samples 1 frame per
    second and checks that nearly every pixel has R~G~B."""
    vals = [grayness(fr) for fr in extract_frames(video_path, w, h, 1)]
    return bool(vals) and min(vals) >= 0.995


def prepare(video_path, w, h, fps, n_clusters, dither, persist_cap=45, seed=0, max_train=400000,
            color_mode="auto"):
    t0 = time.time()
    if color_mode == "auto":
        gray = probe_gray(video_path, w, h)
        log(f"colour check: {'greyscale -- using 1-byte grey colours' if gray else 'colour'}")
    else:
        gray = color_mode == "gray"
    log(f"step 1/4: reading video @ {w}x{h} {fps}fps{' (greyscale)' if gray else ''}")
    blocks = []
    prog = Progress("reading frames", probe_frame_count(video_path, fps))
    for fr in extract_frames(video_path, w, h, fps):
        b, bh, bw = frame_to_blocks(rgb888_to_gray555(fr) if gray else rgb888_to_555(fr, dither))
        blocks.append(b)
        prog.update(len(blocks))
    prog.done(len(blocks))
    if not blocks:
        raise RuntimeError("no frames decoded")
    blocks = np.stack(blocks)
    F, nb = blocks.shape[:2]
    log(f"{F} frames ({F / fps:.1f}s), {nb} blocks/frame")

    keys = ("flat_c", "d_flat", "btc_c0", "btc_c1", "btc_mask", "d_btc", "pal4_c", "pal4_idx", "d_pal4")
    ana = {k: [] for k in keys}
    log("step 2/4: analysing blocks")
    prog = Progress("analysing", F)
    for f in range(F):
        a = analyze_blocks(blocks[f])
        for k in keys:
            ana[k].append(a[k])
        prog.update(f + 1)
    prog.done()
    ana = {k: np.stack(v) for k, v in ana.items()}

    # Codebook: train on textured blocks that FLAT/BTC don't already handle well.
    textured = np.minimum(ana["d_flat"], ana["d_btc"]) > 40.0
    fidx, bidx = np.nonzero(textured)
    rng = np.random.default_rng(seed)
    if len(fidx) > max_train:
        sel = rng.choice(len(fidx), size=max_train, replace=False)
        fidx, bidx = fidx[sel], bidx[sel]
    log(f"step 3/4: training {n_clusters}-entry codebook on {len(fidx)} textured blocks (no percentage for this one)")
    codebook = train_codebook(blocks[fidx, bidx], n_clusters=n_clusters, seed=seed)

    vs = VQSearcher(codebook)
    vq_idx = np.empty((F, nb), dtype=np.uint8)
    d_vq = np.empty((F, nb), dtype=np.float32)
    prog = Progress("matching codebook", F)
    for f in range(F):
        vq_idx[f], d_vq[f] = vs.search(blocks[f])
        prog.update(f + 1)
    prog.done()
    weight = persistence(blocks, cap=persist_cap) if persist_cap > 1 else np.ones((F, nb), dtype=np.float32)
    log(f"analysis done in {time.time() - t0:.1f}s")
    return Prepared(blocks, ana, vq_idx, d_vq, codebook, bh, bw, fps, weight, gray)


def run_pass(P, lam, chunk_cap, label="encoding"):
    """Encodes every frame at this lambda. Returns (frames, stats) where frames
    is a list of per-frame (ops, flat, btc, raw, vq) byte strings."""
    F, nb = P.blocks.shape[:2]
    a = P.ana
    cb = P.codebook
    ref = np.zeros((nb, 16, 3), dtype=np.uint8)
    frames = []
    counts = np.zeros(N_MODES, dtype=np.int64)
    sse = 0.0
    bumped = 0
    frame_budget = chunk_cap - CHUNK_INNER_HDR
    mb = MODE_BYTES_GRAY if P.gray else MODE_BYTES
    prog = Progress(label, F)
    for f in range(F):
        prog.update(f)
        cur = P.blocks[f]
        d_skip = wsse(cur, ref)
        p_col, p_mask, d_paint = paint_candidate(cur, ref)
        w = P.weight[f]
        lf = lam
        while True:
            modes = choose_modes(w, d_skip, a["d_flat"][f], P.d_vq[f], a["d_btc"][f], d_paint,
                                 a["d_pal4"][f], lf, allow_skip=f > 0, mode_bytes=mb)
            m_flat = modes == MODE_FLAT
            m_vq = modes == MODE_VQ
            m_btc = modes == MODE_BTC
            m_raw = modes == MODE_RAW
            m_paint = modes == MODE_PAINT
            m_pal4 = modes == MODE_PAL4
            ops = encode_ops(modes)
            counts_f = np.bincount(modes, minlength=N_MODES)
            size = len(ops) + int((counts_f * mb).sum())
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

        if P.gray:
            flat_bytes = flat_c[:, 0].astype(np.uint8).tobytes()
            rec = np.empty(len(mask), dtype=[("g0", "u1"), ("g1", "u1"), ("m", "<u2")])
            rec["g0"], rec["g1"], rec["m"] = c0[:, 0], c1[:, 0], mask
            btc_bytes = rec.tobytes()
            raw_bytes = raw_b[:, :, 0].astype(np.uint8).tobytes()
        else:
            flat_bytes = pack555(flat_c).astype("<u2").tobytes()
            btc_bytes = np.stack([pack555(c0), pack555(c1), mask], axis=1).astype("<u2").tobytes()
            raw_bytes = pack555(raw_b).astype("<u2").tobytes()
        vq_bytes = vqi.tobytes()
        pc, pm = p_col[m_paint], p_mask[m_paint]
        qc, qi = a["pal4_c"][f][m_pal4], a["pal4_idx"][f][m_pal4]
        if P.gray:
            prec = np.empty(len(pm), dtype=[("g", "u1"), ("m", "<u2")])
            prec["g"], prec["m"] = pc[:, 0], pm
            paint_bytes = prec.tobytes()
            pal4_rec = np.empty((len(qi),), dtype=[("g", "u1", (4,)), ("i", "<u4")])
            pal4_rec["g"] = qc[:, :, 0]
        else:
            paint_bytes = np.stack([pack555(pc), pm], axis=1).astype("<u2").tobytes()
            pal4_rec = np.empty((len(qi),), dtype=[("c", "<u2", (4,)), ("i", "<u4")])
            pal4_rec["c"] = pack555(qc)
        pal4_rec["i"] = qi
        pal4_bytes = pal4_rec.tobytes()
        frames.append((ops, flat_bytes, btc_bytes, raw_bytes, paint_bytes, pal4_bytes, vq_bytes))

        # Reconstruct exactly what the decoder will show.
        ref = ref.copy()
        ref[m_flat] = flat_c[:, None, :]
        ref[m_vq] = cb[vqi]
        ref[m_btc] = btc_reconstruct(c0, c1, mask)
        ref[m_raw] = raw_b
        ref[m_paint] = paint_reconstruct(ref[m_paint], pc, pm)
        ref[m_pal4] = pal4_reconstruct(qc, qi)
        d = cur.astype(np.int32) - ref.astype(np.int32)
        sse += float((d * d).sum())
        counts += np.bincount(modes, minlength=N_MODES)

    prog.done()
    mse = sse / (F * nb * 48)
    psnr = 10 * math.log10(31.0 ** 2 / mse) if mse > 0 else 99.0
    return frames, {"counts": counts, "psnr": psnr, "bumped": bumped}


def build_chunks(frames, chunk_cap, gray=False):
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
        paint = b"".join(fr[4] for fr in cur)
        pal4 = b"".join(fr[5] for fr in cur)
        vq = b"".join(fr[6] for fr in cur)
        sz = (1, 4, 16, 3, 8) if gray else (2, 6, 32, 4, 12)
        hdr = struct.pack("<HHHHHH", len(ops), len(flat) // sz[0], len(btc) // sz[1], len(raw) // sz[2],
                          len(paint) // sz[3], len(pal4) // sz[4])
        chunks.append((len(cur), hdr + ops + flat + btc + raw + paint + pal4 + vq))

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


def compress_chunks(chunks, pool=None, label="compressing"):
    datas = [c[1] for c in chunks]
    prog = Progress(label, len(datas))
    it = pool.imap(_compress_one, datas, chunksize=4) if pool is not None else map(_compress_one, datas)
    results = []
    for r in it:
        results.append(r)
        prog.update(len(results))
    prog.done()
    return [(c[0], c[1], tag, payload) for c, (tag, payload) in zip(chunks, results)]


def file_size(cchunks, n_cb, gray=False):
    total = HEADER_SIZE + n_cb * (16 if gray else 32)
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
    out += MAGIC + struct.pack("<BBHHHIII", VERSION, P.fps, w, h, n_cb, F, max_declen, 1 if P.gray else 0)
    assert len(out) == HEADER_SIZE
    if P.gray:
        out += P.codebook[:, :, 0].astype(np.uint8).tobytes()
    else:
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
    names = ["SKIP", "FLAT", "VQ", "BTC", "RAW", "PAINT", "PAL4"]
    mix = " ".join(f"{n}={c[i] / tot:.1%}" for i, n in enumerate(names))
    log(f"lambda={lam:.3g}: {size} bytes ({size * 8 / (F / fps) / 1000:.1f} kbit/s), "
        f"PSNR {stats['psnr']:.2f} dB, {mix}"
        + (f", {stats['bumped']} frame(s) coarsened to fit chunk cap" if stats["bumped"] else ""))


def encode(video_path, out_path, w=240, h=160, fps=15, n_clusters=256, dither=False,
           lam=None, target_bytes=0, chunk_cap=16384, jobs=0, persist_cap=45, color_mode="auto",
           half_res="auto"):
    if half_res == "auto" and target_bytes and w == 240 and h == 160:
        n_est = probe_frame_count(video_path, fps)
        secs = (n_est / fps) if n_est else 0
        bpps = target_bytes * 8 / secs / (w * h) if secs else 99.0
        half = bpps < HALF_RES_BPPS
        log(f"budget: {bpps:.2f} bits per pixel per second at {w}x{h} -> "
            + ("half resolution (GBA scales it 2x to full screen)" if half else "full resolution"))
    else:
        half = half_res == "on"
    if half:
        w, h = w // 2, h // 2
    P = prepare(video_path, w, h, fps, n_clusters, dither, persist_cap=persist_cap, color_mode=color_mode)
    F = P.blocks.shape[0]
    n_cb = P.codebook.shape[0]

    pool = None
    if jobs != 1:
        import multiprocessing as mp
        pool = mp.Pool(jobs if jobs > 0 else None)

    n_attempt = [0]

    def attempt(l):
        n_attempt[0] += 1
        tag = f"size pass {n_attempt[0]}" if target_bytes else "pass"
        frames, stats = run_pass(P, l, chunk_cap, f"{tag}: encoding")
        cc = compress_chunks(build_chunks(frames, chunk_cap, P.gray), pool, f"{tag}: compressing")
        size = file_size(cc, n_cb, P.gray)
        report(stats, size, F, fps, l)
        return size, cc, stats

    try:
        if not target_bytes:
            l = lam if lam is not None else 200.0
            size, cc, stats = attempt(l)
        else:
            log(f"step 4/4: fitting into {target_bytes} bytes (usually 3-6 passes, 12 at most)")
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
    ap = argparse.ArgumentParser(description="Encode a video to KVC1 (format v3)")
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
    ap.add_argument("--color-mode", choices=["auto", "gray", "color"], default="auto",
                    help="greyscale stores 1-byte grey colours (much smaller); auto detects B&W video")
    ap.add_argument("--half-res", choices=["auto", "on", "off"], default="auto",
                    help="encode at half width/height (the GBA scales it back up); auto picks it "
                         "when --target-bytes is tight for the video's length")
    ap.add_argument("--persist-cap", type=int, default=45,
                    help="weight a block's error by how many frames it stays on screen, up to this "
                         "many (static text/title cards get sharper); 1 disables")
    ap.add_argument("--jobs", type=int, default=0, help="LZ worker processes (0 = all cores)")
    ap.add_argument("--pause-on-error", action="store_true",
                    help="when run in its own window: keep it open on failure, close 5s after success")
    args = ap.parse_args()
    try:
        encode(args.video, args.out, w=args.width, h=args.height, fps=args.fps,
               n_clusters=args.clusters, dither=args.dither, lam=args.lam,
               target_bytes=args.target_bytes, chunk_cap=args.chunk_cap, jobs=args.jobs,
               persist_cap=args.persist_cap, color_mode=args.color_mode, half_res=args.half_res)
    except BaseException:
        if not args.pause_on_error:
            raise
        import traceback
        traceback.print_exc()
        print("\n[kvc1] FAILED -- press Enter to close this window.", flush=True)
        try:
            input()
        except EOFError:
            pass
        sys.exit(1)
    if args.pause_on_error:
        print("[kvc1] done -- this window closes in 5 seconds.", flush=True)
        time.sleep(5)


if __name__ == "__main__":
    main()
