"""The brown hunting spider (ASpiderCreature): the "Meadow Wolf" the user picked from the concepts (2026-10-01; see
Art/Backlog/Creatures/SpiderConcepts.py), as a rig the game animates in code, with an eight-legged stepping gait and
two-bone IK, and a hit zone around every part.

The look: a smooth, segmented wolf spider in stylized realism. A pale stripe runs down its carapace, a heart mark and
chevrons sit on its abdomen, and its legs are ringed and bristled. Its eight small eyes are low domes seated on the face. It has one
material, MI_SpiderBody (the World master), whose texture set Art/Textures/SpiderBody (1024 px: color, normal,
occlusion-roughness-metal) is painted by this script every run (looter_creatures.paint_atlas; deterministic, so an
unchanged painter rewrites the same files). Every part maps onto a rectangle of that atlas (ATLAS); eyes, bristles
and fangs sit on small solid patches of it, so the eyes' gloss comes from the roughness map.

Bones, which the game finds by name: body (the thorax; its rest height is the ride height); head, with the eyes, and
its pedipalps palp_l and palp_r; fang_l and fang_r (each a chelicera with its fang); abdomen; and per leg
femur_<pair>_<side>, tibia_<pair>_<side> and foot_<pair>_<side> (the tip), pairs 0 (front) to 3 (back) on sides l and
r. The origin is the ground under the thorax; the front faces Blender -Y (Unreal +X). The legs rest in the standing
pose solved with the game's two-bone IK: ASpiderCreature measures its legs from these bones and must bend the knees
toward the same pole (knee_pole). Each bone's part, bristles included, has one convex hit hull: PA_Spider's bodies,
"head" being the critical spot.

    blender -b --factory-startup --python Art/Models/Creatures/Spider.py -- --preview
"""
import math
import os
import random

import numpy as np
from mathutils import Vector

import looter_creatures as lc
import looter_model as lm
import looter_textures as lt

RIDE = 60.0          # the thorax's height over the ground (cm)
SET = 'SpiderBody'
SIZE = 1024
# Where each part lies in the atlas (u0, u1, v0, v1).
ATLAS = {
    'abdomen': (0.0, 1.0, 0.5, 1.0), 'carapace': (0.0, 0.5, 0.25, 0.5), 'head': (0.5, 1.0, 0.25, 0.5),
    'femur': (0.0, 0.5, 0.125, 0.25), 'tibia': (0.0, 0.5, 0.0, 0.125), 'palp': (0.5, 0.75, 0.125, 0.25),
    'chel': (0.75, 1.0, 0.125, 0.25), 'coxa': (0.5, 0.75, 0.0, 0.125),
    'eye': (0.75, 0.8125, 0.0, 0.125), 'bristle': (0.8125, 0.875, 0.0, 0.125), 'fang': (0.875, 0.9375, 0.0, 0.125),
    'misc': (0.9375, 1.0, 0.0, 0.125),
}

# Body parts: centers (cm, x forward, y left, z over the ride height) and radii.
CARAPACE = dict(center=(0.0, 0.0, 2.0), radii=(46.0, 36.0, 22.0))
HEAD = dict(center=(34.0, 0.0, 8.0), radii=(24.0, 22.0, 17.0))
ABDOMEN = dict(center=(-72.0, 0.0, 10.0), radii=(62.0, 44.0, 38.0), fwd=(1.0, 0.0, 0.12))
# Eyes (x, y, z, radius), mirrored: a big forward pair, a row of four small ones below, a pair on top. They sit on the
# head's surface, pushed out by a fifth of their radius.
EYES = ((54.0, 6.8, 18.0, 3.6), (58.0, 3.4, 10.0, 1.7), (56.0, 10.0, 10.0, 1.5), (45.0, 10.5, 24.0, 2.5))
EYE_PUSH = 0.2
CHEL = dict(x=55.0, y=7.0, z=-2.0, radii=(10.0, 7.0, 7.0))
PALP = dict(x=50.0, y=12.0, z=0.0, length=40.0, radius=4.0)
# Legs per pair, front (0) to back (3). Angles are degrees from straight ahead.
HIP_ANGLE = (35.0, 70.0, 108.0, 142.0)
REST_ANGLE = (38.0, 72.0, 110.0, 148.0)
REST_RADIUS = (175.0, 155.0, 152.0, 178.0)
HIP_RADIUS = (32.0, 27.0, -2.0)   # an ellipse round the thorax (x, y), and the hips' height off the ride height
FEMUR = (100.0, 90.0, 90.0, 104.0)
TIBIA = (130.0, 116.0, 116.0, 134.0)
LEG_RADIUS = 7.8
SIDES = ((1.0, 'l'), (-1.0, 'r'))   # creature space has y to the left
HAIR_LENGTH, HAIR_RADIUS = 7.0, 0.45
LEG_HAIRS = (14, 16)                # bristles per femur, per tibia
BODY_HAIRS = (('carapace', 50, lambda a, s, u: u > 0.1), ('abdomen', 150, lambda a, s, u: u > -0.4))


