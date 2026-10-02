#!/usr/bin/env python3
"""Art-style exploration: composes art styles of one model from its render passes (Docs/Art/StyleExploration).

    python Tools/Blender/style_compose.py <passdir> <outdir> [--only 1,3] [--plugins a.py,b.py] [--no-builtin]
    blender -b --factory-startup --python Tools/Blender/style_compose.py -- <passdir> <outdir>

The passes come from style_render.py (linear EXR); it needs numpy, OpenCV (opencv-contrib-python-headless) and
Pillow in the Python that runs it, and bpy to read the EXR files. Every style is built from the same passes, so the
comparison is fair: the albedo (what the textures are), the sun's shading and shadows, the sky light, ambient
occlusion, world normals, depth and mist, and two Freestyle line passes (clean and sketchy). Each style gets a
1600x900 PNG and the contact sheet puts them side by side with labels.

A style is a function compose(sc) -> float32 RGB (H, W, 3) in display space 0..1, where sc (load_scene) holds:
    H, W           image size; S = W / 1600, the scale for every pixel size tuned at 1600 px wide
    alpha          geometry coverage (0 = sky); geo = alpha > 0.5; house / ground masks (by material index)
    albedo         the textures' color, sRGB 0..1 (albedo_lin is linear); sky pixels hold the mean color
    beauty_lin     the lit render, linear, straight alpha (tonemap() it); beauty_png the AgX PNG, RGBA 0..1
    sun_n          the sun's shading with its cast shadows, 0..1 (1 = facing the sun, unshadowed)
    ambient_n      sky light and bounce, 0..1; ao ambient occlusion (1 = open); gloss_n the sun's specular
    normal         world-space normals; L the direction toward the sun; V toward the camera; NdotL, NdotV
    rim            (1 - NdotV)^3 on geometry; in_shadow 1 inside cast shadows (and on faces turned away)
    mist           0 near .. 1 far (1 in the sky); depth in meters; horizon the sky's last row
    lines_clean    Freestyle ink coverage 0..1 (clean, 2 px); lines_sketch the sketchy pass
A plugin file defines STYLE = dict(name=..., reference=..., note=...) and compose(sc, lib), lib being this module
(its helpers: meanshift, quantize, hsv_adjust, chroma, contrast, sky_gradient, cloud_mask, cumulus, paper, canvas,
hatch, halftone, brush_strokes, flatten_region, grain, vignette, edge_mag, dilate_mask, erode_mask, ink, haze, comp,
mix, smoothstep, blur, hexc, tonemap, srgb, to8, f32, value_noise, billow, shifted, lum).
"""
import json
import math
import os
import sys

os.environ.setdefault('OPENCV_IO_ENABLE_OPENEXR', '1')
import cv2  # noqa: E402
import numpy as np  # noqa: E402
from PIL import Image, ImageDraw, ImageFont  # noqa: E402

from types import SimpleNamespace  # noqa: E402


# ---------------------------------------------------------------- loading
def load_exr(path):
    """A render pass as float32 (h, w, c), row 0 at the top; bpy reads any EXR channel names (XYZ normals too)."""
    import bpy
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    n = img.channels
    buf = np.empty(w * h * n, np.float32)
    img.pixels.foreach_get(buf)
    bpy.data.images.remove(img)
    return buf.reshape(h, w, n)[::-1].copy()


def load_pass(passdir, name, optional=False):
    path = os.path.join(passdir, f'{name}0001.exr')
    if not os.path.exists(path):
        if optional:
            return None
        raise FileNotFoundError(path)
    return load_exr(path)


# ---------------------------------------------------------------- color and image helpers
def lum(rgb):
    return rgb[..., 0] * 0.2126 + rgb[..., 1] * 0.7152 + rgb[..., 2] * 0.0722


def srgb(x):
    x = np.clip(x, 0.0, 1.0)
    return np.where(x <= 0.0031308, x * 12.92, 1.055 * np.power(x, 1.0 / 2.4) - 0.055).astype(np.float32)


