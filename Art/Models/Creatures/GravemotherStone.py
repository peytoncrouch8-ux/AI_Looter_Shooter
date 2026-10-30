"""The Gravemother's headstone (Saved/Gravemother/CONTRACT.md, BARROW-ART, 2026-10-19): the breakable armour on her
abdomen, as static meshes the game attaches to SK_Gravemother's abdomen bone (Tools/Blender/looter_barrow_stone.py
builds them; its docstring has the slab, the cracks and the texture).

  SM_GravemotherStone_0          intact, its roots binding it, grave earth packed round its foot
  SM_GravemotherStone_1          cracked (at 66% of its health): the cracks open a few mm, the cracked texture
  SM_GravemotherStone_2          shattered (33%): its pieces shifted and tipped, a corner gone, a front root snapped
  SM_GravemotherStoneShard_1..5  the pieces of state 2 that fall when it cracks off, modelled in place round the stone's
                                 pivot: spawned at the stone's transform they are state 2 (1 upper left, 2 upper right,
                                 3 left wedge, 4 lower left, 5 lower right); no roots, no earth, no collision
  SM_StoneChip_1..3              flakes of its granite knocked off by hits, 8-16 cm in the world, pivot at the middle

The stone's pivot is the middle of its foot, where it meets the abdomen; its front (Unreal +X) is out of its carved face,
which faces her head; +Z up along it. It is modelled at the spider's 1x, like SK_Gravemother: attached to her it takes
her 1.8 from her mesh. Its place on the abdomen bone (bone space, Unreal cm and degrees, size 1) is written to
Intermediate/ArtExport_Gravemother/stone_attach.json (and into an export's own folder, and beside the previews).
States 0-2 have one convex hull (UCX) round the slab: what shots hit (the game sets their channels). No Nanite (it moves
with her): LODs at 50% and 25%. Material slots: GravemotherStone (its painted set), GravemotherStoneBreak (fresh
fracture faces), GraveRoots, GraveEarth (round its foot).

    Tools\\artrun.ps1 -Script Art\\Models\\Creatures\\GravemotherStone.py -Preview -Threads 6
        (Saved/ArtPreviews/Creatures/Gravemother/GravemotherStone_*.png)
"""
import json
import math
import os
import random
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_barrow as lb
import looter_barrow_stone as lbs
import looter_creatures as lc
import looter_model as lm
import looter_textures as lt

