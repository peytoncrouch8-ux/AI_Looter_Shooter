"""Hiram Coyle, the ore lift's dead engineman (AHiramCoyle; Docs/Areas/RansomsRest_Expansion.md, section 4, "Dead
Lift"), as the user approved him on 2026-10-13 (Art/Backlog/Creatures/LiftAndCellarConcepts.py): about sixty, square
and soot-creased, big grey mutton chops, kind. In the dead miners' costume set: the duck work coat open over a hickory
shirt, braces, a sweat rag at his neck, dark duck trousers to the knees; an engineer's cap with his goggles pushed up on
it. He sits on his stool by the cold boiler, leaning on his forearms on his knees, looking up at whoever's come.

A rig on the Unpaid's bones as Amos's is (Art/Models/Creatures/Amos.py, run here as a library: its living face, shirt,
trousers and shroud), at size 1. His rest pose is the game's idle, sat on his stool: the origin on the floor, the seat
of his trousers on the stool's top 56 cm up, the stool's middle 3 cm behind his origin; leaning forward, the forearms on
his knees, the hands hanging over them; the shroud falling from his knees like shins and trailing back under the stool.

Models (in /Game/Art/Creatures):
  SK_Hiram     about 11.7k triangles (the head about 3.1k), LODs at 50% and 25% (no Nanite); slots GhostHiram (M_Ghost: his
               ghost skin, hair and chops; the hickory shirt and sweat rag; the duck coat; the dark trousers and braces)
               and GhostCoal (shared; banked low, as Amos's).
  SM_HiramCap  the engineer's cap with the goggles on its front, on the 'hat' bone as SM_UnpaidHat is (its pivot the bone's
               head, modeled as it sits at rest); slot GhostHiramCap (the goggles' glass, the cap's cloth, brass, leather);
               SOCKET_Speaker at his mouth, facing his front: his speaker point.
Bones: Amos's (the Unpaid's names): pelvis, spine_01, spine_02, coal, neck, head, jaw, hat, upperarm/lowerarm/hand_l and
_r with thumb/index/middle/ring/pinky_01_l and _r, tail_01..05 (the shroud from his knees, tail_01 under the pelvis),
tail_l_01/02, tail_r_01/02, skirt_f_01 (the thighs, hips to knees) and skirt_f_02 (the knees and the shroud's top).
The pose table (Intermediate/HiramModel/Hiram_poses.json, Amos_poses.json's layout): idle (the rest), tube (leaned to the
speaking tube on his right, his right hand up to its mouthpiece), tube_left (the same to his left), talk (the idle, the
jaw open 8 degrees: it layers on any pose).

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Hiram.py -Preview
Previews go to Saved/ArtPreviews/RansomsRest/Hiram/.
"""
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_friends as lf
import looter_ghosts as lg
import looter_miners as mn
import looter_textures as lt

smoothstep, gauss, unit, rot_x, rot_y, rot_z, wrap = lg.smoothstep, lg.gauss, lg.unit, lg.rot_x, lg.rot_y, lg.rot_z, lg.wrap
SIDES, SUFFIX = lg.SIDES, lg.SUFFIX
SKIN, SHIRT, COAT_ZONE, DARK = 0, 1, 2, 3
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Hiram')
WORK = os.path.join(lt.REPO, 'Intermediate', 'HiramModel')

BODY_COLORS = ('#BAC4C6', '#8E99A8', '#7D7262', '#3C3734')     # ghost and hair; hickory shirt, rag; duck coat; trousers
CAP_COLORS = ('#C9D2CE', '#4C5361', '#A58E62', '#2C2A29')      # the goggles' glass, the cap's cloth, brass, leather
EMBER_COLOR, EMBER_ALPHA, EMBER_HEAT = 0x96421f, 0.35, -0.5   # his coal banked low, as Amos's (previews only)
TRIANGLES = dict(head=2500, shirt=600, coat=1400, sleeve=300, hand=700, trousers=1000)
TALK_JAW = 8.0

# Built in the upright frame Amos.py builds in (the pelvis 1 m up), then lowered by DROP onto his stool: there the seat
# of his trousers (0.8 m up in that frame) rests on the stool's top.
STOOL = 0.56
DROP = 0.8 - STOOL
STOOL_AT = np.array([0.0, 0.03])               # the stool's middle (x, y) under him

