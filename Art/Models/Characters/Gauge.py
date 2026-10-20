"""SK_Gauge: Gauge, the Iron Hand, the railroad's clockwork track-laying engine (Docs/Story.md "The heroes"), from the
portrait the user picked on 2026-10-19: option C, Company Iron, of Art/Portraits/Gauge.py (its OPTIONS["C"] and builders
are the spec; the painted bust Saved/ArtPreviews/Portraits/Gauge/GaugeC.png). 1.92 m to the stacks' lips, massive: black
enamel and polished brass the day it left the works (no soot), the brass nameplate GAUGE on a level brow over one warm
white headlamp (#FFE2A8), a grille mouth, a boiler chest with the pressure gauge for a heart and the company's enamelled
crest, the firebox glowing in its belly, two stacks and a steam whistle on the drum across its back, domed pauldrons,
piston-rod limbs, three-fingered riveted hands (thumb + two), broad iron feet with brass toe caps.

No MakeHuman body: hard-surface parts (Tools/Blender/gauge_parts.py on gauge_geo.py), each riding one mannequin bone
rigidly (or a fixed blend of two: the pauldrons half the upper arm's, the side tassets going down onto the thighs), on a
skeleton taken from MakeHuman's joints (looter_human) at human proportions and widened for the engine: the shoulders out
and up (SHOULDER), the arms longer (ARM_SCALE) so the hands reach the mannequin's, the hands bigger (HAND_SCALE) with the
index and middle fingers spread apart (FINGER_SPREAD), the hips wider (HIP_X). The game's mannequin animations drive it
through an IK retargeter. Rig: the UE5 mannequin's 63 deform bones (ring and pinky present, unweighted) and the forearms'
lowerarm_twist_01/02 (the wrist's casing and cuff on twist_02, the band over the casings' seam on twist_01); no swing
chains, no hat. Built facing -Y, turned to face -X last (+Y in Unreal, as the mannequin: face_like_mannequin).

Detail: every part is built twice (gauge_parts.Kit): the game mesh (~31.6k triangles), and a detailed bake source
(~126k faces: round bevels, rivets, the boiler's plate seams, lap-seam straps down the limbs' casings, the GAUGE letters,
the crest's wheel, the waist's bellows) whose looks (Tools/Blender/gauge_looks.py) looter_bake casts onto it: five
texture sets by region (GaugeHead, GaugeChest, GaugeBelly, GaugeArms, GaugeLegs), each one slot on M_Character, and the
lamp and the fire (louvres, grille, the collar's seam) as Glow materials. Hit zones: a convex hull per bone carrying
parts (the pauldrons on the collarbones). Checks: Tools/Blender/gauge_audit.py (not in the bake's digest).

    Tools\\artrun.ps1 -Script Art\\Models\\Characters\\Gauge.py -Threads 6 -ScriptArgs <flags>
      --shape [--hi] [--shots full+upper+face]   clay of the game mesh (or the bake source) into Saved/ArtPreviews/Characters/Gauge
      --stats                                    triangles per part and group, texel density per set (no bake)
      --rig-check                                weights and the head's turns in clay (Tools/Blender/gauge_audit.py; no bake)
      --audit <tag> [--no-bake]                  the game's poses, every part against the others (gauge_audit.py;
                                                 without the bake: the clay and the findings only)
      --preview [--shots ...]                    the baked look at the game's idle: front, three-quarter, side, back, face
      --rebake [--size N]                        bake again even when the digest matches
"""
import math
import os
import sys
import time

import bpy  # first: bmesh and mathutils come with it
import bmesh
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
for p in (os.path.join(REPO, 'Tools', 'Blender'), os.path.join(REPO, 'Art', 'Portraits')):
    if p not in sys.path:
        sys.path.insert(0, p)
import gauge_geo as gg  # noqa: E402
import gauge_looks as gl  # noqa: E402
import gauge_parts as gp  # noqa: E402
import looter_bake as bk  # noqa: E402
import looter_human as hu  # noqa: E402
import looter_model as lm  # noqa: E402
import looter_textures as lt  # noqa: E402
import mh  # noqa: E402

