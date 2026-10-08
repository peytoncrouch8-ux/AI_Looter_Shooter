"""Modal synthesis: struck things as a set of modes (frequency, ring time, strength), each a decaying sine, excited by
a strike. The mode sets follow the physics of the shapes: bars, plates, small machined parts, wood, shells, bells.

A mode set is a tuple of three arrays (freqs, t60s, amps). ring() turns it into an impulse response, strike() hits it
with an excitation (a hammer pulse for hard hits, a noise burst for scrapes).
"""
import numpy as np

from .core import SR, NYQ, ns, times, length
from .env import LN1000
from . import filters as F


def modes(freqs, t60s, amps):
    return (np.asarray(freqs, dtype=float), np.asarray(t60s, dtype=float), np.asarray(amps, dtype=float))


def ring(m, dur, r=None, random_phase=0.0):
    """The impulse response of a mode set: a sum of exponentially decaying sines. random_phase 0..1 scatters their
    starting phases (0 starts them all at zero, so the strike's front stays clean)."""
    freqs, t60s, amps = m
    n = ns(dur)
    t = times(n)
    out = np.zeros(n)
    for i in range(freqs.size):
        f = freqs[i]
        if f >= NYQ * 0.95 or f <= 0 or amps[i] == 0:
            continue
        ph = 0.0
        if r is not None and random_phase > 0:
            ph = random_phase * 2.0 * np.pi * r.random()
        out += amps[i] * np.exp(-LN1000 * t / max(t60s[i], 1e-4)) * np.sin(2.0 * np.pi * f * t + ph)
    # Modes that ring longer than dur are faded out, not cut.
    k = min(n // 10, ns(0.02))
    if k > 1:
        out[-k:] *= 0.5 + 0.5 * np.cos(np.pi * np.arange(1, k + 1) / k)
    return out


def strike(m, exc, dur, r=None, random_phase=0.0):
    """The mode set rung by an excitation signal."""
    return F.convolve(exc, ring(m, dur, r, random_phase))[:ns(dur) + length(exc)]


# --- Excitations ----------------------------------------------------------------------------------------------------

def hammer(contact=0.0002):
    """A hit's force pulse: half a sine as long as the contact. Hard steel on steel is ~0.05-0.2 ms (bright); a felt
    mallet or a boot heel is several ms (dark)."""
    k = max(1, ns(contact))
    p = np.sin(np.pi * (np.arange(k) + 0.5) / k)
    return p / p.sum()


def burst(dur, r, t60=None, color=0.0):
    """A noise burst for scrapes and rough hits."""
    from .noise import shaped
    n = max(2, ns(dur))
    x = shaped(n, r, color)
    if t60:
        x *= np.exp(-LN1000 * times(n) / t60)
    return x / np.sqrt(np.sum(x * x))


# --- Mode sets ------------------------------------------------------------------------------------------------------

def _damp(freqs, f_ref, t60, power):
    return t60 * (np.maximum(freqs, 1.0) / f_ref) ** (-power)


BAR = [1.0, 2.756, 5.404, 8.933, 13.344, 18.638, 24.81, 31.87]


def bar(f0, r, t60=1.0, power=0.6, count=6, spread=0.008, tilt=0.7):
    """A free metal bar (a rod, a bolt, a hammer head, a triangle): the free-free beam's stretched partials."""
    k = np.array(BAR[:count])
    f = f0 * k * (1.0 + spread * (2 * r.random(count) - 1))
    a = (1.0 / k ** tilt) * (0.7 + 0.3 * r.random(count))
    return modes(f, _damp(f, f0, t60, power), a)


def plate(f0, r, count=24, t60=0.6, power=0.5, aspect=1.37, spread=0.02, tilt=0.5):
    """A thin metal plate or sheet (a sign, a panel, a box): dense inharmonic modes from (m^2 / aspect + n^2 * aspect)."""
    cand = []
    for m in range(1, 12):
        for n in range(1, 12):
            cand.append(m * m / aspect + n * n * aspect)
    cand = np.unique(np.round(np.array(cand), 3))
    cand = cand / cand[0]
    k = cand[:count]
    f = f0 * k * (1.0 + spread * (2 * r.random(k.size) - 1))
    a = (1.0 / k ** tilt) * (0.4 + 0.6 * r.random(k.size))
    return modes(f, _damp(f, f0, t60, power), a)


def parts(lo, hi, r, count=6, t60=0.05, t60_hi=None, tilt=0.3):
    """Small machined parts (pins, springs, latches, casings): a handful of modes scattered between lo and hi Hz."""
    f = np.sort(np.exp(r.uniform(np.log(lo), np.log(hi), count)))
    t_hi = t60 * 0.5 if t60_hi is None else t60_hi
    u = (np.log(f) - np.log(lo)) / max(np.log(hi / lo), 1e-6)
    d = t60 * (t_hi / t60) ** u
    a = (lo / f) ** tilt * (0.4 + 0.6 * r.random(count))
    return modes(f, d, a)


WOOD = [1.0, 1.58, 2.31, 3.12, 4.05, 5.2, 6.6, 8.1]


def wood(f0, r, t60=0.12, power=1.1, count=7, spread=0.06, tilt=0.6):
    """Wood (planks, crates, stocks, shutters): few modes, irregular, losing the high ones fast."""
    k = np.array(WOOD[:count])
    f = f0 * k * (1.0 + spread * (2 * r.random(count) - 1))
    a = (1.0 / k ** tilt) * (0.5 + 0.5 * r.random(count))
    return modes(f, _damp(f, f0, t60, power), a)


def shell(f0, r, t60=0.25, count=8, spread=0.015):
    """A brass shell casing or a thin tube: near-harmonic ring modes with a few strays, bright and short."""
    k = np.array([1.0, 2.04, 2.97, 3.95, 5.12, 6.03, 7.3, 8.6][:count])
    f = f0 * k * (1.0 + spread * (2 * r.random(count) - 1))
    a = (1.0 / k ** 0.8) * (0.5 + 0.5 * r.random(count))
    return modes(f, _damp(f, f0, t60, 0.7), a)


# A church bell's partials against its prime (strike) tone: hum, prime, tierce (minor third), quint, nominal, and the
# upper ones. Physics of a bell's shape, measured on bells for centuries.
BELL = [(0.5, 1.0, 1.0), (1.0, 0.9, 0.62), (1.19, 0.7, 0.42), (1.5, 0.45, 0.28), (2.0, 1.0, 0.36),
        (2.51, 0.5, 0.2), (2.66, 0.4, 0.18), (3.01, 0.42, 0.13), (4.17, 0.3, 0.09), (5.43, 0.2, 0.06),
        (6.8, 0.14, 0.045), (8.3, 0.1, 0.03)]


def bell(prime, r, t60=6.0, beat=0.8, bright=1.0, count=12):
    """A cast bell: the bell partial series, each split in two a little apart (the slow warble of a real bell, from
    its imperfect roundness). t60 is the hum's ring; upper partials die faster."""
    f, d, a = [], [], []
    for i, (ratio, amp, ring_share) in enumerate(BELL[:count]):
        base = prime * ratio * (1.0 + 0.003 * (2 * r.random() - 1))
        amp = amp * (bright ** (i - 3) if i >= 4 else 1.0)
        split = beat * (0.5 + r.random()) * (1.0 + 0.3 * i)
        for s in (-0.5, 0.5):
            f.append(base + s * split)
            d.append(t60 * ring_share)
            a.append(amp * (0.5 + 0.1 * (2 * r.random() - 1)))
    return modes(f, d, a)


def merge(*sets):
    f = np.concatenate([s[0] for s in sets])
    d = np.concatenate([s[1] for s in sets])
    a = np.concatenate([s[2] for s in sets])
    return (f, d, a)


def scale(m, freq=1.0, t60=1.0, amp=1.0):
    return (m[0] * freq, m[1] * t60, m[2] * amp)