TORSO_B = [(0.90, 0.172, 0.118, 0.000, 2.2), (0.98, 0.168, 0.116, -0.004, 2.2), (1.06, 0.164, 0.114, -0.008, 2.2),
           (1.16, 0.168, 0.116, -0.010, 2.3), (1.26, 0.180, 0.122, -0.010, 2.4), (1.34, 0.192, 0.125, -0.010, 2.6),
           (1.40, 0.199, 0.119, -0.006, 2.8), (1.45, 0.197, 0.107, -0.002, 3.2), (1.475, 0.185, 0.099, 0.000, 3.4),
           (1.50, 0.156, 0.091, 0.000, 3.0), (1.52, 0.116, 0.081, -0.002, 2.6), (1.54, 0.079, 0.071, -0.004, 2.2)]
COAT_B = [(0.82, 0.208, 0.152, 0.012, 2.4), (0.90, 0.204, 0.148, 0.01, 2.4), (1.0, 0.2, 0.143, 0.004, 2.4),
          (1.1, 0.196, 0.137, 0.0, 2.4), (1.2, 0.198, 0.137, -0.004, 2.5), (1.3, 0.202, 0.137, -0.006, 2.6),
          (1.38, 0.207, 0.132, -0.004, 2.8), (1.44, 0.206, 0.122, -0.002, 3.2), (1.47, 0.195, 0.113, 0.0, 3.4),
          (1.5, 0.174, 0.103, 0.0, 3.0), (1.52, 0.131, 0.093, -0.002, 2.6), (1.545, 0.09, 0.083, -0.004, 2.2)]
# His face: tired and kind, square, heavy-lidded, a big nose; Amos's living-face machinery.
FACE = dict(width=0.152, front=0.099, back=0.108, top=0.12, bottom=0.136, mouth=-0.072, brow=0.0145, bags=0.0018, nose=0.03,
            cheek=0.0072, hollow=0.0016, jaw=0.0078, chin=0.0095, taper=0.07, naso=0.0024, smile=0.45, worry=0.2,
            squint=0.65, heavy=0.65, age=0.9, brow_hair=0.0052, droop=0.45, lips=0.85)
SHOULDER = {s: np.array([s * 0.178, -0.004, 1.444]) for s in SIDES}
PIVOT = np.array([0.0, -0.026, 1.622])
HIPS = {s: np.array([s * 0.09, 0.0, 0.91]) for s in SIDES}
KNEES = {s: np.array([s * 0.11, -0.43, 0.86]) for s in SIDES}       # sat: the thighs run forward to the knees
SKIRT_HINGE, SKIRT_KNEE, SKIRT_SHIN = np.array([0.0, -0.005, 0.91]), np.array([0.0, -0.44, 0.86]), np.array([0.0, -0.46, 0.6])
# The shroud from inside his knees, down like shins to the floor (0.24 m up in this frame) and back under the stool.
SHROUD_PATH = [(0.0, -0.4, 0.855), (0.0, -0.49, 0.74), (0.0, -0.47, 0.5), (0.0, -0.36, 0.33), (0.0, -0.06, 0.27),
               (0.0, 0.3, 0.27)]
SHROUD_SIZE = [(0.0, 0.17, 0.06), (0.2, 0.16, 0.08), (0.45, 0.12, 0.07), (0.7, 0.07, 0.048), (1.0, 0.026, 0.024)]
LEAN, HUNCH = 26.0, 10.0                       # he leans on his knees from the waist, his back rounded
HEAD_R = rot_z(-8.0) @ rot_x(-12.0)            # his head up to whoever's come, a little to his right


def log(message):
    print(f'HIRAM: {message}', flush=True)


A = lf.amos_library()
lf.configure(A, FACE=FACE, TORSO=TORSO_B, SHOULDER=SHOULDER, PIVOT=PIVOT, HIPS=HIPS, KNEES=KNEES, SKIRT_HINGE=SKIRT_HINGE,
             SKIRT_KNEE=SKIRT_KNEE, SKIRT_SHIN=SKIRT_SHIN, SHROUD_PATH=SHROUD_PATH, SHROUD_SIZE=SHROUD_SIZE,
             TRIANGLES=dict(A['TRIANGLES'], shirt=TRIANGLES['shirt']), TOP_RISE=0.04)
_lean, _hunch = lg.lean(math.radians(LEAN), 1.07, 0.05), lg.lean(math.radians(HUNCH), 1.34, 0.08)


def warp(V):
    """The upright frame to his rest: the back rounded at the shoulders, then the lean forward from the waist."""
    return _lean(_hunch(np.atleast_2d(np.asarray(V, float))))


def warped(p):
    return warp(p)[0]


def bent(part):
    part.V = warp(part.V)
    return part


# --- The clothes (built upright, weighed upright, then bent to his rest) ---

