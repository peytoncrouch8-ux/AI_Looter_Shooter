"""Stereo for 2D sounds (the interface, the player's own feedback): panning and a mono-safe widener."""
import numpy as np

from .core import SR, ns, times, length
from .noise import velvet
from . import filters as F


def pan(x, p=0.0):
    """Mono to stereo, placed by constant power: p = -1 left, 0 center, +1 right."""
    a = (p + 1.0) * np.pi / 4.0
    return np.vstack([x * np.cos(a), x * np.sin(a)]) * np.sqrt(2.0)


def decorrelator(r, ms=8.0):
    """A short sparse filter that keeps a sound's color but scrambles its fine timing (velvet noise, decaying)."""
    n = max(8, ns(ms / 1000.0))
    h = velvet(n, r, 3000.0) * np.exp(-3.0 * times(n) / (ms / 1000.0))
    h[0] = 0.0
    return h / np.sqrt(np.sum(h * h))


def widen(x, r, amount=0.35, low_cut=300.0, ms=8.0):
    """Mono to stereo with width: a decorrelated copy added to one side and taken from the other (L = x + s,
    R = x - s), so the mono sum is exactly the original. Lows stay centered."""
    if x.ndim == 2:
        x = x.mean(axis=0)
    s = F.convolve(x, decorrelator(r, ms))[:length(x)]
    s = F.filt(s, F.hp(low_cut, 0.7), extend=False) * amount
    return np.vstack([x + s, x - s])


def width(x, amount):
    """Scales a stereo signal's side (0 mono, 1 as is, > 1 wider)."""
    if x.ndim == 1:
        return x
    m = 0.5 * (x[0] + x[1])
    s = 0.5 * (x[0] - x[1]) * amount
    return np.vstack([m + s, m - s])
