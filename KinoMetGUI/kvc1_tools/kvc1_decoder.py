#!/usr/bin/env python3
"""Python decoder for the KVC1-C container (see kvc1_encoder.py's module
docstring for the exact file format). This is a software/preview decoder
for producing a demo render in this sandbox -- it is NOT a substitute for
the real C decoder (KiNomet/kvc1_decode.c, kvc1_load.cpp) that actually
ships on GBA/Windows, it's just enough to reconstruct frames for a look at
what that decoder would actually output.
"""
import struct
import numpy as np
import gba_lz77

MAGIC = b"KVC1"
BLOCK = 4
COLOR_RGB555 = 0
COLOR_GRAY_PALETTE = 1


def blocks_to_frame(blocks, bh, bw):
    """Inverse of kvc1_codec.frame_to_blocks. blocks: (bh*bw,16,3) uint8."""
    b = blocks.reshape(bh, bw, BLOCK, BLOCK, 3).transpose(0, 2, 1, 3, 4)
    return b.reshape(bh * BLOCK, bw * BLOCK, 3)


def blocks_to_frame_mono(blocks, bh, bw):
    """Inverse of kvc1_codec.frame_to_blocks_mono. blocks: (bh*bw,16) uint8."""
    b = blocks.reshape(bh, bw, BLOCK, BLOCK).transpose(0, 2, 1, 3)
    return b.reshape(bh * BLOCK, bw * BLOCK)


def rle_decode_modes(stream, offset, nblocks):
    """Inverse of kvc1_encoder.rle_encode_modes for exactly one frame's worth
    of blocks. Reads run-length tokens (kind<<5)|(run-1) starting at
    `stream[offset:]` until `nblocks` block-modes have been produced (runs
    never cross a frame boundary -- see kvc1_encoder.py's module docstring).
    Returns (modes_array, new_offset)."""
    out = np.empty(nblocks, dtype=np.uint8)
    pos = 0
    off = offset
    while pos < nblocks:
        b = stream[off]
        off += 1
        kind = b >> 5
        run = (b & 0x1F) + 1
        run = min(run, nblocks - pos)
        out[pos:pos + run] = kind
        pos += run
    return out, off


def rgb555_to_888(frame555):
    """Bit-replication expansion (0..31 -> 0..255), matching the usual
    GBA RGB555->RGB888 display conversion."""
    v = frame555.astype(np.uint16)
    v8 = (v << 3) | (v >> 2)
    return np.clip(v8, 0, 255).astype(np.uint8)


