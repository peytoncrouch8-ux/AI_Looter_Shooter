"""Voices for creatures and breath: a glottal source built from harmonics, shaped by moving formants (the vocal
tract's resonances), with aspiration noise through the same formants.

Each harmonic's strength is read off the vocal tract's response at its own frequency, every few milliseconds, so the
vowel can glide and the pitch can bend without filters ringing or aliasing. The tract is a cascade of resonances (each
F^2 / sqrt((F^2 - f^2)^2 + (B f)^2)), as in classic speech synthesizers: the vowel's five formants and three fixed
higher ones, so the top of the voice doesn't die off early. The source falls 6 dB per octave (the glottal pulse's 12,
less the lips' radiation), which tilt adjusts: negative is softer and breathier, positive pressed and brighter.
"""
import numpy as np

from .core import SR, NYQ, ns, times
from .noise import white, smooth_random
from .osc import phase
from . import filters as F

# Average adult vowels: formant frequencies (Hz) and bandwidths (Hz), from the classic measurements of English
# speakers. Physics of the human vocal tract, not anyone's recording.
VOWELS = {
    'a': ([730, 1090, 2440, 3400, 4300], [90, 110, 140, 250, 300]),    # father
    'o': ([570, 840, 2410, 3400, 4300], [80, 100, 140, 250, 300]),     # thought
    'u': ([300, 870, 2240, 3400, 4300], [60, 90, 140, 250, 300]),      # boot
    'oo': ([440, 1020, 2240, 3400, 4300], [70, 100, 140, 250, 300]),   # book
    'e': ([530, 1840, 2480, 3400, 4300], [70, 110, 140, 250, 300]),    # bet
    'i': ([270, 2290, 3010, 3600, 4300], [60, 120, 160, 250, 300]),    # beet
    'ae': ([660, 1720, 2410, 3400, 4300], [90, 110, 140, 250, 300]),   # bat
    'uh': ([640, 1190, 2390, 3400, 4300], [80, 100, 140, 250, 300]),   # but
    'er': ([490, 1350, 1690, 3400, 4300], [70, 100, 120, 250, 300]),   # bird
    'h': ([700, 1200, 2500, 3500, 4500], [300, 300, 300, 350, 400]),   # an open, breathy tract
}
# The tract's higher resonances, the same for every vowel (scaled with the throat's size).
UPPER = ([5500.0, 6600.0, 7700.0], [500.0, 600.0, 700.0])

CTRL = 64  # samples between control points (1.3 ms)


def _track(keys, n, shift):
    """Formant frequency and bandwidth tracks (8 x control points) from (time, vowel) keys, gliding between them."""
    keys = sorted(keys, key=lambda k: k[0])
    idx = np.arange(0, n + CTRL, CTRL)
    t = idx / SR
    kt = np.array([k[0] for k in keys])
    Fs = np.array([VOWELS[k[1]][0] for k in keys], dtype=float)
    Bs = np.array([VOWELS[k[1]][1] for k in keys], dtype=float)
    if np.isscalar(shift):
        sh = np.full(t.size, float(shift))
    else:
        sh = np.interp(t, np.arange(len(shift)) / SR, shift)
    Fc = [np.exp(np.interp(t, kt, np.log(Fs[:, j]))) * sh for j in range(5)]
    Bc = [np.interp(t, kt, Bs[:, j]) * sh for j in range(5)]
    Fc += [f * sh for f in UPPER[0]]
    Bc += [b * sh for b in UPPER[1]]
    return idx, np.stack(Fc), np.stack(Bc)


# How much of each resonance's fall above its peak is taken back out (0: none). A series cut off after a few formants
# falls too steeply at the top, where the higher resonances it leaves out would hold the voice up; easing the upper
# formants' falls stands in for them.
EASE = [0.0, 0.0, 0.6, 0.9, 1.0, 1.0, 1.0, 1.0]


def formant_gain(freqs, Fc, Bc):
    """The tract's magnitude at freqs, for formants Fc and bandwidths Bc (arrays that broadcast together): a cascade
    of resonances F^2 / sqrt((F^2 - f^2)^2 + (B f)^2), the upper ones eased (EASE)."""
    f2 = freqs * freqs
    g = np.ones(np.broadcast(freqs, Fc[0]).shape)
    for j in range(Fc.shape[0]):
        Fj, Bj = Fc[j], Bc[j]
        res = (Fj * Fj) / np.sqrt((Fj * Fj - f2) ** 2 + (Bj * freqs) ** 2 + 1e-9)
        ease = EASE[min(j, len(EASE) - 1)]
        if ease > 0:
            res = res * np.maximum(1.0, f2 / (Fj * Fj)) ** ease
        g = g * res
    return g


