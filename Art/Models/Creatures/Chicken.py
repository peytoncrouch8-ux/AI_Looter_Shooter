"""Chickens for Ransom's Rest's yards (the 2026-10-14 "make it alive" polish; look sheet 44): a speckled brown hen, a white
hen and a red rooster, as small rigs the game poses in code (UCreaturePoseAnimInstance), so the town feels lived in while
its people stay unseen. Stylized realism like Hob and the fauna's crows: one smooth skin cast onto a soft sculpt
(Tools/Blender/looter_animals.py), painted feathers on it, shaped feather cards where the silhouette needs them (the neck's
hackles, the folded wings, the tail, the rooster's saddle and sickles), a comb, wattles, earlobes, a beak in two halves,
eyes, scaled legs and clawed toes.
Refined the same day ("slightly more realistic"): the skins painted as real plumage (overlapping feathers that shade the
ones they lie over, la.plumage), feather sheets with barbs and a rachis instead of stripes, the tails closed fans at a
hen's 45 degrees with the coverts lying on the cushion, soft fluff over the drumsticks and under the tail, smaller hen
heads, fleshier combs with rounded points, longer wattles, knuckled toes and rounder shanks.

Models (/Game/Art/Creatures; skeletal; no hit zones, so shots pass through them, as through Hob):
  SK_HenBrown   a speckled brown hen, 43 cm to the top of her comb (measured, as first built), 4,714 triangles
  SK_HenWhite   the same hen in white (the same mesh on the white plumage's part of the atlas), 4,714 triangles
  SK_Rooster    a black-breasted red rooster, 55 cm to the top of his comb, big comb and wattles, sickles, spurs,
                5,669 triangles
One material slot each, MI_ChickenAtlas (the World master), on the texture set Art/Textures/ChickenAtlas (2048 px:
color, normal, occlusion-roughness-metal), painted by this script every run (deterministic). The two hens are two meshes
rather than one tinted instance: a tint can't whiten brown feathers and keep the comb red and the legs yellow.

Pivot: the ground between the feet; the front faces Blender -Y (Unreal +X). LODs 50% and 25%.

Bones (parent), the same names on all three:
  body (root bone's child; at the hips, the bird's middle)
  neck_1 (body), neck_2 (neck_1), head (neck_2), jaw (head: the lower beak), wattles (head)
  tail (body), tail_tip (tail: the tail feathers' outer halves; the rooster's sickles)
  per side l/r: thigh (body; hip to knee, inside the body), shin (thigh; the feathered drumstick, knee to hock),
    shank (shin; the bare scaled leg, hock to the foot), toes (shank; the three front toes), hallux (shank; the hind toe)
  per side l/r: wing (body; at the shoulder, along the folded wing's top edge: the hinge it opens about), wing_hand (wing)
    and wing_tip (wing): both at the wrist (the folded wing's front-bottom), carrying the primaries; turned about the
    wing's outward axis they fan the primaries open (wing_hand half as far as wing_tip).
The pose table (rest, alert, peck, scratch, walk, flutter, sit and the rooster's crow), in Unreal's component space as
Amos's (Tools/amos_poses.py's format), goes to Intermediate/AnimalsModel/Hen_poses.json and Rooster_poses.json.

    Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Chicken.py -Preview      (renders into Saved/ArtPreviews/RansomsRest/Animals)
    -ScriptArgs check[,pose=peck]   each bird from four sides (work only); only=Rooster builds one bird (its pose table
    isn't written); skinonly, relax0 and diag look at the cast skin alone, unrelaxed, and log its rings; previews=<folder>
    renders into a folder of Animals (the refined pass's are in Animals/Refined).
The wings are built and baked for occlusion apart from the body, so the side they cover isn't baked dark (it shows when
they open), then joined in.
"""
import math
import os

import numpy as np
from mathutils import Matrix, Vector

import looter_animals as la
import looter_creatures as lc
import looter_textures as lt

ARGV = la.args()
PREVIEW = '--preview' in ARGV or lt.want_preview()
SET = 'ChickenAtlas'
SIZE = (2048, 2048)
PX_M, NORMAL_STRENGTH = 0.0004, 1.0
# Where each part lies in the atlas (u0, u1, v0, v1): a skin per plumage (latitude-longitude), a feather sheet per
# plumage (eight strips of feathers, STRIPS), and the bare parts every chicken shares.
ATLAS = {
    'brown_skin': (0.0, 0.5, 0.625, 1.0), 'white_skin': (0.5, 1.0, 0.625, 1.0), 'rooster_skin': (0.0, 0.5, 0.25, 0.625),
    'brown_feathers': (0.5, 0.75, 0.25, 0.625), 'white_feathers': (0.75, 1.0, 0.25, 0.625),
    'rooster_feathers': (0.0, 0.5, 0.0, 0.25),
    'comb': (0.5, 0.625, 0.125, 0.25), 'beak': (0.625, 0.75, 0.1875, 0.25), 'leg': (0.625, 0.875, 0.125, 0.1875),
    'eye': (0.875, 0.9375, 0.1875, 0.25), 'earlobe_red': (0.9375, 1.0, 0.1875, 0.25),
    'earlobe_white': (0.875, 0.9375, 0.125, 0.1875), 'claw': (0.9375, 1.0, 0.125, 0.1875),
}
STRIPS = ('hackle', 'saddle', 'covert', 'fluff', 'secondary', 'primary', 'tail', 'sickle')


def strip(kind, k=0, of=1):
    """The U range of a feather strip (k of `of` side by side within it, for variety)."""
    i = STRIPS.index(kind)
    u0, u1 = i / 8.0, (i + 1) / 8.0
    return (u0 + (u1 - u0) * k / of, u0 + (u1 - u0) * (k + 1) / of)


# --- The birds' shapes (creature space: cm, x forward, y left, z up, the origin on the ground between the feet) ---

HEN = dict(
    name='Hen',
    sculpt=[((-2.0, 0.0, 21.0), (14.0, 9.4, 9.2), (0.0, 8.0, 0.0)),      # the body
            ((7.0, 0.0, 20.6), (8.4, 8.4, 9.4), (0.0, 0.0, 0.0)),       # the breast
            ((-1.5, 0.0, 16.8), (11.0, 8.2, 6.0), (0.0, 0.0, 0.0)),     # the fluffy underbody over the thighs
            ((-12.0, 0.0, 24.6), (7.6, 7.0, 7.0), (0.0, -12.0, 0.0)),   # the rump cushion rising to the tail
            ((-15.2, 0.0, 28.2), (4.0, 3.3, 4.4), (0.0, -42.0, 0.0)),   # the tail's base
            ((4.5, 0.0, 26.0), (6.4, 7.2, 5.0), (0.0, 0.0, 0.0)),       # the shoulders
            ((12.4, 0.0, 38.2), (3.0, 2.2, 2.5), (0.0, -6.0, 0.0)),     # the skull (a hen's head is small)
            ((14.1, 0.0, 37.9), (1.85, 1.6, 1.7), (0.0, 0.0, 0.0)),     # the face
            ((12.8, 0.0, 35.9), (1.9, 1.7, 1.6), (0.0, 0.0, 0.0))],     # the throat
    neck=((7.8, 0.0, 26.8), (10.3, 0.0, 32.2), (11.6, 0.0, 36.0)), neck_r=(4.9, 3.6, 2.6),
    spine=((15.3, 0.0, 38.0), (13.3, 0.0, 38.4), (11.5, 0.0, 37.6), (10.8, 0.0, 34.8), (9.8, 0.0, 31.2),
           (7.5, 0.0, 26.8), (3.5, 0.0, 22.5), (-3.5, 0.0, 21.5), (-10.5, 0.0, 24.2), (-14.9, 0.0, 28.0)),
    rings=30, around=22, zones=(0.24, 0.58, 0.92), head_center=(12.6, 0.0, 38.2), face_center=(14.1, 0.0, 37.9),
    body_bone=(-1.0, 0.0, 21.0),
    legs=dict(hip=(-0.5, 4.4, 19.0), knee=(3.0, 5.0, 14.0), hock=(-0.3, 5.1, 8.6), foot=(0.9, 4.8, 1.05),
              toes=((28.0, 4.3), (0.0, 5.5), (-24.0, 4.0)), hallux=2.3, shank=(0.84, 0.7), toe=0.48, spur=0.0,
              drum=((3.2, 2.8), (2.5, 2.2), (1.2, 1.1))),
    wing=dict(top=((6.6, 27.6), (-11.6, 25.0)), joint=(5.0, 6.4, 26.6), wrist=(3.4, 8.3, 21.4), k=1.0),
    # A hen's tail: a closed fan carried at about 45 degrees, its two halves a narrow roof.
    tail=dict(base=(-14.4, 0.0, 28.6), count=6, elev=50.0, elev_step=3.6, out=2.0, out_step=2.4, length=13.2,
              length_step=-0.55, width=3.5, coverts=7, roof=0.55),
    comb=dict(front=15.0, back=10.6, points=5, height=2.1, thick=(0.42, 0.32, 0.16)),
    wattle=dict(at=(14.5, 0.6, 35.4), radii=(1.0, 0.38, 1.45)),
    earlobe=dict(dir=(0.0, 1.0, -0.55), radii=(0.5, 0.09, 0.4)),
    eye=dict(dir=(0.38, 1.0, 0.24), r=0.6),
    beak=dict(length=2.6, width=0.8, height=0.78, droop=1.1),
    hackles=dict(t=(0.27, 0.6), rows=7, per_row=12, length=(2.8, 4.4), width=0.85, spread=120.0, stand=0.06),
    saddle=None, sickles=None,
)

