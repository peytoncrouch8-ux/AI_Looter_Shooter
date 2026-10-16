"""The Gravemother's fight, concept D "Barrow", drawn as a sheet the fight can be built from (2026-10-15). The user
picked D from Gravemother_Fight_D_Barrow.png (GravemotherFightConcepts.py) and asked for it made better: every move in
time, every effect piece as the FX artist needs it, her turnaround and the spiderling. Concept art kept in Art/Backlog
(see its README): the game's build never reads this file.

The fight as decided (the user's calls, 2026-10-15):
  Grave Clod   front legs dig in and lift a clod overhead (the tell, 1.2 s); she lobs it (about 1.5 s to the player at
               20 m); it bursts into dust and leaves a rubble pile (cover, 10 s). Shooting the clod in her grip bursts
               it on her and stuns her 2 s.
  Furrow       a 2 s tell: the ground heaves and splits in a line toward the player, dust jetting from the crack; then
               she ploughs the line head-low and throws up a berm each side (cover for 20 s, then they crumble).
  Unearthed    at 66% and 33% she slams the floor and 4 spiderlings (the game's spider at 0.45x) dig out of mounds that
               bulge for 1 s first; a mound shot while it bulges kills the spiderling in it.
  Phase 2      the headstone is armour (shots glance off with sparks and chips, no damage). At 33% it cracks off and
               her pale Bone abdomen is bare; she runs faster. Crits: the head, and the abdomen's soft rear while the
               stone is on; the whole abdomen after.
Her look is GravemotherFightConcepts.py's D (its barrow()): mud-grey hide on the game's spider at 1.8x, a toppled
sandstone headstone bound to her abdomen by roots, grave mud caked on her legs.

Everything drawn is buildable at 1080p Medium without Lumen: meshes, sprite cards with soft alpha (painted here as
textures: dust puffs on a 2x2 sheet, dust jets, the spark streak, the flash), decals (the heave crack's tiling strip and
its end cap, the ploughed furrow, the impact scar, the landing shadow, the stone's bullet scar, the mound's cracks) and
emissive surfaces. No compositor bloom (the game has its own), no volumes. Freshly dug grave earth, and its dust, are
darker than the sun-baked floor and the Sink's wall, so clods, rubble, berms, mounds and bursts read against both;
footfalls kick up the paler floor dust. The headstone keeps the grey stone of the sheet the user picked (a warm
sandstone would vanish against the Sink's sandstone wall). The textures are painted into Intermediate/GravemotherBarrow
(never Art/Textures).

Renders go to Saved/ArtPreviews/RansomsRest/Concepts/GravemotherBarrow/BW_<part>_<shot>.png, and BW.json holds every
render's file, the callout points projected into it and the sheet's words. GravemotherBarrowSheets.ps1 assembles them
into Saved/Concepts/Gravemother/Barrow_Moves.png, Barrow_FX.png, Barrow_Hero.png and Barrow_Spiderling.png.
  moves   five strips of six frames, one fixed camera each: clod, countered, furrow, unearthed, phase2
  fx      each effect piece alone, rendered on black (BW_fx_<piece>.png) and again for its alpha (BW_fx_<piece>_A.png)
  hero    her turnaround (front, three-quarter, side, back, top), bare after 33%, the stone up close
  ling    the spiderling digging out: in the mound, half out, out

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Backlog\\Creatures\\GravemotherBarrowSheet.py
    ... -ScriptArgs moves,hero        some parts (none named: all of them)
    ... -ScriptArgs clod,furrow       some strips of the moves (implies moves)
    ... -ScriptArgs fx,quick          half size, few samples
    powershell -NoProfile -File Art\\Backlog\\Creatures\\GravemotherBarrowSheets.ps1
"""
import json
import math
import os
import random
import sys
import time
import zlib

import bmesh
import bpy
import numpy as np
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Euler, Matrix, Vector, noise

import looter_creatures as lc
import looter_textures as lt

REPO = lt.REPO
FIGHT_PY = os.path.join(REPO, 'Art', 'Backlog', 'Creatures', 'GravemotherFightConcepts.py')
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Concepts', 'GravemotherBarrow')
MAP_DIR = os.path.join(REPO, 'Intermediate', 'GravemotherBarrow')
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
QUICK = 'quick' in ARGV
PART_KEYS = ('moves', 'fx', 'hero', 'ling')
STRIP_KEYS = ('clod', 'countered', 'furrow', 'unearthed', 'phase2')
FRAME = (772, 420)              # a moves frame as the sheet shows it
CELL = (772, 434)               # an FX cell


def log(*a):
    print('BARROW:', *a, flush=True)


def borrow_fight():
    """GravemotherFightConcepts.py without its main(): the stage, the posed spider, D's dressing, the FX builders."""
    source = open(FIGHT_PY, encoding='utf-8').read()
    head, mark, tail = source.rpartition('\nmain()')
    if not mark or tail.strip():
        raise RuntimeError(f'{FIGHT_PY} no longer ends with main(): update borrow_fight')
    keep = sys.argv[:]
    sys.argv = [keep[0], '--']            # it reads its own arguments: give it none
    ns = {'__name__': 'gravemother_fight', '__file__': FIGHT_PY}
    try:
        exec(compile(head, FIGHT_PY, 'exec'), ns)
    finally:
        sys.argv = keep
    return ns


FC = borrow_fight()
G = FC['G']
for _name in ('SPIDER', 'Pose', 'spider', 'cpt', 'cdir', 'foot', 'wm', 'done', 'finish_bm', 'card', 'blobs', 'tube',
              'ribbon', 'jag', 'fan', 'splat_radius', 'basis', 'fbm', 'T', 'textured', 'mat_surface', 'mat_emit',
              'mat_trail', 'fresh', 'mnode', 'gravemother', 'barrow', 'headstone_mesh', 'stone_frame', 'clod',
              'ab_point', 'attach', 'figure', 'clear_shot', 'face_cards', 'gm_hide', 'HEAD_C', 'ABD_C', 'TOB',
              'SCALE', 'LING', 'STONE_W', 'STONE_H', 'STONE_T', 'MATS', 'DIG', 'CLOD', 'CHARGE', 'SHRIEK', 'STAND',
              'HEAVY'):
    globals()[_name] = FC[_name]
SCENE = bpy.context.scene
RIDE = SPIDER['RIDE']
UP = Vector((0.0, 0.0, 1.0))
DUST = 0xdcc9a4                 # floor dust (footfalls): a shade lighter than the floor
EARTH_DUST = 0x806b55           # dust of fresh grave earth: darker than the sun-baked floor and wall, so it reads on both
STONE_DUST = 0xd2cdc3


# --- Painted textures: sprites and decals ---

TEX = {}


def grid(h, w):
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    return (x + 0.5) / w, (y + 0.5) / h


def polar(size):
    x, y = grid(size, size)
    u, v = x * 2.0 - 1.0, y * 2.0 - 1.0
    return u, v, np.sqrt(u * u + v * v), np.arctan2(v, u)


def C(value):
    return lt.rgb(value)


def save_tex(name, rgb, alpha):
    """An RGBA texture and its alpha alone (the sheet shows both)."""
    path = os.path.join(MAP_DIR, f'T_BW_{name}.png')
    alpha = np.clip(alpha, 0.0, 1.0)
    lt.write_png(path, lt.to8(np.dstack([np.clip(rgb, 0.0, 1.0), alpha])))
    lt.write_png(os.path.join(MAP_DIR, f'T_BW_{name}_A.png'), lt.to8(alpha))
    TEX[name] = path
    return path


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
    """Distance from every pixel to the segment a-b (and how far along it), x wrapping by wrap if given."""
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