ARGS = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
T0 = time.time()

# The skeleton: MakeHuman's joints for a heavy, broad build (MACRO) at MH_HEIGHT (its head pivot then at ~1.616 m, the
# height gauge_parts.HEAD_Z writes the body for), changed for the engine:
MACRO = {'gender': 1.0, 'muscle': 1.0, 'weight': 0.7}
MH_HEIGHT = 1.765
SHOULDER = (0.31, 1.47)  # the shoulder joints (x, z): out past the boiler (0.51 m wide) and up under the pauldrons
ARM_SCALE = 1.12         # the arms' bones longer by this: from shoulders 12 cm wider than the mannequin's the hands
                         # still reach its hands (the guns hang from them; the retargeter puts these hands there)
HAND_SCALE = 1.28        # the hands bigger about the wrist (the portrait's are half again a man's; guns must fit them)
FINGER_SPREAD = 0.006    # index_* toward the thumb and middle_* toward the little finger by this (m): two broad
                         # fingers (4.1 cm) side by side, a few millimetres apart
HIP_X = 0.13             # the hip joints' x (m): the thighs' casings clear of each other
SIZE = 2048

REGIONS = ('Head', 'Chest', 'Belly', 'Arms', 'Legs')
KINDS = ('Enamel', 'Black', 'Red', 'Brass', 'Iron', 'Dark', 'Dial')
# The texture sets the looks bake into (Art/Textures/<Set>/): one per region, each its own slot on M_Character (a
# little less of the master's roughness breakup: enamel and polished brass are glossy); the glows as Glow materials.
# The body's parts (gauge_parts' Body looks) split by group: the chest (spine_05's: boiler, collar, gauge, crest, drum,
# stacks, whistle) and the belly (the firebox's course, the waist, the girdle): as one set they had 880 px/m.
CHEST_GROUPS = ('Torso',)
TEXTURE_SETS = {f'Gauge{r}': tuple(r + k for k in KINDS) for r in REGIONS}
GAME_MATERIALS = {f'Gauge{r}': ('Character', TEXTURE_SETS[f'Gauge{r}'], {'RoughnessBreakup': 0.06}) for r in REGIONS}
GLOWS = {'Lamp': (0xFFE2A8, 5.0), 'Fire': (0xFF8C2E, 3.0)}
PACK_WEIGHT = {'GaugeDial': 2.2, 'Crest': 1.8, 'CrestBack': 1.4, 'FireDoor': 1.4}   # more texels for the close-up faces
TWIST = (('lowerarm_twist_01', 0.5), ('lowerarm_twist_02', 0.9))


def log(message):
    print(f'GAUGE [{time.time() - T0:6.1f}s] {message}', flush=True)


def arg_value(name, default=None):
    return ARGS[ARGS.index(name) + 1] if name in ARGS and ARGS.index(name) + 1 < len(ARGS) else default


# --------------------------------------------------------------------------------------------------------- skeleton