ROOSTER = dict(
    name='Rooster',
    sculpt=[((-1.5, 0.0, 26.2), (15.2, 10.2, 10.2), (0.0, 0.0, 0.0)),
            ((8.6, 0.0, 26.8), (9.4, 9.4, 10.6), (0.0, -10.0, 0.0)),
            ((-1.0, 0.0, 21.0), (12.0, 9.0, 6.6), (0.0, 0.0, 0.0)),
            ((-13.2, 0.0, 29.6), (8.0, 7.4, 7.6), (0.0, -16.0, 0.0)),
            ((-16.6, 0.0, 33.6), (4.4, 3.8, 5.2), (0.0, -50.0, 0.0)),
            ((5.6, 0.0, 32.0), (7.0, 8.0, 5.6), (0.0, -8.0, 0.0)),
            ((13.5, 0.0, 47.6), (3.5, 2.5, 2.85), (0.0, -6.0, 0.0)),
            ((15.5, 0.0, 47.2), (2.15, 1.85, 1.95), (0.0, 0.0, 0.0)),
            ((13.9, 0.0, 44.9), (2.2, 1.95, 1.9), (0.0, 0.0, 0.0))],
    neck=((8.8, 0.0, 33.4), (11.2, 0.0, 40.0), (12.6, 0.0, 45.2)), neck_r=(5.9, 4.3, 3.0),
    spine=((16.8, 0.0, 47.3), (14.6, 0.0, 47.8), (12.5, 0.0, 46.7), (11.7, 0.0, 43.2), (10.6, 0.0, 38.8),
           (8.4, 0.0, 33.4), (4.0, 0.0, 28.0), (-3.5, 0.0, 26.6), (-11.8, 0.0, 29.0), (-16.4, 0.0, 33.4)),
    rings=32, around=22, zones=(0.24, 0.58, 0.92), head_center=(13.8, 0.0, 47.5), face_center=(15.5, 0.0, 47.2),
    body_bone=(-0.8, 0.0, 26.0),
    legs=dict(hip=(-0.4, 5.0, 23.8), knee=(3.6, 5.8, 17.6), hock=(-0.4, 5.9, 11.6), foot=(1.0, 5.4, 1.2),
              toes=((28.0, 5.0), (0.0, 6.4), (-24.0, 4.7)), hallux=2.8, shank=(1.06, 0.86), toe=0.56, spur=1.7,
              drum=((3.7, 3.2), (2.9, 2.6), (1.45, 1.35))),
    wing=dict(top=((7.6, 33.8), (-12.6, 30.4)), joint=(5.8, 7.0, 32.8), wrist=(4.2, 9.2, 26.6), k=1.15, bar='fluff'),
    tail=dict(base=(-16.0, 0.0, 34.0), count=6, elev=62.0, elev_step=4.2, out=3.0, out_step=2.6, length=17.0,
              length_step=-0.8, width=3.4, coverts=8, roof=0.5),
    comb=dict(front=16.2, back=10.0, points=5, height=4.8, thick=(0.62, 0.48, 0.22)),
    wattle=dict(at=(15.6, 0.75, 43.0), radii=(1.6, 0.5, 2.9)),
    earlobe=dict(dir=(-0.05, 1.0, -0.6), radii=(0.6, 0.09, 0.5)),
    eye=dict(dir=(0.38, 1.0, 0.24), r=0.68),
    beak=dict(length=3.0, width=0.92, height=0.9, droop=1.25),
    hackles=dict(t=(0.25, 0.68), rows=9, per_row=13, length=(4.4, 9.4), width=0.95, spread=110.0, stand=0.06),
    saddle=dict(t=(0.76, 0.92), rows=4, per_row=12, length=(5.0, 7.8), width=0.95, spread=(30.0, 88.0), stand=0.06),
    sickles=dict(main=31.0, lesser=(22.0, 19.5, 16.5), width=2.8),
)

PLUMAGES = (('HenBrown', HEN, 'brown', 'earlobe_red'), ('HenWhite', HEN, 'white', 'earlobe_white'),
            ('Rooster', ROOSTER, 'rooster', 'earlobe_red'))


def bone_specs(spec):
    """(name, head, tail, parent) in creature space."""
    V = Vector
    n0, n1, n2 = (V(p) for p in spec['neck'])
    face = V(spec['face_center'])
    body = V(spec['body_bone'])
    tb = V(spec['tail']['base'])
    up_back = V((-0.55, 0.0, 0.83))
    beak_root = V((face.x + 0.8, 0.0, face.z - 0.4))
    specs = [('body', body, body + V((8.0, 0.0, 0.0)), None),
             ('neck_1', n0, n1, 'body'), ('neck_2', n1, n2, 'neck_1'),
             ('head', n2, face + V((1.5, 0.0, 0.0)), 'neck_2'),
             ('jaw', beak_root, beak_root + V((spec['beak']['length'], 0.0, -0.7)), 'head'),
             ('wattles', V((face.x + 0.4, 0.0, face.z - 2.0)), V((face.x + 0.4, 0.0, face.z - 4.0)), 'head'),
             ('tail', tb, tb + up_back * 6.0, 'body'), ('tail_tip', tb + up_back * 5.0, tb + up_back * 10.0, 'tail')]
    L = spec['legs']
    W = spec['wing']
    (sx, sz), (rx, rz) = W['top']
    hinge = V((rx - sx, 0.0, rz - sz)).normalized()
    for side, sfx in la.SIDES:
        m = lambda p: V((p[0], side * p[1], p[2]))
        foot = m(L['foot'])
        specs += [(f'thigh_{sfx}', m(L['hip']), m(L['knee']), 'body'),
                  (f'shin_{sfx}', m(L['knee']), m(L['hock']), f'thigh_{sfx}'),
                  (f'shank_{sfx}', m(L['hock']), foot, f'shin_{sfx}'),
                  (f'toes_{sfx}', foot, foot + V((5.0, 0.0, -0.6)), f'shank_{sfx}'),
                  (f'hallux_{sfx}', foot, foot + V((-2.5, 0.0, -0.6)), f'shank_{sfx}')]
        joint, wrist = m(W['joint']), m(W['wrist'])
        specs += [(f'wing_{sfx}', joint, joint + hinge * 12.0 * W['k'], 'body'),
                  (f'wing_hand_{sfx}', wrist, wrist + V((-8.0, 0.0, -1.0)) * W['k'], f'wing_{sfx}'),
                  (f'wing_tip_{sfx}', wrist, wrist + V((-12.0, 0.0, -1.6)) * W['k'], f'wing_{sfx}')]
    return specs


def skin_weights(spec):
    """The skin's bones by how far along the spine (t: 0 the face, 1 the tail's base)."""
    z_head, z_neck, z_tail = spec['zones']

    def at(t, p):
        head = 1.0 - la.smooth(z_head - 0.05, z_head + 0.05, t)
        mid = (z_head + z_neck) * 0.5
        neck_2 = la.smooth(z_head - 0.05, z_head + 0.05, t) * (1.0 - la.smooth(mid - 0.05, mid + 0.05, t))
        neck_1 = la.smooth(mid - 0.05, mid + 0.05, t) * (1.0 - la.smooth(z_neck - 0.04, z_neck + 0.07, t))
        tail = 0.7 * la.smooth(z_tail - 0.02, 1.0, t)
        body = max(0.0, 1.0 - head - neck_2 - neck_1 - tail)
        return {k: v for k, v in (('head', head), ('neck_2', neck_2), ('neck_1', neck_1), ('body', body),
                                   ('tail', tail)) if v > 1e-3}
    return at


# --- Building one bird ---

