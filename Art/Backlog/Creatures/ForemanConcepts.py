"""Concepts for the Foreman, Cyrus Teague (Docs/Areas/RansomsRest_Expansion.md, section 6, "The Foreman"), the
Providence's Legendary monster, for the user to pick from (2026-10-13). Concept art kept in Art/Backlog (see its README):
nothing in the game uses it. The pick becomes a game model on the Unpaid's rig at 1.5x (built here at the rig's size and
scaled), as Abel is at 1.3x: his long coat on the skirt bones Abel added (skirt_f, skirt_l, skirt_r), his bowler and its
lamp a static mesh on 'hat', the pick handle on a 'pick' bone under hand_r, the satchel of fuse sticks a static mesh on
spine_01 (a lit stick in his left fist for 'Fire in the hole' is the code's).

  A  The company man: a mouse-brown overcoat to the knee buttoned over a black waistcoat and watch chain, a stiff collar
     and tie; the pick handle carried like a club; the satchel at his hip. Upright, the company's man to the last.
  B  The shift boss: a long canvas duster thrown open, its sleeves shoved up over thick forearms, braces over a collarless
     shirt, a bandolier of fuse sticks across his chest and a coil of fuse on his shoulder, the satchel at his hip, the
     handle on his shoulder. A brute who drove the shift.
  C  The digger: what four years of digging at the fall have made of him: a dark oilskin slicker to the knee, buttoned
     to the throat, its hem torn away, hunched and reaching, one hand clawed for digging, the handle dragged in the other,
     his cuffs burnt black with powder.

Every option: the Unpaid's kin at 1.5 times, a hungry face with a heavy walrus moustache, a bowler with a carbide lamp
(the miners' own, half again as big as theirs with him), the gold coal of a Soulfed Legendary (CreatureRankSettings'
#FFCC00), the shroud below the coat.

Renders go to Saved/ArtPreviews/RansomsRest/Concepts/Foreman/: Foreman_front.png (the three from the front beside a
dead miner, a 1.8 m post and a 1.8 m figure), per option Foreman_A_golden.png (at the Providence's mouth with a dead
miner, three-quarter view) and Foreman_A_ghost.png (in the stope under cool ghost light, lighting a fuse: his coal open).

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Backlog\\Creatures\\ForemanConcepts.py -Preview
    ... -ScriptArgs A,golden          only some: options A B C; shots front golden ghost
    ... -ScriptArgs A,debug           quick checks into Intermediate/MinerConcepts ('fuse' for the fuse pose)
"""
import math
import os
import sys
import time

import bpy
import numpy as np
from mathutils import Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import MinerConcepts as mc  # noqa: E402  (the kit: materials, garments, hats, lamps, stages, the sheet)
import AbelConcepts as ab  # noqa: E402  (the grieving head's shape, the walrus moustache, the cropped hair)

uc, am, lt = mc.uc, mc.am, mc.lt
smoothstep, unit, rotation, wrap, spline = uc.smoothstep, uc.unit, uc.rotation, uc.wrap, uc.spline
ZPart, rot_x, rot_y, rot_z = mc.ZPart, mc.rot_x, mc.rot_y, mc.rot_z
g, surface, frame = mc.g, mc.surface, mc.frame
ARGV = mc.ARGV
OUT = os.path.join(mc.CONCEPTS, 'Foreman')
OPTIONS = ('A', 'B', 'C')
TITLES = {'A': 'The company man', 'B': 'The shift boss', 'C': 'The digger'}
SCALE = 1.5
SKIN, CLOTH1, CLOTH2, ACCENT = 0, 1, 2, 3
SIDES = (1.0, -1.0)
TORSO = am.TORSO['B']                       # broad and heavy, the rancher's frame
SHOULDER = {s: np.array([s * 0.178, -0.004, 1.444]) for s in SIDES}
PIVOT = np.array([0.0, -0.026, 1.622])
COAL_AT = (0.28, 1.33)
UPPER, FORE = 0.32, 0.29


def log(message):
    print(f'FOREMAN: {message}', flush=True)


# The face: the Unpaid's hunger on a heavy, jowled company face; deep sockets, the jaw hanging, a walrus moustache.
HEAD = dict(width=0.154, front=0.099, back=0.108, top=0.12, bottom=0.148, mouth=-0.076, brow=0.017, brow_shelf=0.007,
            socket=0.022, eyeball=0.0068, nose=0.036, cheek=0.007, zyg=0.006, hollow=0.011, nasolabial=0.0034,
            lips=0.003, lips_thin=0.0012, chin=0.01, jaw_angle=0.009, nape=0.2, jaw_taper=0.05, socket_dark=0.97,
            mouth_depth=0.042, temple=0.009, asym=0.1)
HINGE = np.array([0.0, 0.006, -0.03])


