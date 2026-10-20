"""The Gravemother's Barrow fight, its effect and decal textures, script-made (the user's rule: no texture libraries;
Saved/Gravemother/CONTRACT.md, BARROW-ART; the pieces of Saved/Concepts/Gravemother/Barrow_FX.png). Run with Blender's
Python (numpy and Tools/Blender/looter_textures.py's helpers):

    Tools\\artrun.ps1 -Script Art\\Textures\\GraveFight\\make_grave_fight.py -Threads 6

It writes these here, and Saved/ArtPreviews/Creatures/Gravemother/GraveFight_textures.png (each over the Sink's floor or
black, its alpha beside it). Tools/Unreal/build_grave_fight_materials.py imports them to /Game/Art/Effects/GraveFight and
makes their materials.

Cards (camera-facing quads; the colour is a grey shade the material multiplies by the card's tint, alpha its opacity):
  T_GraveFight_DustPuffs     2x2 sheet (1024 px): fresh grave earth's dust, four billowed clumps lit from the top left,
                             gritty, a dense core (MI_FX_GraveDust; tinted #806B55)
  T_GraveFight_FloorPuffs    2x2 sheet (1024 px): the floor's pale dust, softer and thinner, wispy edges (MI_FX_FloorDust)
  T_GraveFight_DustJet       two jets side by side (512 x 1024): a column of dust jetting from a crack, narrow at its foot
                             (the bottom of the texture) and billowing as it rises (MI_FX_GraveDustJet)
  T_GraveFight_SparkStreak   512 x 64: a spark's streak, a white-hot head at the right cooling down its tail
                             (MI_FX_StoneSpark; additive)
  T_GraveFight_Flash         256 px: a hit's flash, a hot core and six rays (MI_FX_GraveFlash; additive)
Decals (T_GraveFight_<Name>_BC: colour, alpha the opacity; _N DirectX normal map; _ORM: occlusion 1, roughness, metal 0):
  HeaveCrack      1024 x 256, 200 x 50 cm: the heave's split, broken lips of lifted earth, hairlines, dust round it.
                  Tiles along U (its length).
  HeaveCrackEnd   256 px, 50 x 50 cm: the strip's end, the split tapering shut: its U = 0 edge is the strip's own edge
                  (lay it on past the strip's last tile; flipped in U at the line's other end).
  Furrow          512 x 256, 200 x 100 cm: the ploughed strip, churned dark earth, clods, ragged soft sides. Tiles in U.
  ImpactScar      512 px: where a clod bursts (or she slams, or a mound bursts): a splat of dark earth thrown out in
                  fingers, a pit, clods at its rim, a dusty halo.
  LandingShadow   256 px: the clod's shadow where it will land: a soft dark disc with a firmer rim (colour only).
  MoundCracks     512 px: a mound's top splitting: cracks out from an opening, a ring crack, broken lips.
"""
import math
import os
import random
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Tools', 'Blender'))
import looter_textures as lt  # noqa: E402 - needs the path above

PREVIEW = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Creatures', 'Gravemother', 'GraveFight_textures.png')
FLOOR = 0xb59a76
WRITTEN = {}


def log(*a):
    print('GRAVEFIGHT:', *a, flush=True)


def C(value):
    return lt.rgb(value)


def grid(h, w):
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    return (x + 0.5) / w, (y + 0.5) / h


def polar(size):
    x, y = grid(size, size)
    u, v = x * 2.0 - 1.0, y * 2.0 - 1.0
    return u, v, np.sqrt(u * u + v * v), np.arctan2(v, u)


def save_card(name, shade, alpha):
    """A card: RGBA, the colour a shade (the material tints it), straight alpha; the colour spread under transparent
    texels so filtering never pulls in black."""
    alpha = np.clip(alpha, 0.0, 1.0)
    rgb = lt.fill_transparent(np.clip(shade, 0.0, 1.0), alpha, threshold=0.02)
    path = os.path.join(HERE, f'T_GraveFight_{name}.png')
    lt.write_png(path, np.concatenate([lt.to8(rgb), lt.to8(alpha)[..., None]], axis=-1))
    WRITTEN[name] = ('card', rgb, alpha)
    log(f'{name}: {rgb.shape[1]} x {rgb.shape[0]}')