class Bird:
    def __init__(self, name, spec, plumage, earlobe):
        self.name, self.spec, self.plumage, self.earlobe = name, spec, plumage, earlobe
        self.skin_region, self.feathers = f'{plumage}_skin', f'{plumage}_feathers'
        self.part = la.Part(name, ATLAS, SIZE)
        # The wings are built apart and baked for occlusion apart, so the side they cover isn't baked dark: it shows
        # when they open.
        self.wing_part = la.Part(name + '_Wings', ATLAS, SIZE)
        elements = [la.element(c, s, r) for c, s, r in spec['sculpt']]
        n = [Vector(p) for p in spec['neck']]
        for t in (0.0, 0.25, 0.5, 0.75, 1.0):
            k = min(int(t * 2), 1)
            f = t * 2 - k
            c = n[k].lerp(n[k + 1], f)
            r = la.lerp(spec['neck_r'][0], spec['neck_r'][2], t) if t > 0 else spec['neck_r'][0]
            elements.append(la.element(c, (r, r * 0.92, r)))
        self.sculpt = la.Sculpt(elements)
        self.build()

    # The skin: face to tail base, one surface.
    def build_skin(self):
        spec = self.spec
        S = spec['rings']
        ts = [i / (S - 1) for i in range(S)]

        def up_at(t):
            return Vector((0.15, 0.0, 1.0)) if t < 0.2 else la.UP
        self.skin = la.cast_skin(self.part, self.sculpt, spec['spine'], ts, around=spec['around'], up_at=up_at,
                                 weights_at=skin_weights(spec), region=self.skin_region, cap_push=0.2,
                                 relax=0 if 'relax0' in ARGV else 8, reach=2.6)
        self.field = la.WeightField(self.part)
        if 'diag' in ARGV:
            N = spec['around']
            for i, (c, ring) in enumerate(zip(self.skin['centers'], self.skin['rings'])):
                fmt = lambda v: f'({v.x:.1f},{v.y:.1f},{v.z:.1f})'
                la.log(f'ring {i} t {ts[i]:.2f} c {fmt(c)} top {fmt(ring[0])} left {fmt(ring[N // 4])} '
                       f'down {fmt(ring[N // 2])} downfront {fmt(ring[N // 2 - 2])} {fmt(ring[N // 2 + 2])}')

    def ring_point(self, t, angle):
        """A point on the skin at t along and angle (degrees from the back, toward the left), with its normal and the
        direction back along the skin there."""
        skin = self.skin
        ts, rings, centers, N = skin['ts'], skin['rings'], skin['centers'], skin['around']
        i = min(range(len(ts)), key=lambda k: abs(ts[k] - t))
        jf = (angle / 360.0 * N) % N
        j0 = int(math.floor(jf))
        f = jf - j0

        def at(row):
            return rings[row][j0 % N].lerp(rings[row][(j0 + 1) % N], f)
        p = at(i)
        nxt = at(min(i + 2, len(ts) - 1)) if i < len(ts) - 2 else p + (p - at(i - 2))
        normal = (p - centers[i]).normalized()
        along = nxt - p
        along = (along - normal * along.dot(normal)).normalized()
        return p, normal, along

    def weigh(self, point_fn):
        return lambda t: self.field.at(point_fn(t, 0.0, 0))

    def build_head(self):
        spec, part = self.spec, self.part
        head_w = {'head': 1.0}
        face = Vector(spec['face_center'])
        hit, _ = self.sculpt.cast(face, (1.0, 0.0, 0.0))
        fx = hit.x if hit is not None else face.x + 1.8
        bk = spec['beak']
        zb = face.z - 0.15
        L = bk['length']
        # The beak: the upper half on the head, the lower on the jaw, both loft from inside the face to the tip.
        up_pts, up_r, lo_pts, lo_r = [], [], [], []
        for u in (0.0, 0.3, 0.6, 0.85, 1.0):
            up_pts.append((fx - 0.8 + u * (L + 0.8), 0.0, zb + 0.32 - bk['droop'] * u ** 1.9))
            up_r.append((bk['height'] * (1.0 - u) ** 0.75 + 0.05, bk['width'] * (1.0 - u) ** 0.85 + 0.05))
        for u in (0.0, 0.35, 0.7, 1.0):
            lo_pts.append((fx - 0.7 + u * (0.8 * L + 0.7), 0.0, zb - 0.32 - 0.55 * bk['droop'] * u ** 1.5))
            lo_r.append((0.45 * bk['height'] * (1.0 - u) ** 0.8 + 0.04, 0.82 * bk['width'] * (1.0 - u) ** 0.9 + 0.04))
        la.tube(part, up_pts, up_r, 'beak', sides=8, weights=head_w, caps=(False, True), u_range=(0.0, 1.0),
                v_range=(0.0, 1.0))
        la.tube(part, lo_pts, lo_r, 'beak', sides=7, weights={'jaw': 1.0}, caps=(False, True), u_range=(0.0, 0.8),
                v_range=(0.0, 1.0))
        self.beak_tip = Vector(up_pts[-1])
        # The eyes, sunk a little into the face, looking out to either side.
        hc = Vector(spec['head_center'])
        e = spec['eye']
        for side, _ in la.SIDES:
            d = Vector((e['dir'][0], side * e['dir'][1], e['dir'][2])).normalized()
            hit, normal = self.sculpt.cast(hc, d)
            normal = normal if normal is not None else d
            center = (hit if hit is not None else hc + d * 2.5) - normal * (e['r'] * 0.42)
            rot = normal.to_track_quat('X', 'Z').to_matrix()
            la.ellipsoid(part, center, (e['r'], e['r'], e['r']), 'eye', rot=rot, segs=10, rings=6, weights=head_w)
            if side > 0:
                self.eye_at = center
        # Earlobes: flat ovals behind and below the eyes.
        lo = spec['earlobe']
        for side, _ in la.SIDES:
            d = Vector((lo['dir'][0], side * lo['dir'][1], lo['dir'][2])).normalized()
            hit, normal = self.sculpt.cast(hc, d)
            if hit is None:
                continue
            rot = normal.to_track_quat('Y', 'Z').to_matrix()
            la.ellipsoid(part, hit - normal * 0.05, lo['radii'], self.earlobe, rot=rot, segs=7, rings=4, weights=head_w,
                         patch=True)
        # Wattles: two lobes hanging under the beak's base.
        wt = self.spec['wattle']
        for side, _ in la.SIDES:
            c = Vector((wt['at'][0], side * wt['at'][1], wt['at'][2]))
            la.ellipsoid(part, c, wt['radii'], 'comb', rot=la.turn(0.0, 0.0, side * 6.0), segs=10, rings=6,
                         weights={'wattles': 1.0}, u_range=(0.05, 0.95), v_range=(0.05, 0.6),
                         deform=lambda a, s, u: (a * (1.0 - 0.25 * max(0.0, -u)), s, u))
        self.build_comb()

    def build_comb(self):
        """A single comb: a soft blade along the top of the head with rounded points, thick at its base."""
        part, cm = self.part, self.spec['comb']
        n = cm['points']
        # The points (four columns each: valley, rising, tip, falling), then the blade trailing over the back of the head.
        cols = 4 * n + 3
        heights = []
        for c in range(cols):
            u = c / (cols - 1)
            # Broad, rounded points on a fleshy blade, the valleys well up the comb.
            pf = (0.6, 0.91, 1.0, 0.91)[c % 4]
            if c == 0:
                pf = 0.22
            elif c >= 4 * n + 1:
                pf = (0.5, 0.36)[c - 4 * n - 1]
            # The middle points tallest, the first and last lower, unevenly as a real comb grows.
            env = 0.66 + 0.34 * math.sin(math.pi * min(1.0, 0.08 + 1.0 * u)) + 0.05 * math.sin(c * 2.3)
            heights.append(cm['height'] * env * pf)
        grid = {1: [], -1: []}
        thick = cm['thick']
        # Five rows up each side (the blade thinning from its base to the points), so its fleshy sides shade smoothly
        # instead of creasing across each long quad.
        fracs = (0.0, 0.28, 0.55, 0.78, 1.0)
        # V by height above the comb's base (not by each column's share of its own height), so the texture stays level
        # across the points instead of zigzagging with them.
        tall = max(heights) + 0.35
        vmap = {}
        for side in (1, -1):
            rows = []
            for r, frac in enumerate(fracs):
                th = la.lerp(thick[0], thick[1], frac / 0.55) if frac <= 0.55 else la.lerp(thick[1], thick[2], (frac - 0.55) / 0.45)
                row = []
                for c in range(cols):
                    u = c / (cols - 1)
                    x = la.lerp(cm['front'], cm['back'], u)
                    hit, _ = self.sculpt.cast((x, 0.0, 80.0), (0.0, 0.0, -1.0), 100.0)
                    base = (hit.z if hit is not None else 40.0) - 0.35
                    z = base + frac * (heights[c] + 0.35)
                    lean = 0.12 * frac * heights[c] * (1.0 if u > 0.5 else 0.0)
                    row.append(part.vert((x - lean, side * th, z), {'head': 1.0}))
                    vmap[(r, c)] = 0.84 * frac * (heights[c] + 0.35) / tall
                rows.append(row)
            grid[side] = rows
        uv = lambda u, v: part.at('comb', u, v)
        top = len(fracs) - 1
        for side, u0 in ((1, 0.0), (-1, 0.55)):
            g = grid[side]
            for r in range(top):
                for c in range(cols - 1):
                    q = (g[r][c], g[r][c + 1], g[r + 1][c + 1], g[r + 1][c])
                    uvs = [uv(u0 + 0.45 * c / (cols - 1), vmap[(r, c)]), uv(u0 + 0.45 * (c + 1) / (cols - 1), vmap[(r, c + 1)]),
                           uv(u0 + 0.45 * (c + 1) / (cols - 1), vmap[(r + 1, c + 1)]),
                           uv(u0 + 0.45 * c / (cols - 1), vmap[(r + 1, c)])]
                    part.face(q, uvs, out=(0.0, side, 0.0))
        L, R = grid[1], grid[-1]
        for c in range(cols - 1):
            q = (L[top][c], L[top][c + 1], R[top][c + 1], R[top][c])
            out = Vector((-(L[top][c + 1].co.z - L[top][c].co.z), 0.0, L[top][c + 1].co.x - L[top][c].co.x))
            if out.z < 0:
                out = -out
            part.face(q, [uv(0.45 * c / (cols - 1), 0.88), uv(0.45 * (c + 1) / (cols - 1), 0.88),
                          uv(0.45 * (c + 1) / (cols - 1), 0.98), uv(0.45 * c / (cols - 1), 0.98)], out=out + Vector((0, 0, 0.3)))
        for c, out in ((0, (1.0, 0.0, 0.0)), (cols - 1, (-1.0, 0.0, 0.0))):
            for r in range(top):
                q = (L[r][c], L[r + 1][c], R[r + 1][c], R[r][c])
                part.face(q, [uv(0.92, 0.1 + 0.8 * fracs[r]), uv(0.92, 0.1 + 0.8 * fracs[r + 1]),
                              uv(0.98, 0.1 + 0.8 * fracs[r + 1]), uv(0.98, 0.1 + 0.8 * fracs[r])], out=out)

    def build_hackles(self, which, region_kind):
        cfg = self.spec[which]
        if not cfg:
            return
        t0, t1 = cfg['t']
        rows = cfg['rows']
        for r in range(rows):
            f = r / max(rows - 1, 1)
            t = la.lerp(t0, t1, f)
            count = cfg['per_row']
            spread = cfg['spread']
            for k in range(count):
                if isinstance(spread, tuple):
                    # Two bands down the sides (the saddle), none along the middle of the back.
                    half = count // 2
                    side = 1.0 if k < half else -1.0
                    kk = k if k < half else k - half
                    a = side * la.lerp(spread[0], spread[1], (kk + 0.5 * (r % 2)) / max(half - 0.5, 1))
                else:
                    a = -spread + 2.0 * spread * (k + 0.5 * (r % 2)) / max(count - 0.5, 1)
                p, normal, along = self.ring_point(t, a)
                if isinstance(spread, tuple):
                    along = (along + Vector((0.0, 0.0, -0.45))).normalized()
                    along = (along - normal * along.dot(normal)).normalized()
                j = float(la.hash01(r * 13.0 + k, 7.0 if region_kind == 'hackle' else 9.0)) - 0.5
                L = la.lerp(cfg['length'][0], cfg['length'][1], f) * (1.0 - 0.18 * abs(a) / 180.0) * (1.0 + 0.16 * j)
                line, _ = la.straight(p, along, normal, L)
                # Lying close, the lower rows over the upper ones' tips; the tips lift a little off the body.
                stand = cfg['stand'] + 0.02 * (rows - r)
                point, nfn = la.hug(line, self.sculpt, lambda t, s=stand: s + 0.18 * t * t)
                la.card(self.part, self.feathers, strip(region_kind, k % 2, 2), (0.0, 0.3, 0.6, 0.85), point,
                        normal=nfn, weigh=self.weigh(point), shape='pointed', W=cfg['width'] * (1.0 - 0.2 * j))

    def bed(self, b, d, side, lift):
        """The folded wing's bed: the point b back along the wing's top edge and d down from it (cm), on the body's side,
        lifted off it by lift; with its outward normal."""
        (sx, sz), (rx, rz) = self.spec['wing']['top']
        a = Vector((rx - sx, 0.0, rz - sz)).normalized()
        dn = Vector((-a.z, 0.0, a.x))
        if dn.z > 0:
            dn = -dn
        origin = Vector((sx, 0.0, sz))
        lateral = Vector((0.0, side, 0.0))
        k_wing = self.spec['wing']['k']
        bc, dc = 6.0 * k_wing, 3.5 * k_wing
        for k in range(41):
            f = k / 40.0
            bb, dd = la.lerp(b, bc, f), la.lerp(d, dc, f)
            mid = origin + a * bb + dn * dd
            hit, normal = self.sculpt.cast(mid, lateral, 40.0)
            if hit is not None and normal.dot(lateral) > 0.2:
                rest = a * (b - bb) + dn * (d - dd)
                return hit + rest + normal * lift, normal
        mid = origin + a * b + dn * d
        return mid + lateral * 8.0, lateral

    def bed_card(self, b0, d0, phi, L, W, side, layer, kind, k, of, weigh, shape, rows=(0.0, 0.33, 0.66),
                 two_sided=True, tip_lift=0.0):
        # A little unevenness, the same on every run: no feather sits exactly in line with the next.
        self.cards = getattr(self, 'cards', 0) + 1
        h = [float(la.hash01(self.cards, q)) - 0.5 for q in (1.0, 2.0, 3.0, 4.0)]
        b0, d0, phi, L = b0 + 0.45 * h[0], d0 + 0.3 * h[1], phi + 5.0 * h[2], L * (1.0 + 0.1 * h[3])
        cos, sin = math.cos(math.radians(phi)), math.sin(math.radians(phi))
        base = 0.12 + 0.075 * layer
        cache = {}

        def point(t, x, lay):
            key = (round(t, 4), round(x, 4), lay)
            if key not in cache:
                b = b0 + t * L * cos - x * sin
                d = d0 + t * L * sin + x * cos
                # Each feather's root tucks under the one before it; its tip lies on it.
                tuck = 0.25 * (1.0 - min(1.0, t * 3.0)) ** 2
                p, n = self.bed(b, d, side, base - tuck + tip_lift * t * t - (0.05 if lay else 0.0))
                cache[key] = (p, n)
            return cache[key][0]
        _, n0 = self.bed(b0 + 0.5 * L * cos, d0 + 0.5 * L * sin, side, base)
        la.card(self.wing_part, self.feathers, strip(kind, k, of), rows, point, two_sided=two_sided, normal=lambda t: n0,
                weigh=weigh, shape=shape, W=W)

    def build_wings(self):
        k = self.spec['wing']['k']
        for side, sfx in la.SIDES:
            wing = {f'wing_{sfx}': 1.0}
            # Primaries: tucked under the secondaries, their tips out behind them; fanned by wing_hand and wing_tip.
            for i in range(7):
                f = i / 6.0                                        # 0 the outermost (lowest), 1 the innermost
                w = {f'wing_tip_{sfx}': 1.0 - f, f'wing_hand_{sfx}': f}
                self.bed_card(k * (2.6 + 0.9 * i), k * (7.4 - 0.45 * i), 6.0 - 1.0 * i, k * (12.4 - 0.35 * i), k * 2.5,
                              side, 0.6 * f, 'primary', i % 2, 2, lambda t, w=w: w, 'primary', tip_lift=0.5)
            # Secondaries: the wing's broad middle, the upper ones over the lower.
            for i in range(7):
                self.bed_card(k * (3.3 - 0.3 * i), k * (6.6 - 0.86 * i), 7.0 - 0.9 * i, k * (11.4 - 0.25 * i), k * 3.3,
                              side, 1.2 + 0.16 * i, 'secondary', i % 2, 2, lambda t: wing, 'flight')
            # Coverts in rows over the secondaries' roots, the wing's bow in front (the rooster's lowest row is his dark
            # wing bar).
            bar = self.spec['wing'].get('bar', 'covert')
            for i in range(7):
                self.bed_card(k * (0.9 - 0.1 * i), k * (6.3 - 0.86 * i), 6.0, k * 5.6, k * 2.6, side, 2.6 + 0.08 * i,
                              bar, i % 2, 2, lambda t: wing, 'round', two_sided=False)
            for i in range(6):
                self.bed_card(k * -0.4, k * (5.8 - 0.95 * i), 5.0, k * 4.0, k * 2.2, side, 3.4 + 0.06 * i, 'covert',
                              (i + 1) % 2, 2, lambda t: wing, 'round', two_sided=False)
            for i in range(4):
                self.bed_card(k * -1.3, k * (4.4 - 1.15 * i), 4.0, k * 3.0, k * 2.0, side, 4.0, 'covert', i % 2, 2,
                              lambda t: wing, 'round', two_sided=False)
            # Scapulars: the back's own feathers lying over the folded wing's top edge, so it grows out of the body.
            (sx, sz), (rx, rz) = self.spec['wing']['top']
            for i in range(7):
                f = (i + 0.35 * (la.hash01(i, side) - 0.5)) / 6.0
                jitter = la.hash01(i + 7.0, side) - 0.5
                hit, normal = self.sculpt.cast((la.lerp(sx - 1.0, rx + 4.0, f), 0.0, la.lerp(sz, rz, f) + 1.3 + jitter),
                                               (0.0, side, 0.0), 40.0)
                if hit is None:
                    continue
                along = Vector((-1.0, 0.0, -0.4 + 0.5 * jitter))
                along = (along - normal * along.dot(normal)).normalized()
                line, _ = la.straight(hit, along, normal, 4.4 * k * (1.0 + 0.4 * jitter))
                point, nfn = la.hug(line, self.sculpt, lambda t: 0.1 + 0.6 * t)
                la.card(self.part, self.feathers, strip('covert', i % 2, 2), (0.0, 0.35, 0.7),
                        point, normal=nfn, weigh=self.weigh(point), shape='round', W=3.1 * k * (1.0 - 0.3 * jitter))

    def build_tail(self):
        """The tail as a closed fan: each half a stack of feathers whose vanes lean in like a roof, the inner ones
        highest; the coverts lie over its base from the cushion of the back, so it grows out of the body."""
        tl = self.spec['tail']
        base = Vector(tl['base'])
        weigh = lambda t: {'tail': 1.0 - 0.6 * t, 'tail_tip': 0.6 * t}
        n = tl['count']
        for side, _ in la.SIDES:
            for k in range(n):
                j = float(la.hash01(k * 3.0 + side, 11.0)) - 0.5
                e = math.radians(tl['elev'] - tl['elev_step'] * k + 2.0 * j)
                o = math.radians(tl['out'] + tl['out_step'] * k)
                d = Vector((-math.cos(e) * math.cos(o), side * math.cos(e) * math.sin(o), math.sin(e)))
                root = base + Vector((-0.2 * k, side * (0.3 + 0.26 * k), -0.5 * k))
                normal = Vector((0.0, side, tl['roof'] + 0.05 * k))
                L = (tl['length'] + tl['length_step'] * k) * (1.0 + 0.05 * j)
                point, nfn = la.straight(root, d, normal, L, curl=0.25, bend=0.0, thick=0.07, twist=-side * 4.0)
                la.card(self.part, self.feathers, strip('tail', k % 2, 2), (0.0, 0.3, 0.6, 0.85), point, two_sided=True,
                        normal=nfn, weigh=weigh, shape='flight', W=tl['width'] * (1.0 - 0.04 * k), thick=0.07)
        # Coverts in two rows over the tail's base, lying on the cushion and up onto the fan.
        kw = self.spec['wing']['k']
        count = tl['coverts']
        for row, (ahead, length, spread) in enumerate(((6.5, 7.0, 3.4), (3.4, 6.0, 2.8))):
            for k in range(count - row):
                f = (k + 0.5) / (count - row)
                y = (f - 0.5) * 2.0 * spread
                j = float(la.hash01(k + 5.0 * row, 13.0)) - 0.5
                hit, normal = self.sculpt.cast(base + Vector((ahead, y, 14.0)), (-0.2, 0.0, -1.0), 30.0)
                if hit is None:
                    continue
                e = math.radians(tl['elev'] - 8.0 + 6.0 * j)
                along = Vector((-math.cos(e), y * 0.06, math.sin(e)))
                along = (along - normal * along.dot(normal)).normalized()
                line, _ = la.straight(hit, along, normal, length * kw * (1.0 + 0.12 * j))
                point, nfn = la.hug(line, self.sculpt, lambda t, r=row: 0.08 + 0.05 * r + 0.25 * t * t)
                la.card(self.part, self.feathers, strip('covert', (k + row) % 2, 2), (0.0, 0.35, 0.7), point, normal=nfn,
                        weigh=lambda t: {'tail': 0.5 + 0.4 * t, 'body': 0.5 - 0.4 * t}, shape='round',
                        W=3.0 * kw * (1.0 - 0.1 * j))
        if self.spec['sickles']:
            self.build_sickles()

    def build_fluff(self):
        """The soft fluff of the underbody: over the tops of the drumsticks (so a leg comes out of the feathers, not a
        hole) and under the tail, breaking the belly's outline."""
        L = self.spec['legs']
        for side, sfx in la.SIDES:
            knee = Vector((L['knee'][0], side * L['knee'][1], L['knee'][2]))
            for k in range(6):
                a = math.radians(-65.0 + 130.0 * k / 5.0)            # round the thigh's top, from the front to the back
                j = float(la.hash01(k + 3.0, side * 2.0 + 5.0)) - 0.5
                origin = Vector((knee.x + 2.2 * math.sin(a), side * 1.0, knee.z + 2.0 + 0.6 * j))
                hit, normal = self.sculpt.cast(origin, (0.15 * math.sin(a), side * 1.0, -0.35), 30.0)
                if hit is None:
                    continue
                along = Vector((-0.25 + 0.3 * math.sin(a), side * 0.2, -1.0))
                along = (along - normal * along.dot(normal)).normalized()
                point, nfn = la.straight(hit - normal * 0.15, along, normal, 2.6 * (1.0 + 0.2 * j), lift=0.3, curl=0.2)
                la.card(self.part, self.feathers, strip('fluff', k % 2, 2), (0.0, 0.35, 0.7), point, normal=nfn,
                        weigh=self.weigh(point), shape='round', W=2.1 * (1.0 - 0.15 * j))
        # Under the tail: the vent's fluff, hanging back and down.
        base = Vector(self.spec['tail']['base'])
        for k in range(6):
            y = (k / 5.0 - 0.5) * 6.0
            j = float(la.hash01(k + 9.0, 4.0)) - 0.5
            hit, normal = self.sculpt.cast(base + Vector((5.5, y * 0.6, -9.0 + 0.8 * j)), (-1.0, y * 0.12, -0.2), 30.0)
            if hit is None:
                continue
            along = Vector((-0.8, y * 0.08, -0.6))
            along = (along - normal * along.dot(normal)).normalized()
            point, nfn = la.straight(hit - normal * 0.1, along, normal, 4.2 * (1.0 + 0.2 * j), lift=0.6, curl=0.2)
            la.card(self.part, self.feathers, strip('fluff', k % 2, 2), (0.0, 0.35, 0.7), point, normal=nfn,
                    weigh=self.weigh(point), shape='round', W=3.0)

    def build_sickles(self):
        """The rooster's sickles: long feathers arching up, over and down behind the tail, their vanes on edge."""
        sk = self.spec['sickles']
        base = Vector(self.spec['tail']['base'])
        weigh = lambda t: {'tail': max(0.0, 1.0 - 1.4 * t), 'tail_tip': min(1.0, 1.4 * t)}
        for side, _ in la.SIDES:
            plan = [(sk['main'], 78.0, 165.0, 0.0, 0.9)] + [(L, 66.0 - 9.0 * k, 135.0 - 12.0 * k, 1.2 + 0.9 * k, 0.3 + 0.5 * k)
                                                           for k, L in enumerate(sk['lesser'])]
            for k, (L, e0, sweep, drop, out) in enumerate(plan):
                root = base + Vector((0.6 - 0.4 * k, side * (0.35 + 0.3 * k), 1.2 - drop))
                steps = 24
                cl = [root]
                for i in range(1, steps + 1):
                    t = i / steps
                    ang = math.radians(e0 - sweep * t ** 1.15)
                    d = Vector((-math.cos(ang), side * out * 0.06, math.sin(ang)))
                    cl.append(cl[-1] + d.normalized() * (L / steps))

                def point(t, x, lay, cl=cl, side=side):
                    f = t * steps
                    i = min(int(f), steps - 1)
                    p = cl[i].lerp(cl[i + 1], f - i)
                    tang = (cl[i + 1] - cl[i]).normalized()
                    across = Vector((0.0, side, 0.0)).cross(tang).normalized()
                    q = p + across * x
                    if lay:
                        q -= Vector((0.0, side, 0.0)) * 0.08
                    return q
                la.card(self.part, self.feathers, strip('sickle', k % 2, 2),
                        (0.0, 0.12, 0.25, 0.38, 0.5, 0.62, 0.74, 0.86), point, two_sided=True,
                        normal=lambda t, side=side: Vector((0.0, side, 0.0)), weigh=weigh, shape='blade',
                        W=sk['width'] * (1.0 - 0.12 * k))

    def build_legs(self):
        L = self.spec['legs']
        part = self.part
        for side, sfx in la.SIDES:
            m = lambda p: Vector((p[0], side * p[1], p[2]))
            hip, knee, hock, foot = m(L['hip']), m(L['knee']), m(L['hock']), m(L['foot'])
            # The drumstick: feathered, its top in the body's fluff.
            top = knee + (knee - hock).normalized() * 1.2
            # (Mapped onto the skin's lower flank, so it wears the flank's feathers, not the pale belly's.)
            la.tube(part, [top, knee.lerp(hock, 0.45), hock + (knee - hock).normalized() * 0.4],
                    list(L['drum']), self.skin_region, sides=10,
                    weights=[{f'thigh_{sfx}': 0.4, f'shin_{sfx}': 0.6}, {f'shin_{sfx}': 1.0},
                             {f'shin_{sfx}': 0.8, f'shank_{sfx}': 0.2}],
                    ref=(1.0, 0.0, 0.0), u_range=(0.17, 0.23), v_range=(0.36, 0.5))
            # The shank: bare and scaled, its front to the front, a little flattened side to side, thickest at the hock
            # joint and again where it spreads into the toes.
            sr = L['shank']
            ankle = foot + Vector((0.0, 0.0, 0.25))
            la.tube(part, [hock + Vector((0.0, 0.0, 0.6)), hock.lerp(ankle, 0.12), hock.lerp(ankle, 0.55), hock.lerp(ankle, 0.88),
                           ankle],
                    [(sr[0] * 1.12, sr[0] * 1.02), (sr[0] * 1.02, sr[0] * 0.9), (sr[1] * 1.08, sr[1] * 0.94),
                     (sr[1] * 1.08, sr[1] * 0.98), (sr[1] * 1.12, sr[1] * 1.1)],
                    'leg', sides=9, weights=[{f'shin_{sfx}': 0.35, f'shank_{sfx}': 0.65}, {f'shin_{sfx}': 0.1, f'shank_{sfx}': 0.9},
                                             {f'shank_{sfx}': 1.0}, {f'shank_{sfx}': 1.0}, {f'shank_{sfx}': 1.0}],
                    ref=(1.0, 0.0, 0.0), caps=(True, False), u_range=(0.0, 0.62))
            if L['spur']:
                at = hock.lerp(ankle, 0.72)
                d = Vector((-0.8, side * 0.45, 0.25)).normalized()
                la.tube(part, [at - d * 0.3, at + d * L['spur'] * 0.55, at + d * L['spur'] + Vector((0, 0, 0.35))],
                        [0.3, 0.2, 0.04], 'claw', sides=5, weights={f'shank_{sfx}': 1.0}, caps=(False, True))
            # The toes: three forward, one back, each ending in a claw.
            pad = foot + Vector((0.2, 0.0, 0.05))
            la.ellipsoid(part, pad, (0.95, 0.8, 0.5), 'leg', segs=8, rings=5, weights={f'shank_{sfx}': 0.6, f'toes_{sfx}': 0.4},
                         u_range=(0.7, 0.95), v_range=(0.1, 0.9))
            toes = [(a, length, f'toes_{sfx}') for a, length in L['toes']] + [(180.0 - 12.0, L['hallux'], f'hallux_{sfx}')]
            for a, length, bone in toes:
                yaw = math.radians(a)
                d = Vector((math.cos(yaw), side * math.sin(yaw), 0.0))
                r = L['toe'] * (0.9 if bone.startswith('hallux') else 1.0)
                start = pad + d * 0.3
                # Knuckled toes: a little thicker at each joint, flat underneath.
                pts = [start, start + d * (0.35 * length) + Vector((0, 0, -0.04)), start + d * (0.7 * length) + Vector((0, 0, -0.1)),
                       start + d * length + Vector((0, 0, -0.18))]
                ws = [{f'shank_{sfx}': 0.4, bone: 0.6}, {bone: 1.0}, {bone: 1.0}, {bone: 1.0}]
                la.tube(part, pts, [(r * 0.9, r * 1.05), (r * 0.82, r * 0.92), (r * 0.74, r * 0.8), (r * 0.6, r * 0.64)], 'leg',
                        sides=6, weights=ws, caps=(True, False), u_range=(0.66, 0.98))
                tip = Vector(pts[-1])
                la.tube(part, [tip - d * 0.15, tip + d * 0.45 + Vector((0, 0, -0.06)), tip + d * 0.75 + Vector((0, 0, -0.32))],
                        [r * 0.55, r * 0.35, 0.03], 'claw', sides=5, weights={bone: 1.0}, caps=(False, True))

    def build(self):
        self.build_skin()
        if 'skinonly' in ARGV:
            self.eye_at = Vector(self.spec['head_center'])
            self.beak_tip = Vector(self.spec['face_center'])
            self.zones = dict(head=0.9, neck=0.6, tail=0.1, eye=la.skin_landmark(self.skin, self.eye_at),
                              rings=la.skin_table(self.skin))
            return
        self.build_head()
        self.build_hackles('hackles', 'hackle')
        self.build_hackles('saddle', 'saddle')
        self.build_wings()
        self.build_tail()
        self.build_fluff()
        self.build_legs()
        spec = self.spec
        z_head, z_neck, z_tail = spec['zones']
        vs, ts = self.skin['vs'], self.skin['ts']

        def v_at(t):
            return vs[min(range(len(ts)), key=lambda k: abs(ts[k] - t))]
        self.zones = dict(head=v_at(z_head), neck=v_at(z_neck), tail=v_at(z_tail),
                          eye=la.skin_landmark(self.skin, self.eye_at), rings=la.skin_table(self.skin))
        if spec['saddle']:
            t0, t1 = spec['saddle']['t']
            self.zones['saddle'] = (v_at(t0 - 0.04), v_at(min(1.0, t1 + 0.05)))

    def finish(self, material):
        obj = self.part.finish(material)
        wings = self.wing_part.finish(material)
        self.rig = la.Rig(bone_specs(self.spec))
        self.rig.bind(obj)
        lt.bake_vertex_ao(obj, samples=16, distance=0.05, ground=True)
        lt.bake_vertex_ao(wings, samples=16, distance=0.05, ground=False)
        obj = la.join(self.name, [obj, wings])
        self.rig.arm['LODs'] = '50,25'
        self.rig.arm['LODScreens'] = '0.1,0.04'
        self.obj = obj
        return obj


