"""The water's textures, script-made (the user's rule: no texture libraries; numpy, fixed seeds, every one tiles), for
M_Water and M_Waterfall (Tools/Unreal/build_world_materials.py imports them to /Game/Art/Textures/Water):

    T_WaterNormal.png   512 x 512, DirectX normal map (green down, as Unreal expects) of small ripples on a 3 m tile
                        (M_Water's RippleScale 0.33 lays one tile on 3 m): wavelengths of about 6 to 50 cm, with a
                        band of longer wind ripples leaning one way, crests a little sharper than troughs.
    T_WaterFoam.png     512 x 512, linear greyscale foam: soft drifts, a lacy net of bubble walls and fine bubbles.
                        Flattened (every value as common as any other), so a threshold t keeps a share 1 - t of the
                        texels: M_Water's shore foam reaches in by its threshold.
    T_WaterStreaks.png  512 across x 1024 along, linear greyscale falling water: thin threads grouped into ropes and
                        broken by blotches; flattened too. M_Waterfall scrolls three copies at different scales and
                        speeds: its density, its see-through gaps and its ragged edges (a threshold) and where it whitens.

Run it with Blender's Python (numpy; Tools/Blender/looter_textures.py's helpers), either way:
    Tools\\artrun.ps1 -Script Art\\Textures\\Water\\make_water_textures.py
    "C:/Program Files/Blender Foundation/Blender 4.4/4.4/python/bin/python.exe" Art/Textures/Water/make_water_textures.py
It writes the three maps here (a few seconds) and previews to Saved/ArtPreviews/Water/: Water_tiles.png (each map
tiled 2 x 2, the normal map lit), Water_creek.png (M_Water's look worked out as the material does: a creek from bank to
bank, shallows, depth, the broken shore foam, ripples, sky and sun) and Water_fall.png (M_Waterfall's sheet over dark
rock: gaps, ragged edges, white where it breaks). Then, in the editor:
    Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_world_materials.py M_Water M_Waterfall M_WaterMist"
"""
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Tools', 'Blender'))
import looter_textures as lt  # noqa: E402 - needs the path above

SIZE = 512
NORMAL_TILE_M = 3.0             # meters one T_WaterNormal tile covers at M_Water's default RippleScale
OUT_DIR = HERE
PREVIEW_DIR = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Water')

# The shared water contract's palette (sRGB): shallow teal, deep blue-green, foam, the falling water's body.
SHALLOW = 0x4FA39A
DEEP = 0x1D5560
FOAM = 0xEEF5F2
FALL_BODY = 0x6F9FA3


# --- Fields ---

def band_noise(shape, seed, k_low, k_high, slope, lean=None):
    """Tileable noise from a band of wavelengths, mean 0 and deviation 1: k cycles per tile from about k_low to k_high,
    amplitude falling as k^-slope. lean = (angle, sharpness) favours waves travelling along one direction (wind
    ripples), cos^sharpness of the angle off it."""
    h, w = shape
    fy = (np.fft.fftfreq(h) * h).astype(np.float32)[:, None]
    fx = (np.fft.rfftfreq(w) * w).astype(np.float32)[None, :]
    k = np.hypot(fx, fy)
    safe = np.where(k > 0.0, k, 1.0)
    amp = safe ** -slope * (1.0 - np.exp(-(k / k_low) ** 4)) * np.exp(-(k / k_high) ** 2)
    if lean is not None:
        angle, sharp = lean
        along = np.abs(fx * math.cos(angle) + fy * math.sin(angle)) / safe
        amp = amp * along ** sharp
    rng = np.random.default_rng(seed)
    spectrum = amp * (rng.standard_normal(k.shape) + 1j * rng.standard_normal(k.shape))
    out = np.fft.irfft2(spectrum, s=shape).astype(np.float32)
    out -= out.mean()
    return out / (out.std() + 1e-8)


def flatten(a):
    """Every value as common as any other (0..1, the order of the texels kept), so a material's threshold maps to the
    share of the texture it keeps."""
    order = np.argsort(a.ravel(), kind='stable')
    ranks = np.empty(a.size, np.float32)
    ranks[order] = np.arange(a.size, dtype=np.float32) / (a.size - 1)
    return ranks.reshape(a.shape)