def aces(x):
    a, b, c, d, e = 2.51, 0.03, 2.43, 0.59, 0.14
    return np.clip((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0)


def tonemap(linear, exposure=1.0):
    return srgb(aces(np.clip(linear, 0, None) * exposure))


def to8(x):
    return (np.clip(x, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)


def to8_dither(x, seed=11):
    rng = np.random.default_rng(seed)
    d = (rng.random(x.shape, dtype=np.float32) - 0.5) / 255.0
    return (np.clip(np.clip(x, 0.0, 1.0) + d, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)


def f32(x8):
    return x8.astype(np.float32) / 255.0


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0 + 1e-9), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def _is_color(x):
    return x.ndim == 3 or (x.ndim == 1 and x.shape[0] == 3)


def mix(a, b, t):
    a = np.asarray(a, np.float32)
    b = np.asarray(b, np.float32)
    t = np.asarray(t, np.float32)
    if t.ndim == 2 and (_is_color(a) or _is_color(b)):
        t = t[..., None]
    return (a + (b - a) * t).astype(np.float32)


def hexc(s):
    s = s.lstrip('#')
    return np.array([int(s[i:i + 2], 16) / 255.0 for i in (0, 2, 4)], np.float32)


def blur(img, sigma):
    if sigma <= 0.05:
        return img
    return cv2.GaussianBlur(img, (0, 0), sigma, borderType=cv2.BORDER_REFLECT)


def hsv_adjust(rgb, sat=1.0, val=1.0, hue=0.0):
    hsv = cv2.cvtColor(np.clip(rgb, 0, 1).astype(np.float32), cv2.COLOR_RGB2HSV)
    hsv[..., 0] = (hsv[..., 0] + hue) % 360.0
    hsv[..., 1] = np.clip(hsv[..., 1] * sat, 0, 1)
    hsv[..., 2] = np.clip(hsv[..., 2] * val, 0, 1)
    return cv2.cvtColor(hsv, cv2.COLOR_HSV2RGB)


def chroma(rgb, amount, lightness=1.0):
    lab = cv2.cvtColor(np.clip(rgb, 0, 1).astype(np.float32), cv2.COLOR_RGB2Lab)
    lab[..., 0] = np.clip(lab[..., 0] * lightness, 0, 100)
    lab[..., 1:] *= amount
    return np.clip(cv2.cvtColor(lab, cv2.COLOR_Lab2RGB), 0, 1)


def contrast(rgb, amount, pivot=0.5):
    return np.clip((rgb - pivot) * amount + pivot, 0, 1).astype(np.float32)


def meanshift(rgb8, sp, sr):
    return cv2.pyrMeanShiftFiltering(rgb8, max(1, int(round(sp))), sr)


def quantize(rgb8, k, mask=None, seed=1):
    """k-means palette on the masked pixels; every pixel snaps to its nearest color. Returns (image 0..1, palette)."""
    h, w, _ = rgb8.shape
    pix = rgb8.reshape(-1, 3).astype(np.float32)
    sel = pix if mask is None else pix[mask.reshape(-1)]
    rng = np.random.default_rng(seed)
    idx = rng.choice(len(sel), size=min(len(sel), 80000), replace=False)
    crit = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 40, 0.25)
    _, _, centers = cv2.kmeans(sel[idx], k, None, crit, 4, cv2.KMEANS_PP_CENTERS)
    d = (pix * pix).sum(1)[:, None] - 2.0 * pix @ centers.T + (centers * centers).sum(1)[None, :]
    q = centers[d.argmin(1)]
    return (q.reshape(h, w, 3) / 255.0).astype(np.float32), centers / 255.0


def value_noise(h, w, scale, seed, octaves=4, persistence=0.5, aspect=1.0):
    """Smooth fractal noise in 0..1; scale is the first octave's cell size in px (cells aspect times wider than tall)."""
    rng = np.random.default_rng(seed)
    out = np.zeros((h, w), np.float32)
    amp, total = 1.0, 0.0
    for _ in range(octaves):
        gw = max(2, int(w / scale)) + 1
        gh = max(2, int(h / (scale / aspect))) + 1
        g = rng.random((gh, gw)).astype(np.float32)
        out += amp * cv2.resize(g, (w, h), interpolation=cv2.INTER_CUBIC)
        total += amp
        amp *= persistence
        scale /= 2.0
    out /= total
    return (out - out.min()) / (out.max() - out.min() + 1e-6)


def billow(h, w, scale, seed, octaves=5, persistence=0.6, aspect=2.0):
    """Fractal noise folded into puffs (cumulus shapes), 0..1."""
    rng = np.random.default_rng(seed)
    out = np.zeros((h, w), np.float32)
    amp, total = 1.0, 0.0
    for _ in range(octaves):
        gw = max(2, int(w / scale)) + 1
        gh = max(2, int(h / (scale / aspect))) + 1
        g = rng.random((gh, gw)).astype(np.float32)
        n = cv2.resize(g, (w, h), interpolation=cv2.INTER_CUBIC)
        out += amp * (1.0 - np.abs(2.0 * n - 1.0))
        total += amp
        amp *= persistence
        scale /= 2.0
    out /= total
    return (out - out.min()) / (out.max() - out.min() + 1e-6)


def sky_gradient(h, w, horizon, stops):
    """Vertical gradient: t = 0 at the horizon row, 1 at the top. Stops: (t, '#hex' or rgb or grey)."""
    y = np.arange(h, dtype=np.float32)
    t = np.clip((horizon - y) / max(horizon, 1), 0.0, 1.0)
    ts = np.array([s[0] for s in stops], np.float32)
    cols = []
    for _, c in stops:
        c = hexc(c) if isinstance(c, str) else np.asarray(c, np.float32)
        cols.append(np.full(3, float(c)) if c.ndim == 0 else c)
    cols = np.stack(cols)
    col = np.stack([np.interp(t, ts, cols[:, c]) for c in range(3)], -1).astype(np.float32)
    return np.repeat(col[:, None, :], w, axis=1)


def _sky_band(h, horizon, band):
    y = np.arange(h, dtype=np.float32)[:, None] / max(horizon, 1)
    return smoothstep(band[0] - 0.08, band[0] + 0.08, y) * smoothstep(band[1] + 0.03, band[1] - 0.15, y)


def cloud_mask(h, w, horizon, seed, scale, coverage, softness, aspect=2.4, band=(0.08, 0.95)):
    """Flat clouds in the sky band (0 = top, 1 = horizon): a soft 0..1 mask."""
    n = value_noise(h, w, scale, seed, octaves=5, persistence=0.55, aspect=aspect)
    thr = 1.0 - coverage
    m = smoothstep(thr - softness, thr + softness, n)
    return (m * _sky_band(h, horizon, band)).astype(np.float32)


def cumulus(h, w, horizon, seed, scale, coverage, softness, aspect=2.0, band=(0.05, 0.9), light=(-0.8, -1.0)):
    """Puffy clouds: (mask, lit). lit is 1 on the edges facing the light (image direction light), 0 underneath."""
    n = billow(h, w, scale, seed, aspect=aspect)
    thr = 1.0 - coverage
    m = smoothstep(thr - softness, thr + softness, n) * _sky_band(h, horizon, band)
    thick = blur(m, scale * 0.07)
    shift = scale * 0.11
    toward_light = shifted(thick, -light[0] * shift, -light[1] * shift)
    lit = np.clip(0.5 + (thick - toward_light) * 3.0, 0, 1)
    lit = np.clip(lit * 0.7 + 0.3 * (1.0 - thick), 0, 1)       # thin wisps stay bright
    return m.astype(np.float32), lit.astype(np.float32)


def shifted(img, dx, dy):
    m = np.float32([[1, 0, dx], [0, 1, dy]])
    return cv2.warpAffine(img, m, (img.shape[1], img.shape[0]), borderMode=cv2.BORDER_REFLECT)


def paper(h, w, seed, fiber=0.06, mottle=0.05):
    rng = np.random.default_rng(seed)
    fine = blur(rng.standard_normal((h, w)).astype(np.float32), 0.8)
    fine /= fine.std() + 1e-6
    mot = value_noise(h, w, 90, seed + 1, octaves=3) - 0.5
    return (1.0 + fiber * fine * 0.5 + mottle * mot * 2.0).astype(np.float32)


def canvas(h, w, seed, scale=3.0):
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    weave = np.sin(xx / scale * math.pi) * np.sin(yy / scale * math.pi)
    n = value_noise(h, w, 5, seed, octaves=2) - 0.5
    return (weave * 0.5 + n * 0.8).astype(np.float32)


def hatch(h, w, angle_deg, spacing, thickness, seed, wobble=0.9):
    """Parallel ink lines (1 on a line): spacing and thickness in px, slightly wavy."""
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    a = math.radians(angle_deg)
    d = xx * math.cos(a) + yy * math.sin(a)
    d += (value_noise(h, w, 60, seed, octaves=2) - 0.5) * wobble * spacing
    phase = d % spacing
    dist = np.minimum(phase, spacing - phase)
    return smoothstep(thickness * 0.5 + 0.6, thickness * 0.5 - 0.4, dist)


def halftone(darkness, period, angle_deg, gain=1.1):
    """Print dots: darkness 0..1 sets each dot's area. Returns the dot mask (1 = ink)."""
    h, w = darkness.shape
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    a = math.radians(angle_deg)
    u = xx * math.cos(a) + yy * math.sin(a)
    v = -xx * math.sin(a) + yy * math.cos(a)
    cu = (u / period - np.floor(u / period + 0.5)) * period
    cv_ = (v / period - np.floor(v / period + 0.5)) * period
    dist = np.sqrt(cu * cu + cv_ * cv_)
    d = blur(darkness, period * 0.45)
    r = period * np.sqrt(np.clip(d, 0, 1) / math.pi) * gain
    return smoothstep(r + 0.7, r - 0.5, dist)


def brush_strokes(img, length, nbins=8, sigma=3.0):
    """Smears each pixel along the local edge direction: paint strokes that follow the forms."""
    g = blur(cv2.cvtColor(to8(img), cv2.COLOR_RGB2GRAY).astype(np.float32) / 255.0, 1.0)
    gx = cv2.Sobel(g, cv2.CV_32F, 1, 0, ksize=3)
    gy = cv2.Sobel(g, cv2.CV_32F, 0, 1, ksize=3)
    mag = np.sqrt(gx * gx + gy * gy) + 1e-6
    ang = np.arctan2(gy, gx) + math.pi / 2.0                       # along the edge, not across it
    c = blur(np.cos(2.0 * ang) * mag, sigma)
    s_ = blur(np.sin(2.0 * ang) * mag, sigma)
    tang = 0.5 * np.arctan2(s_, c)
    bins = np.rint((tang + math.pi / 2.0) / math.pi * nbins).astype(np.int32) % nbins
    out = np.zeros_like(img)
    size = max(3, int(round(length)) | 1)
    half = size // 2
    for b in range(nbins):
        theta = -math.pi / 2.0 + b * math.pi / nbins
        k = np.zeros((size, size), np.uint8)
        dx, dy = math.cos(theta) * half, math.sin(theta) * half
        cv2.line(k, (int(round(half - dx)), int(round(half - dy))), (int(round(half + dx)), int(round(half + dy))), 255, 1,
                 cv2.LINE_AA)
        kf = k.astype(np.float32)
        kf /= kf.sum()
        f = cv2.filter2D(img, -1, kf, borderType=cv2.BORDER_REFLECT)
        sel = bins == b
        out[sel] = f[sel]
    return out


def flatten_region(img, region, sigma, amount):
    return mix(img, blur(img, sigma), region.astype(np.float32) * amount)


def grain(img, amount, seed=3):
    rng = np.random.default_rng(seed)
    g = rng.standard_normal(img.shape[:2]).astype(np.float32)[..., None]
    return np.clip(img + g * amount, 0, 1)


def vignette(img, amount=0.25, power=2.0):
    h, w = img.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    r = np.sqrt(((xx / w - 0.5) * 2) ** 2 + ((yy / h - 0.5) * 2) ** 2) / math.sqrt(2)
    return np.clip(img * (1 - amount * r ** power)[..., None], 0, 1)


def edge_mag(rgb, sigma=0.0):
    g = cv2.cvtColor(to8(rgb), cv2.COLOR_RGB2GRAY).astype(np.float32) / 255.0
    g = blur(g, sigma)
    sx = cv2.Sobel(g, cv2.CV_32F, 1, 0, ksize=3)
    sy = cv2.Sobel(g, cv2.CV_32F, 0, 1, ksize=3)
    return np.sqrt(sx * sx + sy * sy)


def dilate_mask(m, px):
    if px <= 0:
        return m
    k = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * px + 1, 2 * px + 1))
    return cv2.dilate(m, k)