def knee_pole(hip):
    """Where knees bend: up, and a little out from the body (ASpiderCreature's KneePole uses the same rule)."""
    outward = Vector((hip.x, hip.y, 0.0)).normalized()
    return Vector((0.0, 0.0, 1.0)) + outward * 0.4


def taper(front, bottom):
    return lambda a, s, u: (a, s * (1 - front * max(a, 0)), u * (bottom if u < 0 else 1.0))


def flat_bottom(k):
    return lambda a, s, u: (a, s, u * (k if u < 0 else 1.0))


def egg(front, bottom):
    return lambda a, s, u: (a, s * (1 - front * max(a, 0)), u * (1 - front * max(a, 0)) * (bottom if u < 0 else 1.0))


CARAPACE['deform'] = taper(0.15, 0.6)
HEAD['deform'] = flat_bottom(0.6)
ABDOMEN['deform'] = egg(0.12, 0.8)


def lifted(point):
    return Vector(point) + Vector((0.0, 0.0, RIDE))


# --- The texture atlas ---

def paint(region, U, V, seed):
    """Color, roughness, height (meters) and occlusion for one atlas region: a warm brown wolf spider, fur running
    along the body and the limbs."""
    shape = U.shape
    brown, dark, light, belly = lt.rgb(0x6b4a2e), lt.rgb(0x2c1c0f), lt.rgb(0xb89466), lt.rgb(0xa08055)
    n = lt.noise(shape, seed, 12.0, 12.0, octaves=3)
    rough = np.full(shape, 0.66, np.float32)
    occl = np.ones(shape, np.float32)
    if region in ('eye', 'bristle', 'fang', 'misc'):
        color = {'eye': 0x070606, 'bristle': 0x24170c, 'fang': 0x1a120c, 'misc': 0x4a3220}[region]
        rough[:] = {'eye': 0.06, 'bristle': 0.6, 'fang': 0.25, 'misc': 0.7}[region]
        return np.broadcast_to(lt.rgb(color), shape + (3,)).copy(), rough, np.zeros(shape, np.float32), occl
    if region in ('abdomen', 'carapace', 'head'):
        d, s = lc.body_axes(U, V)
        fur = lt.noise(shape, seed + 11, 0.7, 5.0)            # streaks along the body (V runs along the rows)
        col = lt.mix(brown, light, np.clip(0.25 + 0.2 * n, 0, 1))
        col = lt.mix(col, belly, lt.smooth(-0.1, -0.5, d))
        if region == 'abdomen':
            field = lt.smooth(0.62, 0.42, np.abs(s) + 0.05 * n) * lt.smooth(-0.15, 0.25, d) * lt.smooth(0.02, 0.15, V)
            col = lt.mix(col, dark, field * 0.8)
            heart = (np.abs(s) < 0.11 * np.clip(1 - ((V - 0.78) / 0.2) ** 2, 0, 1)) & (d > 0)
            col = lt.mix(col, light * 0.95, lt.blur(heart.astype(np.float32), 1.5))
            for k in range(4):
                vk = 0.5 - k * 0.11
                line = 1 - lt.smooth(0.012, 0.03, np.abs(V - (vk - 0.22 * np.abs(s))))
                col = lt.mix(col, light, line * (np.abs(s) < 0.42) * (d > 0) * 0.85)
            col = lt.mix(col, light * 0.9, lt.specks(shape, seed + 3, 0.04, 1.0) * 0.6 * (d > -0.2))
        else:
            stripe = lt.smooth(0.15, 0.08, np.abs(s)) * (d > 0)
            lateral = lt.smooth(0.12, 0.2, np.abs(s)) * lt.smooth(0.72, 0.55, np.abs(s)) * (d > 0)
            col = lt.mix(col, dark, lateral * 0.85)
            col = lt.mix(col, light, stripe * (0.9 if region == 'carapace' else 0.6))
            col = lt.mix(col, light * 0.95, lt.smooth(0.75, 0.92, np.abs(s)) * (d > -0.3) * 0.7)
        col = col * (1.0 + 0.09 * fur)[..., None]
        height = 0.0004 * fur + 0.0003 * n
        occl = 1.0 - 0.18 * lt.smooth(-0.2, -0.8, d)
        rough = rough + 0.06 * fur
        return col * (1.0 + 0.05 * lt.noise(shape, seed + 5, 3.0, 3.0))[..., None], rough, height, occl
    along, top = lc.limb_axes(U, V)
    fur = lt.noise(shape, seed + 13, 5.0, 0.7)                # streaks along the limb (U runs along the columns)
    col = lt.mix(brown, light, np.clip(0.3 + 0.2 * n, 0, 1))
    col = lt.mix(col, belly * 1.05, lt.smooth(0.0, -0.6, top))
    if region == 'femur':
        col = lt.mix(col, dark, lc.bands(along, (0.42, 0.78), 0.07) * 0.62)
    elif region == 'tibia':
        col = lt.mix(col, dark, lc.bands(along, (0.18, 0.46, 0.72, 0.94), 0.05) * 0.62)
    elif region in ('palp', 'chel'):
        col = lt.mix(col, dark, 0.45)
    else:
        col = lt.mix(col, dark, 0.3)
    col = col * (1.0 + 0.09 * fur)[..., None]
    occl = 1.0 - 0.15 * lt.smooth(0.0, -0.8, top)
    return col, rough + 0.06 * fur, 0.0004 * fur + 0.0002 * n, occl


