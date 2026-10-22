"""
KVC1-C codec core: codebook training and per-frame block classification.

Block model: the 240x160 frame is divided into 4x4-pixel blocks (60x40 = 2400
blocks). Each block is coded one of four ways, chosen per-block per-frame by
comparing against fixed thresholds (see classify_frame):
  SKIP (0): identical enough to the same block in the previous frame -> costs
            nothing, the decoder just leaves that region of VRAM untouched.
  FLAT (1): low internal variance -> one averaged RGB555 colour for all 16
            pixels (3 bytes).
  VQ   (2): matched against a trained, whole-video codebook entry (1 byte:
            the codebook index).
  RAW  (3): fallback for blocks that are neither flat nor a good codebook
            match -> all 16 pixels stored verbatim (48 bytes).

All color math happens in RGB555 space (0..31 per channel) from the start,
since that's the GBA's native 15-bit color -- quantizing at the very end
would throw away the headroom ordered dithering needs.
"""
import numpy as np
from sklearn.cluster import MiniBatchKMeans

BLOCK = 4


def frame_to_blocks(frame555):
    """frame555: (H,W,3) uint8 array, values 0..31. Returns (nblocks,16,3)."""
    h, w, _ = frame555.shape
    bh, bw = h // BLOCK, w // BLOCK
    b = frame555.reshape(bh, BLOCK, bw, BLOCK, 3).transpose(0, 2, 1, 3, 4)
    return b.reshape(bh * bw, BLOCK * BLOCK, 3), bh, bw


_BAYER4 = np.array([
    [0, 8, 2, 10],
    [12, 4, 14, 6],
    [3, 11, 1, 9],
    [15, 7, 13, 5],
], dtype=np.float32) / 16.0 - 0.5  # centered ordered-dither offsets, range [-0.5, 0.5)


def dither_offsets(h, w):
    """Tiles the 4x4 Bayer matrix across an (h,w) frame, scaled to roughly one
    quantization step (8 levels per RGB555 step in 0..255 space)."""
    ty = (h + 3) // 4
    tx = (w + 3) // 4
    tile = np.tile(_BAYER4, (ty, tx))[:h, :w]
    return tile * 8.0  # one 8-bit LSB step at the >>3 truncation point


def rgb888_to_555_dithered(frame888, off):
    """frame888: (H,W,3) uint8. off: (H,W) dither offsets. Returns (H,W,3) uint8 0..31."""
    f = frame888.astype(np.float32) + off[:, :, None]
    f = np.clip(f, 0, 255)
    return (f.astype(np.uint8) >> 3).astype(np.uint8)


def train_codebook(all_blocks555, n_clusters=256, n_reserved_mono=2, seed=0,
                    mono_oversample_frac=0.08):
    """all_blocks555: (N,16,3) uint8 sample of blocks (0..31 per channel) drawn
    from across the whole video. Returns (n_clusters,16,3) uint8 codebook.

    The first n_reserved_mono entries are hardcoded pure black/white (gray)
    rather than trained, and near-gray blocks in the training sample are
    oversampled before fitting the rest -- without this, a 256-entry k-means
    codebook trained on mostly-colourful game footage tends not to allocate
    any cluster near pure gray, so grayscale UI/text elements get matched to
    the nearest *available* cluster, which can have a visible colour tint
    even though its average brightness is right. That showed up as visible
    color fringing on narration text in earlier testing of this exact design
    and is why this reservation/oversampling exists at all, not just as a
    generic quality nicety.
    """
    n = all_blocks555.shape[0]
    flat = all_blocks555.reshape(n, -1).astype(np.float32)

    chan_max = all_blocks555.max(axis=1).astype(np.int16)
    chan_min = all_blocks555.min(axis=1).astype(np.int16)
    chan_spread = (chan_max - chan_min).max(axis=1)  # (N,) -- per-block max channel spread
    is_gray = chan_spread < 2.0

    gray_idx = np.nonzero(is_gray)[0]
    target_gray_n = int(n * mono_oversample_frac)
    if len(gray_idx) > 0 and len(gray_idx) < target_gray_n:
        reps = int(np.ceil(target_gray_n / len(gray_idx)))
        oversampled = np.tile(flat[gray_idx], (reps, 1))[:target_gray_n]
        flat = np.concatenate([flat, oversampled], axis=0)

    n_train_clusters = max(1, n_clusters - n_reserved_mono)
    km = MiniBatchKMeans(n_clusters=n_train_clusters, random_state=seed,
                          batch_size=max(1024, n_train_clusters * 8),
                          n_init=3, max_iter=200)
    km.fit(flat)
    trained = np.clip(np.round(km.cluster_centers_), 0, 31).astype(np.uint8).reshape(n_train_clusters, 16, 3)

    reserved = np.zeros((n_reserved_mono, 16, 3), dtype=np.uint8)
    if n_reserved_mono >= 1:
        reserved[0, :, :] = 0
    if n_reserved_mono >= 2:
        reserved[1, :, :] = 31

    return np.concatenate([reserved, trained], axis=0)