# --- The atlas ---

rgb = lt.rgb
ZONES = {}


def paint_skin(plumage, U, V, seed):
    """The skin's plumage: feathers lying in overlapping rows (la.plumage), small on the head, long and narrow down the
    neck (the hackles), broad on the body, each with its own tone and its plumage's markings, the front ones shading
    the ones they lie over; and the bare red face round the eye."""
    shape = U.shape
    z = ZONES.get(plumage) or next(iter(ZONES.values()))       # (a work run of one bird lacks the others' landmarks)
    d, s = lc.body_axes(U, V)
    zh, zn = z['head'], z['neck']

    def zone(v):
        head = lt.smooth(zh - 0.012, zh + 0.012, v)
        neck = lt.smooth(zn - 0.03, zn + 0.03, v) * (1.0 - head)
        return head, neck, np.clip(1.0 - head - neck, 0.0, 1.0)

    def by_zone(head_v, neck_v, body_v):
        def f(v):
            h, n, b = zone(v)
            return head_v * h + neck_v * n + body_v * b
        return f

    def per_row(head_v, neck_v, body_v):
        f = by_zone(head_v, neck_v, body_v)
        return lambda row: f(v_of(row)).astype(np.float32)
    q, cols_at, v_of = la.skin_rows(z['rings'], V, by_zone(0.5, 0.8, 1.3), by_zone(0.52, 0.62, 1.5))
    # The rows waver round the body, so they never print as rings.
    q = q + 0.25 * lt.noise(shape, seed + 21, 30.0, 8.0)
    f = la.plumage(q, U, cols_at, length=per_row(1.7, 2.3, 1.75), width=per_row(0.66, 0.58, 0.66),
                   pointed=per_row(0.1, 0.8, 0.05), max_length=2.5, seed=seed)
    t, x, h, h2, over = f['t'], f['x'], f['h'], f['h2'], f['over']
    ax = np.abs(x)
    head, neck, body = zone(V)
    shadow = 1.0 - lt.smooth(0.0, 0.4, over)
    fringe = lt.smooth(-0.14, 0.0, f['margin']) * lt.smooth(0.45, 0.8, t)
    shaft = (1.0 - lt.smooth(0.04, 0.14, ax)) * lt.smooth(0.12, 0.35, t) * lt.smooth(1.0, 0.8, t)
    centre = (1.0 - lt.smooth(0.2, 0.42, ax)) * lt.smooth(0.08, 0.3, t) * lt.smooth(0.98, 0.78, t)
    fleck = 1.0 - lt.smooth(0.1, 0.2, np.hypot(x * 1.1, (t - 0.84) * 1.6))
    lines, split = la.barbs(t, x, f['row'], f['col'], seed + 5, count=22.0)
    mottle = lt.smooth(-1.2, 1.4, lt.noise(shape, seed + 23, 26.0, 20.0, octaves=2))
    belly = lt.smooth(-0.2, -0.75, d) * body
    throat = lt.smooth(-0.35, -0.7, d)
    breast = body * lt.smooth(zn - 0.24, zn - 0.04, V) * lt.smooth(0.35, -0.2, d)
    rump = lt.smooth(z['tail'] + 0.08, z['tail'] - 0.02, V)
    tone = (h - 0.5)
    rough = np.full(shape, 0.76, np.float32)
    if plumage == 'brown':
        c = lt.mix(rgb(0x7a4321), rgb(0x8e5430), lt.smooth(0.5, -0.3, d))
        c = lt.mix(c, rgb(0xa26c40), breast * 0.8)
        c = lt.mix(c, rgb(0x8e7a64), belly * 0.85)
        c = lt.mix(c, rgb(0x6c3a1c), mottle * 0.25)
        neck_col = lt.mix(rgb(0xa86c32), rgb(0xc8954f), lt.smooth(0.2, 0.9, t))
        neck_col = lt.mix(neck_col, rgb(0x3a2212), centre * 0.75 * (1.0 - throat))
        neck_col = lt.mix(neck_col, rgb(0x9a6a3c), throat * 0.7)
        c = lt.mix(c, neck_col, neck)
        c = lt.mix(c, rgb(0x8a5026), head)
        c = c * (1.0 + 0.24 * tone)[..., None]
        c = lt.mix(c, rgb(0x6a381c), (h2 < 0.22) * 0.3)
        c = lt.mix(c, rgb(0x3e2414), shaft * 0.3 * body * (1.0 - belly))
        c = lt.mix(c, rgb(0xd9c9a8), fleck * (h2 > 0.8) * 0.75 * body * lt.smooth(-0.3, 0.2, d) * (1.0 - 0.6 * breast))
        c = c * (1.0 + 0.08 * fringe + 0.045 * lines - 0.12 * split)[..., None]
        c = c * (1.0 - 0.24 * shadow)[..., None]
    elif plumage == 'white':
        c = lt.mix(rgb(0xebe5da), rgb(0xe5dfd1), lt.smooth(0.0, 0.7, d))
        c = lt.mix(c, rgb(0xefeae1), breast * 0.8 + neck * 0.6)
        c = lt.mix(c, rgb(0xe2d9c8), belly * 0.8)
        # A yard hen: dust on the belly and the vent, a little on the back.
        dust = lt.smooth(-0.4, 1.6, lt.noise(shape, seed + 25, 12.0, 8.0, octaves=2))
        c = lt.mix(c, rgb(0xcab89a), dust * (0.5 * belly + 0.45 * rump * lt.smooth(0.2, -0.4, d) + 0.12 * body))
        c = lt.mix(c, rgb(0xe6dfd0), mottle * 0.3)
        c = c * (1.0 + 0.06 * tone + 0.04 * fringe + 0.025 * lines - 0.06 * split)[..., None]
        c = lt.mix(c, c * rgb(0xd8cfc0), shadow * 0.55)
    else:
        black = rgb(0x151515)
        # Black below the wing line (the breast, belly, thighs and the fluff under the tail); the back is red under
        # the hackles and saddle, so no black shows between their tips.
        dark = np.clip(body * lt.smooth(0.0, -0.3, d) + breast + neck * throat + rump * lt.smooth(-0.1, -0.5, d), 0.0, 1.0)
        back = lt.mix(rgb(0x6a2214), rgb(0x8c3418), lt.smooth(0.2, 0.9, t) * (0.5 + 0.5 * h))
        back = lt.mix(back, rgb(0x4a160e), shaft * 0.4)
        c = back
        if 'saddle' in z:
            front, rear = z['saddle']
            saddle = lt.smooth(front + 0.015, front - 0.015, V) * lt.smooth(rear - 0.015, rear + 0.015, V)
            saddle_col = lt.mix(rgb(0xb4561c), rgb(0xd88a32), lt.smooth(0.2, 0.9, t))
            saddle_col = lt.mix(saddle_col, rgb(0x8a3414), centre * 0.35)
            c = lt.mix(c, saddle_col, saddle * lt.smooth(-0.6, -0.25, d))
        neck_col = lt.mix(rgb(0xbf5e1e), rgb(0xe39a3c), lt.smooth(0.15, 0.9, t))
        neck_col = lt.mix(neck_col, rgb(0x8a3414), centre * 0.35)
        c = lt.mix(c, neck_col, neck)
        c = lt.mix(c, rgb(0xb24a1e), head)
        # Black below, its feathers' ends catching a green sheen (some a little blue).
        sheen = lt.mix(rgb(0x1b3a2a), rgb(0x222238), (h2 > 0.75) * 1.0)
        black_col = lt.mix(black, sheen, lt.smooth(0.35, 0.95, t) * (0.3 + 0.45 * h))
        c = lt.mix(c, black_col, dark)
        c = c * (1.0 + 0.22 * tone * (1.0 - dark) + 0.08 * fringe + 0.05 * lines - 0.12 * split)[..., None]
        c = c * (1.0 - 0.3 * shadow)[..., None]
        rough = rough - 0.28 * dark * lt.smooth(0.35, 0.95, t) - 0.18 * (neck + head * 0.0) * (1.0 - dark)
    # The bare red face round the eye, out to the beak and down to the earlobe; small feathers fringe its edge.
    eye = z['eye']
    du = np.minimum(np.abs(U - eye['u']), np.abs(U - (1.0 - eye['u']))) * eye['cm_u']
    dv = (V - eye['v']) * eye['cm_v']
    r_face = np.hypot(du * 1.15, np.where(dv > 0, dv * 0.55, dv * 1.2))
    r_face = np.minimum(r_face, np.maximum(du * 0.8 / 1.05, np.abs(dv - 0.05) * 2.2) + 0.12)
    r_face = r_face + 0.1 * (t - 0.5) * head + 0.04 * lt.noise(shape, seed + 31, 2.0, 2.0)
    face = (1.0 - lt.smooth(1.0, 1.12, r_face)) * head
    wrinkle = lt.noise(shape, seed + 33, 1.5, 4.0)
    face_col = lt.mix(rgb(0x9c2420), rgb(0x7c1c1a), lt.smooth(0.3, 1.5, wrinkle) * 0.5)
    c = lt.mix(c, face_col, face)
    rough = rough * (1.0 - face) + 0.62 * face
    height = 0.00045 * f['height'] * (1.0 - face) + 0.00004 * lines * (1.0 - face) + 0.00005 * wrinkle * face
    occl = 1.0 - 0.26 * shadow * (1.0 - face) - 0.1 * belly
    return c, rough, height, occl


