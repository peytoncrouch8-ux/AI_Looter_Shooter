"""SK_FP_Arms: Ellis Ransom's arms as the player sees them in first person (the shared contract of 2026-10-14: the user
wants both hands on every gun, Borderlands-like). Ellis's design (Art/Backlog/Characters/Ellis.py): tan leather gloves
with flared gauntlet cuffs, worn over the sleeves of the weathered oilskin duster, in the duster's colour. The sleeves
run 25 cm up the upper arm (most of it), so where the code floats the shoulder to reach a far handguard the sleeve's cut
end stays below the frame: nothing else of the body is drawn in first person.

The rig and its space. The armature is the root bone and sits at the eye: Unreal +X forward, +Z up, cm (Blender: -Y
forward, +Z up, m, as Art/README.md asks). Bones are the UE5 mannequin's names: clavicle, upperarm, lowerarm, hand,
thumb_01..03, and index/middle/ring/pinky _metacarpal and _01.._03, each _l and _r (46 bones). Lengths are Ellis's (1.80
m, looter_heroes' Skeleton with his build); his anatomical shoulder is 12 cm behind and 24 cm below the eye. The rest
pose (the bind pose) is a relaxed forward hold: elbows bent about 80 degrees and pointing down and out, wrists straight,
hands in a handshake turn (thumbs up, palms in), fingers a little curled (14, 20 and 12 degrees per joint), so the
code's grips turn every joint by as little as possible. The code poses every bone (no animations).
Bone axes: Blender's (Y along each bone); on the arm Z is the elbow's hinge axis, on the hand and fingers Z points out
of the back of the hand (X is then the fingers' curl axis), on the thumb Z points out of its nail. Unreal's import
turns these axes, so the code works from directions, not local axes: Intermediate/FPArms/FPArms_rig.json has every
bone's head and tail in Unreal's component space (cm), each hand's palm frame (FingerDir, ThumbDir, PalmNormal), its
grip point (the middle of a 4 cm handle in a closed fist) and the way a held handle crosses the palm (HandleAxis), and
each finger's curl: turn it toward the palm normal. The look sheet's poser (Tools/Blender/fp_arms_sheet.py) shows one
way to put a hand on a gun from these.

The mesh. One skeletal mesh, FP_Arms (SK_FP_Arms), about 8k triangles, two material slots:
  FPArms_Glove   M_Character, texture set FPArmsGlove (2048): both gloves (mirrored UVs) and their gauntlets and conchos
  FPArms_Duster  M_CharacterCloth (Ellis's duster's sheen), texture set FPArmsDuster (2048): both sleeves
Both sets are painted by this script for this mesh's UVs (Art/Textures/FPArmsGlove, FPArmsDuster), in the colours of the
new third-person Ellis (LEATHER, OILSKIN, GAUNTLET_TONE below: sRGB means measured from his baked sets, 2026-10-15; keep
them within 3% if his looks change): the glove's seams
and stitching (in tonal thread: side seams up every finger and round its tip, three stitched cords, the points, on the
back, the keystone thumb; in pale linen: the gauntlet's hems and side seam), the gauntlet's tooled border and brass
concho, creases at every joint and across the palm, scuffed knuckles and fingertips, a grip-polished palm, an aniline
mottle and a pebbled grain; the oilskin's wax worn chalky on the fold crests, crackled, a twill weave, felled seams
with tonal topstitching. Change the mesh and its UVs change, so the textures are painted again on every run
(deterministic: the same mesh gives the same files).
The hand is modeled as a distance field (palm, pads, knuckles, each finger's chain of tapered capsules blended into
the palm but never into each other), meshed by surface nets and decimated; the gauntlet, its concho and the sleeve are
lofted, the sleeve's folds (bunched above the gauntlet, spiralling down the forearm, crescents in the crook of the
elbow) are geometry, with the textures shading them. Vertex alpha is baked occlusion (the masters multiply it in).
Skinning: fingers by their joints along each finger's chain, blended across the webs; the gauntlet rides the forearm
(its narrow end a third on the hand); the sleeve blends upper arm to forearm over the elbow. No hit zones, no LODs.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Art\\Models\\Characters\\FPArms.py [-Preview]
--preview renders the rest pose to Saved/ArtPreviews/Characters/FPArms_rest.png; keep-textures (after '--') skips the
painting (about two minutes) and uses the files as they are. The look sheet with the guns: Tools/Blender/fp_arms_sheet.py.
"""
import json
import math
import os
import sys
import time

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_heroes as lh
import looter_textures as lt

ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
C = 0.01                                   # the hand is laid out in cm, built in m
T0 = time.time()
WORK = os.path.join(lt.REPO, 'Intermediate', 'FPArms')
GLOVE_SET, DUSTER_SET = 'FPArmsGlove', 'FPArmsDuster'
ATLAS = 2048
HAND_TRIS = 1950                           # one glove's hand after decimation
# The colours the first-person arms must match: the new third-person Ellis's gloves and duster sleeves (Art/Models/
# Characters/Ellis.py's 'Gloves' and 'Duster' looks), measured from his baked sets (Art/Textures/EllisGear, EllisDuster)
# on 2026-10-15 (sRGB means over the pixels his mesh uses): hand glove (120.9, 90.3, 61.1), gauntlet cuff (117.1, 87.0,
# 58.2), duster sleeves (66.6, 54.3, 42.2: the faces weighted to the arms; his whole coat, cape and collar is darker,
# 62.4, 50.1, 38.1, and is not the target: the user's call, "gloves and sleeves match"). LEATHER and OILSKIN are the
# base tones of the painted sets; paint_glove and paint_duster scale every other leather / oilskin tone by the same
# per-channel factor (lv, ov below), so the creases, scuffs, wax and wear keep their contrast. GAUNTLET_TONE darkens
# the cuff alone: Ellis's cuff is a touch darker than his hand, where the painted cuff (no creases, tooled border,
# suede inside) came out lighter.
LEATHER, OILSKIN = 0x7d5f42, 0x5c4b39
LEATHER_PAINTED, OILSKIN_PAINTED = 0xa47c56, 0x8f7052   # the tones the layers were first mixed around (older Ellis)
GAUNTLET_TONE = np.array([0.93, 0.91, 0.88])


def log(message):
    print(f'FPARMS [{time.time() - T0:6.1f}s] {message}', flush=True)


def v3(*a):
    return np.array(a[0] if len(a) == 1 else a, dtype=np.float64)


def unit(v):
    v = np.asarray(v, dtype=np.float64)
    n = np.linalg.norm(v, axis=-1, keepdims=True)
    return v / np.maximum(n, 1e-12)


def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=np.float64) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def rotate(v, axis, degrees):
    """v turned about axis (right hand rule)."""
    k = unit(axis)
    a = math.radians(degrees)
    return v * math.cos(a) + np.cross(k, v) * math.sin(a) + k * (k @ v) * (1.0 - math.cos(a))


# ------------------------------------------------------------------------------------------------- Ellis's arm
# Ellis.py's build: the shoulder and the bone lengths, measured from the eye (the middle of his eyes).
ELLIS = lh.Build(height=1.80, shoulders=0.42, hips=0.21, head=1.12, neck=0.7, hand=1.12, foot=1.05, arm_angle=45.0)
REST_WRIST = v3(-0.165, -0.305, -0.345)    # Ellis's own wrist at rest (Blender, m, from the eye): it sets the elbow
REST_POLE = v3(-0.35, 0.3, -1.0)           # the elbow points down, back and out
REST_TURN = -12.0                          # the thumb leans in from straight up: the palm a touch down
# The forearm (elbow to wrist) is drawn 1.25 times Ellis's 26.1 cm (32.6 cm), the usual view-model trick (2026-10-22,
# Saved/FPHands/PLAN.md gap 1): a gun's fore-end sits 65-75 cm out, and a real-length forearm could only get its elbow
# (the sleeve's cut) out of the frame by standing straight up the screen; a longer one leaves it on a Borderlands-like
# slant. The shoulder, the upper arm, the elbow and the hand's turn stay Ellis's: the wrist (and the hand with it) moves
# out along the forearm's own line, so the hand bones keep their rest orientation. Hands, cuffs and upper arms keep
# their sizes; only the sleeve over the forearm is longer. The code reads the length from the skeleton.
FOREARM_STRETCH = 1.25


def two_bone(S, W, L1, L2, pole):
    """The elbow for a shoulder S and wrist W (W pulled in if out of reach), bending toward pole."""
    d = W - S
    dist = float(np.linalg.norm(d))
    u = d / dist
    dist = min(dist, (L1 + L2) * 0.999)
    a = (L1 * L1 - L2 * L2 + dist * dist) / (2.0 * dist)
    h = math.sqrt(max(L1 * L1 - a * a, 0.0))
    side = unit(pole - u * (pole @ u))
    return S + u * a + side * h, S + u * dist


class Rest:
    """The right arm's rest pose in Blender space (m, the eye at the origin); the left is its mirror image."""

    def __init__(self):
        sk = lh.Skeleton(ELLIS)
        eye = lh.on_head(sk, 0.0, 0.104 * 0.9, 0.022)
        self.clavicle = sk.head('clavicle_r') - eye
        self.shoulder = sk.head('upperarm_r') - eye
        self.upper = float(np.linalg.norm(sk.tail('upperarm_r') - sk.head('upperarm_r')))
        self.fore_body = float(np.linalg.norm(sk.tail('lowerarm_r') - sk.head('lowerarm_r')))   # Ellis's own
        self.fore = self.fore_body * FOREARM_STRETCH
        self.elbow, wrist = two_bone(self.shoulder, REST_WRIST, self.upper, self.fore_body, REST_POLE)
        a = unit(wrist - self.elbow)
        self.wrist = self.elbow + a * self.fore
        up = v3(0.0, 0.0, 1.0)
        s = rotate(unit(up - a * (up @ a)), a, REST_TURN)
        self.R = np.column_stack([a, s, np.cross(a, s)])     # hand frame (finger, thumb, back) -> Blender

    def to_world(self, p):
        """Hand-frame points (m) to Blender space."""
        return self.wrist + np.asarray(p) @ self.R.T

    def matrix(self):
        m = Matrix.Identity(4)
        for i in range(3):
            for j in range(3):
                m[i][j] = self.R[i, j]
            m[i][3] = self.wrist[i]
        return m


# ----------------------------------------------------------------------------------------------------- the hand
# The right hand in its own frame (cm): x along the fingers, y toward the thumb, z out of the back of the hand; the
# wrist joint at the origin. Knuckle (x, y), the finger's fan (degrees toward the thumb), its three bone lengths and
# its radii at the knuckle, the two joints and the tip (gloved). The palm is Ellis's (looter_heroes x 1.12); the fingers
# are a big man's real ones (looter_heroes' x 1.12 made them 11 cm and bony), thickened by the glove, and their knuckles
# spread wider than looter_heroes' so gloved fingers don't merge.
FINGERS = {
    'index': ((9.35, 3.45), 4.0, (3.85, 2.41, 2.02), (1.15, 1.06, 0.98, 0.93)),
    'middle': ((9.55, 1.15), 0.5, (4.24, 2.70, 2.12), (1.18, 1.09, 1.0, 0.95)),
    'ring': ((9.25, -1.15), -3.0, (3.95, 2.50, 2.02), (1.13, 1.04, 0.96, 0.91)),
    'pinky': ((8.55, -3.3), -7.5, (3.18, 2.02, 1.74), (1.01, 0.93, 0.88, 0.84)),
}
REST_CURL = (14.0, 20.0, 12.0)              # each joint's bend at rest (degrees, toward the palm)
THUMB_BASE = v3(1.55, 2.0, -1.0)
THUMB_DIRS = (v3(0.6, 0.6, -0.35), v3(0.88, 0.32, -0.14), v3(0.97, 0.1, -0.06))
THUMB_LEN = (3.95, 3.05, 2.55)
THUMB_R = (1.49, 1.32, 1.21, 1.08)
THUMB_NAIL = v3(0.05, 0.62, 0.78)           # the thumb's nail faces out and up
PALM_OUTLINE = [(-1.6, -2.75), (3.0, -3.85), (7.6, -3.95), (8.9, -2.6), (9.6, -0.4), (9.7, 1.5), (9.4, 3.6),
                (8.2, 4.35), (5.0, 4.0), (1.5, 3.3), (-1.6, 2.85)]