def paint_puffs(size=256):
    """Four dust puffs on a 2x2 sheet: clustered soft billows, ragged edges, lit from the top left. Every dust card in
    the fight is one of these, tinted."""
    rgb_sheet = np.zeros((size * 2, size * 2, 3), np.float32)
    a_sheet = np.zeros((size * 2, size * 2), np.float32)
    u, v, _, _ = polar(size)
    for k in range(4):
        rng = np.random.default_rng(40 + k)
        a = np.zeros((size, size), np.float32)
        for _ in range(int(rng.integers(9, 13))):
            ang, r = rng.uniform(0, 2 * math.pi), 0.3 * math.sqrt(rng.random())
            cx, cy = r * math.cos(ang), r * math.sin(ang) * 0.85 + 0.03
            rad = rng.uniform(0.3, 0.48) * (1.0 - 0.45 * r)
            d = np.sqrt((u - cx) ** 2 + (v - cy) ** 2) / rad
            a = np.maximum(a, lt.smooth(1.0, 0.25, d) * rng.uniform(0.8, 1.0))
        a *= np.clip(0.86 + 0.2 * lt.noise((size, size), 100 + k, 10.0, octaves=2), 0.0, 1.2)
        a = np.clip(a, 0.0, 1.0) * lt.smooth(1.0, 0.88, np.maximum(np.abs(u), np.abs(v)))
        shade = np.clip(lit_shade(a, 9.0, 6.0) * (0.96 + 0.04 * lt.noise((size, size), 300 + k, 12.0)), 0.55, 1.0)
        oy, ox = (k // 2) * size, (k % 2) * size
        rgb_sheet[oy:oy + size, ox:ox + size] = np.dstack([shade, shade * 0.97, shade * 0.92])
        a_sheet[oy:oy + size, ox:ox + size] = a
    save_tex('DustPuffs', rgb_sheet, a_sheet)


def paint_jets(w=128, h=512):
    """Two dust jets side by side: a column of billows, narrow at the crack and spreading as it rises."""
    x, y = grid(h, w)
    u, t = x * 2.0 - 1.0, 1.0 - y
    rgbs, alphas = [], []
    for k in range(2):
        rng = np.random.default_rng(60 + k)
        a = np.zeros((h, w), np.float32)
        n = 16
        for i in range(n):
            ti = (i + rng.uniform(0.2, 0.8)) / n * 0.95
            width = 0.2 + 0.65 * ti ** 0.8
            cx = 0.15 * math.sin(ti * 6.0 + k * 2.0) * ti
            rx = width * rng.uniform(0.7, 1.0)
            ry = rx * w / (2.0 * h) * 1.4
            d = np.sqrt(((u - cx) / rx) ** 2 + ((t - ti) / ry) ** 2)
            a = np.maximum(a, lt.smooth(1.0, 0.35, d) * (1.0 - 0.5 * ti))
        a *= np.clip(0.78 + 0.25 * lt.noise((h, w), 70 + k, 3.0, 9.0, octaves=3), 0.0, 1.2)
        a = np.clip(a * lt.smooth(0.0, 0.05, t) * lt.smooth(1.0, 0.75, t) * lt.smooth(1.0, 0.85, np.abs(u)), 0, 1)
        shade = np.clip(lit_shade(a, 5.0, 4.0), 0.55, 1.0)
        rgbs.append(np.dstack([shade, shade * 0.97, shade * 0.92]))
        alphas.append(a)
    save_tex('DustJets', np.concatenate(rgbs, axis=1), np.concatenate(alphas, axis=1))


CRACK_TILE_CM, CRACK_W_CM = 200.0, 50.0      # the heave crack's strip: it tiles along its 200 cm
CAP_FRAC = 0.25                              # the end cap is the strip's first 50 cm, tapered shut: 50 x 50 cm


def crack_field(h, w, cap):
    """The heave crack on one tile (x along it, periodic; y across): the dark split, broken lips of lifted earth in
    plates, hairline branches and disturbed soil round it. cap: tapered shut over the first quarter, so the cap's join
    edge is the strip's own edge."""
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
    # Hairlines branching off the split, in centimetres; the strip's wrap lets them cross its ends.
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
        # The split runs out as one hairline past the cap's taper.
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
    return rgb, alpha


def paint_crack():
    rgb, a = crack_field(256, 1024, cap=False)
    save_tex('HeaveCrack', rgb, a)
    rgb, a = crack_field(256, 1024, cap=True)
    save_tex('HeaveCrackCap', rgb[:, :256], a[:, :256])


def paint_scar(size=512):
    """Where a clod bursts: a splat of churned dark earth thrown out in fingers, clods round its rim, a dusty halo."""
    u, v, r, th = polar(size)
    rng = np.random.default_rng(9)
    spikes = [(rng.uniform(-math.pi, math.pi), rng.uniform(0.3, 1.0)) for _ in range(16)]
    reach = 0.48 + 0.3 * angle_bumps(th, spikes, 0.07) + 0.04 * np.sin(5 * th + 1.0) + 0.03 * np.sin(11 * th + 2.0)
    n = lt.noise((size, size), 21, 6.0, octaves=3)
    rr = r / reach
    splat = lt.smooth(1.0, 0.8, rr + 0.06 * n)
    pit = lt.smooth(0.42, 0.12, r + 0.05 * n)
    halo = lt.smooth(1.5, 0.95, rr + 0.1 * n) * (1.0 - splat)
    cells = lt.worley((size, size), (14, 14), 22)
    clump = lt.smooth(6.0, 2.0, cells['f1']) * lt.smooth(0.55, 0.9, rr) * splat
    rgb = np.ones((size, size, 3), np.float32) * C(0xa38866) * (0.9 + 0.1 * n)[..., None]
    rgb = lt.mix(rgb, C(0x4d3b2a) * (0.85 + 0.15 * n)[..., None], splat)
    rgb = lt.mix(rgb, C(0x261c14), pit)
    rgb = lt.mix(rgb, C(0x7a634c), clump * 0.8)
    save_tex('ImpactScar', rgb, np.maximum(halo * 0.42, splat * 0.95) * lt.smooth(1.0, 0.94, r))


def paint_shadow(size=256):
    """The clod's landing shadow: a soft dark disc with a firmer rim, scaled up as the clod comes down."""
    u, v, r, th = polar(size)
    a = 0.5 * np.clip(1.0 - r, 0.0, 1.0) ** 0.7 + 0.35 * np.exp(-((r - 0.9) / 0.035) ** 2)
    save_tex('LandShadow', np.ones((size, size, 3), np.float32) * C(0x120d0a), a * lt.smooth(1.0, 0.97, r))


def paint_chip(w=256, h=128):
    """A bullet's glancing scar on the headstone: a pale chipped gouge with a dark score along it."""
    x, y = grid(h, w)
    u, v = x * 2.0 - 1.0, y * 2.0 - 1.0
    n = lt.noise((h, w), 31, 4.0, octaves=3)
    e = np.sqrt((u / 0.8) ** 2 + (v / 0.5) ** 2) + 0.12 * n
    chip = lt.smooth(1.0, 0.85, e)
    groove = lt.smooth(0.16, 0.02, np.abs(v - 0.06 * u + 0.03 * n)) * lt.smooth(0.85, 0.2, np.abs(u + 0.1))
    rng = np.random.default_rng(32)
    dots = np.zeros_like(u)
    for _ in range(9):
        cx, cy, rad = rng.uniform(-0.95, 0.95), rng.uniform(-0.9, 0.9), rng.uniform(0.03, 0.07)
        dots = np.maximum(dots, lt.smooth(rad, rad * 0.5, np.sqrt(((u - cx) * 2.0) ** 2 + (v - cy) ** 2)))
    rgb = np.ones((h, w, 3), np.float32) * C(0xd8d1c4) * (0.92 + 0.08 * n)[..., None]
    rgb = lt.mix(rgb, C(0x9d968a), lt.smooth(0.6, 1.0, e) * chip)
    rgb = lt.mix(rgb, C(0x4b463e), groove)
    save_tex('StoneScar', rgb, np.maximum(chip, dots * 0.9))


def paint_mound_crack(size=512):
    """The spawn mound's top splitting: cracks radiating from an opening, one ring crack, lighter broken lips."""
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
    rgb = lt.mix(np.ones((size, size, 3), np.float32) * C(0x6e5a45), C(0x110c09), mask)
    save_tex('MoundCrack', rgb, np.maximum(mask, lip * 0.45) * lt.smooth(1.0, 0.95, r))


def paint_furrow(h=256, w=512):
    """The furrow she ploughs: a strip of churned dark earth with clods in it and ragged soft edges; it tiles along its
    length (200 x 100 cm)."""
    x, y = grid(h, w)
    n = lt.noise((h, w), 81, 6.0, octaves=3)
    n2 = lt.noise((h, w), 82, 24.0, 4.0, octaves=2)
    ay = np.abs(y - 0.5)
    edge = 0.36 + 0.05 * n2
    a = lt.smooth(edge + 0.07, edge - 0.05, ay + 0.03 * n)
    cells = lt.worley((h, w), (18, 9), 83)
    clod = lt.smooth(7.0, 2.0, cells['f1']) * lt.per_cell(cells['id'], cells['count'], 84, 0.3, 1.0)
    rgb = np.ones((h, w, 3), np.float32) * C(0x4e3d2d) * (0.85 + 0.15 * n)[..., None]
    rgb = lt.mix(rgb, C(0x2e231a), lt.smooth(0.25, 0.0, ay) * 0.6)
    rgb = lt.mix(rgb, C(0x7d6650), clod * 0.75)
    save_tex('Furrow', rgb, a)


def paint_flash(size=256):
    """A shot's hit flash: a hot core and six short rays."""
    u, v, r, th = polar(size)
    rng = np.random.default_rng(51)
    spikes = [(2 * math.pi * k / 6 + rng.uniform(-0.25, 0.25), rng.uniform(0.6, 1.0)) for k in range(6)]
    rays = angle_bumps(th, spikes, 0.045) * lt.smooth(0.95, 0.08, r)
    core = np.exp(-(r / 0.13) ** 2)
    a = np.clip(core * 1.2 + rays * 0.95 + 0.35 * np.exp(-(r / 0.38) ** 2), 0.0, 1.0) * lt.smooth(1.0, 0.9, r)
    hot = np.clip(core * 1.6 + rays * 0.5, 0.0, 1.0)
    save_tex('Flash', lt.mix(np.ones((size, size, 3), np.float32) * C(0xff9a3c), C(0xfff6e0), hot), a)


def paint_streak(w=256, h=32):
    """A spark's streak: a white-hot head at the right, cooling to orange and red down its tail."""
    x, y = grid(h, w)
    v = y * 2.0 - 1.0
    width = 0.12 + 0.88 * x ** 1.6
    a = lt.smooth(width, width * 0.25, np.abs(v)) * x ** 1.2 * lt.smooth(1.0, 0.94, x)
    rgb = lt.mix(np.ones((h, w, 3), np.float32) * C(0xff5a14), C(0xffd37a), lt.smooth(0.2, 0.75, x))
    rgb = lt.mix(rgb, C(0xfffbf0), lt.smooth(0.8, 0.97, x) * lt.smooth(0.6, 0.0, np.abs(v)))
    save_tex('SparkStreak', rgb, a)


def paint_all():
    os.makedirs(MAP_DIR, exist_ok=True)
    t0 = time.time()
    for fn in (paint_puffs, paint_jets, paint_crack, paint_scar, paint_shadow, paint_chip, paint_mound_crack,
               paint_furrow, paint_flash, paint_streak):
        fn()
    log(f'painted {len(TEX)} textures in {time.time() - t0:.1f} s')


# --- Materials ---

def tex_node(nt, name, extension='REPEAT'):
    node = nt.nodes.new('ShaderNodeTexImage')
    node.image = bpy.data.images.load(TEX[name], check_existing=True)
    node.image.colorspace_settings.name = 'sRGB'
    node.image.alpha_mode = 'STRAIGHT'
    node.extension = extension
    node.interpolation = 'Linear'
    return node


def multiply_color(nt, color_out, tint):
    mix = nt.nodes.new('ShaderNodeMix')
    mix.data_type, mix.blend_type = 'RGBA', 'MULTIPLY'
    mix.inputs['Factor'].default_value = 1.0
    nt.links.new(color_out, mix.inputs['A'])
    mix.inputs['B'].default_value = lt.hex_color(tint)
    return mix.outputs['Result']


def vec(nt, op, a, b):
    n = nt.nodes.new('ShaderNodeVectorMath')
    n.operation = op
    for i, value in enumerate((a, b)):
        if isinstance(value, (tuple, list)):
            n.inputs[i].default_value = value
        else:
            nt.links.new(value, n.inputs[i])
    return n.outputs['Vector']


def sheet_uv(nt, cells):
    """A card's UVs into one cell of a sprite sheet (cells across, down), picked per card at random."""
    tc = nt.nodes.new('ShaderNodeTexCoord')
    cx, cy = cells
    if cells == (1, 1):
        return tc.outputs['UV']
    info = nt.nodes.new('ShaderNodeObjectInfo')
    idx = mnode(nt, 'FLOOR', mnode(nt, 'MULTIPLY', info.outputs['Random'], cx * cy * 0.999))
    comb = nt.nodes.new('ShaderNodeCombineXYZ')
    nt.links.new(mnode(nt, 'DIVIDE', mnode(nt, 'MODULO', idx, cx), cx), comb.inputs['X'])
    nt.links.new(mnode(nt, 'DIVIDE', mnode(nt, 'FLOOR', mnode(nt, 'DIVIDE', idx, cx)), cy), comb.inputs['Y'])
    return vec(nt, 'ADD', vec(nt, 'MULTIPLY', tc.outputs['UV'], (1.0 / cx, 1.0 / cy, 1.0)), comb.outputs['Vector'])


def lit_card(nt, color, alpha, out, translucent=0.45):
    """A lit dust card: diffuse plus light through it (dust lit from behind still reads), over transparent."""
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    nt.links.new(color, bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = 1.0
    bsdf.inputs['Specular IOR Level'].default_value = 0.0
    tr = nt.nodes.new('ShaderNodeBsdfTranslucent')
    nt.links.new(color, tr.inputs['Color'])
    lit = nt.nodes.new('ShaderNodeMixShader')
    lit.inputs[0].default_value = translucent
    nt.links.new(bsdf.outputs[0], lit.inputs[1])
    nt.links.new(tr.outputs[0], lit.inputs[2])
    mix = nt.nodes.new('ShaderNodeMixShader')
    nt.links.new(alpha, mix.inputs[0])
    nt.links.new(nt.nodes.new('ShaderNodeBsdfTransparent').outputs[0], mix.inputs[1])
    nt.links.new(lit.outputs[0], mix.inputs[2])
    nt.links.new(mix.outputs[0], out.inputs['Surface'])


def blended(mat):
    mat.surface_render_method = 'BLENDED'
    mat.use_backface_culling = False
    return mat


def mat_card(name, texname, cells, color, opacity=1.0):
    """A dust card from a painted sprite sheet, tinted, its alpha scaled by opacity."""
    if name in MATS:
        return MATS[name]
    mat, nt, out = fresh(name)
    tex = tex_node(nt, texname, 'CLIP')
    nt.links.new(sheet_uv(nt, cells), tex.inputs['Vector'])
    lit_card(nt, multiply_color(nt, tex.outputs['Color'], color),
             mnode(nt, 'MULTIPLY', tex.outputs['Alpha'], opacity, clamp=True), out)
    MATS[name] = blended(mat)
    return mat


def mat_hot(name, texname, strength, tint=0xffffff):
    """An emissive sprite (spark streak, hit flash): its painted color glowing, over transparent by its alpha."""
    if name in MATS:
        return MATS[name]
    mat, nt, out = fresh(name)
    tex = tex_node(nt, texname, 'CLIP')
    nt.links.new(nt.nodes.new('ShaderNodeTexCoord').outputs['UV'], tex.inputs['Vector'])
    em = nt.nodes.new('ShaderNodeEmission')
    nt.links.new(multiply_color(nt, tex.outputs['Color'], tint), em.inputs['Color'])
    em.inputs['Strength'].default_value = strength
    mix = nt.nodes.new('ShaderNodeMixShader')
    nt.links.new(tex.outputs['Alpha'], mix.inputs[0])
    nt.links.new(nt.nodes.new('ShaderNodeBsdfTransparent').outputs[0], mix.inputs[1])
    nt.links.new(em.outputs[0], mix.inputs[2])
    nt.links.new(mix.outputs[0], out.inputs['Surface'])
    MATS[name] = blended(mat)
    return mat


def mat_decal(name, texname, tint=0xffffff, rough=0.95, opacity=1.0):
    """A decal: the painted color and alpha on a flat quad or strip (dithered, so it sorts with the cards)."""
    if name in MATS:
        return MATS[name]
    mat, nt, out = fresh(name)
    tex = tex_node(nt, texname, 'REPEAT')
    nt.links.new(nt.nodes.new('ShaderNodeTexCoord').outputs['UV'], tex.inputs['Vector'])
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    nt.links.new(multiply_color(nt, tex.outputs['Color'], tint), bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = rough
    nt.links.new(mnode(nt, 'MULTIPLY', tex.outputs['Alpha'], opacity, clamp=True), bsdf.inputs['Alpha'])
    nt.links.new(bsdf.outputs[0], out.inputs['Surface'])
    mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = False
    MATS[name] = mat
    return mat


def mat_trail_dust(name, color, opacity):
    """The clod's flight trail: a camera-facing ribbon of dust, thick at the clod (UV x = 1), gone at its tail."""
    if name in MATS:
        return MATS[name]
    mat, nt, out = fresh(name)
    tc = nt.nodes.new('ShaderNodeTexCoord')
    sep = nt.nodes.new('ShaderNodeSeparateXYZ')
    nt.links.new(tc.outputs['UV'], sep.inputs[0])
    along = mnode(nt, 'POWER', sep.outputs['X'], 1.3)
    across = mnode(nt, 'POWER', mnode(nt, 'SUBTRACT', 1.0, mnode(nt, 'ABSOLUTE', mnode(
        nt, 'MULTIPLY_ADD', sep.outputs['Y'], 2.0, -1.0)), clamp=True), 1.1)
    mapping = nt.nodes.new('ShaderNodeMapping')
    mapping.inputs['Scale'].default_value = (9.0, 2.0, 1.0)
    nt.links.new(tc.outputs['UV'], mapping.inputs['Vector'])
    tex = nt.nodes.new('ShaderNodeTexNoise')
    tex.inputs['Scale'].default_value = 1.6
    tex.inputs['Detail'].default_value = 5.0
    nt.links.new(mapping.outputs[0], tex.inputs['Vector'])
    nz = mnode(nt, 'MULTIPLY_ADD', tex.outputs['Fac'], 2.4, -0.7, clamp=True)
    alpha = mnode(nt, 'MULTIPLY', mnode(nt, 'MULTIPLY', along, across), mnode(nt, 'MULTIPLY', nz, opacity), clamp=True)
    rgb = nt.nodes.new('ShaderNodeRGB')
    rgb.outputs[0].default_value = lt.hex_color(color)
    lit_card(nt, rgb.outputs[0], alpha, out)
    MATS[name] = blended(mat)
    return mat


def mat_backdrop():
    mat, nt, out = fresh('BW_Backdrop')
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = (0.0, 0.0, 0.0, 1.0)
    bsdf.inputs['Roughness'].default_value = 1.0
    bsdf.inputs['Specular IOR Level'].default_value = 0.0
    nt.links.new(bsdf.outputs[0], out.inputs['Surface'])
    return mat


def grave_earth():
    """Freshly dug grave earth: darker and richer than the sun-baked floor."""
    return MATS['GMF_Dirt']


def granite():
    return textured('RockGranite', 'GMF_Granite', tint=0xb8b4ac)


def bark():
    return textured('BarkOak', 'GMF_Roots', tint=0x9a8670)


def mud():
    return textured('GroundDirt', 'GMF_Mud', tint=0xa69a8a)


def puff(opacity=1.0, color=EARTH_DUST):
    return mat_card(f'BW_Puff_{color:06x}_{int(opacity * 100)}', 'DustPuffs', (2, 2), color, opacity)


def jet(opacity=1.0):
    return mat_card(f'BW_Jet_{int(opacity * 100)}', 'DustJets', (2, 1), EARTH_DUST, opacity)


def floor_mat():
    """The Sink's floor: the heave is the floor itself rising, so it wears the floor's material."""
    return bpy.data.materials['_SinkFloor']


def berm_earth():
    return textured('GroundDirt', 'BW_Berm', tint=0xb3a08a)


def hole_mat():
    return mat_surface('BW_Hole', 0x0c0907, rough=1.0)


def mat_mound():
    """Grave earth with the mound's crack overlay mapped from above (its 'Top' UVs)."""
    if 'BW_Mound' in MATS:
        return MATS['BW_Mound']
    mat = lt.material('GroundDirt', name='BW_Mound', tint=0x8d7a66)
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
    linked = bsdf.inputs['Base Color'].links
    if linked:
        src = linked[0].from_socket
    else:
        rgb = nt.nodes.new('ShaderNodeRGB')
        rgb.outputs[0].default_value = bsdf.inputs['Base Color'].default_value
        src = rgb.outputs[0]
    uvn = nt.nodes.new('ShaderNodeUVMap')
    uvn.uv_map = 'Top'
    tex = tex_node(nt, 'MoundCrack', 'CLIP')
    nt.links.new(uvn.outputs['UV'], tex.inputs['Vector'])
    mix = nt.nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    nt.links.new(tex.outputs['Alpha'], mix.inputs['Factor'])
    nt.links.new(src, mix.inputs['A'])
    nt.links.new(tex.outputs['Color'], mix.inputs['B'])
    nt.links.new(mix.outputs['Result'], bsdf.inputs['Base Color'])
    MATS['BW_Mound'] = mat
    return mat


# --- Geometry helpers (world metres) ---

def V(p, z=None):
    v = Vector((p[0], p[1], p[2] if len(p) > 2 else 0.0))
    if z is not None:
        v.z = z
    return v


def flat_dir(a, b):
    return Vector((b[0] - a[0], b[1] - a[1], 0.0)).normalized()


def left_of(d):
    return Vector((-d.y, d.x, 0.0))


def turned(d, deg):
    return Matrix.Rotation(math.radians(deg), 3, 'Z') @ Vector(d)


def quad(name, center, size, mat, fwd=(1.0, 0.0, 0.0), normal=(0.0, 0.0, 1.0), shadow=False):
    """A flat quad (a decal), size (along fwd, across) in metres; UV x along fwd."""
    n = Vector(normal).normalized()
    f = Vector(fwd)
    f = (f - n * f.dot(n)).normalized()
    s = n.cross(f)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    vs = [bm.verts.new(Vector(center) + f * x * size[0] * 0.5 + s * y * size[1] * 0.5)
          for x, y in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    face = bm.faces.new(vs)
    for loop, c in zip(face.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
        loop[uv].uv = c
    return finish_bm(name, bm, mat, True, shadow)


def decal(name, at, diameter, texname, turn=0.0, opacity=1.0, z=0.012, aspect=1.0):
    mat = mat_decal(f'BW_Decal_{texname}_{int(opacity * 100)}', texname, opacity=opacity)
    return quad(name, V(at, z), (diameter, diameter * aspect), mat, fwd=(math.cos(turn), math.sin(turn), 0.0))


def puffs(name, at, radius, count, z, size, mat, seed, ring=False):
    """Dust cards round at: count of them within radius (on a ring at radius if ring), heights z (min, max)."""
    rng = random.Random(seed)
    for i in range(count):
        a = rng.uniform(0, 2 * math.pi) if not ring else 2 * math.pi * i / count + rng.uniform(-0.2, 0.2)
        r = radius * (math.sqrt(rng.random()) if not ring else rng.uniform(0.85, 1.1))
        p = V(at) + Vector((math.cos(a) * r, math.sin(a) * r, rng.uniform(*z)))
        card(f'{name}{i}', p, size * rng.uniform(0.7, 1.3), mat)


def lumps(name, at, count, radius, size, mat, seed, z=(0.0, 0.0), ring=False, flat=0.7, box='GroundDirt'):
    """Loose chunks of earth (or stone) round at."""
    rng = random.Random(seed)
    specs = []
    for k in range(count):
        a = rng.uniform(0, 2 * math.pi)
        d = radius * (rng.uniform(0.8, 1.15) if ring else math.sqrt(rng.random()))
        r = size * rng.uniform(0.55, 1.3)
        p = V(at) + Vector((math.cos(a) * d, math.sin(a) * d, rng.uniform(*z) + r * flat * 0.5))
        R = Euler((rng.uniform(-0.8, 0.8), rng.uniform(-0.8, 0.8), rng.uniform(0, 6.3))).to_matrix()
        specs.append((p, (r * 1.2, r, r * flat), R, rng.randint(0, 999), 0.3))
    return blobs(name, specs, mat, box=box, subdiv=1, smooth=False)


def chunks_at(name, pts, size, mat, seed, flat=0.7, box='GroundDirt'):
    rng = random.Random(seed)
    specs = []
    for p in pts:
        r = size * rng.uniform(0.55, 1.3)
        R = Euler((rng.uniform(-0.8, 0.8), rng.uniform(-0.8, 0.8), rng.uniform(0, 6.3))).to_matrix()
        specs.append((Vector(p), (r * 1.2, r, r * flat), R, rng.randint(0, 999), 0.3))
    return blobs(name, specs, mat, box=box, subdiv=1, smooth=False) if specs else None


def thrown(name, at, t, count, seed, speed=(2.5, 6.0), up=(2.5, 6.0), size=0.09, mat=None, heading=None,
           spread=math.pi, start_r=0.2):
    """Chunks thrown out of at, where they are t seconds later: ballistic, resting on the floor once down."""
    rng = random.Random(seed)
    pts = []
    for _ in range(count):
        a = (heading if heading is not None else 0.0) + rng.uniform(-spread, spread)
        vh, vz = rng.uniform(*speed), rng.uniform(*up)
        tt = t * rng.uniform(0.8, 1.15)
        down = vz / 4.9
        if tt >= down:
            d, z = vh * down + 0.3 * vh * min(tt - down, 0.3), 0.0
        else:
            d, z = vh * tt, vz * tt - 4.9 * tt * tt
        pts.append(V(at) + Vector((math.cos(a), math.sin(a), 0.0)) * (d + start_r) + Vector((0.0, 0.0, z + size * 0.5)))
    return chunks_at(name, pts, size, mat or grave_earth(), seed)


def falling(name, top, count, height, spread, size, seed, mat=None):
    """Crumbs dropping from top: a loose column of them below it."""
    rng = random.Random(seed)
    pts = [V(top) + Vector((rng.uniform(-spread, spread), rng.uniform(-spread, spread), -rng.uniform(0.1, height)))
           for _ in range(count)]
    return chunks_at(name, [p for p in pts if p.z > 0.05], size, mat or grave_earth(), seed)


def shards(name, pts, size, seed, mat=None):
    """Angular flakes of headstone granite."""
    rng = random.Random(seed)
    specs = []
    for p in pts:
        r = size * rng.uniform(0.6, 1.3)
        R = Euler((rng.uniform(0, 3), rng.uniform(0, 3), rng.uniform(0, 6.3))).to_matrix()
        specs.append((Vector(p), (r * 1.3, r * 0.75, r * 0.3), R, rng.randint(0, 999), 0.25))
    return blobs(name, specs, mat or granite(), box='RockGranite', subdiv=1, smooth=False)


def top_uv(o, center, radius):
    """A second UV map, 'Top', projecting the mesh from above onto a square 2 x radius round center."""
    me = o.data
    layer = me.uv_layers.new(name='Top')
    mw = o.matrix_world
    for loop in me.loops:
        co = mw @ me.vertices[loop.vertex_index].co
        layer.data[loop.index].uv = ((co.x - center.x) / (2 * radius) + 0.5, (co.y - center.y) / (2 * radius) + 0.5)
    me.uv_layers['UVMap'].active_render = True
    me.uv_layers.active = me.uv_layers['UVMap']


def swept(name, pts, half_w, zf, mat, seed, cols=13):
    """A mound of earth swept along pts: zf(k, u) its height at point k, u from -1 to 1 across half_w * 1.15."""
    bm = bmesh.new()
    n = len(pts)
    grid_v = []
    for k in range(n):
        t = pts[min(k + 1, n - 1)] - pts[max(k - 1, 0)]
        side = Vector((-t.y, t.x, 0.0)).normalized()
        row = []
        for j in range(cols):
            u = -1.0 + 2.0 * j / (cols - 1)
            row.append(bm.verts.new(V(pts[k], 0.0) + side * u * half_w * 1.15 + Vector((0.0, 0.0, zf(k, u) - 0.03))))
        grid_v.append(row)
    for k in range(n - 1):
        for j in range(cols - 1):
            bm.faces.new((grid_v[k][j + 1], grid_v[k + 1][j + 1], grid_v[k + 1][j], grid_v[k][j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    o = finish_bm(name, bm, mat)
    bpy.context.view_layer.update()
    lt.box_uv(o, 'GroundDirt', seed=seed)
    return o


def strip_on(name, pts, half_w, zf, width, mat, u0, tile, lift=0.014, cols=7):
    """A decal strip lying on a swept mound (or the floor: zf 0): UV x in tiles along it from u0, y across."""
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    n = len(pts)
    rows, us, run = [], [], 0.0
    for k in range(n):
        if k:
            run += (V(pts[k], 0.0) - V(pts[k - 1], 0.0)).length
        t = pts[min(k + 1, n - 1)] - pts[max(k - 1, 0)]
        side = Vector((-t.y, t.x, 0.0)).normalized()
        row = []
        for j in range(cols):
            s = (-0.5 + j / (cols - 1)) * width
            u = s / (half_w * 1.15) if half_w else 0.0
            row.append(bm.verts.new(V(pts[k], 0.0) + side * s + Vector((0.0, 0.0, zf(k, u) - 0.03 + lift))))
        rows.append(row)
        us.append(u0 + run / tile)
    for k in range(n - 1):
        for j in range(cols - 1):
            f = bm.faces.new((rows[k][j], rows[k + 1][j], rows[k + 1][j + 1], rows[k][j + 1]))
            for loop, (a, b) in zip(f.loops, ((k, j), (k + 1, j), (k + 1, j + 1), (k, j + 1))):
                loop[uv].uv = (us[a], b / (cols - 1))
    return finish_bm(name, bm, mat, True, False)


def resample(pts, step):
    """Points every step metres along a polyline."""
    out = [V(pts[0])]
    run = 0.0
    for a, b in zip(pts[:-1], pts[1:]):
        a, b = V(a), V(b)
        seg = (b - a).length
        while run + step <= seg:
            run += step
            out.append(a.lerp(b, run / seg))
        run -= seg
    if (out[-1] - V(pts[-1])).length > step * 0.3:
        out.append(V(pts[-1]))
    return out


def crack_line(name, pts, width, zf=None, half_w=0.0, cap=True):
    """The heave crack decal along pts: whole 4:1 tiles (the tile stretched a little to fit), then the end cap. Given
    zf (a swept mound's height, indexed by pts), the strip lies on that mound; otherwise on the floor, every 25 cm."""
    if zf is None:
        pts = resample(pts, 0.25)
        zf = lambda k, u: 0.03
    pts = [V(p) for p in pts]
    n = len(pts)
    cum = [0.0]
    for k in range(n - 1):
        cum.append(cum[-1] + (V(pts[k + 1], 0.0) - V(pts[k], 0.0)).length)
    total = cum[-1]
    split = n - 1
    if cap:
        split = next((k for k in range(1, n) if cum[k] >= total - width), n - 1)
        if split >= n - 1:
            split = max(n - 2, 1)
    tiles = max(1, round(cum[split] / (width * 4.0)))
    objs = [strip_on(name, pts[:split + 1], half_w, zf, width, mat_decal('BW_HeaveCrack', 'HeaveCrack'), 0.0,
                     cum[split] / tiles)]
    if cap and split < n - 1:
        objs.append(strip_on(name + 'Cap', pts[split:], half_w, lambda k, u: zf(k + split, u), width,
                             mat_decal('BW_HeaveCrackCap', 'HeaveCrackCap'), 0.0, max(total - cum[split], 0.05)))
    return objs


# --- Effects ---

def clod_burst(name, at, t, seed, debris=True, opacity=1.0):
    """A clod's burst t seconds after it lands: thick puffs low and close, then spreading, rising and thinning;
    debris thrown out of it."""
    at = V(at)
    if t <= 0.15:
        puffs(name + 'Core', at, 0.55, 10, (0.25, 1.0), 1.0, puff(0.95 * opacity), seed)
        puffs(name + 'Ring', at, 1.0, 9, (0.1, 0.35), 0.75, puff(0.8 * opacity), seed + 1, ring=True)
    elif t <= 0.5:
        puffs(name + 'Core', at, 1.3, 13, (0.4, 1.7), 1.5, puff(0.75 * opacity), seed)
        puffs(name + 'Ring', at, 2.0, 11, (0.15, 0.5), 1.0, puff(0.6 * opacity), seed + 1, ring=True)
    else:
        puffs(name + 'Core', at, 1.9, 13, (0.6, 2.3), 2.0, puff(0.38 * opacity), seed)
        puffs(name + 'Ring', at, 2.6, 9, (0.2, 0.7), 1.4, puff(0.3 * opacity), seed + 1, ring=True)
    if debris:
        thrown(name + 'Debris', at, t, 16, seed, size=0.1)
        if t <= 0.5:
            thrown(name + 'Stones', at, t, 5, seed + 3, size=0.07, mat=granite())


def rubble(name, at, seed, scale=1.0):
    """The pile a burst clod leaves: heaped grave earth with stones, a coffin plank and a bone in it; cover for 10 s."""
    rng = random.Random(seed)
    at = V(at)
    specs = [(at + Vector((0.0, 0.0, -0.05)), (0.75 * scale, 0.65 * scale, 0.82 * scale), None, seed, 0.22)]
    for k in range(6):
        a = 2 * math.pi * k / 6 + rng.uniform(-0.3, 0.3)
        d = rng.uniform(0.45, 0.8) * scale
        rr = rng.uniform(0.25, 0.42) * scale
        specs.append((at + Vector((math.cos(a) * d, math.sin(a) * d, 0.0)), (rr, rr * 0.9, rr * 0.75), None, seed + k,
                      0.3))
    blobs(name, specs, grave_earth(), box='GroundDirt', subdiv=3)
    stones = []
    for k in range(6):
        a = rng.uniform(0, 2 * math.pi)
        d = rng.uniform(0.3, 0.8) * scale
        stones.append(at + Vector((math.cos(a) * d, math.sin(a) * d, rng.uniform(0.15, 0.55) * scale * (1.0 - d / 1.1))))
    blobs(name + 'Stones', [(p, (0.14 * scale, 0.11 * scale, 0.09 * scale), None, k, 0.3) for k, p in enumerate(stones)],
          granite(), box='RockGranite', subdiv=1, smooth=False)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.transform(bm, matrix=T(at + Vector((0.25, -0.3, 0.55)) * 1.0) @ Matrix.Rotation(0.6, 4, 'Y')
                        @ Matrix.Rotation(0.9, 4, 'Z') @ Matrix.Diagonal((1.0 * scale, 0.2 * scale, 0.04 * scale, 1.0)),
                        verts=bm.verts)
    o = finish_bm(name + 'Plank', bm, textured('WoodPlanks', 'GMF_Coffin', tint=0x8a7a68), smooth=False)
    bpy.context.view_layer.update()
    lt.box_uv(o, 'WoodPlanks')
    tube(name + 'Bone', [at + Vector((-0.5, 0.35, 0.3)) * 1.0, at + Vector((-0.15, 0.55, 0.45))],
         [0.045 * scale, 0.035 * scale], mat_surface('GMF_BoneChip', 0xd9d1bf, rough=0.6), sides=8)
    lumps(name + 'Loose', at, 12, 1.25 * scale, 0.08, grave_earth(), seed + 9, ring=True)


def berm_segment(name, origin, d, seed, length=2.0, width=1.4, height=0.78, slump=0.0, inner=-1.0):
    """One berm segment: earth thrown up beside the furrow, steeper on the furrow's side (inner: -1 its right, 1 its
    left), ends tapered so the next one overlaps it. slump 0..1: crumbling, lower and spread, cracked."""
    d = Vector(d).normalized()
    side = left_of(d)
    rng = random.Random(seed)
    height *= (1.0 - 0.55 * slump) * rng.uniform(0.88, 1.12)
    width *= 1.0 + 0.45 * slump
    n, m = 16, 15
    bm = bmesh.new()
    rows = []
    for i in range(n):
        s = i / (n - 1)
        # Full height to near its ends, so the next segment's overlap hides the seam; the free ends round off.
        ends = 0.62 + 0.38 * lc.smoothstep(0.0, 0.12, s) * lc.smoothstep(1.0, 0.88, s)
        ends *= lc.smoothstep(0.0, 0.04, s) * lc.smoothstep(1.0, 0.96, s) * 0.6 + 0.4
        row = []
        for j in range(m):
            u = -1.0 + 2.0 * j / (m - 1)
            steep = 0.75 if u * inner > 0 else 1.3          # steeper on the furrow's side
            prof = max(0.0, 1.0 - u * u) ** steep
            nz = noise.noise(Vector((s * 2.2 + seed * 0.7, u * 2.5, 0.5)))
            fine = noise.noise(Vector((s * 9.0, u * 7.0, seed + 0.3)))
            z = height * ends * prof * (1.0 + 0.22 * nz + 0.1 * fine + 0.4 * slump * fine)
            p = V(origin, 0.0) + d * (s * length) + side * (u * width * 0.5 * (1.0 + 0.12 * nz))
            row.append(bm.verts.new(p + Vector((0.0, 0.0, z - 0.04))))
        rows.append(row)
    for i in range(n - 1):
        for j in range(m - 1):
            bm.faces.new((rows[i][j + 1], rows[i + 1][j + 1], rows[i + 1][j], rows[i][j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    o = finish_bm(name, bm, berm_earth())
    bpy.context.view_layer.update()
    lt.box_uv(o, 'GroundDirt', seed=seed)
    # Clods lying on it (thrown earth), and, crumbling, more slid to its foot.
    pts = []
    for k in range(int(16 + 14 * slump)):
        s, u = rng.uniform(0.05, 0.95), rng.uniform(-0.8, 0.8)
        lift = 1.0
        if slump and rng.random() < 0.55:
            u, lift = rng.choice((-1.0, 1.0)) * rng.uniform(0.9, 1.35), 0.0
        prof = max(0.0, 1.0 - u * u)
        pts.append(V(origin, 0.0) + d * (s * length) + side * (u * width * 0.5) +
                   Vector((0.0, 0.0, height * prof * 0.9 * lift + 0.04)))
    chunks_at(name + 'Clods', pts, 0.09, berm_earth(), seed)
    return o


def berm_line(name, start, end, offset, seed, slump=0.0, upto=1.0, rising=0.0, inner=-1.0):
    """Berm segments along start-end, offset to one side; upto: how far along they are built; rising: the last few
    are still coming up."""
    d = flat_dir(start, end)
    side = left_of(d)
    total = (V(end) - V(start)).length * upto
    step = 1.0                  # each segment half overlaps the next: one ragged ridge, no rhythm of loaves
    count = max(int(round((total - 1.0) / step)), 1)
    for i in range(count):
        rng = random.Random(seed + i)
        a = V(start) + d * (i * step) + side * (offset + rng.uniform(-0.12, 0.12))
        h = 0.78
        if rising and i >= count - 2:
            h *= 0.45 + 0.3 * (count - 1 - i)
        berm_segment(f'{name}{i}', a, turned(d, rng.uniform(-5, 5)), seed + i, height=h, slump=slump, inner=inner)


def mound(name, at, bulge, seed, pebbles=True):
    """A spawn mound bulging (bulge 0..1 through its 1 s): a dome of grave earth, its top cracking open."""
    at = V(at, 0.0)
    r, h = 0.62 + 0.18 * bulge, 0.1 + 0.36 * bulge
    o = blobs(name, [(at + Vector((0.0, 0.0, -0.04)), (r, r * 0.93, h), None, seed, 0.24)], mat_mound(),
              box='GroundDirt', subdiv=4)
    top_uv(o, at, r * 1.05)
    lumps(name + 'Skirt', at, 12, r * 1.05, 0.06, grave_earth(), seed, ring=True)
    if bulge > 0.5:
        # Its crust breaking into plates as it rises.
        rng = random.Random(seed + 3)
        plates = []
        for _ in range(9):
            a, d = rng.uniform(0, 2 * math.pi), r * rng.uniform(0.25, 0.75)
            plates.append(at + Vector((math.cos(a) * d, math.sin(a) * d, h * math.sqrt(max(1.0 - (d / r) ** 2, 0.0)) - 0.03)))
        shards(name + 'Crust', plates, 0.1, seed, mat=grave_earth())
    if pebbles and bulge > 0.2:
        lumps(name + 'Jump', at, 6, r * 0.35, 0.035, grave_earth(), seed + 5, z=(h + 0.02, h + 0.25 * bulge))
    return at + Vector((0.0, 0.0, h))


def crater(name, at, seed, size=1.0, hole=True, collapsed=False):
    """A burst spawn mound: a ring of broken clods round a dark hole (or, shot while bulging, fallen in on itself)."""
    at = V(at, 0.0)
    rng = random.Random(seed)
    decal(name + 'Scar', at, 2.3 * size, 'ImpactScar', turn=seed)
    # A low rim of thrown-up earth, then broken clods of the mound's crust tipped on it.
    rim = []
    for k in range(14):
        a = 2 * math.pi * k / 14 + rng.uniform(-0.15, 0.15)
        d = rng.uniform(0.55, 0.68) * size
        rr = rng.uniform(0.2, 0.3) * size
        rim.append((at + Vector((math.cos(a) * d, math.sin(a) * d, -0.06)), (rr * 1.4, rr, rr * 0.45),
                    Matrix.Rotation(a, 3, 'Z'), seed + k, 0.35))
    if collapsed:
        rim.append((at + Vector((0.0, 0.0, -0.03)), (0.55 * size, 0.5 * size, 0.14 * size), None, seed, 0.35))
    blobs(name + 'Rim', rim, grave_earth(), box='GroundDirt', subdiv=3)
    plates = []
    for k in range(10):
        a = rng.uniform(0, 2 * math.pi)
        d = rng.uniform(0.45, 0.75) * size
        plates.append(at + Vector((math.cos(a) * d, math.sin(a) * d, rng.uniform(0.08, 0.16) * size)))
    shards(name, plates, 0.13 * size * (0.7 if collapsed else 1.0), seed, mat=mat_mound())
    if hole and not collapsed:
        fan(name + 'Hole', at, splat_radius(0.36 * size, seed, 6, 0.4), hole_mat(), z=0.02)
    lumps(name + 'Loose', at, 12, 1.3 * size, 0.06, grave_earth(), seed + 7, ring=True)


def furrow_strip(name, pts):
    """The ploughed furrow: the churned-earth decal, 190 cm wide, tiling every 380 cm along the line."""
    pts = resample(pts, 0.3)
    return strip_on(name, pts, 0.0, lambda k, u: 0.03, 1.9, mat_decal('BW_Furrow', 'Furrow'), 0.0, 3.8, lift=0.016)


def camera_ribbon(name, pts, widths, mat, cam_loc):
    n = len(pts)
    across = []
    for k in range(n):
        t = (pts[min(k + 1, n - 1)] - pts[max(k - 1, 0)]).normalized()
        across.append(t.cross(Vector(cam_loc) - pts[k]).normalized())
    return ribbon(name, pts, widths, mat, across=across, shadow=False)


def tracer(name, p0, p1, flash=1.0):
    """A shot: the game draws its own tracer; this one only says where the shot went, with a hit flash."""
    tube(name, [p0, p0.lerp(p1, 0.7), p1], [0.006, 0.012, 0.02], mat_trail('BW_Tracer', 0xfff0c8, 9.0, power=2.0),
         sides=8, shadow=False)
    if flash:
        card(name + 'Flash', p1, flash, mat_hot('BW_Flash', 'Flash', 5.0), roll_angle=0.3)


def spark_burst(name, at, normal, cam_loc, seed, scale=1.0, count=12, dust=True, chips=True):
    """A shot glancing off the headstone: streaks fanning off the face, a hit flash, stone dust and flakes."""
    rng = random.Random(seed)
    n = Vector(normal).normalized()
    at = Vector(at)
    view = (Vector(cam_loc) - at).normalized()
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    for _ in range(count):
        d = (n + Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.3, 1.0))) * 0.95).normalized()
        start = at + d * rng.uniform(0.02, 0.2) * scale
        end = start + d * rng.uniform(0.2, 0.55) * scale - Vector((0.0, 0.0, 0.04 * scale))
        across = (end - start).cross(view).normalized()
        w0, w1 = 0.006 * scale, 0.024 * scale
        vs = [bm.verts.new(start - across * w0), bm.verts.new(end - across * w1), bm.verts.new(end + across * w1),
              bm.verts.new(start + across * w0)]
        f = bm.faces.new(vs)
        for loop, c in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
            loop[uv].uv = c
    finish_bm(name, bm, mat_hot('BW_Spark', 'SparkStreak', 7.0), True, False)
    card(name + 'Flash', at + n * 0.03, 0.55 * scale, mat_hot('BW_Flash', 'Flash', 5.0), roll_angle=0.2)
    if dust:
        puffs(name + 'Dust', at + n * 0.15 * scale, 0.12 * scale, 4, (-0.05, 0.1), 0.32 * scale,
              puff(0.7, STONE_DUST), seed)
    if chips:
        shards(name + 'Chips', [at + (n + Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0, 1)))).normalized()
                                * rng.uniform(0.15, 0.45) * scale for _ in range(5)], 0.03 * scale, seed)


def star3d(m, c, R, seed):
    rng = random.Random(seed)
    rot = Euler((rng.uniform(0, 3), rng.uniform(0, 3), rng.uniform(0, 3))).to_matrix()
    lc.ellipsoid(m, c, (R * 0.3, R * 0.3, R * 0.3), 'fx', segs=10, rings=6)
    for ax in ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)):
        d = rot @ Vector(ax)
        lc.tube(m, [c + d * R * 0.15, c + d * R], [R * 0.2, R * 0.015], 'fx', sides=6)


def daze(name, center, radius, phase, scale=1.0):
    """The stun cue: four little stars wheeling on a tilted ring over her head, each trailing a fading streak."""
    star = mat_emit('BW_Star', 0xffe7a0, 3.5)
    trail = mat_trail('BW_StarTrail', 0xffdf8a, 2.6, power=1.4)
    tilt = Matrix.Rotation(0.32, 3, 'X') @ Matrix.Rotation(0.18, 3, 'Y')
    m = wm()
    for k in range(4):
        a = phase + 2 * math.pi * k / 4
        p = Vector(center) + tilt @ Vector((math.cos(a) * radius, math.sin(a) * radius, 0.0))
        star3d(m, p, 0.11 * scale, k)
        card(f'{name}Glow{k}', p, 0.42 * scale, mat_hot('BW_StarGlow', 'Flash', 1.6, tint=0xffe7a0), roll_angle=0.4 * k)
        pts = [Vector(center) + tilt @ Vector((math.cos(a - 1.0 + j / 10), math.sin(a - 1.0 + j / 10), 0.0)) * radius
               for j in range(11)]
        tube(f'{name}Trail{k}', pts, [0.004 + 0.022 * scale * (j / 10) ** 1.5 for j in range(11)], trail, sides=6,
             shadow=False)
    done(m, name, star, shadow=False)


def dirt_on(rec, seed, amount=1.0):
    """Grave earth heaped on her after a clod bursts on her: on her head, her carapace and round the stone's foot."""
    rng = random.Random(seed)

    def heap(bone, pts, radii):
        m = wm()
        m.bm.free()
        bm = bmesh.new()
        for i, p in enumerate(pts):
            made = bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
            r = [x * rng.uniform(0.7, 1.2) for x in radii]
            for v in made['verts']:
                d = v.co.copy()
                k = 1.0 + 0.4 * fbm(d * 1.7 + Vector((i, seed, 0.0)), 2)
                v.co = Vector(p) + Vector((d.x * r[0], d.y * r[1], d.z * r[2])) * k
        m.bm = bm
        attach(rec, bone, m, f'Dirt{bone}', grave_earth(), box='GroundDirt')
    hc = HEAD_C
    heap('head', [hc + Vector((rng.uniform(-10, 6), rng.uniform(-10, 10), 15.0)) for _ in range(int(3 * amount))],
         (12.0, 10.0, 7.0))
    heap('body', [Vector((rng.uniform(-25, 15), rng.uniform(-22, 22), RIDE + 20.0)) for _ in range(int(5 * amount))],
         (16.0, 13.0, 8.0))
    heap('abdomen', [ab_point(rng.uniform(-0.2, 0.55), rng.uniform(-60, 60), 0.98)[0] for _ in range(int(6 * amount))],
         (18.0, 15.0, 10.0))


def ling_mud(rec, seed):
    """Grave mud on a spiderling digging out: clumps on its abdomen and carapace, crusts on its lower legs."""
    rng = random.Random(seed)

    def lumps_on(bone, items, name):
        m = wm()
        m.bm.free()
        bm = bmesh.new()
        for i, (c, radii, R) in enumerate(items):
            made = bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
            for v in made['verts']:
                d = v.co.copy()
                k = 1.0 + 0.4 * fbm(d * 1.6 + Vector((seed, i, 0.0)), 2)
                v.co = Vector(c) + R @ Vector((d.x * radii[0], d.y * radii[1], d.z * radii[2])) * k
        m.bm = bm
        attach(rec, bone, m, name, mud(), box='GroundDirt')
    items = []
    for _ in range(9):
        p, n = ab_point(rng.uniform(-0.6, 0.5), rng.uniform(-75, 75), 0.97)
        items.append((p, (12.0, 24.0, 18.0), basis(n, (1.0, 0.0, 0.0))))
    lumps_on('abdomen', items, 'LingMudA')
    lumps_on('body', [(Vector((rng.uniform(-28, 18), rng.uniform(-20, 20), RIDE + 16.0)), (16.0, 13.0, 8.0),
                       Matrix.Identity(3)) for _ in range(4)], 'LingMudB')
    for pair, s, side in FC['LEGS']:
        leg = f'{pair}_{s}'
        K0, F0 = FC['BONES'][f'tibia_{leg}']
        d = (F0 - K0).normalized()
        lumps_on(f'tibia_{leg}', [(K0.lerp(F0, t), (22.0, 10.0, 10.0), basis(d)) for t in (0.62, 0.8, 0.92)],
                 f'LingMud{leg}')


# --- Her poses for the moves (Pose: degrees and centimetres in creature space) ---

Pose = FC['Pose']
STANDT = Pose(head=4, fangs=12, palps=6)
LIFTING = Pose(pitch=12, lift=10, shift=-8, abd=14, head=6, fangs=16, palps=4,
               reach={'0_l': (115, -2, 75), '0_r': (115, -2, 75)}, steps={'1_l': (20, 8, 0), '1_r': (20, 8, 0)})
OVERHEAD = CLOD
THROW = Pose(pitch=-8, lift=2, shift=12, abd=6, head=-6, fangs=26, palps=10,
             reach={'0_l': (165, 30, 60), '0_r': (165, 30, 60)},
             steps={'1_l': (40, 5, 0), '1_r': (40, 5, 0), '3_l': (-35, 0, 0), '3_r': (-35, 0, 0)})
RECOIL = Pose(pitch=16, lift=4, shift=-22, abd=8, head=22, fangs=34, palps=20, rollz=-5,
              reach={'0_l': (55, 95, 115), '0_r': (55, 95, 115)})
STUN = Pose(pitch=-8, lift=-40, spread=1.18, head=-24, abd=-10, fangs=4, palps=-25,
            steps={'0_l': (25, 25, 0), '0_r': (25, 25, 0)})
RISE = Pose(pitch=6, lift=-16, spread=1.08, head=-6, abd=4, fangs=10, rollz=4)
PLOUGH = Pose(pitch=-14, lift=-22, abd=16, head=-16, fangs=26, palps=-25, steps=dict(CHARGE.steps))
SLAM = Pose(pitch=-12, lift=-4, shift=16, abd=12, head=-12, fangs=28, palps=8,
            steps={'0_l': (105, 25, 0), '0_r': (105, 25, 0), '1_l': (45, 15, 0), '1_r': (45, 15, 0),
                   '3_l': (-20, 0, 0), '3_r': (-20, 0, 0)})
FLINCH = Pose(pitch=10, lift=8, abd=-12, head=14, fangs=30, palps=14, rollz=-6,
              reach={'0_l': (90, 80, 70), '0_r': (90, 80, 70)})
SLUMP = Pose(pitch=-4, lift=-10, abd=-14, head=-4, fangs=18, rollz=5)
RUN2 = Pose(pitch=-8, lift=-12, abd=10, head=-8, fangs=18, palps=-10,
            steps={'0_l': (-10, 0, 30), '0_r': (60, 5, 0), '1_l': (45, 0, 25), '1_r': (-35, 0, 0),
                   '2_l': (-30, 0, 0), '2_r': (35, 0, 22), '3_l': (25, 0, 30), '3_r': (-50, 0, 0)})
CALL = Pose(pitch=8, lift=6, abd=16, head=10, fangs=34, palps=16, reach={'0_l': (95, 80, 60), '0_r': (95, 80, 60)})
BARE_RISE = Pose(pitch=14, lift=10, abd=10, head=12, fangs=30, palps=12, reach={'0_l': (100, 85, 80), '0_r': (100, 85, 80)},
                 steps={'3_l': (-15, 0, 0), '3_r': (-15, 0, 0)})
EMERGE_IN = Pose(pitch=48, lift=-8, head=10, fangs=26, palps=20,
                 reach={'0_l': (20, 25, 120), '0_r': (20, 25, 120), '1_l': (10, 50, 80), '1_r': (10, 50, 80)})
EMERGE_HALF = Pose(pitch=26, head=6, fangs=20, palps=10,
                   reach={'0_l': (85, 45, -40), '0_r': (85, 45, -40), '1_l': (40, 75, -30), '1_r': (40, 75, -30)})


def pose_feet(pose, at, toward, scale=SCALE):
    M, feet = FC['pose_matrices'](pose)
    W = Matrix.LocRotScale(V(at, 0.0), Euler((0.0, 0.0, G['heading_to'](at, toward))), Vector((scale,) * 3))
    return {leg: W @ (TOB @ f) for leg, f in feet.items()}


def held_point(pose, at, toward):
    f = pose_feet(pose, at, toward)
    return (f['0_l'] + f['0_r']) * 0.5 + Vector((0.0, 0.0, 0.25))


def her(pose, at, toward, phase=1, stone=True):
    """She, dressed as D: phase 3 bares her pale abdomen. stone False keeps her phase-1 hide with no headstone (the
    frames where it is breaking off: the pieces are built apart)."""
    if phase == 1 and not stone:
        rec = spider('GM_D', pose, at, toward, SCALE, gm_hide('D'))
        barrow(rec, stone=False)
        return rec
    return gravemother('D', pose, at, toward, phase=phase)


def head_pt(rec):
    return cpt(rec, 'head', HEAD_C + Vector((16.0, 0.0, 10.0)))


def rear_pt(rec):
    return cpt(rec, 'abdomen', ABD_C + Vector((-50.0, 0.0, 6.0)))


def stone_pt(rec, z=62.0):
    base, R = stone_frame()
    return cpt(rec, 'abdomen', base + R @ Vector((0.0, STONE_T / 2 + 1.0, z)))


def stone_normal(rec):
    base, R = stone_frame()
    return cdir(rec, 'abdomen', R @ Vector((0.0, 1.0, 0.0)))


def bare_pt(rec):
    return cpt(rec, 'abdomen', ABD_C + Vector((-10.0, 0.0, 36.0)))


def stone_cracks(rec, more=True):
    """Fresh cracks racing across the headstone's face at 33%."""
    base, R = stone_frame()
    face = base + R @ Vector((0.0, STONE_T / 2 + 0.4, 0.0))
    g = wm()
    lines = [((-40, 96), (-22, 84), (-15, 70), (2, 62), (9, 46), (27, 36), (40, 26)),
             ((2, 62), (14, 74), (22, 92), (30, 104)), ((-15, 70), (-30, 58), (-38, 40)),
             ((9, 46), (0, 30), (-6, 12), (-12, 0)), ((27, 36), (36, 52), (39, 66))]
    for i, line in enumerate(lines if more else lines[:1]):
        pts = [face + R @ Vector((x, 0.0, z)) for x, z in line]
        lc.tube(g, pts, [2.2 if i == 0 else 1.3] * len(pts), 'fx', sides=5)
    attach(rec, 'abdomen', g, 'Cracks', mat_surface('BW_CrackDark', 0x15110d, rough=0.9))


def stone_piece(rec, name, which, move=None, ground=None):
    """Half the headstone, split along its crack: on her back (move: a world (offset, axis, degrees) applied about
    its middle), or lying on the floor (ground: (point, yaw degrees, tilt degrees))."""
    m = wm()
    headstone_mesh(m, Vector((0.0, 0.0, 0.0)), STONE_W, STONE_H, STONE_T, Matrix.Identity(3))
    bm = m.bm
    n = Vector((0.66, 0.0, 0.75)).normalized()
    geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
    bmesh.ops.bisect_plane(bm, geom=geom, plane_co=(0.0, 0.0, 60.0), plane_no=n, clear_outer=which == 'low',
                           clear_inner=which == 'high')
    bmesh.ops.holes_fill(bm, edges=[e for e in bm.edges if e.is_boundary])
    if ground is None:
        base, R = stone_frame()
        bmesh.ops.transform(bm, matrix=T(base) @ R.to_4x4(), verts=bm.verts)
        bmesh.ops.transform(bm, matrix=rec['W'] @ TOB @ rec['M']['abdomen'], verts=bm.verts)
        if move:
            c = sum((v.co for v in bm.verts), Vector()) / len(bm.verts)
            off, axis, deg = move
            bmesh.ops.transform(bm, matrix=T(c + Vector(off)) @ Matrix.Rotation(math.radians(deg), 4, Vector(axis))
                                @ T(-c), verts=bm.verts)
    else:
        at, yaw_deg, tilt = ground
        c = sum((v.co for v in bm.verts), Vector()) / len(bm.verts)
        bmesh.ops.transform(bm, matrix=T(V(at, STONE_T * 0.5 * 0.018 + 0.02)) @ Matrix.Rotation(math.radians(yaw_deg), 4, 'Z')
                            @ Matrix.Rotation(math.radians(90.0 + tilt), 4, 'X') @ Matrix.Scale(0.01 * SCALE, 4) @ T(-c),
                            verts=bm.verts)
    return done(m, name, granite(), smooth=False, box='RockGranite')


def root_stubs(rec, seed):
    """The roots that bound the stone, snapped: splintered ends sticking up off her abdomen."""
    rng = random.Random(seed)
    base, R = stone_frame()
    m = wm()
    for k in range(6):
        sgn = 1 if k % 2 else -1
        p0, n = ab_point(rng.uniform(-0.3, 0.2), sgn * rng.uniform(20, 55), 1.02)
        tip = p0 + n * rng.uniform(18, 32) + Vector((rng.uniform(-8, 8), rng.uniform(-8, 8), rng.uniform(4, 14)))
        lc.tube(m, [p0, p0.lerp(tip, 0.6), tip], [3.0, 2.2, 0.5], 'fx', sides=6)
    attach(rec, 'abdomen', m, 'Stubs', bark(), box='BarkOak')


# --- Render ---

SUN = RIM = BACKDROP = None
DATA = {}


def project(cam, marks, size):
    out = []
    for kind, label, p in marks:
        c = world_to_camera_view(SCENE, cam, Vector(p))
        out.append(dict(kind=kind, label=label, x=round(c.x * size[0], 1), y=round((1.0 - c.y) * size[1], 1),
                        depth=round(c.z, 2)))
    return out


def alpha_png(src, dst):
    im = bpy.data.images.load(src, check_existing=False)
    w, h = im.size
    px = np.empty(w * h * 4, np.float32)
    im.pixels.foreach_get(px)
    lt.write_png(dst, lt.to8(px.reshape(h, w, 4)[::-1, :, 3]))
    bpy.data.images.remove(im)


def shoot(key, cam_loc, target, lens, size, marks=(), samples=48, ortho=None, alpha=False, exposure=0.0, rot=None):
    cam = G['camera'](cam_loc, target, lens)
    if rot is not None:
        cam.rotation_euler = rot                    # straight down: the frame's up set by hand
    cam.data.type = 'ORTHO' if ortho else 'PERSP'
    if ortho:
        cam.data.ortho_scale = ortho
    face_cards(cam)
    G['setup_render'](8 if QUICK else samples)
    SCENE.use_nodes = False                         # no compositor: no bloom (the game has its own)
    SCENE.view_settings.exposure = exposure
    SCENE.render.resolution_x, SCENE.render.resolution_y = size
    SCENE.render.resolution_percentage = 50 if QUICK else 100
    path = os.path.join(OUT, f'BW_{key}.png')
    SCENE.render.filepath = path
    t0 = time.time()
    bpy.ops.render.render(write_still=True)
    entry = dict(file=os.path.basename(path), w=size[0], h=size[1], marks=project(cam, marks, size))
    if ortho:
        entry['px_per_m'] = round(size[0] / ortho, 3)
    if alpha:
        BACKDROP.hide_render = True
        SCENE.render.film_transparent = True
        SCENE.render.image_settings.color_mode = 'RGBA'
        SCENE.eevee.taa_render_samples = 8 if QUICK else 16
        rgba = path[:-4] + '_RGBA.png'
        SCENE.render.filepath = rgba
        bpy.ops.render.render(write_still=True)
        alpha_png(rgba, path[:-4] + '_A.png')
        os.remove(rgba)
        entry['alpha'] = os.path.basename(path[:-4] + '_A.png')
        BACKDROP.hide_render = False
    log(f'rendered {key} in {time.time() - t0:.1f} s')
    return entry


def light():
    FC['light']('golden', SUN, RIM)


def frame_seed(key):
    random.seed(zlib.crc32(key.encode()))


# --- The moves. The Sink: the den's mouth at x 8 facing west, the floor west of it; sun from the west-south-west. ---

CLOD_AT, PLAYER = (5.6, 0.6), (-14.3, -1.4)        # 20 m apart
# From behind the player, as the fight is seen: the player in the foreground, she at 20 m.
CLOD_CAM = ((-26.06, -4.39, 4.2), (5.13, 0.55, -0.97), 44.0)
APEX = 2.2                                       # the lob's rise over a straight line, metres
DODGE = 2.6                                      # how far the player has stepped aside when it lands


def clod_geometry():
    fwd = flat_dir(CLOD_AT, PLAYER)
    side = left_of(fwd)
    launch = held_point(OVERHEAD, CLOD_AT, PLAYER)
    land = V(PLAYER, 0.45)
    dig = (lambda f: V((f['0_l'] + f['0_r']) * 0.5, 0.0))(pose_feet(DIG, CLOD_AT, PLAYER))

    def arc_pt(s):
        p = launch.lerp(land, s)
        p.z += 4.0 * APEX * s * (1.0 - s)
        return p
    return fwd, side, launch, land, dig, arc_pt


def dig_spot(name, at, seed, size=1.0):
    """Where her front legs tore in: an impact scar, loosened clods and a low puff."""
    decal(name + 'Scar', at, 1.6 * size, 'ImpactScar', turn=seed)
    lumps(name + 'Clods', at, 10, 0.7 * size, 0.09, grave_earth(), seed, z=(0.0, 0.08))
    puffs(name + 'Puff', at, 0.6 * size, 4, (0.15, 0.5), 0.8, puff(0.75), seed)


def strip_clod(i):
    fwd, side, launch, land, dig, arc_pt = clod_geometry()
    P = V(PLAYER, 0.0)
    dodged = P + side * DODGE
    marks = []
    if i == 0:
        rec = her(DIG, CLOD_AT, PLAYER)
        for k, s in enumerate(('0_l', '0_r')):
            f = foot(rec, s)
            dig_spot(f'Dig{k}', V(f, 0.0), 3 + k)
            thrown(f'Kick{k}', V(f, 0.0), 0.15, 6, 11 + k, size=0.07)
        figure(P, CLOD_AT)
        marks = [('tell', 'TELL: DIGS IN', dig + Vector((0.0, 0.0, 0.6)))]
    elif i == 1:
        her(OVERHEAD, CLOD_AT, PLAYER)
        clod('Held', launch, 0.55, 1)
        falling('Crumbs', launch - Vector((0.0, 0.0, 0.5)), 14, 3.4, 0.35, 0.05, 4)
        puffs('Sift', launch - Vector((0.0, 0.0, 1.6)), 0.3, 4, (-1.4, 0.6), 0.6, puff(0.45), 5)
        crater('Torn', dig, 6, size=0.9)
        figure(P, CLOD_AT)
        marks = [('shoot', 'SHOOT IT: STUNS HER', launch)]
    elif i == 2:
        her(THROW, CLOD_AT, PLAYER)
        p = arc_pt(0.07)
        clod('Thrown', p, 0.55, 1)
        falling('Crumbs', p - Vector((0.0, 0.0, 0.3)), 10, 2.6, 0.5, 0.05, 7)
        crater('Torn', dig, 6, size=0.9)
        figure(P, CLOD_AT)
    elif i == 3:
        s = 0.9                           # 2.55 s: coming down at you, 1.35 s of its 1.5 s flown
        her(STANDT, CLOD_AT, PLAYER)
        p = arc_pt(s)
        clod('Flying', p, 0.55, 1)
        pts = [arc_pt(s - 0.4 + 0.4 * k / 16) for k in range(17)]
        camera_ribbon('Trail', pts, [0.08 + 0.95 * (k / 16) ** 1.3 for k in range(17)],
                      mat_trail_dust('BW_Trail', EARTH_DUST, 1.0), CLOD_CAM[0])
        # Seen from the player the trail runs end-on, so the dust it sheds (camera-facing puffs) carries it.
        for k in range(7):
            sk = s - 0.05 - 0.055 * k
            card(f'Wake{k}', arc_pt(sk), 0.95 - 0.08 * k, puff(0.75 - 0.08 * k))
        shed = []
        for j in range(7):
            sj = s - 0.04 - 0.045 * j
            q = arc_pt(sj) - Vector((0.0, 0.0, 4.9 * ((s - sj) * 1.5) ** 2))
            if q.z > 0.1:
                shed.append(q)
        chunks_at('Shed', shed, 0.06, grave_earth(), 8)
        decal('Shadow', land, 1.0 + 1.6 * s, 'LandShadow', opacity=0.9)
        crater('Torn', dig, 6, size=0.9)
        figure(P.lerp(dodged, 0.6), CLOD_AT)
        marks = [('tell', 'ITS SHADOW: WHERE IT LANDS', V(land, 0.05))]
    elif i == 4:
        her(STANDT, CLOD_AT, PLAYER)
        decal('Scar', P, 2.6, 'ImpactScar', turn=1.0)
        clod_burst('Burst', P, 0.1, 5)
        crater('Torn', dig, 6, size=0.9)
        figure(dodged, CLOD_AT)
    else:
        her(HEAVY, CLOD_AT, PLAYER)
        decal('Scar', P, 2.6, 'ImpactScar', turn=1.0)
        rubble('Pile', P, 7)
        puffs('Settle', P, 1.8, 6, (0.4, 1.8), 1.8, puff(0.22), 9)
        crater('Torn', dig, 6, size=0.9)
        figure(dodged, CLOD_AT)
        marks = [('cover', 'COVER FOR 10 s', P + Vector((0.0, 0.0, 1.0)))]
    return marks


CTR_CAM = ((-5.4, -6.2, 3.4), (5.0, 0.9, 2.2), 30.0)


def strip_countered(i):
    fwd, side, launch, land, dig, arc_pt = clod_geometry()
    eye = V(PLAYER, 1.45)
    marks = []
    if i == 0:
        held = held_point(LIFTING, CLOD_AT, PLAYER)
        her(LIFTING, CLOD_AT, PLAYER)
        clod('Held', held, 0.55, 1)
        falling('Crumbs', held - Vector((0.0, 0.0, 0.5)), 10, 2.4, 0.35, 0.05, 4)
        crater('Torn', dig, 6, size=0.9)
        marks = [('shoot', 'IN HER GRIP: SHOOT IT', held)]
    elif i == 1:
        her(OVERHEAD, CLOD_AT, PLAYER)
        clod('Held', launch, 0.55, 1)
        hit = launch + (eye - launch).normalized() * 0.5
        tracer('Shot', launch + (eye - launch) * 0.45, hit, flash=1.1)
        thrown('Pop', hit, 0.08, 8, 12, size=0.05, speed=(2.0, 4.0), up=(1.0, 3.0), start_r=0.05)
        crater('Torn', dig, 6, size=0.9)
        marks = [('shoot', 'HIT', hit)]
    elif i == 2:
        rec = her(RECOIL, CLOD_AT, PLAYER)
        puffs('Burst', launch, 1.2, 15, (-0.7, 0.8), 1.7, puff(0.95), 13)
        puffs('BurstLow', launch - Vector((0.0, 0.0, 1.4)), 1.4, 7, (-0.6, 0.4), 1.1, puff(0.6), 14)
        falling('Rain', launch, 26, 3.6, 1.3, 0.08, 15)
        dirt_on(rec, 3, 0.5)
        crater('Torn', dig, 6, size=0.9)
    elif i in (3, 4):
        rec = her(STUN, CLOD_AT, PLAYER)
        dirt_on(rec, 3, 1.0)
        top = cpt(rec, 'head', HEAD_C + Vector((0.0, 0.0, 30.0))) + Vector((0.0, 0.0, 1.0))
        daze('Daze', top, 1.0, 0.3 + 1.4 * (i - 3), scale=2.2)
        lumps('Fallen', V(launch, 0.0), 18, 2.2, 0.09, grave_earth(), 16)
        if i == 3:
            puffs('Settle', V(launch, 0.0), 1.8, 9, (0.3, 1.8), 1.5, puff(0.45), 17)
        else:
            puffs('Settle', V(launch, 0.0), 2.4, 6, (0.2, 1.2), 1.8, puff(0.18), 17)
        crater('Torn', dig, 6, size=0.9)
        marks = [('crit', 'HEAD: CRIT', head_pt(rec)), ('tell', 'STUNNED 2 s', top)]
    else:
        rec = her(RISE, CLOD_AT, PLAYER)
        dirt_on(rec, 3, 0.4)
        c = cpt(rec, 'body', Vector((0.0, 0.0, RIDE)))
        falling('Shake', c + Vector((0.0, 0.0, 0.8)), 16, 1.4, 1.4, 0.07, 18)
        puffs('Shake', V(c, 0.0), 2.2, 8, (0.2, 1.0), 1.2, puff(0.5), 19)
        lumps('Fallen', V(launch, 0.0), 18, 2.2, 0.09, grave_earth(), 16)
        crater('Torn', dig, 6, size=0.9)
    return marks


FUR_S, FUR_P = (1.0, 2.0), (-12.5, -1.0)
FUR_CAM = ((-7.6, -12.7, 6.6), (-7.4, 0.15, 0.5), 24.0)


def furrow_geometry():
    fwd = flat_dir(FUR_S, FUR_P)
    start = V(FUR_S, 0.0) + fwd * 2.8
    end = V(FUR_P, 0.0) + fwd * 1.5
    pts = [p for p in FC['jag'](start, end, 8, 0.22, step=0.3)]
    return fwd, start, end, pts


def heave(name, pts, frac, seed, jets=True):
    """The Furrow's tell: the floor heaving along pts up to frac of it, split along its crest by the heave crack, a
    bulge at its crawling front, dust jetting from the newest stretch of crack."""
    n = len(pts)
    upto = max(int(n * frac), 5)
    line = [V(p, 0.0) for p in pts[:upto]]
    noise_ = noise.noise
    hts = []
    for k in range(upto):
        t = k / (upto - 1)
        h = 0.3 + 0.12 * noise_(Vector((k * 0.22, seed, 0.0))) + 0.05 * noise_(Vector((k * 0.9, seed, 3.0)))
        h += 0.28 * lc.smoothstep(0.76, 0.93, t)           # the bulge at the crawling front
        h *= lc.smoothstep(0.0, 0.06, t) * (1.0 - lc.smoothstep(0.95, 1.0, t) * 0.8)
        hts.append(h)

    def zf(k, u):
        k = min(max(k, 0), upto - 1)
        rough = 0.3 * noise_(Vector((k * 0.45, u * 3.0, seed + 2.0))) * lc.smoothstep(0.2, 0.6, abs(u))
        return hts[k] * max(0.0, math.cos(u * math.pi / 2)) ** 1.7 * (1.0 + rough)
    half_w = 1.15
    swept(name, line, half_w, zf, floor_mat(), seed, cols=17)       # the floor itself rising: no seam at its foot
    crack_to = max(int(upto * 0.86), 3)
    crack_line(name + 'Crack', line[:crack_to], 0.7, zf=zf, half_w=half_w)
    # Plates of the floor's crust pushed up and tipped along both lips of the crack.
    rng = random.Random(seed)
    plates = []
    for k in range(1, crack_to, 2):
        t = line[min(k + 1, upto - 1)] - line[k - 1]
        side = Vector((-t.y, t.x, 0.0)).normalized()
        for sgn in (1, -1):
            off = rng.uniform(0.3, 0.55)
            plates.append(line[k] + side * sgn * off + Vector((0.0, 0.0, zf(k, sgn * off / (half_w * 1.15)) - 0.01)))
    shards(name + 'Plates', plates, 0.14, seed, mat=berm_earth())
    front = line[-1] + Vector((0.0, 0.0, hts[int(upto * 0.88)] + 0.1))
    lumps(name + 'Pop', front, 8, 0.6, 0.06, floor_mat(), seed, z=(0.05, 0.55))
    if jets:
        for j, t in enumerate((0.42, 0.6, 0.76, 0.9)):
            k = int(crack_to * t)
            if k < 2:
                continue
            base = line[k] + Vector((0.0, 0.0, zf(k, 0.0)))
            hgt = (1.3, 2.0, 2.4, 1.8)[j]
            for c in range(2):          # two cards a jet, a little apart: a thicker column
                card(f'{name}Jet{j}_{c}', base + Vector((0.12 * c, 0.1 * c, hgt * (0.45 - 0.08 * c))), hgt * (1.0 - 0.15 * c),
                     jet((0.75, 0.95, 1.0, 1.0)[j]), roll_angle=0.0, aspect=0.45)
            puffs(f'{name}JetFoot{j}', base, 0.35, 2, (0.05, 0.25), 0.6, puff(0.8), seed + j)
    return front, zf, line


def strip_furrow(i):
    fwd, start, end, pts = furrow_geometry()
    side = left_of(fwd)
    figure_at = V(FUR_P, 0.0)
    marks = []
    if i == 0:
        rec = her(DIG, FUR_S, FUR_P)
        for k, s in enumerate(('0_l', '0_r')):
            f = foot(rec, s)
            dig_spot(f'Dig{k}', V(f, 0.0), 3 + k)
            thrown(f'Kick{k}', V(f, 0.0), 0.15, 6, 11 + k, size=0.07)
        heave('Heave', pts, 0.1, 3, jets=False)
        figure(figure_at, FUR_S)
        marks = [('tell', 'TELL: DIGS IN', V(start, 0.6))]
    elif i == 1:
        her(DIG, FUR_S, FUR_P)
        front, zf, line = heave('Heave', pts, 0.45, 3)
        figure(figure_at, FUR_S)
        marks = [('tell', 'THE HEAVE, CRAWLING AT YOU', front)]
    elif i == 2:
        her(HEAVY, FUR_S, FUR_P)
        front, zf, line = heave('Heave', pts, 1.0, 3)
        figure(figure_at + side * 2.4, FUR_S)
        marks = [('tell', 'STEP OUT OF ITS LINE', front)]
    elif i == 3:
        n = len(pts)
        mid = start.lerp(end, 0.5)
        at = mid - fwd * 2.4
        her(PLOUGH, at, FUR_P)
        k0 = int(n * 0.5)
        ahead = [V(p, 0.0) for p in pts[k0:]]
        na = len(ahead)

        def zf_ahead(k, u):
            return (0.22 * max(0.0, math.cos(u * math.pi / 2)) ** 1.7 * lc.smoothstep(0, 3, k)
                    * (1 - 0.8 * lc.smoothstep(na - 3, na - 1, k)))
        swept('Ahead', ahead, 1.15, zf_ahead, floor_mat(), 4, cols=17)
        crack_line('AheadCrack', ahead[2:-2], 0.7, zf=lambda k, u: zf_ahead(k + 2, u), half_w=1.15)
        trench = [V(p, 0.0) for p in pts[:k0 - 2]]
        furrow_strip('Trench', trench)
        for sgn in (1, -1):
            berm_line(f'Berm{sgn}', start, mid - fwd * 3.6, sgn * 1.45, 20 + sgn, rising=1.0, inner=-sgn)
        nose = mid + Vector((0.0, 0.0, 0.3))
        for sgn in (1, -1):
            thrown(f'Spray{sgn}', nose, 0.28, 14, 30 + sgn, speed=(3.0, 6.0), up=(3.0, 5.5), size=0.1,
                   heading=math.atan2(side.y * sgn + fwd.y * 0.3, side.x * sgn + fwd.x * 0.3), spread=0.5)
            puffs(f'SprayDust{sgn}', nose + side * sgn * 1.6, 0.9, 5, (0.3, 1.4), 1.2, puff(0.75), 33 + sgn)
        for k, p in enumerate(trench[::4]):
            puffs(f'Wake{k}', p, 0.8, 2, (0.3, 1.2), 1.3, puff(0.5 * (k + 1) / max(len(trench) // 4, 1)), 40 + k)
        figure(figure_at + side * 2.4, FUR_S)
    elif i in (4, 5):
        crumbling = i == 5
        trench = [V(p, 0.0) for p in pts]
        furrow_strip('Trench', trench)
        for sgn in (1, -1):
            berm_line(f'Berm{sgn}', start, end, sgn * 1.45, 20 + sgn, slump=1.0 if crumbling else 0.0, inner=-sgn)
        if crumbling:
            her(STANDT, (5.0, -3.0), (-8.0, -10.0))
            for k in range(5):
                p = start.lerp(end, 0.1 + 0.2 * k) + side * (1.45 if k % 2 else -1.45)
                puffs(f'Crumble{k}', p, 0.8, 2, (0.2, 0.7), 0.9, puff(0.4), 50 + k)
            marks = [('cover', 'CRUMBLING AT 20 s', start.lerp(end, 0.55) - side * 1.45 + Vector((0.0, 0.0, 0.6)))]
        else:
            her(CHARGE, end, end + turned(fwd, 70) * 5.0)
            for k, p in enumerate(trench[::5]):
                puffs(f'Settle{k}', p, 1.0, 2, (0.3, 1.4), 1.6, puff(0.28), 60 + k)
            marks = [('cover', 'BERMS: COVER FOR 20 s', start.lerp(end, 0.45) - side * 1.45 + Vector((0.0, 0.0, 0.8)))]
        figure(start.lerp(end, 0.45) - side * 2.6, FUR_S)
    return marks


UN_AT, UN_TO = (2.6, 0.8), (-16.0, -1.0)
UN_CAM = ((-7.5, -3.8, 2.8), (1.4, 0.5, 1.25), 28.0)
SHOT_MOUND = 2


def mound_spots():
    f = flat_dir(UN_AT, UN_TO)
    return [V(UN_AT, 0.0) + turned(f, a) * 5.4 for a in (55, 18, -18, -55)]


def ling_out(name, at, toward, stage, seed):
    """A spiderling at a stage of digging out: 'in' (head and front legs out of the mound), 'half', 'out'."""
    at = V(at, 0.0)
    if stage == 'in':
        rec = spider(name, EMERGE_IN, (at.x, at.y, -0.12), toward, LING)
    elif stage == 'half':
        rec = spider(name, EMERGE_HALF, (at.x, at.y, -0.13), toward, LING)
    else:
        rec = spider(name, RUN2 if seed % 2 else CHARGE, (at.x, at.y, 0.0), toward, LING)
    ling_mud(rec, seed)
    return rec


def strip_unearthed(i):
    spots = mound_spots()
    f = flat_dir(UN_AT, UN_TO)
    marks = []
    if i == 0:
        rec = her(SLAM, UN_AT, UN_TO)
        front = V((foot(rec, '0_l') + foot(rec, '0_r')) * 0.5, 0.0)
        decal('Slam', front, 3.4, 'ImpactScar', turn=0.4)
        puffs('SlamRing', front, 2.4, 16, (0.1, 0.5), 1.3, puff(0.85), 3, ring=True)
        thrown('SlamKick', front, 0.2, 12, 4, size=0.07)
        for k, s in enumerate(spots):
            crack_line(f'Run{k}', [front.lerp(s, t) for t in (0.12, 0.3, 0.45, 0.6)], 0.45)
        marks = [('tell', 'THE SLAM (66% AND 33%)', front + Vector((0.0, 0.0, 0.4)))]
    elif i == 1:
        her(CALL, UN_AT, UN_TO)
        for k, s in enumerate(spots):
            crack_line(f'Run{k}', [V(UN_AT, 0.0) + f * 2.8 + (s - V(UN_AT, 0.0) - f * 2.8) * t for t in (0.0, 0.5, 0.92)],
                       0.45)
            top = mound(f'Mound{k}', s, 0.6, 10 + k)
            marks.append(('shoot', 'SHOOT THE MOUNDS: 1 s' if k == SHOT_MOUND else '', top))
    elif i == 2:
        her(HEAVY, UN_AT, UN_TO)
        for k, s in enumerate(spots):
            if k == SHOT_MOUND:
                crater(f'Mound{k}', s, 10 + k, collapsed=True)
                puffs(f'Collapse{k}', s, 0.6, 7, (0.2, 0.9), 1.0, puff(0.85), 20 + k)
                thrown(f'Fall{k}', s, 0.12, 8, 21, size=0.06, speed=(1.0, 2.5), up=(1.5, 3.0))
                tube(f'Leg{k}', [s + Vector((0.2, 0.1, 0.05)), s + Vector((0.28, 0.14, 0.22)), s + Vector((0.2, 0.2, 0.3))],
                     [0.02, 0.016, 0.006], SPIDER['MATERIAL'], sides=6)
                tracer('Shot', s + (V(PLAYER, 1.4) - s) * 0.4 + Vector((0.0, 0.0, 0.3)), s + Vector((0.0, 0.0, 0.25)),
                       flash=0.0)
                marks.append(('shoot', 'SHOT IN THE GROUND: ONE LESS', s + Vector((0.0, 0.0, 0.3))))
            else:
                mound(f'Mound{k}', s, 1.0, 10 + k)
    else:
        her(HEAVY if i < 5 else STANDT, UN_AT, UN_TO)
        stage = {3: 'in', 4: 'half', 5: 'out'}[i]
        for k, s in enumerate(spots):
            if k == SHOT_MOUND:
                crater(f'Mound{k}', s, 10 + k, collapsed=True)
                tube(f'Leg{k}', [s + Vector((0.2, 0.1, 0.05)), s + Vector((0.28, 0.14, 0.22)), s + Vector((0.2, 0.2, 0.3))],
                     [0.02, 0.016, 0.006], SPIDER['MATERIAL'], sides=6)
                continue
            crater(f'Mound{k}', s, 10 + k)
            toward = V(PLAYER, 0.0) + Vector((0.0, (k - 1.5) * 3.0, 0.0))
            if stage == 'out':
                run_at = s + flat_dir(s, toward) * 1.2
                ling_out(f'Ling{k}', run_at, toward, 'out', 30 + k)
                puffs(f'Kick{k}', run_at - flat_dir(s, toward) * 0.5, 0.35, 3, (0.05, 0.3), 0.5, puff(0.7, DUST), 40 + k)
            else:
                ling_out(f'Ling{k}', s, toward, stage, 30 + k)
                thrown(f'Spray{k}', s, 0.12 if stage == 'in' else 0.4, 12, 50 + k, size=0.06, speed=(1.5, 3.5),
                       up=(2.0, 4.0))
                puffs(f'Burst{k}', s, 0.5, 5, (0.2, 0.9), 0.9, puff(0.85 if stage == 'in' else 0.5), 60 + k)
    return marks


PH_AT, PH_TO = (0.5, 1.0), (-12.0, 6.0)
PH_CAM = ((-1.6, -8.0, 5.0), (0.3, 1.0, 1.3), 30.0)        # from her left, up, so the stone shows


def strip_phase2(i):
    marks = []
    f = flat_dir(PH_AT, PH_TO)
    left = left_of(f)
    floor_low = V(PH_AT, 0.0) + left * 2.2 - f * 2.2          # beside and behind her, toward the camera
    floor_high = V(PH_AT, 0.0) + left * 0.8 - f * 4.0
    cam = PH_CAM[0]
    if i == 0:
        rec = her(STANDT, PH_AT, PH_TO)
        stone_cracks(rec)
        hit = stone_pt(rec, 70.0)
        spark_burst('Glance', hit, stone_normal(rec), cam, 3, scale=1.4)
        shards('Flying', [hit + Vector((random.uniform(-0.8, 0.8), random.uniform(-0.8, 0.8), random.uniform(-0.2, 0.9)))
                          for _ in range(8)], 0.06, 4)
        marks = [('cover', 'THE STONE CRACKS AT 33%', stone_pt(rec, 40.0))]
    elif i == 1:
        rec = her(FLINCH, PH_AT, PH_TO, stone=False)
        n = stone_normal(rec)
        stone_piece(rec, 'Low', 'low', move=(left * 0.08 - n * 0.02, f, 6.0))
        stone_piece(rec, 'High', 'high', move=(-left * 0.06 + n * 0.12 + UP * 0.08, f, -8.0))
        root_stubs(rec, 5)
        c = stone_pt(rec, 60.0)
        shards('Chips', [c + Vector((random.uniform(-0.9, 0.9), random.uniform(-0.9, 0.9), random.uniform(-0.3, 0.8)))
                         for _ in range(10)], 0.06, 6)
        puffs('Grit', c, 0.6, 5, (-0.3, 0.4), 0.7, puff(0.65, STONE_DUST), 7)
    elif i == 2:
        rec = her(SLUMP, PH_AT, PH_TO, phase=3)
        root_stubs(rec, 5)
        stone_piece(rec, 'Low', 'low', move=(left * 0.9 - UP * 0.9, f, 38.0))
        stone_piece(rec, 'High', 'high', move=(-f * 1.1 - UP * 0.5 + left * 0.3, left, -50.0))
        c = cpt(rec, 'abdomen', ABD_C)
        falling('Crust', c + Vector((0.0, 0.0, 0.6)), 16, 1.6, 1.1, 0.09, 8, mat=mud())
        puffs('Grit', c, 1.0, 5, (-0.4, 0.6), 0.9, puff(0.5, STONE_DUST), 9)
        marks = [('crit', 'BONE UNDER IT', bare_pt(rec))]
    else:
        pose = {3: STANDT, 4: BARE_RISE, 5: RUN2}[i]
        at = V(PH_AT) + f * (2.6 if i == 5 else 0.0)
        rec = her(pose, at, V(at) + f * 10.0, phase=3)
        root_stubs(rec, 5)
        stone_piece(rec, 'Low', 'low', ground=(floor_low, 30.0, 4.0))
        stone_piece(rec, 'High', 'high', ground=(floor_high, -60.0, -8.0))
        if i == 3:
            for k, p in enumerate((floor_low, floor_high)):
                puffs(f'Land{k}', p, 0.9, 7, (0.1, 0.8), 1.1, puff(0.8), 10 + k)
                shards(f'Chips{k}', [p + Vector((random.uniform(-1.2, 1.2), random.uniform(-1.2, 1.2), 0.03))
                                     for _ in range(7)], 0.06, 11 + k)
            marks = [('crit', 'BARE: ALL OF IT A CRIT', bare_pt(rec))]
        elif i == 4:
            marks = [('crit', 'ABDOMEN: ALL CRIT', bare_pt(rec)), ('crit', 'HEAD: CRIT', head_pt(rec))]
        else:
            for s in ('0_l', '1_r', '2_l', '3_r', '3_l'):
                puffs(f'Kick{s}', V(foot(rec, s), 0.0), 0.4, 3, (0.05, 0.4), 0.7, puff(0.7, DUST), zlib.crc32(s.encode()))
            for k in range(4):
                puffs(f'Wake{k}', V(at) - f * (2.5 + 1.4 * k), 1.0, 2, (0.2, 1.0), 1.4, puff(0.45 - 0.08 * k, DUST), 70 + k)
            marks = [('crit', 'FASTER NOW', head_pt(rec))]
    return marks


STRIPS = {
    'clod': dict(fn=strip_clod, cam=CLOD_CAM, title='Grave Clod  |  projectile',
                 summary='Front legs dig in and tear a clod of grave earth overhead: the tell, 1.2 s. She lobs it at '
                         'where you stand: about 1.5 s to 20 m, its shadow growing where it will land. It bursts into '
                         'dust and debris and leaves a rubble pile: cover for 10 s.',
                 frames=[(0.0, 'Dig in: her front legs tear into the floor.'),
                         (0.8, 'Lift: the clod overhead, dirt sifting off it. Shoot it now.'),
                         (1.2, 'Throw: a high lob at where you stand.'),
                         (2.55, 'Flight: coming down at you, shedding dust, its shadow on the floor.'),
                         (2.8, 'Burst: dust and debris where you stood.'),
                         (4.0, 'Rubble: a pile of grave earth; cover for 10 s.')]),
    'countered': dict(fn=strip_countered, cam=CTR_CAM, title='Grave Clod countered  |  shoot it in her grip',
                      summary='A hit on the clod while she holds it bursts it over her: the dirt rains on her head '
                              'and back and she is stunned 2 s, legs splayed and head down, her crits open.',
                      frames=[(0.6, 'Lift: the clod coming up in her grip.'),
                              (0.8, 'The shot: a hit on the clod.'),
                              (0.9, 'It bursts over her: dirt rains on her.'),
                              (1.3, 'Stunned: legs splayed, head down, stars over her head.'),
                              (2.4, 'Still stunned: the head is low and open.'),
                              (3.0, 'She shakes it off and stands.')]),
    'furrow': dict(fn=strip_furrow, cam=FUR_CAM, title='Furrow  |  ground charge',
                   summary='A 2 s tell: the floor heaves and splits in a line toward you, dust jetting from the '
                           'crack. Then she ploughs the line head-low and throws up a berm each side: cover for '
                           '20 s, then they crumble.',
                   frames=[(0.0, 'Dig in: head low, front legs in the floor.'),
                           (0.7, 'The heave crawls at you, splitting, dust jetting.'),
                           (1.6, 'It reaches where you stood: step out of its line.'),
                           (2.4, 'The plough: head low, earth thrown both sides.'),
                           (3.2, 'Berms left each side of the furrow: cover.'),
                           (23.2, 'At 20 s the berms slump and crumble away.')]),
    'unearthed': dict(fn=strip_unearthed, cam=UN_CAM, title='Unearthed  |  at 66% and 33%',
                      summary='She slams the floor and four mounds bulge for 1 s; a mound shot while it bulges kills '
                              'the spiderling in it. The rest dig out: the game\'s spider at 0.45x, caked in grave mud.',
                      frames=[(0.0, 'The slam: cracks race out to four spots.'),
                              (0.4, 'Four mounds bulge (1 s): shoot them.'),
                              (0.8, 'One shot: it caves in on the spiderling.'),
                              (1.0, 'The rest burst: heads and front legs out.'),
                              (1.3, 'Clawing out, shedding mud.'),
                              (1.8, 'Out and running at you.')]),
    'phase2': dict(fn=strip_phase2, cam=PH_CAM, title='Phase 2  |  at 33% the stone comes off',
                   summary='The headstone is armour: shots glance off it with sparks and chips. The hit that takes '
                           'her to 33% cracks it off: the roots snap, it falls, her pale Bone abdomen is bare and all '
                           'of it is a crit. She runs faster from here.',
                   frames=[(0.0, 'The 33% hit: cracks race across the stone.'),
                           (0.3, 'It splits; the roots snap.'),
                           (0.6, 'The halves slide off her back.'),
                           (1.0, 'They hit the floor: her abdomen is bare.'),
                           (1.5, 'She rears: the whole abdomen is a crit now.'),
                           (2.0, 'And runs faster for the rest of the fight.')]),
}


def part_moves(keys):
    moves = DATA.setdefault('moves', {})
    for key in keys:
        spec = STRIPS[key]
        frames = []
        for i, (t, caption) in enumerate(spec['frames']):
            frame_seed(f'{key}{i}')
            light()
            marks = spec['fn'](i) or []
            cam, target, lens = spec['cam']
            entry = shoot(f'moves_{key}_{i + 1}', cam, target, lens, FRAME, marks)
            entry.update(t=t, caption=caption)
            frames.append(entry)
            clear_shot()
        moves[key] = dict(title=spec['title'], summary=spec['summary'], frames=frames)


# --- The FX pieces, each alone on black (and again for its alpha) ---

FXO = Vector((0.0, -150.0, 0.0))
FX_AZ, FX_EL = math.radians(200.0), math.radians(20.0)
FX_BACK = Vector((math.sin(FX_AZ) * math.cos(FX_EL), math.cos(FX_AZ) * math.cos(FX_EL), math.sin(FX_EL)))
FX_RIGHT = Vector((-math.cos(FX_AZ), math.sin(FX_AZ), 0.0))


def dims_cm(objs):
    lo, hi = Vector((1e9,) * 3), Vector((-1e9,) * 3)
    bpy.context.view_layer.update()
    for o in objs:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            lo = Vector((min(lo[i], w[i]) for i in range(3)))
            hi = Vector((max(hi[i], w[i]) for i in range(3)))
    d = (hi - lo) * 100.0
    return f'{d.x:.0f} x {d.y:.0f} x {d.z:.0f} cm'


def new_objs(before):
    return [o for o in FC['SHOT'] if o not in before and o.type == 'MESH']


def build_piece(fn):
    before = set(FC['SHOT'])
    fn()
    return new_objs(before)


def cut_below(objs, z=0.0):
    """Ground pieces end flat at the floor (the sheet shows them with no floor under them)."""
    for o in objs:
        if o.type != 'MESH':
            continue
        bm = bmesh.new()
        bm.from_mesh(o.data)
        geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
        bmesh.ops.bisect_plane(bm, geom=geom, plane_co=(0.0, 0.0, z), plane_no=(0.0, 0.0, 1.0), clear_inner=True)
        bmesh.ops.holes_fill(bm, edges=[e for e in bm.edges if e.is_boundary])
        bm.to_mesh(o.data)
        bm.free()
    return objs


def fx_clods():
    out = []
    for k, (r, x) in enumerate(((0.3, -1.45), (0.42, -0.3), (0.55, 1.15))):
        objs = build_piece(lambda: clod(f'Clod{k}', FXO + FX_RIGHT * x + UP * 0.6, r, k + 1))
        out.append((('S', 'M', 'L')[k], dims_cm(objs)))
    return out


def fx_trail():
    """Coming down at you: the trail from the upper left to the clod at the lower right."""
    p0, p1 = FXO - FX_RIGHT * 3.0 + UP * 1.5, FXO + FX_RIGHT * 2.0 + UP * 0.45
    pts = [p0.lerp(p1, k / 16) + UP * 0.6 * math.sin(math.pi * k / 20) for k in range(17)]
    camera_ribbon('Trail', pts, [0.08 + 0.95 * (k / 16) ** 1.3 for k in range(17)],
                  mat_trail_dust('BW_Trail', EARTH_DUST, 1.0), FXO + FX_BACK * 40.0)
    clod('Head', pts[-1], 0.42, 2)
    chunks_at('Shed', [pts[k] - UP * (0.12 + 0.05 * (16 - k)) for k in range(5, 15, 2)], 0.06, grave_earth(), 3)
    return [('ribbon', '540 cm long, 8 to 100 cm wide, faces the camera')]


def fx_burst():
    for k, t in enumerate((0.1, 0.4, 1.0)):
        clod_burst(f'Burst{k}', FXO + FX_RIGHT * (-5.3 + 5.2 * k), t, 5)
    return [('0.1 s', '19 cards of 75-100 cm, within 1 m'), ('0.4 s', '24 cards of 100-150 cm, within 2 m'),
            ('1.0 s', '22 cards of 140-200 cm, within 2.6 m, 30-38% opaque')]


def debris_set(at, scale=1.0):
    """The five debris meshes a burst throws: an earth clump, a field stone, a coffin splinter, a bone, a root end."""
    parts = []
    pos = [at + FX_RIGHT * x for x in (-0.8, -0.4, 0.02, 0.45, 0.82)]
    parts.append(('clump', build_piece(lambda: blobs('Clump', [(pos[0], (0.13, 0.11, 0.09), None, 3, 0.35)], grave_earth(),
                                                          box='GroundDirt', subdiv=1, smooth=False))))
    parts.append(('stone', build_piece(lambda: blobs('Stone', [(pos[1], (0.1, 0.08, 0.06), None, 4, 0.3)], granite(),
                                                          box='RockGranite', subdiv=1, smooth=False))))

    def splinter():
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0)
        for v in bm.verts:
            if v.co.x > 0:
                v.co.y *= 0.3
                v.co.z *= 0.6
        bmesh.ops.transform(bm, matrix=T(pos[2]) @ Matrix.Rotation(0.5, 4, 'Z') @ Matrix.Rotation(0.3, 4, 'Y')
                            @ Matrix.Diagonal((0.4, 0.08, 0.03, 1.0)), verts=bm.verts)
        o = finish_bm('Splinter', bm, textured('WoodPlanks', 'GMF_Coffin', tint=0x8a7a68), smooth=False)
        bpy.context.view_layer.update()
        lt.box_uv(o, 'WoodPlanks')
    parts.append(('splinter', build_piece(splinter)))
    parts.append(('bone', build_piece(lambda: tube('Bone', [pos[3] - FX_RIGHT * 0.14, pos[3] + FX_RIGHT * 0.14 + UP * 0.04],
                                                   [0.035, 0.028], mat_surface('GMF_BoneChip', 0xd9d1bf, rough=0.6),
                                                   sides=8))))
    parts.append(('root', build_piece(lambda: tube('Root', [pos[4] - FX_RIGHT * 0.16, pos[4], pos[4] + FX_RIGHT * 0.12
                                                            + UP * 0.1], [0.04, 0.03, 0.008], bark(), sides=7))))
    return [(k, dims_cm(objs)) for k, objs in parts]


def fx_debris():
    return debris_set(FXO + UP * 0.45)


def fx_rubble():
    objs = cut_below(build_piece(lambda: rubble('Pile', FXO - FX_RIGHT * 0.2, 7)))
    return [('pile', dims_cm([o for o in objs if 'Loose' not in o.name]) + ', loose clods round it')]


def fx_jets():
    p0, p1 = FXO - FX_RIGHT * 1.7, FXO + FX_RIGHT * 1.3
    crack_line('Crack', [p0.lerp(p1, k / 8) for k in range(9)], 0.7, cap=True)
    for j, (t, h, o) in enumerate(((0.2, 1.0, 0.55), (0.45, 1.7, 0.8), (0.68, 2.1, 0.95))):
        card(f'Jet{j}', p0.lerp(p1, t) + UP * (h * 0.45), h, jet(o), roll_angle=0.0, aspect=0.34)
    return [('jet cards', '34 x 100 to 70 x 210 cm, upright, on the newest crack'), ('crack', 'the strip at 70 cm wide')]


def fx_berm():
    one = cut_below(build_piece(lambda: berm_segment('Seg', FXO - FX_RIGHT * 4.6, FX_RIGHT, 3)))
    cut_below(build_piece(lambda: berm_line('Row', FXO - FX_RIGHT * 1.6, FXO + FX_RIGHT * 4.6, 0.0, 9)))
    return [('segment', dims_cm([o for o in one if 'Clods' not in o.name])),
            ('row', 'one every 100 cm, each half over the next, turned up to 5 degrees, heights +/-12%')]


def fx_mounds():
    closed = cut_below(build_piece(lambda: mound('Closed', FXO - FX_RIGHT * 1.1, 1.0, 3, pebbles=False)))
    burst = cut_below(build_piece(lambda: crater('Burst', FXO + FX_RIGHT * 1.15, 4)))
    return [('bulging', dims_cm([o for o in closed if 'Skirt' not in o.name]) + ' at full bulge (scaled up over 1 s)'),
            ('burst', dims_cm([o for o in burst if 'Scar' not in o.name and 'Loose' not in o.name]) + ', a hole decal in it')]


def fx_chips():
    pts = [FXO + FX_RIGHT * x + UP * (0.5 + 0.06 * (k % 2)) for k, x in enumerate((-0.36, -0.18, 0.0, 0.18, 0.36))]
    objs = build_piece(lambda: shards('Chips', pts, 0.06, 4))
    return [('each', '8 to 16 cm, flat flakes of the headstone\'s granite'), ('set', dims_cm(objs))]


def fx_sparks():
    spark_burst('Glance', FXO + UP * 0.45 + FX_RIGHT * 0.15, -FX_BACK + UP * 0.5 - FX_RIGHT * 0.6,
                FXO + FX_BACK * 40.0, 3, scale=1.0)
    return [('streaks', '20-55 cm long, 1-5 cm wide'), ('flash', 'a 55 cm card'), ('stone dust', '4 cards of 30 cm')]


def fx_daze():
    daze('Daze', FXO + UP * 0.5, 0.85, 0.4, scale=1.7)
    return [('ring', '170 cm across over her head, 4 stars of 37 cm'), ('trails', 'tubes, a radian behind each star')]


# (key, builder, ortho width m, content height m, kind, title, note): the camera looks a little above the content,
# so the alpha inset (top right) stays clear of it.
FX_PIECES = [
    ('clods', fx_clods, 3.9, 0.6, 'MESH', 'Grave clod: 3 sizes', 'earth with field stones, a coffin plank and a bone'),
    ('trail', fx_trail, 6.6, 1.0, 'CARD', 'Clod flight trail', 'a ribbon of earth dust behind the clod, crumbs falling off'),
    ('burst', fx_burst, 15.5, 1.1, 'CARD', 'Dust burst at 0.1, 0.4, 1.0 s', 'puff cards from the 2x2 sheet; debris meshes'),
    ('debris', fx_debris, 2.1, 0.45, 'MESH', 'Debris: 5 meshes', 'clump, stone, coffin splinter, bone, root end'),
    ('rubble', fx_rubble, 3.8, 0.45, 'MESH', 'Rubble pile', 'what a clod leaves: crouch cover for 10 s'),
    ('jets', fx_jets, 4.4, 1.0, 'CARD', 'Dust jets along the crack', 'upright cards from the jet sheet'),
    ('berm', fx_berm, 10.6, 0.3, 'MESH', 'Berm: one segment, and a row', 'one segment repeats along each side of the furrow'),
    ('mounds', fx_mounds, 4.4, 0.3, 'MESH', 'Spawn mound: bulging and burst', 'crack overlay mapped from above; crater over a hole'),
    ('chips', fx_chips, 1.2, 0.53, 'MESH', 'Headstone chips: 5 meshes', 'granite flakes knocked off by shots'),
    ('sparks', fx_sparks, 1.9, 0.55, 'EMISSIVE', 'Sparks: a shot glancing off', 'streaks, a flash card, stone dust, chips'),
    ('daze', fx_daze, 2.4, 0.5, 'EMISSIVE', 'Stun cue', 'four stars wheeling over her head for the 2 s'),
]


def part_fx():
    hidden = []
    for o in bpy.data.objects:
        if o.name in ('_Floor', '_SinkRing') or o in G['SET']:
            if not o.hide_render:
                hidden.append(o)
                o.hide_render = True
    fx = DATA.setdefault('fx', {})
    for key, fn, ortho, zc, kind, title, note in FX_PIECES:
        frame_seed(f'fx{key}')
        light()
        target = FXO + UP * (zc + 0.1 * ortho / 1.78)
        BACKDROP.location = target - FX_BACK * 25.0
        BACKDROP.rotation_euler = FX_BACK.to_track_quat('Z', 'Y').to_euler()
        BACKDROP.hide_render = False
        dims = fn()
        entry = shoot(f'fx_{key}', target + FX_BACK * 40.0, target, 50.0, CELL, ortho=ortho, alpha=True)
        entry.update(kind=kind, title=title, note=note, dims=dims)
        fx[key] = entry
        clear_shot()
    BACKDROP.hide_render = True
    for o in hidden:
        o.hide_render = False


# --- Her turnaround, bare after 33%, the stone up close ---

TURN_AT = (-1.0, 0.6)
TURN_CAM = ((-17.5, 3.8, 3.3), (-1.0, 0.55, 1.3), 52.0)


def facing(theta):
    to_cam = V(TURN_CAM[0], 0.0) - V(TURN_AT, 0.0)
    return V(TURN_AT, 0.0) + turned(to_cam, theta)


HERO_VIEWS = [
    ('three', -40.0, (1560, 878), 1, ('head', 'stone', 'rear'), 'Three-quarter'),
    ('front', 0.0, (772, 434), 1, ('head', 'stone'), 'Front'),
    ('side', -90.0, (772, 434), 1, ('head', 'stone', 'rear'), 'Side'),
    ('back', 180.0, (772, 434), 1, ('stone', 'rear'), 'Back'),
    ('top', None, (772, 434), 1, ('head', 'stone', 'rear'), 'Top'),
    ('bare', -70.0, (772, 434), 3, ('head', 'bare'), 'After 33%: bare'),
]
MARK_TEXT = {'head': ('crit', 'HEAD: CRIT'), 'stone': ('cover', 'HEADSTONE: ARMOUR'),
             'rear': ('crit', 'SOFT REAR: CRIT'), 'bare': ('crit', 'BARE ABDOMEN: ALL CRIT')}


def hero_marks(rec, keys):
    pts = {'head': lambda: head_pt(rec), 'stone': lambda: stone_pt(rec, 70.0), 'rear': lambda: rear_pt(rec),
           'bare': lambda: bare_pt(rec)}
    return [(MARK_TEXT[k][0], MARK_TEXT[k][1], pts[k]()) for k in keys]


def part_hero():
    hero = DATA.setdefault('hero', {})
    for key, theta, size, phase, keys, title in HERO_VIEWS:
        frame_seed(f'hero{key}')
        light()
        if key == 'top':
            rec = her(STANDT, TURN_AT, (TURN_AT[0], TURN_AT[1] + 10.0))
            mid = (head_pt(rec) + rear_pt(rec)) * 0.5
            cam, target, lens = (mid.x, mid.y, 26.0), (mid.x, mid.y, 0.0), 70.0
        else:
            rec = her(STANDT, TURN_AT, facing(theta), phase=phase)
            cam, target, lens = TURN_CAM
            if key == 'three':
                lens = 50.0
        entry = shoot(f'hero_{key}', cam, target, lens, size, hero_marks(rec, keys),
                      rot=(0.0, 0.0, 0.0) if key == 'top' else None)
        entry.update(title=title)
        hero[key] = entry
        clear_shot()
    # The stone up close: the roots binding it, a shot glancing off, the scars of earlier ones.
    frame_seed('heroclose')
    light()
    rec = her(STANDT, TURN_AT, facing(-40.0))
    s, n = stone_pt(rec, 58.0), stone_normal(rec)
    to_cam = (n * 0.75 + (V(TURN_CAM[0]) - s).normalized() * 0.65 + UP * 0.2).normalized()
    cam = s + to_cam * 3.9
    base, R = stone_frame()
    for k, (x, z, turn) in enumerate(((-18.0, 84.0, 0.3), (14.0, 40.0, -0.5), (-6.0, 22.0, 1.1))):
        p = cpt(rec, 'abdomen', base + R @ Vector((x, STONE_T / 2 + 0.6, z)))
        axis = cdir(rec, 'abdomen', R @ Vector((math.cos(turn), 0.0, math.sin(turn))))
        quad(f'Scar{k}', p, (0.2, 0.1), mat_decal('BW_StoneScar', 'StoneScar'), fwd=axis, normal=n)
    hit = cpt(rec, 'abdomen', base + R @ Vector((20.0, STONE_T / 2 + 0.6, 78.0)))
    spark_burst('Glance', hit, n, cam, 3, scale=1.1)
    root = cpt(rec, 'abdomen', base + R @ Vector((-12.0, STONE_T / 2 + 2.5, 40.0)))
    marks = [('cover', 'HEADSTONE: ARMOUR, SHOTS GLANCE', hit), ('cover', 'ROOTS BIND IT ON', root),
             ('crit', 'SOFT REAR: CRIT', rear_pt(rec))]
    entry = shoot('hero_close', cam, s - n * 0.15 + UP * 0.05, 50.0, (776, 878), marks)
    entry.update(title='The stone up close')
    hero['close'] = entry
    clear_shot()


# --- The spiderling digging out ---

LING_AT = (-6.0, -3.0)
LING_TO = (-12.0, -6.5)
LING_CAM = ((-8.7, -5.4, 1.2), (-5.95, -2.9, 0.32), 40.0)
LING_SHOTS = [('in', 0.6, 'In the mound: its legs break through the cracking top.'),
              ('half', 1.1, 'Half out: hauling itself over the rim, shedding mud.'),
              ('out', 1.5, 'Out: the game\'s spider at 0.45x, caked in grave mud.')]


def part_ling():
    ling = DATA.setdefault('ling', {})
    shots = []
    for key, t, caption in LING_SHOTS:
        frame_seed(f'ling{key}')
        light()
        at = V(LING_AT, 0.0)
        if key == 'in':
            mound('Mound', at, 1.0, 3)
            ling_out('Ling', at, LING_TO, 'in', 31)
            thrown('Spray', at + UP * 0.4, 0.1, 10, 5, size=0.035, speed=(0.8, 1.8), up=(1.0, 2.5), start_r=0.05)
        elif key == 'half':
            crater('Crater', at, 3)
            ling_out('Ling', at, LING_TO, 'half', 31)
            falling('Shed', at + UP * 0.6, 12, 0.6, 0.35, 0.03, 6, mat=mud())
            puffs('Puff', at, 0.5, 4, (0.15, 0.6), 0.6, puff(0.5), 7)
        else:
            crater('Crater', at, 3)
            run = at + flat_dir(at, LING_TO) * 0.9
            ling_out('Ling', run, LING_TO, 'out', 32)
            falling('Shed', run + UP * 0.45, 6, 0.4, 0.3, 0.03, 8, mat=mud())
            puffs('Kick', at, 0.4, 3, (0.05, 0.3), 0.5, puff(0.6, DUST), 9)
        entry = shoot(f'ling_{key}', *LING_CAM, (772, 560))
        entry.update(t=t, caption=caption)
        shots.append(entry)
        clear_shot()
    ling['shots'] = shots
    # Sizes of the spiderling at 0.45x: body length and leg span, from the posed mesh.
    rec = spider('Measure', STANDT, (0.0, -200.0), (0.0, -210.0), LING)
    ling['size'] = dims_cm([rec['obj']])
    clear_shot()


# --- Main ---

def write_data():
    path = os.path.join(OUT, 'BW.json')
    old = {}
    if os.path.exists(path):
        with open(path, encoding='utf-8') as f:
            old = json.load(f)
    for part, value in DATA.items():
        if part == 'moves':
            old.setdefault('moves', {}).update(value)
        else:
            old[part] = value
    old['order'] = list(STRIP_KEYS)
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(old, f, indent=1)
    log('wrote', path)


def main():
    global SUN, RIM, BACKDROP
    os.makedirs(OUT, exist_ok=True)
    t0 = time.time()
    paint_all()
    MATS['GMF_Dirt'] = lt.material('GroundDirt', name='GMF_Dirt', tint=0x8d7a66)     # fresh grave earth everywhere
    FC['FIG'] = G['mannequin']()
    SUN, RIM = FC['build_stage']()
    for o in bpy.data.objects:          # a plain stage: no coffins or fallen blocks in the way
        if o.name.startswith(('_Coffin', '_Block')) or o.name == '_RockG':
            o.hide_render = True
    bd = bpy.data.meshes.new('_Backdrop')
    bd.from_pydata([(-200, -200, 0), (200, -200, 0), (200, 200, 0), (-200, 200, 0)], [], [(0, 1, 2, 3)])
    bd.materials.append(mat_backdrop())
    BACKDROP = bpy.data.objects.new('_Backdrop', bd)
    SCENE.collection.objects.link(BACKDROP)
    BACKDROP.visible_shadow = False
    BACKDROP.hide_render = True
    strips = [k for k in STRIP_KEYS if k in ARGV]
    parts = [k for k in PART_KEYS if k in ARGV] + (['moves'] if strips and 'moves' not in ARGV else [])
    parts = parts or list(PART_KEYS)
    if 'moves' in parts:
        part_moves(strips or list(STRIP_KEYS))
    if 'fx' in parts:
        part_fx()
    if 'hero' in parts:
        part_hero()
    if 'ling' in parts:
        part_ling()
    DATA['textures'] = {k: dict(file=v, alpha=v[:-4] + '_A.png') for k, v in TEX.items()}     # shown as painted
    write_data()
    log(f'all done in {time.time() - t0:.0f} s')


main()