FEATHER_LOOKS = {
    # strip: (root, middle, tip colours) per plumage; markings are added in paint_feathers.
    'brown': dict(hackle=(0x6a3c1c, 0xa8743a, 0xc8954f), saddle=(0x6a3c1c, 0xa8743a, 0xc8954f),
                  covert=(0x6a3c1e, 0x7e4826, 0x86502a), fluff=(0x7a5c42, 0x8a6a4e, 0x96785a),
                  secondary=(0x4a2e1a, 0x7a4826, 0x7e4c2a), primary=(0x2e2118, 0x35261b, 0x2e2118),
                  tail=(0x2a1c14, 0x231812, 0x1f1510), sickle=(0x2a1c14, 0x231812, 0x1f1510)),
    'white': dict({k: (0xe0dace, 0xebe5da, 0xefeae0) for k in STRIPS},
                  primary=(0xdcd5c8, 0xe5ded2, 0xdad1c0), tail=(0xdcd5c8, 0xe6dfd2, 0xd8cebb)),
    'rooster': dict(hackle=(0x8a3010, 0xcc6a20, 0xe49c40), saddle=(0x8a3010, 0xc8641e, 0xe0943a),
                    covert=(0x5a1a10, 0x7a2616, 0x8c3418), fluff=(0x141414, 0x171717, 0x1a1a1a),
                    secondary=(0x131413, 0x141614, 0x121412), primary=(0x1a1613, 0x1c1714, 0x181412),
                    tail=(0x090d0b, 0x0a100c, 0x0b120d), sickle=(0x090d0b, 0x0a100c, 0x0b120d)),
}
# The outline each strip's feathers are cut to (la.outline), so the painter knows where a vane's edge lies.
SHAPES = dict(hackle='pointed', saddle='pointed', covert='round', fluff='round', secondary='flight', primary='primary',
              tail='flight', sickle='blade')