# --- The maps ---

def ripple_height():
    """Small ripples (height, deviation 1): a broad band of short waves every way, a band of longer wind ripples leaning
    one way, crests sharpened (h + a h^2 lifts the peaks and flattens the troughs)."""
    shape = (SIZE, SIZE)
    # Per 3 m tile: 4 cycles = 75 cm waves, 24 cycles = 12 cm; the fine band 6 cm and under, faint. The height falls
    # steeply with frequency (k^-2.2), so the slopes, which the normal map shows, aren't all fine grain.
    chop = band_noise(shape, 11, 4.0, 24.0, 2.2)
    wind = band_noise(shape, 12, 5.0, 14.0, 1.6, lean=(0.45, 8.0))
    fine = band_noise(shape, 13, 24.0, 60.0, 2.0)
    h = 0.8 * chop + 0.6 * wind + 0.06 * fine
    h /= h.std()
    h = h + 0.22 * (h * h - 1.0)
    return h - h.mean()


def make_normal():
    h = ripple_height()
    px_m = NORMAL_TILE_M / SIZE
    # Scale the height so the steepest ripples (99th percentile) tilt about 40 degrees: the map uses most of its 8 bits,
    # and M_Water's RippleStrength scales it down.
    gx = (np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)) / (2.0 * px_m)
    gy = (np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)) / (2.0 * px_m)
    slope = np.hypot(gx, gy)
    scale = math.tan(math.radians(40.0)) / float(np.percentile(slope, 99.0))
    return lt.normal_map(h * scale, px_m), h


def make_foam():
    """Shore foam: soft drifts, a lacy net (the soft borders of warped cells), and round holes of every size punched
    through it, so a high cover reads as foam full of bubble holes and a low one as strands and flecks."""
    shape = (SIZE, SIZE)
    x, y = lt.coords(shape)
    drifts = lt.noise(shape, 21, 30.0, octaves=4, gain=0.55)
    wx = lt.noise(shape, 22, 26.0, octaves=2) * 14.0
    wy = lt.noise(shape, 23, 26.0, octaves=2) * 14.0
    net = lt.worley(shape, (8, 8), 24, x=x + wx, y=y + wy)
    lace = np.exp(-net['edge'] / 6.0) + 0.3 * lt.per_cell(net['id'], net['count'], 25, -1.0, 1.0)
    cell = SIZE / 22.0
    pits = lt.worley(shape, (22, 22), 26, x=x + wx * 0.4, y=y + wy * 0.4)
    radius = lt.per_cell(pits['id'], pits['count'], 27, 0.12, 0.5) * cell
    holes = 1.0 - smooth_np(radius * 0.55, radius, pits['f1'])
    grain = lt.noise(shape, 28, 1.3)

    def norm(a):
        return (a - a.mean()) / (a.std() + 1e-8)
    field = 1.0 * drifts + 0.8 * norm(lace) - 0.75 * norm(holes) + 0.12 * grain
    return flatten(field)


def smooth_np(e0, e1, x):
    t = np.clip((x - e0) / np.maximum(e1 - e0, 1e-6), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def make_streaks():
    """Falling water, 512 across x 1024 along (rows run with the flow): thin threads, grouped into ropes, broken into
    blotches and gaps."""
    shape = (2 * SIZE, SIZE)
    threads = lt.noise(shape, 61, 2.4, 90.0, octaves=2, gain=0.6)
    ropes = lt.noise(shape, 62, 18.0, 200.0, octaves=2)
    blotches = lt.noise(shape, 63, 9.0, 30.0, octaves=3, gain=0.6)
    # The threads waver a little across as they fall.
    x, y = lt.coords(shape)
    sway = lt.noise(shape, 64, 30.0, 120.0) * 6.0
    threads = lt.sample(threads, x + sway, y)
    field = 0.7 * threads + 0.45 * ropes + 0.3 * blotches
    return flatten(field)


# --- Previews: the materials worked out in numpy, as build_world_materials.py's HLSL does ---

def srgb_to_linear(c):
    c = np.asarray(c, np.float32)
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def linear_to_srgb(c):
    c = np.clip(c, 0.0, None)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(c, 1.0 / 2.4) - 0.055)


def lin(hex_value):
    return srgb_to_linear(lt.rgb(hex_value))


