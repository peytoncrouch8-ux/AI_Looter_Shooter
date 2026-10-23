"""The brown hunting spider (ASpiderCreature): the "Meadow Wolf" the user picked from the concepts (2026-10-01; see
Art/Backlog/Creatures/SpiderConcepts.py), as a rig the game animates in code, with an eight-legged stepping gait and
two-bone IK, and a hit zone around every part.

The look (SPIDER-LOOK, 2026-10-22: the user found the spider "not correctly stylized": legs like blocky striped poles, a
body that read photographic and flat): stylized realism, a wolf spider in painted chitin and fur.
  - Legs in their real segments, each a smooth tapered plate: on the femur bone a coxa, trochanter and a club-shaped
    femur; on the tibia bone a patella, tibia, metatarsus and tarsus to a pointed tip (TIBIA_SEGS). Each segment swells
    from a narrow neck, eases off and ends in a hard rim; the next starts inside it (a dark joint between). The radii stay
    inside the old legs' (femur_radius, tibia_radius: SK_Gravemother's mud sleeves wrap the tibia by that rule; the
    patella as big as the old knee ball), and the hairs stand as the old ones did, so the hit hulls keep about their size.
    Spines in pairs under the tibia and metatarsus, a row on the femur, a tuft under the tarsus.
  - The pedipalps the same way: femur, patella, a slightly clubbed tibia and tarsus.
  - One painted atlas (PAINTED hides: BROWN): broad soft value shapes, crisp marks, fur in clumps (relief in the normal
    map, darker crevices), a painted sheen along the top of every leg plate, pale rims and dark joints; the carapace's
    fovea and the grooves radiating from it toward the legs; a dark face round the eyes; glossier chitin than fur
    (roughness map). The old fine streak noise (it read as wood grain) is gone.
  - Bones, rest pose, the body's shapes (CARAPACE, HEAD, ABDOMEN) and the atlas's layout (ATLAS) are as before.

It has one material, MI_SpiderBody (the World master), whose texture set Art/Textures/SpiderBody (2048 px: color,
normal, occlusion-roughness-metal) is painted by this script every run (looter_creatures.paint_atlas; deterministic, so an
unchanged painter rewrites the same files). Every part maps onto a rectangle of that atlas (ATLAS); eyes, bristles and
fangs sit on small solid patches of it, so the eyes' gloss comes from the roughness map.

The Gravemother: BONE (MI_SpiderBody_Pale, the fallback before SK_Gravemother) and Gravemother.py's GRAVE are older hides
(no 'painted' key): their bodies keep the old painter; their leg rings sit at the new joints. BONE writes
Art/Textures/SpiderBody_Pale/T_SpiderBody_Pale_BC.png, a color map only (its normal and ORM maps would be byte copies of
SpiderBody's, so the game's MI_SpiderBody_Pale uses T_SpiderBody_N and T_SpiderBody_ORM). Spitter.py and
looter_barrow.spider_parts (Gravemother.py, GravemotherStone.py) run this script's parts code (all above its rig's
heading line, which must not appear earlier): keep the names they read (ATLAS, paint, BONE, knee_pole, TIBIA, LEG_RADIUS
on its own line, SIDES, ABDOMEN, lifted, parts, hairs, bones; the abdomen's bristles made by lc.bristle, four vertices
each).

Bones, which the game finds by name: body (the thorax; its rest height is the ride height); head, with the eyes, and
its pedipalps palp_l and palp_r; fang_l and fang_r (each a chelicera with its fang); abdomen; and per leg
femur_<pair>_<side>, tibia_<pair>_<side> and foot_<pair>_<side> (the tip), pairs 0 (front) to 3 (back) on sides l and
r. The origin is the ground under the thorax; the front faces Blender -Y (Unreal +X). The legs rest in the standing
pose solved with the game's two-bone IK: ASpiderCreature measures its legs from these bones and must bend the knees
toward the same pole (knee_pole). Each bone's part, bristles included, has one convex hit hull: PA_Spider's bodies,
"head" being the critical spot.

    Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Spider.py -Preview     (Saved/ArtPreviews/Creatures/Spider.png)
    Tools\\artrun.ps1 -Export Spider -Family Spider                        (Intermediate/ArtExport_Spider, no import)
"""
import math
import os
import random
import shutil
import tempfile

import numpy as np
from mathutils import Vector

import looter_creatures as lc
import looter_model as lm
import looter_textures as lt

RIDE = 60.0          # the thorax's height over the ground (cm)
SET = 'SpiderBody'
PALE_SET = 'SpiderBody_Pale'
SIZE = 2048
K = SIZE / 1024.0                         # the painters' pixel sizes were set at 1024 px
PX_M, NORMAL_STRENGTH = 0.0015, 1.2       # the atlas's metres per pixel (for the normal map) and the normals' strength
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
BODY_HAIRS = (('carapace', 30, lambda a, s, u: u > 0.1), ('abdomen', 100, lambda a, s, u: u > -0.4))