def paint_feathers(plumage, U, V, seed):
    """The feather sheets: two feathers side by side in each strip (U across, V root to tip), painted as real feathers
    are: a rachis down the middle, fine barbs slanting out from it toward the tip (the odd split where they part), a
    downy root, a soft fringe at the vane's edge, and each kind's markings."""
    shape = U.shape
    looks = FEATHER_LOOKS[plumage]
    c = np.zeros(shape + (3,), np.float32)
    rough = np.full(shape, 0.72, np.float32)
    height = np.zeros(shape, np.float32)
    k = np.clip(np.floor(U * 8.0), 0, 7).astype(np.int64)
    sub = la.frac(U * 16.0)                                 # each strip holds two feathers side by side
    fid = np.floor(U * 16.0)                                # which feather
    x = (sub - 0.5) * 2.0                                   # across: -1 .. 1 (negative: the outer web)
    t = V
    ax = np.abs(x)
    t_rows = V[:, 0]
    soft = lt.noise(shape, seed + 5, 6.0, 30.0)
    for i, kind in enumerate(STRIPS):
        m = k == i
        if not m.any():
            continue
        # The vane's edge in texture space (1 there): the card's outline at each t.
        width_t = np.array([2.0 * la.outline(SHAPES[kind], float(v), 1.0) for v in t_rows], np.float32)[:, None]
        e = np.clip(ax / np.maximum(width_t, 0.05), 0.0, 1.5)
        count = 30.0 if kind in ('tail', 'sickle', 'primary', 'secondary') else 20.0
        lines = la.line_noise(t * count - ax * count * 0.55 + fid * 37.0, seed + i)
        split_at = la.hash01(fid, 3.0 + i) * 0.5 + 0.35
        split = (1.0 - lt.smooth(0.0, 0.03, np.abs(t - split_at - ax * 0.25))) * lt.smooth(0.35, 0.75, e) * \
            (la.hash01(fid, 9.0 + i) > 0.45)
        rachis = (1.0 - lt.smooth(0.02, 0.05, ax / np.maximum(1.0 - 0.6 * t, 0.2))) * lt.smooth(0.98, 0.85, t)
        fringe = lt.smooth(0.78, 1.0, e)
        down = 1.0 - lt.smooth(0.02, 0.18, t)
        r0, r1, r2 = (rgb(v) for v in looks[kind])
        base = lt.mix(lt.mix(r0, r1, lt.smooth(0.0, 0.45, t)), r2, lt.smooth(0.55, 1.0, t))
        base = base * (1.0 + 0.05 * soft + 0.06 * (la.hash01(fid, 1.0 + i) - 0.5))[..., None]
        r = np.full(shape, 0.72, np.float32)
        if plumage == 'brown':
            if kind in ('hackle', 'saddle'):
                base = lt.mix(base, rgb(0x3a2212), (1.0 - lt.smooth(0.32, 0.52, e)) * lt.smooth(0.1, 0.3, t) *
                              lt.smooth(0.95, 0.75, t) * 0.8)
            elif kind == 'covert':
                base = lt.mix(base, rgb(0x3e2414), (1.0 - lt.smooth(0.08, 0.22, e)) * lt.smooth(0.2, 0.5, t) * 0.35)
                fleck = (1.0 - lt.smooth(0.08, 0.16, np.hypot(e * 0.8, (t - 0.87) * 1.5))) * (la.hash01(fid, 2.0) > 0.5)
                base = lt.mix(base, rgb(0xd9c9a8), fleck * 0.45)
            elif kind == 'secondary':
                base = lt.mix(base, rgb(0x4a2c18), lt.smooth(0.0, 0.25, x) * 0.45)
                base = lt.mix(base, rgb(0x5a341c), lt.smooth(0.3, 1.4, lt.noise(shape, seed + 9, 3.0, 9.0)) * 0.25)
                base = lt.mix(base, rgb(0x94643a), fringe * lt.smooth(0.8, 0.95, t) * 0.4)
            elif kind == 'primary':
                base = lt.mix(base, rgb(0x7a4a26), (x < 0) * lt.smooth(0.55, 0.85, e) * lt.smooth(0.15, 0.4, t) * 0.6)
            elif kind == 'fluff':
                base = lt.mix(base, rgb(0xa8906e), lt.smooth(0.3, 1.2, lt.noise(shape, seed + 11, 1.0, 5.0)) * 0.3)
            elif kind in ('tail', 'sickle'):
                base = lt.mix(base, rgb(0x5c3c22), fringe * 0.55)
                r[:] = 0.58
            base = lt.mix(base, rgb(0xb08a60), rachis * 0.3)
            base = lt.mix(base, rgb(0x8a7a6a), down * 0.5)
            base = base * (1.0 + 0.06 * fringe + 0.06 * lines - 0.25 * split)[..., None]
        elif plumage == 'white':
            base = lt.mix(base, rgb(0xd8d2c6), down * 0.5)
            base = lt.mix(base, rgb(0xfaf7f0), rachis * 0.3)
            if kind in ('primary', 'tail'):
                # Worn ends: a yard hen's flight feathers and tail are scuffed and a little dusty.
                base = lt.mix(base, rgb(0xc8b89c), lt.smooth(0.75, 1.0, t) * lt.smooth(0.2, 1.4, lt.noise(shape, seed + 13, 3.0, 10.0)) * 0.4)
            base = base * (1.0 + 0.03 * fringe + 0.025 * lines - 0.08 * split)[..., None]
        else:
            if kind in ('hackle', 'saddle'):
                base = lt.mix(base, rgb(0x8a3414), (1.0 - lt.smooth(0.1, 0.3, e)) * lt.smooth(0.2, 0.5, t) * 0.35)
                r[:] = 0.5
            elif kind == 'covert':
                base = lt.mix(base, rgb(0xa84a1e), fringe * lt.smooth(0.5, 0.9, t) * 0.4)
            elif kind == 'secondary':
                # The wing bay: the outer webs chestnut, a triangle of it on the folded wing.
                base = lt.mix(base, rgb(0x8a4a1c), (x < 0) * lt.smooth(0.12, 0.35, e) * lt.smooth(0.15, 0.4, t) * 0.85)
            elif kind == 'primary':
                base = lt.mix(base, rgb(0x6a3a1a), (x < 0) * lt.smooth(0.6, 0.85, e) * 0.7)
            if kind in ('tail', 'sickle', 'fluff', 'secondary', 'primary'):
                sheen = (1.0 - lt.smooth(0.2, 0.9, e)) * lt.smooth(0.2, 0.9, t) * (0.6 + 0.4 * lines)
                base = lt.mix(base, rgb(0x16402a), np.clip(sheen * 0.3, 0.0, 1.0))
                # Glossy, but feathers, not lacquer: the barbs break the highlight.
                r[:] = 0.47 if kind in ('tail', 'sickle') else 0.6
                r = r + 0.08 * lines
            base = lt.mix(base, rgb(0x2a2626), down * 0.4 * (kind in ('hackle', 'saddle', 'covert')))
            base = base * (1.0 + 0.07 * fringe + 0.07 * lines - 0.25 * split)[..., None]
        c[m] = base[m]
        rough[m] = (r + 0.1 * fringe)[m]
        h = 0.00012 * (1.0 - np.clip(e, 0.0, 1.0) ** 2) + 0.00018 * rachis + 0.00003 * lines - 0.00008 * split
        height[m] = h[m]
    occl = 1.0 - 0.25 * (1.0 - lt.smooth(0.0, 0.3, t))
    return c, rough, height, occl