texture_dir = os.path.join(lt.TEXTURE_DIR, SET)
os.makedirs(texture_dir, exist_ok=True)
lc.paint_atlas(ATLAS, paint, SIZE, 0.003, texture_dir, SET, normal_strength=1.2)
MATERIAL = lt.material(SET)


# --- The parts, one per bone ---

rng = random.Random(1847)
parts = {}     # bone: the part's mesh (its hit hull is made from it)
hairs = {}     # bone: its bristles (no hull)
bones = []     # (name, head, tail, parent), creature space


def part(bone):
    parts[bone] = lc.Mesh(ATLAS)
    return parts[bone]


def bristles(bone):
    if bone not in hairs:
        hairs[bone] = lc.Mesh(ATLAS)
    return hairs[bone]


def body_hairs(bone, spec, count, where):
    center = lifted(spec['center'])
    fwd = Vector(spec.get('fwd', (1.0, 0.0, 0.0))).normalized()
    m = bristles(bone)
    for p, n, a, s, u in lc.surface_points(center, spec['radii'], fwd, rng, count, where, spec.get('deform')):
        lc.bristle(m, p - n * 0.5, n * 0.45 - fwd * 0.9, HAIR_LENGTH * rng.uniform(0.7, 1.3), HAIR_RADIUS, 'bristle')


# Thorax (with the pedicel to the abdomen).
m = part('body')
lc.ellipsoid(m, lifted(CARAPACE['center']), CARAPACE['radii'], 'carapace', deform=CARAPACE['deform'])
ab_center = lifted(ABDOMEN['center'])
ab_fwd = Vector(ABDOMEN['fwd']).normalized()
lc.tube(m, [lifted((-CARAPACE['radii'][0] * 0.85, 0.0, 1.2)), ab_center + ab_fwd * ABDOMEN['radii'][0] * 0.9], [6.0, 6.0],
        'misc', sides=8, patch=True)
body_hairs('body', CARAPACE, *BODY_HAIRS[0][1:])
bones.append(('body', lifted((0.0, 0.0, 0.0)), lifted((25.0, 0.0, 0.0)), None))