GRIP_POINT = v3(6.2, 0.1, -3.6)             # the middle of a 4 cm handle in the closed fist (cm)
HANDLE_AXIS = v3(0.18, 1.0, 0.0) / math.hypot(0.18, 1.0)   # that handle's way across the palm, toward the thumb


class Chain:
    """A finger (or the thumb) in the hand frame, m: joints from its metacarpal's base (fingers) or its base (thumb) to
    the tip, the radius at each, the curl axis (the thumb: its own), the bone names in order."""

    def __init__(self, name, joints, radii, lateral, s0, bones):
        self.name, self.joints, self.radii, self.lateral, self.s0, self.bones = name, joints, radii, lateral, s0, bones
        self.lengths = [float(np.linalg.norm(joints[i + 1] - joints[i])) for i in range(len(joints) - 1)]
        self.cum = np.concatenate([[0.0], np.cumsum(self.lengths)]) - s0   # along the chain, 0 at the knuckle

    def dorsal(self, k):
        """Out of the back of segment k."""
        e = unit(self.joints[k + 1] - self.joints[k])
        if self.name == 'thumb':
            n = THUMB_NAIL - e * (THUMB_NAIL @ e)
            return unit(n)
        return unit(np.cross(e, self.lateral))


def finger_chain(name, curl=REST_CURL):
    (kx, ky), fan, lengths, radii = FINGERS[name]
    f = math.radians(fan)
    d0 = v3(math.cos(f), math.sin(f), 0.0)
    knuckle = v3(kx, ky, 0.0)
    base = v3(1.35, ky * 0.55, -0.45)
    pts, total, p = [base, knuckle], 0.0, knuckle
    for L, c in zip(lengths, curl):
        total += math.radians(c)
        p = p + (d0 * math.cos(total) - v3(0, 0, 1.0) * math.sin(total)) * L
        pts.append(p)
    rr = [radii[0] * 1.15] + list(radii)
    lateral = v3(-math.sin(f), math.cos(f), 0.0)
    bones = [f'{name}_metacarpal', f'{name}_01', f'{name}_02', f'{name}_03']
    return Chain(name, [q * C for q in pts], [r * C for r in rr], lateral, float(np.linalg.norm(knuckle - base)) * C,
                 bones)


def thumb_chain():
    p = THUMB_BASE.copy()
    pts = [p]
    for d, L in zip(THUMB_DIRS, THUMB_LEN):
        p = p + unit(d) * L
        pts.append(p)
    lateral = unit(np.cross(THUMB_NAIL, unit(THUMB_DIRS[1])))
    return Chain('thumb', [q * C for q in pts], [r * C for r in THUMB_R], lateral, 0.0,
                 ['thumb_01', 'thumb_02', 'thumb_03'])


CHAINS = [finger_chain(n) for n in FINGERS] + [thumb_chain()]


# --------------------------------------------------------------------------------------------- distance fields
def smin(a, b, k):
    k = np.maximum(k, 1e-7)
    h = np.maximum(k - np.abs(a - b), 0.0) / k
    return np.minimum(a, b) - h * h * k * 0.25


def smax(a, b, k):
    return -smin(-a, -b, k)


def closest(P, a, b):
    ba = b - a
    t = np.clip(((P - a) @ ba) / max(float(ba @ ba), 1e-12), 0.0, 1.0)
    return t, a + t[:, None] * ba


def capsule(P, a, b, ra, rb, up=None, flat=1.0):
    """A tapered capsule; flat squashes its section along up."""
    t, c = closest(P, a, b)
    v = P - c
    r = ra + (rb - ra) * t
    if up is not None and flat != 1.0:
        v = v + np.outer((v @ up) * (1.0 / flat - 1.0), up)
        return (np.linalg.norm(v, axis=1) - r) * flat
    return np.linalg.norm(v, axis=1) - r


def ellipsoid(P, c, axes, radii):
    q = np.stack([(P - c) @ ax for ax in axes], axis=1) / np.asarray(radii)[None, :]
    return (np.linalg.norm(q, axis=1) - 1.0) * min(radii)


def outline_sdf(x, y, pts):
    """Signed distance to a closed 2D outline (negative inside)."""
    d = np.full(x.shape, np.inf)
    inside = np.zeros(x.shape, dtype=bool)
    for i in range(len(pts)):
        (ax, ay), (bx, by) = pts[i], pts[(i + 1) % len(pts)]
        ex, ey = bx - ax, by - ay
        wx, wy = x - ax, y - ay
        t = np.clip((wx * ex + wy * ey) / (ex * ex + ey * ey), 0.0, 1.0)
        d = np.minimum(d, np.hypot(wx - ex * t, wy - ey * t))
        crosses = ((ay <= y) & (y < by)) | ((by <= y) & (y < ay))
        inside ^= crosses & (x < ax + (y - ay) * ex / (ey if abs(ey) > 1e-12 else 1e-12))
    return np.where(inside, -d, d)


def palm_sdf(P):
    """The palm: a slab inside the outline, its back gently domed, its heel thick, its middle cupped."""
    x, y, z = P[:, 0] / C, P[:, 1] / C, P[:, 2] / C
    d2 = outline_sdf(x, y, PALM_OUTLINE)
    top = 1.05 - 0.22 * (y / 4.2) ** 2 + 0.12 * smoothstep(6.0, 9.0, x)
    heel = 0.45 * (1.0 - smoothstep(1.0, 5.0, x))
    cup = 0.3 * np.exp(-((x - 5.5) / 2.2) ** 2 - (y / 2.0) ** 2)
    slab = smax(z - top, -1.45 - heel + cup - z, 0.6)
    return smax(d2, slab, 2.6) * C


def chain_sdf(P, chain, joint_k=0.25):
    d = None
    first = 0 if chain.name == 'thumb' else 1      # a finger's metacarpal is inside the palm
    for k in range(first, len(chain.joints) - 1):
        a, b = chain.joints[k], chain.joints[k + 1]
        up = chain.dorsal(k)
        seg = capsule(P, a, b, chain.radii[k], chain.radii[k + 1], up=up, flat=0.9 if chain.name != 'thumb' else 0.88)
        d = seg if d is None else smin(d, seg, joint_k * C)
    return d


def body_sdf(P):
    """The palm with the wrist, the pads and the knuckles: everything the fingers grow from."""
    d = palm_sdf(P)
    d = smin(d, capsule(P, v3(-7.0, 0.0, -0.2) * C, v3(0.5, 0.0, -0.2) * C, 2.95 * C, 2.8 * C, up=v3(0, 0, 1.0),
                        flat=0.72), 1.5 * C)
    ax = unit((0.75, 0.5, -0.25))
    side = unit(np.cross(ax, (0.0, 0.0, 1.0)))
    d = smin(d, ellipsoid(P, v3(3.4, 2.3, -1.15) * C, [ax, side, np.cross(side, ax)], v3(3.0, 1.75, 1.3) * C), 1.2 * C)
    d = smin(d, ellipsoid(P, v3(4.0, -2.75, -0.95) * C, [v3(1, 0, 0), v3(0, 1, 0), v3(0, 0, 1)], v3(3.6, 1.25, 1.1) * C),
             1.0 * C)
    for ch in CHAINS[:4]:
        knuckle = np.linalg.norm(P - (ch.joints[1] + v3(0, 0, 0.05) * C), axis=1) - ch.radii[1] * 0.92
        d = smin(d, knuckle, 0.9 * C)
    return d


def hand_sdf(P):
    body = body_sdf(P)
    out = body
    for ch in CHAINS:
        f = chain_sdf(P, ch)
        if ch.name == 'thumb':
            along = (P - ch.joints[0]) @ unit(ch.joints[1] - ch.joints[0])
            k = 0.3 * C + 1.3 * C * (1.0 - smoothstep(0.03, 0.055, along))
        else:
            along = (P - ch.joints[1]) @ unit(ch.joints[2] - ch.joints[1])
            k = 0.15 * C + 0.85 * C * (1.0 - smoothstep(-0.005, 0.025, along))
        # Each finger blends into the palm, never into its neighbours (a hard min between them).
        out = np.minimum(out, smin(body, f, k))
    return out


# ------------------------------------------------------------------------------------------------ surface nets
CORNERS = np.array([(i, j, k) for k in (0, 1) for j in (0, 1) for i in (0, 1)])
EDGES = [(a, b) for a in range(8) for b in range(8) if a < b and np.abs(CORNERS[a] - CORNERS[b]).sum() == 1]


def surface_nets(F, origin, h):
    """A quad mesh of the zero surface of F (sampled on a grid from origin, spacing h)."""
    nx, ny, nz = F.shape
    cs = [F[i:nx - 1 + i, j:ny - 1 + j, k:nz - 1 + k] for i, j, k in CORNERS]
    acc = np.zeros((nx - 1, ny - 1, nz - 1, 3))
    cnt = np.zeros((nx - 1, ny - 1, nz - 1))
    for a, b in EDGES:
        fa, fb = cs[a], cs[b]
        cross = (fa < 0) != (fb < 0)
        t = np.where(cross, fa / np.where(cross, fa - fb, 1.0), 0.0)
        for ax in range(3):
            acc[..., ax] += np.where(cross, CORNERS[a][ax] + t * (CORNERS[b][ax] - CORNERS[a][ax]), 0.0)
        cnt += cross
    active = cnt > 0
    vid = -np.ones(active.shape, dtype=np.int64)
    vid[active] = np.arange(int(active.sum()))
    I, J, K = np.nonzero(active)
    verts = origin[None, :] + (np.stack([I, J, K], axis=1) + acc[active] / cnt[active][:, None]) * h
    inside = F < 0
    quads = []
    for axis in range(3):
        sl0 = [slice(1, -1)] * 3
        sl1 = [slice(1, -1)] * 3
        sl0[axis], sl1[axis] = slice(0, -1), slice(1, None)
        e = inside[tuple(sl0)] != inside[tuple(sl1)]
        idx = list(np.nonzero(e))
        for o in range(3):
            if o != axis:
                idx[o] = idx[o] + 1
        i, j, k = idx
        if axis == 0:
            q = np.stack([vid[i, j - 1, k - 1], vid[i, j, k - 1], vid[i, j, k], vid[i, j - 1, k]], axis=1)
        elif axis == 1:
            q = np.stack([vid[i - 1, j, k - 1], vid[i - 1, j, k], vid[i, j, k], vid[i, j, k - 1]], axis=1)
        else:
            q = np.stack([vid[i - 1, j - 1, k], vid[i, j - 1, k], vid[i, j, k], vid[i - 1, j, k]], axis=1)
        flip = ~inside[i, j, k]
        q[flip] = q[flip][:, ::-1]
        quads.append(q)
    quads = np.vstack(quads)
    return verts, quads[(quads >= 0).all(axis=1)]


def project(verts, sdf, steps=2, eps=2e-4):
    """Moves points onto the zero surface along the field's gradient."""
    for _ in range(steps):
        d = sdf(verts)
        g = np.stack([(sdf(verts + np.eye(3)[i] * eps) - d) / eps for i in range(3)], axis=1)
        verts = verts - (d / np.maximum((g * g).sum(axis=1), 1e-6))[:, None] * g
    return verts