def paint_bare(region, U, V, seed):
    shape = U.shape
    n = lt.noise(shape, seed + 2, 3.0, 3.0, octaves=2)
    if region == 'comb':
        # Fleshy and waxy: finely pitted, darker and bluer where it grows from the head, brighter at the points.
        pits = lt.noise(shape, seed + 4, 1.2, 1.2)
        veins = lt.noise(shape, seed + 6, 1.0, 5.0)                 # running up the blade

        c = lt.mix(rgb(0x7a1a1e), rgb(0xb42628), lt.smooth(0.0, 0.35, V))
        c = lt.mix(c, rgb(0xc8382e), lt.smooth(0.5, 0.85, V) * 0.5)
        c = lt.mix(c, rgb(0x8c1c20), lt.smooth(0.6, 1.8, veins) * 0.3)
        c = c * (1.0 + 0.05 * n - 0.05 * lt.smooth(0.5, 1.5, pits))[..., None]
        return c, 0.5 - 0.08 * lt.smooth(-1.0, 1.0, n), 0.00005 * pits, 1.0 - 0.25 * (1.0 - lt.smooth(0.0, 0.25, V))
    if region == 'beak':
        top = lt.smooth(0.25, 0.0, np.abs(V - 0.5))
        c = lt.mix(rgb(0xc89c4e), rgb(0xe8d098), lt.smooth(0.3, 1.0, U))
        c = lt.mix(c, rgb(0x9c7436), top * 0.45 * lt.smooth(0.9, 0.4, U))
        nostril = 1.0 - lt.smooth(0.02, 0.05, np.hypot((U - 0.16) * 1.0, (np.abs(V - 0.5) - 0.17) * 0.6))
        c = lt.mix(c, rgb(0x3a2a1a), nostril)
        return c * (1.0 + 0.04 * n)[..., None], np.full(shape, 0.36, np.float32), 0.0001 * n, np.ones(shape, np.float32)
    if region == 'leg':
        front = lt.smooth(0.22, 0.08, np.abs(V - 0.5))
        rows = la.frac(U * 22.0)
        plates = 1.0 - lt.smooth(0.0, 0.12, np.minimum(rows, 1.0 - rows))
        cells = lt.worley(shape, (24, 6), seed + 7)
        small = 1.0 - lt.smooth(0.0, 2.0, cells['edge'])
        lines = np.where(front > 0.5, plates, small)
        c = lt.mix(rgb(0xd6ac48), rgb(0xe4c066), front * 0.5)
        c = lt.mix(c, rgb(0x9a7228), lines * 0.6)
        c = lt.mix(c, rgb(0x7a6a52), lt.smooth(0.55, 0.98, U) * 0.35 * lt.smooth(0.4, 1.4, n + 1.0))
        return c, np.full(shape, 0.5, np.float32), -0.00025 * lines, 1.0 - 0.25 * lines
    if region == 'eye':
        r = 1.0 - V
        ang = 2.0 * np.pi * U
        streak = 0.5 + 0.5 * np.sin(ang * 23.0 + 3.0 * n)
        c = lt.mix(rgb(0xc8741e), rgb(0xe6a040), streak * 0.4)
        c = lt.mix(c, rgb(0x6a3a14), lt.smooth(0.3, 0.42, r))
        c = lt.mix(c, rgb(0x0c0806), 1.0 - lt.smooth(0.11, 0.14, r))
        c = lt.mix(c, rgb(0x2a1a10), lt.smooth(0.42, 0.5, r))
        return c, np.full(shape, 0.08, np.float32), np.zeros(shape, np.float32), np.ones(shape, np.float32)
    if region == 'earlobe_red':
        return np.broadcast_to(rgb(0xa82a26), shape + (3,)) * (1.0 + 0.05 * n)[..., None], np.full(shape, 0.62, np.float32), \
            np.zeros(shape, np.float32), np.ones(shape, np.float32)
    if region == 'earlobe_white':
        return np.broadcast_to(rgb(0xe2ddd2), shape + (3,)) * (1.0 + 0.03 * n)[..., None], np.full(shape, 0.6, np.float32), \
            np.zeros(shape, np.float32), np.ones(shape, np.float32)
    if region == 'claw':
        c = lt.mix(rgb(0xc4ae86), rgb(0x4e4032), lt.smooth(0.3, 1.0, U))
        return c, np.full(shape, 0.4, np.float32), np.zeros(shape, np.float32), np.ones(shape, np.float32)
    raise ValueError(region)


def paint(region, U, V, k):
    if region.endswith('_skin'):
        return paint_skin(region[:-5], U, V, 100 + k)
    if region.endswith('_feathers'):
        return paint_feathers(region[:-9], U, V, 200 + k)
    return paint_bare(region, U, V, 300 + k)


# --- Poses (the extremes the game's code blends between) ---