# The user's Tripo remake (2026-10-29): the headstone Tripo made on her back, cut off and set in the stone's frame at
# the scripted slab's size, broken by the same cracks (parts_gravemother_stone.py; its texture
# parts_gravemother_stone_look.py: the scripted granite with Tripo's carving baked in). LOOTER_GRAVEMOTHER_TRIPO=0 keeps
# the scripted slab.
TRIPO = os.environ.get('LOOTER_GRAVEMOTHER_TRIPO', '1') != '0'
S = lb.spider_parts()
KEEP = 'keep' in lb.argv()          # -ScriptArgs keep: its texture sets as they are
if TRIPO:
    sys.path.insert(0, os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Tripo'))
    import parts_gravemother_build as gb
    import parts_gravemother_look as gl
    import parts_gravemother_stone as gst
    import parts_gravemother_stone_look as gsl
    BONES = {n: (np.array(h[:]), np.array(t[:])) for n, h, t, p in S['bones']}
    S = gl.tripo_spider(S)                                     # her abdomen: the roots and earth fit it
    TRIPO_STONE = gb.build(BONES)[0]['stone']
    LOW = (TRIPO_STONE['V'], TRIPO_STONE['F'])
    if not (KEEP and all(os.path.exists(lt.texture_path(lbs.SET, k)) for k in ('BC', 'N', 'ORM'))):
        gsl.paint((*LOW, gst.corner_uvs(*LOW, '0')), TRIPO_STONE['tripo'])
else:
    lb.ensure_set(lbs.SET, lbs.paint_stone, always=not KEEP)
lb.ensure_set(lb.ROOT_SET, lb.paint_roots, always=not KEEP)
lb.ensure_set(lb.EARTH_SET, lb.paint_earth)
STONE = lt.material(lbs.SET, name='GravemotherStone')
MATS = [STONE, lb.stone_break(), lb.roots(), lb.earth()]
TO_MODEL = lb.STONE_TO_MODEL @ Matrix.Scale(0.01, 4)            # stone cm to the model's metres


def compact(bm, mats):
    """Only the materials a model's faces use, in their order."""
    used = sorted({f.material_index for f in bm.faces})
    remap = {old: new for new, old in enumerate(used)}
    for f in bm.faces:
        f.material_index = remap[f.material_index]
    return [mats[i] for i in used]


def hull(parent, points_cm):
    bm = bmesh.new()
    for p in points_cm:
        bm.verts.new(TO_MODEL @ Vector(p))
    made = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in made['geom_interior'] + made['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    obj = lb.obj_from_bm('UCX_' + parent.name, bm, [], smooth=False)
    obj.parent = parent
    obj.display_type = 'WIRE'
    return obj


def model(name, bm, mats, lods=True, collision=None):
    bmesh.ops.transform(bm, matrix=TO_MODEL, verts=bm.verts)
    mats = compact(bm, mats)
    obj = lb.obj_from_bm(name, bm, mats, smooth=True, sharp=55.0)
    obj['Nanite'] = 0
    if lods:
        obj['LODs'] = '50,25'
        obj['LODScreens'] = '0.5,0.2'
    if collision:
        obj['Collision'] = collision
    return obj


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


# --- The three states ---

def slab(bm, state, only=None):
    if TRIPO:
        gst.slab(bm, LOW, state, S, only)
    else:
        lbs.slab(bm, state, only)


def hull_points(bm):
    """The slab's simple collision: the scripted outline's, or round the Tripo slab's own points (its stone faces)."""
    if not TRIPO:
        return lbs.hull_points()
    pts = [v.co.copy() for f in bm.faces if f.material_index in (lbs.M_STONE, lbs.M_BREAK) for v in f.verts]
    middle = sum(pts, Vector()) / len(pts)
    return [p + (p - middle).normalized() * 0.6 for p in pts]


states = []
for state in (0, 1, 2):
    bm = lb.new_bm()
    slab(bm, state)
    points = hull_points(bm)
    lbs.roots_into(bm, S, snapped=('FrontRight',) if state == 2 else ())
    lbs.mud_collar(bm, S)
    lbs.finish_uvs(bm)
    obj = model(f'GravemotherStone_{state}', bm, MATS)
    hull(obj, points)
    states.append(obj)

# --- The shards (state 2's pieces) ---

shards, centres = [], []
for i, key in enumerate(lbs.SHARD_ORDER, 1):
    bm = lb.new_bm()
    slab(bm, 2, only=key)
    lbs.finish_uvs(bm)
    xs, ys, zs = zip(*[tuple(v.co) for v in bm.verts])
    centre = Vector(((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, (min(zs) + max(zs)) / 2))
    obj = model(f'GravemotherStoneShard_{i}', bm, MATS, lods=False, collision='None')
    obj['LODs'] = '50'
    obj['LODScreens'] = '0.25'
    shards.append(obj)
    centres.append((obj.name, key, centre))

# --- The chips (world size: 8 to 16 cm) ---


def chip(name, seed, size_m):
    """A flake of the headstone: a ragged outline, thin and uneven, its top the weathered face, the rest fresh break."""
    rng = random.Random(seed)
    n = 7
    outline = []
    for k in range(n):
        a = 2.0 * math.pi * (k + rng.uniform(-0.25, 0.25)) / n
        r = size_m * 0.5 * rng.uniform(0.55, 1.0)
        outline.append(Vector((math.cos(a) * r * rng.uniform(0.8, 1.2), math.sin(a) * r * 0.75, 0.0)))
    thick = size_m * rng.uniform(0.16, 0.26)
    bm = lb.new_bm()
    uv = bm.loops.layers.uv.active
    top = [bm.verts.new(p + Vector((0.0, 0.0, thick * rng.uniform(0.35, 0.6)))) for p in outline]
    low = [bm.verts.new(p * 0.82 + Vector((rng.uniform(-0.1, 0.1) * size_m, 0.0, -thick * rng.uniform(0.35, 0.6))))
           for p in outline]
    face = bm.faces.new(top)
    face.material_index = 0
    u0, u1, v0, v1 = lbs.REGIONS['front0']
    for loop in face.loops:
        co = loop.vert.co
        # A patch of the weathered face, away from its carving.
        loop[uv].uv = (u0 + (0.22 + co.x / 0.8 * 0.5) * (u1 - u0), v0 + (0.3 + co.y / 1.16 * 0.5) * (v1 - v0))
    bottom = bm.faces.new(list(reversed(low)))
    bottom.material_index = 1
    for k in range(n):
        j = (k + 1) % n
        bm.faces.new((low[k], low[j], top[j], top[k])).material_index = 1
    bm.normal_update()
    lb.box_uv_faces(bm, [f for f in bm.faces if f.material_index == 1], 320.0 / 1024.0 * 1.0 / 1.0)
    obj = lb.obj_from_bm(name, bm, [STONE, MATS[1]], smooth=False)
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    return obj


chips = [chip(f'StoneChip_{i}', 70 + i, size) for i, size in ((1, 0.09), (2, 0.12), (3, 0.16))]

# --- Where it sits on her: stone_attach.json ---

probe = lm.armature('_AbdomenProbe')
lm.bones(probe, [(name, lc.to_blender(head), lc.to_blender(tail), parent) for name, head, tail, parent in S['bones']])
abdomen_rest = probe.data.bones['abdomen'].matrix_local.copy()
probe_data = probe.data
bpy.data.objects.remove(probe)
bpy.data.armatures.remove(probe_data)
place = lb.stone_attach(S, abdomen_rest)


def r3(v):
    return [round(float(c), 3) for c in v]


def rot(r):
    return dict(pitch=round(r[0], 3), yaw=round(r[1], 3), roll=round(r[2], 3))


attach = dict(
    about=('The headstone on SK_Gravemother (Art/Models/Creatures/GravemotherStone.py): attach SM_GravemotherStone_<state> '
           'to the mesh\'s "abdomen" bone, KeepRelativeTransform, with this location, rotation and scale (the bone\'s own '
           'space, Unreal cm and degrees, the rig at size 1: her 1.8 comes from her mesh). "component" is the same place in '
           'the mesh\'s component space at rest, and "abdomenRest" the bone\'s rest transform the bone-space numbers were '
           'worked out with (Blender bone axes through the FBX import: Y along the bone, mirrored): a test can check '
           'RefPose(abdomen) * bone == component. The shards are modelled round the stone\'s pivot: spawn each at the '
           'stone\'s world transform; "centre" is each one\'s middle in the stone\'s own space (cm), to spin it about.'),
    bone='abdomen',
    location=r3(place['bone']['location']), rotation=rot(place['bone']['rotation']), scale=[1.0, 1.0, 1.0],
    axes=dict(x=r3(place['bone']['axes'][0]), y=r3(place['bone']['axes'][1]), z=r3(place['bone']['axes'][2])),
    component=dict(location=r3(place['component']['location']), rotation=rot(place['component']['rotation']),
                   axes=dict(x=r3(place['component']['axes'][0]), y=r3(place['component']['axes'][1]),
                             z=r3(place['component']['axes'][2]))),
    abdomenRest=dict(location=r3(place['bone_rest']['location']), rotation=rot(place['bone_rest']['rotation'])),
    stone=dict(width=lb.STONE_W, height=lb.STONE_H, thickness=lb.STONE_T, note='cm at size 1; x1.8 on her'),
    states=['SM_GravemotherStone_0', 'SM_GravemotherStone_1', 'SM_GravemotherStone_2'],
    shards=[dict(name='SM_' + name, piece=key, centre=r3((c.y, c.x, c.z))) for name, key, c in centres],
    chips=['SM_StoneChip_1', 'SM_StoneChip_2', 'SM_StoneChip_3'],
)
for folder in filter(None, (lb.export_dir(), lb.FAMILY_DIR, lb.PREVIEW_DIR)):
    os.makedirs(folder, exist_ok=True)
    with open(os.path.join(folder, 'stone_attach.json'), 'w', encoding='utf-8') as f:
        json.dump(attach, f, indent=2)
lb.log('stone_attach:', json.dumps({k: attach[k] for k in ('location', 'rotation', 'component', 'abdomenRest')}))
for obj in states + shards + chips:
    lb.log(f'{obj.name}: {triangles(obj)} triangles, materials {", ".join(m.name for m in obj.data.materials)}')

# --- Previews ---

if lt.want_preview():
    out = lb.PREVIEW_DIR
    shots = []
    spacing = 1.05
    for i, obj in enumerate(states):
        obj.location = ((i - 1) * spacing, 0.0, 0.0)
    for key, cam in (('front', (0.0, -4.4, 1.0)), ('back', (0.0, 4.4, 1.5))):
        shots.append(lb.render(states, os.path.join(out, f'GravemotherStone_states_{key}.png'), cam, (0.0, 0.0, 0.62),
                               lens=42.0, size=(1500, 800), ground_z=-0.12, sun_dir=(-0.5, -0.8, 0.7)))
    shots.append(lb.render([states[1]], os.path.join(out, 'GravemotherStone_cracked_close.png'), (0.2, -1.5, 1.1),
                           (0.0, 0.0, 0.7), lens=40.0, size=(1500, 800), ground_z=-0.12, sun_dir=(-0.5, -0.8, 0.7)))
    for obj in states:
        obj.location = (0.0, 0.0, 0.0)
        obj.hide_render = True
    # The shards drawn apart, each a little out from its place, and the chips.
    for i, obj in enumerate(shards):
        _, key, c = centres[i]
        d = Vector((-c.x, -c.y, c.z - 60.0)) * 0.01
        obj.location = Vector((d.x, d.y, d.z)).normalized() * 0.22
    for i, obj in enumerate(chips):
        obj.location = (0.62 + i * 0.16, -0.3, 0.05)
    shots.append(lb.render(shards + chips, os.path.join(out, 'GravemotherStone_shards.png'), (0.3, -3.0, 1.0),
                           (0.25, 0.0, 0.6), lens=42.0, size=(1500, 800), ground_z=-0.12, sun_dir=(-0.5, -0.8, 0.7)))
    lb.sheet(shots, os.path.join(out, 'GravemotherStone_sheet.png'), cols=2)
    for obj in shards + chips:
        obj.location = (0.0, 0.0, 0.0)
