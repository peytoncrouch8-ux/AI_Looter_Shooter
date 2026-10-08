"""Reading and writing 16-bit PCM WAV files (Python's wave module), with triangular dither on the way down."""
import wave

import numpy as np

from .core import SR


def write(path, x, seed=0):
    """x: float samples in [-1, 1], mono (n,) or stereo (2, n)."""
    x = np.atleast_2d(np.asarray(x, dtype=float))
    ch, n = x.shape
    r = np.random.default_rng(seed)
    # TPDF dither of one step peak to peak: the rounding error becomes a steady, faint hiss instead of distortion.
    d = (r.random((ch, n)) - r.random((ch, n)))
    q = np.round(x * 32767.0 + d)
    q = np.clip(q, -32768, 32767).astype('<i2')
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(ch)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(q.T.reshape(-1).tobytes())


def read(path):
    """Returns (samples as floats, mono (n,) or (channels, n); the sample rate)."""
    with wave.open(str(path), 'rb') as w:
        ch = w.getnchannels()
        sw = w.getsampwidth()
        rate = w.getframerate()
        raw = w.readframes(w.getnframes())
    if sw != 2:
        raise ValueError(f'{path}: {sw * 8}-bit, expected 16')
    a = np.frombuffer(raw, dtype='<i2').astype(float) / 32768.0
    a = a.reshape(-1, ch).T
    return (a[0] if ch == 1 else a), rate