# The legs' segments: (name, t0, t1, atlas region, u0, u1, sides, stations), t along the bone (0 its head .. 1 its
# tail), u along the region, stations where its rings lie (s 0..1 along it: the neck, the swell, the rim). Each starts a little inside the one before. On the tibia bone u is t itself (0 knee .. 1 claw), as
# Gravemother.py's mud reads it.
FEMUR_SEGS = (('coxa', -0.08, 0.17, 'coxa', 0.0, 1.0, 8, (0.0, 0.3, 0.65, 1.0)),
              ('femur', 0.15, 1.0, 'femur', 0.0, 1.0, 10, (0.0, 0.2, 0.45, 0.72, 0.9, 1.0)))
TIBIA_SEGS = (('patella', -0.04, 0.14, 'tibia', 0.0, 0.14, 10, (0.0, 0.4, 0.85, 1.0)),
              ('tibia', 0.12, 0.52, 'tibia', 0.12, 0.52, 10, (0.0, 0.25, 0.58, 0.88, 1.0)),
              ('metatarsus', 0.50, 0.87, 'tibia', 0.50, 0.87, 8, (0.0, 0.25, 0.58, 0.88, 1.0)),
              ('tarsus', 0.85, 0.985, 'tibia', 0.85, 0.985, 6, (0.0, 0.5, 1.0)))
# The painter's segments per limb region (u spans; a joint at each boundary), and its dark bands (segment, s).
LIMB_SEGMENTS = {'coxa': ((0.0, 0.7), (0.7, 1.0)), 'femur': ((0.0, 1.0),),
                 'tibia': ((0.0, 0.13), (0.13, 0.51), (0.51, 0.86), (0.86, 1.0)),
                 'palp': ((0.0, 0.45), (0.45, 0.6), (0.6, 1.0))}
LIMB_BANDS = {'femur': ((0, 0.36), (0, 0.72)), 'tibia': ((1, 0.56), (2, 0.48)), 'palp': ((0, 0.62),)}
TIBIA_JOINTS = (0.13, 0.51, 0.86)
# Leg hairs: spines (t on the bone, degrees round the leg from its top, length share of HAIR_LENGTH), mirrored pairs
# where the angle isn't 0; then short loose hairs (count, length range) per bone.
FEMUR_SPINES = ((0.36, 0.0, 0.62), (0.56, 0.0, 0.66), (0.76, 0.0, 0.6))
TIBIA_SPINES = ((0.24, 150.0, 0.78), (0.36, 150.0, 0.74), (0.46, 150.0, 0.66),
                (0.62, 150.0, 0.62), (0.77, 150.0, 0.55),
                (0.91, 180.0, 0.32), (0.94, 160.0, 0.3))
LOOSE_HAIRS = {'femur': (11, 0.6, 1.1), 'tibia': (14, 0.6, 1.15)}


def knee_pole(hip):
    """Where knees bend: up, and a little out from the body (ASpiderCreature's KneePole uses the same rule)."""
    outward = Vector((hip.x, hip.y, 0.0)).normalized()
    return Vector((0.0, 0.0, 1.0)) + outward * 0.4


def femur_radius(t, r=None):
    """The femur's outer radius at t (0 hip .. 1 knee): the old leg's, which no segment passes."""
    r = LEG_RADIUS if r is None else r
    t = min(max(t, 0.0), 1.0)
    return r * (1.0 + 0.14 * math.sin(math.pi * min(t * 1.5, 1.0)) - 0.2 * t)


def tibia_radius(t, r=None):
    """The lower leg's outer radius at t (0 knee .. 1 claw): the old leg's (Gravemother.py's tibia_curve, the mud)."""
    r = LEG_RADIUS if r is None else r
    t = min(max(t, 0.0), 1.0)
    return r * (0.84 - 0.62 * t ** 1.1)


def plate(s, swell=1.0, neck=0.8):
    """A chitin segment's profile along it (s 0..1), as a share of the leg's radius there: a narrow neck where it leaves
    the joint, swelling over its first third, easing off, a hard rim just before its far end."""
    s = min(max(s, 0.0), 1.0)
    body = neck + (swell - neck) * lc.smoothstep(0.0, 0.28, s) - 0.14 * lc.smoothstep(0.3, 0.85, s)
    return body + 0.09 * math.exp(-((s - 0.9) / 0.05) ** 2)


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
# A hide is a palette for the painter: the colors of the base fur and of each mark, and how strongly each mark mixes in
# (the _k values). A hide with 'painted' takes the painted look (paint_painted); the others (BONE, Gravemother.py's
# GRAVE) the older painter. Only color comes from BONE's; roughness, height and occlusion are BROWN's (MI_SpiderBody_Pale
# uses SpiderBody's normal and ORM maps).