def _tilt(freqs, tilt_db, ref=150.0):
    return np.where(freqs > ref, (np.maximum(freqs, 1.0) / ref) ** (tilt_db / 6.0206), 1.0)


def voice(f0, keys, r, breath=0.15, jitter=0.008, shimmer=0.06, tilt=0.0, shift=1.0, vib_rate=0.0, vib_depth=0.0,
          rough=0.0, top=None):
    """A voiced sound. f0: pitch per sample (its length is the sound's); keys: [(time, vowel), ...]; breath: the
    aspiration's share; jitter, shimmer: pitch and loudness wobble; tilt: dB per octave against an ordinary voice's
    source (-4 soft and breathy, +3 pressed and bright); shift: formants scaled (> 1 smaller throat, < 1 bigger; a
    number or an array); vib_rate, vib_depth (semitones): vibrato; rough: subharmonic growl (0..1); top: highest
    harmonic in Hz."""
    f0 = np.asarray(f0, dtype=float).copy()
    n = f0.size
    t = times(n)
    if jitter > 0:
        f0 *= 1.0 + jitter * smooth_random(n, r, 40.0) + 0.5 * jitter * smooth_random(n, r, 9.0)
    if vib_depth > 0 and vib_rate > 0:
        f0 *= 2.0 ** (vib_depth / 12.0 * np.sin(2 * np.pi * vib_rate * t + 2 * np.pi * r.random()))
    ph = 2.0 * np.pi * phase(f0, n)
    idx, Fc, Bc = _track(keys, n, shift)
    idx_c = np.minimum(idx, n - 1)
    f0c = f0[idx_c]
    limit = min(NYQ * 0.92, top or NYQ)
    slope = -6.0 + tilt
    out = np.zeros(n)
    k = 1
    while True:
        fk = k * f0c
        if fk.min() >= limit:
            break
        a = formant_gain(fk, Fc, Bc) * _tilt(fk, slope) * np.clip((limit - fk) / (0.05 * NYQ), 0.0, 1.0)
        out += np.interp(np.arange(n), idx_c, a) * np.sin(k * ph)
        k += 1
        if k > 400:
            break
    if rough > 0:
        # A subharmonic (period doubling), as in a growl or a strained throat.
        sub = 0.5 * f0c
        a = formant_gain(sub, Fc, Bc) * _tilt(sub, slope)
        out += rough * np.interp(np.arange(n), idx_c, a) * np.sin(0.5 * ph)
    if shimmer > 0:
        out *= 1.0 + shimmer * smooth_random(n, r, 30.0)
    out /= max(np.sqrt(np.mean(out * out)), 1e-9)
    if breath > 0:
        asp = aspiration(n, keys, r, shift)
        # Breath comes in puffs with each glottal cycle.
        asp *= 0.55 + 0.45 * np.cos(ph)
        asp /= max(np.sqrt(np.mean(asp * asp)), 1e-9)
        out = out * np.sqrt(1.0 - breath) + asp * np.sqrt(breath)
    return out


def aspiration(n, keys, r, shift=1.0, tilt=3.0, frame=1024):
    """Breath noise through the formants (a whisper of the vowels), unit RMS. Breath is brighter than voice: turbulent
    noise doesn't fall off like the glottal pulses, and the lips lift it (tilt, dB per octave over 400 Hz)."""
    n = int(n)
    idx, Fc, Bc = _track(keys, n, shift)
    tc = idx / SR

    def gain(freqs, tt):
        j = int(np.clip(np.searchsorted(tc, tt), 0, tc.size - 1))
        g = formant_gain(freqs, Fc[:, j:j + 1], Bc[:, j:j + 1])
        return g * _tilt(freqs, tilt, 400.0) * (freqs > 60)

    y = F.stft_filter(white(n, r), gain, frame)
    return y / max(np.sqrt(np.mean(y * y)), 1e-9)


def whisper(dur, keys, r, shift=1.0, tilt=3.0):
    return aspiration(ns(dur), keys, r, shift, tilt)


def hiss(n, r, centers=(2600.0, 4300.0, 6800.0), qs=(3.0, 3.0, 2.5), levels=(0.6, 1.0, 0.7), lo=1200.0):
    """A sibilant hiss: noise through a few sharp resonances (teeth and tongue), unit RMS."""
    x = white(int(n), r)
    y = np.zeros_like(x)
    for c, q, lv in zip(centers, qs, levels):
        y += lv * F.filt(x, F.bp(c, q), extend=False)
    y = F.filt(y, F.hp(lo, 0.7), extend=False)
    return y / max(np.sqrt(np.mean(y * y)), 1e-9)
