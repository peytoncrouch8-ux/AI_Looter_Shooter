"""Measuring sounds, since they can't be listened to here: levels and loudness, spectrum balance, DC, clipping, how
they start and end, and loop seams.

Loudness follows ITU-R BS.1770's K-weighting (its 48 kHz filter coefficients); "L200" is the loudest 200 ms window,
which tracks how loud a short sound seems better than the 400 ms momentary window does.
"""
import numpy as np

from .core import SR, a2db, length
from . import filters as F

# BS.1770 K-weighting at 48 kHz: a high shelf (the head's effect), then a low cut.
K_SHELF = (np.array([1.53512485958697, -2.69169618940638, 1.19839281085285]),
           np.array([1.0, -1.69065929318241, 0.73248077421585]))
K_HP = (np.array([1.0, -2.0, 1.0]), np.array([1.0, -1.99004745483398, 0.99007225036621]))

BANDS = [('sub', 20, 60), ('low', 60, 250), ('lowmid', 250, 1000), ('mid', 1000, 2000), ('presence', 2000, 5000),
         ('high', 5000, 10000), ('air', 10000, 24000)]


def _chans(x):
    return x[None, :] if x.ndim == 1 else x


def k_weighted(x):
    return F.filt(_chans(x), K_SHELF, K_HP, extend=False)


def window_loudness(x, win=0.2, hop=0.02):
    """Loudness (LUFS-like) of each window: -0.691 + 10 log10 of the K-weighted mean square, summed over channels."""
    k = k_weighted(x)
    p = np.sum(k * k, axis=0)
    w = max(1, int(win * SR))
    h = max(1, int(hop * SR))
    c = np.concatenate([[0.0], np.cumsum(p)])
    n = p.size
    if n <= w:
        return np.array([-0.691 + 10 * np.log10(max(c[-1] / w, 1e-20))])
    starts = np.arange(0, n - w + 1, h)
    ms = (c[starts + w] - c[starts]) / w
    return -0.691 + 10 * np.log10(np.maximum(ms, 1e-20))


def loudness_max(x, win=0.2):
    return float(np.max(window_loudness(x, win)))


def integrated(x):
    """Gated integrated loudness (BS.1770's absolute and relative gates on 400 ms blocks)."""
    blocks = window_loudness(x, 0.4, 0.1)
    blocks = blocks[blocks > -70]
    if not blocks.size:
        return -70.0
    rel = 10 * np.log10(np.mean(10 ** (blocks / 10))) - 10
    blocks = blocks[blocks > rel]
    return float(10 * np.log10(np.mean(10 ** (blocks / 10)))) if blocks.size else -70.0


def spectrum(x):
    """Power spectrum of the whole sound (mono sum) and its frequencies."""
    m = x if x.ndim == 1 else x.mean(axis=0)
    P = np.abs(np.fft.rfft(m)) ** 2
    f = np.fft.rfftfreq(m.size, 1.0 / SR)
    return f, P


def band_shares(x):
    f, P = spectrum(x)
    total = P[(f >= 20)].sum() + 1e-20
    return {name: float(P[(f >= lo) & (f < hi)].sum() / total) for name, lo, hi in BANDS}


def centroid(x):
    f, P = spectrum(x)
    sel = f >= 20
    return float(np.sum(f[sel] * P[sel]) / (np.sum(P[sel]) + 1e-20))


def peak_to_neighbors(x, lo=2000.0, hi=5000.0):
    """How far the strongest third-octave in [lo, hi] stands over the average of the octaves around it (dB): a
    sharp, piercing spike shows up high here."""
    f, P = spectrum(x)
    edges = 1000.0 * 2.0 ** (np.arange(-15, 15) / 3.0)
    centers, levels = [], []
    for a, b in zip(edges[:-1], edges[1:]):
        sel = (f >= a) & (f < b)
        if sel.any():
            centers.append(np.sqrt(a * b))
            levels.append(10 * np.log10(P[sel].mean() + 1e-30))
    centers = np.array(centers)
    levels = np.array(levels)
    worst = -99.0
    for i, c in enumerate(centers):
        if lo <= c <= hi:
            around = np.r_[levels[max(0, i - 4):max(0, i - 1)], levels[i + 2:i + 5]]
            if around.size:
                worst = max(worst, levels[i] - float(np.mean(around)))
    return worst