def decode_kvc1(path):
    """Returns (frames, fps, width, height). `frames` is always (N,H,W,3)
    uint8 RGB555-domain (0..31 per channel) -- for a color_mode==1 (gray
    palette) file, the palette lookup is applied here so callers never need
    to care which path produced the data."""
    with open(path, "rb") as f:
        data = f.read()

    assert data[0:4] == MAGIC, f"bad magic {data[0:4]!r}"
    version = data[4]
    width, height = struct.unpack_from("<HH", data, 5)
    fps = data[9]
    gop = data[10]
    n_clusters = struct.unpack_from("<H", data, 11)[0]
    n_frames = struct.unpack_from("<I", data, 13)[0]
    pos = 17

    color_mode = COLOR_RGB555
    palette31 = None
    if version >= 2:
        color_mode = data[pos]
        pos += 1
        if color_mode == COLOR_GRAY_PALETTE:
            palette_len = data[pos]
            pos += 1
            palette31 = np.frombuffer(data, dtype=np.uint8, count=palette_len, offset=pos).copy()
            pos += palette_len

    codebook_len = struct.unpack_from("<I", data, pos)[0]
    pos += 4
    codebook_bytes = data[pos:pos + codebook_len]
    pos += codebook_len
    if color_mode == COLOR_GRAY_PALETTE:
        n_cb = codebook_len // 16
        codebook = np.frombuffer(codebook_bytes, dtype=np.uint8).reshape(n_cb, 16)
    else:
        n_cb = codebook_len // 48
        codebook = np.frombuffer(codebook_bytes, dtype=np.uint8).reshape(n_cb, 16, 3)

    mode_name = "gray-palette" if color_mode == COLOR_GRAY_PALETTE else "RGB555"
    print(f"[kvc1-decode] {path}: v{version} {mode_name}, {width}x{height}@{fps}fps, gop={gop}, "
          f"{n_clusters} clusters ({n_cb} actual), {n_frames} frames"
          + (f", {len(palette31)}-level palette" if palette31 is not None else ""))

    bh, bw = height // BLOCK, width // BLOCK
    nblocks = bh * bw

    frames555 = np.zeros((n_frames, height, width, 3), dtype=np.uint8)
    mode_hist = {0: 0, 1: 0, 2: 0, 3: 0}

    if color_mode == COLOR_GRAY_PALETTE:
        prev_blocks = np.zeros((nblocks, BLOCK * BLOCK), dtype=np.uint8)
    else:
        prev_blocks = np.zeros((nblocks, BLOCK * BLOCK, 3), dtype=np.uint8)

    frame_idx = 0
    while frame_idx < n_frames:
        chunk_frames = min(gop, n_frames - frame_idx)

        streams = []
        for _ in range(4):
            tag, comp_len, dec_len = struct.unpack_from("<BII", data, pos)
            pos += 9
            payload = data[pos:pos + comp_len]
            pos += comp_len
            if tag == 1:
                stream = gba_lz77.decompress_reference(payload)
            else:
                stream = payload
            assert len(stream) == dec_len, (len(stream), dec_len)
            streams.append(stream)
        mode_stream, flat_stream, vq_stream, raw_stream = streams

        mode_off = flat_off = vq_off = raw_off = 0
        for fi in range(chunk_frames):
            modes, mode_off = rle_decode_modes(mode_stream, mode_off, nblocks)
            for m in modes:
                mode_hist[int(m)] += 1

            cur_blocks = prev_blocks.copy()  # SKIP: inherit previous frame's pixels

            flat_pos = np.nonzero(modes == 1)[0]
            n_flat = len(flat_pos)
            if n_flat:
                if color_mode == COLOR_GRAY_PALETTE:
                    flat_vals = np.frombuffer(flat_stream, dtype=np.uint8, count=n_flat, offset=flat_off)
                    flat_off += n_flat
                    cur_blocks[flat_pos] = flat_vals[:, None]
                else:
                    flat_vals = np.frombuffer(flat_stream, dtype=np.uint8, count=n_flat * 3, offset=flat_off).reshape(n_flat, 3)
                    flat_off += n_flat * 3
                    cur_blocks[flat_pos] = flat_vals[:, None, :]

            vq_pos = np.nonzero(modes == 2)[0]
            n_vq = len(vq_pos)
            if n_vq:
                vq_codes = np.frombuffer(vq_stream, dtype=np.uint8, count=n_vq, offset=vq_off)
                vq_off += n_vq
                cur_blocks[vq_pos] = codebook[vq_codes]

            raw_pos = np.nonzero(modes == 3)[0]
            n_raw = len(raw_pos)
            if n_raw:
                if color_mode == COLOR_GRAY_PALETTE:
                    raw_vals = np.frombuffer(raw_stream, dtype=np.uint8, count=n_raw * 16, offset=raw_off).reshape(n_raw, 16)
                    raw_off += n_raw * 16
                else:
                    raw_vals = np.frombuffer(raw_stream, dtype=np.uint8, count=n_raw * 48, offset=raw_off).reshape(n_raw, 16, 3)
                    raw_off += n_raw * 48
                cur_blocks[raw_pos] = raw_vals

            if color_mode == COLOR_GRAY_PALETTE:
                idx_frame = blocks_to_frame_mono(cur_blocks, bh, bw)
                gray31 = palette31[idx_frame]
                frames555[frame_idx + fi] = gray31[:, :, None]
            else:
                frames555[frame_idx + fi] = blocks_to_frame(cur_blocks, bh, bw)
            prev_blocks = cur_blocks

        frame_idx += chunk_frames

    total = sum(mode_hist.values())
    print(f"[kvc1-decode] mode mix while decoding: SKIP={mode_hist[0]/total:.1%} "
          f"FLAT={mode_hist[1]/total:.1%} VQ={mode_hist[2]/total:.1%} RAW={mode_hist[3]/total:.1%}")

    return frames555, fps, width, height


if __name__ == "__main__":
    import sys
    frames, fps, w, h = decode_kvc1(sys.argv[1])
    print(f"decoded {frames.shape[0]} frames @ {w}x{h} {fps}fps")