def braces():
    """Both braces up over his shoulders, crossed at the back, buttoned to his waistband."""
    parts = []
    surface = A['surface']
    for s in SIDES:
        keys = [(s * 0.62, 1.04), (s * 0.58, 1.17), (s * 0.53, 1.3), (s * 0.5, 1.41), (s * 0.78, 1.5), (s * 1.5708, 1.535),
                (s * 2.3, 1.5), (s * 2.75, 1.4), (s * 2.95, 1.26), (s * 2.8, 1.12), (s * 2.66, 1.04)]
        pts = [surface(TORSO_B, [z], np.array([t]), 0.02)[0, 0] for t, z in keys]
        piece = lg.ribbon(pts, 0.026, n=34, cols=3, out=lambda C: lg.outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.05)
        part = lg.Part('Brace', piece, DARK)
        parts.append(lg.torso_weights(part, part.V[:, 2], part.V[:, 0]))
    pieces = []
    for t in (-0.62, 0.62):
        p = surface(TORSO_B, [1.04], np.array([t]), 0.027)[0, 0]
        n = lg.outward(t)
        pieces.append(lg.ellipsoid(p, (0.007, 0.007, 0.0045), np.array([[0, 0, 1.0], np.cross(n, (0, 0, 1.0)), n]), 8, 4))
    parts.append(lg.Part('BraceButtons', lg.merge(pieces), DARK, cavity=0.4).weigh('pelvis', 1.0))
    return parts


def sweat_rag():
    """The sweat rag knotted round his neck, its ends hanging on his chest."""
    pieces = [lg.torus((0.0, -0.01, 1.548), (0.0, -0.25, 1.0), 0.068, 0.012, segs=16, sides=5, squash=0.8),
              lg.ellipsoid((0.0, -0.08, 1.53), (0.018, 0.013, 0.015), segs=10, rings=6)]
    for s in SIDES:
        tail = [(0.0, -0.085, 1.525), (s * 0.014, -0.095, 1.49), (s * 0.022, -0.1, 1.45)]
        pieces.append(lg.ribbon(tail, [(0.0, 0.016), (1.0, 0.024)], n=8, cols=3, out=(0.0, -1.0, 0.1), curl=0.15,
                                twist=0.1 * s))
    part = lg.Part('Rag', lg.merge(pieces), SHIRT, cavity=0.15)
    return [part.weigh('neck', 0.4).weigh('spine_02', 0.6)]


def coat_open(zz):
    return 0.42 + 0.2 * smoothstep(1.3, 0.85, zz) - 0.1 * smoothstep(1.42, 1.53, zz)


def coat_hem(th):
    """The hem: hip length at the back and sides, lifted at the front where it lies over his lap."""
    return 0.85 + 0.14 * smoothstep(1.7, 0.7, np.abs(th)) + 0.006 * np.sin(5 * th + 0.4)


def work_coat():
    """The duck work coat hanging open: boxy, stiff folds, burnt through where it laps over the coal; its collar turned
    down; patch pockets at the hips."""
    parts = []
    rng = np.random.default_rng(31)
    amps, phases = rng.uniform(0.5, 1.0, 4), rng.uniform(0.0, 2.0 * np.pi, 4)

    def folds(th, zz):
        f = sum(a * np.cos(k * th + p) for a, k, p in zip(amps, (5, 7, 11, 15), phases)) / amps.sum()
        return 0.008 + 0.012 * smoothstep(1.15, 0.85, zz) * (f - 0.3 * np.abs(f))

    def sd(th, zz):
        out = np.maximum(coat_hem(th) - zz, coat_open(zz) - np.abs(th))
        out = np.maximum(out, lg.armhole(th, zz, 1.385, 0.25, 0.058))
        return np.maximum(out, zz - 1.545)
    (V, F), th, zz, edge = lg.shell(COAT_B, 0.83, 1.545, sd, folds, rows=110, cols=180, hole=0.05, coal_at=A['COAL_AT'])
    ember = lg.scorch(edge, V, heat=0.3)
    cav = 0.5 * smoothstep(0.02, 0.0, edge)
    low = lg.reduce((V, F), TRIANGLES['coat'])
    near = lg.nearest(V, low[0])
    coat = lg.Part('Coat', low, COAT_ZONE, ember=ember[near], cavity=cav[near])
    parts.append(bent(lg.torso_weights(coat, low[0][:, 2], low[0][:, 0])))
    surface = A['surface']
    for s in SIDES:
        t0, t1 = sorted((s * 0.75, s * 1.15))
        ts, zs = np.linspace(t0, t1, 6), np.linspace(1.0, 1.1, 4)
        P = surface(COAT_B, zs, ts, 0.022)
        Tg, Zg = np.meshgrid(np.linspace(0, 1, 6), np.linspace(0, 1, 4))
        rim = smoothstep(0.25, 0.0, np.minimum(np.minimum(Tg, 1 - Tg), np.minimum(Zg, 1 - Zg)))
        part = lg.Part('Pocket', (P.reshape(-1, 3), [lg.grid_faces(4, 6, closed=False)]), COAT_ZONE, cavity=(0.5 * rim).ravel())
        parts.append(bent(lg.torso_weights(part, part.V[:, 2], part.V[:, 0], shoulders=False)))
    ang = np.linspace(-np.pi, np.pi, 40, endpoint=False)
    rows = [(1.552, 0.094, 0.0), (1.535, 0.11, 0.0), (1.505, 0.13, 0.012), (1.47, 0.148, 0.03)]
    rings = np.array([np.stack([r0 * np.sin(ang), -0.004 - r0 * 0.96 * np.cos(ang),
                                z0 - drop * 2.2 * np.cos(ang).clip(0.0, None) ** 2], -1) for z0, r0, drop in rows])
    Vc, Fc, used = lg.cut(rings, np.broadcast_to((0.44 - np.abs(ang))[None, :], rings.shape[:2]) * 0.1)
    part = lg.Part('CoatCollar', (Vc, Fc), COAT_ZONE, cavity=0.12)
    parts.append(bent(part.weigh('spine_02', 0.75).weigh('neck', 0.25)))
    return parts