def tex_sample(tex, u, v):
    """A texture at UV (wrapping, bilinear), V down the image as in Unreal."""
    h, w = tex.shape[:2]
    return lt.sample(tex, u * w - 0.5, v * h - 0.5)


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def creek_preview(normal8, foam, t=7.3):
    """A creek 8 m wide from bank to bank, 16 m of it, seen from above at 50 degrees: M_Water's look at time t."""
    W, H, ppm = 1024, 640, 64.0
    x, y = lt.coords((H, W))
    um, vm = x / ppm, y / ppm                                   # UV 0 in meters
    width = 8.0
    bank = np.minimum(vm - 1.0, width + 1.0 - vm)               # meters from the nearer bank (1 m of bank each side)
    shore = np.clip(1.0 - bank / 1.5, 0.0, 1.0)                 # R
    depth = np.clip(bank / 2.6, 0.0, 1.0) ** 0.8                # G
    flow = np.stack([np.ones_like(um), np.zeros_like(um)], axis=-1) * np.clip(bank / 1.0, 0.2, 1.0)[..., None]
    n = normal8.astype(np.float32) / 255.0 * 2.0 - 1.0
    # The defaults the material builds with.
    p = dict(RippleScale=0.33, RippleStrength=0.45, FlowSpeed=0.8, FlowCycle=0.5, IdleSpeed=0.05, FoamScale=0.4,
             FoamReach=0.35, FoamCover=0.7, FoamSoft=0.25, FoamOpacity=0.9, LapSpeed=0.12, FoamDrift=0.03, DepthPower=0.8,
             SkyTint=0.5, Roughness=0.07)
    jitter = np.mod(0.5 + 0.25 * np.sin(um * 0.71 + 1.7 * np.sin(vm * 0.53)) + 0.25 * np.sin(vm * 0.83 + 1.9 * np.sin(um * 0.47)), 1.0)
    # Foam
    lap = np.sin((t * p['LapSpeed'] + jitter) * 6.28318)
    reach = p['FoamReach'] * (1.0 + 0.2 * lap)
    tt = np.clip((shore - 1.0 + reach) / reach, 0.0, 1.0)
    fu = um * p['FoamScale'] + np.mod(0.6 * t * p['FoamDrift'] * p['FoamScale'], 1.0) + 0.05 * np.sin(t * 0.7 + vm * 1.3)
    fv = vm * p['FoamScale'] + np.mod(-0.8 * t * p['FoamDrift'] * p['FoamScale'], 1.0) + 0.05 * np.sin(t * 0.53 + um * 1.1)
    nfoam = tex_sample(foam, fu, fv)
    cut = 1.0 - tt * p['FoamCover']
    foam_mask = smoothstep(cut, cut + p['FoamSoft'], nfoam) * (0.4 + 0.6 * tt) * p['FoamOpacity'] * (tt > 0.0)
    # Ripples: two taps, phase-blended along the flow
    moving = np.clip(np.linalg.norm(flow, axis=-1) * 3.0, 0.0, 1.0)
    pa = np.mod(t * p['FlowCycle'] + jitter, 1.0)
    pb = np.mod(pa + 0.5, 1.0)
    wgt = 0.5 + (1.0 - np.abs(1.0 - 2.0 * pa) - 0.5) * moving
    slide_a = flow * (p['FlowSpeed'] * (pa - 0.5) / p['FlowCycle'])[..., None]
    slide_b = flow * (p['FlowSpeed'] * (pb - 0.5) / p['FlowCycle'])[..., None]
    rs = p['RippleScale']
    ia = np.mod(np.array([0.8, 0.6]) * t * p['IdleSpeed'] * rs, 1.0)
    ib = np.mod(np.array([-0.6, -0.8]) * t * p['IdleSpeed'] * rs * 1.37, 1.0)
    qa = np.stack([um, vm], -1) - slide_a
    qb = np.stack([um, vm], -1) - slide_b
    c, s = 0.8, 0.6
    rb = np.stack([c * qb[..., 0] - s * qb[..., 1], s * qb[..., 0] + c * qb[..., 1]], -1)
    na = tex_sample(n, qa[..., 0] * rs + ia[0], qa[..., 1] * rs + ia[1])[..., :2]
    nbr = tex_sample(n, rb[..., 0] * rs * 1.37 + 0.37 + ib[0], rb[..., 1] * rs * 1.37 + 0.61 + ib[1])[..., :2]
    nb = np.stack([c * nbr[..., 0] + s * nbr[..., 1], -s * nbr[..., 0] + c * nbr[..., 1]], -1)
    wv = wgt[..., None]
    xy = (na * wv + nb * (1.0 - wv)) / np.sqrt(wgt * wgt + (1.0 - wgt) ** 2)[..., None]
    xy = xy * (p['RippleStrength'] * (1.0 + 0.5 * moving) * (1.0 - 0.7 * foam_mask))[..., None]
    # Tangent space to world: U along +x, V down the image (DirectX green down: +y toward +V).
    nw = np.stack([xy[..., 0], -xy[..., 1], np.ones_like(um)], -1)
    nw /= np.linalg.norm(nw, axis=-1, keepdims=True)
    # Colour
    view = np.array([0.0, -0.64, 0.77])                          # toward the camera (the far bank at the top)
    sun = np.array([0.35, 0.55, 0.76])
    sun /= np.linalg.norm(sun)
    base = lin(SHALLOW) + (lin(DEEP) - lin(SHALLOW)) * (depth ** p['DepthPower'])[..., None]
    ndv = np.clip(np.sum(nw * view, -1), 0.0, 1.0)
    fres = 0.02 + 0.98 * (1.0 - ndv) ** 5
    sky = srgb_to_linear(lt.rgb(0xA8CCD8))
    base = base + (sky - base) * np.clip(fres * p['SkyTint'], 0.0, 1.0)[..., None]
    base = base + (lin(FOAM) - base) * foam_mask[..., None]
    rough = p['Roughness'] + (0.55 - p['Roughness']) * foam_mask
    ndl = np.clip(np.sum(nw * sun, -1), 0.0, 1.0)
    half = (sun + view) / np.linalg.norm(sun + view)
    ndh = np.clip(np.sum(nw * half, -1), 0.0, 1.0)
    a2 = (rough ** 2) ** 2
    d = a2 / (np.pi * (ndh * ndh * (a2 - 1.0) + 1.0) ** 2)
    spec = np.clip(d * 0.04 * 0.25, 0.0, 30.0) * ndl
    sky_reflect = fres * 0.6
    lit = base * (ndl * 2.6 + 0.55)[..., None] + (spec * 3.0)[..., None] + (sky_reflect[..., None] * sky * 1.2)
    exposed = lit / (1.0 + lit)                                  # a soft tone curve for the preview
    img = linear_to_srgb(exposed * 1.6)
    # The banks: wet earth beyond the water
    earth = lt.rgb(0x6B5B45)
    img[bank < 0.0] = earth
    return np.clip(img, 0.0, 1.0)


