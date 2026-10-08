"""Saturation and distortion, run oversampled so the new harmonics they make don't fold back as aliasing."""
import numpy as np

from .core import length, db2a


def _up(x, factor):
    n = length(x)
    X = np.fft.rfft(x, axis=-1)
    m = n * factor
    Y = np.zeros(x.shape[:-1] + (m // 2 + 1,), dtype=complex)
    Y[..., :X.shape[-1]] = X * factor
    return np.fft.irfft(Y, m, axis=-1)


def _down(y, factor, n):
    Y = np.fft.rfft(y, axis=-1)
    keep = n // 2 + 1
    X = Y[..., :keep] / factor
    # A short taper under the old Nyquist so the brick wall doesn't ring.
    edge = max(8, keep // 24)
    X[..., keep - edge:] *= np.linspace(1.0, 0.0, edge)
    return np.fft.irfft(X, n, axis=-1)


def oversampled(x, fn, factor=4):
    """fn applied at factor times the rate, then filtered back down."""
    pad = 256
    n = length(x)
    xp = np.pad(x, [(0, 0)] * (x.ndim - 1) + [(pad, pad)])
    m = length(xp)
    if m % 2:
        xp = np.pad(xp, [(0, 0)] * (x.ndim - 1) + [(0, 1)])
        m += 1
    y = _down(fn(_up(xp, factor)), factor, m)
    return y[..., pad:pad + n]


def drive(x, db=12.0, kind='tanh', bias=0.0, factor=4, mix=1.0):
    """Saturates x pushed by db of gain. kind: 'tanh' (warm), 'soft' (x/(1+|x|), gentler), 'hard' (clipped),
    'fold' (folds back over itself: buzzy, metallic), 'cubic' (a soft knee that stays clean longer). bias makes it
    lopsided (even harmonics, like a strained voice or a tube); its offset is taken out again."""
    g = float(db2a(db))

    def shape(u):
        v = u * g + bias
        if kind == 'soft':
            return v / (1.0 + np.abs(v)) - bias / (1.0 + abs(bias))
        if kind == 'hard':
            return np.clip(v, -1.0, 1.0) - np.clip(bias, -1.0, 1.0)
        if kind == 'fold':
            return np.sin(v * np.pi / 2.0) - np.sin(bias * np.pi / 2.0)
        if kind == 'cubic':
            c = np.clip(v, -1.0, 1.0)
            cb = np.clip(bias, -1.0, 1.0)
            return (c - c ** 3 / 3.0) * 1.5 - (cb - cb ** 3 / 3.0) * 1.5
        return np.tanh(v) - np.tanh(bias)

    y = oversampled(x, shape, factor)
    if mix < 1.0:
        p_in = np.max(np.abs(x)) or 1.0
        p_out = np.max(np.abs(y)) or 1.0
        y = (1.0 - mix) * x / p_in + mix * y / p_out
    return y


def ring_mod(x, carrier):
    """x times a carrier signal (same length): sidebands that sound metallic or not quite alive."""
    return x * carrier[..., :length(x)]


def crush(x, bits=8):
    """Coarse amplitude steps (grit, radio, broken things), oversampled to keep it clean of aliasing."""
    q = 2.0 ** (bits - 1)
    return oversampled(x, lambda u: np.round(u * q) / q, 4)