def save_decal(name, rgb, alpha, height, px_m, rough, normal_strength=1.0):
    """A decal's three maps: colour with its opacity, the normal map of its relief, roughness."""
    alpha = np.clip(alpha, 0.0, 1.0)
    rgb = lt.fill_transparent(np.clip(rgb, 0.0, 1.0), alpha, threshold=0.02)
    base = os.path.join(HERE, f'T_GraveFight_{name}')
    lt.write_png(base + '_BC.png', np.concatenate([lt.to8(rgb), lt.to8(alpha)[..., None]], axis=-1))
    lt.write_png(base + '_N.png', lt.normal_map(height.astype(np.float32), px_m, normal_strength))
    orm = np.stack([np.ones_like(rough), np.clip(rough, 0.0, 1.0), np.zeros_like(rough)], axis=-1)
    lt.write_png(base + '_ORM.png', lt.to8(orm))
    WRITTEN[name] = ('decal', rgb, alpha)
    log(f'{name}: {rgb.shape[1]} x {rgb.shape[0]}')


def lit_shade(alpha, blur_px, k):
    """A soft shape lit from the top left: brighter where its blurred edge faces up and left."""
    h = lt.blur(alpha, blur_px)
    gy, gx = np.gradient(h)
    return 0.84 + k * (gy * 0.9 + gx * 0.45)


def angle_bumps(th, spikes, width):
    out = np.zeros_like(th)
    for a, k in spikes:
        d = np.angle(np.exp(1j * (th - a)))
        out += k * np.exp(-(d / width) ** 2)
    return out


def seg_dist(px, py, ax, ay, bx, by, wrap=None):
    dx = px - ax
    if wrap:
        dx = dx - wrap * np.round(dx / wrap)
    vx, vy = bx - ax, by - ay
    t = np.clip((dx * vx + (py - ay) * vy) / max(vx * vx + vy * vy, 1e-9), 0.0, 1.0)
    return np.sqrt((dx - t * vx) ** 2 + (py - ay - t * vy) ** 2), t


def wander(start, heading, length, steps, rng, turn=0.25):
    pts = [start]
    for _ in range(steps):
        heading += rng.uniform(-turn, turn)
        x, y = pts[-1]
        pts.append((x + math.cos(heading) * length / steps, y + math.sin(heading) * length / steps))
    return pts


# --- Cards ---

def puff(size, seed, blobs, spread, grit, soft):
    """One dust clump: soft billows clustered round the middle, ragged at its edge, lit from the top left."""
    u, v, _, _ = polar(size)
    rng = np.random.default_rng(seed)
    a = np.zeros((size, size), np.float32)
    for _ in range(blobs):
        ang, r = rng.uniform(0, 2 * math.pi), spread * math.sqrt(rng.random())
        cx, cy = r * math.cos(ang), r * math.sin(ang) * 0.85 + 0.03
        rad = rng.uniform(0.42, 0.62) * (1.0 - 0.4 * r)
        d = np.sqrt((u - cx) ** 2 + (v - cy) ** 2) / rad
        a = np.maximum(a, lt.smooth(1.0, soft, d) * rng.uniform(0.82, 1.0))
    # Billowed: the edge torn by noise, a few thinner wisps inside, never holes.
    edge = lt.noise((size, size), seed + 100, size / 28.0, octaves=3)
    a = a * np.clip(0.84 + 0.24 * edge, 0.0, 1.2)
    a = a * (1.0 - 0.18 * lt.smooth(0.4, 1.6, lt.noise((size, size), seed + 101, size / 40.0)))
    a = np.clip(a, 0.0, 1.0) * lt.smooth(1.0, 0.88, np.maximum(np.abs(u), np.abs(v)))
    shade = np.clip(lit_shade(a, size / 28.0, 5.5) * (0.95 + 0.05 * lt.noise((size, size), seed + 102, size / 40.0)),
                    0.5, 1.0)
    if grit:
        # Fresh earth's dust carries grit: small darker clumps in it, thickest in the core.
        specks = lt.specks((size, size), seed + 103, 0.04, size / 300.0) * lt.smooth(0.35, 0.85, a)
        shade = shade * (1.0 - 0.25 * specks)
    return shade, a


