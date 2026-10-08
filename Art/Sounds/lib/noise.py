"""Noise of every color, sparse impulse streams (grit, crackle, ratchets) and slow random curves for modulation.

Every function takes a numpy Generator, so the same seed gives the same noise. periodic=True makes noise that loops
without a seam (it's shaped in one FFT of exactly n samples, which is circular).
"""
import numpy as np

from .core import SR, NYQ, next_pow2


def white(n, r):
    return r.standard_normal(int(n))


def shaped(n, r, slope=0.0, lo=None, hi=None, periodic=False):
    """Noise whose spectrum tilts by slope dB per octave (0 white, -3 pink, -6 brown, +3 blue), optionally band-limited
    between lo and hi Hz with soft half-octave edges. Unit RMS."""
    n = int(n)
    m = n if periodic else next_pow2(n + 2048)
    X = np.fft.rfft(r.standard_normal(m))
    f = np.fft.rfftfreq(m, 1.0 / SR)
    g = (np.maximum(f, 10.0) / 1000.0) ** (slope / 6.0206)
    if lo:
        u = np.clip(np.log2(np.maximum(f, 1e-3) / lo) * 2.0 + 1.0, 0.0, 1.0)
        g *= np.sin(0.5 * np.pi * u) ** 2
    if hi:
        u = np.clip(np.log2(hi / np.maximum(f, 1e-3)) * 2.0 + 1.0, 0.0, 1.0)
        g *= np.sin(0.5 * np.pi * u) ** 2
    g[0] = 0.0
    y = np.fft.irfft(X * g, m)[:n]
    s = np.sqrt(np.mean(y * y))
    return y / s if s > 0 else y


def pink(n, r, **kw):
    return shaped(n, r, -3.0, **kw)


def brown(n, r, **kw):
    return shaped(n, r, -6.0, **kw)


def blue(n, r, **kw):
    return shaped(n, r, 3.0, **kw)


def band(n, r, lo, hi, periodic=False):
    return shaped(n, r, 0.0, lo, hi, periodic)


def dust(n, r, rate, amp=(0.25, 1.0), bipolar=True, periodic=False):
    """Random single-sample impulses, rate per second (a number or an array per sample). The raw grit that filters
    turn into crunch, crackle and sizzle."""
    n = int(n)
    rate = np.broadcast_to(np.asarray(rate, dtype=float), (n,))
    hits = r.random(n) < rate / SR
    out = np.zeros(n)
    k = int(hits.sum())
    if k:
        a = r.uniform(amp[0], amp[1], k)
        if bipolar:
            a *= np.where(r.random(k) < 0.5, -1.0, 1.0)
        out[hits] = a
    return out


def velvet(n, r, density=2000.0):
    """Velvet noise: one +-1 impulse per period of 1/density seconds, at a random place in it. Smooth, sparse, and a
    good decorrelator."""
    n = int(n)
    period = max(1, int(SR / density))
    out = np.zeros(n)
    starts = np.arange(0, n, period)
    pos = starts + r.integers(0, period, starts.size)
    pos = pos[pos < n]
    out[pos] = np.where(r.random(pos.size) < 0.5, -1.0, 1.0)
    return out


def smooth_random(n, r, rate=5.0, periodic=False):
    """A slow random curve in [-1, 1] with about rate turns per second (cosine-eased between random points)."""
    n = int(n)
    step = SR / max(rate, 1e-3)
    if periodic:
        m = max(2, int(round(n / step)))
        step = n / m
        pts = r.uniform(-1.0, 1.0, m + 1)
        pts[-1] = pts[0]
    else:
        m = int(np.ceil(n / step)) + 1
        pts = r.uniform(-1.0, 1.0, m + 1)
    pos = np.arange(n) / step
    i = np.floor(pos).astype(int)
    u = pos - i
    u = 0.5 - 0.5 * np.cos(np.pi * u)
    return pts[i] * (1.0 - u) + pts[i + 1] * u


def times_poisson(dur, r, rate, start=0.0, min_gap=0.0):
    """Random event times in [start, dur): a Poisson process with rate per second (a number, or a function of time)."""
    out = []
    t = start
    peak_rate = rate if not callable(rate) else max(rate(x) for x in np.linspace(start, dur, 64))
    if peak_rate <= 0:
        return out
    while True:
        t += r.exponential(1.0 / peak_rate)
        if t >= dur:
            break
        here = rate(t) if callable(rate) else rate
        if r.random() * peak_rate <= here and (not out or t - out[-1] >= min_gap):
            out.append(t)
    return out
