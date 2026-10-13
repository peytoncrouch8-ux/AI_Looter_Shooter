"""Thaddeus Bright, Tilly's dead father (AThaddeusBright; Docs/Areas/RansomsRest_Expansion.md, section 4, "Knocking
Below"), the undertaker of Bright & Daughter, as the user approved him on 2026-10-13
(Art/Backlog/Creatures/LiftAndCellarConcepts.py): long and thin and stooped, bald under a fringe of grey, wire spectacles
low on his nose and a carpenter's pencil behind his ear; a collarless shirt, its sleeves rolled and held with garters; a
black waistcoat; a long leather apron to below the knee. At his trestles in the cold room he holds his cloth tape out
between his hands, measuring a coffin that isn't there.

A rig on the Unpaid's bones as Amos's is (Art/Models/Creatures/Amos.py, run here as a library: its living face, shirt and
shroud), at size 1. His rest pose is the game's idle, the measure: stood at his work, stooped over it from the waist and
the shoulders, his head bowed to it; his hands a coffin's width apart, 98 cm up and 42 cm before him, the tape stretched
between his fingers and thumbs, its tail hanging from his right hand, its leather case from his left.

Models (in /Game/Art/Creatures):
  SK_Thaddeus            about 10.8k triangles (the head about 3.1k), LODs at 50% and 25% (no Nanite); slots
                         GhostThaddeus (M_Ghost: his ghost skin; the shirt, his grey fringe and the tape; the black
                         waistcoat and garters; the leather apron and the tape's case) and GhostCoal (shared; banked low).
                         The apron and the tape are his own skin (the apron's bib bends with his chest, its skirt rides
                         skirt_f_01/02; the tape stretches between his hands), so SM_ThaddeusApron and SM_ThaddeusTape
                         aren't made.
  SM_ThaddeusSpectacles  his wire spectacles and the pencil behind his right ear, on the 'hat' bone as SM_UnpaidHat is
                         (its pivot the bone's head, modeled as it sits at rest); slot GhostThaddeusSpectacles (glass,
                         wire, the frames' brass, the pencil); SOCKET_Speaker at his mouth, facing his front: his speaker
                         point (the code's hat part, its HatPath).
Bones: Amos's (the Unpaid's names): pelvis, spine_01, spine_02, coal, neck, head, jaw, hat, upperarm/lowerarm/hand_l and
_r with thumb/index/middle/ring/pinky_01_l and _r, tail_01..05 (the shroud from his waist), tail_l_01/02, tail_r_01/02,
skirt_f_01 and skirt_f_02 (the apron's skirt: the thighs, then the knees to its hem).
The pose table (Intermediate/ThaddeusModel/Thaddeus_poses.json, Amos_poses.json's layout): idle (the rest: the measure at
full stretch), together (the measure's start, his hands together), mark (the tape held, his right hand lifted, the wrist
flicked: a pencil's tick), knock_up and knock_down (his right fist raised, and brought down on the board; the left hand on
the tape), look (straightened from his work to whoever he talks to, the tape slack in his hands), talk (the idle, the jaw
open 8 degrees: it layers on any pose).

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Thaddeus.py -Preview
Previews go to Saved/ArtPreviews/RansomsRest/Thaddeus/.
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
SKIN, SHIRT, BLACK, LEATHER = 0, 1, 2, 3
ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'RansomsRest', 'Thaddeus')
WORK = os.path.join(lt.REPO, 'Intermediate', 'ThaddeusModel')

BODY_COLORS = ('#BAC4C6', '#D6D1C4', '#3E3C3F', '#8B7356')     # ghost; shirt, fringe, tape; waistcoat, garters; leather
SPECS_COLORS = ('#D9E2E2', '#8F8F86', '#9A8A6A', '#6E5E44')    # the lenses, wire, the frames' brass, the pencil
EMBER_COLOR, EMBER_ALPHA, EMBER_HEAT = 0x96421f, 0.35, -0.5   # his coal banked low, as Amos's (previews only)
TRIANGLES = dict(head=2500, shirt=450, waistcoat=650, sleeve=260, hand=760)
TALK_JAW = 8.0
ARM_LENGTHS = (0.3, 0.275)                     # a long man's arms
HAND_SCALE = 1.08

FACE = dict(width=0.14, front=0.097, back=0.106, top=0.124, bottom=0.138, mouth=-0.073, brow=0.012, bags=0.0022, nose=0.034,
            cheek=0.0058, hollow=0.0042, jaw=0.0045, chin=0.008, taper=0.1, naso=0.0026, smile=0.2, worry=0.55,
            squint=0.35, heavy=0.72, age=1.0, brow_hair=0.0036, droop=0.6, lips=0.65)
SKIRT_HINGE, SKIRT_KNEE, SKIRT_SHIN = np.array([0.0, -0.1, 0.95]), np.array([0.0, -0.13, 0.66]), np.array([0.0, -0.14, 0.46])
# The shroud from his waist, under the waistcoat and the apron, trailing back behind him.
SHROUD_PATH = [(0.0, 0.01, 1.0), (0.0, 0.05, 0.72), (0.0, 0.17, 0.5), (0.0, 0.42, 0.36), (0.0, 0.75, 0.28), (0.0, 1.1, 0.24)]
SHROUD_SIZE = [(0.0, 0.14, 0.085), (0.2, 0.14, 0.09), (0.45, 0.11, 0.08), (0.7, 0.065, 0.05), (1.0, 0.026, 0.024)]
STOOP, HUNCH = 14.0, 12.0                      # bent over his work from the waist; his long back rounded
HEAD_R = rot_z(6.0) @ rot_x(28.0)              # bowed to the tape, a little to his left
APRON_HEM = 0.46


def log(message):
    print(f'THADDEUS: {message}', flush=True)


A = lf.amos_library()
lf.configure(A, FACE=FACE, SKIRT_HINGE=SKIRT_HINGE, SKIRT_KNEE=SKIRT_KNEE, SKIRT_SHIN=SKIRT_SHIN, SHROUD_PATH=SHROUD_PATH,
             SHROUD_SIZE=SHROUD_SIZE, FADE_HEM=0.8, FADE_DECAY=0.4, TRIANGLES=dict(A['TRIANGLES'], shirt=TRIANGLES['shirt']))
TORSO, SHOULDER, PIVOT, COAL_AT = A['TORSO'], A['SHOULDER'], A['PIVOT'], A['COAL_AT']
surface = A['surface']
_stoop, _hunch = lg.lean(math.radians(STOOP), 1.1, 0.06), lg.lean(math.radians(HUNCH), 1.34, 0.08)


def warp(V):
    """The upright frame to his rest: the long back rounded, then stooped over his work from the waist."""
    return _stoop(_hunch(np.atleast_2d(np.asarray(V, float))))


def warped(p):
    return warp(p)[0]


def bent(part):
    part.V = warp(part.V)
    return part


def amos_part(p):
    q = lg.Part(p.name, (p.V, p.F), p.zones, slot=p.slot, cavity=p.cavity, ember=p.ember, fade=p.fade, strip_uv=p.strip_uv)
    q.weights = dict(p.weights)
    return q


# --- The clothes (built upright, weighed upright, then bent to his rest) ---

def waistcoat():
    """The black waistcoat buttoned to the chest, its V open over the shirt, burnt through at the coal; its back left out
    under the apron's strap is kept (he turns from his work)."""
    def folds(th, zz):
        # Clear of the shirt's blouse over the waistband.
        return 0.013 + 0.022 * smoothstep(1.24, 1.04, zz) + 0.002 * np.cos(9 * th) * smoothstep(1.2, 1.0, zz)

    def sd(th, zz):
        at = np.abs(th)
        hem = 0.97 - 0.04 * np.exp(-((at - 0.15) / 0.12) ** 2)
        out = np.maximum(hem - zz, np.where(zz > 1.28, (0.035 + 0.85 * (zz - 1.28)) - at, -1.0))
        out = np.maximum(out, lg.armhole(th, zz, 1.39, 0.36, 0.09))
        return np.maximum(out, zz - 1.5)
    (V, F), th, zz, edge = lg.shell(TORSO, 0.92, 1.5, sd, folds, rows=70, cols=140, hole=A['HOLE_RADIUS'] + 0.004,
                                    coal_at=COAL_AT)
    ember = lg.scorch(edge, V, heat=0.3)
    cav = 0.5 * smoothstep(0.02, 0.0, edge)
    low = lg.reduce((V, F), TRIANGLES['waistcoat'])
    near = lg.nearest(V, low[0])
    # Its buttons are under the apron's bib.
    return [bent(lg.torso_weights(lg.Part('Waistcoat', low, BLACK, ember=ember[near], cavity=cav[near]), low[0][:, 2],
                                  low[0][:, 0], shoulders=False))]


