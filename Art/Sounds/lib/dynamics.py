"""Dynamics: envelope following, transient shaping, compression and a look-ahead limiter."""
import numpy as np

from .core import SR, ns, a2db, db2a, length
from . import filters as F


def sliding_min(a, w):
    """The minimum of a over a centered window of w samples (doubling spans: O(n log w), no Python loop per sample)."""
    w = int(w)
    if w <= 1:
        return a.copy()
    n = a.size
    pad = w // 2
    b = np.concatenate([np.full(pad, np.inf), a, np.full(w, np.inf)])
    r = b
    span = 1
    while span * 2 <= w:
        r = np.minimum(r[:-span], r[span:])
        span *= 2
    return np.minimum(r[:n], r[w - span:w - span + n])


def sliding_max(a, w):
    return -sliding_min(-a, w)


def smooth(a, sec):
    """One-pole smoothing (a lag of about sec seconds)."""
    if sec <= 0:
        return a.copy()
    f = 1.0 / (2.0 * np.pi * sec)
    return F.filt(a, F.lp1(f), extend=False)


def level(x):
    """The absolute level, the loudest channel's."""
    return np.max(np.abs(x), axis=0) if x.ndim == 2 else np.abs(x)


def envelope(x, attack=0.0005, release=0.03):
    """Peak-ish envelope: held over the attack window, then let go with the release."""
    e = sliding_max(level(x), max(1, ns(attack)))
    return np.maximum(smooth(e, release), 0.0)


def transient(x, attack_db=6.0, sustain_db=0.0, fast=0.0008, slow=0.02):
    """Pushes or softens the hits (attack_db) and what rings after (sustain_db), from where a fast envelope rises
    over a slow one."""
    a = level(x)
    ef = smooth(sliding_max(a, max(1, ns(fast))), fast)
    es = smooth(sliding_max(a, max(1, ns(fast))), slow)
    d = a2db(ef + 1e-9) - a2db(es + 1e-9)
    g = attack_db * np.clip(d / 6.0, 0.0, 1.0) + sustain_db * np.clip(-d / 6.0, 0.0, 1.0)
    g = smooth(g, 0.0005)
    return x * db2a(g)


def compress(x, thresh_db=-18.0, ratio=4.0, attack=0.002, release=0.08, knee_db=6.0, makeup_db=0.0):
    """A feed-forward compressor on the peak envelope (the loudest channel drives both)."""
    e = a2db(envelope(x, attack, release) + 1e-9)
    over = e - thresh_db
    # Soft knee: a quadratic blend across the knee.
    k = knee_db / 2.0
    red = np.where(over <= -k, 0.0, np.where(over >= k, over * (1.0 - 1.0 / ratio),
                   (1.0 - 1.0 / ratio) * (over + k) ** 2 / (4.0 * k)))
    g = db2a(-red + makeup_db)
    return x * g


def limit(x, ceiling_db=-1.0, lookahead=0.0015, release=0.05):
    """A look-ahead limiter: no sample passes the ceiling, and the gain moves smoothly. The gain is the sliding
    minimum of what each sample needs, box-smoothed over the look-ahead (so it never goes over what any sample in
    reach needs), and it recovers no faster than the release."""
    c = float(db2a(ceiling_db))
    a = level(x)
    need = np.minimum(1.0, c / np.maximum(a, 1e-12))
    L = max(1, ns(lookahead))
    g = sliding_min(need, 2 * L + 1)
    kernel = np.ones(L + 1) / (L + 1)
    g = np.convolve(g, kernel, mode='same')
    # Smoothed around 1 (no reduction), since the filter starts from rest.
    rel = smooth(g - 1.0, release) + 1.0
    g = np.minimum(g, rel)
    g = np.minimum(g, need)
    return x * g
