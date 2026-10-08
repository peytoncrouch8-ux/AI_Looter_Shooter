"""Oscillators: sines with any pitch curve, band-limited classic waves built from harmonics (they can't alias, even
mid-sweep), FM, plucked strings and struck membranes as sums of decaying partials.
"""
import numpy as np

from .core import SR, NYQ, ns, times
from .env import LN1000


def phase(freq, n=None, phase0=0.0):
    """The running phase in cycles of a frequency that may change every sample."""
    if np.isscalar(freq):
        return phase0 + float(freq) * np.arange(int(n)) / SR
    f = np.asarray(freq, dtype=float)
    return phase0 + np.concatenate([[0.0], np.cumsum(f[:-1])]) / SR


def _n(freq, dur, n):
    if n is not None:
        return int(n)
    if not np.isscalar(freq):
        return len(freq)
    return ns(dur)


def sine(freq, dur=None, n=None, phase0=0.0):
    n = _n(freq, dur, n)
    return np.sin(2.0 * np.pi * phase(freq, n, phase0))


def expsweep(f0, f1, dur, shape=1.0, n=None):
    """A frequency curve gliding from f0 to f1 geometrically; shape > 1 moves most of the glide to the start."""
    n = ns(dur) if n is None else int(n)
    u = np.arange(n) / max(n - 1, 1)
    u = 1.0 - (1.0 - u) ** shape
    return f0 * (f1 / f0) ** u


def glide(points, dur=None, n=None):
    """A frequency curve through (time, Hz) points, geometric between them, held after the last."""
    pts = sorted(points)
    n = ns(dur if dur is not None else pts[-1][0]) if n is None else int(n)
    t = times(n)
    return np.exp(np.interp(t, [p[0] for p in pts], [np.log(p[1]) for p in pts]))


def additive(freq, amps, dur=None, n=None, phase0=0.0, top=0.92):
    """Harmonics k * freq with amplitudes amps (a list, or a function of k); each fades out as it nears the top of the
    band (top * Nyquist), so nothing aliases even when freq moves."""
    n = _n(freq, dur, n)
    f = np.broadcast_to(np.asarray(freq, dtype=float), (n,))
    ph = 2.0 * np.pi * phase(freq if not np.isscalar(freq) else float(freq), n, phase0)
    out = np.zeros(n)
    limit = NYQ * top
    edge = NYQ * 0.05
    count = len(amps) if not callable(amps) else int(limit / max(float(f.min()), 1.0)) + 1
    for k in range(1, count + 1):
        a = amps(k) if callable(amps) else amps[k - 1]
        if a == 0:
            continue
        mask = np.clip((limit - k * f) / edge, 0.0, 1.0)
        if not mask.any():
            break
        out += a * mask * np.sin(k * ph)
    return out


def saw(freq, dur=None, n=None, phase0=0.0):
    return additive(freq, lambda k: (2.0 / np.pi) / k, dur, n, phase0)


def square(freq, dur=None, n=None, phase0=0.0):
    return additive(freq, lambda k: (4.0 / np.pi) / k if k % 2 else 0.0, dur, n, phase0)


def triangle(freq, dur=None, n=None, phase0=0.0):
    return additive(freq, lambda k: (8.0 / np.pi ** 2) * (-1) ** ((k - 1) // 2) / k ** 2 if k % 2 else 0.0, dur, n, phase0)


def fm(carrier, ratio, index, dur=None, n=None):
    """Two-operator FM: a sine at carrier, its phase pushed by a sine at carrier * ratio; index may be an envelope."""
    n = _n(carrier, dur, n)
    pc = 2.0 * np.pi * phase(carrier, n)
    fmod = np.asarray(carrier, dtype=float) * ratio
    pm = 2.0 * np.pi * phase(fmod if not np.isscalar(fmod) else float(fmod), n)
    return np.sin(pc + np.asarray(index) * np.sin(pm))


def partials(freqs, t60s, amps, dur, phases=None, attack=0.0):
    """A sum of decaying sines (what a struck or plucked thing leaves ringing). Partials over the band are dropped."""
    n = ns(dur)
    t = times(n)
    out = np.zeros(n)
    for i, (f, d, a) in enumerate(zip(freqs, t60s, amps)):
        if f >= NYQ * 0.95 or a == 0:
            continue
        ph = 0.0 if phases is None else phases[i]
        out += a * np.exp(-LN1000 * t / max(d, 1e-4)) * np.sin(2.0 * np.pi * f * t + ph)
    if attack > 0:
        k = ns(attack)
        if k > 1:
            out[:k] *= 0.5 - 0.5 * np.cos(np.pi * np.arange(k) / k)
    return out


def string(f0, dur, r, pluck=0.2, t60=2.0, bright=0.5, inharm=0.00015, damping=0.35, detune=0.0):
    """A plucked steel string: harmonics shaped by where it's plucked, the high ones dying first, slightly stretched
    (stiffness). bright 0..1 sets how much top the pluck has. detune > 0 splits each partial in two for a chorused,
    beating ring (like doubled strings)."""
    freqs, t60s, amps, phases = [], [], [], []
    k = 1
    while True:
        fk = k * f0 * np.sqrt(1.0 + inharm * k * k)
        if fk > NYQ * 0.9 or k > 160:
            break
        a = abs(np.sin(k * np.pi * pluck)) / (k ** (2.0 - bright))
        d = t60 / (1.0 + damping * (k - 1) ** 1.15)
        if detune > 0:
            cents = detune * (2.0 * r.random() - 1.0)
            for s in (1.0, 2.0 ** (cents / 1200.0)):
                freqs.append(fk * s)
                t60s.append(d)
                amps.append(a * 0.5)
                phases.append(0.0)
        else:
            freqs.append(fk)
            t60s.append(d)
            amps.append(a)
            phases.append(0.0)
        k += 1
    return partials(freqs, t60s, amps, dur, phases, attack=0.0006)


MEMBRANE = [1.0, 1.594, 2.136, 2.296, 2.653, 2.918, 3.156, 3.501, 3.600, 3.652, 4.060, 4.154]


def membrane(f0, dur, r, t60=0.4, bend=0.12, bend_time=0.05):
    """A struck drum head: the circular membrane's modes, the pitch falling a little as the head relaxes (bend)."""
    n = ns(dur)
    t = times(n)
    drop = 1.0 + bend * np.exp(-t / max(bend_time, 1e-4))
    out = np.zeros(n)
    for i, ratio in enumerate(MEMBRANE):
        f = f0 * ratio * (1.0 + 0.004 * (2 * r.random() - 1))
        if f * (1 + bend) >= NYQ * 0.9:
            break
        a = 1.0 / (1.0 + 0.8 * i) * (0.6 + 0.4 * r.random())
        d = t60 / (1.0 + 0.35 * i)
        ph = 2.0 * np.pi * phase(f * drop, n)
        out += a * np.exp(-LN1000 * t / d) * np.sin(ph)
    return out