def apron_weights(part, V):
    """The bib on his chest by height (the spine's), the skirt from the waist down on the front chain: the thighs on
    skirt_f_01, from the knees to the hem on skirt_f_02."""
    z = V[:, 2]
    below = smoothstep(1.02, 0.94, z)
    upper = lg.torso_weights(lg.Part('_', (V, []), 0), np.maximum(z, 1.0), V[:, 0], shoulders=False)
    for bone, w in upper.weights.items():
        part.weigh(bone, w * (1.0 - below))
    thigh = smoothstep(0.95, 0.82, z)
    knee = smoothstep(0.72, 0.6, z)
    part.weigh('pelvis', below * (1.0 - thigh))
    part.weigh('skirt_f_01', below * thigh * (1.0 - knee))
    part.weigh('skirt_f_02', below * thigh * knee)
    return part


def apron():
    """The long leather apron: a bib over his chest hung from a strap round his neck, tied round his waist, falling
    straight from the waist to below the knee; worn shiny at the front, a pocket on the bib."""
    zs = np.concatenate([np.linspace(APRON_HEM, 1.15, 13), np.linspace(1.17, 1.37, 11)])      # finer over the chest
    us = np.linspace(-1.0, 1.0, 13)
    Z, U = np.meshgrid(zs, us, indexing='ij')
    w = np.interp(Z, [APRON_HEM, 0.7, 1.0, 1.08, 1.2, 1.37], [0.215, 0.205, 0.19, 0.15, 0.115, 0.105])
    X = U * w
    front = np.array([surface(TORSO, [min(max(z, 0.95), 1.4)], np.array([0.0]))[0, 0] for z in zs])
    y_front = np.interp(Z, zs, front[:, 1]) - 0.03 - 0.016 * smoothstep(1.28, 1.06, Z)      # clear of the waistcoat
    Y = y_front + np.where(Z > 1.0, 1.6, 1.0) * X ** 2 - 0.012 * smoothstep(1.0, 0.6, Z)
    Y = Y + 0.006 * np.sin(U * 9.0 + 1.3) * smoothstep(1.0, 0.65, Z) + 0.004 * np.sin(U * 15.0 + Z * 6.0) * smoothstep(0.95, 0.6, Z)
    P = np.stack([X, Y, Z], -1)
    edge = np.minimum(1.0 - np.abs(U), (Z - APRON_HEM) / 0.1)
    rows, cols = P.shape[:2]
    # The coal has burnt through the bib where it lies over it: a ragged hole, its edge scorched.
    center = A['coal_center']()[0]
    ang = np.arctan2(Z - center[2], X - center[0])
    rh = 0.05 * (1.0 + 0.14 * np.sin(ang * 5 + 1.0) + 0.09 * np.sin(ang * 11.0))
    gap = np.hypot(X - center[0], Z - center[2]) - rh
    # A thin solid: the front, the back a few millimeters behind it, the outer edges closing them.
    Vf, (Ff,), used = lg.cut(P, -gap, closed=False)
    Vb, (Fb,), _ = lg.cut(P + np.array([0.0, 0.004, 0.0]), -gap, closed=False)
    n = len(Vf)
    remap = -np.ones(rows * cols, dtype=np.int64)
    remap[used] = np.arange(n)
    ring = np.concatenate([np.arange(cols), (np.arange(1, rows) * cols) + cols - 1, (rows - 1) * cols + np.arange(cols - 2, -1, -1),
                           np.arange(rows - 2, 0, -1) * cols])
    ring = remap[ring]
    V = np.vstack([Vf, Vb])
    F = [Ff, Fb[:, ::-1] + n, np.stack([ring, np.roll(ring, -1), np.roll(ring, -1) + n, ring + n], 1)]
    hole = gap.ravel()[used]
    ember = np.concatenate([lg.scorch(hole, Vf, heat=0.3)] * 2)
    cav = np.concatenate([np.maximum(0.4 * smoothstep(0.06, 0.0, edge.ravel()[used]), 0.5 * smoothstep(0.02, 0.0, hole)),
                          np.full(n, 0.3)])
    shine = np.concatenate([(-0.1 * gauss(U, 0.5) * smoothstep(0.6, 0.9, Z)).ravel()[used], np.zeros(n)])
    parts = [apron_weights(lg.Part('Apron', (V, F), LEATHER, cavity=np.clip(cav + shine, 0, 1), ember=ember), V)]
    # The pocket on the bib, the neck strap and the waist ties with their knot behind.
    pk = (np.abs(U) < 0.42) & (Z > 1.1) & (Z < 1.24)
    Vp, Fp, used = lg.cut(P + np.array([0.0, -0.005, 0.0]), np.where(pk, -1.0, 1.0), closed=False)
    parts.append(apron_weights(lg.Part('ApronPocket', (Vp, Fp), LEATHER, cavity=0.3), Vp))
    pieces = []
    for s in SIDES:
        top = P[-1, 0 if s < 0 else -1]
        strap = [top, top + np.array([s * 0.005, 0.02, 0.06]), np.array([s * 0.058, 0.0, 1.535]),
                 np.array([s * 0.04, 0.05, 1.56]), np.array([0.0, 0.07, 1.555])]
        pieces.append(lg.ribbon(strap, 0.009, n=14, cols=3,
                                out=lambda C: unit(np.column_stack([C[:, 0], C[:, 1] + 0.02, 0.3 + 0.0 * C[:, 0]])), curl=0.05))
    strap = lg.Part('ApronStrap', lg.merge(pieces), LEATHER, cavity=0.2)
    parts.append(lg.torso_weights(strap, strap.V[:, 2], strap.V[:, 0], shoulders=False))
    pieces = []
    for s in SIDES:
        ties = [surface(TORSO, [1.02], np.array([t]), 0.041)[0, 0] for t in np.linspace(s * 1.6, s * 3.0, 6)]
        pieces.append(lg.ribbon(ties, 0.009, n=12, cols=3, out=lambda C: lg.outward(np.arctan2(C[:, 0], -C[:, 1])), curl=0.05))
    knot = surface(TORSO, [1.02], np.array([np.pi]), 0.05)[0, 0]
    pieces.append(lg.ellipsoid(knot, (0.018, 0.012, 0.012), segs=10, rings=6))
    for s in SIDES:
        pieces.append(lg.ribbon([knot, knot + np.array([s * 0.02, 0.01, -0.06]), knot + np.array([s * 0.03, 0.012, -0.14])],
                                [(0.0, 0.009), (1.0, 0.007)], n=8, cols=3, out=(0.0, 1.0, 0.0), curl=0.05))
    ties = lg.Part('ApronTies', lg.merge(pieces), LEATHER, cavity=0.2)
    parts.append(ties.weigh('pelvis', 1.0))
    return [bent(p) for p in parts]