def erode_mask(m, px):
    if px <= 0:
        return m
    k = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * px + 1, 2 * px + 1))
    return cv2.erode(m, k)


def ink(out, line_alpha, color, strength=1.0):
    return mix(out, np.asarray(color, np.float32), np.clip(line_alpha * strength, 0, 1))


def haze(fill, mist, color, strength, start=0.04, end=1.0):
    return mix(fill, np.asarray(color, np.float32), smoothstep(start, end, mist) * strength)


def comp(fill, sky, alpha):
    return mix(sky, fill, alpha)


# ---------------------------------------------------------------- the scene, from the passes
def load_scene(passdir):
    """Reads the passes and derives what the styles shade with (see the module docstring)."""
    print('STYLE: loading passes', flush=True)
    P = {name: load_pass(passdir, name) for name in (
        'full_Image', 'full_Alpha', 'full_Depth', 'full_Mist', 'full_Normal', 'full_DiffCol', 'full_DiffDir',
        'full_DiffInd', 'full_GlossDir', 'full_AO', 'full_IndexMA', 'sun_DiffDir', 'sun_Image')}
    P['full_Freestyle'] = load_pass(passdir, 'full_Freestyle', optional=True)
    P['sun_Freestyle'] = load_pass(passdir, 'sun_Freestyle', optional=True)
    scene_path = os.path.join(passdir, 'scene.json')
    scene = json.load(open(scene_path)) if os.path.exists(scene_path) else {}

    alpha = np.clip(P['full_Alpha'][..., 0], 0, 1)
    H, W = alpha.shape
    S = W / 1600.0
    a_safe = np.maximum(alpha, 1e-3)[..., None]
    geo = alpha > 0.5
    albedo_lin = np.clip(P['full_DiffCol'][..., :3] / a_safe, 0, 1)
    albedo_lin[~geo] = albedo_lin[geo].mean(axis=0)   # no dark halo when filters run over the sky
    albedo = srgb(albedo_lin)
    beauty_lin = np.clip(P['full_Image'][..., :3] / a_safe, 0, None)
    beauty_lin[~geo] = beauty_lin[geo].mean(axis=0)
    sun = lum(P['sun_DiffDir'][..., :3] / a_safe)
    sun_n = np.clip(sun / np.percentile(sun[geo], 99.0), 0, 1)
    sky_direct = np.clip(P['full_DiffDir'][..., :3] - P['sun_DiffDir'][..., :3], 0, None) / a_safe
    ambient = lum(sky_direct + P['full_DiffInd'][..., :3] / a_safe)
    ambient_n = np.clip(ambient / np.percentile(ambient[geo], 98.0), 0, 1)
    ao = np.clip(P['full_AO'][..., 0] / a_safe[..., 0], 0, 1)
    gloss = lum(P['full_GlossDir'][..., :3] / a_safe)
    gloss_n = np.clip(gloss / max(np.percentile(gloss[geo], 99.5), 1e-4), 0, 1)
    normal = P['full_Normal'][..., :3].copy()
    normal /= np.maximum(np.linalg.norm(normal, axis=-1, keepdims=True), 1e-6)
    normal[~geo] = 0.0
    depth = P['full_Depth'][..., 0].copy()
    depth[depth > 1e9] = depth[depth <= 1e9].max()
    mist = np.clip(P['full_Mist'][..., 0], 0, 1)
    mist[~geo] = 1.0
    matid = np.rint(P['full_IndexMA'][..., 0]).astype(np.int32)
    ground = (matid == 100) & geo
    house = (matid >= 1) & (matid < 100) & geo
    lines_clean = P['full_Freestyle'][..., 3] if P['full_Freestyle'] is not None else np.zeros((H, W), np.float32)
    lines_sketch = P['sun_Freestyle'][..., 3] if P['sun_Freestyle'] is not None else lines_clean
    beauty_png = np.asarray(Image.open(os.path.join(passdir, 'beauty_current.png')).convert('RGBA'), np.float32) / 255.0

    # The sun and camera as style_render.py placed them (scene.json), else its defaults.
    L = np.array(scene.get('sun', [-0.775, -0.320, 0.545]), np.float32)
    cam_forward = np.array(scene.get('cam_forward', [0.566, 0.821, 0.069]), np.float32)
    V = -cam_forward
    NdotL = np.clip(normal @ L, 0, 1)
    NdotV = np.clip(normal @ V, 0, 1)
    rim = (1.0 - NdotV) ** 3 * geo
    # Cast shadows: the sun's shading divided by what an unshadowed surface at that angle would get.
    ratio = sun / np.maximum(NdotL, 1e-3)
    k_sun = np.percentile(ratio[geo & (NdotL > 0.4)], 90)
    lit_frac = np.clip(sun / (NdotL * k_sun + 1e-4), 0, 1)
    lit_frac[NdotL < 0.03] = 0.0
    in_shadow = 1.0 - smoothstep(0.3, 0.65, lit_frac)       # 1 in cast shadow (or facing away)
    # Horizon: the first row where nearly every pixel is geometry.
    rows = np.where((alpha > 0.5).mean(axis=1) > 0.97)[0]
    horizon = int(rows.min()) if len(rows) else H // 2
    print(f'STYLE: {W}x{H}, horizon row {horizon}, sun ref {np.percentile(sun[geo], 99):.2f}, k_sun {k_sun:.2f}',
          flush=True)
    return SimpleNamespace(passdir=passdir, alpha=alpha, H=H, W=W, S=S, geo=geo, albedo_lin=albedo_lin, albedo=albedo,
                           beauty_lin=beauty_lin, beauty_png=beauty_png, sun_n=sun_n, ambient_n=ambient_n, ao=ao,
                           gloss_n=gloss_n, normal=normal, depth=depth, mist=mist, matid=matid, ground=ground,
                           house=house, lines_clean=lines_clean, lines_sketch=lines_sketch, L=L, V=V,
                           cam_forward=cam_forward, NdotL=NdotL, NdotV=NdotV, rim=rim, lit_frac=lit_frac,
                           in_shadow=in_shadow, horizon=horizon)


