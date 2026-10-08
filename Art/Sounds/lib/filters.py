"""Filters: biquads (the audio EQ cookbook's designs), one-poles and combs, applied exactly through the FFT.

A recursive (IIR) filter's response is a ratio of polynomials in z, so it can be evaluated straight on the FFT's bins.
Padding the signal by the filter's ring time makes the circular result equal the real causal one, with no Python loop
per sample. A filter is a section (b, a); a chain is a list of sections, applied in one pass.

Sweeping filters (a cutoff that moves) crossfade between a bank of fixed filters along the sweep, and stft_filter
applies any magnitude curve that changes over time (formants).
"""
import numpy as np

from .core import SR, NYQ, next_pow2, length


# --- Designs (sections) ---------------------------------------------------------------------------------------------

def _w(f):
    f = float(np.clip(f, 5.0, NYQ * 0.98))
    w0 = 2.0 * np.pi * f / SR
    return np.cos(w0), np.sin(w0)


def _norm(b, a):
    b = np.asarray(b, dtype=float)
    a = np.asarray(a, dtype=float)
    return (b / a[0], a / a[0])


def lp(f, q=0.7071):
    c, s = _w(f)
    al = s / (2.0 * q)
    return _norm([(1 - c) / 2, 1 - c, (1 - c) / 2], [1 + al, -2 * c, 1 - al])


def hp(f, q=0.7071):
    c, s = _w(f)
    al = s / (2.0 * q)
    return _norm([(1 + c) / 2, -(1 + c), (1 + c) / 2], [1 + al, -2 * c, 1 - al])


def bp(f, q=1.0):
    """Band-pass with 0 dB at its center."""
    c, s = _w(f)
    al = s / (2.0 * q)
    return _norm([al, 0.0, -al], [1 + al, -2 * c, 1 - al])


def notch(f, q=1.0):
    c, s = _w(f)
    al = s / (2.0 * q)
    return _norm([1.0, -2 * c, 1.0], [1 + al, -2 * c, 1 - al])


def peak(f, db, q=1.0):
    c, s = _w(f)
    A = 10.0 ** (db / 40.0)
    al = s / (2.0 * q)
    return _norm([1 + al * A, -2 * c, 1 - al * A], [1 + al / A, -2 * c, 1 - al / A])


def lowshelf(f, db, q=0.7071):
    c, s = _w(f)
    A = 10.0 ** (db / 40.0)
    al = s / (2.0 * q)
    sq = 2.0 * np.sqrt(A) * al
    return _norm([A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq)],
                 [(A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq])


def highshelf(f, db, q=0.7071):
    c, s = _w(f)
    A = 10.0 ** (db / 40.0)
    al = s / (2.0 * q)
    sq = 2.0 * np.sqrt(A) * al
    return _norm([A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq)],
                 [(A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq])


def allpass(f, q=0.7071):
    c, s = _w(f)
    al = s / (2.0 * q)
    return _norm([1 - al, -2 * c, 1 + al], [1 + al, -2 * c, 1 - al])


def lp1(f):
    """One-pole low-pass (6 dB per octave)."""
    p = np.exp(-2.0 * np.pi * min(f, NYQ * 0.98) / SR)
    return (np.array([1.0 - p]), np.array([1.0, -p]))


def hp1(f):
    p = np.exp(-2.0 * np.pi * min(f, NYQ * 0.98) / SR)
    return (np.array([(1 + p) / 2, -(1 + p) / 2]), np.array([1.0, -p]))


