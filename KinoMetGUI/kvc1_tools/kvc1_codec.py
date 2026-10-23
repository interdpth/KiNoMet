"""
KVC1 (version 2) codec core: frame analysis, codebook training and
rate-distortion mode selection.

Block model: the frame is divided into 4x4-pixel blocks (240x160 -> 60x40 =
2400 blocks). Each block, each frame, is coded one of five ways:
  SKIP: unchanged from what is already on screen (0 bytes).
  FLAT: one RGB555 colour for all 16 pixels (2 bytes).
  VQ:   index into a whole-video codebook of 4x4 blocks (1 byte).
  BTC:  two RGB555 colours + a 16-bit pixel mask (6 bytes) -- sharp edges
        and text for very little data.
  RAW:  16 explicit RGB555 pixels (32 bytes).

The mode is picked per block by minimising  D + lambda * R  (distortion plus
lambda times bytes). One lambda for the whole video gives roughly constant
quality; the encoder searches lambda to hit a byte budget.

SKIP is judged against the *reconstructed* previous frame (what the decoder
actually has on screen), so errors never accumulate.

All colour math is in RGB555 space (0..31 per channel), the GBA's native
format.
"""
import numpy as np
from sklearn.cluster import MiniBatchKMeans

BLOCK = 4

MODE_SKIP, MODE_FLAT, MODE_VQ, MODE_BTC, MODE_RAW = 0, 1, 2, 3, 4
MODE_BYTES = np.array([0, 2, 1, 6, 32], dtype=np.float32)

# Per-channel weights for the distortion measure (R, G, B): the eye is most
# sensitive to green, least to blue.
CH_W = np.array([1.0, 2.0, 0.6], dtype=np.float32)
_SQRT_W = np.sqrt(CH_W)


def frame_to_blocks(frame555):
    """frame555: (H,W,3) uint8, values 0..31. Returns (nblocks,16,3), bh, bw."""
    h, w, _ = frame555.shape
    bh, bw = h // BLOCK, w // BLOCK
    b = frame555[:bh * BLOCK, :bw * BLOCK].reshape(bh, BLOCK, bw, BLOCK, 3).transpose(0, 2, 1, 3, 4)
    return b.reshape(bh * bw, BLOCK * BLOCK, 3), bh, bw


def blocks_to_frame(blocks, bh, bw):
    """Inverse of frame_to_blocks. blocks: (nblocks,16,C)."""
    c = blocks.shape[-1]
    return blocks.reshape(bh, bw, BLOCK, BLOCK, c).transpose(0, 2, 1, 3, 4).reshape(bh * BLOCK, bw * BLOCK, c)


_BAYER4 = np.array([
    [0, 8, 2, 10],
    [12, 4, 14, 6],
    [3, 11, 1, 9],
    [15, 7, 13, 5],
], dtype=np.float32) / 16.0 - 0.5


