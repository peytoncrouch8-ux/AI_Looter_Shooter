"""Recorded sources for the recipes that may use them (guns and bullet impacts only: the user's rule, 2026-10-21; every
other sound stays synthesized). Reads a WAV of any common format, brings it to the pipeline's rate and to mono, finds the
single shots in a take of several, slices one out, trims, fades and levels it, so a recipe can shape a recording the way
it shapes a synthesized layer.

The recordings live in Art/Sounds/packs/<Pack>/ (Git LFS; their sources and licence in Art/Sounds/packs/README.md).
Licence (Sonniss GDC bundles): games OK, edits OK, no attribution, no resale or redistribution as sounds, no AI training.
"""
import os
import struct

import numpy as np

from .core import SR, ns, normalize, fade, db2a
from . import filters as F

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PACKS = os.path.join(HERE, 'packs')

_CACHE = {}


# --- Reading ---------------------------------------------------------------------------------------------------------

def read_any(path):
    """(samples as floats, (channels, n) or (n,) for mono; the file's rate) for PCM 8/16/24/32-bit and 32/64-bit float
    WAVs, plain or WAVE_FORMAT_EXTENSIBLE, skipping the chunks recorders add (bext, iXML, LIST, markers...)."""
    with open(path, 'rb') as f:
        data = f.read()
    if data[:4] not in (b'RIFF', b'RF64') or data[8:12] != b'WAVE':
        raise ValueError(f'{path}: not a WAV')
    pos = 12
    fmt = None
    body = None
    while pos + 8 <= len(data):
        cid = data[pos:pos + 4]
        size = struct.unpack('<I', data[pos + 4:pos + 8])[0]
        start = pos + 8
        if cid == b'fmt ':
            fmt = data[start:start + size]
        elif cid == b'data':
            if size == 0xFFFFFFFF or start + size > len(data):
                size = len(data) - start
            body = data[start:start + size]
        pos = start + size + (size & 1)
    if fmt is None or body is None:
        raise ValueError(f'{path}: no fmt or data chunk')
    tag, ch, rate, _, align, bits = struct.unpack('<HHIIHH', fmt[:16])
    if tag == 0xFFFE and len(fmt) >= 26:
        tag = struct.unpack('<H', fmt[24:26])[0]
    width = align // ch
    n = len(body) // align
    raw = np.frombuffer(body[:n * align], dtype=np.uint8).reshape(n, ch, width)
    if tag == 3:
        x = np.frombuffer(raw.tobytes(), dtype='<f4' if width == 4 else '<f8').astype(float).reshape(n, ch)
    elif tag == 1:
        if width == 1:
            x = (raw[..., 0].astype(float) - 128.0) / 128.0
        else:
            # Little-endian signed integers of any width: assemble into int32 from the top bytes down.
            b = raw.astype(np.int64)
            v = np.zeros((n, ch), dtype=np.int64)
            for i in range(width):
                v |= b[..., i] << (8 * i)
            sign = 1 << (8 * width - 1)
            v = (v ^ sign) - sign
            x = v.astype(float) / float(sign)
    else:
        raise ValueError(f'{path}: WAV format {tag} not read')
    x = x.T
    return (x[0] if ch == 1 else x), rate