_DARK, _LIGHT = lt.rgb(0x2a190c), lt.rgb(0xbc9a6c)
# The Meadow Wolf: warm brown, a dark field round a light heart mark and light chevrons, dark leg bands and joints, a
# dark face round the eyes.
BROWN = dict(
    painted=True,
    base=lt.rgb(0x654633), light=_LIGHT, belly=lt.rgb(0x46321f),
    field=_DARK, field_k=0.82, heart=lt.rgb(0xcaa26f), heart_k=1.0, heart_shape=(0.15, 0.76, 0.22),
    heart_outline=False, chevron=_LIGHT, chevron_k=0.85, speck=_LIGHT * 0.9, speck_k=0.5,
    lateral=_DARK, lateral_k=0.85, stripe=lt.rgb(0xcfa872), stripe_k=(0.92, 0.7), margin=_LIGHT * 0.95, margin_k=0.7,
    ring=lt.rgb(0x2b1a0d), ring_k=0.78, membrane=lt.rgb(0x1d120a), palp=lt.rgb(0x3b2615), palp_k=0.55,
    joint=_DARK, joint_k=0.3,
    face=lt.rgb(0x1e130b), face_k=0.8, groove=lt.rgb(0x2a190c),
    solid={'eye': 0x070606, 'bristle': 0x24170c, 'fang': 0x1a120c, 'misc': 0x4a3220})
# The Gravemother: bone, the user's pick (2026-10-07) of three looks (Art/Backlog/Creatures/GravemotherConcepts.py, A).
# A chalky bone white, kept under about 0.75 so direct sun still shades it; the brown spider's marks faded to grey
# ghost-lines (the heart drawn as an outline), its leg rings kept dark; a darker face round the eyes, since black eyes
# alone on a pale head read as a toy's; a few old healed scars across her back and grave dust settled on it.
BONE = dict(
    base=lt.rgb(0xbab2a1), light=lt.rgb(0xcfc8b8), belly=lt.rgb(0x968e80),
    field=lt.rgb(0x908a7f), field_k=0.6, heart=lt.rgb(0x524d46), heart_k=1.0, heart_shape=(0.15, 0.76, 0.23),
    heart_outline=True, chevron=lt.rgb(0x58524a), chevron_k=0.85, speck=lt.rgb(0x847d72), speck_k=0.4,
    lateral=lt.rgb(0x857e74), lateral_k=0.65, stripe=lt.rgb(0xd4cdbf), stripe_k=(0.6, 0.4),
    margin=lt.rgb(0xcac3b3), margin_k=0.5,
    ring=lt.rgb(0x463f38), ring_k=0.78, palp=lt.rgb(0x645d54), palp_k=0.6, joint=lt.rgb(0x5c554c), joint_k=0.45,
    solid={'eye': 0x070606, 'bristle': 0x857d71, 'fang': 0x1e1a16, 'misc': 0x5a534a},
    face=lt.rgb(0x6e675e), face_k=0.6,
    scar=lt.rgb(0x72605a), scar_core=lt.rgb(0xdcd6ca), scar_k=0.9,
    dust=lt.rgb(0x95805a), dust_k=0.75)


def scars(shape, U, V, seed, count):
    """A few old healed slashes across a body region's back: (cut, core) masks, 0..1, the cut's darker edges and its
    pale healed middle. Color only (every hide shares the normal map)."""
    rng = np.random.default_rng(seed)
    cut = np.zeros(shape, np.float32)
    core = np.zeros(shape, np.float32)
    h, w = shape
    for _ in range(count):
        u0, v0 = rng.uniform(0.36, 0.64), rng.uniform(0.25, 0.85)
        angle = rng.uniform(-0.9, 0.9) + (math.pi / 2 if rng.random() < 0.5 else 0.0)
        length, bend = rng.uniform(0.08, 0.16), rng.uniform(-0.04, 0.04)
        nearest = np.full(shape, 9.0, np.float32)
        for t in np.linspace(-0.5, 0.5, 40):
            pu = u0 + math.cos(angle) * length * t - math.sin(angle) * bend * (1 - 4 * t * t)
            pv = v0 + math.sin(angle) * length * t * 2.0 + math.cos(angle) * bend * (1 - 4 * t * t)
            taper = 1.0 - (2 * abs(t)) ** 3                      # thinning to nothing at both ends
            nearest = np.minimum(nearest, np.sqrt(((U - pu) * w) ** 2 + ((V - pv) * h) ** 2) / max(taper, 0.05))
        cut = np.maximum(cut, 1.0 - lt.smooth(2.8, 6.2, nearest))
        core = np.maximum(core, 1.0 - lt.smooth(0.6, 1.8, nearest))
    return cut, core


def segment_coords(along, segments):
    """Where a limb region's pixels lie in their segment: (s 0..1 along it, the segment's index)."""
    s = np.zeros_like(along)
    index = np.zeros(along.shape, np.int32)
    for k, (a, b) in enumerate(segments):
        inside = (along >= a) & (along <= b)
        s = np.where(inside, (along - a) / (b - a), s)
        index = np.where(inside, k, index)
    return s, index