def trousers():
    """Dark duck trousers sat: the hips and seat round the torso's bottom, the thighs run forward to the knees, each leg
    ending at the knee in a ragged hem. The waist on the pelvis, the thighs on skirt_f_01, the knees on skirt_f_02."""
    surface = A['surface']
    theta = np.linspace(0.0, 2.0 * np.pi, 72, endpoint=False)
    z = np.linspace(0.93, 1.05, 8)
    pieces = [lg.closed_loft(surface(TORSO_B, z, theta, grow=0.018)), lg.ellipsoid((0.0, 0.01, 0.9), (0.18, 0.12, 0.1))]
    for s in SIDES:
        H, K = HIPS[s] + np.array([0.0, 0.0, 0.01]), KNEES[s] + np.array([0.0, -0.05, 0.0])
        mid = (H + K) * 0.5 + np.array([s * 0.008, 0.0, 0.012])
        pieces.append(lg.tube(lg.spline([H, mid, K], 12), np.linspace(0.09, 0.064, 12), segs=24))
    V, F = lg.union(pieces, 0.006, smooth=4)
    side, t = A['leg_param'](V)
    K_y = np.where(side > 0, KNEES[1.0][1], KNEES[-1.0][1])
    K_z = np.where(side > 0, KNEES[1.0][2], KNEES[-1.0][2])
    ang = np.arctan2(V[:, 2] - K_z, V[:, 0] - np.where(side > 0, KNEES[1.0][0], KNEES[-1.0][0]))
    line = (1.0 - 0.07 * (0.5 + 0.5 * np.sin(5.0 * ang + side)) - 0.05 * np.sin(11.0 * ang + 2.0 * side) ** 2
            - 0.03 * np.sin(23.0 * ang) ** 2)
    below = (t > line) & (V[:, 1] < -0.3)
    F = [f[~below[f].any(1)] for f in F]
    V, F = lg.clean((V, F))
    low = lg.reduce((V, F), TRIANGLES['trousers'])
    Vl = low[0]
    side, t = A['leg_param'](Vl)
    cav = 0.22 * smoothstep(0.9, 0.99, t) * (Vl[:, 1] < -0.3)
    part = lg.Part('Trousers', low, DARK, cavity=cav)
    leg = smoothstep(0.1, 0.35, t) * (1.0 - smoothstep(0.95, 1.06, Vl[:, 2]))
    knee = smoothstep(0.82, 0.97, t) * leg
    upper = lg.torso_weights(lg.Part('_', (Vl, []), 0), np.maximum(Vl[:, 2], 0.9), Vl[:, 0], shoulders=False, neck=False)
    for bone, w in upper.weights.items():
        part.weigh(bone, w * (1.0 - leg))
    part.weigh('skirt_f_01', leg - knee)
    part.weigh('skirt_f_02', knee)
    return [part]


def amos_part(p, zone=None):
    """An Amos.py part as a looter_ghosts part."""
    q = lg.Part(p.name, (p.V, p.F), p.zones if zone is None else zone, slot=p.slot, cavity=p.cavity, ember=p.ember,
                fade=p.fade, strip_uv=p.strip_uv)
    q.weights = dict(p.weights)
    return q