# --- Arms: sleeves rolled and gartered, the tape stretched between his fingers and thumbs ---

PINCH = (0.95, 1.05, 1.1, 1.05)


def rest_arms():
    arms = {}
    for s in SIDES:
        S = warped(SHOULDER[s])
        arm = lf.rest_arm(S, np.array([s * 0.25, -0.42, 0.98]), (-s * 0.42, -0.85, -0.3), (s * 0.15, 0.1, 1.0),
                          (s * 0.8, 0.35, -0.5), ARM_LENGTHS)
        arm['G'] = lg.grip_of(arm['W'], arm['f'], arm['u'], HAND_SCALE)
        arm['grip_scale'] = HAND_SCALE
        arms[s] = arm
    return arms


ARMS = rest_arms()


def tape():
    """His cloth tape: stretched from his left fingers to his right with a little sag, its inch marks; its tail hanging
    from his right hand; the leather case hanging from his left. Skinned to his hands, the stretch shared between them."""
    a, b = ARMS[1.0]['G'], ARMS[-1.0]['G']
    mid = (a + b) / 2 + np.array([0.0, -0.01, -0.02])
    pts = lg.spline([a, mid, b], 30)
    piece = lg.ribbon(pts, 0.008, n=30, cols=3, out=(0.0, 0.1, 1.0), curl=0.02)
    V = piece[0]
    along = np.clip(((V - a) @ (b - a)) / np.dot(b - a, b - a), 0.0, 1.0)
    marks = 0.45 * (np.sin(np.linalg.norm(V - a, axis=1) * 2.0 * np.pi / 0.0254) > 0.75)
    run = lg.Part('Tape', piece, SHIRT, cavity=marks)
    share = smoothstep(0.0, 1.0, along)
    run.weigh('hand_l', 1.0 - share).weigh('hand_r', share)
    tail = [b, b + np.array([-0.02, -0.01, -0.1]), b + np.array([0.0, 0.02, -0.22]), b + np.array([0.03, 0.01, -0.3])]
    piece = lg.ribbon(lg.spline(tail, 16), 0.008, n=16, cols=3, out=(0.0, -1.0, 0.0), curl=0.02, twist=0.3)
    marks = 0.45 * (np.sin(np.linalg.norm(piece[0] - b, axis=1) * 2.0 * np.pi / 0.0254) > 0.75)
    tail = lg.Part('TapeTail', piece, SHIRT, cavity=marks).weigh('hand_r', 1.0)
    c = a + np.array([0.0, 0.0, -0.06])
    case = lg.merge([lg.ellipsoid(c, (0.032, 0.011, 0.032), segs=14, rings=7),
                     lg.torus(a + np.array([0.0, 0.0, -0.022]), (0.0, 1.0, 0.0), 0.011, 0.002, segs=10, sides=4)])
    case = lg.Part('TapeCase', case, LEATHER, cavity=0.2).weigh('hand_l', 1.0)
    return [run, tail, case]


