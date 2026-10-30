"""The Gravemother dressed for her Barrow fight (Saved/Gravemother/CONTRACT.md, BARROW-ART, 2026-10-19; the user's concept
D "Barrow", Saved/Concepts/Gravemother/Barrow_Hero.png): SK_Gravemother, Spider.py's mesh and rig as they are (its parts
built by Spider.py itself, its bones by their names, so ASpiderCreature's gait, leg IK, attack and death curl drive her
unchanged), in her own hide and dressing. The game draws her at 1.8x, as it drew SK_Spider for her.

What's hers (Tools/Blender/looter_barrow.py has the shared pieces):
  - The hide, texture set Gravemother (2048 px, Spider.py's atlas): her own look since CHAR-ART (2026-10-22; GRAVE
    below): Spider.py's painter in a pale corpse-bone palette (#C2B9A6) with the spider's marks only faded ghosts, the
    hide crazed like old bone, dark root veins spreading from where the stone's roots go in, lichen round the stone,
    grave earth climbing her from below (her belly, every leg's underside, the lower legs dark and wet), old scars and
    grave dust. (Until then a mud-grey palette, #8C8070, that read as a grey re-tint of the spider.) Under where the
    headstone sits her back is the pale Bone hide (#BAB2A1), bald and pressed, with dark puckers where the roots go in:
    hidden by the stone and the mud round its foot, bare once the stone cracks off.
  - The abdomen has a slot of its own, GravemotherAbdomen (the same maps). GravemotherBare (texture set GravemotherBare,
    the same atlas, the abdomen all pale Bone as the game's MI_SpiderBody_Pale has it, its root puckers and the stone's
    pale print) is for that slot once the stone is off: Tools/Unreal/build_grave_fight_materials.py makes
    MI_GravemotherBare; the game may swap it in (SetMaterialByName GravemotherAbdomen). Without the swap the bare print
    shows alone.
  - Grave earth (GravemotherMud, dark and wet since CHAR-ART: looter_barrow.paint_mud) caked on every lower leg: a crusted sleeve thickening toward the claw, its
    top ragged, clods stuck on it.
  - Root stubs: where the roots that bind the stone go into her hide (looter_barrow.ROOTS), their last few cm stand out
    of her back, snapped and splintered: hidden inside the roots while the stone is on (GraveRoots, the oak bark tinted
    #5B4330).
  - The abdomen's bristles under the stone's foot are left out (they'd come through the stone; and the print is bald).
  - Hit zones as SK_Spider's (a convex hull round every bone's parts, the dressing included), and the abdomen's soft rear:
    a sphere on the abdomen bone (USP_AbdomenRear), the only round shape on that bone. The importer keeps one body per
    bone and drops shape names, so the game tells the rear by its shape: a hit on the abdomen within the sphere (a few cm
    of slack) is the soft rear (Saved/Gravemother/PLAN.md, 5.4).

    Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\Gravemother.py -Preview -Threads 6
        (Saved/ArtPreviews/Creatures/Gravemother/Gravemother_*.png: her with the headstone, from behind, bare, her legs)
    Tools\\artrun.ps1 -Export Gravemother,GravemotherStone,GraveFight -Family Gravemother
"""
import math
import os
import random
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector, noise

import looter_barrow as lb
import looter_barrow_stone as lbs
import looter_creatures as lc
import looter_model as lm
import looter_textures as lt

# The user's Tripo remake (2026-10-29, Art/Models/Creatures/Tripo/Gravemother.glb): her body rebuilt from it as clean
# quads on the spider's same bones and rest pose (six legs made eight, the tilt stood level, the size the spider's),
# painted by this script's painter on the same atlas (parts_gravemother_look.py; parts_gravemother_build.py for the
# shape). LOOTER_GRAVEMOTHER_TRIPO=0 keeps the scripted body (Spider.py's parts dressed).
TRIPO = os.environ.get('LOOTER_GRAVEMOTHER_TRIPO', '1') != '0'

SET, BARE_SET = 'Gravemother', 'GravemotherBare'
SIZE = 2048
PX_M = 0.003                 # Spider.py's per-pixel slope scale: the same relief per pixel as the spider's
NORMAL_STRENGTH = 1.2
# The soft rear (creature cm): a sphere round the abdomen's rear cap, below the stone's back edge.
REAR_AT, REAR_RADIUS = -0.6, 30.0          # its centre on the abdomen's axis at this a; cm
UV_MUD = lb.SCALE * 0.01 * 0.5             # GravemotherMud's repeat per creature cm (she is 1.8x)
UV_ROOT = lb.ROOT_UV_PER_CM