def head(fig, R, jaw, pivot, hair_zone=ACCENT):
    center = pivot + R @ np.array([0.0, -0.012, 0.09])
    face = ZPart('Head', SKIN)
    piece, cav, grid = ab.head_shape(HEAD, jaw, grief=0.15, stubble=0.28)
    face.add(uc.transform(piece, R, center), cavity=cav)
    for s in SIDES:
        ear = uc.ellipsoid((s * (HEAD['width'] / 2.0 - 0.003), 0.014, -0.006), (0.0095, 0.019, 0.029), segs=16, rings=10)
        face.add(uc.transform(ear, R, center))
    neck = [pivot + np.array([0.0, 0.012, -0.14]), pivot + np.array([0.0, 0.004, -0.07]), center + R @ np.array([0.0, 0.018, -0.07])]
    face.add(uc.tube(neck, [0.066, 0.06, 0.054], segs=20))
    zm = HEAD['mouth']
    face.add(uc.transform(uc.ellipsoid((0.0, -0.036, zm - 0.012), (0.034, 0.042, 0.028), segs=20, rings=12), R, center),
             cavity=1.0)
    for k in range(8):                          # the teeth, upper on the head and lower turned with the jaw
        a = (k - 3.5) * 0.17
        p = np.array([HEAD['width'] * 0.35 * math.sin(a), -HEAD['front'] * 0.85 * math.cos(a), zm + 0.004])
        face.add(uc.transform(uc.ellipsoid(p, (0.0045, 0.0038, 0.0084), segs=8, rings=6), R, center), cavity=0.15)
        q = np.array([HEAD['width'] * 0.32 * math.sin(a), -HEAD['front'] * 0.79 * math.cos(a), zm - 0.006])
        wq = float(smoothstep(zm + 0.003, zm - 0.012, q[2]))
        q = rotation((1.0, 0.0, 0.0), jaw * wq) @ (q - HINGE) + HINGE
        face.add(uc.transform(uc.ellipsoid(q, (0.0042, 0.0036, 0.0076), segs=8, rings=6), R, center), cavity=0.15)
    fig.add(face)
    (V, F), hcav = ab.hair_sheet(grid, 'short')
    fig.add(ZPart('HairCap', hair_zone).add((V @ R.T + center, F), cavity=hcav))
    (V, F), bcav = ab.beard_piece(grid, 'walrus')
    fig.add(ZPart('Moustache', hair_zone).add((V @ R.T + center, F), cavity=bcav))
    fig.marks['head'] = center
    return center


# --- His long coats: knee length, on Abel's skirt bones. Profiles in the frame of TORSO (the coats grow off them). ---

LOWER = [(0.36, 0.262, 0.216, 0.06, 2.0), (0.5, 0.25, 0.2, 0.045, 2.0), (0.65, 0.232, 0.178, 0.028, 2.1),
         (0.8, 0.21, 0.152, 0.012, 2.2), (0.92, 0.19, 0.134, 0.004, 2.2), (1.0, 0.178, 0.124, 0.0, 2.2),
         (1.06, 0.172, 0.12, -0.004, 2.2)]
COAT = LOWER + [r for r in TORSO if r[0] > 1.06]