def skeleton(body):
    """name -> (head, tail, parent): MakeHuman's mannequin bones (looter_human) with Gauge's changes (the module's
    docstring): each arm from its new shoulder along MakeHuman's own directions, ARM_SCALE longer; the hand and fingers
    carried to the new wrist and scaled about it, the two fingers spread; the legs moved out to HIP_X."""
    sk = {k: [np.array(h, dtype=np.float64), np.array(t, dtype=np.float64), p] for k, (h, t, p) in body.skeleton().items()}
    for s, sx in (('l', 1.0), ('r', -1.0)):
        S0, E0, W0 = sk[f'upperarm_{s}'][0].copy(), sk[f'upperarm_{s}'][1].copy(), sk[f'lowerarm_{s}'][1].copy()
        Sn = np.array([sx * SHOULDER[0], S0[1], SHOULDER[1]])
        En = Sn + (E0 - S0) * ARM_SCALE
        Wn = En + (W0 - E0) * ARM_SCALE
        sk[f'clavicle_{s}'][1] = Sn
        sk[f'upperarm_{s}'][0], sk[f'upperarm_{s}'][1] = Sn, En
        sk[f'lowerarm_{s}'][0], sk[f'lowerarm_{s}'][1] = En, Wn
        for n in [n for n in sk if n.endswith(f'_{s}') and n.startswith(('hand_', 'thumb_', 'index_', 'middle_', 'ring_',
                                                                        'pinky_'))]:
            sk[n][0] = Wn + (sk[n][0] - W0) * HAND_SCALE
            sk[n][1] = Wn + (sk[n][1] - W0) * HAND_SCALE
        lat = gg.unit(sk[f'middle_01_{s}'][0] - sk[f'index_01_{s}'][0])
        for chain, k in (('index', -1.0), ('middle', 1.0), ('ring', 1.6), ('pinky', 2.0)):
            for n in [f'{chain}_metacarpal_{s}'] + [f'{chain}_0{i}_{s}' for i in (1, 2, 3)]:
                sk[n][0] = sk[n][0] + lat * k * FINGER_SPREAD
                sk[n][1] = sk[n][1] + lat * k * FINGER_SPREAD
        dx = np.array([sx * HIP_X - sk[f'thigh_{s}'][0][0], 0.0, 0.0])
        for n in (f'thigh_{s}', f'calf_{s}', f'foot_{s}', f'ball_{s}'):
            sk[n][0], sk[n][1] = sk[n][0] + dx, sk[n][1] + dx
    return {k: (v[0], v[1], v[2]) for k, v in sk.items()}


class Figure:
    def __init__(self):
        self.b = hu.Body(MACRO, {}, MH_HEIGHT, 0.0)
        self.sk = skeleton(self.b)
        self.names = list(self.sk)
        sk = self.sk
        log(f'skeleton: {len(self.names)} bones; head pivot {np.round(sk["head"][0], 3)}, shoulders '
            f'{np.round(sk["upperarm_l"][0], 3)}, wrists {np.round(sk["hand_l"][0], 3)}, hips {np.round(sk["thigh_l"][0], 3)}, '
            f'arm {np.linalg.norm(sk["upperarm_l"][1] - sk["upperarm_l"][0]) + np.linalg.norm(sk["lowerarm_l"][1] - sk["lowerarm_l"][0]):.3f} m')


