"""GBA BIOS LZ77UnComp-compatible compressor + a Python port of the decoder,
used only to verify round-trip correctness before trusting the real
Compression::LZDecomp()/GBA_LZDECOMP() call (KiNomet/Compression.cpp) to
decode what this produces.

Format (verified against KiNomet/Compression.cpp's own GBA_LZDECOMP, which is
itself the project's existing non-GBA software port of the real BIOS call --
this is not a guess, it's read directly off that function):
  header: u32 LE = 0x10 | (decompressed_length << 8)
  body: repeated groups of [1 flag byte] + up to 8 blocks (MSB of flag first):
    flag bit 0 -> 1 literal byte
    flag bit 1 -> 2-byte back-reference: byte0 = ((length-3)<<4) | ((disp-1)>>8),
                  byte1 = (disp-1) & 0xFF  (so length in 3..18, disp in 1..4096)
"""
import struct


def decompress_reference(data: bytes) -> bytes:
    """Python port of Compression.cpp's GBA_LZDECOMP, for round-trip testing only."""
    header = struct.unpack_from("<I", data, 0)[0]
    magic = header & 0xFF
    assert magic == 0x10, f"bad magic {magic:#x}"
    declen = header >> 8
    out = bytearray()
    pos = 4
    while len(out) < declen:
        flags = data[pos]; pos += 1
        for i in range(8):
            if len(out) >= declen:
                break
            is_ref = bool(flags & (0x80 >> i))
            if not is_ref:
                out.append(data[pos]); pos += 1
            else:
                b0 = data[pos]; b1 = data[pos + 1]; pos += 2
                length = (b0 >> 4) + 3
                disp = ((b0 & 0xF) << 8 | b1) + 1
                for _ in range(length):
                    out.append(out[len(out) - disp])
    return bytes(out)


def compress(data: bytes, min_match=3, max_match=18, max_disp=4096) -> bytes:
    """Greedy LZ77 encoder producing the format above. Not optimal, just
    correct -- a hash-chain-based longest-match search, good enough for this
    content (long flat/skip runs compress very well regardless of optimality)."""
    n = len(data)
    out = bytearray()
    out += struct.pack("<I", 0x10 | (n << 8))

    # hash of 3-byte prefixes -> list of positions, for fast candidate lookup
    chain = {}

    def hash3(p):
        return data[p] | (data[p + 1] << 8) | (data[p + 2] << 16)

    positions = {}  # hash -> list of recent positions (most recent first, capped)

    pos = 0
    pending_flag_byte_idx = None
    flag_bits = []
    blocks = []

    def flush_group():
        nonlocal flag_bits, blocks
        if not flag_bits:
            return
        flag = 0
        for i, b in enumerate(flag_bits):
            if b:
                flag |= (0x80 >> i)
        out.append(flag)
        for blk in blocks:
            out.extend(blk)
        flag_bits = []
        blocks = []

    while pos < n:
        best_len = 0
        best_disp = 0
        if pos + min_match <= n:
            h = hash3(pos)
            cand_list = positions.get(h)
            if cand_list:
                lo = max(0, pos - max_disp)
                tries = 0
                for cand in reversed(cand_list):
                    if cand < lo:
                        break
                    tries += 1
                    if tries > 32:
                        break
                    max_l = min(max_match, n - pos)
                    l = 0
                    while l < max_l and data[cand + l] == data[pos + l]:
                        l += 1
                    if l >= min_match and l > best_len:
                        best_len = l
                        best_disp = pos - cand
                        if l == max_match:
                            break

        if best_len >= min_match:
            b0 = ((best_len - 3) << 4) | (((best_disp - 1) >> 8) & 0xF)
            b1 = (best_disp - 1) & 0xFF
            flag_bits.append(1)
            blocks.append(bytes([b0, b1]))
            # register hash entries for the consumed span (sparse: every position,
            # capped list length to keep this fast)
            end = pos + best_len
            while pos < end and pos + 3 <= n:
                h2 = hash3(pos)
                lst = positions.setdefault(h2, [])
                lst.append(pos)
                if len(lst) > 64:
                    del lst[0]
                pos += 1
            pos = end
        else:
            flag_bits.append(0)
            blocks.append(bytes([data[pos]]))
            if pos + 3 <= n:
                h2 = hash3(pos)
                lst = positions.setdefault(h2, [])
                lst.append(pos)
                if len(lst) > 64:
                    del lst[0]
            pos += 1

        if len(flag_bits) == 8:
            flush_group()

    flush_group()
    return bytes(out)


if __name__ == "__main__":
    import random
    random.seed(0)
    tests = [
        b"",
        b"A",
        b"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        bytes(range(256)) * 4,
        bytes([0, 0, 0, 1, 2, 3] * 1000),
        bytes(random.randrange(256) for _ in range(50000)),
    ]
    for i, t in enumerate(tests):
        c = compress(t)
        d = decompress_reference(c)
        ok = d == t
        ratio = (len(t) / len(c)) if c else 0
        print(f"test {i}: len={len(t)} complen={len(c)} ratio={ratio:.2f} match={ok}")
        assert ok, f"ROUND TRIP FAILED on test {i}"
    print("all round-trip tests passed")