S = lb.spider_parts()
if TRIPO:
    sys.path.insert(0, os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Tripo'))
    import parts_gravemother_build as gb
    import parts_gravemother_look as gl
    # Her Tripo abdomen (an egg of its own) in S, looter_barrow told so: the stone keeps the game's place.
    S = gl.tripo_spider(S)
ATLAS = S['ATLAS']
AB = lb.Abdomen(S)
STONE = lb.stone_frame(S)
STONE_INV = STONE.inverted()

# --- The hides ---

EARTH_SMEAR = lt.rgb(0x5f4f3e)

# Her own look (CHAR-ART item 4b, the user's OK 2026-10-22: "not a grey spider re-tint: her own material/texture identity,
# the headstone, earth and pale hide"). The mud-grey hide above read as the spider's pattern in grey. GRAVE: a pale,
# corpse-bone hide (#C2B9A6, under 0.75 so the sun still shades it) whose spider marks are only faded ghosts (rings
# kept as dark knuckles at the joints), crazed like old bone, dark root veins spreading from where the stone's roots go
# in, lichen near the stone; grave earth climbing her from below (her belly and the underside of every leg dark, wet
# earth on the lower legs: looter_barrow.paint_mud), so she reads pale on top and dark at the ground from across the Sink.
GRAVE = dict(
    painted=True,         # Spider.py's painted hide (SPIDER-LOOK, 2026-10-22: setae tufts, crisp marks, matte)
    base=lt.rgb(0xc2b9a6), light=lt.rgb(0xd3ccbc), belly=lt.rgb(0x5e5042),
    field=lt.rgb(0xa39a8a), field_k=0.35, heart=lt.rgb(0x6b6359), heart_k=0.6, heart_shape=(0.15, 0.76, 0.23),
    heart_outline=True, chevron=lt.rgb(0x7b7368), chevron_k=0.35, speck=lt.rgb(0x8f877b), speck_k=0.3,
    lateral=lt.rgb(0x968e80), lateral_k=0.4, stripe=lt.rgb(0xd6cfc0), stripe_k=(0.4, 0.3),
    margin=lt.rgb(0xcbc4b4), margin_k=0.4,
    ring=lt.rgb(0x5d554b), ring_k=0.45, palp=lt.rgb(0x6e665b), palp_k=0.55, joint=lt.rgb(0x4a4239), joint_k=0.6,
    solid={'eye': 0x070606, 'bristle': 0x6b6258, 'fang': 0x1e1a16, 'misc': 0x5a534a},
    face=lt.rgb(0x5e574e), face_k=0.65,
    scar=lt.rgb(0x8a7d72), scar_core=lt.rgb(0xe2dccf), scar_k=0.8,
    dust=lt.rgb(0x8a7556), dust_k=0.5)
GRAVE_EARTH = lt.rgb(0x433629)       # wet grave earth on her belly and under her legs (#4F4134 read faint, ART-FIX)
VEIN = lt.rgb(0x3e3128)              # the roots' dark veins under the pale hide
LICHEN = lt.rgb(0x7d8466)            # grey-green lichen near the stone
CRAZE = lt.rgb(0x7d7364)             # the fine crazing of old bone


def grave_marks(region, U, V, seed, col, rough, height, occl):
    """What makes her the Gravemother and not a pale spider (GRAVE): the hide crazed like old bone on top, earth
    climbing from below (the belly, the undersides, the lower legs), the knuckles dark."""
    shape = U.shape
    if region in ('abdomen', 'carapace', 'head'):
        d, s = lc.body_axes(U, V)
        cells = lt.worley(shape, (int(shape[1] / 18), int(shape[0] / 18)), seed + 950)
        craze = (1.0 - lt.smooth(0.3, 1.3, cells['edge'])) * lt.smooth(-0.1, 0.5, d) * \
            lt.smooth(-0.3, 0.6, lt.noise(shape, seed + 951, 20.0))
        col = lt.mix(col, CRAZE, craze * 0.45)
        height = height - 0.0003 * craze
        earth = lt.smooth(0.05, -0.65, d + 0.12 * lt.noise(shape, seed + 952, 10.0, octaves=2))
        crumbs = lt.specks(shape, seed + 953, 0.15, 1.6) * lt.smooth(0.25, -0.1, d)
        col = lt.mix(col, GRAVE_EARTH, np.clip(earth * 0.85 + crumbs * 0.5, 0.0, 1.0))
        rough = mix1(rough, 0.92, earth)
        occl = occl * (1.0 - 0.15 * earth)
    elif region in ('femur', 'tibia', 'coxa', 'palp', 'chel'):
        along, top = lc.limb_axes(U, V)
        # (ART-FIX 2026-10-22: the earth read faint in game: the whole lower leg darkens, deepest toward the claw, the
        # earth wraps more of every underside and covers fully; a ragged edge round the knee.)
        ragged = 0.12 * lt.noise(shape, seed + 955, 14.0, 4.0, octaves=2)
        low = lt.smooth(0.45, -0.5, top + 0.15 * lt.noise(shape, seed + 954, 8.0, octaves=2))      # the underside
        reach = {'femur': 0.6 * lt.smooth(0.6, 1.0, along + ragged),
                 'tibia': 0.55 + 0.45 * lt.smooth(0.0, 0.5, along + ragged)}.get(region, 0.0)
        earth = np.clip(low * 0.75 + reach, 0.0, 1.0)
        col = lt.mix(col, GRAVE_EARTH, earth * 0.92)
        rough = mix1(rough, 0.92, earth)
    return col, rough, height, occl


def root_veins(U, V, shape, seed):
    """Dark veins under the abdomen's pale hide, spreading from where each of the stone's roots goes in (fading by about
    45 cm), forked and wandering: 0..1."""
    P = AB.from_uv(U, V)
    near = np.zeros(shape, np.float32)
    for root in lb.ROOTS:
        p, _ = lb.root_anchor(S, root)
        dist = np.linalg.norm(P - np.array(p, np.float32), axis=-1)
        near = np.maximum(near, lt.smooth(45.0, 6.0, dist))
    n = lt.noise(shape, seed + 960, 9.0, 18.0, octaves=2)
    slope = np.abs(np.roll(n, -1, axis=1) - np.roll(n, 1, axis=1)) * 0.5 + np.abs(np.roll(n, -1, axis=0) - np.roll(n, 1, axis=0)) * 0.5 + 1e-4
    line = 1.0 - lt.smooth(0.0, 1.1, np.abs(n) / slope)
    fork = lt.smooth(-0.2, 0.6, lt.noise(shape, seed + 961, 30.0))
    return line * fork * near


def mix1(a, b, t):
    """A single-channel image toward b by t (lt.mix is for colours: it gives t a channel axis)."""
    return a + (b - a) * t


def abdomen_marks(U, V, shape, seed):
    """Masks on the abdomen by place: the stone's print (where its foot and the mud round it lay; 1 under the foot,
    0 well clear of the mud), its foot's own pressed rectangle, and the puckers where the roots go in."""
    P = AB.from_uv(U, V)                                          # creature cm
    M = np.array(STONE_INV, np.float32)
    Q = P @ M[:3, :3].T + M[:3, 3]                                # stone cm
    ragged = 2.5 * lt.noise(shape, seed + 1, 22.0, octaves=2)
    ex = np.maximum(np.abs(Q[..., 0]) - (lb.STONE_W / 2.0 + 7.0), 0.0)
    ey = np.maximum(np.abs(Q[..., 1]) - (lb.STONE_T / 2.0 + 10.0), 0.0)
    # Only on her back: points below the foot (her flanks and belly, z far under it) are clear of it.
    out = np.hypot(ex, ey) + np.maximum(Q[..., 2] - 30.0, 0.0) + np.maximum(-26.0 - Q[..., 2], 0.0) * 1.5 + ragged
    pr = lt.smooth(6.0, 0.0, out)
    fx = np.maximum(np.abs(Q[..., 0]) - lb.STONE_W / 2.0, 0.0)
    fy = np.maximum(np.abs(Q[..., 1]) - lb.STONE_T / 2.0, 0.0)
    foot = lt.smooth(1.5, 0.0, np.hypot(fx, fy) + 0.6 * ragged) * lt.smooth(28.0, 12.0, Q[..., 2]) * lt.smooth(-30.0, -18.0, Q[..., 2])
    pucker = np.zeros(shape, np.float32)
    ring = np.zeros(shape, np.float32)
    for root in lb.ROOTS:
        p, _ = lb.root_anchor(S, root)
        d = np.linalg.norm(P - np.array(p, np.float32), axis=-1)
        r = root['r1']
        pucker = np.maximum(pucker, lt.smooth(r * 1.05, r * 0.6, d))
        ring = np.maximum(ring, lt.smooth(r * 2.6, r * 1.1, d) * (1.0 - lt.smooth(r * 1.05, r * 0.6, d)))
    return pr, foot, pucker, ring


def overlay(region, U, V, seed, col, rough, height, occl, bare):
    """What she wears over the spider's painted hide: mud on her lower legs and knees, earth on her belly, the stone's
    pale print on her back (bare: the abdomen all pale Bone, the print only pressed in)."""
    shape = U.shape
    n = lt.noise(shape, seed + 900, 10.0, octaves=3)
    if region == 'tibia':
        along, top = lc.limb_axes(U, V)
        edge = 0.5 + 0.08 * lt.noise(shape, seed + 901, 30.0, 6.0)
        caked = lt.smooth(edge - 0.04, edge + 0.08, along)
        cracks = lt.worley(shape, (int(shape[1] / 24), int(shape[0] / 16)), seed + 902)
        crack = (1.0 - lt.smooth(0.25, 1.1, cracks["edge"])) * lt.smooth(-0.5, 0.5, lt.noise(shape, seed + 910, 12.0))
        mud = lt.mix(lt.rgb(0x66533f), lt.rgb(0x4d3e31), lt.smooth(-1.0, 1.0, n))
        mud = lt.mix(mud, lt.rgb(0x33281f), crack * 0.6)
        splash = lt.specks(shape, seed + 903, 0.18, 2.5) * lt.smooth(edge - 0.25, edge, along)
        col = lt.mix(col, mud, np.clip(caked + splash * 0.8, 0.0, 1.0))
        height = height + caked * (0.0012 * lt.noise(shape, seed + 904, 3.0, octaves=2) - 0.0008 * crack)
        rough = mix1(rough, 0.93, caked)
    elif region == 'femur':
        along, top = lc.limb_axes(U, V)
        splash = lt.specks(shape, seed + 905, 0.22, 2.2) * lt.smooth(0.65, 0.95, along)
        smear = lt.smooth(0.8, 1.0, along) * lt.smooth(-0.6, 0.6, n)
        col = lt.mix(col, lt.rgb(0x6e5a46), np.clip(splash * 0.8 + smear * 0.5, 0.0, 1.0))
        rough = mix1(rough, 0.9, splash)
    elif region == 'coxa':
        col = lt.mix(col, lt.rgb(0x6e5a46), lt.specks(shape, seed + 906, 0.25, 2.0) * 0.6)
    if region in ('abdomen', 'carapace', 'head'):
        d, s = lc.body_axes(U, V)
        belly = lt.smooth(-0.15, -0.6, d) * lt.smooth(-0.2, 0.9, n)
        col = lt.mix(col, EARTH_SMEAR, belly * 0.55)
        rough = mix1(rough, 0.9, belly * 0.5)
    if region == 'abdomen':
        pr, foot, pucker, ring = abdomen_marks(U, V, shape, seed)
        bald = pr if not bare else np.ones(shape, np.float32)
        pale = lt.mix(lt.rgb(0xbab2a1), lt.rgb(0xcac3b3), lt.smooth(-1.0, 1.4, lt.noise(shape, seed + 907, 40.0)))
        pale = pale * (1.0 + 0.04 * lt.noise(shape, seed + 908, 2.0))[..., None]
        if not bare:
            # Long under the stone: no fur's streaks, no dust, a little bruised at the print's edge.
            col = lt.mix(col, pale, pr)
            edge = pr * (1.0 - pr) * 4.0
            col = lt.mix(col, lt.rgb(0x8c8274), edge * 0.5)
        col = lt.mix(col, lt.rgb(0xa79f8f), foot * 0.6)
        col = lt.mix(col, lt.rgb(0x7a6f63), ring * 0.7)
        # The roots' veins under the hide, and grey-green lichen round the stone's foot (GRAVE).
        veins = root_veins(U, V, shape, seed) * (1.0 - foot)
        col = lt.mix(col, VEIN, veins * 0.75)
        height = height - 0.0005 * veins
        lichen = lt.specks(shape, seed + 962, 0.22, 2.4) * lt.smooth(0.6, 0.05, pr) * (pr > 0.01) * \
            lt.smooth(-0.2, 0.6, lt.noise(shape, seed + 963, 16.0))
        col = lt.mix(col, LICHEN, np.clip(lichen * 0.8, 0.0, 1.0))
        col = lt.mix(col, lt.rgb(0x2a231d), pucker)
        # Grave earth crumbs left round the print.
        crumbs = lt.specks(shape, seed + 909, 0.12, 1.8) * (pr > 0.02) * (1.0 - pr) * 2.0
        col = lt.mix(col, lt.rgb(0x6e5c49), np.clip(crumbs, 0.0, 1.0) * 0.8)
        height = height - 0.0004 * foot + 0.0006 * ring - 0.0015 * pucker
        rough = mix1(rough, 0.48, bald * (1.0 - pucker) * (0.8 if bare else 1.0))
        occl = occl * (1.0 - 0.35 * pucker)
    return col, rough, height, occl


def hide_painter(bare):
    paint = S['paint']
    palette = S['BONE'] if bare else GRAVE

    def painter(region, U, V, seed):
        col, rough, height, occl = paint(region, U, V, seed, palette)
        col, rough, height, occl = grave_marks(region, U, V, seed, col, rough, height, occl)
        col, rough, height, occl = overlay(region, U, V, seed, col, rough, height, occl, bare)
        return np.clip(col, 0.0, 1.0), np.clip(rough, 0.0, 1.0), height, np.clip(occl, 0.0, 1.0)
    return painter


KEEP = 'keep' in lb.argv()          # -ScriptArgs keep: the texture sets as they are (a quicker look at the mesh)
for name, bare in ((SET, False), (BARE_SET, True)):
    folder = os.path.join(lt.TEXTURE_DIR, name)
    if KEEP and all(os.path.exists(lt.texture_path(name, k)) for k in ('BC', 'N', 'ORM')):
        continue
    os.makedirs(folder, exist_ok=True)
    lc.paint_atlas(ATLAS, hide_painter(bare), SIZE, PX_M, folder, name, normal_strength=NORMAL_STRENGTH)
    lb.log(f'{name}: painted ({SIZE} px)')
lb.ensure_set(lb.MUD_SET, lb.paint_mud, always=not KEEP)

HIDE = lt.material(SET, name='GravemotherHide')
ABDOMEN = lt.material(SET, name='GravemotherAbdomen')
MUD = lb.mud()
ROOTS = lb.roots()

# --- The dressing (creature cm) ---

parts, hairs, bones = S['parts'], S['hairs'], S['bones']
BONE_AT = {name: (Vector(h), Vector(t)) for name, h, t, p in bones}
rng = random.Random(1907)


def bald_under_stone():
    """The abdomen's bristles under the stone's foot and the mud round it, each bristle whole (lc.bristle: three ring
    points, then its tip)."""
    bm = hairs['abdomen'].bm
    bm.verts.ensure_lookup_table()
    gone = []
    verts = list(bm.verts)
    for k in range(0, len(verts) - 3, 4):
        base = (verts[k].co + verts[k + 1].co + verts[k + 2].co) / 3.0
        q = STONE_INV @ base
        if abs(q.x) < lb.STONE_W / 2.0 + 9.0 and abs(q.y) < lb.STONE_T / 2.0 + 12.0 and q.z < 32.0:
            gone += verts[k:k + 4]
    bmesh.ops.delete(bm, geom=gone, context='VERTS')
    return len(gone) // 4


def tibia_curve(pair, side, suffix):
    """Spider.py's bowed tibia for a leg (its points and radii as functions of t, 0 knee .. 1 claw) and the leg's plane."""
    leg = f'{pair}_{suffix}'
    hip, knee = BONE_AT[f'femur_{leg}']
    _, foot = BONE_AT[f'tibia_{leg}']
    pole = S['knee_pole'](hip)
    plane = (knee - hip).cross(foot - knee).normalized()
    tdir = (foot - knee).normalized()
    tbend = (pole - tdir * pole.dot(tdir)).normalized()
    length = S['TIBIA'][pair]
    r = S['LEG_RADIUS']

    def point(t):
        return knee.lerp(foot, t) + tbend * (math.sin(math.pi * t) * length * 0.06)

    def radius(t):
        return r * (0.84 - 0.62 * t ** 1.1)
    return point, radius, plane, pole


def mud_sleeve(bm, pair, side, suffix, seed):
    """Grave mud caked on a lower leg: a crust round the tibia from a ragged line part way down, thickening toward the
    claw and thinning off again just above it, lumpy and cracked, a couple of clods stuck on."""
    point, radius, plane, pole = tibia_curve(pair, side, suffix)
    rings, sides = 18, 12
    t0, t1 = 0.33, 0.95
    grid = []
    for k in range(rings + 1):
        t = t0 + (t1 - t0) * k / rings
        p = point(t)
        tan = (point(min(t + 0.01, 1.0)) - point(max(t - 0.01, 0.0))).normalized()
        ref = plane.cross(tan).normalized()
        if ref.dot(pole) < 0:
            ref = -ref
        side_v = tan.cross(ref)
        row = []
        for j in range(sides):
            a = 2.0 * math.pi * j / sides
            d = ref * math.cos(a) + side_v * math.sin(a)
            start = 0.5 + 0.09 * noise.noise(Vector((math.cos(a) * 1.3, math.sin(a) * 1.3, seed * 0.7)))
            on = lc.smoothstep(start - 0.02, start + 0.1, t)
            thick = on * (0.35 + 0.65 * lc.smoothstep(0.55, 0.88, t)) * lc.smoothstep(0.97, 0.88, t)
            # Patchy: caked thick in places, a thin skin elsewhere.
            patch = noise.noise(Vector((math.cos(a), math.sin(a), t * 5.0 + seed)))
            thick *= 0.3 + 0.7 * lc.smoothstep(-0.35, 0.45, patch)
            thick *= 1.0 + 0.45 * lb.fbm(p * 0.22 + d * 1.1 + Vector((seed, 0.0, 0.0)), 3)
            r = radius(t) * (0.96 + 0.6 * max(thick, 0.0))
            row.append(bm.verts.new(p + d * r))
        grid.append(row)
    for k in range(rings):
        for j in range(sides):
            j1 = (j + 1) % sides
            f = bm.faces.new((grid[k][j], grid[k + 1][j], grid[k + 1][j1], grid[k][j1]))
            f.material_index = 0
    for k, end in ((0, t0 - 0.02), (rings, t1 + 0.015)):
        hub = bm.verts.new(point(end))
        for j in range(sides):
            j1 = (j + 1) % sides
            bm.faces.new((hub, grid[k][j1], grid[k][j]) if k == 0 else (hub, grid[k][j], grid[k][j1]))
    # Clods stuck on, low on the leg.
    for c in range(2):
        t = rng.uniform(0.6, 0.86)
        p = point(t)
        tan = (point(t + 0.01) - point(t - 0.01)).normalized()
        out = plane.cross(tan).normalized() * rng.choice((-1.0, 1.0))
        out = (out + Vector((0.0, 0.0, -0.4))).normalized()
        rr = radius(t) * rng.uniform(0.55, 0.85)
        R = lb.frames([p - tan, p + tan])[0]
        lb.blob(bm, p + out * radius(t) * 1.25, (rr * 1.5, rr, rr * 0.9), Matrix((R[0], R[1], R[2])).transposed(),
                seed=seed * 10 + c, lump=0.35, subdiv=2)


def root_stubs(bm):
    """The roots' ends standing out of her hide where they dive into it (looter_barrow.root_dive): snapped a hand's
    length out, splintered. While the stone is on they are inside the roots' own last stretch (a hair thinner)."""
    for i, root in enumerate(lb.ROOTS):
        anchor, back = lb.root_dive(S, root)
        r = root['r1']
        length = rng.uniform(0.5, 0.75) * lb.ROOT_DIVE_CM
        pts = [anchor - back * 2.5, anchor + back * (length * 0.5), anchor + back * length]
        lb.tube(bm, pts, [r * 0.92, r * 0.88, r * 0.8], sides=7, uv_per=UV_ROOT, wobble=0.12, seed=40 + i,
                caps=(True, False))
        tip = pts[-1]
        for k in range(5):
            jitter = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))) * 0.5
            end = tip + (back + jitter).normalized() * rng.uniform(1.0, 3.6) * (r / 3.0)
            base = tip - back * 0.8 + jitter.normalized() * r * 0.35
            lb.tube(bm, [base, end], [r * 0.38, 0.1], sides=4, uv_per=UV_ROOT)
        # The torn end itself: a flat ragged cap over the tube's open end.
        a = back.orthogonal().normalized()
        lb.blob(bm, tip, (r * 0.8, r * 0.8, r * 0.3), Matrix((a, back.cross(a), back)).transposed(), seed=60 + i,
                lump=0.2, subdiv=1)


