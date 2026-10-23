#!/usr/bin/env python3
"""
Reference decoder for KVC1 v2 files (format: see kvc1_encoder.py). Reads only
the file -- shares nothing with the encoder except the LZ77 port -- so it
checks the bitstream, not the encoder's own bookkeeping.

  python kvc1_decode_ref.py VideoFile.bin --preview out.mp4 [--audio src.mp4]
  python kvc1_decode_ref.py VideoFile.bin --dump frames.u16   (raw u16 frames, for C comparison)
"""
import argparse
import os
import struct
import subprocess
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gba_lz77  # noqa: E402


def lz_decompress(data):
    """Same algorithm as gba_lz77.decompress_reference, faster for big inputs."""
    header = struct.unpack_from("<I", data, 0)[0]
    assert header & 0xFF == 0x10, "bad LZ77 magic"
    declen = header >> 8
    out = bytearray(declen)
    w = 0
    pos = 4
    while w < declen:
        flags = data[pos]
        pos += 1
        for i in range(8):
            if w >= declen:
                break
            if not flags & (0x80 >> i):
                out[w] = data[pos]
                pos += 1
                w += 1
            else:
                b0, b1 = data[pos], data[pos + 1]
                pos += 2
                length = (b0 >> 4) + 3
                disp = ((b0 & 0xF) << 8 | b1) + 1
                for _ in range(length):
                    out[w] = out[w - disp]
                    w += 1
    return bytes(out)


def decode(path):
    """Yields each frame as an (h,w) uint16 RGB555 array (copies)."""
    data = open(path, "rb").read()
    assert data[:4] == b"KVC1", "bad magic"
    ver, fps, w, h, n_cb, n_frames, max_declen, _ = struct.unpack_from("<BBHHHIII", data, 4)
    assert ver == 2, f"unsupported version {ver}"
    pos = 24
    cb = np.frombuffer(data, dtype="<u2", count=n_cb * 16, offset=pos).reshape(n_cb, 16)
    pos += n_cb * 32
    bw, bh = w // 4, h // 4
    nb = bw * bh
    fb = np.zeros((bh, bw, 16), dtype=np.uint16)  # block-major framebuffer
    flatfb = fb.reshape(nb, 16)
    done = 0
    while done < n_frames:
        tag, nfr, _, clen, dlen = struct.unpack_from("<BBHII", data, pos)
        pos += 12
        payload = data[pos:pos + clen]
        pos += (clen + 3) & ~3
        dec = lz_decompress(payload) if tag == 1 else payload
        assert len(dec) == dlen <= max_declen, "chunk size mismatch"
        ops_len, n_flat, n_btc, n_raw = struct.unpack_from("<HHHH", dec, 0)
        p = 8
        ops = dec[p:p + ops_len]; p += ops_len
        flat = np.frombuffer(dec, dtype="<u2", count=n_flat, offset=p); p += 2 * n_flat
        btc = np.frombuffer(dec, dtype="<u2", count=3 * n_btc, offset=p).reshape(n_btc, 3); p += 6 * n_btc
        raw = np.frombuffer(dec, dtype="<u2", count=16 * n_raw, offset=p).reshape(n_raw, 16); p += 32 * n_raw
        vq = np.frombuffer(dec, dtype=np.uint8, offset=p)
        oi = fi = bi = ri = vi = 0
        bits = np.arange(16, dtype=np.uint16)
        for _ in range(nfr):
            b = 0
            while b < nb:
                op = ops[oi]; oi += 1
                if op < 0x80:
                    b += op + 1
                    continue
                n = (op & 31) + 1
                kind = op >> 5
                if kind == 4:      # FLAT
                    flatfb[b:b + n] = flat[fi:fi + n, None]; fi += n
                elif kind == 5:    # VQ
                    flatfb[b:b + n] = cb[vq[vi:vi + n]]; vi += n
                elif kind == 6:    # BTC
                    e = btc[bi:bi + n]; bi += n
                    m = ((e[:, 2:3] >> bits) & 1).astype(bool)
                    flatfb[b:b + n] = np.where(m, e[:, 1:2], e[:, 0:1])
                else:              # RAW
                    flatfb[b:b + n] = raw[ri:ri + n]; ri += n
                b += n
            assert b == nb, "op run crossed a frame boundary"
            yield fb.reshape(bh, bw, 4, 4).transpose(0, 2, 1, 3).reshape(h, w).copy()
            done += 1
        assert oi == len(ops) and fi == len(flat) and bi == len(btc) and ri == len(raw) and vi == len(vq), \
            "chunk has leftover data"
    assert pos == len(data), f"trailing bytes in file ({len(data) - pos})"


def header(path):
    data = open(path, "rb").read(24)
    ver, fps, w, h, n_cb, n_frames, max_declen, _ = struct.unpack_from("<BBHHHIII", data, 4)
    return {"fps": fps, "w": w, "h": h, "n_cb": n_cb, "n_frames": n_frames, "max_declen": max_declen}


def to_rgb888(fr):
    r = (fr & 31).astype(np.uint8)
    g = ((fr >> 5) & 31).astype(np.uint8)
    b = ((fr >> 10) & 31).astype(np.uint8)
    rgb = np.stack([r, g, b], axis=-1)
    return (rgb << 3) | (rgb >> 2)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("kvc1")
    ap.add_argument("--preview", help="write an mp4 of the decoded video")
    ap.add_argument("--audio", help="audio source to mux into the preview")
    ap.add_argument("--audio-rate", type=int, default=10512)
    ap.add_argument("--scale", type=int, default=3)
    ap.add_argument("--dump", help="write raw u16 frames here")
    args = ap.parse_args()
    hd = header(args.kvc1)
    proc = None
    if args.preview:
        cmd = ["ffmpeg", "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24",
               "-s", f"{hd['w']}x{hd['h']}", "-r", str(hd["fps"]), "-i", "-"]
        if args.audio:
            cmd += ["-i", args.audio, "-map", "0:v", "-map", "1:a",
                    "-af", f"aresample={args.audio_rate},aformat=sample_fmts=u8:channel_layouts=mono",
                    "-c:a", "aac", "-b:a", "96k", "-shortest"]
        cmd += ["-vf", f"scale=iw*{args.scale}:ih*{args.scale}:flags=neighbor",
                "-c:v", "libx264", "-crf", "14", "-pix_fmt", "yuv420p", args.preview]
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    dump = open(args.dump, "wb") if args.dump else None
    n = 0
    for fr in decode(args.kvc1):
        if proc:
            proc.stdin.write(to_rgb888(fr).tobytes())
        if dump:
            dump.write(fr.astype("<u2").tobytes())
        n += 1
    if proc:
        proc.stdin.close()
        proc.wait()
    if dump:
        dump.close()
    print(f"decoded {n} frames OK")


if __name__ == "__main__":
    main()
