"""The basics every recipe uses: the sample rate, seeded randomness, sample and decibel math, and laying layers together.

Signals are numpy float arrays: mono is 1-D, stereo is (2, n). Times are in seconds unless a name says samples.
"""
import zlib

import numpy as np

SR = 48000
NYQ = SR * 0.5


# --- Randomness -----------------------------------------------------------------------------------------------------

def seed_of(*parts):
    """A stable 32-bit seed from strings and numbers (Python's hash() changes between runs, so it can't be used)."""
    return zlib.crc32('|'.join(str(p) for p in parts).encode('utf-8')) & 0xFFFFFFFF


def rng(*parts):
    """A random generator that gives the same numbers for the same parts every run."""
    return np.random.default_rng(seed_of(*parts))


def child(r, *parts):
    """A new generator drawn from r and some names: layers get their own streams, so adding one doesn't shift the rest."""
    return np.random.default_rng(seed_of(int(r.integers(0, 2 ** 31)), *parts))


def jitter(r, value, spread):
    """value scaled by a random factor in [1 - spread, 1 + spread]."""
    return value * (1.0 + spread * (2.0 * r.random() - 1.0))


# --- Samples and levels ---------------------------------------------------------------------------------------------

def ns(sec):
    """Seconds to samples."""
    return int(round(max(0.0, float(sec)) * SR))


def times(n):
    """The time of each of n samples."""
    return np.arange(int(n)) / SR


def silence(sec):
    return np.zeros(ns(sec))


def db2a(db):
    return 10.0 ** (np.asarray(db, dtype=float) / 20.0)


def a2db(a):
    return 20.0 * np.log10(np.maximum(np.abs(a), 1e-12))


def length(x):
    return x.shape[-1]


def peak(x):
    return float(np.max(np.abs(x))) if x.size else 0.0


def rms(x):
    return float(np.sqrt(np.mean(np.square(x)))) if x.size else 0.0


def gain(x, db):
    return x * float(db2a(db))


def normalize(x, peak_db=-1.0):
    p = peak(x)
    return x * (float(db2a(peak_db)) / p) if p > 0 else x


def norm_rms(x, rms_db=-20.0):
    v = rms(x)
    return x * (float(db2a(rms_db)) / v) if v > 0 else x


def next_pow2(n):
    return 1 << int(np.ceil(np.log2(max(2, int(n)))))


def fit(x, n):
    """x cut or padded with silence to n samples."""
    n = int(n)
    have = length(x)
    if have >= n:
        return x[..., :n]
    pad = [(0, 0)] * (x.ndim - 1) + [(0, n - have)]
    return np.pad(x, pad)


def stereo(x):
    """Mono to two identical channels; stereo stays."""
    return np.vstack([x, x]) if x.ndim == 1 else x


def mono(x):
    return x if x.ndim == 1 else x.mean(axis=0)


def add_at(dst, src, at=0.0, db=0.0):
    """dst with src added at a time, growing dst when src runs past its end. Stereo wins over mono."""
    start = ns(at)
    if dst.ndim != src.ndim:
        dst, src = stereo(dst), stereo(src)
    end = start + length(src)
    if end > length(dst):
        dst = fit(dst, end)
    else:
        dst = dst.copy()
    dst[..., start:end] += src * float(db2a(db))
    return dst


def layers(*items):
    """Sums layers given as a signal, (signal, at) or (signal, at, db). None items are skipped."""
    out = np.zeros(1)
    for item in items:
        if item is None:
            continue
        if isinstance(item, tuple):
            sig = item[0]
            at = item[1] if len(item) > 1 else 0.0
            db = item[2] if len(item) > 2 else 0.0
        else:
            sig, at, db = item, 0.0, 0.0
        if sig is None or sig.size == 0:
            continue
        out = add_at(out, sig, at, db)
    return out


def concat(*parts):
    return np.concatenate([np.asarray(p) for p in parts], axis=-1)


# --- Shaping in time ------------------------------------------------------------------------------------------------

def fade(x, fade_in=0.0, fade_out=0.0):
    """Raised-cosine fades at the ends (seconds), so nothing starts or stops with a click."""
    y = x.copy()
    n = length(y)
    a = min(ns(fade_in), n)
    if a > 1:
        y[..., :a] *= 0.5 - 0.5 * np.cos(np.pi * np.arange(a) / a)
    b = min(ns(fade_out), n)
    if b > 1:
        y[..., n - b:] *= 0.5 + 0.5 * np.cos(np.pi * np.arange(1, b + 1) / b)
    return y


def trim_tail(x, floor_db=-70.0, fade_sec=0.012):
    """Cuts the silence after the last sample louder than floor_db under the peak, with a short fade."""
    p = peak(x)
    if p <= 0:
        return x[..., :1]
    level = np.max(np.abs(x), axis=0) if x.ndim == 2 else np.abs(x)
    above = np.nonzero(level > p * float(db2a(floor_db)))[0]
    end = int(above[-1]) + 1 if above.size else 1
    end = min(length(x), end + ns(0.002))
    return fade(x[..., :end], 0.0, min(fade_sec, end / SR * 0.25))


def trim_head(x, floor_db=-66.0, keep=0.0005):
    """Cuts the silence before the first sample louder than floor_db under the peak (keeps a hair before it)."""
    p = peak(x)
    if p <= 0:
        return x
    level = np.max(np.abs(x), axis=0) if x.ndim == 2 else np.abs(x)
    above = np.nonzero(level > p * float(db2a(floor_db)))[0]
    start = max(0, int(above[0]) - ns(keep)) if above.size else 0
    return x[..., start:]


def reverse(x):
    return x[..., ::-1].copy()


def varispeed(x, ratio):
    """Plays x faster (ratio > 1: shorter and higher) or slower, like tape. Band-limited when speeding up."""
    if abs(ratio - 1.0) < 1e-6:
        return x.copy()
    n = length(x)
    m = max(2, int(round(n / ratio)))
    X = np.fft.rfft(x, axis=-1)
    if ratio > 1.0:
        # Keep only what stays under the new Nyquist, with a short taper so it doesn't ring.
        keep = int(X.shape[-1] / ratio)
        taper = np.ones(X.shape[-1])
        edge = max(4, keep // 20)
        taper[keep:] = 0.0
        taper[max(0, keep - edge):keep] = np.linspace(1.0, 0.0, keep - max(0, keep - edge))
        X = X * taper
    src = np.fft.irfft(X, n, axis=-1)
    pos = np.arange(m) * ratio
    if x.ndim == 1:
        return np.interp(pos, np.arange(n), src)
    return np.vstack([np.interp(pos, np.arange(n), ch) for ch in src])


def lerp(a, b, u):
    return a + (b - a) * u


def smoothstep(u):
    u = np.clip(u, 0.0, 1.0)
    return u * u * (3.0 - 2.0 * u)