STYLES = []  # the built-in styles: (number, name, reference, note, compose)


def style(number, name, reference, note):
    def register(fn):
        STYLES.append((number, name, reference, note, fn))
        return fn
    return register


# ---------------------------------------------------------------- 0. the current look
@style(0, 'Current look', 'what the kit renders today (stylized realism)',
       'Textured PBR, soft sun, no lines. The control tile.')
def current(sc):
    sky = sky_gradient(sc.H, sc.W, sc.horizon, [(0.0, '#d2d6d4'), (0.45, '#9fb9d8'), (1.0, '#6f93c4')])
    fill = sc.beauty_png[..., :3]
    fill = haze(fill, sc.mist, hexc('#c9cfcf'), 0.75)
    return comp(fill, sky, sc.beauty_png[..., 3])


# ---------------------------------------------------------------- 1. ink and flat color (Sable)
@style(1, 'Ink & flat color', 'Sable / Moebius',
       'Flat fills, hard two-tone shade, bold ink lines, stipple, pastel sky. Textures carry no detail.')
def ink_flat(sc):
    flat8 = meanshift(to8(sc.albedo), 16 * sc.S, 30)
    fill, _ = quantize(flat8, 10, mask=sc.geo, seed=4)
    fill = hsv_adjust(fill, sat=1.1, val=1.0)
    fill = np.clip(fill * 0.82 + 0.17, 0, 1)                      # lift toward pastel
    fill = fill * np.array([1.03, 0.99, 0.96], np.float32)         # warm paper cast
    lit = smoothstep(0.42, 0.5, sc.sun_n)
    shade = fill * np.array([0.70, 0.64, 0.86], np.float32)        # violet shade
    out = mix(shade, fill, lit)
    out *= mix(0.86, 1.0, smoothstep(0.2, 0.9, sc.ao))[..., None]
    # Stipple inside the shade near the terminator, and a sparse one on the sc.ground.
    band = (1.0 - lit) * smoothstep(0.08, 0.42, sc.sun_n)
    dots = halftone(np.clip(band * 0.42, 0, 1), 8 * sc.S, 25)
    out = mix(out, out * 0.78, dots * (1.0 - lit) * sc.geo)
    ground_dots = halftone(np.full((sc.H, sc.W), 0.12, np.float32), 9 * sc.S, 40) * sc.ground * (1.0 - smoothstep(0.1, 0.6, sc.mist))
    out = mix(out, out * 0.86, ground_dots)
    out = haze(out, sc.mist, hexc('#f1d9bb'), 0.8, start=0.03, end=0.8)
    sky = sky_gradient(sc.H, sc.W, sc.horizon, [(0.0, '#f6dcb8'), (0.3, '#e4c9d9'), (0.65, '#bcd4ee'), (1.0, '#9ec1ea')])
    clouds = cloud_mask(sc.H, sc.W, sc.horizon, 11, 150 * sc.S, 0.30, 0.03, aspect=3.2, band=(0.15, 0.85))
    cloud_edge = np.clip(dilate_mask(clouds, max(1, int(2 * sc.S))) - clouds, 0, 1)
    sky = mix(sky, hexc('#fbf6ef'), clouds)
    sky = mix(sky, hexc('#2a1e2c'), cloud_edge * 0.9)
    out = comp(out, sky, sc.alpha)
    out = ink(out, sc.lines_clean, hexc('#1d1419'), 0.98)
    return grain(out, 0.012)