def finish(bm, name, material, uv_per):
    """A dressing piece as a Blender object (creature cm to Blender metres), tiling cube UVs in its own cm."""
    bm.normal_update()
    lb.box_uv_faces(bm, list(bm.faces), uv_per)
    bmesh.ops.transform(bm, matrix=lc.TO_BLENDER, verts=bm.verts)
    return lb.obj_from_bm(name, bm, [material], smooth=True, sharp=75.0)


if TRIPO:
    # The Tripo body (parts_gravemother_look.body): its pieces rigid on the spider's bones, mud on its own tibias, the
    # skulls in BoneDry, a hull round each bone's pieces; the root stubs as before.
    bm = lb.new_bm()
    root_stubs(bm)
    stubs = finish(bm, 'RootStubs', ROOTS, UV_ROOT)
    rig, gravemother, bones = gl.body(S, dict(hide=HIDE, abdomen=ABDOMEN, mud=MUD, bone=lb.bone()), stubs)
    removed = 0
else:
    removed = bald_under_stone()
    dressing = {}
    for pair in range(4):
        for side, suffix in S['SIDES']:
            bm = lb.new_bm()
            mud_sleeve(bm, pair, side, suffix, seed=pair * 2 + (1 if side > 0 else 0))
            dressing.setdefault(f'tibia_{pair}_{suffix}', []).append(finish(bm, f'Mud{pair}{suffix.upper()}', MUD,
                                                                           UV_MUD))
    bm = lb.new_bm()
    root_stubs(bm)
    dressing['abdomen'] = [finish(bm, 'RootStubs', ROOTS, UV_ROOT)]
    # The rig (the spider's), the skin, the hit zones.
    rig = lm.armature()
    lm.bones(rig, [(name, lc.to_blender(head), lc.to_blender(tail), parent) for name, head, tail, parent in bones])
    skin = {}
    for bone, mesh in parts.items():
        material = ABDOMEN if bone == 'abdomen' else HIDE
        skin[bone] = [mesh.finish(bone.title().replace('_', ''), material)]
        if bone in hairs:
            skin[bone].append(hairs.pop(bone).finish(bone.title().replace('_', '') + 'Hair', material))
        skin[bone] += dressing.get(bone, [])
        # As the spider's: a hull round everything the bone moves (the mud and the stubs too), so a shot at any drawn
        # part hits it.
        lm.hit_hull(rig, bone, skin[bone])
    gravemother = lm.skin(rig, 'Gravemother', skin)