def body_marks(region, U, V, seed, shape, n, hide, col):
    """The marks on the body regions (both painters): the abdomen's field, heart, chevrons and specks; the carapace's
    and head's stripe, lateral bands and margins, the face round the eyes."""
    d, s = lc.body_axes(U, V)
    if region == 'abdomen':
        field = lt.smooth(0.62, 0.42, np.abs(s) + 0.05 * n) * lt.smooth(-0.15, 0.25, d) * lt.smooth(0.02, 0.15, V)
        col = lt.mix(col, hide['field'], field * hide['field_k'])
        width, middle, half = hide['heart_shape']
        heart = ((np.abs(s) < width * np.clip(1 - ((V - middle) / half) ** 2, 0, 1)) & (d > 0)).astype(np.float32)
        if hide['heart_outline']:
            # A ghost of the mark: its outline drawn, its inside only washed.
            soft = lt.blur(heart, 2.5)
            edge = np.clip(4.0 * soft * (1.0 - soft), 0, 1)
            col = lt.mix(col, hide['heart'], np.clip(edge * 0.95 + soft * 0.3, 0, 1) * hide['heart_k'])
        else:
            col = lt.mix(col, hide['heart'], lt.blur(heart, 1.5 * (K if hide.get('painted') else 1.0)) * hide['heart_k'])
        for k in range(4):
            vk = 0.5 - k * 0.11
            line = 1 - lt.smooth(0.012, 0.03, np.abs(V - (vk - 0.22 * np.abs(s))))
            col = lt.mix(col, hide['chevron'], line * (np.abs(s) < 0.42) * (d > 0) * hide['chevron_k'])
        speck_size = K if hide.get('painted') else 1.0
        col = lt.mix(col, hide['speck'], lt.specks(shape, seed + 3, 0.04, speck_size) * hide['speck_k'] * (d > -0.2))
    else:
        stripe = lt.smooth(0.15, 0.08, np.abs(s)) * (d > 0)
        lateral = lt.smooth(0.12, 0.2, np.abs(s)) * lt.smooth(0.72, 0.55, np.abs(s)) * (d > 0)
        col = lt.mix(col, hide['lateral'], lateral * hide['lateral_k'])
        col = lt.mix(col, hide['stripe'], stripe * hide['stripe_k'][0 if region == 'carapace' else 1])
        col = lt.mix(col, hide['margin'], lt.smooth(0.75, 0.92, np.abs(s)) * (d > -0.3) * hide['margin_k'])
        if 'face' in hide and region == 'head':
            face = lt.smooth(0.5, 0.74, V) * lt.smooth(-0.75, -0.25, d) * np.clip(0.85 + 0.25 * n, 0, 1)
            col = lt.mix(col, hide['face'], face * hide['face_k'])
    return col