def relax(verts, quads, sdf, iterations=3, lam=0.5):
    rows = np.concatenate([quads[:, i] for i in range(4)])
    cols = np.concatenate([quads[:, (i + 1) % 4] for i in range(4)])
    for _ in range(iterations):
        acc = np.zeros_like(verts)
        cnt = np.zeros(len(verts))
        np.add.at(acc, rows, verts[cols])
        np.add.at(cnt, rows, 1.0)
        np.add.at(acc, cols, verts[rows])
        np.add.at(cnt, cols, 1.0)
        verts = project(verts + lam * (acc / np.maximum(cnt, 1)[:, None] - verts), sdf)
    return verts


# ----------------------------------------------------------------------------------------------- chain fields
def chain_field(P, ch):
    """For points P (hand frame, m): the distance to the chain's surface, how far along it (m, 0 at the knuckle; on
    the tip's cap it goes on round the cap), the turn round it (radians, 0 at the back, +pi/2 toward the thumb side or
    the thumb's lateral), the radius there and the segment."""
    n = len(P)
    best = np.full(n, np.inf)
    s = np.zeros(n)
    phi = np.zeros(n)
    rad = np.zeros(n)
    segk = np.zeros(n, dtype=np.int64)
    last = len(ch.joints) - 2
    for k in range(len(ch.joints) - 1):
        a, b = ch.joints[k], ch.joints[k + 1]
        L = ch.lengths[k]
        e = (b - a) / L
        tt = np.clip(((P - a) @ e) / L, 0.0, 1.0)
        c = a + tt[:, None] * (b - a)
        v = P - c
        r = ch.radii[k] + (ch.radii[k + 1] - ch.radii[k]) * tt
        d = np.linalg.norm(v, axis=1) - r
        better = d < best
        if not better.any():
            continue
        dors = ch.dorsal(k)
        lat = np.cross(dors, e)
        if ch.name != 'thumb':
            lat = ch.lateral
        ang = np.arctan2(v @ lat, v @ dors)
        along = ch.cum[k] + tt * L
        if k == last:
            beyond = np.maximum((P - b) @ e, 0.0)
            radial = np.linalg.norm(v - np.outer(v @ e, e), axis=1)
            along = along + r * np.arctan2(beyond, np.maximum(radial, 1e-6))
        best = np.where(better, d, best)
        s = np.where(better, along, s)
        phi = np.where(better, ang, phi)
        rad = np.where(better, r, rad)
        segk = np.where(better, k, segk)
    return best, s, phi, rad, segk


# ------------------------------------------------------------------------------------------------ the glove mesh
def glove_hand(h=0.0015):
    """The hand's surface (hand frame, m): sampled, meshed, relaxed onto the field, then decimated in Blender."""
    lo, hi = v3(-7.5, -6.0, -8.5) * C, v3(22.0, 10.5, 4.0) * C
    n = np.ceil((hi - lo) / h).astype(int) + 1
    X, Y, Z = np.meshgrid(*[lo[i] + np.arange(n[i]) * h for i in range(3)], indexing='ij')
    F = hand_sdf(np.stack([X.ravel(), Y.ravel(), Z.ravel()], axis=1)).reshape(n)
    verts, quads = surface_nets(F, lo, h)
    verts = relax(verts, quads, hand_sdf)
    log(f'hand field {tuple(n)}: {len(quads)} quads')
    mesh = bpy.data.meshes.new('_HandDense')
    mesh.from_pydata([tuple(p) for p in verts], [], [tuple(int(i) for i in q) for q in quads])
    mesh.validate()
    obj = bpy.data.objects.new('_HandDense', mesh)
    bpy.context.scene.collection.objects.link(obj)
    # Keep more triangles round the finger joints, where the code bends the glove.
    protect = obj.vertex_groups.new(name='Protect')
    w = np.full(len(verts), 0.08)
    for ch in CHAINS:
        d, s, _, rad, _ = chain_field(verts, ch)
        near = d < 0.006
        bend = np.zeros(len(verts))
        for sj in ch.cum[1:-1]:
            bend = np.maximum(bend, np.exp(-((s - sj) / 0.011) ** 2))
        w = np.where(near & (s > -0.006), np.maximum(w, 0.65 + 0.35 * bend), w)
    for i, wi in enumerate(w):
        protect.add([i], float(wi), 'REPLACE')
    mod = obj.modifiers.new('Decimate', 'DECIMATE')
    mod.ratio = HAND_TRIS / (2.0 * len(quads))
    mod.use_collapse_triangulate = True
    mod.vertex_group = 'Protect'
    mod.vertex_group_factor = 6.0
    depsgraph = bpy.context.evaluated_depsgraph_get()
    low = bpy.data.meshes.new_from_object(obj.evaluated_get(depsgraph))
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    bm = bmesh.new()
    bm.from_mesh(low)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    # Decimation leaves a few slivers with no area (no tangent space): fold them away.
    bmesh.ops.dissolve_degenerate(bm, dist=2e-5, edges=bm.edges[:])
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.verts.index_update()
    v = np.array([vv.co[:] for vv in bm.verts])
    f = np.array([[vv.index for vv in face.verts] for face in bm.faces])
    bm.free()
    bpy.data.meshes.remove(low)
    log(f'hand decimated to {len(f)} triangles')
    return v, f


# The gauntlet in the hand frame (cm): its narrow end over the wrist, flaring back up the forearm, longer over the back
# of the arm; its outer face, the rolled top edge, a little of its inside, and the narrow end's rolled edge.
GAUNTLET_X0, GAUNTLET_SEGS = 0.6, 20
CONCHO_AT = (math.radians(58.0), 5.7)       # (turn from the back toward the thumb side, cm up from the narrow end)
CONCHO_R = 1.15


def gauntlet_top(theta):
    return 9.8 + 1.0 * np.cos(theta)


def gauntlet_radii(h, top=None):
    """A stiff flared cuff: snug over the wrist, opening almost straight-sided to well over the wrist's size."""
    f = np.clip(h / 10.0, 0.0, 1.15) ** 1.12
    return 3.3 + 2.3 * f, 2.6 + 2.65 * f, -0.25 + 0.12 * np.clip(h / 10.0, 0, 1)


def gauntlet_shape(theta, h, inset=0.0):
    """The gauntlet's half sizes across the thumb side and the back (cm) at turn theta and height h, with the
    leather's waver and two soft creases on the palm side near the wrist; and its middle's offset (z)."""
    ry, rz, zc = gauntlet_radii(h)
    wob = (0.1 * np.sin(3.0 * theta + 1.3) + 0.06 * np.sin(5.0 * theta + 0.4)) * np.clip(h / 12.0, 0, 1)
    crease = -0.14 * np.exp(-((np.abs(theta) - (math.pi - 0.38)) / 0.12) ** 2) * np.exp(-((h - 3.0) / 2.0) ** 2)
    return ry + wob + crease - inset, rz + wob + crease - inset, zc


def gauntlet_surface(theta, h, inset=0.0):
    """A point on the gauntlet (cm, hand frame) at turn theta (0 the back of the hand, + toward the thumb) and h cm
    up from the narrow end; inset moves it inward. Returns the point and the outward normal."""
    ry, rz, zc = gauntlet_shape(theta, h, inset)
    y, z = ry * np.sin(theta), zc + rz * np.cos(theta)
    x = GAUNTLET_X0 - h
    # The normal of the cone-like surface: across (from the ellipse) and along (from the flare).
    ny, nz = np.sin(theta) / ry, np.cos(theta) / rz
    dr = 0.18
    n = np.stack([np.full_like(theta, dr), ny * np.hypot(ry, rz) / math.sqrt(2), nz * np.hypot(ry, rz) / math.sqrt(2)],
                 axis=-1)
    return np.stack([x, y, z], axis=-1), unit(n)


GAUNTLET_T = 0.25                           # the leather's thickness (cm)


def gauntlet_mesh():
    """Rows of (theta, h, inset) round the gauntlet's profile, inside of the narrow end to inside of the top."""
    segs = GAUNTLET_SEGS
    theta = np.linspace(-math.pi, math.pi, segs + 1)    # the seam (palm side) at +-pi, duplicated for the UVs
    t = GAUNTLET_T
    top = gauntlet_top(theta)
    rows = [(np.full_like(theta, 1.0), t, 'in'), (np.full_like(theta, 0.05), t * 0.85, 'rim0'),
            (np.full_like(theta, 0.0), t * 0.35, 'rim0')]
    for u in (0.0, 0.1, 0.24, 0.4, 0.56, 0.7, 0.82, 0.92, 0.98):
        rows.append((top * u + 0.0, 0.0, 'out'))
    rows += [(top + 0.06, t * 0.25, 'rim1'), (top + 0.06, t * 0.75, 'rim1'), (top - 1.2, t, 'in'),
             (top - 3.0, t, 'in')]
    verts, normals, uvs, part = [], [], [], []
    arc = 0.0
    prev = None
    for h, inset, kind in rows:
        p, nrm = gauntlet_surface(theta, h, inset)
        if prev is not None:
            arc += float(np.mean(np.linalg.norm(p - prev, axis=1)))
        prev = p
        ry, rz, _ = gauntlet_radii(np.mean(h))
        r = 0.5 * (ry + rz) - inset
        verts.append(p)
        normals.append(nrm if kind != 'in' else -nrm)
        uvs.append(np.stack([theta * r, np.full_like(theta, arc)], axis=1))
        part.append(np.full(len(theta), 1.0 if kind in ('out', 'rim1', 'rim0') else 2.0))
    nr = len(rows)
    faces = []
    for i in range(nr - 1):
        for j in range(segs):
            a, b = i * (segs + 1) + j, i * (segs + 1) + j + 1
            faces.append([a, b, b + segs + 1, a + segs + 1])
    th = np.tile(theta, nr)
    hh = np.concatenate([r[0] for r in rows])
    return (np.vstack(verts) * C, np.array(faces), np.vstack(uvs) * C, np.concatenate(part), th, hh)


def concho_mesh():
    """A scalloped brass concho on the gauntlet's outer face: a low dome on a rim, eight lobes."""
    th, h = CONCHO_AT
    center, n = gauntlet_surface(np.array([th]), np.array([h]))
    center, n = center[0], n[0]
    t1 = unit(np.cross(n, v3(1.0, 0.0, 0.0)))
    t2 = np.cross(n, t1)
    segs = 24
    psi = np.linspace(0.0, 2.0 * math.pi, segs, endpoint=False)
    lobe = 1.0 + 0.07 * np.cos(8.0 * psi)
    profile = [(1.0, -0.06), (1.0, 0.1), (0.86, 0.2), (0.55, 0.3), (0.0, 0.34)]   # (radius share, height cm)
    verts, rho_all, psi_all, uvs = [], [], [], []
    for rs, z in profile:
        if rs == 0.0:
            verts.append(center[None, :] + n[None, :] * z)
            rho_all.append([0.0])
            psi_all.append([0.0])
            uvs.append([[0.0, 0.0]])
            continue
        r = CONCHO_R * rs * (lobe if rs > 0.9 else np.ones(segs))
        verts.append(center[None, :] + np.outer(r * np.cos(psi), t1) + np.outer(r * np.sin(psi), t2) + n[None, :] * z)
        rho_all.append(r / CONCHO_R)
        psi_all.append(psi)
        uvs.append(np.stack([r * np.cos(psi), r * np.sin(psi)], axis=1) * (1.0 + 0.25 * (rs > 0.95) * (z < 0)))
    faces = []
    for i in range(3):
        for j in range(segs):
            a, b = i * segs + j, i * segs + (j + 1) % segs
            faces.append([a, b, b + segs, a + segs])
    tip = 4 * segs
    for j in range(segs):
        faces.append([3 * segs + j, 3 * segs + (j + 1) % segs, tip])
    return (np.vstack(verts) * C, faces, np.vstack(uvs) * C, np.concatenate(rho_all), np.concatenate(psi_all))