# --- Arms: forearms on his knees, the big hands hanging loose over them ---

def rest_arms():
    arms = {}
    for s in SIDES:
        S = warped(SHOULDER[s])
        arms[s] = lf.rest_arm(S, np.array([s * 0.13, -0.445, 0.975]), (-s * 0.12, -0.35, -0.93), (s * 0.15, -0.95, 0.25),
                              (s * 0.7, 0.2, -0.8))
    return arms


ARMS = rest_arms()
LOOSE = (0.45, 0.55, 0.65, 0.72)


# --- The rig ---

def materials():
    look = dict(lg.GHOST_LOOK, EmberStrength=0.8)
    body = lg.ghost_slot('GhostHiram', BODY_COLORS, **look)
    cap = lg.ghost_slot('GhostHiramCap', CAP_COLORS, **dict(lg.PROP_LOOK, RimStrength=1.0, RimPower=5.0))
    return dict(body=body, coal=lg.coal_slot(), cap=cap)


def build_rig(mats):
    # Amos's shirt; under the rag and the coat his throat, the breast pocket and its stalk don't show.
    shirt = [amos_part(p) for p in A['build_shirt']() if p.name not in ('Stalk', 'Chest', 'Pocket')]
    for p in shirt:
        bent(p)
    parts = shirt + [bent(p) for p in braces() + sweat_rag()] + work_coat()
    trouser_parts = trousers()
    parts += trouser_parts
    fingers = {}
    for k, s in enumerate(SIDES):
        arm_parts, lines = lf.build_arm(A, s, ARMS[s], LOOSE, spread=0.06, thumb=0.3, sleeve='coat', sleeve_zone=COAT_ZONE,
                                        radii=(0.074, 0.066, 0.058), sleeve_tris=TRIANGLES['sleeve'],
                                        hand_tris=TRIANGLES['hand'], hand_scale=1.1, seed=41 + k)
        parts += arm_parts
        fingers[s] = lines
    neck_base = [warped((0.0, PIVOT[1] + 0.01, 1.47)), warped((0.0, PIVOT[1] - 0.002, 1.55))]
    head_parts, head = lf.build_head(A, warped(PIVOT), HEAD_R, neck_base, TRIANGLES['head'], beard='chops', hair='short',
                                     hair_zone=SHIRT, seat=0.15)
    parts += head_parts
    shroud_parts, tail = A['build_shroud'](trouser_parts[0])
    parts += [amos_part(p) for p in shroud_parts]
    # The bones, laid on the rest the parts were built in.
    center, nc, pc = A['coal_center']()
    specs = [('pelvis', (0.0, 0.005, 1.0), (0.0, 0.0, 1.12), None),
             ('spine_01', (0.0, 0.0, 1.12), warped((0.0, -0.005, 1.27)), 'pelvis'),
             ('spine_02', warped((0.0, -0.005, 1.27)), warped((0.0, -0.01, 1.48)), 'spine_01'),
             ('coal', warped(center), warped(center + nc * 0.04), 'spine_02')]
    specs += lf.head_bones(head, warped((0.0, -0.02, 1.5)))
    specs += lf.arm_bones(ARMS, fingers)
    specs += lf.tail_bones(tail)
    specs += [('skirt_f_01', SKIRT_HINGE, SKIRT_KNEE, 'pelvis'), ('skirt_f_02', SKIRT_KNEE, SKIRT_SHIN, 'skirt_f_01')]
    # Down onto the stool.
    lower = np.array([0.0, 0.0, -DROP])
    for p in parts:
        p.V = p.V + lower
    specs = [(n, np.asarray(h, float) + lower, np.asarray(t, float) + lower, par) for n, h, t, par in specs]
    for key in ('pivot', 'center', 'hat', 'hinge'):
        head[key] = head[key] + lower
    for s in SIDES:
        for key in ('S', 'E', 'W'):
            ARMS[s][key] = ARMS[s][key] + lower
    rig = lf.make_rig(specs)
    body, arrays, layout = lg.skinned(rig, 'Hiram', parts, [mats['body'], mats['coal']])
    lg.triangle_report(log, 'Hiram', body, parts)
    head_tris = sum(p.triangles() for p in parts if p.name in ('Head', 'Ear', 'Mouth', 'Teeth'))
    log(f'  the head: {head_tris} triangles')
    return rig, body, head, arrays


def hit_zones(rig, V, faces, W, names):
    zones = lg.HitZones(A['HULL_BONES'], tuple(n for n in A['NO_HULL'] if n != 'fork'), A['COAL_REACH'])
    zones.build(rig, V, faces, W, names, log)
    return zones