# --- The rig ---

def materials():
    body = lg.ghost_slot('GhostThaddeus', BODY_COLORS, **dict(lg.GHOST_LOOK, EmberStrength=0.8))
    specs = lg.ghost_slot('GhostThaddeusSpectacles', SPECS_COLORS, **dict(lg.PROP_LOOK, RimStrength=1.0, RimPower=5.0))
    return dict(body=body, coal=lg.coal_slot(), specs=specs)


def build_rig(mats):
    # Amos's shirt; under the waistcoat and the bib its pocket, stalk and buttons don't show.
    shirt = [amos_part(p) for p in A['build_shirt']() if p.name not in ('Stalk', 'Pocket', 'Buttons')]
    parts = [bent(p) for p in shirt] + waistcoat() + apron()
    fingers = {}
    for k, s in enumerate(SIDES):
        arm_parts, lines = lf.build_arm(A, s, ARMS[s], PINCH, spread=0.03, thumb=0.75, sleeve='rolled', sleeve_zone=SHIRT,
                                        sleeve_tris=TRIANGLES['sleeve'], hand_tris=TRIANGLES['hand'], hand_scale=HAND_SCALE,
                                        seed=31 + k, garter=BLACK)
        parts += arm_parts
        fingers[s] = lines
    parts += tape()
    neck_base = [warped((0.0, PIVOT[1] + 0.01, 1.47)), warped((0.0, PIVOT[1] - 0.002, 1.55))]
    head_parts, head = lf.build_head(A, warped(PIVOT), HEAD_R, neck_base, TRIANGLES['head'], beard='stubble', hair='thin',
                                     hair_zone=SHIRT, seat=0.13)
    parts += head_parts
    # The shroud's top is skinned as his waist is: a ring at the waist on the pelvis stands in for Amos's trousers.
    th = np.linspace(0.0, 2.0 * np.pi, 24, endpoint=False)
    waist = lg.Part('_', (np.vstack([surface(TORSO, [z], th)[0] for z in (0.9, 1.0, 1.08)]), []), 0).weigh('pelvis', 1.0)
    shroud_parts, tail = A['build_shroud'](waist)
    parts += [amos_part(p) for p in shroud_parts]
    center, nc, pc = A['coal_center']()
    specs = [('pelvis', (0.0, 0.005, 1.0), (0.0, 0.0, 1.12), None),
             ('spine_01', (0.0, 0.0, 1.12), warped((0.0, -0.005, 1.27)), 'pelvis'),
             ('spine_02', warped((0.0, -0.005, 1.27)), warped((0.0, -0.01, 1.48)), 'spine_01'),
             ('coal', warped(center), warped(center + nc * 0.04), 'spine_02')]
    specs += lf.head_bones(head, warped((0.0, -0.02, 1.5)))
    specs += lf.arm_bones(ARMS, fingers)
    specs += lf.tail_bones(tail)
    specs += [('skirt_f_01', SKIRT_HINGE, SKIRT_KNEE, 'pelvis'), ('skirt_f_02', SKIRT_KNEE, SKIRT_SHIN, 'skirt_f_01')]
    rig = lf.make_rig(specs)
    body, arrays, layout = lg.skinned(rig, 'Thaddeus', parts, [mats['body'], mats['coal']])
    lg.triangle_report(log, 'Thaddeus', body, parts)
    head_tris = sum(p.triangles() for p in parts if p.name in ('Head', 'Ear', 'Mouth', 'Teeth'))
    log(f'  the head: {head_tris} triangles')
    return rig, body, head, arrays