def fall_preview(streaks, t=4.2):
    """M_Waterfall's sheet over dark rock: 3 m across (U 0..1), 20 m of fall, the mesh's alpha fading at the edges and
    the foot, its foam (R) rising from the lip down; at time t."""
    W, H = 384, 1280
    x, y = lt.coords((H, W))
    u = x / W
    along = y / H * 20.0                                         # meters along the flow (the lip at the top)
    edge = 1.0 - np.abs(u - 0.5) * 2.0
    alpha = np.clip(edge * 2.2, 0.0, 1.0) ** 0.8 * (1.0 - smoothstep(14.0, 20.0, along)) * smoothstep(0.0, 0.6, along)
    foam_r = 0.15 + 0.75 * smoothstep(0.5, 4.0, along)
    p = dict(Speed=6.0, Length=12.0, Across=1.0, GapCalm=0.12, GapBreak=0.3, CutMax=0.82, Soft=0.08, Body=0.55,
             FoamAmount=0.5, FoamSoft=0.15, Opacity=0.9)

    def layer(across, length, speed, du):
        return tex_sample(streaks, u * across + du, along / length - np.mod(t * speed / length, 1.0))
    s1 = layer(p['Across'], p['Length'], p['Speed'], 0.0)
    s2 = layer(p['Across'] * 1.73, p['Length'] * 0.61, p['Speed'] * 1.35, 0.37)
    s3 = layer(p['Across'] * 2.91, p['Length'] * 0.37, p['Speed'] * 1.8, 0.71)
    streak = s1 * 0.5 + s2 * 0.3 + s3 * 0.2
    brk = smoothstep(0.25, 0.75, foam_r)
    streak = 0.5 + (streak - 0.5) * (0.45 + 0.55 * brk)
    gap = p['GapCalm'] + (p['GapBreak'] - p['GapCalm']) * brk
    cut = gap + (p['CutMax'] - gap) * (1.0 - np.clip(alpha, 0.0, 1.0))
    keep = smoothstep(cut - p['Soft'], cut + p['Soft'], streak)
    dense = keep * (p['Body'] + (1.0 - p['Body']) * streak)
    opacity = np.clip(dense * p['Opacity'] * np.clip(alpha * 3.0, 0.0, 1.0), 0.0, 1.0)
    fine = s2 * 0.6 + s3 * 0.4
    white_cut = 1.0 - foam_r * p['FoamAmount']
    white = smoothstep(white_cut, white_cut + p['FoamSoft'], fine)
    body = lin(FALL_BODY) * (0.8 + 0.4 * s1)[..., None]
    col = body + (lin(FOAM) - body) * white[..., None]
    col = linear_to_srgb(np.clip(col, 0.0, 1.0))
    rock = lt.rgb(0x3B3833) * (0.85 + 0.15 * lt.noise((H, W), 5, 6.0))[..., None]
    img = rock + (col - rock) * opacity[..., None]
    return np.clip(img, 0.0, 1.0), opacity, keep