def paint_sheet(name, seed, cell, blobs, spread, grit, soft, density):
    shade = np.zeros((cell * 2, cell * 2), np.float32)
    alpha = np.zeros((cell * 2, cell * 2), np.float32)
    for k in range(4):
        s, a = puff(cell, seed + k, blobs + k % 2 * 2, spread, grit, soft)
        oy, ox = (k // 2) * cell, (k % 2) * cell
        shade[oy:oy + cell, ox:ox + cell] = s
        alpha[oy:oy + cell, ox:ox + cell] = a * density
    save_card(name, np.dstack([shade, shade * 0.975, shade * 0.94]), alpha)


def paint_jets(w=256, h=1024):
    """Two dust jets side by side: a column of billows, narrow at the crack (the texture's bottom), spreading and thinning
    as it rises."""
    x, y = grid(h, w)
    u, t = x * 2.0 - 1.0, 1.0 - y
    shades, alphas = [], []
    for k in range(2):
        rng = np.random.default_rng(60 + k)
        a = np.zeros((h, w), np.float32)
        n = 20
        for i in range(n):
            ti = (i + rng.uniform(0.2, 0.8)) / n * 0.95
            width = 0.16 + 0.7 * ti ** 0.8
            cx = 0.14 * math.sin(ti * 6.0 + k * 2.0) * ti
            rx = width * rng.uniform(0.65, 1.0)
            ry = rx * w / (2.0 * h) * 1.4
            d = np.sqrt(((u - cx) / rx) ** 2 + ((t - ti) / ry) ** 2)
            a = np.maximum(a, lt.smooth(1.0, 0.3, d) * (1.0 - 0.55 * ti))
        a *= np.clip(0.75 + 0.3 * lt.noise((h, w), 70 + k, 6.0, 18.0, octaves=3), 0.0, 1.2)
        a = np.clip(a * lt.smooth(0.0, 0.04, t) * lt.smooth(1.0, 0.72, t) * lt.smooth(1.0, 0.84, np.abs(u)), 0, 1)
        grit = lt.specks((h, w), 80 + k, 0.04, 1.5) * lt.smooth(0.4, 0.0, t) * lt.smooth(0.2, 0.6, a)
        shade = np.clip(lit_shade(a, 8.0, 4.0), 0.55, 1.0) * (1.0 - 0.3 * grit)
        shades.append(np.dstack([shade, shade * 0.97, shade * 0.93]))
        alphas.append(np.clip(a + grit * 0.3, 0.0, 1.0))
    save_card('DustJet', np.concatenate(shades, axis=1), np.concatenate(alphas, axis=1))


def paint_flash(size=256):
    u, v, r, th = polar(size)
    rng = np.random.default_rng(51)
    spikes = [(2 * math.pi * k / 6 + rng.uniform(-0.25, 0.25), rng.uniform(0.6, 1.0)) for k in range(6)]
    rays = angle_bumps(th, spikes, 0.045) * lt.smooth(0.95, 0.08, r)
    core = np.exp(-(r / 0.13) ** 2)
    a = np.clip(core * 1.2 + rays * 0.95 + 0.35 * np.exp(-(r / 0.38) ** 2), 0.0, 1.0) * lt.smooth(1.0, 0.9, r)
    hot = np.clip(core * 1.6 + rays * 0.5, 0.0, 1.0)
    save_card('Flash', lt.mix(np.ones((size, size, 3), np.float32) * C(0xffa04a), C(0xfff6e2), hot), a)


def paint_streak(w=512, h=64):
    x, y = grid(h, w)
    v = y * 2.0 - 1.0
    width = 0.12 + 0.88 * x ** 1.6
    a = lt.smooth(width, width * 0.25, np.abs(v)) * x ** 1.2 * lt.smooth(1.0, 0.95, x)
    rgb = lt.mix(np.ones((h, w, 3), np.float32) * C(0xff5a14), C(0xffd37a), lt.smooth(0.2, 0.75, x))
    rgb = lt.mix(rgb, C(0xfffbf0), lt.smooth(0.8, 0.97, x) * lt.smooth(0.6, 0.0, np.abs(v)))
    save_card('SparkStreak', rgb, a)


# --- Decals ---

CRACK_TILE_CM, CRACK_W_CM = 200.0, 50.0
CAP_FRAC = 0.25


def crack_field(h, w, cap):
    """The heave crack on one tile (x along it, periodic; y across): the dark split, broken lips of lifted earth in
    plates, hairline branches, disturbed soil and dust round it; with cap, tapered shut over the first quarter (so the
    cap's join edge is the strip's own edge). Colour, opacity, height (m), roughness."""
    x, y = grid(h, w)
    rng = np.random.default_rng(5)
    ph = rng.uniform(0, 2 * math.pi, 10)

    def yc_f(xx):
        return 0.5 + sum(a * np.sin(2 * math.pi * k * xx + p) for k, a, p in
                         zip((1, 2, 3, 5, 8, 13), (0.05, 0.035, 0.022, 0.012, 0.007, 0.004), ph[:6]))

    def hw_f(xx):
        return 0.085 + 0.018 * np.sin(2 * math.pi * 2 * xx + ph[6]) + 0.01 * np.sin(2 * math.pi * 7 * xx + ph[7])

    def taper_f(xx):
        if not cap:
            return np.ones_like(xx) if isinstance(xx, np.ndarray) else 1.0
        return np.clip(1.0 - xx / CAP_FRAC, 0.0, 1.0) ** 0.6

    n1 = lt.noise((h, w), 11, 2.0, octaves=2)
    n2 = lt.noise((h, w), 12, 14.0, 6.0, octaves=3)
    yc, hw, taper = yc_f(x), hw_f(x), taper_f(x)
    jit = 0.006 * (np.sin(2 * math.pi * 31 * x + ph[8]) + 0.6 * np.sin(2 * math.pi * 53 * x + ph[9])) + 0.006 * n1
    ad = np.abs(y - yc)
    w_open = np.maximum(hw * taper, 1e-3)
    core = lt.smooth(w_open + 0.004, w_open - 0.004, ad + jit) * (taper > 0.03)
    depth = np.clip(1.0 - ad / w_open, 0.0, 1.0)
    lipw = 0.1 * np.sqrt(taper) + 0.012
    lip = lt.smooth(w_open + lipw, w_open + lipw * 0.35, ad + 0.02 * n1) * (1.0 - core)
    lip *= lt.smooth(-1.0, -0.4, n2)
    cells = lt.worley((h, w), (int(w / 40), int(h / 40)), 13)
    plate = lt.per_cell(cells['id'], cells['count'], 14, 0.82, 1.1)
    seam = lt.smooth(2.5, 0.5, cells['edge'])
    dw = 0.34 * taper ** 0.35 + 0.05
    dust = lt.smooth(w_open + dw, w_open + 0.04, ad) * np.clip(0.55 + 0.35 * n2, 0.0, 1.0) * taper ** 0.3
    X, Y = x * CRACK_TILE_CM, y * CRACK_W_CM
    hair = np.zeros_like(x)
    rng2 = np.random.default_rng(7)
    K = 10
    for k in range(K):
        xk = (k + rng2.uniform(0.15, 0.85)) / K
        tk = float(taper_f(xk))
        if tk < 0.15:
            continue
        side = 1 if k % 2 else -1
        start = (xk * CRACK_TILE_CM, float(yc_f(xk) + side * hw_f(xk) * tk) * CRACK_W_CM)
        ang = math.radians(rng2.uniform(30, 70)) * (1 if rng2.random() < 0.5 else -1)
        heading = math.atan2(side * math.sin(abs(ang)), math.cos(ang))
        pts = wander(start, heading, rng2.uniform(7, 16) * tk, 4, random.Random(k), 0.3)
        for j in range(len(pts) - 1):
            d, t = seg_dist(X, Y, *pts[j], *pts[j + 1], wrap=CRACK_TILE_CM)
            wseg = 0.45 - 0.3 * (j + t) / (len(pts) - 1)
            hair = np.maximum(hair, lt.smooth(wseg, wseg * 0.3, d))
    if cap:
        y0 = float(yc_f(CAP_FRAC * 0.8)) * CRACK_W_CM
        d, t = seg_dist(X, Y, CAP_FRAC * 0.8 * CRACK_TILE_CM, y0, CAP_FRAC * 0.97 * CRACK_TILE_CM, y0 + 1.6)
        hair = np.maximum(hair, lt.smooth(0.3 - 0.2 * t, 0.06, d))
    rgb = np.ones((h, w, 3), np.float32) * C(0x8a7356) * (0.92 + 0.08 * n2)[..., None]
    lip_col = lt.mix(C(0xb39878)[None, None, :] * plate[..., None], C(0x5d4936), seam * 0.8)
    rgb = lt.mix(rgb, lip_col, lip)
    rgb = lt.mix(rgb, C(0x2b2018), hair)
    rgb = lt.mix(rgb, lt.mix(C(0x4f3c2b), C(0x0d0a07), depth ** 0.5), core)
    alpha = np.maximum.reduce([dust * 0.5, lip * 0.92, hair * 0.85, core])
    alpha *= lt.smooth(0.0, 0.07, y) * lt.smooth(1.0, 0.93, y)
    if cap:
        alpha *= lt.smooth(CAP_FRAC, CAP_FRAC * 0.9, x)
    # Relief: the lips stand up a little in plates (seams between them), the split falls away.
    height = 0.006 * lip * plate - 0.002 * seam * lip - 0.02 * core * depth ** 0.5 - 0.002 * hair + 0.001 * dust * n2
    rough = 0.92 - 0.25 * core
    return rgb, alpha, height, rough


def paint_crack():
    rgb, a, h, r = crack_field(256, 1024, cap=False)
    save_decal('HeaveCrack', rgb, a, h, CRACK_TILE_CM / 1024 * 0.01, r)
    rgb, a, h, r = crack_field(256, 1024, cap=True)
    save_decal('HeaveCrackEnd', rgb[:, :256], a[:, :256], h[:, :256], CRACK_TILE_CM / 1024 * 0.01, r[:, :256])


def paint_furrow(h=256, w=512):
    """The ploughed strip (200 x 100 cm, tiling along its length): churned dark earth in ridges along it, clods lit from
    above, a damp darker middle, ragged soft sides."""
    x, y = grid(h, w)
    n = lt.noise((h, w), 81, 6.0, octaves=3)
    n2 = lt.noise((h, w), 82, 24.0, 4.0, octaves=2)
    ay = np.abs(y - 0.5)
    edge = 0.37 + 0.05 * n2
    a = lt.smooth(edge + 0.07, edge - 0.05, ay + 0.03 * n)
    ridges = np.sin(y * 2.0 * math.pi * 7.0 + 1.6 * lt.noise((h, w), 85, 30.0, 3.0)) * 0.5 + 0.5
    # Clods turned up here and there, of every size; crumbs between.
    xx, yy = lt.coords((h, w))
    wx, wy = xx + 7.0 * lt.noise((h, w), 86, 8.0), yy + 7.0 * lt.noise((h, w), 87, 8.0)
    cells = lt.worley((h, w), (20, 10), 83, x=wx, y=wy)
    clod_r = lt.per_cell(cells['id'], cells['count'], 84, 0.0, 1.0)
    size = lt.per_cell(cells['id'], cells['count'], 88, 4.0, 11.0)
    clod = np.sqrt(np.clip(1.0 - (cells['f1'] / size) ** 2, 0.0, 1.0)) * (clod_r > 0.55)
    crumbs = lt.noise((h, w), 89, 1.5, octaves=2)
    height = (0.012 * clod + 0.004 * ridges * lt.smooth(0.4, 0.1, ay) + 0.003 * n + 0.0012 * crumbs
              - 0.006 * lt.smooth(0.2, 0.0, ay))
    gy, gx = np.gradient(lt.blur(height, 1.5))
    lit = np.clip(1.0 + (-gy * 0.9 - gx * 0.4) * 160.0, 0.7, 1.3)
    rgb = np.ones((h, w, 3), np.float32) * C(0x56442f) * (0.85 + 0.15 * n)[..., None]
    rgb = lt.mix(rgb, C(0x33271b), lt.smooth(0.22, 0.0, ay) * 0.55)
    rgb = lt.mix(rgb, C(0x7a634b), clod * 0.6)
    rgb = rgb * lit[..., None]
    rough = 0.94 - 0.2 * lt.smooth(0.2, 0.0, ay)
    save_decal('Furrow', rgb, a, height, 2.0 / w, rough)


def paint_scar(size=512):
    """Where a clod bursts: a splat of churned dark earth thrown out in fingers, a pit, clods round its rim, a halo."""
    u, v, r, th = polar(size)
    rng = np.random.default_rng(9)
    spikes = [(rng.uniform(-math.pi, math.pi), rng.uniform(0.3, 1.0)) for _ in range(16)]
    reach = 0.48 + 0.3 * angle_bumps(th, spikes, 0.07) + 0.04 * np.sin(5 * th + 1.0) + 0.03 * np.sin(11 * th + 2.0)
    n = lt.noise((size, size), 21, 6.0, octaves=3)
    rr = r / reach
    splat = lt.smooth(1.0, 0.8, rr + 0.06 * n)
    pit = lt.smooth(0.42, 0.12, r + 0.05 * n)
    halo = lt.smooth(1.5, 0.95, rr + 0.1 * n) * (1.0 - splat)
    cells = lt.worley((size, size), (16, 16), 22)
    clump = lt.smooth(6.0, 2.0, cells['f1']) * lt.smooth(0.5, 0.9, rr) * splat
    rgb = np.ones((size, size, 3), np.float32) * C(0xa38866) * (0.9 + 0.1 * n)[..., None]
    rgb = lt.mix(rgb, C(0x4d3b2a) * (0.85 + 0.15 * n)[..., None], splat)
    rgb = lt.mix(rgb, C(0x261c14), pit)
    rgb = lt.mix(rgb, C(0x7a634c), clump * 0.8)
    height = -0.03 * pit + 0.008 * clump + 0.003 * splat * n
    save_decal('ImpactScar', rgb, np.maximum(halo * 0.42, splat * 0.95) * lt.smooth(1.0, 0.94, r), height,
               2.6 / size, 0.93 - 0.1 * pit)


def paint_shadow(size=256):
    u, v, r, th = polar(size)
    a = 0.5 * np.clip(1.0 - r, 0.0, 1.0) ** 0.7 + 0.35 * np.exp(-((r - 0.9) / 0.035) ** 2)
    a = a * lt.smooth(1.0, 0.97, r)
    save_decal('LandingShadow', np.ones((size, size, 3), np.float32) * C(0x120d0a), a, np.zeros((size, size)),
               1.0 / size, np.full((size, size), 0.95, np.float32))


def paint_mound_crack(size=512):
    """A spawn mound's top splitting: cracks radiating from an opening, a ring crack, lighter broken lips."""
    u, v, r, th = polar(size)
    rng = random.Random(41)
    n = lt.noise((size, size), 42, 3.0, octaves=2)
    mask = lt.smooth(0.11, 0.05, r + 0.02 * n)
    lipd = np.full_like(r, 9.0)
    lines = []
    for k in range(7):
        a = 2 * math.pi * k / 7 + rng.uniform(-0.3, 0.3)
        lines.append((wander((math.cos(a) * 0.04, math.sin(a) * 0.04), a, rng.uniform(0.5, 0.82), 6, rng, 0.3), 0.05))
    a0 = rng.uniform(0, 6.3)
    lines.append(([(math.cos(a0 + 0.3 * j) * 0.34, math.sin(a0 + 0.3 * j) * 0.34) for j in range(8)], 0.03))
    for pts, w0 in lines:
        for j in range(len(pts) - 1):
            d, t = seg_dist(u, v, *pts[j], *pts[j + 1])
            wseg = w0 * (1.0 - 0.75 * (j + t) / (len(pts) - 1)) + 0.005
            mask = np.maximum(mask, lt.smooth(wseg, wseg * 0.5, d + 0.006 * n))
            lipd = np.minimum(lipd, d - wseg)
    lip = lt.smooth(0.018, 0.0, lipd) * (1.0 - mask)
    rgb = lt.mix(np.ones((size, size, 3), np.float32) * C(0x8c7660), C(0x110c09), mask)
    height = -0.02 * mask + 0.004 * lip
    save_decal('MoundCracks', rgb, np.maximum(mask, lip * 0.45) * lt.smooth(1.0, 0.95, r), height, 1.7 / size,
               0.93 - 0.1 * mask)


# --- The preview sheet ---

def preview():
    tiles = []
    floor = C(FLOOR)
    for name, (kind, rgb, alpha) in WRITTEN.items():
        h, w = alpha.shape
        scale = max(1, int(math.ceil(max(h, w) / 512)))
        rgb_s, a_s = rgb[::scale, ::scale], alpha[::scale, ::scale]
        if kind == 'card':
            back = np.zeros_like(rgb_s)
            tint = C(0x806b55) if 'Dust' in name or 'Puffs' in name else np.ones(3, np.float32)
            if name == 'FloorPuffs':
                tint = C(0xdcc9a4)
            over_dark = back * (1 - a_s[..., None]) + rgb_s * tint * a_s[..., None]
            over_floor = floor * (1 - a_s[..., None]) + rgb_s * tint * a_s[..., None]
            pair = [over_dark, over_floor] if 'Spark' not in name and 'Flash' not in name else [rgb_s * a_s[..., None]]
        else:
            pair = [floor * (1 - a_s[..., None]) + rgb_s * a_s[..., None]]
        pair.append(np.repeat(a_s[..., None], 3, axis=-1))
        tile = np.concatenate([np.pad(p, ((0, 0), (0, 6), (0, 0)), constant_values=0.1) for p in pair], axis=1)
        tiles.append(tile)
    width = max(t.shape[1] for t in tiles)
    rows = [np.pad(t, ((0, 8), (0, width - t.shape[1]), (0, 0)), constant_values=0.08) for t in tiles]
    lt.write_png(PREVIEW, lt.to8(np.clip(np.concatenate(rows, axis=0), 0.0, 1.0)))
    log(f'preview {PREVIEW}')


paint_sheet('DustPuffs', 40, 512, 10, 0.34, True, 0.3, 1.0)
paint_sheet('FloorPuffs', 140, 512, 9, 0.42, False, 0.05, 0.9)
paint_jets()
paint_streak()
paint_flash()
paint_crack()
paint_furrow()
paint_scar()
paint_shadow()
paint_mound_crack()
preview()