def hit_zones(rig, V, faces, W, names):
    zones = lg.HitZones(A['HULL_BONES'], tuple(n for n in A['NO_HULL'] if n != 'fork'), A['COAL_REACH'])
    zones.build(rig, V, faces, W, names, log)
    return zones


# --- His spectacles and pencil (on the 'hat' bone) ---

def spectacles_parts(head):
    """In rig space at rest: the round wire spectacles low on his nose, their temples back over his ears; the carpenter's
    pencil behind his right ear. Zones: 0 glass, 2 the frames, 3 the pencil."""
    R, center = head['R'], head['center']

    def place(V):
        return np.asarray(V, float) @ R.T + center
    parts = []
    for s in SIDES:
        c = np.array([s * 0.031, -0.106, -0.014])
        V, F = lg.torus(c, (0.0, -1.0, 0.0), 0.016, 0.0013, segs=16, sides=3)
        V = c + (V - c) * np.array([1.0, 1.0, 0.8])
        parts.append(lg.Part('Rim', (place(V), F), 2, cavity=0.2))
        V, F = lg.ellipsoid(c, (0.0158, 0.0012, 0.0126), segs=12, rings=3)
        parts.append(lg.Part('Lens', (place(V), F), 0))
        temple = [c + np.array([s * 0.016, 0.0, 0.002]), np.array([s * 0.066, -0.06, 0.004]), np.array([s * 0.071, 0.0, 0.0]),
                  np.array([s * 0.068, 0.016, -0.02])]
        V, F = lg.tube(lg.spline(temple, 8), 0.0012, segs=3, domes=1)
        parts.append(lg.Part('Temple', (place(V), F), 2))
    V, F = lg.tube(lg.spline([(-0.015, -0.108, -0.012), (0.0, -0.112, -0.006), (0.015, -0.108, -0.012)], 6), 0.0013, segs=4,
                   domes=1)
    parts.append(lg.Part('Bridge', (place(V), F), 2))
    V, F = lg.tube([np.array([-0.078, -0.02, 0.02]), np.array([-0.081, 0.04, -0.012])], 0.0042, segs=6, domes=1, pointed=0.5)
    parts.append(lg.Part('Pencil', (place(V), F), 3, cavity=0.1))
    return parts