def long_coat(fig, kind, zone=CLOTH2, collar_zone=None, seed=11):
    """kind 'over' (an overcoat buttoned to the chest, its skirts parting below the waist, a back vent), 'duster' (thrown
    open, flaring, a deep back vent) or 'slicker' (buttoned to the throat, its hem torn away and fading)."""
    rng = np.random.default_rng(seed)
    amps, phases = rng.uniform(0.5, 1.0, 5), rng.uniform(0.0, 2.0 * np.pi, 5)
    hem_z = {'over': 0.42, 'duster': 0.36, 'slicker': 0.44}[kind]
    depth = {'over': 0.014, 'duster': 0.022, 'slicker': 0.018}[kind]

    def folds(th, zz):
        f = sum(a * np.cos(k * th + p) for a, k, p in zip(amps, (6, 9, 13, 17, 23), phases)) / amps.sum()
        skirt = depth * smoothstep(1.0, 0.55, zz) * (f - 0.3 * np.abs(f))
        chest = 0.002 * np.cos(11 * th) * smoothstep(1.1, 1.3, zz)
        flare = (0.03 * smoothstep(0.95, 0.4, zz) * smoothstep(0.2, 0.9, np.abs(th))) if kind == 'duster' else 0.0
        return 0.02 + skirt + chest + flare

    def sd(th, zz):
        at = np.abs(th)
        ragged = 0.0
        if kind == 'slicker':
            ragged = 0.07 * (0.5 + 0.5 * np.sin(th * 7.0 + 1.0) * np.sin(th * 3.0)) + 0.03 * np.sin(th * 17.0)
        out = hem_z + ragged + 0.008 * np.sin(5 * th + 0.4) - zz
        if kind == 'over':
            front = np.where(zz > 1.36, 0.08 + 1.8 * (zz - 1.36), np.where(zz > 1.0, 0.045, 0.045 + 0.5 * (1.0 - zz)))
            vent = np.where(zz < 0.8, (0.012 + 0.07 * (0.8 - zz)) - np.abs(wrap(th - np.pi)), -1.0)
        elif kind == 'duster':
            front = 0.42 + 0.5 * smoothstep(1.3, 0.5, zz) - 0.14 * smoothstep(1.42, 1.53, zz)
            vent = np.where(zz < 0.9, (0.015 + 0.09 * (0.9 - zz)) - np.abs(wrap(th - np.pi)), -1.0)
        else:
            front = np.where(zz > 1.48, 0.06 + 1.5 * (zz - 1.48), 0.035)
            vent = np.where(zz < 0.7, (0.01 + 0.05 * (0.7 - zz)) - np.abs(wrap(th - np.pi)), -1.0)
        out = np.maximum(np.maximum(out, front - at), vent)
        out = np.maximum(out, mc.armhole(th, zz, 1.385, 0.26, 0.062))
        return np.maximum(out, zz - 1.548)
    (V, F), th, zz, edge = mc.shell(COAT, hem_z - 0.06, 1.548, sd, folds, rows=170, cols=240, hole=0.072, coal_at=COAL_AT)
    fade = 1.0
    if kind == 'slicker':
        fade = 1.0 - 0.75 * smoothstep(0.62, hem_z - 0.02, zz) * (0.6 + 0.4 * np.sin(th * 11.0))
    part = ZPart('Coat', zone).add((V, F), ember=mc.scorch(edge, V), cavity=mc.scorch_cavity(edge), fade=fade)
    if kind == 'over':
        for zb in (1.34, 1.25, 1.16, 1.07):
            p = surface(COAT, [zb], np.array([0.05]), grow=0.026)[0, 0]
            part.add(uc.ellipsoid(p, (0.0085, 0.0085, 0.005), np.array([[0, 0, 1.0], [1.0, 0, 0], [0, -1.0, 0]])),
                     cavity=0.35)
        for s in SIDES:                          # flap pockets at the hips
            t0, t1 = sorted((s * 0.62, s * 1.08))
            am.patch(part, COAT, t0, t1, 0.9, 0.94, grow=0.028, cavity_edge=0.6)
    elif kind == 'slicker':
        for zb in (1.44, 1.32, 1.2, 1.08, 0.96, 0.84, 0.72):
            p = surface(COAT, [zb], np.array([0.04]), grow=0.026)[0, 0]
            part.add(uc.ellipsoid(p, (0.009, 0.009, 0.005), np.array([[0, 0, 1.0], [1.0, 0, 0], [0, -1.0, 0]])),
                     cavity=0.5)
    else:
        for s in SIDES:
            t0, t1 = sorted((s * 0.75, s * 1.2))
            am.patch(part, COAT, t0, t1, 0.84, 0.98, grow=0.03, cavity_edge=0.45)
    fig.add(part, solidify=0.009)
    # The collar: an overcoat's velvet collar and lapels, the duster's turned down, the slicker's turned up.
    theta = np.linspace(-np.pi, np.pi, 96, endpoint=False)
    if kind == 'slicker':
        rows = [(1.53, 0.1, 0.0), (1.57, 0.096, 0.0), (1.615, 0.094, 0.0), (1.64, 0.1, 0.0)]
        gap = 0.3
    else:
        rows = [(1.555, 0.096, 0.0), (1.537, 0.112, 0.0), (1.505, 0.133, 0.012), (1.468, 0.152, 0.03)]
        gap = 0.5 if kind == 'over' else 0.44
    rings = np.array([np.stack([r0 * np.sin(theta), -0.006 - r0 * 0.96 * np.cos(theta),
                                z0 - drop * 2.2 * np.cos(theta).clip(0.0, None) ** 2], -1) for z0, r0, drop in rows])
    Vc, Fc, used = uc.cut(rings, np.broadcast_to((gap - np.abs(theta))[None, :], rings.shape[:2]) * 0.1)
    fig.add(ZPart('CoatCollar', collar_zone if collar_zone is not None else zone).add((Vc, Fc), cavity=0.12),
            solidify=0.009)
    if kind == 'over':
        lapels = ZPart('Lapels', zone)
        for s in SIDES:
            zs = np.linspace(1.36, 1.5, 12)
            path = [surface(COAT, [zr], np.array([s * (0.08 + 1.8 * (zr - 1.36) + 0.05)]), grow=0.03)[0, 0] for zr in zs]
            uc.ribbon(lapels, path, [(0.0, 0.006), (0.5, 0.034), (1.0, 0.026)], n=24, cols=4,
                      out=lambda C: mc.outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.1)
        fig.add(lapels, solidify=0.006)


def waistcoat(fig, zone=ACCENT):
    """A black waistcoat buttoned up under the overcoat's V, its points at the waist; the watch chain across it."""
    def folds(th, zz):
        return 0.012 + 0.002 * np.cos(9 * th) * smoothstep(1.2, 1.0, zz)

    def sd(th, zz):
        at = np.abs(th)
        hem = 0.98 - 0.04 * np.exp(-((at - 0.15) / 0.12) ** 2)
        out = np.maximum(hem - zz, np.where(zz > 1.3, (0.03 + 0.9 * (zz - 1.3)) - at, -1.0))
        out = np.maximum(out, mc.armhole(th, zz, 1.39, 0.36, 0.09))
        return np.maximum(out, zz - 1.5)
    (V, F), th, zz, edge = mc.shell(TORSO, 0.92, 1.5, sd, folds, hole=0.068, coal_at=COAL_AT)
    part = ZPart('Waistcoat', zone).add((V, F), ember=mc.scorch(edge, V), cavity=mc.scorch_cavity(edge))
    am.buttons(part, TORSO, [(0.02, z) for z in (1.27, 1.2, 1.13, 1.06, 0.99)], grow=0.02, size=0.006)
    fig.add(part, solidify=0.006)