def hen_poses(bird):
    """The pose table: each pose a dict of bone turns (creature axes, degrees, about the bone's head, under the parent's
    turn); the legs are planted with the game's two-bone IK so the feet stay put where they should."""
    rig, spec = bird.rig, bird.spec
    L = spec['legs']
    foot = {sfx: Vector((L['foot'][0], side * L['foot'][1], L['foot'][2] + 0.25)) for side, sfx in la.SIDES}
    back = Vector((-1.0, 0.0, 0.0))

    def plant(pose, sfx, target):
        rig.plant(pose, f'shin_{sfx}', f'shank_{sfx}', target, back, flat=(f'toes_{sfx}', f'hallux_{sfx}'))
        return pose

    def stand(pose):
        for _, sfx in la.SIDES:
            plant(pose, sfx, foot[sfx])
        return pose

    def wings_open(pose, up, fan):
        for side, sfx in la.SIDES:
            hinge = (rig.tail[f'wing_{sfx}'] - rig.head[f'wing_{sfx}']).normalized()
            pose[f'wing_{sfx}'] = la.about(hinge, -side * up)
            pose[f'wing_hand_{sfx}'] = la.about((0.0, 1.0, 0.0), -fan * 0.5)
            pose[f'wing_tip_{sfx}'] = la.about((0.0, 1.0, 0.0), -fan)
        return pose
    poses = {'rest': stand({})}
    poses['alert'] = stand({'body': (0.0, -7.0, 0.0), 'neck_1': (0.0, -14.0, 0.0), 'neck_2': (0.0, -6.0, 0.0),
                            'head': (0.0, 24.0, 0.0), 'tail': (0.0, -10.0, 0.0)})
    # The peck: the beak to the ground in front of the feet (fitted: the head goes down until the beak's tip does).
    lo, hi = 0.0, 2.0
    for _ in range(18):
        s = (lo + hi) * 0.5
        p = {'body': (0.0, 16.0 * s, 0.0), 'neck_1': (0.0, 34.0 * s, 0.0), 'neck_2': (0.0, 30.0 * s, 0.0),
             'head': (0.0, 30.0 * s, 0.0), 'tail': (0.0, -8.0 * s, 0.0)}
        tip = rig.where(p, 'head', bird.beak_tip)
        if tip.z > 0.4:
            lo = s
        else:
            hi = s
    poses['peck'] = stand(p)
    scratch = stand({'body': (0.0, 10.0, 0.0), 'neck_1': (0.0, 16.0, 0.0), 'neck_2': (0.0, 12.0, 0.0), 'head': (0.0, 10.0, 0.0)})
    plant(scratch, 'r', foot['r'] + Vector((-7.0, 0.0, 2.2)))
    scratch['toes_r'] = (0.0, 35.0, 0.0)
    poses['scratch'] = scratch
    walk = {'body': (0.0, 4.0, 0.0), 'body@move': (0.0, 0.0, -0.5), 'neck_1': (0.0, 10.0, 0.0), 'head': (0.0, -6.0, 0.0)}
    plant(walk, 'l', foot['l'] + Vector((4.6, 0.0, 0.0)))
    plant(walk, 'r', foot['r'] + Vector((-4.0, 0.0, 2.6)))
    walk['toes_r'] = (0.0, 40.0, 0.0)
    poses['walk'] = walk
    flutter = {'body': (0.0, 18.0, 0.0), 'body@move': (0.0, 0.0, 2.0), 'neck_1': (0.0, 20.0, 0.0), 'neck_2': (0.0, -6.0, 0.0),
               'head': (0.0, -12.0, 0.0), 'tail': (0.0, -14.0, 0.0)}
    plant(flutter, 'l', foot['l'] + Vector((5.5, 0.0, 3.0)))
    plant(flutter, 'r', foot['r'] + Vector((-6.0, 0.0, 4.5)))
    poses['flutter_up'] = wings_open(dict(flutter), 105.0, 55.0)
    poses['flutter_down'] = wings_open(dict(flutter), 40.0, 45.0)
    sit = {'body@move': (0.0, 0.0, -9.5), 'body': (0.0, 4.0, 0.0), 'neck_1': (0.0, 14.0, 0.0), 'neck_2': (0.0, 6.0, 0.0),
           'head': (0.0, -14.0, 0.0)}
    for _, sfx in la.SIDES:
        rig.plant(sit, f'shin_{sfx}', f'shank_{sfx}', foot[sfx] + Vector((1.5, 0.0, 0.2)), Vector((-1.0, 0.0, 0.3)),
                  flat=(f'toes_{sfx}', f'hallux_{sfx}'))
    poses['sit'] = sit
    if bird.spec['sickles']:
        crow = stand({'body': (0.0, -16.0, 0.0), 'neck_1': (0.0, -16.0, 0.0), 'neck_2': (0.0, -12.0, 0.0),
                      'head': (0.0, -6.0, 0.0), 'jaw': (0.0, 26.0, 0.0), 'wattles': (0.0, -12.0, 0.0),
                      'tail': (0.0, -8.0, 0.0)})
        for side, sfx in la.SIDES:
            hinge = (rig.tail[f'wing_{sfx}'] - rig.head[f'wing_{sfx}']).normalized()
            crow[f'wing_{sfx}'] = la.about(hinge, -side * 12.0)
        poses['crow'] = crow
    return poses


NOTES = {
    'rest': 'standing, as modelled: the feet flat, the head level',
    'alert': 'stretched up tall, the head level and still (watching the player)',
    'peck': 'the beak at the ground in front of the feet; blend rest -> peck -> rest in 0.25 s, two to four times',
    'scratch': 'the right foot raked back, its toes curled (mirror the leg bones for the left); body tipped forward',
    'walk': 'the left foot forward and planted, the right lifted behind (mirror for the other step); the head is held '
            'still in the world while the body walks under it, then snaps forward (a head-bob)',
    'flutter_up': 'running flight: wings up and fanned, legs running; flap between flutter_up and flutter_down at 8 Hz',
    'flutter_down': 'the downstroke of the flutter',
    'sit': 'settled down on its belly (brooding, dust-bathing, roosting), the legs folded under',
    'crow': "the rooster's crow: chest out, neck up, beak open, wings a little lowered",
}


# --- Build ---

ONLY = [a[5:] for a in ARGV if a.startswith('only=')]
birds = [Bird(name, spec, plumage, lobe) for name, spec, plumage, lobe in PLUMAGES if not ONLY or name in ONLY]
for b in birds:
    ZONES[b.plumage] = b.zones
la.paint_atlas(ATLAS, paint, SIZE, PX_M, SET, NORMAL_STRENGTH)
MATERIAL = lt.material(SET)
for b in birds:
    b.finish(MATERIAL)
    _comb = max((v.co.z for v in b.obj.data.vertices if v.co.y < -0.08), default=0.0)
    la.log(f'{b.name}: {la.tris(b.obj)} triangles, {len(b.rig.order)} bones, {100.0 * _comb:.1f} cm to the comb, materials '
           f'{", ".join(m.name for m in b.obj.data.materials)}')
BY_NAME = {b.name: b for b in birds}
for b in birds:
    b.poses = hen_poses(b)
    title = {'HenBrown': 'Hen', 'Rooster': 'Rooster'}.get(b.name)
    if title and __name__ != '__sheet__' and not ONLY:
        la.write_poses(title, b.rig.table(b.poses, {k: NOTES[k] for k in b.poses if k in NOTES}))


# --- Previews ---

def check():
    """Each bird from the front, the left, the back and three-quarter, back faces culled as in the game (work only)."""
    la.backface_culling([MATERIAL])
    studio = la.Studio(samples=16)
    la.hide([b.rig.arm for b in birds])
    pose = [a[5:] for a in ARGV if a.startswith('pose=')]
    for b in birds:
        objs = [studio.snapshot(b.rig, b.obj, b.poses[pose[0]] if pose else {}, at=((k - 1.5) * 0.55, 0.0, 0.0),
                                yaw=heading, name=f'{b.name}{k}')
                for k, heading in enumerate((90.0, 0.0, -90.0, 145.0))]
        studio.shoot(f'_check_{b.name}', objs, (0.0, -1.0, 0.12), lens=60.0, size=(2000, 700), margin=1.03)
        studio.clear()


def previews():
    la.backface_culling([MATERIAL])
    studio = la.Studio()
    rigs = [b.rig.arm for b in birds]
    la.hide(rigs)
    brown, white, rooster = BY_NAME['HenBrown'], BY_NAME['HenWhite'], BY_NAME['Rooster']
    report = {}
    for b in birds:
        report[b.name] = dict(triangles=la.tris(b.obj), bones=len(b.rig.order), materials=[m.name for m in b.obj.data.materials])
    # The three at one scale.
    objs = [studio.snapshot(brown.rig, brown.obj, brown.poses['rest'], at=(-0.42, 0.05, 0.0), yaw=-28.0, name='A'),
            studio.snapshot(rooster.rig, rooster.obj, rooster.poses['rest'], at=(0.0, 0.28, 0.0), yaw=-12.0, name='B'),
            studio.snapshot(white.rig, white.obj, white.poses['peck'], at=(0.46, -0.05, 0.0), yaw=30.0, name='C')]
    studio.shoot('Chickens', objs, (-0.55, -1.0, 0.32), lens=60.0, size=(1800, 1000), margin=1.08)
    studio.clear()
    # The hen's poses, side by side.
    names = ('alert', 'peck', 'scratch', 'walk', 'sit')
    objs = [studio.snapshot(brown.rig, brown.obj, brown.poses[p], at=((k - 2) * 0.42, 0.0, 0.0), yaw=-62.0, name=p)
            for k, p in enumerate(names)]
    studio.shoot('Hen_Poses', objs, (-0.12, -1.0, 0.2), lens=55.0, size=(2000, 800), margin=1.05, height_bias=0.38)
    studio.clear()
    objs = [studio.snapshot(white.rig, white.obj, white.poses['flutter_up'], at=(-0.32, 0.0, 0.0), yaw=-40.0, name='fu'),
            studio.snapshot(brown.rig, brown.obj, brown.poses['flutter_down'], at=(0.34, 0.12, 0.0), yaw=-70.0, name='fd')]
    studio.shoot('Hen_Flutter', objs, (-0.35, -1.0, 0.55), lens=55.0, size=(1600, 1000), margin=1.1)
    studio.clear()
    objs = [studio.snapshot(rooster.rig, rooster.obj, rooster.poses['crow'], at=(-0.3, 0.0, 0.0), yaw=-75.0, name='crow'),
            studio.snapshot(rooster.rig, rooster.obj, rooster.poses['walk'], at=(0.32, 0.1, 0.0), yaw=-30.0, name='rwalk')]
    studio.shoot('Rooster_Poses', objs, (-0.3, -1.0, 0.25), lens=55.0, size=(1600, 1000), margin=1.08)
    studio.clear()
    # Close: the heads.
    objs = [studio.snapshot(brown.rig, brown.obj, brown.poses['alert'], at=(-0.1, 0.0, 0.0), yaw=-40.0, name='hh'),
            studio.snapshot(rooster.rig, rooster.obj, rooster.poses['rest'], at=(0.12, 0.12, -0.07), yaw=-115.0, name='rh')]
    eye = Vector((0.02, -0.6, 0.43))
    studio.shoot('Chicken_Heads', objs, None, lens=50.0, size=(1600, 1000), eye=eye, target=Vector((0.02, 0.0, 0.37)))
    studio.clear()
    la.write_report('Chicken', report)


if PREVIEW and __name__ != '__sheet__':
    if 'check' in ARGV:
        check()
    else:
        previews()