def build_spectacles(mats, head):
    obj = lg.static_prop('ThaddeusSpectacles', spectacles_parts(head), [mats['specs']], head['hat'])
    mouth = head['center'] + head['R'] @ np.array([0.0, -0.1, A['MOUTH_Z']])
    lf.speaker(obj, head['R'], mouth)
    log(f'SM_ThaddeusSpectacles: {lg.triangles_of(obj)} triangles; SOCKET_Speaker '
        f'{tuple(np.round(lg.to_ue(mouth - head["hat"]), 1))} cm from its pivot')
    return obj


# --- Build ---

def build():
    mats = materials()
    rig, body, head, (V, faces, colors, W, names) = build_rig(mats)
    specs = build_spectacles(mats, head)
    hit_zones(rig, V, faces, W, names)
    return rig, body, head, specs, colors


RIG, BODY, HEAD, SPECS, RAW_COLORS = build()
POSER = lg.Poser(RIG, ARMS, props={'hat': (SPECS, HEAD['hat'])})


def grip_at(side, shift, f=None, u=None, pole=None):
    """An arm to its rest grip moved by shift (m), its hand turned to f, u (its rest frame unless given)."""
    a = ARMS[side]
    return dict(grip=a['G'] + np.asarray(shift, float), f=a['f'] if f is None else f, u=a['u'] if u is None else u,
                pole=(side * 0.8, 0.35, -0.5) if pole is None else pole)


