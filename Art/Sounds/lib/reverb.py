"""Convolution reverb with impulse responses made from noise: an outdoor slap-back (the valley's walls and false
fronts throwing a shot back), an open-air tail (a long, dark, sparse wash for bells and ghosts) and a small room.

An IR's diffuse part is noise split into octave bands, each decaying at its own rate (highs die first, as air and
walls absorb them). IRs are normalized to unit energy, so a wet level in dB means the same across them.
"""
import numpy as np

from .core import SR, ns, times, db2a, length, fit
from .env import LN1000
from .noise import white, smooth_random
from . import filters as F

OCTAVES = [31.25, 62.5, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0]


def _band_masks(m):
    """Octave masks on an m-point FFT's bins that sum to one (cos^2 crossovers in log frequency)."""
    f = np.maximum(np.fft.rfftfreq(m, 1.0 / SR), 1.0)
    lf = np.log2(f)
    cs = np.log2(np.array(OCTAVES))
    masks = []
    for i, c in enumerate(cs):
        d = lf - c
        w = np.cos(0.5 * np.pi * np.clip(d, -1.0, 1.0)) ** 2
        if i == 0:
            w = np.where(d < 0, 1.0, w)
        if i == len(cs) - 1:
            w = np.where(d > 0, 1.0, w)
        masks.append(w)
    return masks


def decaying_noise(dur, r, t60_low, t60_high, f_low=250.0, f_high=8000.0):
    """Noise whose octave bands decay at rates from t60_low (at and under f_low) to t60_high (at and over f_high)."""
    n = ns(dur)
    t = times(n)
    W = np.fft.rfft(white(n, r))
    out = np.zeros(n)
    for c, mask in zip(OCTAVES, _band_masks(n)):
        u = np.clip(np.log(c / f_low) / np.log(f_high / f_low), 0.0, 1.0)
        t60 = t60_low * (t60_high / t60_low) ** u
        out += np.fft.irfft(W * mask, n) * np.exp(-LN1000 * t / t60)
    return out


def _unit(ir):
    e = np.sqrt(np.sum(ir * ir))
    return ir / e if e > 0 else ir


def room(r, t60=0.4, size=1.0, damp=0.35, dur=None):
    """A small room (a shop, a cellar): early reflections in the first ~20 ms, then a dense tail."""
    dur = dur or t60 * 1.3
    n = ns(dur)
    ir = np.zeros(n)
    # Early reflections: sparse taps, sign and strength scattered, darker as they come later.
    span = 0.022 * size
    for i in range(14):
        tt = 0.0015 + span * r.random() ** 0.8
        k = ns(tt)
        if k < n:
            ir[k] += (1.0 - tt / (span * 1.2)) * r.uniform(0.3, 1.0) * (1 if r.random() < 0.5 else -1)
    ir = F.filt(ir, F.lp(7000.0, 0.6), extend=False)
    tail = decaying_noise(dur, r, t60, t60 * damp)
    onset = np.clip((times(n) - 0.004 * size) / (0.012 * size), 0.0, 1.0) ** 2
    ir += tail * onset * 0.35
    return _unit(ir)


def early(r, span=0.04, count=18, lp=6000.0, first=0.003):
    """Early reflections only: the ground and anything near (a wall, a wagon, a rock) answering within span seconds,
    each fainter and darker. Thickens a dry hit into a body without a tail."""
    n = ns(span + 0.01)
    ir = np.zeros(n)
    for i in range(count):
        tt = first + (span - first) * (r.random() ** 1.3)
        k = ns(tt)
        if k < n:
            ir[k] += np.exp(-2.5 * tt / span) * r.uniform(0.4, 1.0) * (1 if r.random() < 0.7 else -1)
    ir = F.filt(ir, F.lp(lp, 0.6), extend=False)
    return _unit(ir)