def watch_chain(fig, zone=ACCENT):
    chain = ZPart('Chain', zone)
    c0 = surface(TORSO, [1.13], np.array([0.02]), 0.024)[0, 0]
    c1 = surface(TORSO, [1.08], np.array([0.62]), 0.024)[0, 0]
    mid = (c0 + c1) * 0.5 + np.array([0.0, -0.016, -0.05])
    pts = spline([c0, mid, c1], 26)
    for k in range(len(pts) - 1):
        d = unit(pts[k + 1] - pts[k])
        chain.add(uc.ellipsoid((pts[k] + pts[k + 1]) * 0.5, (0.004, 0.002, 0.002),
                               np.array([d, (0, 0, 1.0), np.cross(d, (0, 0, 1.0))]), 8, 5), cavity=0.15)
    fig.add(chain, material='satchel')


def tie(fig, zone=ACCENT):
    theta = np.linspace(0.0, 2.0 * np.pi, 72, endpoint=False)
    collar = np.array([np.stack([rr * np.sin(theta), -0.008 - rr * 0.92 * np.cos(theta), np.full_like(theta, zz)], -1)
                       for zz, rr in ((1.53, 0.068), (1.56, 0.066), (1.592, 0.064))])
    fig.add(ZPart('StiffCollar', CLOTH1).add(uc.closed_loft(collar)))
    t = ZPart('Tie', zone)
    t.add(uc.ellipsoid((0.0, -0.074, 1.535), (0.014, 0.01, 0.014)))
    t.add(uc.tube([(0.0, -0.075, 1.52), (0.003, -0.09, 1.44), (0.0, -0.1, 1.36)], [0.013, 0.02, 0.016], [0.004, 0.005, 0.005],
                  segs=12, up=(0.0, -1.0, 0.0)))
    fig.add(t)


def strap(fig, keys, grow, width=0.022, zone=SKIN):
    part = ZPart('Strap', zone)
    am.ribbon_on(part, COAT, keys, width, grow, n=90, curl=0.02)
    fig.add(part, solidify=0.005, material='satchel')
    return part


SHOULDER_STRAP = {1.0: [(1.08, 1.0), (0.6, 1.16), (0.0, 1.3), (-0.45, 1.42), (-0.95, 1.52), (-1.5708, 1.56),
                        (-2.3, 1.49), (-2.9, 1.32), (-3.45, 1.16), (-4.1, 1.04), (-4.68, 1.0)]}
SHOULDER_STRAP[-1.0] = [(-t, z) for t, z in SHOULDER_STRAP[1.0]]


def satchel(fig, side, z=1.0, theta=1.25, grow=0.055, sticks=5):
    """The powder satchel of canvas and leather on its strap, flap thrown back, the fuse sticks standing in it in their
    red paper, fuses curling out of their tops."""
    th = side * theta
    origin = surface(COAT, [z], np.array([th]), grow)[0, 0]
    n = uc.torso_normal(th)
    y = -n
    zz = np.array([0.0, 0.0, 1.0])
    x = np.cross(y, zz)
    M = frame(origin, x, y, zz)
    bag, paper, dark, brass = ZPart('Satchel', SKIN), ZPart('Sticks', CLOTH1), ZPart('Fuses', CLOTH2), ZPart('Buckles', ACCENT)
    a = np.linspace(0.0, 2.0 * np.pi, 48, endpoint=False)
    rings = []
    for k, zr in enumerate(np.linspace(-0.22, 0.0, 12)):
        w = 0.135 + 0.006 * math.sin(k)
        d = 0.05 - 0.01 * smoothstep(-0.05, 0.0, zr)
        c, s = np.cos(a), np.sin(a)
        e = 0.5
        rings.append(np.stack([w * np.sign(c) * np.abs(c) ** e, -0.052 + d * np.sign(s) * np.abs(s) ** e,
                               np.full_like(a, zr)], -1))
    V, F = uc.orient(uc.closed_loft(np.array(rings)))
    bag.add((V, F), cavity=0.12 * (np.abs(V[:, 0]) > 0.125))
    # The flap thrown back over the top edge, hanging down the back against him.
    q = np.linspace(0.0, 1.0, 10)[:, None]
    xs = np.linspace(-0.13, 0.13, 12)[None, :]
    flap = np.stack([xs + 0.0 * q, -0.01 + 0.012 * q + 0.0 * xs, 0.012 - 0.11 * q + 0.0 * xs], -1)
    bag.add((flap.reshape(-1, 3), [uc.grid_faces(10, 12, closed=False)]), cavity=0.1)
    for bx in (-0.07, 0.07):
        brass.add(uc.box_piece((bx, -0.104, -0.08), (0.012, 0.003, 0.009)), cavity=0.2)
        bag.add(uc.box_piece((bx, -0.103, -0.11), (0.011, 0.0025, 0.035)), cavity=0.3)
    rng = np.random.default_rng(3)
    tops = []
    for k in range(sticks):
        bx = (k - (sticks - 1) / 2) * 0.046 + rng.uniform(-0.006, 0.006)
        tilt = np.array([rng.uniform(-0.15, 0.15), rng.uniform(-0.15, 0.05), 1.0])
        d = unit(tilt)
        b = np.array([bx, -0.05, -0.12])
        top = b + d * (0.17 + rng.uniform(0.0, 0.04))
        paper.add(am.tube([b, top], 0.0165, segs=14, domes=1), cavity=0.05)
        tops.append(top)
        fz = [top, top + d * 0.025 + np.array([rng.uniform(-0.01, 0.01), -0.008, 0.0]),
              top + d * 0.04 + np.array([rng.uniform(-0.03, 0.03), -0.02, -0.01])]
        dark.add(am.tube(spline(fz, 8), 0.0026, segs=6, domes=1))
    for part in (bag, paper, dark, brass):
        part.V = [am.apply(M, V) for V in part.V]
        fig.add(part, material='satchel')
    return [am.apply(M, t[None])[0] for t in tops]