# ---------------------------------------------------------------- 2. painted stylized (Jumanji / Fortnite)
@style(2, 'Painted stylized', 'Jumanji / Fortnite',
       'Hand-painted texture detail, saturated, warm key + cool fill, sc.rim light, soft bloom. No lines.')
def painted(sc):
    img8 = to8(tonemap(sc.beauty_lin, 1.3))
    smooth = cv2.bilateralFilter(img8, 7, 30, 5 * sc.S)
    smooth = cv2.detailEnhance(smooth, sigma_s=5 * sc.S, sigma_r=0.22)
    out = f32(smooth)
    out = mix(out, brush_strokes(out, 7 * sc.S, 8), 0.4)
    out = hsv_adjust(out, sat=1.45, val=1.02)
    out = contrast(out, 1.1, 0.45)
    out = np.clip(out * 0.93 + 0.05, 0, 1)                           # no pure black
    key = smoothstep(0.1, 0.5, sc.sun_n)
    out = mix(out * np.array([0.88, 0.94, 1.12], np.float32), out * np.array([1.06, 1.0, 0.93], np.float32), key)
    rim_col = hexc('#d8efff')
    out = np.clip(out + rim_col * (sc.rim * sc.house * 0.7 * (0.3 + 0.7 * sc.in_shadow))[..., None], 0, 1)
    bright = np.clip(out - 0.72, 0, 1)
    out = np.clip(out + blur(bright, 18 * sc.S) * 0.5, 0, 1)
    out = haze(out, sc.mist, hexc('#bcd9f3'), 0.7, start=0.03, end=0.9)
    sky = sky_gradient(sc.H, sc.W, sc.horizon, [(0.0, '#bfe3f7'), (0.4, '#5aaae8'), (1.0, '#2b7ad0')])
    clouds, lit_c = cumulus(sc.H, sc.W, sc.horizon, 21, 210 * sc.S, 0.46, 0.09, aspect=1.9, band=(0.08, 0.9))
    cloud_col = mix(hexc('#8fa8c4'), hexc('#ffffff'), lit_c)
    sky = mix(sky, cloud_col, smoothstep(0.08, 0.75, clouds))
    sky = f32(cv2.bilateralFilter(to8(sky), 9, 40, 6 * sc.S))
    out = comp(out, sky, sc.alpha)
    return vignette(out, 0.18)


# ---------------------------------------------------------------- 3. cel-shaded toon with colored lines
@style(3, 'Cel toon, colored lines', 'Wind Waker / Hi-Fi Rush',
       'Three-step shade ramp, hard specular shapes, outlines in a darker fill color, flat sky.')
def cel(sc):
    flat8 = meanshift(to8(sc.albedo), 10 * sc.S, 24)
    fill = f32(flat8)
    fill = flatten_region(fill, sc.ground, 14 * sc.S, 0.95)
    fill = chroma(fill, 2.1, 0.94)                                   # toon colors: everything gets a hue
    fill = fill * np.array([1.03, 0.99, 0.92], np.float32)         # warm cream plaster, not grey
    fill = contrast(fill, 1.12, 0.5)
    ramp = np.where(sc.sun_n > 0.55, 1.0, np.where(sc.sun_n > 0.2, 0.72, 0.5)).astype(np.float32)
    ramp = blur(ramp, 0.6 * sc.S)
    tint = mix(np.array([0.72, 0.74, 1.0], np.float32), np.array([1.0, 1.0, 1.0], np.float32), ramp)
    out = fill * ramp[..., None] * tint
    out *= mix(0.78, 1.0, smoothstep(0.15, 0.8, sc.ao))[..., None]
    spec = smoothstep(0.55, 0.62, sc.gloss_n) * (sc.sun_n > 0.55)
    out = mix(out, hexc('#fff4dc'), spec * 0.85)
    out = haze(out, sc.mist, hexc('#b8dcf0'), 0.6, start=0.04, end=0.9)
    sky = sky_gradient(sc.H, sc.W, sc.horizon, [(0.0, '#c9ecf8'), (0.2, '#7cc6ee'), (1.0, '#3a95dd')])
    clouds = cloud_mask(sc.H, sc.W, sc.horizon, 31, 170 * sc.S, 0.33, 0.015, aspect=2.6, band=(0.15, 0.85))
    under = np.clip(clouds - shifted(clouds, 0, -7 * sc.S), 0, 1)
    sky = mix(sky, hexc('#ffffff'), clouds)
    sky = mix(sky, hexc('#9cc9ea'), under * 0.9)
    out = comp(out, sky, sc.alpha)
    thin = erode_mask(sc.lines_clean, max(0, int(round(0.6 * sc.S))))
    line_col = np.clip(blur(out, 2.0) * 0.38, 0, 1)
    out = mix(out, line_col, np.clip(thin * 1.1, 0, 1))
    return out