def rgb888_to_555(frame888, dither=False):
    """frame888: (H,W,3) uint8. Returns (H,W,3) uint8 in 0..31 (rounded, or
    ordered-dithered when dither=True). Dithering is off by default: its
    noise defeats SKIP/FLAT and costs a lot of bytes at low bitrates."""
    f = frame888.astype(np.float32)
    if dither:
        h, w, _ = frame888.shape
        tile = np.tile(_BAYER4, ((h + 3) // 4, (w + 3) // 4))[:h, :w] * 8.0
        f = f + tile[:, :, None]
        return (np.clip(f, 0, 255).astype(np.uint8) >> 3).astype(np.uint8)
    return np.clip(np.rint(f * (31.0 / 255.0)), 0, 31).astype(np.uint8)


def pack555(rgb):
    """rgb: (...,3) integer 0..31 -> (...) uint16 GBA colour (R low bits)."""
    rgb = rgb.astype(np.uint16)
    return rgb[..., 0] | (rgb[..., 1] << 5) | (rgb[..., 2] << 10)


def unpack555(v):
    v = v.astype(np.uint16)
    return np.stack([v & 31, (v >> 5) & 31, (v >> 10) & 31], axis=-1).astype(np.uint8)


def wsse(a, b):
    """Weighted squared error per block. a,b: (nb,16,3) numeric. Returns (nb,) float32."""
    d = a.astype(np.float32) - b.astype(np.float32)
    return ((d * d) * CH_W).sum(axis=(1, 2))


def analyze_blocks(blocks):
    """Everything about a frame's blocks that doesn't depend on the previous
    frame: FLAT and BTC candidates and their distortions.
    blocks: (nb,16,3) uint8. Returns dict of arrays."""
    x = blocks.astype(np.float32)
    nb = x.shape[0]

    flat_c = np.clip(np.rint(x.mean(axis=1)), 0, 31).astype(np.uint8)          # (nb,3)
    d_flat = wsse(x, flat_c[:, None, :].astype(np.float32))

    # BTC: split pixels into two groups (start at the luma mean, then two
    # rounds of 2-means in weighted RGB), one colour per group.
    lum = x @ np.array([0.3, 0.59, 0.11], dtype=np.float32)                    # (nb,16)
    m = lum > lum.mean(axis=1, keepdims=True)
    for _ in range(3):
        n1 = m.sum(axis=1, keepdims=True).astype(np.float32)
        n0 = 16.0 - n1
        c1 = (x * m[:, :, None]).sum(axis=1) / np.maximum(n1, 1.0)
        c0 = (x * ~m[:, :, None]).sum(axis=1) / np.maximum(n0, 1.0)
        c1 = np.where(n1 > 0, c1, c0)
        c0 = np.where(n0 > 0, c0, c1)
        c0q = np.clip(np.rint(c0), 0, 31)
        c1q = np.clip(np.rint(c1), 0, 31)
        e0 = (((x - c0q[:, None, :]) ** 2) * CH_W).sum(axis=2)
        e1 = (((x - c1q[:, None, :]) ** 2) * CH_W).sum(axis=2)
        m = e1 < e0
    n1 = m.sum(axis=1, keepdims=True).astype(np.float32)
    n0 = 16.0 - n1
    c1 = (x * m[:, :, None]).sum(axis=1) / np.maximum(n1, 1.0)
    c0 = (x * ~m[:, :, None]).sum(axis=1) / np.maximum(n0, 1.0)
    c1 = np.where(n1 > 0, c1, c0)
    c0 = np.where(n0 > 0, c0, c1)
    btc_c0 = np.clip(np.rint(c0), 0, 31).astype(np.uint8)
    btc_c1 = np.clip(np.rint(c1), 0, 31).astype(np.uint8)
    btc_rec = np.where(m[:, :, None], btc_c1[:, None, :], btc_c0[:, None, :])
    d_btc = wsse(x, btc_rec)
    btc_mask = (m.astype(np.uint32) << np.arange(16, dtype=np.uint32)).sum(axis=1).astype(np.uint16)

    return {
        "flat_c": flat_c, "d_flat": d_flat,
        "btc_c0": btc_c0, "btc_c1": btc_c1, "btc_mask": btc_mask, "d_btc": d_btc,
    }


def btc_reconstruct(c0, c1, mask):
    """c0,c1: (k,3) uint8, mask: (k,) uint16 -> (k,16,3) uint8."""
    bits = ((mask[:, None].astype(np.uint32) >> np.arange(16, dtype=np.uint32)) & 1).astype(bool)
    return np.where(bits[:, :, None], c1[:, None, :], c0[:, None, :]).astype(np.uint8)


def train_codebook(sample_blocks, n_clusters=256, seed=0):
    """sample_blocks: (N,16,3) uint8 drawn from across the video (textured
    blocks only -- FLAT/BTC already cover flat and two-tone areas).
    Clusters in channel-weighted space. Returns (n_clusters,16,3) uint8."""
    n = sample_blocks.shape[0]
    if n == 0:
        return np.zeros((n_clusters, 16, 3), dtype=np.uint8)
    k = min(n_clusters, n)
    feats = (sample_blocks.astype(np.float32) * _SQRT_W).reshape(n, -1)
    km = MiniBatchKMeans(n_clusters=k, random_state=seed, batch_size=max(2048, k * 16),
                         n_init=3, max_iter=100)
    km.fit(feats)
    centers = (km.cluster_centers_.reshape(k, 16, 3) / _SQRT_W)
    cb = np.clip(np.rint(centers), 0, 31).astype(np.uint8)
    if k < n_clusters:
        cb = np.concatenate([cb, np.repeat(cb[:1], n_clusters - k, axis=0)], axis=0)
    return cb


class VQSearcher:
    """Nearest codebook entry (weighted squared error) for many blocks at once."""

    def __init__(self, codebook):
        self.cb = codebook
        c = (codebook.astype(np.float32) * _SQRT_W).reshape(codebook.shape[0], -1)
        self.c = c
        self.c_norm = (c * c).sum(axis=1)

    def search(self, blocks):
        x = (blocks.astype(np.float32) * _SQRT_W).reshape(blocks.shape[0], -1)
        d2 = (x * x).sum(axis=1)[:, None] - 2.0 * (x @ self.c.T) + self.c_norm[None, :]
        idx = d2.argmin(axis=1)
        best = np.maximum(d2[np.arange(len(idx)), idx], 0.0)
        return idx.astype(np.uint8), best.astype(np.float32)


def choose_modes(d_skip, d_flat, d_vq, d_btc, lam, allow_skip=True):
    """Rate-distortion choice per block. Returns (nb,) uint8 modes."""
    J = np.stack([
        d_skip if allow_skip else np.full_like(d_flat, np.inf),
        d_flat + lam * MODE_BYTES[MODE_FLAT],
        d_vq + lam * MODE_BYTES[MODE_VQ],
        d_btc + lam * MODE_BYTES[MODE_BTC],
        np.full_like(d_flat, lam * MODE_BYTES[MODE_RAW]),
    ], axis=1)
    return J.argmin(axis=1).astype(np.uint8)