def bandolier(fig, keys, grow, count=6):
    """A strap across his chest with fuse sticks in its loops."""
    part = strap(fig, keys, grow, width=0.026, zone=SKIN)
    paper, dark = ZPart('BandolierSticks', CLOTH1), ZPart('BandolierFuses', CLOTH2)
    pts = [surface(COAT, [z], np.array([t]), grow + 0.02)[0, 0] for t, z in keys[1:6]]
    path = spline(pts, 60)
    for k in range(count):
        p = path[6 + k * 8]
        nxt = path[min(7 + k * 8, len(path) - 1)]
        along = unit(nxt - p)
        out = unit(np.array([p[0], p[1] + 0.01, 0.0]))
        d = unit(np.cross(along, out))
        if d[2] < 0:
            d = -d
        paper.add(am.tube([p - d * 0.085, p + d * 0.085], 0.0155, segs=12, domes=1), cavity=0.05)
        top = p + d * 0.085
        dark.add(am.tube(spline([top, top + d * 0.02 + out * 0.01, top + d * 0.03 + out * 0.03 - along * 0.01], 8), 0.0024,
                         segs=6, domes=1))
    fig.add(paper, material='satchel')
    fig.add(dark, material='satchel')


def fuse_coil(fig, s=1.0):
    """A coil of safety fuse hung over the shoulder."""
    coil = ZPart('FuseCoil', CLOTH2)
    S = SHOULDER[s] + np.array([s * 0.02, 0.0, 0.07])
    for k in range(4):
        c = S + np.array([s * (0.02 + 0.006 * k), 0.0, -0.13 - 0.004 * k])
        coil.add(uc.torus(c, (1.0, 0.04 * k, 0.0), 0.15 + 0.004 * k, 0.0055, segs=48, sides=8, squash=1.0), cavity=0.15)
    fig.add(coil, material='satchel')


# --- Hands, arms, props ---

def big_hand(fig, name, E, W, fwd, up, side, curl, spread=0.05, thumb=0.8, claw=0.2, bare=False, zone=SKIN):
    """A big gnarled hand on a heavy wrist (bare to the elbow when the sleeves are shoved up)."""
    dv = unit(W - E)
    L = np.linalg.norm(W - E)
    start = E + dv * (0.02 if bare else L * 0.55)
    limb = [uc.tube([start, start + (W - start) * 0.4, W - dv * 0.01], [0.046, 0.04, 0.03], [0.05, 0.044, 0.036], segs=22,
                    up=up)]
    limb += uc.hand(W, fwd, up, side, curl=curl, spread=spread, thumb=thumb, length=1.12, thin=1.1, claw=claw,
                    knuckle=1.45, scale=1.14)
    fig.add(ZPart(name, zone).add(uc.union(limb, 0.0026, smooth=3)))


FIST = (1.15, 1.2, 1.2, 1.15)


def grip_arm(fig, s, S, G, h, elbow_guess, pole, sleeve_kind, zone, bare=False):
    E, W, f, u = mc.grip(S, G, h, s, elbow_guess, pole, reach=0.098, drop=0.034)
    E = mc.elbow(S, W, pole, UPPER, FORE)
    mc.sleeve(fig, s, E, W, sleeve_kind, zone, 51 + (s > 0), shoulder=S, size=1.12)
    big_hand(fig, f'Hand{s:+.0f}', E, W, f, u, s, FIST, bare=bare)
    return E, W


def free_arm(fig, s, S, W, fwd, up, curl, pole, sleeve_kind, zone, bare=False, claw=0.2, thumb=0.4):
    E = mc.elbow(S, W, pole, UPPER, FORE)
    mc.sleeve(fig, s, E, W, sleeve_kind, zone, 51 + (s > 0), shoulder=S, size=1.12)
    big_hand(fig, f'Hand{s:+.0f}', E, W, unit(fwd), unit(mc.perp(up, fwd)), s, curl, claw=claw, thumb=thumb, bare=bare)


def lit_stick(fig, G, h):
    """A fuse stick in his fist, its fuse spitting sparks (the 'Fire in the hole' wind-up)."""
    h = unit(h)
    paper, dark = ZPart('LitStick', CLOTH1), ZPart('LitFuse', CLOTH2)
    paper.add(am.tube([G - h * 0.08, G + h * 0.12], 0.0165, segs=14, domes=1))
    top = G + h * 0.12
    side = unit(np.cross(h, (1.0, 0.0, 0.0)))
    tip = top + h * 0.035 + side * 0.03
    dark.add(am.tube(spline([top, top + h * 0.025 + side * 0.008, tip], 8), 0.0026, segs=6, domes=1))
    fig.add(paper, material='satchel')
    fig.add(dark, material='satchel')
    rng = np.random.default_rng(9)
    sparks = ZPart('Sparks', SKIN)
    for k in range(16):
        d = unit(rng.normal(size=3) + h * 0.8)
        L = rng.uniform(0.02, 0.07)
        sparks.add(am.tube([tip + d * 0.006, tip + d * L], [0.0022, 0.0008], segs=5, domes=1))
    fig.add(sparks, material='flame')
    fig.flames.append((tip, 0.05))