# ---------------------------------------------------------------- 4. gouache storybook
@style(4, 'Gouache storybook', 'watercolor wash + pencil',
       'Soft washes, wet edges, paper grain, pencil underdrawing, washed shadows. Everything fades to paper.')
def gouache(sc):
    flat8 = meanshift(to8(sc.albedo), 8 * sc.S, 18)
    fill = f32(flat8)
    fill = flatten_region(fill, sc.ground, 10 * sc.S, 0.7)
    fill = hsv_adjust(fill, sat=1.4, val=1.0)
    shade = 0.6 + 0.4 * smoothstep(0.05, 0.6, blur(sc.sun_n, 1.5 * sc.S))
    shade *= 0.85 + 0.15 * smoothstep(0.1, 0.9, sc.ao)
    out = fill * shade[..., None]
    out = mix(out, out * np.array([0.86, 0.9, 1.05], np.float32), (1.0 - smoothstep(0.1, 0.5, sc.sun_n)) * 0.8)
    out = np.clip(out * 0.86 + 0.12, 0, 1)                             # no true black in watercolor
    out = mix(out, blur(out, 4 * sc.S), 0.35)                              # pigment bleeding
    edges = np.clip(edge_mag(fill, 1.0) * 2.4, 0, 1)
    pooled = blur(edges, 1.2 * sc.S)
    out *= (1.0 - 0.38 * pooled)[..., None]                             # pigment pools at the edges of each wash
    gran = value_noise(sc.H, sc.W, 6 * sc.S, 41, octaves=2) - 0.5
    out *= (1.0 + gran * 0.14 * (1.0 - lum(out)))[..., None]
    pap = hexc('#f7f1e4')
    out = haze(out, sc.mist, pap, 0.9, start=0.02, end=0.7)
    sky = sky_gradient(sc.H, sc.W, sc.horizon, [(0.0, '#f6efe0'), (0.25, '#d6e4ea'), (1.0, '#9dbfd6')])
    bloom = value_noise(sc.H, sc.W, 220 * sc.S, 51, octaves=3, aspect=2.5)
    sky = mix(sky, pap, smoothstep(0.55, 0.8, bloom) * 0.8)
    out = comp(out, sky, sc.alpha)
    out *= paper(sc.H, sc.W, 61, fiber=0.045, mottle=0.06)[..., None]
    pencil = shifted(sc.lines_sketch, 1.5 * sc.S, -1.0 * sc.S)
    out = ink(out, pencil, hexc('#4a3f44'), 0.7)
    return np.clip(out, 0, 1)


# ---------------------------------------------------------------- 5. comic halftone
@style(5, 'Comic halftone', 'Spider-Verse print',
       'Flat posterized color, Ben-Day dots in the shade, thick black inks, slight misregistration.')
def comic(sc):
    flat8 = meanshift(to8(sc.albedo), 14 * sc.S, 28)
    fill, _ = quantize(flat8, 9, mask=sc.geo, seed=7)
    fill = hsv_adjust(fill, sat=1.45, val=1.08)
    fill = contrast(fill, 1.15, 0.5)
    lit = smoothstep(0.42, 0.5, sc.sun_n)
    out = mix(fill * np.array([0.82, 0.76, 0.98], np.float32), fill, lit)
    dot_dark = np.clip((1.0 - smoothstep(0.0, 0.5, sc.sun_n)) * 0.45, 0, 1) * sc.geo
    dots = halftone(dot_dark, 8 * sc.S, 15)
    out = mix(out, out * np.array([0.6, 0.55, 0.8], np.float32), dots * (1.0 - lit) * 0.85)
    tone_dots = halftone(np.clip(sc.mist * 0.45, 0, 1) * sc.ground, 8 * sc.S, 45)
    out = mix(out, out * 0.75, tone_dots)
    out = np.stack([shifted(out[..., 0], 2.2 * sc.S, 0.6 * sc.S), out[..., 1], shifted(out[..., 2], -1.6 * sc.S, -0.5 * sc.S)], -1)
    sky = sky_gradient(sc.H, sc.W, sc.horizon, [(0.0, '#ffd27a'), (0.35, '#ff8f5a'), (0.7, '#c45aa0'), (1.0, '#5a3a8a')])
    sky_dots = halftone(np.clip(sky_gradient(sc.H, sc.W, sc.horizon, [(0.0, 0.0), (0.5, 0.75), (1.0, 0.0)])[..., 0], 0, 1), 9 * sc.S, 35)
    sky = mix(sky, sky * np.array([0.55, 0.45, 0.8], np.float32), sky_dots * 0.8)
    out = comp(out, sky, sc.alpha)
    thick = dilate_mask(sc.lines_clean, max(1, int(round(0.8 * sc.S))))
    out = ink(out, np.clip(thick * 1.2, 0, 1), hexc('#0b0a10'), 1.0)
    out *= paper(sc.H, sc.W, 71, fiber=0.03, mottle=0.03)[..., None]
    return np.clip(out, 0, 1)