# Head and eyes.
m = part('head')
head_center = lifted(HEAD['center'])
lc.ellipsoid(m, head_center, HEAD['radii'], 'head', deform=HEAD['deform'])
for x, y, z, r in EYES:
    for side, _ in SIDES:
        eye = lc.seat_on(head_center, HEAD['radii'], lifted((x, side * y, z)), r * EYE_PUSH)
        big = r > 2.0
        lc.ellipsoid(m, eye, (r, r, r), 'eye', segs=10 if big else 8, rings=7 if big else 5, patch=True)
bones.append(('head', lifted((26.0, 0.0, 8.0)), lifted((46.0, 0.0, 8.0)), 'body'))

# Pedipalps: two-part feelers reaching forward and down.
for side, suffix in SIDES:
    start = lifted((PALP['x'], side * PALP['y'], PALP['z']))
    elbow = start + Vector((PALP['length'] * 0.55, side * 4.0, -PALP['length'] * 0.35))
    tip = elbow + Vector((PALP['length'] * 0.3, side * 1.0, -PALP['length'] * 0.55))
    m = part(f'palp_{suffix}')
    r = PALP['radius']
    lc.tube(m, [start, start.lerp(elbow, 0.5), elbow], [r, r * 0.95, r * 0.85], 'palp', sides=8)
    lc.tube(m, [elbow, elbow.lerp(tip, 0.5), tip], [r * 0.85, r * 0.9, r * 0.7], 'palp', sides=8)
    bones.append((f'palp_{suffix}', start, elbow, 'head'))

# Chelicerae, each with a curved fang hooking inward underneath. They spread open for a bite.
for side, suffix in SIDES:
    base = lifted((CHEL['x'], side * CHEL['y'], CHEL['z']))
    m = part(f'fang_{suffix}')
    radii = CHEL['radii']
    lc.ellipsoid(m, base, radii, 'chel', fwd=(0.35, 0, -1), up=(1, 0, 0.3), segs=12, rings=8)
    tip0 = base + Vector((radii[1] * 0.3, -side * 2.0, -radii[0] * 0.85))
    lc.tube(m, [tip0, tip0 + Vector((4, -side * 3, -6)), tip0 + Vector((5, -side * 7, -9))],
            [radii[1] * 0.35, radii[1] * 0.22, 0.4], 'fang', sides=6, patch=True)
    bones.append((f'fang_{suffix}', base, base + Vector((4.0, 0.0, -15.0)), 'head'))

# The abdomen sways from a pivot at the pedicel; spinnerets at its back.
m = part('abdomen')
lc.ellipsoid(m, ab_center, ABDOMEN['radii'], 'abdomen', fwd=ABDOMEN['fwd'], segs=32, rings=18, deform=ABDOMEN['deform'])
rear = ab_center - ab_fwd * ABDOMEN['radii'][0]
for side, _ in SIDES:
    lc.tube(m, [rear + ab_fwd * 4 + Vector((0, side * 3, -2)), rear - ab_fwd * 6 + Vector((0, side * 4, -5))], [3.0, 1.2],
            'misc', sides=6, patch=True)
body_hairs('abdomen', ABDOMEN, *BODY_HAIRS[1][1:])
pivot = lifted((-40.0, 0.0, 6.0))
bones.append(('abdomen', pivot, pivot + Vector((-50.0, 0.0, 12.0)), 'body'))