def resample(x, rate, to=SR):
    """x from rate to the pipeline's rate, band-limited (FFT): fine for the few seconds a slice holds."""
    if rate == to:
        return x
    n = x.shape[-1]
    m = int(round(n * to / rate))
    X = np.fft.rfft(x, axis=-1)
    k = m // 2 + 1
    if X.shape[-1] >= k:
        X = X[..., :k]
        # A short taper under the new Nyquist, so the cut doesn't ring.
        edge = max(4, k // 40)
        X[..., k - edge:] *= np.linspace(1.0, 0.0, edge)
    else:
        X = np.concatenate([X, np.zeros(X.shape[:-1] + (k - X.shape[-1],), dtype=complex)], axis=-1)
    return np.fft.irfft(X, m, axis=-1) * (m / n)


def path_of(name, pack='Sonniss'):
    return os.path.join(PACKS, pack, name)


def load(name, pack='Sonniss', channel='mix'):
    """A pack file at the pipeline's rate, mono: channel 'mix' (the mean), 'left', 'right' or an index; 'stereo' keeps
    both. Cached: a recipe's variations read the same file many times."""
    key = (pack, name, channel)
    if key in _CACHE:
        return _CACHE[key].copy()
    x, rate = read_any(path_of(name, pack))
    if x.ndim == 2 and channel != 'stereo':
        if channel == 'mix':
            x = x.mean(axis=0)
        elif channel == 'left':
            x = x[0]
        elif channel == 'right':
            x = x[min(1, x.shape[0] - 1)]
        else:
            x = x[int(channel)]
    x = resample(x, rate)
    x = x - np.mean(x)
    _CACHE[key] = x
    return x.copy()


# --- Finding and slicing shots ---------------------------------------------------------------------------------------

def envelope_db(x, win=0.001):
    """The level (dB under full scale) of each win-long block's peak, and the block length in samples."""
    m = x if x.ndim == 1 else np.max(np.abs(x), axis=0)
    m = np.abs(m)
    w = max(1, ns(win))
    k = m.size // w
    blocks = m[:k * w].reshape(k, w).max(axis=1)
    return 20.0 * np.log10(np.maximum(blocks, 1e-9)), w


def onsets(x, rise_db=10.0, gap=0.08, floor_db=-9.0, hp=2500.0, look=0.02):
    """Where each shot in a take starts (samples). A shot's front is its brightest moment, so the take is heard above
    hp: a 1 ms block that stands rise_db over the quietest of the look seconds before it and 3 dB over their loudest,
    within -floor_db of the take's loudest such block, at least gap seconds after the last one. The bright band keeps a
    shot found under the previous one's still-loud, darker tail in a burst; the floor keeps the mechanism's clacks and
    the echoes, which are bright too but softer, from counting as shots."""
    m = x if x.ndim == 1 else x.mean(axis=0)
    e, w = envelope_db(F.filt(m, F.hpn(hp, 4), extend=False))
    top = float(e.max())
    lk = max(1, int(round(look * SR / w)))
    hold = max(1, int(round(gap * SR / w)))
    out = []
    last = -hold
    for i in range(lk, e.size):
        if i - last < hold or e[i] < top + floor_db:
            continue
        before = e[i - lk:i]
        if e[i] - before.min() >= rise_db and e[i] >= before.max() + 3.0:
            out.append(refine(m, i * w, 0.002))
            last = i
    return out


def refine(x, at, search=0.004, share=0.1):
    """An onset near sample at made exact: the first sample within search seconds before it (or after) that reaches
    share of the loudest sample in the 2 ms that follow, so takes line up to the sample when layered."""
    m = np.abs(x if x.ndim == 1 else x.mean(axis=0))
    s0 = max(0, at - ns(search))
    s1 = min(m.size, at + ns(search) + ns(0.002))
    seg = m[s0:s1]
    if seg.size == 0:
        return at
    top = float(seg.max())
    hit = np.nonzero(seg >= share * top)[0]
    return s0 + int(hit[0]) if hit.size else at


def shots(x, **kw):
    """The take's shots as (start, end) samples: each runs to the next one's start (or the take's end)."""
    on = onsets(x, **kw)
    n = x.shape[-1]
    return [(s, on[i + 1] if i + 1 < len(on) else n) for i, s in enumerate(on)]


def slice_shot(x, index, length=None, pre=0.0005, fade_out=0.03, **kw):
    """The index-th shot of a take (wrapping round when the take has fewer), from a hair before its onset, at most length
    seconds long and never into the next shot, faded out at its end."""
    sl = shots(x, **kw)
    if not sl:
        raise ValueError('no shot found in the take')
    s, e = sl[index % len(sl)]
    return cut(x, s, e, length, pre, fade_out)


def slice_at(x, t, length, pre=0.0005, fade_out=0.03, search=0.006, until=None):
    """The shot that starts near t seconds (its onset found exactly, refine), length seconds long (or up to until
    seconds, the next shot's start), faded out at its end: for takes whose shots are listed by hand."""
    s = refine(x, ns(t), search)
    e = x.shape[-1] if until is None else ns(until)
    return cut(x, s, e, length, pre, fade_out)


def cut(x, s, e, length=None, pre=0.0005, fade_out=0.03):
    s = max(0, s - ns(pre))
    if length is not None:
        e = min(e, s + ns(length))
    y = x[..., s:e].copy()
    return fade(y, 0.0, min(fade_out, y.shape[-1] / SR * 0.3))


def count(x, **kw):
    return len(shots(x, **kw))


# --- Shaping ---------------------------------------------------------------------------------------------------------

def gate_tail(x, after, t60):
    """x left as it is for after seconds, then faded out with a decay of t60 seconds (60 dB): a recording's own room or
    field cut short so the recipe's tail takes over."""
    n = x.shape[-1]
    t = np.arange(n) / SR
    g = np.where(t < after, 1.0, np.exp(-6.9 * (t - after) / max(t60, 1e-4)))
    return x * g


def body_only(x, at, length, fade_in=0.004, fade_out=0.02):
    """A window of x from at for length seconds with soft edges (a recording's tail or room on its own)."""
    s = ns(at)
    y = x[..., s:s + ns(length)].copy()
    return fade(y, fade_in, fade_out)


def tilt(x, low_db=0.0, high_db=0.0, low_f=200.0, high_f=4000.0):
    """A broad tone change: shelves at low_f and high_f."""
    st = []
    if abs(low_db) > 1e-3:
        st.append(F.lowshelf(low_f, low_db))
    if abs(high_db) > 1e-3:
        st.append(F.highshelf(high_f, high_db))
    return F.filt(x, *st, extend=False) if st else x


def level_peak(x, db=-1.0):
    return normalize(x, db)


def level_loud(x, win=0.01):
    """x scaled so its loudest win seconds (RMS) sit at 1, as the gun recipes mix their layers."""
    w = max(1, ns(win))
    p = x * x if x.ndim == 1 else np.mean(x * x, axis=0)
    c = np.concatenate([[0.0], np.cumsum(p)])
    if p.size <= w:
        r = np.sqrt(c[-1] / max(1, p.size))
    else:
        r = np.sqrt(np.max(c[w:] - c[:-w]) / w)
    return x / r if r > 0 else x


def gain_db(x, db):
    return x * float(db2a(db))


# --- Writing (the pack's excerpts) -----------------------------------------------------------------------------------

def write24(path, x, rate=SR):
    """x as a 24-bit PCM WAV: the pack keeps its excerpts at more depth than the game's 16-bit files."""
    x = np.atleast_2d(np.asarray(x, dtype=float))
    ch, n = x.shape
    q = np.clip(np.round(x * 8388607.0), -8388608, 8388607).astype('<i4')
    b = q.T.reshape(-1).view(np.uint8).reshape(-1, 4)[:, :3].tobytes()
    head = struct.pack('<4sI4s4sIHHIIHH4sI', b'RIFF', 36 + len(b), b'WAVE', b'fmt ', 16, 1, ch, rate, rate * ch * 3,
                       ch * 3, 24, b'data', len(b))
    with open(path, 'wb') as f:
        f.write(head)
        f.write(b)
        if len(b) & 1:
            f.write(bytes(1))