def mark():
    """The tape held at its stretch, his right hand lifted off it a little, the wrist flicked down: a pencil's tick."""
    f = lg.rotation((1.0, 0.0, 0.0), math.radians(25.0)) @ ARMS[-1.0]['f']
    u = lg.rotation((1.0, 0.0, 0.0), math.radians(25.0)) @ ARMS[-1.0]['u']
    return dict(arms={-1.0: grip_at(-1.0, (0.02, -0.02, 0.09), f, u)})


def knock(up):
    """His right fist raised over the board (up), or brought down on it; the left hand still on the tape."""
    fore = mn.ue_dir(0.35, 0.3, 0.95) if up else mn.ue_dir(0.85, 0.05, -0.52)
    return dict(arms={-1.0: dict(upper=mn.ue_dir(0.55, 0.32, -0.6), fore=fore, wrist=0.0 if up else 10.0)})


def look():
    """Straightened from his work to whoever he talks to, his head up; the tape slack before him in both hands."""
    own = {'spine_01': rot_x(-STOOP + 3.0), 'spine_02': rot_x(-HUNCH + 4.0), 'neck': rot_x(-6.0), 'head': rot_x(-16.0)}
    arms = {1.0: dict(upper=mn.ue_dir(0.25, -0.2, -0.95), fore=mn.ue_dir(0.8, 0.3, 0.12), wrist=-6.0),
            -1.0: dict(upper=mn.ue_dir(0.25, 0.2, -0.95), fore=mn.ue_dir(0.8, -0.3, 0.12), wrist=-6.0)}
    return dict(own=own, arms=arms)


POSES = {'idle': {}, 'together': dict(arms={1.0: grip_at(1.0, (-0.19, 0.03, 0.0)), -1.0: grip_at(-1.0, (0.19, 0.03, 0.0))}),
         'mark': mark(), 'knock_up': knock(True), 'knock_down': knock(False), 'look': look(),
         'talk': dict(own={'jaw': rot_x(TALK_JAW)})}
HEADER = dict(units='Unreal component space at size 1: cm; x forward, y right, z up',
              quaternions='FQuat (X, Y, Z, W); turn = the whole turn from rest; own = the turn under the parent\'s '
                          '(Turned = Above * Own); in a pose, a bone not listed stays at rest under its parent',
              rest='the rest pose is the idle, the measure at full stretch: the actor\'s origin on the floor, stooped over '
                   'his work, his hands 50 cm apart about 98 cm up and 42 cm before him, the tape stretched between them '
                   '(it is his skin, shared between the hands: it stretches as they part)',
              together='the measure\'s start: his hands together before him (blend idle to together for the stretch)',
              mark='the tape held, his right hand lifted 9 cm and the wrist flicked down: a pencil\'s tick',
              knock='knock_up: his right fist raised over the board; knock_down: brought down on it (blend between them '
                    'for the rap); the left hand stays on the tape',
              look='straightened from his work to whoever he talks to, his head up, the tape slack before him',
              talk=f'the idle with the jaw opened {TALK_JAW:.0f} degrees (PitchBy on jaw): it layers on any pose',
              speaker='SOCKET_Speaker is on SM_ThaddeusSpectacles (they ride the head)')