def paint_painted(region, U, V, seed, hide):
    """The painted hide (BROWN): broad soft values and crisp marks, fur in clumps, sheen along the top of every chitin
    plate, pale rims and dark joints; roughness low on chitin, higher on fur."""
    shape = U.shape
    base, light, belly = hide['base'], hide['light'], hide['belly']
    n = lt.noise(shape, seed, 12.0 * K, 12.0 * K, octaves=3)
    if region in ('abdomen', 'carapace', 'head'):
        d, s = lc.body_axes(U, V)
        broad = lt.noise(shape, seed + 7, 60.0 * K, 60.0 * K, octaves=2)
        col = lt.mix(base, light, np.clip(0.28 + 0.12 * broad, 0, 1))
        col = lt.mix(col, belly, lt.smooth(-0.1, -0.55, d))
        col = body_marks(region, U, V, seed, shape, n, hide, col)
        # Fur in clumps (longer along the body): each a little lighter or darker, dark in the crevices between them.
        cells = lt.worley(shape, (max(int(shape[1] / (22.0 * K)), 4), max(int(shape[0] / (30.0 * K)), 4)), seed + 31,
                          stretch=(1.0, 0.6))
        tuft = lt.smooth(0.0, 8.0 * K, cells['edge'])
        tone = lt.per_cell(cells['id'], cells['count'], seed + 32, -1.0, 1.0)
        strokes = lt.noise(shape, seed + 11, 1.2 * K, 7.0 * K)          # hairs along the body (V runs along the rows)
        shade = (1.0 + 0.05 * tone) * (0.86 + 0.14 * tuft) * (1.0 + 0.03 * strokes)
        shade = shade * (1.0 + 0.1 * lt.smooth(0.2, 1.0, d))            # painted top light
        col = col * shade[..., None]
        height = 0.0011 * tuft + 0.00012 * strokes
        fur = 0.62 if region == 'abdomen' else 0.5
        rough = fur - 0.08 * tuft + 0.04 * strokes + 0.12 * lt.smooth(-0.2, -0.8, d)
        occl = (1.0 - 0.22 * lt.smooth(-0.2, -0.85, d)) * (0.84 + 0.16 * tuft)
        if region == 'carapace':
            # The fovea, a dark dash behind the middle, and grooves from it toward the legs (cm over the carapace).
            dx = (U - 0.5) * 187.0
            dy = (V - 0.44) * 113.0
            r = np.sqrt(dx * dx + dy * dy)
            angle = np.degrees(np.arctan2(np.abs(dx), dy))
            groove = np.zeros(shape, np.float32)
            for a in (40.0, 74.0, 106.0, 140.0):
                groove = np.maximum(groove, np.exp(-((angle - a) / (5.5 + 40.0 / np.maximum(r, 1.0))) ** 2))
            groove = groove * lt.smooth(4.0, 9.0, r) * lt.smooth(34.0, 20.0, r) * (d > 0.2)
            fovea = np.exp(-(dx / 1.4) ** 2 - (dy / 5.0) ** 2)
            col = lt.mix(col, hide['groove'], np.clip(groove * 0.45 + fovea * 0.9, 0, 1))
            height = height - 0.0016 * groove - 0.003 * fovea
            occl = occl * (1.0 - 0.25 * groove - 0.4 * fovea)
        return col, rough, height, occl
    if region == 'chel':
        # The chelicerae: dark, glossy, hairy toward the front.
        along, top = lc.limb_axes(U, V)
        strokes = lt.noise(shape, seed + 13, 1.2 * K, 9.0 * K)
        col = lt.mix(base, hide['palp'], 0.85)
        col = lt.mix(col, light, lt.smooth(0.5, 0.95, top) * 0.18)
        col = col * (1.0 + 0.04 * strokes)[..., None]
        return col, 0.38 + 0.05 * strokes, 0.0001 * strokes, 1.0 - 0.2 * lt.smooth(0.0, -0.8, top)
    along, top = lc.limb_axes(U, V)
    s, index = segment_coords(along, LIMB_SEGMENTS.get(region, ((0.0, 1.0),)))
    lit = lt.smooth(-0.8, 0.6, top)                       # 0 under the leg .. 1 on its top
    col = lt.mix(belly, base, lit)
    if region == 'palp':
        col = lt.mix(col, hide['palp'], hide['palp_k'])
    crown = np.sin(np.pi * np.clip(s, 0.0, 1.0)) ** 0.7  # each plate's lit middle
    joint = 1.0 - lt.smooth(0.0, 0.08, s)                 # the membrane after a joint
    if region == 'tibia':
        joint = joint * (index != 0)                      # the patella's near end is the knee's knob, on show
    shoulder = lt.smooth(0.93, 1.0, s)                    # rolling off into the next joint
    rim = np.exp(-((s - 0.89) / 0.03) ** 2)               # the plate's hard edge
    for k, at in LIMB_BANDS.get(region, ()):
        band = (index == k) * (1.0 - lt.smooth(0.07, 0.13, np.abs(s - at) + 0.01 * n))
        col = lt.mix(col, hide['ring'], band * hide['ring_k'])
    if region == 'tibia':
        col = lt.mix(col, hide['ring'], (index == 0) * 0.22 * hide['ring_k'])                    # the patella
        col = lt.mix(col, hide['ring'], (index == 3) * lt.smooth(0.2, 1.0, s) * 0.8)            # dark to the tip
    if region == 'coxa':
        col = lt.mix(col, belly, 0.3)
    gloss = lt.smooth(0.45, 0.95, top) * crown            # painted sheen along the top of the plate
    col = lt.mix(col, light, gloss * 0.2)
    col = lt.mix(col, light, rim * 0.2 * lit)
    col = lt.mix(col, hide['membrane'], np.clip(joint * 0.85 + shoulder * 0.25, 0.0, 1.0))
    # Pale hairs flecked along the leg (lit side), so it reads furred, not polished.
    flecks = lt.specks(shape, seed + 17, 0.07, 1.2 * K) * lt.smooth(0.0, 0.3, lt.noise(shape, seed + 19, 6.0 * K, 1.0 * K))
    col = lt.mix(col, light * 1.08, flecks * 0.4 * (0.4 + 0.6 * lit))
    hairs = lt.noise(shape, seed + 13, 10.0 * K, 1.0 * K)   # fine hairs along the limb (U runs along the columns)
    col = col * (1.0 + 0.02 * hairs + 0.03 * n)[..., None]
    height = 0.0009 * crown + 0.0006 * rim - 0.0015 * joint - 0.0008 * shoulder + 0.00012 * hairs
    rough = 0.56 - 0.1 * gloss + 0.08 * flecks + 0.25 * np.clip(joint + shoulder, 0.0, 1.0) + 0.04 * hairs
    occl = 1.0 - 0.25 * lt.smooth(0.2, -0.8, top) - 0.3 * joint
    return col, rough, height, occl