def armature(fig, name):
    """The skeleton as a Blender armature at the origin (looter_human.Body.armature's rolls)."""
    arm = bpy.data.objects.new(name, bpy.data.armatures.new(name))
    bpy.context.scene.collection.objects.link(arm)
    bk.select([arm], arm)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones
    for bone, (h, t, parent) in fig.sk.items():
        e = edit.new(bone)
        e.head, e.tail = Vector(h), Vector(t)
        d = (t - h) / max(np.linalg.norm(t - h), 1e-9)
        e.align_roll(Vector((0.0, 0.0, 1.0)) if abs(d[1]) > 0.7 else Vector((0.0, -1.0, 0.0)))
        e.use_connect, e.use_deform = False, True
    for bone, (h, t, parent) in fig.sk.items():
        if parent:
            edit[bone].parent = edit[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    return arm


def add_twist_bones(arm):
    """lowerarm_twist_01/02_<s> under lowerarm_<s> at TWIST's shares down the forearm (Ellis.py's)."""
    bk.select([arm], arm)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones
    names = []
    for s in 'lr':
        lo = edit[f'lowerarm_{s}']
        h, t = lo.head.copy(), lo.tail.copy()
        for name, f in TWIST:
            e = edit.new(f'{name}_{s}')
            e.head, e.tail = h + (t - h) * f, h + (t - h) * min(f + 0.1, 1.0)
            e.roll = lo.roll
            e.use_connect, e.use_deform = False, True
            e.parent = lo
            names.append(e.name)
    bpy.ops.object.mode_set(mode='OBJECT')
    return names


# ------------------------------------------------------------------------------------------------------------ parts

SLOTS = {}
NOTES = {}
PART_INFO = {}            # part -> (bones, group), from the game mesh's build


def slot(key):
    """The look (gauge_looks) for a key, one material per look."""
    if key not in SLOTS:
        SLOTS[key] = gl.make(key, f'Gauge_{key}', NOTES)
        SLOTS[key]['LookKey'] = key
    return SLOTS[key]


def look_of(ob):
    return ob.material_slots[0].material['LookKey'] if ob.material_slots and ob.material_slots[0].material else None


def make_parts(fig, hi):
    """Every part as an object: the game mesh (named <part>_low until the detail goes) or the bake source."""
    K = gp.build(fig.sk, hi)
    NOTES.update(K.notes)
    obs = []
    for p in K.parts:
        name = p['name'] + ('' if hi else '_low')
        look = p['look']
        if look.startswith('Body'):
            look = ('Chest' if p['group'] in CHEST_GROUPS else 'Belly') + look[4:]
        obs.append(gg.make_object(name, p['shape'], slot(look), sharp=p['sharp'] or (40.0 if hi else 50.0),
                                  weighted=not hi))
        if not hi:
            PART_INFO[p['name']] = (p['bones'], p['group'])
    log(f'{"detail" if hi else "game mesh"}: {len(obs)} parts, {sum(len(o.data.polygons) for o in obs)} faces; the '
        f'two fingers\' bones {", ".join(f"{g * 100:.1f}" for g in K.notes.get("finger_gap", []))} cm apart')
    return obs


def prepare_uvs(objects):
    for ob in objects:
        bk.smart_uv(ob, angle=55.0)
        me = ob.data
        me.uv_layers.active = me.uv_layers['UVMap']
        me.uv_layers['UVMap'].active_render = True


def triangles(ob):
    return sum(len(p.vertices) - 2 for p in ob.data.polygons)


def stats(parts):
    """Triangles per part (sides together) and per group; each set's area and texel density at SIZE."""
    by_part, by_group = {}, {}
    for ob in parts:
        base = ob.name[:-4] if ob.name.endswith('_low') else ob.name
        key = base[:-2] if base.endswith(('_l', '_r')) else base
        n = triangles(ob)
        by_part[key] = by_part.get(key, 0) + n
        g = PART_INFO[base][1]
        g = g[:-2] if g.endswith(('_l', '_r')) else g
        by_group[g] = by_group.get(g, 0) + n
    for k, n in sorted(by_part.items(), key=lambda kv: -kv[1]):
        log(f'  {k:22s} {n:6d}')
    for k, n in sorted(by_group.items(), key=lambda kv: -kv[1]):
        log(f'group {k:16s} {n:6d}')
    log(f'total {sum(by_part.values())} triangles')
    for set_name, keys in TEXTURE_SETS.items():
        area = 0.0
        for ob in parts:
            if look_of(ob) in keys:
                area += sum(p.area for p in ob.data.polygons) * PACK_WEIGHT.get(ob.name[:-4], 1.0) ** 2
        log(f'set {set_name}: {area:.2f} m2, about {SIZE * math.sqrt(0.7 / max(area, 1e-6)):.0f} px/m at {SIZE}')


# ------------------------------------------------------------------------------------------------------------- bake

BAKE_SOURCES = [os.path.abspath(__file__)] + [os.path.join(REPO, 'Tools', 'Blender', f) for f in (
    'gauge_geo.py', 'gauge_parts.py', 'gauge_looks.py', 'looter_looks.py', 'looter_bake.py', 'looter_human.py')] + \
    [os.path.join(REPO, 'Art', 'Portraits', 'mh.py')]


def bake_wanted(size):
    stamp = os.path.join(lt.TEXTURE_DIR, 'GaugeHead', 'bake.digest')
    want = bk.digest(BAKE_SOURCES, f'size={size} blender={bpy.app.version_string}')
    have = open(stamp).read().strip() if os.path.exists(stamp) else ''
    missing = any(not os.path.exists(lt.texture_path(s, k)) for s in TEXTURE_SETS for k in ('BC', 'N', 'ORM'))
    return stamp, want, have != want or missing or '--rebake' in ARGS


def bake_all(lows, highs, size):
    """Packs each set's UVs on the game mesh and bakes it from the detail (cast from 1 cm out, 3 cm reach: the detail
    lies within millimetres of the game mesh), or reads the sets back when the digest matches (Ellis.py's bake_all)."""
    stamp, want, rebake = bake_wanted(size)
    out = {}
    for set_name, keys in TEXTURE_SETS.items():
        obs = [o for o in lows if look_of(o) in keys]
        src = [o for o in highs if look_of(o) in keys]
        bk.pack(obs, weights={o: PACK_WEIGHT[o.name[:-4]] for o in obs if o.name[:-4] in PACK_WEIGHT})
        if not rebake:
            out[set_name] = bk.load_set(set_name)
        if out.get(set_name) is None:
            out[set_name] = bk.bake_set(set_name, obs, size, sources=src, hide=highs, cage=0.01, reach=0.03)
            log(f'baked {set_name} ({len(obs)} parts from {len(src)})')
        else:
            log(f'{set_name}: kept (baked from these scripts before)')
    os.makedirs(os.path.dirname(stamp), exist_ok=True)
    with open(stamp, 'w') as f:
        f.write(want + '\n')
    return out


# -------------------------------------------------------------------------------------------------------------- rig

def zone_of(bone):
    """The hit zone (physics body) a bone's parts belong to."""
    if bone == 'neck_02':
        return 'head'
    side = bone[-1]
    if bone.startswith('lowerarm_twist'):
        return f'lowerarm_{side}'
    if bone.startswith(('thumb_', 'index_', 'middle_')):
        return f'hand_{side}'
    if bone.startswith('ball_'):
        return f'foot_{side}'
    return bone


def main_bone(bones):
    return max(bones, key=lambda b: bones[b] if not callable(bones[b]) else 0.5)


def weights(fig, ob, names):
    bones, _group = PART_INFO[ob.name]
    P = np.array([v.co[:] for v in ob.data.vertices])
    return hu.blend(names, len(P), {b: (f(P) if callable(f) else f) for b, f in bones.items()})


def rig(fig, parts):
    """Weights every part, joins them into one mesh 'Gauge' (SK_Gauge) under the armature (with the forearms' twist
    bones), and makes the hit zones."""
    arm = armature(fig, 'Gauge_Rig')
    names = fig.names + add_twist_bones(arm)
    fig.rig_names = names
    fig.rest = {}
    zones = {}
    for ob in parts:
        W = weights(fig, ob, names)
        co = np.array([v.co[:] for v in ob.data.vertices])
        fig.rest[ob.name] = (co, [list(p.vertices) for p in ob.data.polygons], W, PART_INFO[ob.name][1])
        zones.setdefault(zone_of(main_bone(PART_INFO[ob.name][0])), []).append(co)
        for j, bone in enumerate(names):
            col = W[:, j]
            nz = np.nonzero(col > 1e-4)[0]
            if not len(nz):
                continue
            g = ob.vertex_groups.get(bone) or ob.vertex_groups.new(name=bone)
            for i in nz:
                g.add([int(i)], float(col[i]), 'REPLACE')
    skin = next(o for o in parts if o.name == 'Cranium')
    bk.select(parts, skin)
    bpy.ops.object.join()
    skin = bpy.context.view_layer.objects.active
    skin.name = skin.data.name = 'Gauge'
    skin.parent = arm
    mod = skin.modifiers.new('Armature', 'ARMATURE')
    mod.object = arm
    arm['LODs'] = '50,25'
    arm['LODScreens'] = '0.5,0.2'
    for bone, pts in sorted(zones.items()):
        hit_hull(arm, bone, np.concatenate(pts))
    log(f'rigged: {len(arm.data.bones)} bones, {triangles(skin)} triangles, {len(skin.vertex_groups)} weighted bones, '
        f'{len(zones)} hit zones, slots {", ".join(s.material.name for s in skin.material_slots)}')
    return arm, skin


def hit_hull(arm, bone, P, cap=48):
    """A convex hit zone round points, on bone (UCX_: a physics-asset body's hull): every part riding the bone inside
    it. The hull's corners thinned to cap by farthest-point sampling (a hull of thousands of corners cooks slowly)."""
    def hull_of(pts):
        bm = bmesh.new()
        for p in pts:
            bm.verts.new(tuple(p))
        res = bmesh.ops.convex_hull(bm, input=bm.verts)
        drop = [v for v in res['geom_interior'] + res['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
        bmesh.ops.delete(bm, geom=list(set(drop)), context='VERTS')
        return bm
    bm = hull_of(P[::max(1, len(P) // 6000)])
    V = np.array([v.co[:] for v in bm.verts])
    if len(V) > cap:
        bm.free()
        keep = [int(np.argmax(np.linalg.norm(V - V.mean(0), axis=1)))]
        d = np.linalg.norm(V - V[keep[0]], axis=1)
        while len(keep) < cap:
            keep.append(int(np.argmax(d)))
            d = np.minimum(d, np.linalg.norm(V - V[keep[-1]], axis=1))
        bm = hull_of(V[keep])
    me = bpy.data.meshes.new(f'UCX_{bone}')
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(f'UCX_{bone}', me)
    bpy.context.scene.collection.objects.link(ob)
    ob.parent = arm
    ob.display_type = 'WIRE'
    ob.hide_render = True
    ob['Bone'] = bone
    return ob


def game_materials(skin):
    """The looks swapped for the game's: one textured material per region set (MI_<Slot> on M_Character), Glow for the
    lamp and the fire."""
    swap = {}
    for slot_name, (master, keys, params) in GAME_MATERIALS.items():
        sets = {s for s, ks in TEXTURE_SETS.items() if set(keys) & set(ks)}
        assert len(sets) == 1, f'{slot_name}: its looks must bake into one set, not {sets}'
        mat = lt.material(sets.pop(), name=slot_name, master=master, **params)
        for k in keys:
            swap[f'Gauge_{k}'] = mat
    for k, (hex_value, strength) in GLOWS.items():
        swap[f'Gauge_{k}'] = lm.material(f'Gauge{k}', hex_value, 'Glow', Glow=strength)
    for i, s_ in enumerate(skin.material_slots):
        if s_.material is not None and s_.material.name in swap:
            skin.material_slots[i].material = swap[s_.material.name]
    # slots that now share a material merged into one: each face onto the first slot with its material
    first = {}
    for i, s_ in enumerate(skin.material_slots):
        first.setdefault(s_.material.name, i)
    remap = np.array([first[s_.material.name] for s_ in skin.material_slots])
    idx = np.empty(len(skin.data.polygons), dtype=np.int32)
    skin.data.polygons.foreach_get('material_index', idx)
    skin.data.polygons.foreach_set('material_index', remap[idx].astype(np.int32))
    skin.data.update()
    bk.select([skin])
    bpy.ops.object.material_slot_remove_unused()
    log(f'game slots: {", ".join(s.material.name for s in skin.material_slots)}')


def viewer_materials(skin, baked):
    """The baked maps on Principled BSDFs for the previews (Ellis.py's), the glows burning."""
    mats = {}
    for set_name, keys in TEXTURE_SETS.items():
        imgs = baked[set_name]['images']
        mat = bpy.data.materials.new(f'View_{set_name}')
        mat.use_nodes = True
        nt = mat.node_tree
        nt.nodes.clear()
        out = nt.nodes.new('ShaderNodeOutputMaterial')
        b = nt.nodes.new('ShaderNodeBsdfPrincipled')
        nt.links.new(b.outputs[0], out.inputs['Surface'])
        col = nt.nodes.new('ShaderNodeTexImage')
        col.image = imgs['COLOR']
        orm = nt.nodes.new('ShaderNodeTexImage')
        orm.image = imgs['ORM']
        sep = nt.nodes.new('ShaderNodeSeparateColor')
        nt.links.new(orm.outputs['Color'], sep.inputs[0])
        ao = nt.nodes.new('ShaderNodeMix')
        ao.data_type, ao.blend_type = 'RGBA', 'MULTIPLY'
        ao.inputs[0].default_value = 0.7                     # the game's DiffuseAO
        nt.links.new(col.outputs['Color'], ao.inputs[6])
        nt.links.new(sep.outputs[0], ao.inputs[7])
        nt.links.new(ao.outputs[2], b.inputs['Base Color'])
        nt.links.new(sep.outputs[1], b.inputs['Roughness'])
        nt.links.new(sep.outputs[2], b.inputs['Metallic'])
        nrm = nt.nodes.new('ShaderNodeTexImage')
        nrm.image = imgs['NORMAL']
        nm = nt.nodes.new('ShaderNodeNormalMap')
        nt.links.new(nrm.outputs['Color'], nm.inputs['Color'])
        nt.links.new(nm.outputs['Normal'], b.inputs['Normal'])
        for k in keys:
            mats[f'Gauge_{k}'] = mat
    for k in GLOWS:
        mats[f'Gauge_{k}'] = bpy.data.materials.get(f'Gauge_{k}')
    for i, s_ in enumerate(skin.material_slots):
        if s_.material is not None and s_.material.name in mats:
            skin.material_slots[i].material = mats[s_.material.name]


def face_like_mannequin(arm, skin):
    """The finished figure turned a quarter about Z into its data (bones, vertices, hit zones), the last step before
    the export, so SK_Gauge faces +Y in Unreal with its left at +X, as SKM_Manny_Simple (Ellis.py's)."""
    turn = Matrix.Rotation(math.radians(-90.0), 4, 'Z')
    bk.select([arm], arm)
    arm.data.transform(turn)
    skin.data.transform(turn)
    for ob in arm.children:
        if ob is not skin:
            ob.matrix_world = turn @ ob.matrix_world
    bpy.context.view_layer.update()
    toe = arm.data.bones['ball_l'].head_local - arm.data.bones['foot_l'].head_local
    log(f'turned to face -X here (+Y in Unreal): foot_l to ball_l {tuple(round(c, 3) for c in toe.normalized())}')


# ---------------------------------------------------------------------------------------------------------- renders

def shots(fig):
    hz = fig.sk['head'][0][2]
    return {'full': (0.98, 5.0, 0.25), 'upper': (1.42, 2.4, 0.1), 'face': (hz + 0.19, 1.1, 0.0),
            'feet': (0.22, 1.7, 0.35), 'hands': (0.95, 2.2, 0.1)}


def preview(fig, path, views=(('front', 0.0), ('three', 35.0), ('side', 90.0), ('back', 180.0)), size=900, samples=32,
            aim=None):
    """Cycles renders round the figure under a sun and a sky (Ellis.py's); in clay with --shape."""
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = samples
    sc.cycles.use_denoising = True
    sc.render.resolution_x = int(size * 0.75)
    sc.render.resolution_y = size
    sc.render.film_transparent = False
    try:
        sc.view_settings.view_transform = 'AgX'
    except Exception:
        pass
    world = bpy.data.worlds.get('Preview') or bpy.data.worlds.new('Preview')
    sc.world = world
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.32, 0.36, 0.42, 1.0)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.6
    if 'Sun' not in bpy.data.objects:
        ld = bpy.data.lights.new('Sun', 'SUN')
        ld.energy = 3.5
        ld.angle = math.radians(8)
        lo = bpy.data.objects.new('Sun', ld)
        sc.collection.objects.link(lo)
        lo.rotation_euler = (math.radians(50), 0.0, math.radians(-35))
    if '--shape' in ARGS:
        clay = bpy.data.materials.get('_Clay') or bpy.data.materials.new('_Clay')
        clay.diffuse_color = (0.55, 0.52, 0.48, 1.0)
        bpy.context.view_layer.material_override = clay
    cam = bpy.data.objects.get('PreviewCam')
    if cam is None:
        cd = bpy.data.cameras.new('PreviewCam')
        cd.lens = 70
        cam = bpy.data.objects.new('PreviewCam', cd)
        sc.collection.objects.link(cam)
    sc.camera = cam
    out = []
    for name, yaw in views:
        a = math.radians(yaw)
        target = Vector((0.0, 0.0, aim[0]))
        cam.location = target + Vector((math.sin(a) * aim[1], -math.cos(a) * aim[1], aim[2]))
        cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = f'{path}_{name}.png'
        bpy.ops.render.render(write_still=True)
        out.append(sc.render.filepath)
    return out


def render_set(fig, out, prefix):
    sh = shots(fig)
    for shot in arg_value('--shots', 'full+face').split('+'):
        views = (('front', 0.0), ('three', 35.0)) if shot == 'face' else \
            (('front', 0.0), ('three', 35.0), ('side', 90.0), ('back', 180.0))
        preview(fig, os.path.join(out, f'{prefix}_{shot}'), views, aim=sh[shot])


def fetch_makehuman():
    if not os.path.exists(os.path.join(mh.DATA, '3dobjs', 'base.obj')):
        import subprocess
        log('fetching MakeHuman\'s CC0 assets (Art/ThirdParty/MakeHuman/fetch.py)')
        subprocess.run([sys.executable, os.path.join(REPO, 'Art', 'ThirdParty', 'MakeHuman', 'fetch.py')], check=True)


def main():
    fetch_makehuman()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    fig = Figure()
    out = os.path.join(REPO, 'Saved', 'ArtPreviews', 'Characters', 'Gauge')
    os.makedirs(out, exist_ok=True)
    lows = make_parts(fig, False)
    if '--shape' in ARGS:
        if '--hi' in ARGS:
            for ob in lows:
                ob.hide_render = True
            make_parts(fig, True)
        render_set(fig, out, 'Gauge_clay_hi' if '--hi' in ARGS else 'Gauge_clay')
        log(f'clay renders in {out}')
        return
    prepare_uvs(lows)
    if '--stats' in ARGS:
        stats(lows)
        return
    size = int(arg_value('--size', SIZE))
    highs = []
    baked = None
    if '--rig-check' not in ARGS and '--no-bake' not in ARGS:
        if bake_wanted(size)[2]:
            highs = make_parts(fig, True)
        baked = bake_all(lows, highs, size)
    for ob in highs:
        me = ob.data
        bpy.data.objects.remove(ob)
        bpy.data.meshes.remove(me)
    for ob in lows:
        ob.name = ob.name[:-4]
    arm, skin = rig(fig, lows)
    if '--rig-check' in ARGS:
        import gauge_audit
        gauge_audit.rig_check(fig, arm, skin, log, REPO)
        return
    if '--audit' in ARGS:
        import gauge_audit
        gauge_audit.run(fig, arm, skin, baked, arg_value('--audit', 'after'), viewer_materials, log, REPO)
        return
    if '--preview' in ARGS:
        import looter_audit as la
        looks = [s_.material for s_ in skin.material_slots]
        viewer_materials(skin, baked)
        la.pose(arm, la.POSES['idle'])
        for ob in arm.children:
            if ob.name.startswith('UCX_'):
                ob.hide_render = True
        render_set(fig, out, 'Gauge')
        la.reset(arm)
        for s_, m in zip(skin.material_slots, looks):        # the looks back, for game_materials
            s_.material = m
        log('preview written')
    game_materials(skin)
    face_like_mannequin(arm, skin)
    log('ready to export')


if __name__ == '__main__':
    main()
