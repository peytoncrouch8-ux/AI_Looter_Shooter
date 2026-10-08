"""Pictures of sounds: a waveform with its level in dB, and a log-frequency spectrogram, written as PNG with zlib (no
image library needed). A tiny 5x7 pixel font labels them.
"""
import struct
import zlib

import numpy as np

from .core import SR

FONT = {
    '0': [14, 17, 19, 21, 25, 17, 14], '1': [4, 12, 4, 4, 4, 4, 14], '2': [14, 17, 1, 2, 4, 8, 31],
    '3': [31, 2, 4, 2, 1, 17, 14], '4': [2, 6, 10, 18, 31, 2, 2], '5': [31, 16, 30, 1, 1, 17, 14],
    '6': [6, 8, 16, 30, 17, 17, 14], '7': [31, 1, 2, 4, 8, 8, 8], '8': [14, 17, 17, 14, 17, 17, 14],
    '9': [14, 17, 17, 15, 1, 2, 12], 'A': [14, 17, 17, 17, 31, 17, 17], 'B': [30, 17, 17, 30, 17, 17, 30],
    'C': [14, 17, 16, 16, 16, 17, 14], 'D': [28, 18, 17, 17, 17, 18, 28], 'E': [31, 16, 16, 30, 16, 16, 31],
    'F': [31, 16, 16, 30, 16, 16, 16], 'G': [14, 17, 16, 23, 17, 17, 15], 'H': [17, 17, 17, 31, 17, 17, 17],
    'I': [14, 4, 4, 4, 4, 4, 14], 'J': [7, 2, 2, 2, 2, 18, 12], 'K': [17, 18, 20, 24, 20, 18, 17],
    'L': [16, 16, 16, 16, 16, 16, 31], 'M': [17, 27, 21, 21, 17, 17, 17], 'N': [17, 17, 25, 21, 19, 17, 17],
    'O': [14, 17, 17, 17, 17, 17, 14], 'P': [30, 17, 17, 30, 16, 16, 16], 'Q': [14, 17, 17, 17, 21, 18, 13],
    'R': [30, 17, 17, 30, 20, 18, 17], 'S': [15, 16, 16, 14, 1, 1, 30], 'T': [31, 4, 4, 4, 4, 4, 4],
    'U': [17, 17, 17, 17, 17, 17, 14], 'V': [17, 17, 17, 17, 17, 10, 4], 'W': [17, 17, 17, 21, 21, 21, 10],
    'X': [17, 17, 10, 4, 10, 17, 17], 'Y': [17, 17, 17, 10, 4, 4, 4], 'Z': [31, 1, 2, 4, 8, 16, 31],
    '.': [0, 0, 0, 0, 0, 12, 12], '-': [0, 0, 0, 31, 0, 0, 0], '_': [0, 0, 0, 0, 0, 0, 31],
    ':': [0, 12, 12, 0, 12, 12, 0], '%': [24, 25, 2, 4, 8, 19, 3], '/': [0, 1, 2, 4, 8, 16, 0],
    '(': [2, 4, 8, 8, 8, 4, 2], ')': [8, 4, 2, 2, 2, 4, 8], '+': [0, 4, 4, 31, 4, 4, 0],
    '=': [0, 0, 31, 0, 31, 0, 0], '!': [4, 4, 4, 4, 4, 0, 4], ' ': [0] * 7,
}


class Canvas:
    def __init__(self, w, h, color=(18, 18, 22)):
        self.w, self.h = int(w), int(h)
        self.px = np.zeros((self.h, self.w, 3), dtype=np.uint8)
        self.px[:, :] = color

    def rect(self, x0, y0, x1, y1, color):
        x0, x1 = max(0, int(x0)), min(self.w, int(x1))
        y0, y1 = max(0, int(y0)), min(self.h, int(y1))
        if x1 > x0 and y1 > y0:
            self.px[y0:y1, x0:x1] = color

    def blend_rect(self, x0, y0, x1, y1, color, alpha):
        x0, x1 = max(0, int(x0)), min(self.w, int(x1))
        y0, y1 = max(0, int(y0)), min(self.h, int(y1))
        if x1 > x0 and y1 > y0:
            region = self.px[y0:y1, x0:x1].astype(float)
            self.px[y0:y1, x0:x1] = (region * (1 - alpha) + np.array(color) * alpha).astype(np.uint8)

    def text(self, x, y, s, color=(230, 230, 230), scale=1):
        cx = int(x)
        for ch in str(s).upper():
            rows = FONT.get(ch, FONT[' '])
            for j, bits in enumerate(rows):
                for i in range(5):
                    if bits & (1 << (4 - i)):
                        self.rect(cx + i * scale, y + j * scale, cx + (i + 1) * scale, y + (j + 1) * scale, color)
            cx += 6 * scale

    def image(self, x, y, img):
        h, w = img.shape[:2]
        h = min(h, self.h - y)
        w = min(w, self.w - x)
        if h > 0 and w > 0:
            self.px[y:y + h, x:x + w] = img[:h, :w]

    def save(self, path):
        raw = b''.join(b'\x00' + self.px[j].tobytes() for j in range(self.h))

        def chunk(tag, data):
            c = struct.pack('>I', len(data)) + tag + data
            return c + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)

        png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', self.w, self.h, 8, 2, 0, 0, 0))
        png += chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b'')
        with open(path, 'wb') as f:
            f.write(png)