# ---------------------------------------------------------------- 6. flat-shaded graphic
@style(6, 'Flat-shaded graphic', 'Firewatch / Grow Home',
       'No textures: one color per surface, faceted lighting, long cool shadows, big gradient sky with a sun.')
def flat_graphic(sc):
    base8 = to8(flatten_region(sc.albedo, sc.ground, 12 * sc.S, 0.95))
    flat8 = meanshift(meanshift(base8, 24 * sc.S, 52), 16 * sc.S, 40)
    fill, _ = quantize(flat8, 6, mask=sc.geo, seed=9)
    fill = hsv_adjust(fill, sat=1.8, val=1.05)
    fill = contrast(fill, 1.3, 0.45)
    sky_col = hexc('#86aed1')
    sun_col = hexc('#ffe2b0')
    n_smooth = blur(sc.normal, 1.2 * sc.S)
    n_smooth /= np.maximum(np.linalg.norm(n_smooth, axis=-1, keepdims=True), 1e-6)
    ndl = np.clip(n_smooth @ sc.L, 0, 1)
    up = np.clip(n_smooth[..., 2] * 0.5 + 0.5, 0, 1)
    shadow = blur(sc.in_shadow, 0.8 * sc.S)
    light = sky_col * (0.32 + 0.28 * up)[..., None] + sun_col * (ndl * (1.0 - shadow) * 1.25)[..., None]
    out = np.clip(fill * light * 1.08, 0, 1)
    out = mix(out, out * np.array([0.7, 0.78, 1.08], np.float32), shadow * 0.7)
    out = haze(out, sc.mist, hexc('#f2c48f'), 0.6, start=0.08, end=0.95)
    sky = sky_gradient(sc.H, sc.W, sc.horizon, [(0.0, '#f6b26b'), (0.18, '#f2cf96'), (0.45, '#d6b7c2'), (1.0, '#5e7fb8')])
    yy, xx = np.mgrid[0:sc.H, 0:sc.W].astype(np.float32)
    sun_r = 0.05 * sc.W
    sun_x, sun_y = 0.2 * sc.W, sc.horizon - 0.36 * sc.horizon
    disc = smoothstep(sun_r + 1.5, sun_r - 1.5, np.sqrt((xx - sun_x) ** 2 + (yy - sun_y) ** 2))
    sky = mix(sky, hexc('#fff3d6'), disc * 0.92)
    bands = cloud_mask(sc.H, sc.W, sc.horizon, 81, 260 * sc.S, 0.3, 0.01, aspect=9.0, band=(0.3, 0.9))
    sky = mix(sky, hexc('#f9d7b3'), bands * 0.85)
    out = comp(out, sky, sc.alpha)
    return grain(vignette(out, 0.12), 0.008)


# ---------------------------------------------------------------- 7. oil-painted realism
@style(7, 'Oil-painted realism', 'Arcane / Dishonored',
       'Brush strokes that follow the forms over the real textures, teal/orange grade, painterly sky.')
def oil(sc):
    img = tonemap(sc.beauty_lin, 1.3)
    strokes = brush_strokes(img, 9 * sc.S, 12, sigma=4.0 * sc.S)
    dabs = f32(cv2.xphoto.oilPainting(to8(strokes), 3, 1, cv2.COLOR_BGR2Lab))
    out = mix(strokes, dabs, 0.3)
    out = f32(cv2.detailEnhance(to8(out), sigma_s=8 * sc.S, sigma_r=0.15))
    out = contrast(out, 1.18, 0.45)
    out = hsv_adjust(out, sat=1.35, val=1.04)
    out = np.clip(out * 0.95 + 0.04, 0, 1)
    out = mix(out * np.array([0.8, 0.9, 1.1], np.float32), out * np.array([1.1, 1.0, 0.86], np.float32),
              smoothstep(0.3, 0.75, lum(out)))                         # teal shadows, orange lights
    out = np.clip(out + (out - blur(out, 12 * sc.S)) * 0.3, 0, 1)         # clarity
    out = haze(out, sc.mist, hexc('#9fb3c4'), 0.7, start=0.04, end=0.95)
    sky = sky_gradient(sc.H, sc.W, sc.horizon, [(0.0, '#f0cfa4'), (0.25, '#c9b4a4'), (0.6, '#6e8fb0'), (1.0, '#2f5587')])
    big, lit_c = cumulus(sc.H, sc.W, sc.horizon, 91, 300 * sc.S, 0.44, 0.12, aspect=1.8, band=(0.03, 0.9), light=(-1.0, -0.7))
    cloud_col = mix(hexc('#5f7390'), hexc('#fff1dc'), smoothstep(0.2, 0.9, lit_c))
    sky = mix(sky, cloud_col, smoothstep(0.05, 0.6, big))
    sky = mix(sky, brush_strokes(sky, 13 * sc.S, 12, sigma=6.0 * sc.S), 0.7)
    out = comp(out, sky, sc.alpha)
    out *= (1.0 + 0.035 * canvas(sc.H, sc.W, 93, 3.0 * sc.S))[..., None]
    return grain(vignette(out, 0.35, 1.6), 0.008)


# ---------------------------------------------------------------- 8. pen and ink crosshatch
@style(8, 'Pen & ink crosshatch', 'ink drawing with a tinted wash',
       'Sketchy outlines, three layers of hatching in the shade, cream paper, a light color wash.')