rear_c = AB.center + AB.fwd * (REAR_AT * AB.radii[0])
rear = lm.hit_sphere(rig, 'abdomen', lc.to_blender(rear_c), REAR_RADIUS * 0.01)
rear.name = 'USP_AbdomenRear'


def spread_patch_uvs(obj):
    """Spider.py maps its eyes, fangs and bristles onto one point of a solid patch of the atlas: faces without UV area,
    which Unreal's tangents warn about. Each such face is laid flat round that point instead (a few texels, still inside
    its solid patch), so its colour is unchanged and its tangents are sound."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    uv = bm.loops.layers.uv.active
    spread = 0
    for f in bm.faces:
        us = [l[uv].uv.copy() for l in f.loops]
        area = 0.5 * abs(sum(a.x * b.y - b.x * a.y for a, b in zip(us, us[1:] + us[:1])))
        if area > 1e-12:
            continue
        centre = sum(us, Vector((0.0, 0.0))) / len(us)
        n = f.normal if f.normal.length > 1e-9 else Vector((0.0, 0.0, 1.0))
        a = n.orthogonal().normalized()
        b = n.cross(a)
        mid = f.calc_center_median()
        for loop in f.loops:
            d = loop.vert.co - mid
            loop[uv].uv = centre + Vector((max(-0.004, min(0.004, d.dot(a) * 0.05)),
                                           max(-0.004, min(0.004, d.dot(b) * 0.05))))
        spread += 1
    bm.to_mesh(obj.data)
    bm.free()
    return spread


lb.log(f'{spread_patch_uvs(gravemother)} faces of the solid patches given UV area (tangents)')
colors = gravemother.data.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
colors.data.foreach_set('color', [1.0] * (4 * len(gravemother.data.loops)))
rig['LODs'] = '50,25'
rig['LODScreens'] = '0.4,0.16'
tris = sum(len(p.vertices) - 2 for p in gravemother.data.polygons)
lb.log(f'Gravemother: {tris} triangles, {len(bones)} bones, {removed} bristles left out under the stone, materials '
       f'{", ".join(m.name for m in gravemother.data.materials)}; soft rear: a sphere of {REAR_RADIUS} cm at '
       f'{tuple(round(c, 1) for c in lb.unreal_point(rear_c))} (component cm)')

# --- Previews ---


def stone_preview(state=0):
    """The headstone in a state, placed on her (as GravemotherStone.py builds it, in her space): a preview helper."""
    bm = lb.new_bm()
    if TRIPO:
        import parts_gravemother_stone as gst
        stone = gb.build({n: (np.array(h[:]), np.array(t[:])) for n, h, t, p in S['bones']})[0]['stone']
        gst.slab(bm, (stone['V'], stone['F']), state, S)
    else:
        lbs.slab(bm, state)
    lbs.roots_into(bm, S, snapped=('FrontRight',) if state == 2 else ())
    lbs.mud_collar(bm, S)
    lbs.finish_uvs(bm)
    bmesh.ops.transform(bm, matrix=lc.TO_BLENDER @ STONE, verts=bm.verts)
    mats = [lt.material(lbs.SET, name='GravemotherStone'), lb.stone_break(), ROOTS, lb.earth()]
    return lb.obj_from_bm(f'_StonePreview{state}', bm, mats, smooth=True, sharp=60.0)


if lt.want_preview():
    out = lb.PREVIEW_DIR
    lb.ensure_set(lbs.SET, lbs.paint_stone)
    lb.ensure_set(lb.EARTH_SET, lb.paint_earth)
    stone = stone_preview(0)
    shown = [gravemother, stone]
    shots = []
    # Her front three-quarter (as the hero sheet), from behind (the soft rear under the stone's back edge), side.
    for key, cam, target in (('three', (-2.2, -3.4, 1.9), (0.0, 0.25, 0.75)), ('back', (1.4, 3.6, 1.7), (0.0, 0.6, 0.75)),
                             ('side', (3.9, -0.6, 1.3), (0.0, 0.3, 0.7))):
        shots.append(lb.render(shown, os.path.join(out, f'Gravemother_{key}.png'), cam, target, lens=40.0,
                               size=(1280, 820), sun_dir=(-0.45, -0.75, 0.75)))
    # Bare after 33%: the stone off; the abdomen in its own hide, then in GravemotherBare (the swap).
    shots.append(lb.render([gravemother], os.path.join(out, 'Gravemother_bare_print.png'), (-2.4, 2.6, 2.3),
                           (0.0, 0.55, 0.7), lens=40.0, size=(1280, 820), sun_dir=(-0.45, -0.75, 0.75)))
    slot = list(gravemother.data.materials).index(ABDOMEN)
    gravemother.data.materials[slot] = lt.material(BARE_SET, name='_GravemotherBarePreview')
    shots.append(lb.render([gravemother], os.path.join(out, 'Gravemother_bare_swap.png'), (-2.4, 2.6, 2.3),
                           (0.0, 0.55, 0.7), lens=40.0, size=(1280, 820), sun_dir=(-0.45, -0.75, 0.75)))
    gravemother.data.materials[slot] = ABDOMEN
    # The stone's foot close, from her left and behind: the roots diving into her, the earth packed round it.
    shots.append(lb.render([gravemother, stone], os.path.join(out, 'Gravemother_stone_foot.png'), (-1.3, 2.2, 1.9),
                           (0.0, 0.75, 0.9), lens=45.0, size=(1280, 820), sun_dir=(-0.45, -0.75, 0.75)))
    # Her legs close: the caked mud.
    shots.append(lb.render([gravemother, stone], os.path.join(out, 'Gravemother_legs.png'), (-1.3, -1.9, 0.55),
                           (-0.3, -0.55, 0.35), lens=50.0, size=(1280, 820), sun_dir=(-0.45, -0.75, 0.75)))
    lb.sheet(shots, os.path.join(out, 'Gravemother_sheet.png'), cols=2)