def build_glove():
    """The right glove (hand, gauntlet, concho) as one object in the hand frame, UV-mapped into the glove atlas.
    Returns the object and its per-vertex data: part (0 hand, 1 gauntlet outside, 2 its inside, 3 concho), and
    (theta, h) on the gauntlet, (rho, psi) on the concho."""
    hv, hf = glove_hand()
    gv, gf, guv, gpart, gth, gh = gauntlet_mesh()
    cv, cf, cuv, crho, cpsi = concho_mesh()
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.new('UVMap')
    part, extra_a, extra_b = [], [], []

    def add(verts, faces, uv=None, p=0.0, ea=None, eb=None):
        base = len(bm.verts)
        made = [bm.verts.new(tuple(v)) for v in verts]
        bm.verts.ensure_lookup_table()
        for f in faces:
            face = bm.faces.new([made[i] for i in f])
            if uv is not None:
                for loop, i in zip(face.loops, f):
                    loop[uv_layer].uv = (float(uv[i][0]), float(uv[i][1]))
        part.extend([p] * len(verts) if np.isscalar(p) else list(p))
        extra_a.extend([0.0] * len(verts) if ea is None else list(ea))
        extra_b.extend([0.0] * len(verts) if eb is None else list(eb))
        return base

    add(hv, hf, None, 0.0)
    n_hand = len(hf)
    add(gv, gf, guv, gpart, gth, gh)
    add(cv, cf, cuv, 3.0, crho, cpsi)
    mesh = bpy.data.meshes.new('Glove_r')
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new('Glove_r', mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.shade_smooth()

    # The hand's faces get Blender's smart projection; then every island is scaled to its true size and packed.
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='DESELECT')
    bpy.ops.object.mode_set(mode='OBJECT')
    for poly in mesh.polygons:
        poly.select = poly.index < n_hand
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(60.0), island_margin=0.0, area_weight=0.0, correct_aspect=True,
                             scale_to_bounds=False)
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.select_all(action='SELECT')
    bpy.ops.uv.average_islands_scale()
    bpy.ops.uv.pack_islands(rotate=True, margin=0.006)
    bpy.ops.object.mode_set(mode='OBJECT')
    data = dict(part=np.array(part), a=np.array(extra_a), b=np.array(extra_b))
    log(f'glove: {len(mesh.vertices)} vertices, {sum(len(p.vertices) - 2 for p in mesh.polygons)} triangles')
    return obj, data


# ------------------------------------------------------------------------------------------------- the sleeve
SLEEVE_SEGS = 16
SLEEVE_TUCK = -5.4                          # the sleeve's end inside the gauntlet (hand frame x, cm)
UPPER_STEP = 6.2                            # the sleeve's rings on the upper arm, past the elbow's folds (cm apart)


def sleeve_h(v):
    """Where along the gauntlet (cm up from its narrow end) the sleeve's ring at v is (on the forearm)."""
    return GAUNTLET_X0 - SLEEVE_TUCK + v


def sleeve_rim(theta_g, rim_v):
    """The sleeve's v at the gauntlet's top edge, turn by turn (the top is higher over the back of the arm)."""
    return rim_v + (gauntlet_top(theta_g) - gauntlet_top(math.pi / 2))


def sleeve_fold(theta_g, v, rim_v, elbow_v, inner_theta):
    """The oilskin's folds (cm, outward): bunched rings just above the gauntlet, two spiral folds down the forearm,
    crescents in the crook of the elbow, a soft drape on the upper arm."""
    out = np.zeros(np.broadcast(theta_g, v).shape)
    above = v - sleeve_rim(theta_g, rim_v)
    ring = np.exp(-np.maximum(above, 0.0) / 4.5) * smoothstep(-0.6, 0.4, above)
    out += 0.42 * ring * np.sin(2 * math.pi * above / 2.3 + 0.9 * np.sin(2 * theta_g + 0.6)) * (0.7 + 0.3 * np.sin(3 * theta_g))
    fore = smoothstep(rim_v + 2.0, rim_v + 5.0, v) * (1.0 - smoothstep(elbow_v - 4.0, elbow_v - 1.5, v))
    for phase, amp in ((0.4, 0.3), (2.7, 0.22)):
        d = np.angle(np.exp(1j * (theta_g - phase - (v - rim_v) * 0.16)))
        out += amp * fore * np.exp(-(d / 0.32) ** 2)
    d_in = np.angle(np.exp(1j * (theta_g - inner_theta)))
    crook = np.exp(-(d_in / 1.05) ** 2)
    for dv, amp in ((-3.4, 0.38), (-1.2, 0.55), (1.0, 0.5), (3.1, 0.32)):
        bend = d_in * 1.2
        out += amp * crook * np.exp(-((v - elbow_v - dv - 0.6 * bend * bend) / 0.75) ** 2)
    out -= 0.18 * crook * smoothstep(elbow_v - 4.5, elbow_v, v) * (1.0 - smoothstep(elbow_v, elbow_v + 4.5, v))
    upper = smoothstep(elbow_v + 3.0, elbow_v + 6.0, v)
    out += upper * (0.16 * np.sin(4 * theta_g + 0.3 + 0.05 * v) + 0.1 * np.sin(7 * theta_g + 1.1))
    return out


class SleevePath:
    """The sleeve's centre line (Blender, m) from its tucked end to the cut on the upper arm, by arc length (cm), with a
    parallel-transported frame whose (zz, yy) start as the hand frame's back and thumb directions."""

    def __init__(self, rest):
        R = rest.R
        a = R[:, 0]
        tuck = rest.to_world(v3(SLEEVE_TUCK, 0.0, -0.2) * C)
        E, S = rest.elbow, rest.shoulder
        up_dir = unit(S - E)
        p1 = E + unit(tuck - E) * 0.065
        p2 = E + up_dir * 0.06
        cut = E + up_dir * 0.25
        pts = [tuck + (p1 - tuck) * t for t in np.linspace(0.0, 1.0, 40)]
        for t in np.linspace(0.0, 1.0, 30)[1:]:
            pts.append((1 - t) ** 2 * p1 + 2 * (1 - t) * t * E + t * t * p2)
        pts += [p2 + (cut - p2) * t for t in np.linspace(0.0, 1.0, 30)[1:]]
        P = np.array(pts)
        seg = np.linalg.norm(np.diff(P, axis=0), axis=1)
        self.s = np.concatenate([[0.0], np.cumsum(seg)]) / C
        self.P = P
        T = np.gradient(P, axis=0)
        T = unit(T)
        zz, yy = [R[:, 2]], [R[:, 1]]
        for i in range(1, len(P)):
            z = unit(zz[-1] - T[i] * (T[i] @ zz[-1]))
            zz.append(z)
            yy.append(unit(np.cross(T[i], z)))
        self.T, self.zz, self.yy = T, np.array(zz), np.array(yy)
        # Arc lengths of the gauntlet's top (its middle height, on the thumb side) and of the elbow.
        self.rim_v = gauntlet_top(math.pi / 2) - (GAUNTLET_X0 - SLEEVE_TUCK)
        self.elbow_v = float(self.s[int(np.argmin(np.linalg.norm(P - E, axis=1)))])
        self.length = float(self.s[-1])
        inner = unit(unit(rest.wrist - E) + unit(S - E))
        k = int(np.argmin(np.abs(self.s - self.elbow_v)))
        self.inner_theta = math.atan2(inner @ self.yy[k], inner @ self.zz[k])
        self.a = a

    def at(self, v):
        i = np.clip(np.searchsorted(self.s, v) - 1, 0, len(self.s) - 2)
        t = ((v - self.s[i]) / np.maximum(self.s[i + 1] - self.s[i], 1e-9))[:, None]
        lerp = lambda A: A[i] * (1 - t) + A[i + 1] * t
        return lerp(self.P), unit(lerp(self.zz)), unit(lerp(self.yy)), unit(lerp(self.T))


def sleeve_radii(v, path):
    """The sleeve's free half sizes (cm) across the thumb side (ry) and the back (rz) at v: puffed where it bunches
    out of the gauntlet, looser at the elbow. (Inside the gauntlet sleeve_ring holds it to the gauntlet's inside.)"""
    rim = path.rim_v
    elbow = path.elbow_v
    knots = [0, rim - 1.0, rim + 0.9, rim + 3.0, rim + 7.0, elbow - 2.0, elbow + 1.0, path.length]
    ry = np.interp(v, knots, [4.6, 5.0, 5.2, 5.15, 5.1, 5.5, 6.1, 6.4])
    rz = np.interp(v, knots, [4.2, 4.6, 4.85, 4.8, 4.75, 5.25, 5.9, 6.25])
    return ry, rz


def sleeve_ring(theta_g, v, path):
    """The sleeve's half sizes at turns theta_g and v, with its folds: held inside the gauntlet below its top edge,
    folded and bunched above it. Also returns the fold height and how far out of the gauntlet it is (0..1)."""
    ry, rz = sleeve_radii(v, path)
    h = sleeve_h(v)
    gy, gz, _ = gauntlet_shape(theta_g, h, GAUNTLET_T + 0.12)
    out = smoothstep(gauntlet_top(theta_g) - 0.3, gauntlet_top(theta_g) + 0.9, h)
    fold = sleeve_fold(theta_g, v, path.rim_v, path.elbow_v, path.inner_theta) * (0.12 + 0.88 * out)
    r_y = (1.0 - out) * np.minimum(ry, gy) + out * ry + fold
    r_z = (1.0 - out) * np.minimum(rz, gz) + out * rz + fold
    return r_y, r_z, fold, out


def sleeve_rows(path):
    """Arc lengths of the sleeve's rings: closer where it folds (above the gauntlet and at the elbow). The upper arm's
    rings are far apart (its radii and drape change slowly, and the framing keeps it under the frame): the rings the
    longer forearm needs (FOREARM_STRETCH) come from there, so the mesh keeps its triangle count."""
    vs = [0.0]
    while vs[-1] < path.length:
        v = vs[-1]
        step = 1.55
        if path.rim_v - 2.0 < v < path.rim_v + 7.0:
            step = 0.62
        elif abs(v - path.elbow_v) < 5.5:
            step = 0.72
        elif v > path.elbow_v + 5.5:
            step = UPPER_STEP
        vs.append(min(v + step, path.length))
        if path.length - vs[-1] < 0.5:
            vs[-1] = path.length
    return np.array(vs)


def sleeve_layout(v, circ, length):
    """The duster atlas: the sleeve unrolled with its underarm seam at the island's edges (u from the seam), widths
    true to its girth; the cut's lining in the corner."""
    scale = 0.94 / max(length, 44.0)
    return scale