TABLE = POSER.table(POSES, os.path.join(WORK, 'Thaddeus_poses.json'), HEADER, log)


# --- Previews (Saved/ArtPreviews/RansomsRest/Thaddeus) ---

def want_preview():
    return '--preview' in ARGV or lt.want_preview()


def figure_copy(where, M, pose=None):
    if pose:
        POSER.apply(POSES[pose])
        body = mn.posed_copy(BODY, where)
        place = Matrix(SPECS.matrix_world)
        mn.rest_pose(RIG)
        POSER.carry()
    else:
        body = mn.dup(BODY, where)
        body.modifiers.clear()
        place = Matrix.Translation(Vector(SPECS.location))
    body.matrix_world = M
    mn.preview_look(body, RAW_COLORS, rank=EMBER_COLOR, rank_alpha=EMBER_ALPHA)
    specs = mn.place_copy(SPECS, where, M, place)
    mn.preview_look(specs)
    return body, specs


def hide_sources(hidden=True):
    for obj in (BODY, SPECS):
        obj.hide_render = hidden
    for c in RIG.children:
        if c.name.startswith(('UCX_', 'USP_')):
            c.hide_render = hidden


def clear(where):
    for o in list(where.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.collections.remove(where)


def shot_turn(mc, path):
    """Front, side and back at rest, beside a 1.8 m post and a 1.8 m figure; then the look and the knock."""
    where = bpy.data.collections.new('Turn')
    bpy.context.scene.collection.children.link(where)
    mc.uc.post(where, (-1.9, 0.0, 0.0))
    mc.mannequin(where, (-1.45, 0.3, 0.0), 0.0)
    for k, (t, pose) in enumerate(((0.0, None), (90.0, None), (180.0, None), (0.0, 'look'), (0.0, 'knock_up'))):
        figure_copy(where, mn.figure_matrix((-0.75 + 0.8 * k, 0.0, 0.0), t), pose)
    mc.studio(where)
    cam = mc.uc.camera((0.65, -40.0, 1.0 + 40.0 * math.tan(math.radians(5.0))), (0.65, 0.0, 1.0), ortho=4.8)
    mc.render(path, (2400, 1100), 96)
    bpy.data.objects.remove(cam)
    clear(where)


def shot_cellar(mc, path, mode):
    """At his trestles in the cold room, the low sun in through the high window (golden), or by night (ghost)."""
    mn.kit()
    import LiftAndCellarConcepts as lc
    where = bpy.data.collections.new('Cellar')
    bpy.context.scene.collection.children.link(where)
    lc.cold_room(where)
    toward = (-0.86, -0.25, 0.44)
    if mode == 'golden':
        lc.golden_through(where, toward, strength=5.0)
        exposure = 0.3
    else:
        lc.moonlight(where, toward, strength=2.4, world=1.0)
        mc.area_light(where, (1.6, -2.6, 2.1), (-0.2, 0.3, 1.0), 2.0, 160.0)
        exposure = 0.6
    figure_copy(where, mn.figure_matrix((0.0, -0.05, 0.0), 0.0))
    eye = Vector((1.25, -2.9, 1.45))
    mc.lens_shot(path, eye, Vector((-0.05, -0.35, 1.1)), 30.0, (1600, 1100), 96, exposure=exposure)
    clear(where)


def shot_face(mc, path):
    where = bpy.data.collections.new('Face')
    bpy.context.scene.collection.children.link(where)
    for k, pose in enumerate(('look', 'talk')):
        figure_copy(where, mn.figure_matrix((0.6 * k, 0.0, 0.0), 0.0), pose)
    mc.studio(where)
    eye = Vector((0.3 + 0.1, -1.7, 1.62))
    mc.lens_shot(path, eye, Vector((0.3, -0.05, 1.55)), 60.0, (1600, 1000), 96)
    clear(where)


SHOTS = {'turn': ('Thaddeus_turn.png', shot_turn),
         'golden': ('Thaddeus_golden.png', lambda mc, p: shot_cellar(mc, p, 'golden')),
         'ghost': ('Thaddeus_ghost.png', lambda mc, p: shot_cellar(mc, p, 'ghost')), 'face': ('Thaddeus_face.png', shot_face)}


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