# --- The options ---

LOOKS = {'A': dict(body=(mc.SKIN_TINT, 0xd9d4c7, 0x5e5243, 0x2e2b2b), hat=(0x2f2b29, 0x2f2b29, 0xb79a62, 0x1f1d1c)),
         'B': dict(body=(mc.SKIN_TINT, 0xc4baa2, 0x9c8a66, 0x4a3c32), hat=(0x5a4a3c, 0x5a4a3c, 0xb79a62, 0x2e2622)),
         'C': dict(body=(mc.SKIN_TINT, 0x9aa0a0, 0x3e4a44, 0x5a4d40), hat=(0x34302d, 0x34302d, 0xb79a62, 0x24201e))}
SATCHEL_LOOK = (0x8f7f62, 0xa9533f, 0x3a3532, 0xc2a36b)      # canvas and leather, red paper, fuse, brass
HANDLE_LOOK = (0xd8d4cc, 0x9c8668, 0x8b9196, 0xc9cbc6)       # hickory
POSE = {'A': dict(hunch=5.0, lean=2.0), 'B': dict(hunch=7.0, lean=3.0), 'C': dict(hunch=20.0, lean=14.0)}
BUDGET = {'A': dict(body=11.9, hat=1.4, satchel=1.3, handle=0.3), 'B': dict(body=12.1, hat=1.4, satchel=1.9, handle=0.3),
          'C': dict(body=11.6, hat=1.4, satchel=1.3, handle=0.3)}


def foreman(key, pose='idle'):
    fig = mc.Ghost(key, pose)
    fig.scale = SCALE
    fig.rank = mc.SOULFED
    fig.looks = {'body': LOOKS[key]['body'], 'hat': LOOKS[key]['hat'], 'lamp': mc.LAMP_LOOK['carbide'],
                 'pick': HANDLE_LOOK, 'satchel': SATCHEL_LOOK}
    fuse = pose == 'fuse'
    mc.torso_core(fig, TORSO, COAL_AT, radius=0.052, z_lo=0.9)
    if key == 'A':
        mc.shirt(fig, CLOTH1, profile=TORSO, collar=False, placket=False, collar_y=-0.006, coal_at=COAL_AT, hole=0.06)
        tie(fig)
        waistcoat(fig)
        watch_chain(fig)
        long_coat(fig, 'over', collar_zone=ACCENT)
        satchel(fig, 1.0)
        strap(fig, SHOULDER_STRAP[1.0], 0.048)
        sleeves = ('coat', CLOTH2, False)
    elif key == 'B':
        mc.shirt(fig, CLOTH1, profile=TORSO, neck=0.07, buttons_at=(1.42, 1.48), collar=False, collar_y=-0.006,
                 coal_at=COAL_AT, hole=0.06)
        mc.braces(fig, ACCENT, width=0.02, grow=0.02, profile=TORSO, front_t=0.6)
        long_coat(fig, 'duster', seed=12)
        bandolier(fig, SHOULDER_STRAP[-1.0], 0.048)
        satchel(fig, -1.0, theta=1.3)
        fuse_coil(fig, 1.0)
        sleeves = ('rolled', CLOTH2, True)
    else:
        mc.shirt(fig, CLOTH1, profile=TORSO, collar=False, collar_y=-0.006, coal_at=COAL_AT, hole=0.06)
        long_coat(fig, 'slicker', seed=13)
        satchel(fig, 1.0, theta=1.35, z=0.96)
        strap(fig, SHOULDER_STRAP[1.0], 0.048)
        sleeves = ('coat', CLOTH2, False)
    p = POSE[key]
    warp_hunch = uc.lean(math.radians(p['hunch']), pivot_z=1.34, width=0.08)
    warp_lean = uc.lean(math.radians(p['lean']))

    def warp(V):
        return warp_lean(warp_hunch(np.atleast_2d(np.asarray(V, float))))
    fig.warp(warp)
    S = {s: warp(SHOULDER[s])[0] for s in SIDES}
    pivot = warp(PIVOT)[0]
    kind, zone, bare = sleeves
    # The arms in the posed frame.
    if key == 'A':
        G = np.array([-0.275, -0.15, 0.9])
        h = unit((0.0, -0.5, -0.86))
        grip_arm(fig, -1.0, S[-1.0], G, h, (-0.26, -0.02, 1.15), (-0.6, 0.6, 0.0), kind, zone, bare)
        mc.add_pick(fig, G, h, 0.12, (0.0, 0.0, 1.0), head=False)
    elif key == 'B':
        G = np.array([-0.155, -0.2, 1.27])
        h = unit(np.array([-0.195, -0.01, 1.54]) - G)
        grip_arm(fig, -1.0, S[-1.0], G, h, (-0.28, -0.09, 1.13), (-0.5, 0.2, -1.0), kind, zone, bare)
        mc.add_pick(fig, G, h, 0.13, (0.0, 0.0, 1.0), head=False)
    else:
        W = S[-1.0] + np.array([0.0, -0.4, -0.38])
        free_arm(fig, -1.0, S[-1.0], W, (0.05, -0.6, -0.8), (-0.2, -0.3, 1.0), (0.35, 0.45, 0.55, 0.65), (-0.8, 0.4, -0.2),
                 kind, zone, claw=0.45, thumb=0.3)
    if fuse:
        G = S[1.0] + np.array([-0.05, -0.36, 0.1])
        h = unit((0.15, -0.25, 0.95))
        grip_arm(fig, 1.0, S[1.0], G, h, S[1.0] + np.array([0.12, -0.12, -0.2]), (0.6, 0.0, -1.0), kind, zone, bare)
        lit_stick(fig, G, h)
    elif key == 'C':
        G = S[1.0] + np.array([0.03, -0.3, -0.42])
        h = unit((0.0, -0.45, 0.89))
        grip_arm(fig, 1.0, S[1.0], G, h, S[1.0] + np.array([0.08, -0.08, -0.3]), (0.7, 0.5, -0.3), kind, zone, bare)
        mc.add_pick(fig, G, h, 0.72, (0.0, 0.0, 1.0), head=False)
    else:
        W = S[1.0] + np.array([0.075, -0.115, -0.56])
        free_arm(fig, 1.0, S[1.0], W, (0.05, -0.4, -1.0), (1.0, -0.2, 0.1), (0.5, 0.62, 0.72, 0.8), (0.6, 0.6, 0.0),
                 kind, zone, bare)
    R = {'A': rot_z(-4.0) @ rot_x(-6.0), 'B': rot_z(6.0) @ rot_x(-8.0), 'C': rot_x(-26.0)}[key]
    if fuse:
        R = rot_z(10.0) @ R
    jaw = math.radians(16.0 if not fuse else 28.0)
    hz = ACCENT if key == 'A' else (CLOTH2 if key == 'C' else ACCENT)
    head(fig, R, jaw, pivot, hair_zone=hz)
    M = mc.add_hat(fig, 'bowler', R, pivot=pivot, tilt=(-3.0, 2.0) if key != 'B' else (-7.0, -6.0), seat=0.153)
    mc.add_lamp(fig, 'carbide', M)
    path = [(0.0, 0.0, 0.92), (0.0, 0.06, 0.6), (0.0, 0.2, 0.36), (0.0, 0.48, 0.2), (0.0, 0.85, 0.12), (0.0, 1.25, 0.08)]
    size = [(0.0, 0.17, 0.12), (0.25, 0.17, 0.13), (0.5, 0.12, 0.1), (0.75, 0.07, 0.06), (1.0, 0.03, 0.03)]
    fig.add(uc.shroud(ZPart('Shroud', SKIN), path, size, rows=64, cols=72, strips=10, split=0.12, ends=(0.74, 1.0),
                      spread=0.1, seed=5, fade_from=0.2, folds=0.34, narrow=0.55, wave=0.035), solidify=0.006)
    fig.add(ZPart('Coal', SKIN).add(uc.coal_piece(*fig.coal)), material='coal')
    return fig