# --- His cap ---

def cap_parts():
    """The engineer's cap in hat space (origin at the band's middle, front -Y): a soft crown standing up from the band
    and bellying over it, pleated into its flat top, a short stiff peak; his driving goggles pushed up on its front, their
    strap round the crown. Zones: 0 glass, 1 the cap's cloth, 2 brass, 3 leather."""
    cols = 24
    th = np.linspace(0.0, 2.0 * np.pi, cols, endpoint=False)
    zc = [0.0, 0.02, 0.045, 0.065, 0.078, 0.084]
    rc = [0.09, 0.093, 0.099, 0.101, 0.096, 0.08]
    rings = np.array([np.stack([rr * np.sin(th), -rr * np.cos(th) * 1.1, np.full_like(th, zz)], -1) for zz, rr in zip(zc, rc)])
    rings[..., 2] += 0.004 * np.sin(th * 12.0)[None, :] * (np.array(zc)[:, None] > 0.06)
    n = len(zc) * cols
    pleat = 0.15 * (rings.reshape(-1, 3)[:, 2] > 0.07) * (0.5 + 0.5 * np.sin(np.tile(th, len(zc)) * 12.0))
    crown = lg.Part('Crown', (np.vstack([rings.reshape(-1, 3), [[0.0, 0.0, 0.082]]]),
                              [lg.grid_faces(len(zc), cols), lg.fan(n, np.arange(cols) + n - cols, top=True)]), 1,
                    cavity=np.concatenate([pleat, [0.1]]))
    band = np.array([np.stack([0.0915 * np.sin(th), -0.0915 * np.cos(th) * 1.1, np.full_like(th, z)], -1)
                     for z in (0.0, 0.009, 0.018)])
    parts = [crown, lg.Part('Band', (band.reshape(-1, 3), [lg.grid_faces(3, cols)]), 3, cavity=0.15)]
    a = np.linspace(-1.1, 1.1, 14)
    q = np.linspace(0.0, 1.0, 3)[:, None]
    r = 0.09 + 0.052 * q * np.cos(a * 0.9) ** 0.7
    top = np.stack([r * np.sin(a), -r * np.cos(a) * 1.1, 0.002 - 0.024 * q ** 1.3 + 0.0 * a], -1)
    under = top - np.array([0.0, 0.0, 0.005])
    V = np.vstack([top.reshape(-1, 3), under.reshape(-1, 3)])
    F = [lg.grid_faces(3, 14, closed=False)[:, ::-1], lg.grid_faces(3, 14, closed=False) + 42]
    edge = np.arange(13)
    F.append(np.stack([28 + edge, 28 + edge + 1, 42 + 28 + edge + 1, 42 + 28 + edge], 1)[:, ::-1])
    parts.append(lg.Part('Peak', (V, F), 3, cavity=np.concatenate([np.full(42, 0.15), np.full(42, 0.4)])))
    for s in SIDES:
        c = np.array([s * 0.036, -0.107, 0.05])
        d = unit(np.array([s * 0.25, -1.0, 0.25]))
        parts.append(lg.Part('GoggleCup', lg.tube([c - d * 0.012, c + d * 0.01], [0.021, 0.019], segs=12, domes=1), 2,
                             cavity=0.15))
        xl = unit(np.cross(d, (0, 0, 1.0)))
        lens = lg.ellipsoid(c + d * 0.011, (0.016, 0.016, 0.003), np.array([xl, np.cross(d, xl), d]), 12, 4)
        parts.append(lg.Part('Lens', lens, 0))
    parts.append(lg.Part('Bridge', lg.tube([(-0.012, -0.112, 0.052), (0.0, -0.115, 0.054), (0.012, -0.112, 0.052)], 0.004,
                                           segs=6, domes=1), 2, cavity=0.2))
    strap = np.array([np.stack([0.099 * np.sin(th), -0.099 * np.cos(th) * 1.1, np.full_like(th, z)], -1)
                      for z in (0.038, 0.049, 0.06)])
    parts.append(lg.Part('Strap', (strap.reshape(-1, 3), [lg.grid_faces(3, cols)]), 3, cavity=0.2))
    return parts


CAP_TILT = (6.0, -3.0)                         # pulled down a little in front, a touch to his left