def build_sleeve(rest):
    """The right sleeve (Blender space) with its UVs and per-vertex (theta from the seam, v, fold height)."""
    path = SleevePath(rest)
    vs = sleeve_rows(path)
    segs = SLEEVE_SEGS
    theta_s = np.linspace(0.0, 2.0 * math.pi, segs + 1)       # from the underarm seam (the palm side)
    theta_g = theta_s - math.pi                               # the gauntlet's turn: 0 at the back
    scale = sleeve_layout(None, None, path.length)
    verts, uvs, data = [], [], []
    for v in vs:
        P, zz, yy, T = path.at(np.array([v]))
        P, zz, yy = P[0], zz[0], yy[0]
        r_y, r_z, fold, _ = sleeve_ring(theta_g, v, path)
        ring = P[None, :] + (np.outer(r_y * np.sin(theta_g), yy) + np.outer(r_z * np.cos(theta_g), zz)) * C
        verts.append(ring)
        ry, rz = sleeve_radii(v, path)
        circ = math.pi * (3 * (ry + rz) - math.sqrt((3 * ry + rz) * (ry + 3 * rz)))
        u = 0.5 + (theta_s / (2 * math.pi) - 0.5) * circ * scale
        uvs.append(np.stack([u, np.full_like(u, 0.03 + v * scale)], axis=1))
        data.append(np.stack([theta_s, np.full_like(theta_s, v), fold], axis=1))
    nr = len(vs)
    faces = []
    for i in range(nr - 1):
        for j in range(segs):
            a, b = i * (segs + 1) + j, i * (segs + 1) + j + 1
            faces.append([a, b, b + segs + 1, a + segs + 1])
    # The cut end's lining: a ring stepped in, closed by a fan, mapped into the atlas's corner.
    last = (nr - 1) * (segs + 1)
    P, zz, yy, T = path.at(np.array([path.length]))
    P, zz, yy, T = P[0], zz[0], yy[0], T[0]
    ry, rz = sleeve_radii(path.length, path)
    inner = P[None, :] - T[None, :] * 0.006 + (np.outer(0.82 * ry * np.sin(theta_g), yy) +
                                                np.outer(0.82 * rz * np.cos(theta_g), zz)) * C
    base = len(vs) * (segs + 1)
    verts.append(inner)
    verts.append((P - T * 0.012)[None, :])
    lining = np.stack([0.075 + 0.06 * np.sin(theta_g), 0.075 + 0.06 * np.cos(theta_g)], axis=1)
    uvs.append(lining)
    uvs.append(np.array([[0.075, 0.075]]))
    data.append(np.stack([theta_s, np.full_like(theta_s, -1.0), np.zeros_like(theta_s)], axis=1))
    data.append(np.array([[0.0, -1.0, 0.0]]))
    lining_faces = []
    for j in range(segs):
        lining_faces.append([last + j, last + j + 1, base + j + 1, base + j])
        lining_faces.append([base + j, base + j + 1, base + segs + 1])
    V = np.vstack(verts)
    UV = np.vstack(uvs)
    D = np.vstack(data)
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.new('UVMap')
    made = [bm.verts.new(tuple(p)) for p in V]
    lining_uv = {}
    for f in faces:
        face = bm.faces.new([made[i] for i in f])
        for loop, i in zip(face.loops, f):
            loop[uv_layer].uv = tuple(UV[i])
    for f in lining_faces:
        face = bm.faces.new([made[i] for i in f])
        for loop, i in zip(face.loops, f):
            if i < base:
                j = i - last
                loop[uv_layer].uv = (0.075 + 0.07 * math.sin(theta_g[j]), 0.075 + 0.07 * math.cos(theta_g[j]))
            else:
                loop[uv_layer].uv = tuple(UV[i])
    mesh = bpy.data.meshes.new('Sleeve_r')
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new('Sleeve_r', mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.shade_smooth()
    log(f'sleeve: {len(mesh.vertices)} vertices, {sum(len(p.vertices) - 2 for p in mesh.polygons)} triangles, '
        f'{path.length:.1f} cm long (gauntlet top {path.rim_v:.1f}, elbow {path.elbow_v:.1f})')
    return obj, dict(theta=D[:, 0], v=D[:, 1], fold=D[:, 2], path=path, scale=scale)


# ------------------------------------------------------------------------------------------------------ weights
def hand_weights(P, part, gh):
    """Bone weights for the glove's vertices (hand frame, m): {bone (no side): weights}."""
    n = len(P)
    W = {}

    def add(bone, w):
        W[bone] = W.get(bone, np.zeros(n)) + w

    hand = part == 0
    # Each chain's own weights, by how far along it a vertex is, blended across its joints.
    fields, chainw = [], []
    for ch in CHAINS:
        d, s, _, rad, _ = chain_field(P, ch)
        cw = {}
        if ch.name == 'thumb':
            marks = ch.cum[1:-1]
            names = ch.bones
            base = np.ones(n)
            prev = np.ones(n)
            ws = []
            for k, sj in enumerate(marks):
                hw = 0.85 * ch.radii[k + 1]
                ws.append(smoothstep(sj - hw, sj + hw, s))
            cw[names[0]] = 1.0 - ws[0]
            cw[names[1]] = ws[0] * (1.0 - ws[1])
            cw[names[2]] = ws[1]
            # The thenar end of the thumb's metacarpal blends into the hand.
            into = smoothstep(0.004, 0.022, s)
            cw = {k: v * into for k, v in cw.items()}
            cw['hand'] = 1.0 - into
        else:
            marks = ch.cum[1:-1]        # knuckle (0), the middle and end joints
            ws = []
            for k, sj in enumerate(marks):
                hw = 0.85 * ch.radii[k + 1]
                ws.append(smoothstep(sj - hw, sj + hw, s))
            cw[ch.bones[0]] = 1.0 - ws[0]
            cw[ch.bones[1]] = ws[0] * (1.0 - ws[1])
            cw[ch.bones[2]] = ws[1] * (1.0 - ws[2])
            cw[ch.bones[3]] = ws[2]
            # The palm behind the knuckles: the metacarpal toward the knuckle, the hand toward the wrist.
            back = smoothstep(-0.05, -0.012, s)
            cw = {k: v * (back if k == ch.bones[0] else 1.0) for k, v in cw.items()}
            cw['hand'] = cw.get('hand', 0) + (1.0 - back) * (1.0 - ws[0])
        fields.append(d)
        chainw.append(cw)
    D = np.stack(fields, axis=1)
    A = 1.0 / (np.maximum(D - D.min(axis=1, keepdims=True), 0.0) + 0.0025) ** 4
    A /= A.sum(axis=1, keepdims=True)
    for c, cw in enumerate(chainw):
        for bone, w in cw.items():
            add(bone, A[:, c] * w * hand)
    # The wrist inside the gauntlet goes over to the forearm.
    x = P[:, 0] / C
    fore = (1.0 - smoothstep(-3.5, -0.8, x)) * hand
    for bone in list(W):
        W[bone] = W[bone] * (1.0 - fore)
    add('lowerarm', fore)
    # The gauntlet and its concho ride the forearm; the narrow end a third on the hand.
    g = (part == 1) | (part == 2) | (part == 3)
    on_hand = (1.0 - smoothstep(0.5, 3.0, gh)) * 0.33 * g * (part != 3)
    add('hand', on_hand)
    add('lowerarm', g * (1.0 - on_hand))
    return W


def sleeve_weights(v, path):
    """The sleeve: the forearm below the elbow, the upper arm above, blended over the crook."""
    up = smoothstep(path.elbow_v - 3.5, path.elbow_v + 3.5, v)
    up = np.where(v < 0, 1.0, up)
    return {'upperarm': up, 'lowerarm': 1.0 - up}


def set_weights(obj, weights, side):
    W = {k: np.asarray(v, dtype=np.float64) for k, v in weights.items()}
    names = list(W)
    M = np.stack([W[k] for k in names], axis=1)
    M = np.where(M < 0.01, 0.0, M)
    order = np.argsort(-M, axis=1)
    keep = np.zeros_like(M, dtype=bool)
    np.put_along_axis(keep, order[:, :4], True, axis=1)
    M = np.where(keep, M, 0.0)
    M /= np.maximum(M.sum(axis=1, keepdims=True), 1e-9)
    for c, bone in enumerate(names):
        group = obj.vertex_groups.new(name=f'{bone}_{side}')
        for i in np.nonzero(M[:, c] > 1e-4)[0]:
            group.add([int(i)], float(M[i, c]), 'REPLACE')


# --------------------------------------------------------------------------------------------------- the rig
def rig_bones(rest):
    """The right arm's bones: (name, head, tail, parent, roll vector), Blender space."""
    hw = lambda p: rest.to_world(p)
    hinge = unit(np.cross(rest.elbow - rest.shoulder, rest.wrist - rest.elbow))
    back = rest.R[:, 2]
    out = [('clavicle', rest.clavicle, rest.shoulder, None, v3(0, 0, 1.0)),
           ('upperarm', rest.shoulder, rest.elbow, 'clavicle', hinge),
           ('lowerarm', rest.elbow, rest.wrist, 'upperarm', hinge),
           ('hand', rest.wrist, hw(CHAINS[1].joints[1]), 'lowerarm', back)]
    for ch in CHAINS:
        parent = 'hand'
        start = 0 if ch.name == 'thumb' else 0
        for k, bone in enumerate(ch.bones):
            head, tail = ch.joints[k], ch.joints[k + 1]
            roll = rest.R @ ch.dorsal(k)
            out.append((bone, hw(head), hw(tail), parent, roll))
            parent = bone
    return out


def mirror(p):
    return v3(-p[0], p[1], p[2])


def build_armature(rest):
    arm = bpy.data.objects.new('FP_Arms_Rig', bpy.data.armatures.new('FP_Arms_Rig'))
    bpy.context.scene.collection.objects.link(arm)
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones
    specs = rig_bones(rest)
    for side in ('r', 'l'):
        m = (lambda p: p) if side == 'r' else mirror
        for name, head, tail, parent, roll in specs:
            bone = edit.new(f'{name}_{side}')
            bone.head, bone.tail = Vector(m(head)), Vector(m(tail))
            bone.align_roll(Vector(m(roll)))
            bone.use_connect = False
            bone.use_deform = True
        for name, head, tail, parent, roll in specs:
            if parent:
                edit[f'{name}_{side}'].parent = edit[f'{parent}_{side}']
    bpy.ops.object.mode_set(mode='OBJECT')
    arm['FPArms'] = 1
    return arm


# ------------------------------------------------------------------------------------------------ the textures
def rasterize(uv, attrs, size):
    """Per-texel attributes of a mesh: uv (T, 3, 2) in 0..1, attrs (T, 3, K) per corner. Row 0 is the top (V = 1).
    Returns (size, size, K) values, a mask of covered texels and each texel's triangle."""
    K = attrs.shape[2]
    img = np.zeros((size, size, K), np.float32)
    mask = np.zeros((size, size), bool)
    tri = np.full((size, size), -1, np.int64)
    X = uv[..., 0] * size - 0.5
    Y = (1.0 - uv[..., 1]) * size - 0.5
    for t in range(len(uv)):
        xs, ys = X[t], Y[t]
        x0, x1 = max(int(math.floor(xs.min())), 0), min(int(math.ceil(xs.max())), size - 1)
        y0, y1 = max(int(math.floor(ys.min())), 0), min(int(math.ceil(ys.max())), size - 1)
        if x1 < x0 or y1 < y0:
            continue
        gx, gy = np.meshgrid(np.arange(x0, x1 + 1), np.arange(y0, y1 + 1))
        (xa, xb, xc), (ya, yb, yc) = xs, ys
        den = (yb - yc) * (xa - xc) + (xc - xb) * (ya - yc)
        if abs(den) < 1e-12:
            continue
        l0 = ((yb - yc) * (gx - xc) + (xc - xb) * (gy - yc)) / den
        l1 = ((yc - ya) * (gx - xc) + (xa - xc) * (gy - yc)) / den
        l2 = 1.0 - l0 - l1
        tol = -0.02
        inside = (l0 >= tol) & (l1 >= tol) & (l2 >= tol)
        if not inside.any():
            continue
        L = np.stack([l0[inside], l1[inside], l2[inside]], axis=1)
        L = np.clip(L, 0.0, None)
        L /= L.sum(axis=1, keepdims=True)
        vals = L @ attrs[t]
        yy, xx = gy[inside], gx[inside]
        img[yy, xx] = vals
        mask[yy, xx] = True
        tri[yy, xx] = t
    return img, mask, tri


def mesh_corners(obj):
    """Triangles of a mesh: their corners' UVs (T, 3, 2) and vertex indices (T, 3), plus vertex normals."""
    mesh = obj.data
    mesh.calc_loop_triangles()
    tris = mesh.loop_triangles
    loops = np.empty(3 * len(tris), np.int64)
    tris.foreach_get('loops', loops)
    verts = np.empty(3 * len(tris), np.int64)
    tris.foreach_get('vertices', verts)
    uv = np.empty(2 * len(mesh.loops), np.float64)
    mesh.uv_layers['UVMap'].data.foreach_get('uv', uv)
    uv = uv.reshape(-1, 2)[loops].reshape(-1, 3, 2)
    co = np.empty(3 * len(mesh.vertices))
    mesh.vertices.foreach_get('co', co)
    nor = np.empty(3 * len(mesh.vertices))
    mesh.vertices.foreach_get('normal', nor)
    return uv, verts.reshape(-1, 3), co.reshape(-1, 3), nor.reshape(-1, 3)


def texel_size(uv, idx, co):
    """Meters per texel of the atlas for this mesh (its UV islands are packed at one scale)."""
    a3 = 0.5 * np.linalg.norm(np.cross(co[idx[:, 1]] - co[idx[:, 0]], co[idx[:, 2]] - co[idx[:, 0]]), axis=1).sum()
    e1, e2 = uv[:, 1] - uv[:, 0], uv[:, 2] - uv[:, 0]
    a2 = 0.5 * np.abs(e1[:, 0] * e2[:, 1] - e1[:, 1] * e2[:, 0]).sum()
    return math.sqrt(a3 / a2) / ATLAS


def hash01(*keys):
    h = np.zeros(np.broadcast(*keys).shape, dtype=np.uint64)
    for i, k in enumerate(keys):
        h ^= (np.asarray(k).astype(np.int64).astype(np.uint64) + np.uint64(0x9E3779B97F4A7C15 + i * 0x632BE5AB)) * \
            np.uint64(0xBF58476D1CE4E5B9)
        h ^= h >> np.uint64(29)
    h *= np.uint64(0x94D049BB133111EB)
    h ^= h >> np.uint64(32)
    return (h & np.uint64(0xFFFFFF)).astype(np.float64) / float(0xFFFFFF)


def vnoise3(P, scale, seed=0):
    """Smooth value noise over 3D points (m), cells scale m across, in -1..1."""
    q = P / scale
    i = np.floor(q).astype(np.int64)
    f = q - i
    u = f * f * (3.0 - 2.0 * f)
    out = 0.0
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                w = (u[:, 0] if dx else 1 - u[:, 0]) * (u[:, 1] if dy else 1 - u[:, 1]) * (u[:, 2] if dz else 1 - u[:, 2])
                out = out + w * hash01(i[:, 0] + dx, i[:, 1] + dy, i[:, 2] + dz + seed * 7919)
    return out * 2.0 - 1.0


def fbm3(P, scale, seed=0, octaves=3):
    out, amp, total = 0.0, 1.0, 0.0
    for o in range(octaves):
        out = out + amp * vnoise3(P, scale, seed + o * 13)
        total += amp
        amp *= 0.5
        scale *= 0.5
    return out / total


def stitches(across, along, offset, pitch=3.7, length=2.7, width=0.78, slant=0.18):
    """A row of stitches offset mm from a seam (across and along in mm): the thread's cover (0..1) and its height
    (mm), and the needle holes."""
    a = along + (across - offset) * slant
    a = (a / pitch) % 1.0 * pitch - pitch * 0.5
    ax = np.maximum(np.abs(a) - (length - width) * 0.5, 0.0)
    d = np.hypot(ax, across - offset)
    cover = smoothstep(width * 0.5, width * 0.5 - 0.22, d)
    height = np.sqrt(np.clip(1.0 - (d / (width * 0.5)) ** 2, 0.0, 1.0)) * 0.22
    hole_d = np.hypot(np.abs(a) - length * 0.5 - 0.15, across - offset)
    hole = smoothstep(0.42, 0.12, hole_d) * (1.0 - cover)
    return cover, height, hole


def grain_image(size, seed, cells, strength=1.0):
    """Pebbled leather (image space): little domes between a network of fine creases. Tiles 2 x 2 over the atlas."""
    half = size // 2
    w = lt.worley((half, half), (cells, cells), seed, jitter=0.95)
    dome = np.sqrt(np.clip(w['edge'] / max(1.0, half / cells * 0.45), 0.0, 1.0))
    dome = np.tile(dome, (2, 2))
    fine = np.tile(lt.noise((half, half), seed + 1, 0.9), (2, 2))
    return (dome - 0.5) * strength + fine * 0.12 * strength


def crinkle_image(size, seed, sigma, along=None):
    """A network of thin wrinkles (ridged noise), 0 flat to 1 on a wrinkle."""
    n1 = lt.noise((size, size), seed, sigma, sigma * (along or 1.0))
    n2 = lt.noise((size, size), seed + 5, sigma * 0.55, sigma * 0.55 * (along or 1.0))
    r = np.clip(1.0 - np.abs(n1) / 0.22, 0.0, 1.0) ** 2.0 + 0.6 * np.clip(1.0 - np.abs(n2) / 0.18, 0.0, 1.0) ** 2.0
    return np.clip(r, 0.0, 1.0)


def mixc(a, b, t):
    t = np.asarray(t, dtype=np.float64)[..., None]
    return a * (1.0 - t) + b * t


def rgbv(hexv):
    return lt.rgb(hexv).astype(np.float64)


def lv(hexv):
    """A leather tone of the glove, moved with LEATHER (the tones were mixed around LEATHER_PAINTED)."""
    return rgbv(hexv) * (rgbv(LEATHER) / rgbv(LEATHER_PAINTED))


def ov(hexv):
    """An oilskin tone of the sleeve, moved with OILSKIN (the tones were mixed around OILSKIN_PAINTED)."""
    return rgbv(hexv) * (rgbv(OILSKIN) / rgbv(OILSKIN_PAINTED))


def paint_glove(obj, data):
    """The glove atlas (FPArmsGlove): painted in the hand frame from the glove's own seams, joints and wear."""
    uv, idx, co, nor = mesh_corners(obj)
    part = data['part']
    attrs = np.concatenate([co[idx], nor[idx], part[idx][..., None], data['a'][idx][..., None],
                            data['b'][idx][..., None]], axis=2)
    img, mask, _ = rasterize(uv, attrs, ATLAS)
    px_m = texel_size(uv, idx, co)
    log(f'glove atlas rasterized: {mask.mean() * 100:.0f}% covered, {px_m * 1000:.3f} mm a texel')
    T = img[mask].astype(np.float64)
    P, N = T[:, 0:3], unit(T[:, 3:6])
    prt = np.rint(T[:, 6]).astype(int)
    ea, eb = T[:, 7], T[:, 8]
    n = len(P)
    hand = prt == 0
    cm = P / C
    color = np.tile(rgbv(LEATHER), (n, 1))
    height = np.zeros(n)
    rough = np.full(n, 0.6)
    metal = np.zeros(n)
    cavity = np.zeros(n)              # darkening in creases and seams (0..1)
    scuff = np.zeros(n)               # pale, rough wear
    polish = np.zeros(n)              # dark, smooth grip wear
    thread = np.zeros(n)
    thread_h = np.zeros(n)
    tonal = np.zeros(n)               # 1 where the thread is the leather's own dark brown, 0 the pale waxed linen

    def seam(across, along, valid, rows=(-1.5, 1.5), groove=True, pitch=3.0, length=2.1, width=0.62, tone=1.0):
        """A sewn seam: rows of stitches either side of a closed seam's groove (across and along in mm). The hand's
        seams are sewn in tonal thread; the gauntlet's hems in pale linen, for show."""
        nonlocal thread, thread_h, height, cavity, tonal
        valid = valid & (np.abs(across) < 6.0)
        if not valid.any():
            return
        for off in rows:
            c, hgt, hole = stitches(across, along, off, pitch=pitch, length=length, width=width)
            thread = np.where(valid, np.maximum(thread, c), thread)
            thread_h = np.where(valid, np.maximum(thread_h, hgt), thread_h)
            cavity = np.where(valid, np.maximum(cavity, hole * 0.6), cavity)
            height = np.where(valid, height - hole * 0.1, height)
            tonal = np.where(valid & (c > 0.05), tone, tonal)
        if groove:
            g = np.exp(-(across / 0.5) ** 2)
            height = np.where(valid, height - 0.3 * g + 0.14 * np.exp(-((np.abs(across) - 1.1) / 0.8) ** 2), height)
            cavity = np.where(valid, np.maximum(cavity, 0.6 * g), cavity)

    ridges = np.zeros(n)              # the rubbed, paler crests beside creases

    def crease(d, width, depth, where, dark=0.45):
        """A fold in the leather across a line (d: mm from it): a soft valley with a ridge riding beside it."""
        nonlocal height, cavity, ridges
        valley = np.exp(-(d / width) ** 2)
        ridge = np.exp(-((d - 1.4 * width) / (0.9 * width)) ** 2)
        height = height + where * depth * (0.45 * ridge - valley)
        cavity = np.maximum(cavity, where * valley * dark)
        ridges = np.maximum(ridges, where * ridge * min(1.0, depth * 3.0))

    breakup = 0.5 + 0.5 * np.clip(fbm3(P, 0.008, 23, 2) * 2.2 + 0.3, -1, 1)   # creases fade in and out along their length

    # --- the hand: chains (fields worked out on the hand's texels only, then spread to all)
    H = np.nonzero(hand)[0]

    def full(a, fill=0.0):
        out = np.full(n, fill)
        out[H] = a
        return out

    nearest = np.full(n, -1)
    best = np.full(n, np.inf)
    for c, ch in enumerate(CHAINS):
        d = full(chain_field(P[H], ch)[0], np.inf)
        closer = hand & (d < best)
        best = np.where(closer, d, best)
        nearest = np.where(closer, c, nearest)
    body = full(body_sdf(P[H]), np.inf)
    on_body = hand & (body < best + 0.0008)
    for c, ch in enumerate(CHAINS):
        d, s, phi, rad, segk = (full(a, np.inf if i == 0 else 0.0) for i, a in enumerate(chain_field(P[H], ch)))
        mine = hand & (nearest == c) & ~on_body
        s_mm, r_mm = s * 1000.0, rad * 1000.0
        if ch.name != 'thumb':
            on = mine & (s > -0.002)
            for sign in (1.0, -1.0):
                across = r_mm * (sign * phi - math.pi / 2)
                seam(across, s_mm, on & (sign * phi > 0.3))
            # joints: deep creases across the palm side, soft wrinkles bunched over the back
            palmar = np.clip((np.abs(phi) - 1.9) / 0.6, 0.0, 1.0)
            dorsal = np.clip((1.3 - np.abs(phi)) / 0.6, 0.0, 1.0)
            for k, sj in enumerate(ch.cum[1:-1]):
                sj_mm = sj * 1000.0 + (2.5 if k == 0 else 0.0)
                for off, depth in ((-1.0, 0.3), (1.3, 0.2)):
                    crease(s_mm - sj_mm - off - 0.0003 * (phi * r_mm) ** 2, 0.6, depth, mine * palmar, dark=0.6)
                if k >= 1:
                    for w in range(3):
                        off = (w - 1.0) * 2.0 + 0.4 * math.sin(k * 3 + w)
                        bendc = 0.0015 * (phi * r_mm) ** 2
                        crease(s_mm - sj_mm - off + bendc, 0.6, 0.1 * (1.0 - 0.25 * abs(w - 1)),
                               mine * dorsal * breakup, dark=0.18)
                    scuff = np.maximum(scuff, mine * dorsal * np.exp(-((s_mm - sj_mm) / 6.0) ** 2) * 0.55)
            tip = ch.cum[-1] * 1000.0
            scuff = np.maximum(scuff, mine * smoothstep(tip - 9.0, tip + 3.0, s_mm) * 0.75)
            polish = np.maximum(polish, mine * palmar * 0.65)
        else:
            on = mine & (s > 0.012)
            for sign in (1.0, -1.0):
                across = r_mm * (sign * phi - math.pi / 2)
                seam(across, s_mm, on & (sign * phi > 0.3))
            # the keystone: the thumb sewn into the palm round its base
            ring = (s_mm - 33.0 - 6.0 * np.cos(phi + 0.6))
            seam(ring, phi * r_mm, hand & (nearest == c) & (d < 0.004), rows=(-1.6, 1.6))
            for sj in ch.cum[1:-1]:
                crease(s_mm - sj * 1000.0, 0.6, 0.2, mine * np.clip((np.abs(phi) - 1.6) / 0.6, 0, 1), dark=0.5)
            tip = ch.cum[-1] * 1000.0
            scuff = np.maximum(scuff, mine * smoothstep(tip - 9.0, tip + 3.0, s_mm) * 0.6)
            polish = np.maximum(polish, mine * np.clip((np.abs(phi) - 1.4) / 0.8, 0, 1) * 0.6)
    # --- the hand: the body (palm and back)
    x, y, z = cm[:, 0], cm[:, 1], cm[:, 2]
    back = on_body & (N[:, 2] > 0.25)
    palm = on_body & (N[:, 2] < -0.35)
    # Points: three raised cords up the back between the metacarpals, each sewn down its middle in small tonal
    # stitches, a soft channel either side.
    for i, (y1, y0) in enumerate(((2.25, 1.35), (0.0, 0.1), (-2.1, -1.2))):
        yl = y0 + (y1 - y0) * smoothstep(1.2, 6.4, x)
        across = (y - yl) * 10.0
        on = back & (x > 1.3) & (x < 6.6)
        fade = smoothstep(1.3, 2.2, x) * (1.0 - smoothstep(5.6, 6.5, x))
        cvr, hgt, hole = stitches(across, x * 10.0, 0.0, pitch=2.5, length=1.7, width=0.55, slant=0.3)
        thread = np.where(on, np.maximum(thread, cvr * fade), thread)
        thread_h = np.where(on, np.maximum(thread_h, hgt * fade), thread_h)
        tonal = np.where(on & (cvr > 0.05), 1.0, tonal)
        cavity = np.where(on, np.maximum(cavity, hole * fade * 0.5), cavity)
        cord = np.exp(-(across / 1.4) ** 2) * fade
        channel = np.exp(-((np.abs(across) - 2.0) / 0.5) ** 2) * fade
        height += on * (cord * 0.22 - channel * 0.1)
        cavity = np.maximum(cavity, on * channel * 0.3)
    # The seam down the pinky side, where the back meets the palm.
    edge = on_body & (y < -2.2) & (N[:, 1] < -0.45) & (x < 8.4) & (x > 0.6)
    seam(N[:, 2] * 11.0, x * 10.0, edge)
    # Palm creases and the grip's polish; the back's wrinkles behind the knuckles and at the wrist.
    for (x0, y0, x1, y1, depth) in ((7.4, -3.8, 8.3, 2.2, 0.32), (5.6, -3.6, 7.0, 3.4, 0.28), (2.0, 1.4, 6.6, 2.9, 0.26)):
        t = np.clip(((x - x0) * (x1 - x0) + (y - y0) * (y1 - y0)) / ((x1 - x0) ** 2 + (y1 - y0) ** 2), 0, 1)
        dist = np.hypot(x - (x0 + t * (x1 - x0)), y - (y0 + t * (y1 - y0))) * 10.0
        crease(dist, 0.8, depth, palm * np.sin(math.pi * np.clip(t, 0, 1)) ** 0.5, dark=0.55)
    polish = np.maximum(polish, palm * 0.75)
    for k in range(2):
        xk = 7.1 - k * 0.55 + 0.1 * np.sin(y * 1.7 + k)
        fade = smoothstep(-3.5, -2.3, y) * (1.0 - smoothstep(2.8, 4.0, y))
        crease((x - xk) * 10.0, 0.75, 0.16, back * fade * breakup, dark=0.3)
    for k in range(3):
        xk = 0.9 + k * 0.5 + 0.08 * np.sin(y * 2.3 + k * 2)
        crease((x - xk) * 10.0, 0.85, 0.2 * (1.0 - k / 4), on_body * (N[:, 2] > -0.2) * breakup, dark=0.32)
    knuck = np.zeros(n)
    for ch in CHAINS[:4]:
        kp = ch.joints[1] / C + v3(0, 0, 1.0)
        knuck = np.maximum(knuck, np.exp(-np.sum((cm - kp) ** 2, axis=1) / 1.1 ** 2))
    scuff = np.maximum(scuff, hand * knuck * 0.85)
    # --- the gauntlet (outside 1, inside 2) and the concho (3)
    gt = (prt == 1)
    gin = (prt == 2)
    th, gh = ea, eb
    top = gauntlet_top(th)
    ry, rz, _ = gauntlet_radii(gh)
    rmm = 0.5 * (ry + rz) * 10.0
    along_g = th * rmm
    hem = dict(groove=False, pitch=3.7, length=2.7, width=0.78, tone=0.0)
    seam((top - gh) * 10.0 - 4.5, along_g, gt, rows=(0.0,), **hem)
    seam((top - gh) * 10.0 - 8.5, along_g, gt, rows=(0.0,), **hem)
    seam(gh * 10.0 - 4.0, along_g, gt, rows=(0.0,), **hem)
    side = (np.abs(th) - math.pi) * rmm
    seam(side, gh * 10.0, gt & (gh < top - 0.2), rows=(-2.0, 2.0), groove=True, pitch=3.7, length=2.7, width=0.78,
         tone=0.0)
    border = (top - gh) * 10.0 - 14.0
    gb = np.exp(-(border / 0.55) ** 2)
    height -= gt * gb * 0.28
    cavity = np.maximum(cavity, gt * gb * 0.6)
    # stamped crescents below the border
    cres_u = (along_g / 4.2) % 1.0 * 4.2 - 2.1
    cres_v = (top - gh) * 10.0 - 17.5
    rr = np.hypot(cres_u, cres_v + 1.1)
    cres = smoothstep(0.35, 0.0, np.abs(rr - 1.5)) * (cres_v > -1.2) * (np.abs(cres_u) < 1.7)
    height -= gt * cres * 0.16
    cavity = np.maximum(cavity, gt * cres * 0.5)
    rim = smoothstep(2.5, 0.0, (top - gh) * 10.0) + smoothstep(2.0, 0.0, gh * 10.0)
    scuff = np.maximum(scuff, gt * np.clip(rim, 0, 1) * 0.7)
    flex = np.exp(-((gh - 2.5) / 1.8) ** 2) * (0.6 + 0.4 * np.sin(th * 9.0))
    crinkle_g = gt * flex
    concho = prt == 3
    # --- colour
    big = fbm3(P, 0.05, 3, 3)
    mid = fbm3(P, 0.012, 7, 3)
    # aniline leather: the dye sits unevenly, a patina in broad clouds and a finer mottle
    tone = 0.14 * big + 0.07 * mid + 0.035 * fbm3(P, 0.003, 29, 2)
    color = color * (1.0 + tone)[:, None]
    color = mixc(color, color * np.array([1.06, 0.98, 0.9]), np.clip(0.5 + big, 0, 1) * 0.5)   # warmer in the clouds
    # sweat and oil darken the palm and the inside of the fingers; sun fades the back
    color = mixc(color, lv(0x86603f), np.clip(0.4 * (N[:, 2] < -0.3) * hand, 0, 1))
    color = mixc(color, lv(0xb58d63), np.clip(0.28 * (N[:, 2] > 0.4) * hand, 0, 1))
    sc = np.clip(scuff * (0.55 + 0.45 * fbm3(P, 0.004, 11, 2) * 2.0), 0, 1)
    color = mixc(color, lv(0xc6a983), sc * 0.75)
    po = np.clip(polish * (0.6 + 0.4 * fbm3(P, 0.008, 17, 2)), 0, 1)
    color = mixc(color, lv(0x74502f), po * 0.55)
    gtone = (gt | gin)
    color = mixc(color, lv(0xa9805a), gtone * 0.35)
    color = mixc(color, lv(0xa38667), gin * 0.85)               # the suede inside
    color = mixc(color, lv(0x4a3222), np.clip(cavity, 0, 1) * 0.6)
    color = mixc(color, lv(0xc9ab84), np.clip(ridges, 0, 1) * 0.3)
    color = np.where(gtone[:, None], color * GAUNTLET_TONE, color)   # the cuff lands on Ellis's (GAUNTLET_TONE)
    linen = mixc(np.tile(rgbv(0xdccaa2), (n, 1)), rgbv(0x9a8466), np.clip(polish + cavity * 0.3, 0, 1) * 0.6)
    tcol = mixc(linen, lv(0x5a3d27), tonal)
    color = mixc(color, tcol, thread)
    # the concho: brass, tarnished in its engraving
    rho, psi = ea, eb
    petals = np.cos(8.0 * psi) * 0.5 + 0.5
    engr = concho * ((smoothstep(0.06, 0.0, np.abs(rho - 0.72)) + smoothstep(0.05, 0.0, np.abs(rho - 0.35))) +
                     smoothstep(0.1, 0.0, np.abs(rho - (0.42 + 0.22 * petals))) * (rho > 0.38) * (rho < 0.7))
    engr = np.clip(engr, 0, 1)
    brass = mixc(np.tile(rgbv(0xcda25a), (n, 1)), rgbv(0x5a4428), engr * 0.85)
    brass = mixc(brass, rgbv(0x8a6d3e), np.clip(0.3 + 0.3 * mid, 0, 1) * smoothstep(0.85, 1.0, rho))
    color = np.where(concho[:, None], brass, color)
    metal = np.where(concho, 1.0, 0.0)
    # --- roughness
    rough = 0.52 + 0.06 * big + 0.16 * sc - 0.16 * po + 0.1 * cavity
    rough = np.where(thread > 0.5, 0.72, rough)
    rough = np.where(gin, 0.9, rough)
    rough = np.where(concho, 0.3 + 0.35 * engr + 0.1 * smoothstep(0.85, 1.0, rho), rough)
    height = height + thread_h * thread - 0.25 * concho * engr
    height = height + 0.04 * crinkle_g
    # --- images
    img_c = np.zeros((ATLAS, ATLAS, 3))
    img_c[mask] = color
    img_h = np.zeros((ATLAS, ATLAS))
    img_h[mask] = height * 0.001
    img_r = np.zeros((ATLAS, ATLAS))
    img_r[mask] = rough
    img_m = np.zeros((ATLAS, ATLAS))
    img_m[mask] = metal
    img_a = np.zeros((ATLAS, ATLAS))
    img_a[mask] = 1.0 - 0.35 * np.clip(cavity, 0, 1)
    # Image-space detail: the leather's pebbled grain and fine crinkle (the concho and the suede keep their own).
    leather = np.zeros((ATLAS, ATLAS), bool)
    leather[mask] = ~concho & ~gin
    grain = grain_image(ATLAS, 41, 230)
    crinkle = crinkle_image(ATLAS, 43, 5.0)
    img_h += leather * (grain * 0.00011 - crinkle * 0.0001)
    img_c *= (1.0 - leather * (0.08 * crinkle - 0.04 * grain))[..., None]
    img_r += leather * 0.05 * crinkle
    write_set(GLOVE_SET, img_c, img_h, img_r, img_m, img_a, mask, px_m, strength=1.0)


def paint_duster(obj, data):
    """The duster atlas (FPArmsDuster): the sleeve unrolled by its own layout, painted from its folds and seams."""
    path, scale = data['path'], data['scale']
    size = ATLAS
    rows, cols = np.mgrid[0:size, 0:size]
    U = (cols + 0.5) / size
    V = 1.0 - (rows + 0.5) / size
    v = (V - 0.03) / scale
    ry, rz = sleeve_radii(np.clip(v, 0, path.length), path)
    circ = math.pi * (3 * (ry + rz) - np.sqrt((3 * ry + rz) * (ry + 3 * rz)))
    ts = ((U - 0.5) / (circ * scale) + 0.5) * 2 * math.pi
    sleeve = (v >= -0.3) & (v <= path.length + 0.3) & (ts >= -0.05) & (ts <= 2 * math.pi + 0.05)
    lining = np.hypot(U - 0.075, V - 0.075) < 0.075
    mask = sleeve | lining
    tg = ts - math.pi
    vc = np.clip(v, 0.0, path.length)
    fold_at = lambda t_, v_: sleeve_ring(t_, v_, path)[2]
    _, _, fold, outside = sleeve_ring(tg, vc, path)
    rmm = 0.5 * (ry + rz) * 10.0
    u_mm = ts * rmm
    v_mm = v * 10.0
    px_m = 1.0 / (scale * size * 100.0)
    # curvature of the folds: crests (positive) wear chalky, valleys darken
    eps = 0.15
    lap = (fold_at(tg, vc + eps) + fold_at(tg, vc - eps) - 2 * fold) / eps ** 2
    lap_t = (fold_at(tg + 0.05, vc) + fold_at(tg - 0.05, vc) - 2 * fold) / (0.05 * rmm / 10) ** 2
    crest = np.clip(-(lap + lap_t) * 0.35, 0, 1)
    valley = np.clip((lap + lap_t) * 0.35, 0, 1)
    # Matches the new Ellis's duster sleeves (Art/Models/Characters/Ellis.py's 'Duster' look, measured from his baked
    # set on 2026-10-15, the faces weighted to the arms): the coat colour, waxed. The dressing darkens and warms the
    # cotton; where it's worn off it goes back chalky. OILSKIN is chosen with the 0.84 wax in, so the sleeve's mean
    # lands on his sleeves'.
    color = np.tile(rgbv(OILSKIN) * 0.84, (size, size, 1))
    height = np.zeros((size, size))
    cavity = np.zeros((size, size))
    thread = np.zeros((size, size))
    thread_h = np.zeros((size, size))
    # mottled wax: darker where it's heavy (and glossier), lighter where it's worn
    wax = lt.noise((size, size), 61, 90.0, octaves=3)
    wear = lt.noise((size, size), 63, 40.0, octaves=3)
    color = mixc(color, ov(0x5b4331), np.clip(0.5 + 0.35 * wax, 0, 1) * 0.75)
    chalk = np.clip(crest * 1.3 + 0.25 * np.clip(wear, 0, 1), 0, 1)
    color = mixc(color, ov(0x9c846a), chalk * 0.55)
    color = mixc(color, ov(0x33251b), valley * 0.6)
    # the crook of the elbow and the forearm's top rubbed paler
    rub = np.exp(-((v - path.elbow_v) / 5.0) ** 2) * np.clip(np.cos(tg - path.inner_theta - math.pi), 0, 1)
    color = mixc(color, ov(0xa08a70), np.clip(rub * (0.5 + 0.5 * wear), 0, 1) * 0.6)
    # crackled wax: thin pale lines, more on the folds
    crk = crinkle_image(size, 67, 3.2)
    crk2 = crinkle_image(size, 69, 7.0, along=2.5)
    lines = np.clip(crk * (0.35 + 0.9 * crest) + crk2 * 0.25, 0, 1)
    color = mixc(color, ov(0xb29a7e), lines * 0.45)
    height -= lines * 0.00016
    # twill weave
    tw = np.sin((cols + rows) * (2 * math.pi / 3.2)) * 0.5 + 0.5
    tw2 = np.sin((cols - rows * 0.2) * (2 * math.pi / 9.0))
    height += (tw * 0.00003 + tw2 * 0.00001)
    color *= (1.0 - 0.04 * tw)[..., None]
    # the cloth's own soft rumple, so a highlight breaks up over it instead of sliding like a sheen on plastic
    rumple = lt.noise((size, size), 71, 7.0, octaves=2)
    height += rumple * 0.00007

    def seam(across, along, valid, rows_=(-3.0, -7.0, 3.0, 7.0), groove=0.35):
        nonlocal thread, thread_h, cavity, height
        for off in rows_:
            cvr, hgt, hole = stitches(across, along, off, pitch=3.4, length=2.6, width=0.75, slant=0.0)
            thread = np.where(valid, np.maximum(thread, cvr), thread)
            thread_h = np.where(valid, np.maximum(thread_h, hgt), thread_h)
            cavity = np.where(valid, np.maximum(cavity, hole * 0.6), cavity)
        g = np.exp(-(across / 0.6) ** 2)
        height = np.where(valid, height - groove * 0.001 * g + 0.00025 * np.exp(-((np.abs(across) - 4.5) / 3.0) ** 2), height)
        cavity = np.where(valid, np.maximum(cavity, g * 0.8), cavity)

    # felled seams: the underarm (at the island's edges) and the back of the arm
    seam(np.minimum(u_mm, 2 * math.pi * rmm - u_mm), v_mm, sleeve & (v > path.rim_v - 2.0), rows_=(3.0, 7.0))
    seam((ts - math.pi) * rmm, v_mm, sleeve & (v > path.rim_v - 2.0))
    color = mixc(color, ov(0x4b3a2c), thread * 0.85)
    height += thread * thread_h * 0.001
    color = mixc(color, ov(0x36281e), np.clip(cavity, 0, 1) * 0.5)
    color = np.where(lining[..., None], ov(0x3a302a) * (1.0 + 0.1 * wax[..., None]), color)
    # inside the gauntlet: darker, it never sees the light
    color *= (0.55 + 0.45 * outside)[..., None]
    rough = 0.72 - 0.08 * wax + 0.16 * chalk + 0.06 * lines + 0.06 * cavity
    rough = np.where(thread > 0.5, 0.75, rough)
    rough = np.where(lining, 0.9, rough)
    height = height + fold * 0.0
    ao = 1.0 - 0.35 * np.clip(cavity + valley * 0.6, 0, 1)
    write_set(DUSTER_SET, color, height, rough, np.zeros((size, size)), ao, mask, px_m, strength=1.0)


def write_set(name, color, height, rough, metal, ao, mask, px_m, strength=1.0):
    """Fills the gaps between UV islands (so filtering and mips never pull in black) and writes the set."""
    m = mask.astype(np.float32)
    color = lt.fill_transparent(np.clip(color, 0, 1).astype(np.float32), m)
    others = np.stack([height / 0.002 + 0.5, rough, metal], axis=-1).astype(np.float32)
    others = lt.fill_transparent(others, m)
    ao3 = lt.fill_transparent(np.stack([ao, ao, ao], axis=-1).astype(np.float32), m)[..., 0]
    h = (others[..., 0] - 0.5) * 0.002
    occ = np.clip(ao3 * lt.occlusion(h, px_m, radii_px=(2, 5), strength=0.6), 0, 1)
    lt.save_set(name, color, h, px_m, np.clip(others[..., 1], 0.05, 1.0), np.clip(others[..., 2], 0, 1),
                normal_strength=strength, ao=occ)


# ------------------------------------------------------------------------------------------------- the build
def mirrored_copy(obj, name):
    """The left side: the object mirrored across the eye's vertical plane, its normals and groups following."""
    copy = obj.copy()
    copy.data = obj.data.copy()
    copy.name = copy.data.name = name
    bpy.context.scene.collection.objects.link(copy)
    copy.data.transform(Matrix.Scale(-1.0, 4, (1.0, 0.0, 0.0)))
    copy.data.flip_normals()
    for group in copy.vertex_groups:
        if group.name.endswith('_r'):
            group.name = group.name[:-2] + '_l'
    return copy


def rig_json(arm, rest):
    """The rest pose for the code, in Unreal's component space (cm): every bone's head and tail, each hand's palm frame
    and grip point."""
    def ue(p):
        p = np.asarray(p)
        return [round(float(-p[1] * 100.0), 3), round(float(-p[0] * 100.0), 3), round(float(p[2] * 100.0), 3)]

    def ue_dir(d):
        d = unit(np.asarray(d))
        return [round(float(-d[1]), 5), round(float(-d[0]), 5), round(float(d[2]), 5)]

    bones = []
    for bone in arm.data.bones:
        bones.append(dict(name=bone.name, parent=bone.parent.name if bone.parent else 'root', head=ue(bone.head_local),
                          tail=ue(bone.tail_local)))
    hands = {}
    for side, m in (('r', lambda p: p), ('l', mirror)):
        R = rest.R
        hands[side] = dict(
            Wrist=ue(m(rest.wrist)), FingerDir=ue_dir(m(R[:, 0])), ThumbDir=ue_dir(m(R[:, 1])),
            PalmNormal=ue_dir(m(-R[:, 2])), GripPoint=ue(m(rest.to_world(GRIP_POINT * C))),
            HandleAxis=ue_dir(m(R @ HANDLE_AXIS)),
            RestCurlDegrees=dict(knuckle=REST_CURL[0], middle=REST_CURL[1], end=REST_CURL[2]))
    out = dict(note='SK_FP_Arms rest (bind) pose, Unreal component space (cm), root at the eye. Curl a finger by '
                    'turning each of its bones toward PalmNormal (about the axis across the finger). A held handle '
                    'runs along HandleAxis through GripPoint (the middle of a 4 cm handle in the closed fist).',
               bones=bones, hands=hands, upperarm_cm=round(rest.upper * 100, 2), lowerarm_cm=round(rest.fore * 100, 2),
               lowerarm_body_cm=round(rest.fore_body * 100, 2), forearm_stretch=FOREARM_STRETCH)
    os.makedirs(WORK, exist_ok=True)
    path = os.path.join(WORK, 'FPArms_rig.json')
    with open(path, 'w') as fh:
        json.dump(out, fh, indent=1)
    log(f'rig written to {os.path.relpath(path, lt.REPO)}')


def build():
    rest = Rest()
    log(f'rest: shoulder {np.round(rest.shoulder * 100, 1)} cm, elbow {np.round(rest.elbow * 100, 1)}, wrist '
        f'{np.round(rest.wrist * 100, 1)}; upper arm {rest.upper * 100:.1f} cm, forearm {rest.fore * 100:.1f} cm')
    glove, gdata = build_glove()
    sleeve, sdata = build_sleeve(rest)
    if 'keep-textures' not in ARGV:
        paint_glove(glove, gdata)
        paint_duster(sleeve, sdata)
    # The masters Ellis's own gloves and duster use (his GAME_MATERIALS), so first and third person shade alike.
    glove_mat = lt.material(GLOVE_SET, name='FPArms_Glove', master='Character')
    duster_mat = lt.material(DUSTER_SET, name='FPArms_Duster', master='CharacterCloth', FuzzStrength=0.2, FuzzLift=0.08)
    glove.data.materials.append(glove_mat)
    sleeve.data.materials.append(duster_mat)
    # weights, then into the rest pose
    co = np.empty(3 * len(glove.data.vertices))
    glove.data.vertices.foreach_get('co', co)
    set_weights(glove, hand_weights(co.reshape(-1, 3), gdata['part'], gdata['b']), 'r')
    glove.data.transform(rest.matrix())
    set_weights(sleeve, sleeve_weights(sdata['v'], sdata['path']), 'r')
    arm = build_armature(rest)
    parts = [glove, sleeve, mirrored_copy(glove, 'Glove_l'), mirrored_copy(sleeve, 'Sleeve_l')]
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    body = bpy.context.view_layer.objects.active
    body.name = body.data.name = 'FP_Arms'
    body.parent = arm
    body.modifiers.new('Armature', 'ARMATURE').object = arm
    body['Nanite'] = 0
    lt.bake_vertex_ao(body, samples=24, distance=0.04, ground=False, strength=0.45)
    tris = sum(len(p.vertices) - 2 for p in body.data.polygons)
    log(f'SK_FP_Arms: {tris} triangles, {len(body.data.vertices)} vertices, {len(arm.data.bones)} bones, materials '
        f'{", ".join(m.name for m in body.data.materials)}')
    rig_json(arm, rest)
    return arm, body, rest


ARM, BODY, REST = build()

if lt.want_preview():
    lt.preview([BODY], os.path.join(lt.PREVIEW_DIR, 'Characters', 'FPArms_rest.png'), view=(-0.4, 1.0, 0.6),
               ground=False, lens=50.0, fit=0.9)