def paint(region, U, V, seed, hide=BROWN):
    """Color, roughness, height (meters) and occlusion for one atlas region in a hide (BROWN or BONE, or Gravemother.py's
    GRAVE): a wolf spider, fur running along the body and the limbs."""
    shape = U.shape
    rough = np.full(shape, 0.66, np.float32)
    occl = np.ones(shape, np.float32)
    if region in ('eye', 'bristle', 'fang', 'misc'):
        color = hide['solid'][region]
        rough[:] = {'eye': 0.06, 'bristle': 0.6, 'fang': 0.25, 'misc': 0.7}[region]
        return np.broadcast_to(lt.rgb(color), shape + (3,)).copy(), rough, np.zeros(shape, np.float32), occl
    if hide.get('painted'):
        col, r, height, ao = paint_painted(region, U, V, seed, hide)
        return np.clip(col, 0, 1), np.clip(r, 0, 1), height, np.clip(ao, 0, 1)
    base, light, belly = hide['base'], hide['light'], hide['belly']
    n = lt.noise(shape, seed, 12.0, 12.0, octaves=3)
    if region in ('abdomen', 'carapace', 'head'):
        d, s = lc.body_axes(U, V)
        fur = lt.noise(shape, seed + 11, 0.7, 5.0)            # streaks along the body (V runs along the rows)
        col = lt.mix(base, light, np.clip(0.25 + 0.2 * n, 0, 1))
        col = lt.mix(col, belly, lt.smooth(-0.1, -0.5, d))
        col = body_marks(region, U, V, seed, shape, n, hide, col)
        if 'scar' in hide and region != 'head':
            cut, core = scars(shape, U, V, seed + (41 if region == 'abdomen' else 43), 4 if region == 'abdomen' else 1)
            col = lt.mix(col, hide['scar'], cut * hide['scar_k'])
            col = lt.mix(col, hide['scar_core'], core * 0.75)
        if 'dust' in hide:
            # Grave dust settled on her back: blotchy, thickest along the top, gone down the sides.
            blotch = np.clip(0.55 + 0.6 * lt.noise(shape, seed + 21, 14.0, 9.0, octaves=2), 0, 1)
            grit = lt.specks(shape, seed + 23, 0.12, 1.2)
            col = lt.mix(col, hide['dust'], lt.smooth(0.2, 0.75, d) * (blotch * 0.8 + grit * 0.4) * hide['dust_k'])
        col = col * (1.0 + 0.09 * fur)[..., None]
        height = 0.0004 * fur + 0.0003 * n
        occl = 1.0 - 0.18 * lt.smooth(-0.2, -0.8, d)
        rough = rough + 0.06 * fur
        return col * (1.0 + 0.05 * lt.noise(shape, seed + 5, 3.0, 3.0))[..., None], rough, height, occl
    along, top = lc.limb_axes(U, V)
    fur = lt.noise(shape, seed + 13, 5.0, 0.7)                # streaks along the limb (U runs along the columns)
    col = lt.mix(base, light, np.clip(0.3 + 0.2 * n, 0, 1))
    col = lt.mix(col, belly * 1.05, lt.smooth(0.0, -0.6, top))
    if region == 'femur':
        col = lt.mix(col, hide['ring'], lc.bands(along, (0.42, 0.78), 0.07) * hide['ring_k'])
        if 'dust' in hide:
            col = lt.mix(col, hide['dust'], lt.smooth(0.3, 0.9, top) * 0.25 * hide['dust_k'])
    elif region == 'tibia':
        # Dark knuckles at the joints (TIBIA_JOINTS) and toward the claw.
        col = lt.mix(col, hide['ring'], lc.bands(along, TIBIA_JOINTS + (0.96,), 0.05) * hide['ring_k'])
    elif region in ('palp', 'chel'):
        col = lt.mix(col, hide['palp'], hide['palp_k'])
    else:
        col = lt.mix(col, hide['joint'], hide['joint_k'])
    col = col * (1.0 + 0.09 * fur)[..., None]
    occl = 1.0 - 0.15 * lt.smooth(0.0, -0.8, top)
    return col, rough + 0.06 * fur, 0.0004 * fur + 0.0002 * n, occl


def paint_pale():
    """The Gravemother's color map, Art/Textures/SpiderBody_Pale/T_SpiderBody_Pale_BC.png. paint_atlas writes a whole
    set, and this one's normal and ORM maps would be byte copies of SpiderBody's: the set is painted in a temporary
    folder and only its color map is kept. A review stage that switches lc.paint_atlas off (to build the spider without
    writing its texture sets) paints nothing here, so nothing is copied."""
    out_dir = os.path.join(lt.TEXTURE_DIR, PALE_SET)
    name = f'T_{PALE_SET}_BC.png'
    with tempfile.TemporaryDirectory() as scratch:
        lc.paint_atlas(ATLAS, lambda region, U, V, seed: paint(region, U, V, seed, BONE), SIZE, PX_M, scratch, PALE_SET,
                       normal_strength=NORMAL_STRENGTH)
        painted = os.path.join(scratch, name)
        if os.path.exists(painted):
            os.makedirs(out_dir, exist_ok=True)
            shutil.copyfile(painted, os.path.join(out_dir, name))