def build_cap(mats, head):
    Rh = head['R'] @ rot_x(CAP_TILT[0]) @ rot_y(CAP_TILT[1])
    parts = cap_parts()
    for p in parts:
        p.V = p.V @ Rh.T + head['hat']
    obj = lg.static_prop('HiramCap', parts, [mats['cap']], head['hat'])
    mouth = head['center'] + head['R'] @ np.array([0.0, -0.1, A['MOUTH_Z']])
    lf.speaker(obj, head['R'], mouth)
    log(f'SM_HiramCap: {lg.triangles_of(obj)} triangles; SOCKET_Speaker {tuple(np.round(lg.to_ue(mouth - head["hat"]), 1))} '
        f'cm from its pivot (in the hat bone\'s rest axes: turn it as the bone is turned)')
    return obj


# --- Build ---

def build():
    mats = materials()
    rig, body, head, (V, faces, colors, W, names) = build_rig(mats)
    cap = build_cap(mats, head)
    hit_zones(rig, V, faces, W, names)
    return rig, body, head, cap, colors


RIG, BODY, HEAD, CAP, RAW_COLORS = build()
POSER = lg.Poser(RIG, ARMS, props={'hat': (CAP, HEAD['hat'])})


def turn(forward=0.0, right=0.0, lean=0.0):
    """DeadFriendPoses::Turn in Blender's axes: Yaw * Pitch * Roll (degrees: nod forward, turn to his right, tip his top
    to his right)."""
    return rot_z(-right) @ rot_x(forward) @ rot_y(-lean)


def tube(side):
    """Leaned to the speaking tube at his side (+1 his right): turned and tipped toward it, his ear and mouth at it, his
    near hand up to its mouthpiece; the far forearm still on his knee."""
    own = {'spine_01': turn(4.0, 18.0 * side, 8.0 * side), 'spine_02': turn(2.0, 10.0 * side, 5.0 * side),
           'neck': turn(-2.0, 10.0 * side, 4.0 * side), 'head': turn(-4.0, 14.0 * side, 8.0 * side)}
    near = -1.0 if side > 0 else 1.0
    arms = {near: dict(upper=mn.ue_dir(0.15, 0.9 * side, -0.25), fore=mn.ue_dir(0.25, -0.05 * side, 0.95), wrist=-10.0)}
    return dict(own=own, arms=arms)


POSES = {'idle': {}, 'tube': tube(1.0), 'tube_left': tube(-1.0), 'talk': dict(own={'jaw': rot_x(TALK_JAW)})}
HEADER = dict(units='Unreal component space at size 1: cm; x forward, y right, z up',
              quaternions='FQuat (X, Y, Z, W); turn = the whole turn from rest; own = the turn under the parent\'s '
                          '(Turned = Above * Own); in a pose, a bone not listed stays at rest under its parent',
              rest=f'the rest pose is the idle, sat on his stool: the actor\'s origin on the floor, the stool\'s top '
                   f'{100 * STOOL:.0f} cm up, its middle at component ({-100 * STOOL_AT[1]:.0f}, {-100 * STOOL_AT[0]:.0f}); '
                   f'the forearms on his knees; the shroud from the knees down like shins and back under the stool',
              tube='leaned to the speaking tube on his right (tube_left: on his left): the spine, chest, neck and head '
                   'turned and tipped toward it, the near hand up to its mouthpiece; the far forearm stays on his knee',
              talk=f'the idle with the jaw opened {TALK_JAW:.0f} degrees (PitchBy on jaw): it layers on any pose',
              speaker='SOCKET_Speaker is on SM_HiramCap (the cap rides the head)')
TABLE = POSER.table(POSES, os.path.join(WORK, 'Hiram_poses.json'), HEADER, log)


# --- Previews (Saved/ArtPreviews/RansomsRest/Hiram) ---

def want_preview():
    return '--preview' in ARGV or lt.want_preview()


def figure_copy(where, M, pose=None):
    """A copy of him (no armature) and his cap, posed, standing at M."""
    if pose:
        POSER.apply(POSES[pose])
        body = mn.posed_copy(BODY, where)
        cap_place = Matrix(CAP.matrix_world)
        mn.rest_pose(RIG)
        POSER.carry()
    else:
        body = mn.dup(BODY, where)
        body.modifiers.clear()
        cap_place = Matrix.Translation(Vector(CAP.location))
    body.matrix_world = M
    mn.preview_look(body, RAW_COLORS, rank=EMBER_COLOR, rank_alpha=EMBER_ALPHA)
    cap = mn.place_copy(CAP, where, M, cap_place)
    mn.preview_look(cap)
    return body, cap


def hide_sources(hidden=True):
    for obj in (BODY, CAP):
        obj.hide_render = hidden
    for c in RIG.children:
        if c.name.startswith(('UCX_', 'USP_')):
            c.hide_render = hidden