# A dark-to-bright heat palette: black, deep violet, red, orange, pale yellow.
_STOPS = np.array([[0.0, 6, 6, 10], [0.25, 60, 16, 90], [0.5, 180, 40, 60], [0.75, 245, 140, 30], [1.0, 255, 250, 210]])


def heat(v):
    """Values 0..1 to RGB."""
    v = np.clip(v, 0.0, 1.0)
    out = np.zeros(v.shape + (3,))
    for c in range(3):
        out[..., c] = np.interp(v, _STOPS[:, 0], _STOPS[:, c + 1])
    return out.astype(np.uint8)


F_LO, F_HI = 30.0, 22000.0


def spectrogram(x, w, h, dur, floor_db=-90.0, ref=None):
    """A log-frequency spectrogram image (h, w, 3) of mono x over dur seconds (x may be shorter: the rest is dark).
    Levels are dB under ref (the loudest bin of the sound by default)."""
    if dur <= 0.15:
        nfft = 256
    elif dur <= 0.6:
        nfft = 512
    elif dur <= 3.0:
        nfft = 1024
    else:
        nfft = 2048
    win = np.hanning(nfft)
    pad = np.concatenate([np.zeros(nfft // 2), x, np.zeros(nfft)])
    cols = np.zeros((nfft // 2 + 1, w))
    for i in range(w):
        center = int((i + 0.5) / w * dur * SR)
        if center >= len(x) + nfft // 2:
            break
        seg = pad[center:center + nfft]
        if seg.size < nfft:
            seg = np.pad(seg, (0, nfft - seg.size))
        cols[:, i] = np.abs(np.fft.rfft(seg * win)) ** 2
    f = np.fft.rfftfreq(nfft, 1.0 / SR)
    rows = F_LO * (F_HI / F_LO) ** (np.arange(h)[::-1] / (h - 1))
    img = np.zeros((h, w))
    for i in range(w):
        img[:, i] = np.interp(rows, f, cols[:, i])
    db = 10 * np.log10(img + 1e-20)
    top = ref if ref is not None else db.max()
    v = (db - (top + floor_db)) / (-floor_db)
    return heat(v), top


def freq_row(fr, h):
    return int(round((1.0 - np.log(fr / F_LO) / np.log(F_HI / F_LO)) * (h - 1)))


def waveform(c, x, x0, y0, w, h, dur, color=(120, 190, 255), env_color=(255, 170, 60)):
    """Draws min/max sample bars per column and the RMS level in dB (-60..0) as a line."""
    mid = y0 + h // 2
    c.rect(x0, mid, x0 + w, mid + 1, (60, 60, 70))
    spc = dur * SR / w
    prev = None
    for i in range(w):
        a = int(i * spc)
        b = int((i + 1) * spc)
        if a >= len(x):
            break
        seg = x[a:max(b, a + 1)]
        lo, hi = float(seg.min()), float(seg.max())
        c.rect(x0 + i, mid - int(hi * h / 2), x0 + i + 1, mid - int(lo * h / 2) + 1, color)
        rms = np.sqrt(np.mean(seg * seg)) + 1e-9
        lv = np.clip((20 * np.log10(rms) + 60) / 60, 0, 1)
        yy = y0 + h - 1 - int(lv * (h - 1))
        if prev is not None:
            c.rect(x0 + i, min(prev, yy), x0 + i + 1, max(prev, yy) + 1, env_color)
        prev = yy