_cache = {}


def figure(key, pose='idle'):
    if (key, pose) not in _cache:
        t0 = time.time()
        _cache[(key, pose)] = foreman(key, pose)
        log(f'{key} {pose}: {_cache[(key, pose)].triangles()} triangles (concept density), built in {time.time() - t0:.1f} s')
    return _cache[(key, pose)]


# --- Shots ---

def shot_front():
    """The three from the front at 1.5x, beside a dead miner (option A), a 1.8 m post and a 1.8 m figure."""
    uc.reset()
    where = uc.collection('Front')
    miner = mc.figure('A', 0)
    figs = [figure(k) for k in OPTIONS]
    items = [(-0.08, 0.08), (-0.25, 0.25), tuple(x for x in (mc.bounds_of(miner)[0][0], mc.bounds_of(miner)[1][0]))]
    items += [(mc.bounds_of(f)[0][0], mc.bounds_of(f)[1][0]) for f in figs]
    xs, total = uc.lineup(items, gap=0.4)
    uc.post(where, (xs[0], 0.0, 0.0))
    uc.label('1.8 m', (xs[0] + 0.1, -0.05, 1.78), 0.075, align='LEFT')
    mc.mannequin(where, (xs[1], 0.0, 0.0))
    mc.instantiate(miner, where, (xs[2], 0.0, 0.0))
    uc.label('a dead miner', (xs[2] + sum(items[2]) / 2, -0.7, 0.05), 0.1)
    for x, fig, lo_hi in zip(xs[3:], figs, items[3:]):
        mc.instantiate(fig, where, (x, 0.0, 0.0))
        uc.label(f'Foreman {fig.key}', (x + sum(lo_hi) / 2, -0.8, 0.05), 0.12)
    mc.studio(where)
    width = total + 0.6
    uc.camera((0.0, -40.0, 1.5 + 40.0 * math.tan(math.radians(6.0))), (0.0, 0.0, 1.5), ortho=width)
    mc.render(os.path.join(OUT, 'Foreman_front.png'), (2400, int(2400 * 3.3 / width)), 96)