def clear(where):
    for o in list(where.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.collections.remove(where)


def stool(mc, where, at):
    dark = lt.material('WoodPlanks', name='StoolWood', tint=0x857563)
    sx, sy = at
    V, F = mc.lathe([(STOOL - 0.045, 0.0), (STOOL - 0.045, 0.17), (STOOL, 0.17), (STOOL, 0.0)], (sx, sy, 0.0), (0, 0, 1), 32)
    mc.uc.mesh_object('Seat', V, F, dark, where, uvs=mc.box_uv(V))
    for k in range(3):
        a = k * 2.0 * np.pi / 3 + 0.3
        foot = np.array([sx + 0.24 * math.cos(a), sy + 0.24 * math.sin(a), 0.0])
        mc.timber(where, foot, (sx + 0.1 * math.cos(a), sy + 0.1 * math.sin(a), STOOL - 0.04), (0.04, 0.04), dark, 'Leg')


def lift_kit():
    mn.kit()
    import LiftAndCellarConcepts as lc
    return lc


def shot_turn(mc, path):
    """Front, side and back at rest on stools, beside a 1.8 m post and a 1.8 m figure; and the tube pose."""
    where = bpy.data.collections.new('Turn')
    bpy.context.scene.collection.children.link(where)
    mc.uc.post(where, (-1.9, 0.0, 0.0))
    mc.mannequin(where, (-1.45, 0.3, 0.0), 0.0)
    for k, (t, pose) in enumerate(((0.0, None), (90.0, None), (180.0, None), (0.0, 'tube'))):
        x = -0.6 + 0.95 * k
        M = mn.figure_matrix((x, 0.0, 0.0), t)
        figure_copy(where, M, pose)
        stool(mc, where, tuple((M @ Vector((STOOL_AT[0], STOOL_AT[1], 0.0)))[:2]))
    mc.studio(where)
    cam = mc.uc.camera((0.75, -40.0, 0.95 + 40.0 * math.tan(math.radians(5.0))), (0.75, 0.0, 0.95), ortho=5.0)
    mc.render(path, (2400, 1000), 96)
    bpy.data.objects.remove(cam)
    clear(where)


def shot_winch(mc, path, mode):
    """On his stool in the winch house, the sun in through the open landing (golden), or by night (ghost)."""
    lc = lift_kit()
    where = bpy.data.collections.new('Winch')
    bpy.context.scene.collection.children.link(where)
    sx, sy = lc.winch_house(where)
    turn_ = -30.0
    R = Matrix.Rotation(math.radians(turn_), 4, 'Z')
    origin = Vector((sx, sy, 0.0)) - R @ Vector((STOOL_AT[0], STOOL_AT[1], 0.0))
    toward = lc.am.bearing(247.5, 15.0)
    if mode == 'golden':
        lc.golden_through(where, toward)
        exposure = 0.0
    else:
        lc.moonlight(where, toward, strength=0.9, world=0.55)
        mc.area_light(where, (-2.6, -2.2, 2.0), (0.2, 0.6, 1.0), 2.5, 45.0)
        exposure = 0.6
    figure_copy(where, Matrix.Translation(origin) @ R)
    eye = Vector((-1.75, -2.55, 1.3))
    mc.lens_shot(path, eye, Vector((0.3, 0.55, 0.95)), 30.0, (1600, 1100), 96, exposure=exposure)
    clear(where)


def shot_face(mc, path):
    """His face close, at rest and talking."""
    where = bpy.data.collections.new('Face')
    bpy.context.scene.collection.children.link(where)
    for k, pose in enumerate((None, 'talk')):
        figure_copy(where, mn.figure_matrix((0.55 * k, 0.0, 0.0), 0.0), pose)
    mc.studio(where)
    c = HEAD['center']
    eye = Vector((0.275 + 0.1, c[1] - 1.6, c[2] + 0.1))
    mc.lens_shot(path, eye, Vector((0.275, c[1], c[2] - 0.02)), 70.0, (1600, 1000), 96)
    clear(where)


SHOTS = {'turn': ('Hiram_turn.png', shot_turn), 'golden': ('Hiram_golden.png', lambda mc, p: shot_winch(mc, p, 'golden')),
         'ghost': ('Hiram_ghost.png', lambda mc, p: shot_winch(mc, p, 'ghost')), 'face': ('Hiram_face.png', shot_face)}


def previews():
    mc = mn.kit()
    os.makedirs(OUT, exist_ok=True)
    hide_sources(True)
    wanted = [k for k in SHOTS if k in ARGV] or list(SHOTS)
    for key in wanted:
        name, shot = SHOTS[key]
        shot(mc, os.path.join(OUT, name))
    hide_sources(False)


if want_preview():
    previews()