texture_dir = os.path.join(lt.TEXTURE_DIR, SET)
os.makedirs(texture_dir, exist_ok=True)
lc.paint_atlas(ATLAS, paint, SIZE, PX_M, texture_dir, SET, normal_strength=NORMAL_STRENGTH)
paint_pale()
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


def ring_frame(curve, t, plane, flip, dt=0.01):
    """The tangent, the top (toward the pole, in the leg's plane) and the side at t on a curve."""
    tangent = (curve(t + dt) - curve(t - dt)).normalized()
    top = (plane.cross(tangent) * flip).normalized()
    return tangent, top, tangent.cross(top)


def segment(m, curve, t0, t1, radius, region, u0, u1, sides, stations, plane, up, tip=None, round_start=False):
    """A limb segment: a smooth tapered tube along curve(t) from t0 to t1 with radius(t, s) (s 0..1 along it), U from u0
    to u1 of its region, V round it (0.5 its top). Its far end is rounded off (a shoulder ring, then a point a little
    beyond), so no flat cap shows at a joint; its near end, inside the segment before, closes to a low point; tip: the far
    end closes to that point instead (the claw); round_start: the near end is rounded as the far one (the knee's knob,
    on top of the joint where it shows)."""
    flip = -1.0 if plane.cross((curve(t1) - curve(t0)).normalized()).dot(up) < 0 else 1.0
    rows = []      # (centre, radius, u, frame)
    if isinstance(stations, int):
        stations = [k / stations for k in range(stations + 1)]
    for s in stations:
        t = t0 + (t1 - t0) * s
        rows.append((curve(t), radius(t, s), u0 + (u1 - u0) * s, ring_frame(curve, t, plane, flip)))
    start_tan, end_tan = rows[0][3][0], rows[-1][3][0]
    r0, r1 = rows[0][1], rows[-1][1]
    if round_start:
        rows.insert(0, (rows[0][0] - start_tan * r0 * 0.38, r0 * 0.8, u0, rows[0][3]))
        hub0 = rows[0][0] - start_tan * r0 * 0.32
    else:
        hub0 = rows[0][0] - start_tan * r0 * 0.5      # its neck sits inside the segment before: a low point will do
    if tip is None:
        rows.append((rows[-1][0] + end_tan * r1 * 0.38, r1 * 0.8, u1, rows[-1][3]))
        hub1 = rows[-1][0] + end_tan * r1 * 0.32
    else:
        hub1 = tip
    bm = m.bm
    rings_v = []
    for centre, r, u, (tangent, top, side) in rows:
        rings_v.append([bm.verts.new(centre + (top * math.cos(a) + side * math.sin(a)) * r)
                        for a in (-math.pi + 2.0 * math.pi * j / sides for j in range(sides))])
    uv = lambda u, v: m.at(region, u, v)
    for k in range(len(rows) - 1):
        ua, ub = rows[k][2], rows[k + 1][2]
        for j in range(sides):
            j1 = (j + 1) % sides
            va, vb = j / sides, (j + 1) / sides
            m.face((rings_v[k][j], rings_v[k + 1][j], rings_v[k + 1][j1], rings_v[k][j1]),
                   [uv(ua, va), uv(ub, va), uv(ub, vb), uv(ua, vb)])
    for ring, hub, end_u, first in ((rings_v[0], hub0, u0, True), (rings_v[-1], hub1, u1, False)):
        h = bm.verts.new(hub)
        for j in range(sides):
            j1 = (j + 1) % sides
            m.face((h, ring[j1], ring[j]) if first else (h, ring[j], ring[j1]),
                   [uv(end_u, 0.5), uv(end_u, (j + 1) / sides), uv(end_u, j / sides)])


def leg_hair(m, curve, radius_at, t, around, length, lean, plane, up):
    """A spine or hair on a limb at t, round it by around degrees from its top, leaning lean degrees toward the tip."""
    flip = -1.0 if plane.cross((curve(1.0) - curve(0.0)).normalized()).dot(up) < 0 else 1.0
    tangent, top, side = ring_frame(curve, t, plane, flip)
    a = math.radians(around)
    normal = top * math.cos(a) + side * math.sin(a)
    lean = math.radians(lean)
    lc.bristle(m, curve(t) + normal * radius_at(t) * 0.82, normal * math.cos(lean) + tangent * math.sin(lean),
               length, HAIR_RADIUS, 'bristle')


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
        lc.ellipsoid(m, eye, (r, r, r), 'eye', segs=10 if big else 6, rings=7 if big else 4, patch=True)
bones.append(('head', lifted((26.0, 0.0, 8.0)), lifted((46.0, 0.0, 8.0)), 'body'))