class FrameClassifier:
    """Holds the codebook and precomputed per-entry stats, classifies/encodes
    one frame at a time against the previous frame's RGB555 pixels."""

    def __init__(self, codebook, skip_t=30, flat_t=300, vq_t=3000):
        self.codebook = codebook  # (C,16,3) uint8
        self.n_clusters = codebook.shape[0]
        self.skip_t = skip_t
        self.flat_t = flat_t
        self.vq_t = vq_t

        cb_flat = codebook.reshape(self.n_clusters, -1).astype(np.float32)
        self._cb_flat = cb_flat
        cb_max = codebook.max(axis=1).astype(np.int16)
        cb_min = codebook.min(axis=1).astype(np.int16)
        self._cb_is_gray = (cb_max - cb_min).max(axis=1) < 0.5

    def encode_frame(self, cur_blocks, prev_blocks):
        """cur_blocks, prev_blocks: (nblocks,16,3) uint8 (prev_blocks may be
        None for the first frame, forcing non-SKIP everywhere).
        Returns: modes (nblocks,) uint8, flat_vals list of (3,) uint8 triples,
        vq_idx list of ints, raw_blocks list of (16,3) uint8 arrays -- in
        block-scan order, consumed by the caller in that same order.
        """
        nblocks = cur_blocks.shape[0]
        cur_f = cur_blocks.astype(np.int16)

        if prev_blocks is not None:
            diff = np.abs(cur_f - prev_blocks.astype(np.int16)).sum(axis=(1, 2))
            is_skip = diff < self.skip_t
        else:
            is_skip = np.zeros(nblocks, dtype=bool)

        mean = cur_f.mean(axis=1)  # (nblocks,3)
        var = ((cur_f - mean[:, None, :]) ** 2).sum(axis=(1, 2))
        is_flat = (~is_skip) & (var < self.flat_t)

        need_vq = ~(is_skip | is_flat)
        block_max = cur_blocks.max(axis=1).astype(np.int16)
        block_min = cur_blocks.min(axis=1).astype(np.int16)
        block_chan_spread = (block_max - block_min).max(axis=1)
        block_is_gray = block_chan_spread < 2.0

        modes = np.zeros(nblocks, dtype=np.uint8)
        modes[is_flat] = 1

        flat_vals = np.clip(np.round(mean[is_flat]), 0, 31).astype(np.uint8)

        vq_candidates = np.nonzero(need_vq)[0]
        vq_idx_out = np.zeros(0, dtype=np.int32)
        is_vq = np.zeros(nblocks, dtype=bool)
        if len(vq_candidates) > 0:
            cand_flat = cur_f[vq_candidates].reshape(len(vq_candidates), -1).astype(np.float32)
            # squared L2 distance to every codebook entry
            d2 = ((cand_flat[:, None, :] - self._cb_flat[None, :, :]) ** 2).sum(axis=2)
            disallow = block_is_gray[vq_candidates][:, None] & (~self._cb_is_gray[None, :])
            d2 = np.where(disallow, np.inf, d2)
            best_idx = d2.argmin(axis=1)
            best_d2 = d2[np.arange(len(vq_candidates)), best_idx]
            good = best_d2 < self.vq_t
            vq_block_positions = vq_candidates[good]
            is_vq[vq_block_positions] = True
            modes[vq_block_positions] = 2
            vq_idx_out = best_idx[good].astype(np.int32)

        is_raw = need_vq & (~is_vq)
        modes[is_raw] = 3
        raw_blocks = cur_blocks[is_raw]

        return modes, flat_vals, vq_idx_out, raw_blocks