def outdoor(r, dur=1.5, slaps=((0.13, -8.0), (0.31, -13.0)), slap_lp=3200.0, tail_t60=1.0, tail_db=-14.0,
            tail_start=0.04, tail_peak=0.2, tail_lp=2200.0, flutter=0.35):
    """Open ground with buildings and hills: a few distinct slap-backs, then a rolling diffuse tail that swells as
    echoes come back from farther terrain, dark from the air's absorption."""
    n = ns(dur)
    ir = np.zeros(n)
    for tt, db in slaps:
        k = ns(tt)
        if k >= n:
            continue
        # A facade or a hill reflects a smeared burst, not a single click.
        m = ns(0.02)
        b = white(m, r) * np.exp(-LN1000 * times(m) / 0.012)
        b = F.filt(b, F.lpn(slap_lp, 4), extend=False)
        b /= np.max(np.abs(b)) + 1e-9
        ir[k:k + m] += fit(b, min(m, n - k)) * db2a(db)
    t = times(n)
    tail = decaying_noise(dur, r, tail_t60, tail_t60 * 0.45, 200.0, 4000.0)
    rise = np.clip((t - tail_start) / max(tail_peak - tail_start, 1e-3), 0.0, 1.0)
    rise = rise * rise * (3 - 2 * rise)
    roll = 1.0 + flutter * smooth_random(n, r, 7.0)
    tail = F.filt(tail * rise * roll, F.lpn(tail_lp, 4), extend=False)
    # Levels are peaks against a 0 dB reflection: a slap at -8 dB peaks at 0.4, and so does a tail at -8 dB.
    tail /= np.max(np.abs(tail)) + 1e-12
    ir += tail * db2a(tail_db)
    return _unit(ir)


def open_air(r, t60=2.6, predelay=0.03, lp=2400.0, dur=None, density=1.0):
    """A wide, dark open-air wash (a valley, a canyon): a soft build, then a long tail."""
    dur = dur or t60 * 1.15
    n = ns(dur)
    t = times(n)
    tail = decaying_noise(dur, r, t60, t60 * 0.35, 200.0, 5000.0)
    rise = np.clip((t - predelay) / 0.08, 0.0, 1.0) ** 1.5
    if density < 1.0:
        # Sparse: the tail breaks into separate distant echoes.
        tail *= 0.3 + 0.7 * np.clip(smooth_random(n, r, 14.0) * 2.0 - (1.0 - density), 0.0, 1.0)
    ir = F.filt(tail * rise, F.lpn(lp, 2), extend=False)
    return _unit(ir)


def plate(r, t60=1.2, dur=None):
    """A bright, smooth, dense plate (for stingers and the interface): no early echoes, an instant dense tail."""
    dur = dur or t60 * 1.2
    n = ns(dur)
    tail = decaying_noise(dur, r, t60, t60 * 0.55, 300.0, 9000.0)
    rise = np.clip(times(n) / 0.004, 0.0, 1.0)
    return _unit(tail * rise)


def stereo_ir(make, r1, r2, **kw):
    """Two decorrelated IRs (different seeds), for stereo width from a mono source."""
    a = make(r1, **kw)
    b = make(r2, **kw)
    m = max(length(a), length(b))
    return np.vstack([fit(a, m), fit(b, m)])


def apply(x, ir, wet_db=-12.0, dry_db=0.0, pre_hp=None, pre_lp=None):
    """x plus its reverb at wet_db (the IR has unit energy), keeping the whole tail."""
    send = x
    if pre_hp:
        send = F.filt(send, F.hp(pre_hp), extend=False)
    if pre_lp:
        send = F.filt(send, F.lp(pre_lp), extend=False)
    wet = F.convolve(send, ir) * db2a(wet_db)
    out = wet.copy()
    dry = x * db2a(dry_db)
    if out.ndim == 2 and dry.ndim == 1:
        dry = np.vstack([dry, dry])
    out[..., :length(dry)] += dry
    return out
