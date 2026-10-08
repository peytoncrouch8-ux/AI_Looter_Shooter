"""Envelopes: percussive decays, attack-release shapes and breakpoint curves."""
import numpy as np

from .core import SR, ns, times

LN1000 = 6.907755  # ln(1000): a T60 decay falls 60 dB, a factor of 1000


def end_fade(e, sec=0.004):
    """Brings an envelope to zero over its last few milliseconds, so a layer cut at its length never clicks."""
    k = min(ns(sec), e.size // 8)
    if k > 1:
        e[-k:] *= 0.5 + 0.5 * np.cos(np.pi * np.arange(1, k + 1) / k)
    return e


def decay(n_or_dur, t60, attack=0.0):
    """exp decay that falls 60 dB in t60 seconds, with an optional raised-cosine attack. n_or_dur: samples (int) or
    seconds (float)."""
    n = n_or_dur if isinstance(n_or_dur, (int, np.integer)) else ns(n_or_dur)
    t = times(n)
    e = np.exp(-LN1000 * t / max(t60, 1e-5))
    if attack > 0:
        a = ns(attack)
        if a > 1:
            e[:a] *= 0.5 - 0.5 * np.cos(np.pi * np.arange(a) / a)
    return end_fade(e)


def perc(dur, attack=0.001, t60=0.2, hold=0.0, curve=1.0):
    """A hit's envelope: a quick rise, an optional hold, then a T60 decay (curve > 1 bends it to fall faster early)."""
    n = ns(dur)
    t = times(n)
    e = np.ones(n)
    a = ns(attack)
    if a > 1:
        e[:a] = (0.5 - 0.5 * np.cos(np.pi * np.arange(a) / a))
    tail = np.clip(t - attack - hold, 0.0, None)
    k = np.exp(-LN1000 * tail / max(t60, 1e-5))
    if curve != 1.0:
        k = k ** curve
    return end_fade(e * k)


def ar(dur, attack, release, shape=2.0):
    """Rises over attack, falls over release (powers of a cosine; shape 1 is linear-ish)."""
    n = ns(dur)
    t = times(n)
    up = np.clip(t / max(attack, 1e-5), 0.0, 1.0)
    down = np.clip((dur - t) / max(release, 1e-5), 0.0, 1.0)
    return ((0.5 - 0.5 * np.cos(np.pi * np.minimum(up, down))) ** (shape / 2.0))


def adsr(dur, attack, decay_s, sustain, release):
    n = ns(dur)
    t = times(n)
    e = np.interp(t, [0.0, attack, attack + decay_s, max(attack + decay_s, dur - release), dur],
                  [0.0, 1.0, sustain, sustain, 0.0])
    return e


def bp(points, dur=None, n=None, curve='lin'):
    """A breakpoint curve through (time, value) points. curve: 'lin', 'exp' (geometric, for frequencies; values > 0)
    or 'cos' (eased)."""
    pts = sorted(points)
    if n is None:
        n = ns(dur if dur is not None else pts[-1][0])
    t = times(n)
    xs = np.array([p[0] for p in pts], dtype=float)
    ys = np.array([p[1] for p in pts], dtype=float)
    if curve == 'exp':
        return np.exp(np.interp(t, xs, np.log(np.maximum(ys, 1e-9))))
    if curve == 'cos':
        idx = np.clip(np.searchsorted(xs, t, side='right') - 1, 0, len(xs) - 2)
        x0, x1 = xs[idx], xs[idx + 1]
        u = np.clip((t - x0) / np.maximum(x1 - x0, 1e-9), 0.0, 1.0)
        u = 0.5 - 0.5 * np.cos(np.pi * u)
        return ys[idx] + (ys[idx + 1] - ys[idx]) * u
    return np.interp(t, xs, ys)


def swell(dur, peak_at, t60_after, rise_shape=2.0):
    """Rises to 1 at peak_at (a curved swell, like reversed reverb), then decays by T60."""
    n = ns(dur)
    t = times(n)
    up = np.clip(t / max(peak_at, 1e-5), 0.0, 1.0) ** rise_shape
    down = np.exp(-LN1000 * np.clip(t - peak_at, 0.0, None) / max(t60_after, 1e-5))
    return end_fade(up * down)