def head_ms(x, floor_db=-40.0):
    """Milliseconds before the sound first reaches floor_db under its peak (a late start feels laggy)."""
    a = np.max(np.abs(_chans(x)), axis=0)
    p = a.max()
    if p <= 0:
        return 0.0
    idx = np.nonzero(a > p * 10 ** (floor_db / 20))[0]
    return float(idx[0] / SR * 1000.0) if idx.size else 0.0


def tail_db(x, ms=10.0):
    """The level of the last few milliseconds under the peak (a one-shot should end near silence)."""
    a = _chans(x)
    k = max(1, int(ms / 1000.0 * SR))
    end = np.sqrt(np.mean(a[:, -k:] ** 2))
    return float(a2db(end) - a2db(np.max(np.abs(a))))


def quiet_clicks(x, quiet_db=-30.0, ratio=12.0, floor_db=-66.0):
    """Clicks where the sound is quiet (a layer cut off, a step in a fade): sharp second-difference spikes many times
    the local level, in stretches more than quiet_db under the peak. Returns how many separate clicks."""
    m = x if x.ndim == 1 else x.mean(axis=0)
    if m.size < 64:
        return 0
    pk = float(np.max(np.abs(m))) + 1e-12
    d2 = np.abs(np.diff(m, 2))
    k = max(8, int(0.005 * SR))
    c = np.concatenate([[0.0], np.cumsum(m * m)])
    idx = np.arange(1, m.size - 1)
    lo = np.clip(idx - k // 2, 0, m.size)
    hi = np.clip(idx + k // 2, 0, m.size)
    local = np.sqrt((c[hi] - c[lo]) / np.maximum(hi - lo, 1))
    quiet = local < pk * 10 ** (quiet_db / 20)
    spikes = (d2 > ratio * local) & (d2 > pk * 10 ** (floor_db / 20)) & quiet
    hits = np.nonzero(spikes)[0]
    if not hits.size:
        return 0
    return int(1 + np.sum(np.diff(hits) > k))


def seam(x):
    """For loops: the jump from the last sample to the first against the usual step between neighbors (1 is a
    seamless join), and the level change across the seam in dB (50 ms each side)."""
    a = _chans(x)
    steps = np.abs(np.diff(a, axis=1))
    typical = float(np.median(steps)) + 1e-9
    jump = float(np.max(np.abs(a[:, 0] - a[:, -1])))
    k = int(0.05 * SR)
    e1 = np.sqrt(np.mean(a[:, -k:] ** 2)) + 1e-12
    e2 = np.sqrt(np.mean(a[:, :k] ** 2)) + 1e-12
    return jump / typical, float(abs(a2db(e2) - a2db(e1)))


def measure(x):
    """Every number the check reports for one sound."""
    a = _chans(x)
    pk = float(np.max(np.abs(a)))
    return {
        'channels': int(a.shape[0]),
        'duration': float(a.shape[1] / SR),
        'peak_db': float(a2db(pk)),
        'rms_db': float(a2db(np.sqrt(np.mean(a * a)))),
        'l200': loudness_max(x, 0.2),
        'lufs_m': loudness_max(x, 0.4),
        'lufs_i': integrated(x),
        'dc': float(np.max(np.abs(np.mean(a, axis=1)))),
        'clipped': int(np.sum(np.abs(a) >= 32766.0 / 32768.0)),
        'clicks': quiet_clicks(x),
        'centroid': centroid(x),
        'bands': band_shares(x),
        'spike_2_5k': peak_to_neighbors(x),
        'head_ms': head_ms(x),
        'tail_db': tail_db(x),
    }