def tiles_preview(normal8, foam, streaks):
    """Each map tiled 2 x 2: the normal map lit from the top left over the shallow teal, the foam, the streaks."""
    n = normal8.astype(np.float32) / 255.0 * 2.0 - 1.0
    light = np.array([-0.45, -0.45, 0.77])
    light /= np.linalg.norm(light)
    # DirectX: +green toward the bottom of the image, so the image's up is -y.
    shade = np.clip(n[..., 0] * light[0] + n[..., 1] * light[1] + n[..., 2] * light[2], 0.0, 1.0)
    lit = linear_to_srgb(lin(SHALLOW) * (0.35 + 1.6 * shade)[..., None])
    tiled = [np.tile(lit, (2, 2, 1)), np.tile(np.repeat(foam[..., None], 3, -1), (2, 2, 1))]
    streak_tiles = np.tile(np.repeat(streaks[..., None], 3, -1), (1, 2, 1))
    gap = np.ones((1024, 16, 3), np.float32)
    return np.concatenate([tiled[0], gap, tiled[1], gap, streak_tiles], axis=1)


def main():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:]
    global OUT_DIR, PREVIEW_DIR
    # --out <folder>: a trial run, maps and previews there.
    if '--out' in argv and argv.index('--out') + 1 < len(argv):
        OUT_DIR = PREVIEW_DIR = argv[argv.index('--out') + 1]
    os.makedirs(OUT_DIR, exist_ok=True)
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    normal8, _ = make_normal()
    foam = make_foam()
    streaks = make_streaks()
    lt.write_png(os.path.join(OUT_DIR, 'T_WaterNormal.png'), normal8)
    lt.write_png(os.path.join(OUT_DIR, 'T_WaterFoam.png'), lt.to8(foam))
    lt.write_png(os.path.join(OUT_DIR, 'T_WaterStreaks.png'), lt.to8(streaks))
    lt.write_png(os.path.join(PREVIEW_DIR, 'Water_tiles.png'), lt.to8(tiles_preview(normal8, foam, streaks)))
    lt.write_png(os.path.join(PREVIEW_DIR, 'Water_creek.png'), lt.to8(creek_preview(normal8, foam)))
    fall, opacity, keep = fall_preview(streaks)
    lt.write_png(os.path.join(PREVIEW_DIR, 'Water_fall.png'), lt.to8(fall))
    print(f'WATER fall preview: sheet covers {float((opacity > 0.05).mean()) * 100.0:.0f}% of its frame, '
          f'solid {float((opacity > 0.6).mean()) * 100.0:.0f}%')
    print(f'WATER done: {OUT_DIR} and {PREVIEW_DIR}')


main()