def _butter_qs(order):
    return [1.0 / (2.0 * np.cos((2 * k - 1) * np.pi / (2 * order))) for k in range(1, order // 2 + 1)]


def lpn(f, order=4):
    """Butterworth low-pass of an even order, as a chain."""
    return [lp(f, q) for q in _butter_qs(order)]


def hpn(f, order=4):
    return [hp(f, q) for q in _butter_qs(order)]


def comb(delay_sec, feedback, damp_hz=None):
    """A feedback comb (metallic ringing, flanges, tubes): y = x + g * y[n - D], optionally darker on each pass."""
    d = max(1, int(round(delay_sec * SR)))
    a = np.zeros(d + 2)
    a[0] = 1.0
    if damp_hz:
        # A one-pole low-pass inside the loop, folded into the denominator: 1 - g * z^-D * (1 - p) / (1 - p z^-1).
        p = np.exp(-2.0 * np.pi * damp_hz / SR)
        a = np.zeros(d + 2)
        a[0] = 1.0
        a[1] = -p
        a[d] += -feedback * (1 - p)
        b = np.array([1.0, -p])
        return (b, a)
    a[d] = -feedback
    return (np.array([1.0]), a)


# --- Applying -------------------------------------------------------------------------------------------------------

def _flatten(sections):
    out = []
    for s in sections:
        if isinstance(s, list):
            out.extend(_flatten(s))
        elif s is not None:
            out.append(s)
    return out


def response(sections, m):
    """The chain's complex response on the m-point FFT's bins."""
    k = np.arange(m // 2 + 1)
    z = np.exp(-2j * np.pi * k / m)
    H = np.ones(k.size, dtype=complex)
    for b, a in sections:
        H *= np.polyval(b[::-1], z) / np.polyval(a[::-1], z)
    return H


def ring_samples(sections, floor=1e-5, cap_sec=12.0):
    """How long the chain rings before falling under floor (from its slowest pole)."""
    worst = 0
    for b, a in sections:
        if len(a) <= 1:
            continue
        if len(a) > 64:
            # A comb: its poles sit near radius g ** (1 / D), and the loop gain is at most the sum of the feedback taps.
            d = int(np.nonzero(a)[0][-1])
            g = min(float(np.sum(np.abs(a[1:]))), 0.999999)
            r = max(g, 1e-9) ** (1.0 / d)
        else:
            r = float(np.max(np.abs(np.roots(a))))
        if r >= 1.0:
            return int(cap_sec * SR)
        if r > 0:
            worst = max(worst, int(np.log(floor) / np.log(r)) + 1)
    return min(worst + 64, int(cap_sec * SR))


def filt(x, *sections, extend=True, circular=False):
    """Runs x through the chain. extend keeps the ringing past the end; circular filters a loop as a loop."""
    secs = _flatten(sections)
    if not secs:
        return x.copy()
    n = length(x)
    if circular:
        H = response(secs, n)
        return np.fft.irfft(np.fft.rfft(x, axis=-1) * H, n, axis=-1)
    tail = ring_samples(secs)
    m = next_pow2(n + tail)
    H = response(secs, m)
    y = np.fft.irfft(np.fft.rfft(x, m, axis=-1) * H, m, axis=-1)
    return y[..., :n + (tail if extend else 0)]


def convolve(x, h):
    """Linear convolution through the FFT (mono or stereo x, mono or stereo h)."""
    n = length(x) + length(h) - 1
    m = next_pow2(n)
    X = np.fft.rfft(x, m, axis=-1)
    Hh = np.fft.rfft(h, m, axis=-1)
    if X.ndim == 1 and Hh.ndim == 2:
        X = X[None, :]
    return np.fft.irfft(X * Hh, m, axis=-1)[..., :n]


# Shorthands: x through one filter.
def lowpass(x, f, q=0.7071, order=2, **kw):
    return filt(x, lpn(f, order) if order > 2 else lp(f, q), **kw)


def highpass(x, f, q=0.7071, order=2, **kw):
    return filt(x, hpn(f, order) if order > 2 else hp(f, q), **kw)


def bandpass(x, f, q=1.0, **kw):
    return filt(x, bp(f, q), **kw)


def dc_block(x, f=18.0, circular=False):
    if circular:
        y = x - np.mean(x, axis=-1, keepdims=True)
        return filt(y, hpn(f, 2), circular=True)
    return filt(x, hpn(f, 2), extend=False)


# --- Moving filters -------------------------------------------------------------------------------------------------

_DESIGNS = {'lp': lp, 'hp': hp, 'bp': bp, 'notch': notch}


def sweep(x, kind, freq, q=0.7071, per_octave=6, gain_db=None):
    """x through a filter whose frequency follows freq (an array per sample, or a number): a bank of fixed filters on
    a log grid, crossfaded sample by sample. kind: 'lp', 'hp', 'bp', 'notch' or 'peak' (with gain_db)."""
    n = length(x)
    f = np.broadcast_to(np.asarray(freq, dtype=float), (n,))
    f = np.clip(f, 20.0, NYQ * 0.95)
    lo, hi = float(f.min()), float(f.max())

    def design(c):
        return peak(c, gain_db, q) if kind == 'peak' else _DESIGNS[kind](c, q)

    if hi / lo < 1.01:
        return filt(x, design(lo), extend=False)
    k = int(np.clip(np.ceil(np.log2(hi / lo) * per_octave) + 1, 2, 64))
    grid = np.geomspace(lo, hi, k)
    bank = np.stack([filt(x, design(c), extend=False) for c in grid], axis=0)
    pos = np.log(f / lo) / np.log(hi / lo) * (k - 1)
    i = np.clip(np.floor(pos).astype(int), 0, k - 2)
    u = pos - i
    cols = np.arange(n)
    if x.ndim == 1:
        return bank[i, cols] * (1 - u) + bank[i + 1, cols] * u
    return bank[i, :, cols].T * (1 - u) + bank[i + 1, :, cols].T * u


def stft_filter(x, gain_fn, frame=1024):
    """x through a magnitude curve that may change over time: gain_fn(freqs, t) returns the gain at freqs (Hz) for the
    frame centered at t seconds. Zero-phase per frame, overlap-added with no wrap-around."""
    n = length(x)
    hop = frame // 4
    win = 0.5 - 0.5 * np.cos(2 * np.pi * np.arange(frame) / frame)
    m = 2 * frame
    off = frame // 2
    freqs = np.fft.rfftfreq(m, 1.0 / SR)
    xp = np.concatenate([np.zeros(frame), x, np.zeros(2 * frame)])
    out = np.zeros(len(xp) + m)
    for s in range(0, len(xp) - frame + 1, hop):
        seg = xp[s:s + frame]
        if not np.any(seg):
            continue
        buf = np.zeros(m)
        buf[off:off + frame] = seg * win
        t = (s - frame + frame / 2) / SR
        y = np.fft.irfft(np.fft.rfft(buf) * gain_fn(freqs, t), m)
        out[s:s + m] += y
    # Buffer index j stands for xp index s - off + j, so out holds xp shifted by off. Periodic Hann at a quarter-frame
    # hop sums to 2.
    return out[off + frame:off + frame + n] / 2.0