def shot_golden(key):
    """At the Providence's mouth in the golden light, three-quarter view, a dead miner at his side for scale."""
    uc.reset()
    where = uc.collection('Golden')
    mc.portal_stage(where)
    mc.golden(where)
    mc.instantiate(figure(key), where, (0.0, 1.6, 0.0), -0.35)
    mc.instantiate(mc.figure('A', 1), where, (-1.5, 2.6, 0.0), -0.2)
    mc.mannequin(where, (1.35, 2.9, 0.0), -0.3)
    eye = Vector((2.4, -4.2, 1.7))
    mc.lens_shot(os.path.join(OUT, f'Foreman_{key}_golden.png'), eye, Vector((-0.1, 1.8, 1.6)), 30.0, (1600, 1100), 96,
                 focus=(eye - Vector((0.0, 1.6, 1.6))).length, fstop=5.6)


def shot_ghost(key):
    """In the stope under cool ghost light, lighting a fuse (his coal open), the fall behind him, two of the shift
    rising."""
    uc.reset()
    where = uc.collection('Ghost')
    mc.gallery_stage(where, length=16.0, width=8.0, height=5.0, rubble=True, seed=7)
    mc.ghost_light(where, fill=(2.0, -7.0, 3.6), fill_energy=600.0)
    f = figure(key, 'fuse')
    mc.instantiate(f, where, (0.2, 2.0, 0.0), 0.3, rank=(mc.SOULFED[0], mc.SOULFED[1] * 1.6), light=3.0)
    mc.instantiate(mc.figure('A', 2), where, (-2.0, 5.2, 0.0), 0.4, light=2.0)
    mc.instantiate(mc.figure('A', 1), where, (2.3, 6.0, 0.0), -0.1, rank=mc.RESTLESS, light=2.0)
    eye = Vector((1.6, -3.8, 1.5))
    mc.lens_shot(os.path.join(OUT, f'Foreman_{key}_ghost.png'), eye, Vector((0.0, 3.0, 2.0)), 28.0, (1600, 1100), 96,
                 exposure=0.4)


def shot_debug(key, pose='idle'):
    uc.reset()
    where = uc.collection('Debug')
    fig = figure(key, pose)
    mc.instantiate(fig, where, scale=1.0, flare=1.0)
    mc.studio(where, 0x9d9484)
    c = Vector((0.0, -0.1, 1.15))
    paths = []
    for name, d in (('front', (0.0, -1.0, 0.05)), ('three', (0.8, -1.0, 0.1)), ('side', (1.0, 0.0, 0.05)),
                    ('back', (0.3, 1.0, 0.1))):
        cam = uc.camera(c + Vector(d).normalized() * 3.8, c, lens=50.0)
        paths.append(os.path.join(mc.WORK, f'debug_F{key}_{pose}_{name}.png'))
        uc.render(paths[-1], (560, 900), samples=12)
        bpy.data.objects.remove(cam)
    uc.compose(paths, os.path.join(mc.WORK, f'debug_F{key}_{pose}.png'))
    head_at = Vector(fig.marks['head'])
    cam = uc.camera(head_at + Vector((0.45, -1.0, 0.1)).normalized() * 0.95, head_at + Vector((0.0, 0.0, 0.04)), lens=70.0)
    uc.render(os.path.join(mc.WORK, f'debug_F{key}_{pose}_head.png'), (700, 700), samples=16)
    bpy.data.objects.remove(cam)


def sheet_section(s):
    """The Foreman's part of the look sheet (MinerConcepts.sheet)."""
    s.heading('The Foreman, Cyrus Teague: pick A, B or C (Legendary, 1.5x)')
    s.image(os.path.join(OUT, 'Foreman_front.png'),
            'From the front beside a dead miner, a 1.8 m figure and post. Bowler and carbide lamp, gold Soulfed coal,\n'
            'the satchel of fuse sticks, the pick handle.')
    captions = {'A': 'Overcoat over waistcoat and watch chain, a stiff\ncollar; the handle carried like a club.',
                'B': 'Duster thrown open, sleeves shoved up, a bandolier\nof sticks, fuse on the shoulder, handle shouldered.',
                'C': 'An oilskin slicker torn at the hem, hunched from\ndigging, one hand clawed, powder-black cuffs.'}
    for k in OPTIONS:
        total, parts = mc.estimate(BUDGET[k])
        s.row([(os.path.join(OUT, f'Foreman_{k}_golden.png'), f'{k}  {TITLES[k]}   ~{total:.1f}k tris', captions[k]),
               (os.path.join(OUT, f'Foreman_{k}_ghost.png'), '',
                f'In the stope, cool ghost light, lighting a fuse:\nhis coal open. ({parts})')])


def main():
    keys = [a for a in ARGV if a in OPTIONS] or list(OPTIONS)
    if 'debug' in ARGV:
        poses = [p for p in ('idle', 'fuse') if p in ARGV] or ['idle']
        for key in keys:
            for pose in poses:
                shot_debug(key, pose)
        return
    shots = [a for a in ARGV if a in ('front', 'golden', 'ghost')]
    if not shots:
        if '--preview' not in ARGV and not lt.want_preview():
            for key in keys:
                figure(key)
            return
        shots = ['front', 'golden', 'ghost']
    if 'front' in shots:
        shot_front()
    for key in keys:
        if 'golden' in shots:
            shot_golden(key)
        if 'ghost' in shots:
            shot_ghost(key)


if __name__ == '__main__':
    main()