def crosshatch(sc):
    pap = hexc('#f4ecd9')
    wash_c = hsv_adjust(blur(sc.albedo, 3 * sc.S), sat=1.15, val=1.0)
    wash_c = flatten_region(wash_c, sc.ground, 10 * sc.S, 0.8)
    wash = mix(pap, wash_c * 0.85 + 0.15 * pap, 0.9)
    shade_t = 0.75 + 0.25 * smoothstep(0.1, 0.7, sc.sun_n)
    wash = mix(wash, wash * np.array([0.8, 0.85, 0.97], np.float32), (1.0 - shade_t) * 2.5)
    out = wash.copy()
    s1 = (1.0 - smoothstep(0.25, 0.6, sc.sun_n)) * sc.geo
    s2 = (1.0 - smoothstep(0.05, 0.25, sc.sun_n)) * sc.geo
    s3 = (1.0 - smoothstep(0.0, 0.1, sc.sun_n)) * sc.geo * (1.0 - smoothstep(0.1, 0.5, sc.ao))
    far = 1.0 - smoothstep(0.15, 0.7, sc.mist)
    inkc = hexc('#2a2320')
    h1 = hatch(sc.H, sc.W, 42, 6.5 * sc.S, 1.1 * sc.S, 101)
    h2 = hatch(sc.H, sc.W, -38, 6.5 * sc.S, 1.1 * sc.S, 102)
    h3 = hatch(sc.H, sc.W, 5, 7.0 * sc.S, 1.0 * sc.S, 103)
    out = ink(out, h1 * s1 * far, inkc, 0.85)
    out = ink(out, h2 * s2 * far, inkc, 0.85)
    out = ink(out, h3 * s3 * far, inkc, 0.8)
    sky = sky_gradient(sc.H, sc.W, sc.horizon, [(0.0, pap), (1.0, pap * np.array([0.97, 0.97, 1.0], np.float32))])
    clouds = cloud_mask(sc.H, sc.W, sc.horizon, 111, 200 * sc.S, 0.28, 0.02, aspect=3.0, band=(0.2, 0.8))
    cloud_edge = np.clip(dilate_mask(clouds, max(1, int(round(1.4 * sc.S)))) - clouds, 0, 1)
    sky = ink(sky, cloud_edge, inkc, 0.7)
    sky = ink(sky, hatch(sc.H, sc.W, 42, 7 * sc.S, 0.9 * sc.S, 104) * clouds * 0.25, inkc, 0.5)
    out = comp(out, sky, sc.alpha)
    out *= paper(sc.H, sc.W, 121, fiber=0.07, mottle=0.05)[..., None]
    out = ink(out, np.clip(blur(sc.lines_sketch, 0.5) * 1.3, 0, 1), inkc, 0.95)
    return np.clip(out, 0, 1)


# ---------------------------------------------------------------- run
FONT = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
FONT_BOLD = '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
if not os.path.exists(FONT_BOLD):
    FONT_BOLD = FONT


def label_tile(img8, number, name, reference, tile_w):
    tile_h = int(round(tile_w * img8.shape[0] / img8.shape[1]))
    tile = Image.fromarray(img8).resize((tile_w, tile_h), Image.LANCZOS)
    bar = int(0.11 * tile_w)
    out = Image.new('RGB', (tile_w, tile_h + bar), (18, 18, 22))
    out.paste(tile, (0, 0))
    draw = ImageDraw.Draw(out)
    try:
        big = ImageFont.truetype(FONT_BOLD, int(bar * 0.36))
        small = ImageFont.truetype(FONT, int(bar * 0.27))
    except OSError:
        big = small = ImageFont.load_default()
    title = f'{number}  {name}' if number else name
    draw.text((int(0.02 * tile_w), tile_h + int(bar * 0.14)), title, font=big, fill=(255, 150, 60))
    draw.text((int(0.02 * tile_w), tile_h + int(bar * 0.58)), reference, font=small, fill=(200, 205, 215))
    return out


def contact_sheet(results, path, tile_w=800, cols=3, gap=12):
    tiles = [label_tile(r['_img'], r['number'], r['name'], r['reference'], tile_w) for r in results]
    rows = (len(tiles) + cols - 1) // cols
    sheet = Image.new('RGB', (cols * tile_w + (cols + 1) * gap, rows * tiles[0].height + (rows + 1) * gap), (10, 10, 12))
    for i, t in enumerate(tiles):
        r, c = divmod(i, cols)
        sheet.paste(t, (gap + c * (tile_w + gap), gap + r * (t.height + gap)))
    sheet.save(path, optimize=True)
    return sheet.size


def load_plugin(path, number):
    """A style from a plugin file: STYLE = dict(name, reference, note) and compose(sc, lib)."""
    import importlib.util
    spec = importlib.util.spec_from_file_location(f'style_plugin_{number}', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    info = getattr(module, 'STYLE', {})
    lib = sys.modules[__name__]
    name = info.get('name', os.path.splitext(os.path.basename(path))[0])
    return (info.get('number', number), name, info.get('reference', ''), info.get('note', ''),
            lambda sc: module.compose(sc, lib))


def slugify(name):
    slug = name.lower().replace('&', 'and').replace(',', '').replace('/', ' ').replace('-', ' ')
    return '_'.join(slug.split())


def main(argv):
    passdir, outdir = argv[0], argv[1]
    only = {int(v) for v in argv[argv.index('--only') + 1].split(',')} if '--only' in argv else None
    plugins = argv[argv.index('--plugins') + 1].split(',') if '--plugins' in argv else []
    os.makedirs(outdir, exist_ok=True)
    styles = [] if '--no-builtin' in argv else list(STYLES)
    for i, path in enumerate(plugins):
        styles.append(load_plugin(path, 9 + i))
    sc = load_scene(passdir)
    results = []
    for number, name, reference, note, fn in styles:
        if only is not None and number not in only:
            continue
        print(f'STYLE: {number} {name}', flush=True)
        img8 = to8_dither(fn(sc))
        path = os.path.join(outdir, f'{number:02d}_{slugify(name)}.png')
        Image.fromarray(img8).save(path, optimize=True)
        results.append(dict(number=number, name=name, reference=reference, note=note, file=os.path.basename(path),
                            _img=img8))
    with open(os.path.join(outdir, 'styles.json'), 'w') as f:
        json.dump([{k: v for k, v in r.items() if k != '_img'} for r in results], f, indent=1)
    if len(results) > 1:
        size = contact_sheet(results, os.path.join(outdir, 'contact_sheet.png'))
        print(f'STYLE: sheet {size}', flush=True)
    print('STYLE: done', flush=True)


if __name__ == '__main__':
    main(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:])