# Legs, standing: each foot on the ground at its resting spot. A coxa and an arched femur on the femur bone; the knee
# joint and a bowed tibia tapering to the claw on the tibia bone; bristles lying along each toward its tip.
for pair in range(4):
    for side, suffix in SIDES:
        ha, ra = math.radians(HIP_ANGLE[pair]), math.radians(REST_ANGLE[pair])
        hip = Vector((math.cos(ha) * HIP_RADIUS[0], side * math.sin(ha) * HIP_RADIUS[1], RIDE + HIP_RADIUS[2]))
        rest = Vector((math.cos(ra) * REST_RADIUS[pair], side * math.sin(ra) * REST_RADIUS[pair], 0.0))
        pole = knee_pole(hip)
        knee, foot = lc.solve_two_bone(hip, rest, FEMUR[pair], TIBIA[pair], pole)
        plane = (knee - hip).cross(foot - knee).normalized()
        leg = f'{pair}_{suffix}'
        r = LEG_RADIUS
        m = part(f'femur_{leg}')
        fdir = (knee - hip).normalized()
        lc.ellipsoid(m, hip, (r * 1.25, r * 1.15, r * 1.1), 'coxa', fwd=fdir, segs=10, rings=6)
        fbend = (pole - fdir * pole.dot(fdir)).normalized()
        steps = [i / 6 for i in range(7)]
        fpts = [hip.lerp(knee, t) + fbend * (math.sin(math.pi * t) * FEMUR[pair] * 0.05) for t in steps]
        frad = [r * (1.0 + 0.14 * math.sin(math.pi * min(t * 1.5, 1.0)) - 0.2 * t) for t in steps]
        lc.tube(m, fpts, frad, 'femur', sides=10, up=pole, plane=plane)
        m = part(f'tibia_{leg}')
        lc.ellipsoid(m, knee, (r * 0.95, r * 0.92, r * 0.92), 'coxa', fwd=fdir, segs=10, rings=6)
        tdir = (foot - knee).normalized()
        tbend = (pole - tdir * pole.dot(tdir)).normalized()
        tsteps = [i / 8 for i in range(9)]
        tpts = [knee.lerp(foot, t) + tbend * (math.sin(math.pi * t) * TIBIA[pair] * 0.06) for t in tsteps]
        trad = [r * (0.84 - 0.62 * t ** 1.1) + (0.05 * r if t > 0.98 else 0.0) for t in tsteps]
        lc.tube(m, tpts, trad, 'tibia', sides=8, up=pole, plane=plane)
        for bone, points, rads, count in ((f'femur_{leg}', fpts, frad, LEG_HAIRS[0]), (f'tibia_{leg}', tpts, trad, LEG_HAIRS[1])):
            flip = -1.0 if plane.cross((points[-1] - points[0]).normalized()).dot(pole) < 0 else 1.0
            hm = bristles(bone)
            for _ in range(count):
                t = rng.uniform(0.05, 0.92)
                k = min(int(t * (len(points) - 1)), len(points) - 2)
                f = t * (len(points) - 1) - k
                p = points[k].lerp(points[k + 1], f)
                tangent = (points[k + 1] - points[k]).normalized()
                ref = plane.cross(tangent).normalized() * flip
                around = math.radians(rng.uniform(-115, 115))
                normal = ref * math.cos(around) + tangent.cross(ref) * math.sin(around)
                rad = rads[k] * (1 - f) + rads[k + 1] * f
                lc.bristle(hm, p + normal * rad * 0.8, normal * 0.55 + tangent * 0.85,
                           HAIR_LENGTH * rng.uniform(0.6, 1.2), HAIR_RADIUS, 'bristle')
        bones += [(f'femur_{leg}', hip, knee, 'body'), (f'tibia_{leg}', knee, foot, f'femur_{leg}'),
                  (f'foot_{leg}', foot, foot + (foot - knee).normalized() * 8.0, f'tibia_{leg}')]


# --- The rig ---

rig = lm.armature()
lm.bones(rig, [(name, lc.to_blender(head), lc.to_blender(tail), parent) for name, head, tail, parent in bones])
skin = {}
for bone, mesh in parts.items():
    skin[bone] = [mesh.finish(bone.title().replace('_', ''), MATERIAL)]
    if bone in hairs:
        skin[bone].append(hairs.pop(bone).finish(bone.title().replace('_', '') + 'Hair', MATERIAL))
    # The hull holds everything the bone moves, bristles included: a shot at any drawn part hits it (the
    # Looter.Creatures.Spider.HitZones test checks every vertex).
    lm.hit_hull(rig, bone, skin[bone])
spider = lm.skin(rig, 'Spider', skin)
# No baked vertex occlusion: the parts move against each other, and the bristles would darken everything around them.
# The occlusion map's painted shading (bellies, the undersides of the legs) does that job; vertex alpha stays 1.
colors = spider.data.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
colors.data.foreach_set('color', [1.0] * (4 * len(spider.data.loops)))
# Level-of-detail shares (LOD1, LOD2), for when the rig importer makes skeletal LODs: spiders are often seen from
# mid-distance.
rig['LODs'] = '50,25'
rig['LODScreens'] = '0.3,0.12'
lt._log(f'Spider: {sum(len(p.vertices) - 2 for p in spider.data.polygons)} triangles, {len(bones)} bones')

if lt.want_preview():
    lt.preview([spider], lt.preview_path('Creatures', 'Spider'), view=(0.7, -1.0, 0.55))