# Pedipalps: jointed feelers reaching forward and down, a femur, a short patella and a slightly clubbed tibia and tarsus.
for side, suffix in SIDES:
    start = lifted((PALP['x'], side * PALP['y'], PALP['z']))
    elbow = start + Vector((PALP['length'] * 0.55, side * 4.0, -PALP['length'] * 0.35))
    tip = elbow + Vector((PALP['length'] * 0.3, side * 1.0, -PALP['length'] * 0.55))
    m = part(f'palp_{suffix}')
    r = PALP['radius']
    plane = (elbow - start).cross(tip - elbow).normalized()
    up = Vector((0.0, 0.0, 1.0))
    upper = lambda t, a=start, b=elbow: a.lerp(b, t)
    lower = lambda t, a=elbow, b=tip: a.lerp(b, t)
    segment(m, upper, -0.04, 1.0, lambda t, s: r * 0.94 * plate(s, 1.0, 0.85), 'palp', 0.0, 0.45, 8, 3, plane, up)
    segment(m, lower, -0.06, 0.24, lambda t, s: r * 0.92 * plate(s, 1.05, 0.82), 'palp', 0.45, 0.6, 8, 2, plane, up)
    segment(m, lower, 0.2, 1.0, lambda t, s: r * 0.8 * (0.84 + 0.3 * math.sin(math.pi * min(s * 0.62 + 0.1, 1.0))),
            'palp', 0.6, 1.0, 8, 3, plane, up)
    hm = bristles(f'palp_{suffix}')
    for k in range(6):
        leg_hair(hm, lower, lambda t: r * 0.95, 0.55 + 0.07 * k, rng.uniform(-140, 140), HAIR_LENGTH * rng.uniform(0.3, 0.5),
                 55.0, plane, up)
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

# Legs, standing: each foot on the ground at its resting spot. The femur bone carries the coxa, trochanter and femur
# (arched); the tibia bone the patella, tibia, metatarsus and tarsus (bowed) to the claw; spines and hairs on both.
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
        fdir = (knee - hip).normalized()
        fbend = (pole - fdir * pole.dot(fdir)).normalized()
        tdir = (foot - knee).normalized()
        tbend = (pole - tdir * pole.dot(tdir)).normalized()
        fcurve = lambda t, h=hip, k=knee, b=fbend, n=FEMUR[pair]: h.lerp(k, t) + b * (math.sin(math.pi * t) * n * 0.05)
        tcurve = lambda t, k=knee, f=foot, b=tbend, n=TIBIA[pair]: k.lerp(f, t) + b * (math.sin(math.pi * t) * n * 0.06)
        m = part(f'femur_{leg}')
        # The coxa swells from the body, then pinches to the trochanter's ring (painted: LIMB_SEGMENTS) before the femur.
        coxa = lambda t, s: r * (0.95 + 0.22 * math.sin(math.pi * min(s * 1.25 + 0.1, 1.0)) - 0.08 * lc.smoothstep(0.7, 1.0, s))
        upper = {'coxa': coxa,
                 'femur': lambda t, s: femur_radius(t, r) * plate(s, 1.0, 0.84)}
        for name, t0, t1, region, u0, u1, sides, rings in FEMUR_SEGS:
            segment(m, fcurve, t0, t1, upper[name], region, u0, u1, sides, rings, plane, pole)
        m = part(f'tibia_{leg}')
        lower = {'patella': lambda t, s: tibia_radius(t, r) * (1.0 + 0.16 * math.sin(math.pi * min(s * 0.9 + 0.05, 1.0))),
                 'tibia': lambda t, s: tibia_radius(t, r) * plate(s, 1.0, 0.8),
                 'metatarsus': lambda t, s: tibia_radius(t, r) * plate(s, 1.0, 0.82),
                 'tarsus': lambda t, s: tibia_radius(t, r) * plate(s, 0.95, 0.85) * (1.0 - 0.35 * s ** 2)}
        for name, t0, t1, region, u0, u1, sides, rings in TIBIA_SEGS:
            segment(m, tcurve, t0, t1, lower[name], region, u0, u1, sides, rings, plane, pole,
                    tip=foot if name == 'tarsus' else None, round_start=name == 'patella')
        # Spines (mirrored pairs where they stand off the top), then short loose hairs, all leaning toward the tip.
        for bone, curve, radius_at, spines in ((f'femur_{leg}', fcurve, lambda t: femur_radius(t, r), FEMUR_SPINES),
                                               (f'tibia_{leg}', tcurve, lambda t: tibia_radius(t, r), TIBIA_SPINES)):
            hm = bristles(bone)
            for t, around, length in spines:
                for a in ((around, -around) if 0.0 < around < 180.0 else (around,)):
                    leg_hair(hm, curve, radius_at, t, a + rng.uniform(-8, 8), HAIR_LENGTH * length * rng.uniform(0.9, 1.1),
                             52.0, plane, pole)
            count, low, high = LOOSE_HAIRS['femur' if bone.startswith('femur') else 'tibia']
            for _ in range(count):
                leg_hair(hm, curve, radius_at, rng.uniform(0.05, 0.92), rng.uniform(-115, 115),
                         HAIR_LENGTH * rng.uniform(low, high), 57.0, plane, pole)
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
