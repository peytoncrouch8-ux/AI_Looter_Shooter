"""The Stone Sentinel from the user's Tripo model: its look (parts_sentinel_cut.py makes its shape; StoneSentinel.py runs
both). Not a model: Tools\\models.ps1 skips parts_*.py.

The user's bar (Docs/Pipeline/Assets.md, Tripo parts and melee tools): crisp, painted, our style, nothing that reads as
AI-made. Tripo's own paint is soft and smeared (bright flaky blotches, lime moss, a grey cast), so its colour is not
used as it is. What it is good for is its carving: the face, the cord's twist, the hem's key band, the folds and the
cracks live in its maps, not in its mesh. So (all on the remeshed statue's own atlas, 2048 px, UVMap):
1. unwrap(): smart projection, islands at even density, the caps' islands (joint faces, mostly hidden) at under half.
2. bake(): cast from the remeshed pieces onto Tripo's, posed alike (Cycles, selected to active): Tripo's colour and its
   normal map (with the pieces' own shape difference) land on the atlas; masks of the caps and the iron from the
   statue's own slots; each texel's place and normal.
3. compose(): our granite (RockGranite, box-mapped at UV_DENSITY as the plinth) is the stone; Tripo's carving comes
   through as light and shade only (its colour over its own local mean: cracks, crevices and the carved lines dark,
   its blotches clipped); moss is MossColor (M_World's) in crisp patches where Tripo grew it, and in upward folds;
   caps are plain fresh-cut granite; iron staples (staples()) bridge cracks, rust streaking below each.
4. write(): T_TripoStoneSentinel_BC / _ORM / _N, material StoneSentinelBody on the World master; uv.digest beside them
   (looter_bake's guard), so an export whose atlas differs bakes again.
"""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_bake as lbk
import looter_rebake as rb
import looter_textures as lt

SET = 'TripoStoneSentinel'
MATERIAL = 'StoneSentinelBody'
SIZE = 2048
GRANITE = 'RockGranite'
UV_DENSITY = 420.0                       # the stand-in's and the plinth's granite, px a metre
CAP_UV = 0.45                            # the caps' share of density
MOSS = np.array([0.13, 0.21, 0.05], np.float32) * 0.8   # M_World's MossColor (linear, the plinth's moss), in shade
IRON = np.array([0.050, 0.046, 0.042], np.float32)
RUST = np.array([0.20, 0.085, 0.035], np.float32)

# Iron staples across cracks (a mender's work on an old grave statue): (bone, a point near the surface, the side it is
# seen from, the bar's rough way, length m). Placed in Tripo's stance, on the remeshed pieces, before the pose.
STAPLES = [
    ('head', (0.12, 0.16, 2.12), (0.45, 0.75, 0.5), (0.0, 0.3, -1.0), 0.13),
    ('spine_02', (-0.18, 0.40, 1.80), (-0.2, 1.0, 0.15), (1.0, 0.0, 0.3), 0.14),
    ('upperarm_l', (0.62, 0.10, 1.72), (1.0, 0.1, 0.2), (0.0, 1.0, 0.25), 0.13),
    ('lowerarm_r', (-0.74, 0.02, 1.22), (-1.0, 0.0, 0.0), (0.0, 1.0, -0.4), 0.12),
    ('calf_l', (0.24, -0.48, 0.52), (0.35, -1.0, 0.0), (1.0, 0.0, 0.15), 0.15),
    ('thigh_r', (-0.20, 0.44, 0.95), (-0.3, 1.0, 0.0), (1.0, 0.0, -0.2), 0.14),
    ('calf_r', (-0.42, 0.30, 0.40), (-0.8, 0.6, 0.0), (0.0, 0.2, 1.0), 0.13),
]


def log(message):
    print(f'SENTINEL: {message}', flush=True)


def _material(name, color):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.diffuse_color = color
    return mat


# --- Staples ---

def staples(low):
    """Adds the iron staples to the remeshed pieces (Tripo's stance): each a flat bar 2.4 cm wide, 1 cm proud, laid on
    the stone along the crack's way and following its curve, its ends sunk into drilled holes. Slot _Iron."""
    iron = _material('_Iron', (0.1, 0.1, 0.1, 1.0))
    for bone, near, side, way, length in STAPLES:
        obj = low[bone]
        me = obj.data
        if '_Iron' not in [m.name for m in me.materials]:
            me.materials.append(iron)
        slot = [m.name for m in me.materials].index('_Iron')
        tree = BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])
        side = Vector(side).normalized()
        hit, normal, _, _ = tree.ray_cast(Vector(near) + side * 0.5, -side, 1.0)
        if hit is None:
            raise RuntimeError(f'staple on {bone}: no surface under {near}')
        t = Vector(way) - normal * Vector(way).dot(normal)
        t.normalize()
        bm = bmesh.new()
        bm.from_mesh(me)
        rows = []
        steps = 8
        for k in range(steps + 1):
            u = (k / steps - 0.5) * length
            p = hit + t * u
            h, n, _, _ = tree.ray_cast(p + normal * 0.05, -normal, 0.15)
            if h is None:
                h, n = p, normal
            w = n.cross(t).normalized()
            sink = 0.025 if k in (0, steps) else 0.0          # the ends turned down into the stone
            base = h - n * (0.004 + sink)
            top = h + n * 0.010 - n * sink * 0.6
            rows.append([bm.verts.new(base - w * 0.012), bm.verts.new(base + w * 0.012),
                         bm.verts.new(top + w * 0.012), bm.verts.new(top - w * 0.012)])
        for a, b in zip(rows, rows[1:]):
            for i in range(4):
                j = (i + 1) % 4
                f = bm.faces.new((a[i], a[j], b[j], b[i]))
                f.material_index = slot
                f.smooth = False
        for ring, flip in ((rows[0], True), (rows[-1], False)):
            f = bm.faces.new(ring[::-1] if flip else ring)
            f.material_index = slot
            f.smooth = False
        bm.to_mesh(me)
        bm.free()
    log(f'{len(STAPLES)} iron staples laid')


# --- 1. UVs ---

def _uv_islands(bm, uv):
    """Faces grouped into UV islands (sharing an edge whose two corners match in UV)."""
    seen, out = set(), []
    for f in bm.faces:
        if f.index in seen:
            continue
        stack, group = [f], []
        seen.add(f.index)
        while stack:
            g = stack.pop()
            group.append(g)
            for loop in g.loops:
                other = loop.link_loop_radial_next
                if other == loop or other.face.index in seen:
                    continue
                # Same edge, the corners at its two ends equal on both sides.
                a0, a1 = loop[uv].uv, loop.link_loop_next[uv].uv
                b0, b1 = other.link_loop_next[uv].uv, other[uv].uv
                if (a0 - b0).length < 1e-6 and (a1 - b1).length < 1e-6:
                    seen.add(other.face.index)
                    stack.append(other.face)
        out.append(group)
    return out


def unwrap(body, margin=0.003):
    """UVMap on body: smart projection, even density, cap islands at CAP_UV, packed. Returns the atlas's digest."""
    lbk.smart_uv(body, angle=62.0, margin=0.0)
    lbk.select([body])
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.select_all(action='SELECT')
    bpy.ops.uv.average_islands_scale()
    bpy.ops.object.mode_set(mode='OBJECT')
    cap = body.data.materials.find('_Cap')
    bm = bmesh.new()
    bm.from_mesh(body.data)
    bm.faces.ensure_lookup_table()
    uv = bm.loops.layers.uv['UVMap']
    shrunk = 0
    for group in _uv_islands(bm, uv):
        if all(f.material_index == cap for f in group):
            pts = [l[uv].uv.copy() for f in group for l in f.loops]
            c = sum(pts, Vector((0.0, 0.0))) / len(pts)
            for f in group:
                for l in f.loops:
                    l[uv].uv = c + (l[uv].uv - c) * CAP_UV
            shrunk += 1
    bm.to_mesh(body.data)
    bm.free()
    lbk.select([body])
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.select_all(action='SELECT')
    bpy.ops.uv.pack_islands(margin=margin, rotate=True, rotate_method='ANY', shape_method='CONCAVE',
                              margin_method='FRACTION')
    bpy.ops.object.mode_set(mode='OBJECT')
    me = body.data
    uvs = np.empty(len(me.loops) * 2, np.float32)
    me.uv_layers['UVMap'].data.foreach_get('uv', uvs)
    area3 = sum(p.area for p in me.polygons)
    area2 = sum(_uv_area(me, p) for p in me.polygons)
    log(f'atlas: {len(_uv_islands_count(body))} islands, {shrunk} cap islands shrunk; {area2 * 100:.0f}% of it used; '
        f'{SIZE * math.sqrt(area2 / max(area3, 1e-9)):.0f} px a metre on the stone')
    return lbk.uv_digest([body])


def _uv_islands_count(body):
    bm = bmesh.new()
    bm.from_mesh(body.data)
    found = _uv_islands(bm, bm.loops.layers.uv['UVMap'])
    bm.free()
    return found


def _uv_area(me, p):
    layer = me.uv_layers['UVMap'].data
    pts = [layer[i].uv for i in p.loop_indices]
    return 0.5 * abs(sum(pts[i].x * pts[(i + 1) % len(pts)].y - pts[(i + 1) % len(pts)].x * pts[i].y
                         for i in range(len(pts))))


# --- 2. Bakes ---

def _emit(name, build):
    return rb._emission_material(name, build)


def _texture(nodes, links, image, colorspace):
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = image
    image.colorspace_settings.name = colorspace
    uvn = nodes.new('ShaderNodeUVMap')
    uvn.uv_map = 'UVMap'
    links.new(uvn.outputs['UV'], tex.inputs['Vector'])
    return tex


def _swap(objects, pick):
    """Each object's slots swapped for pick(slot material name); returns what to restore."""
    kept = []
    for o in objects:
        for slot in o.material_slots:
            kept.append((slot, slot.material))
            slot.material = pick(slot.material.name if slot.material else '')
    return kept


def _bake(body, sources, img, kind, cage=0.03, reach=0.07, normal=False):
    sc = rb._scene()
    sc.render.bake.margin = 16
    target = bpy.data.materials.new('_bake_target')
    target.use_nodes = True
    node = target.node_tree.nodes.new('ShaderNodeTexImage')
    node.image = img
    target.node_tree.nodes.active = node
    kept = _swap([body], lambda name: target)
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    for s in sources:
        s.select_set(True)
    body.select_set(True)
    bpy.context.view_layer.objects.active = body
    if normal:
        bpy.ops.object.bake(type='NORMAL', normal_space='TANGENT', use_selected_to_active=bool(sources),
                            cage_extrusion=cage, max_ray_distance=reach, margin=16, margin_type='EXTEND',
                            use_clear=False, target='IMAGE_TEXTURES')
    else:
        bpy.ops.object.bake(type='EMIT', use_selected_to_active=bool(sources), cage_extrusion=cage,
                            max_ray_distance=reach, margin=16 if sources else 0, margin_type='EXTEND',
                            use_clear=False, target='IMAGE_TEXTURES')
    for slot, m in kept:
        slot.material = m
    bpy.data.materials.remove(target)
    for o in bpy.context.view_layer.objects:
        o.select_set(False)


def bake(body, sources, maps, size=SIZE):
    """{'tripo': Tripo's colour (linear), 'hit', 'N': normal (OpenGL 0..1), 'cap', 'iron', 'mask', 'P' (m), 'Nw'}."""
    out = {}
    tripo_bc = _emit('_bake_tripo_bc', lambda n, l, into: l.new(
        _texture(n, l, maps['BC'], 'sRGB').outputs['Color'], into))
    white = _emit('_bake_white', lambda n, l, into: setattr(into, 'default_value', (1.0, 1.0, 1.0, 1.0)))
    black = _emit('_bake_black', lambda n, l, into: setattr(into, 'default_value', (0.0, 0.0, 0.0, 1.0)))
    for kind, pick in (('tripo', lambda name: tripo_bc if name == '_Skin' else black),
                       ('hit', lambda name: white if name == '_Skin' else black)):
        img = rb._image('_bake_' + kind, size)
        rb._fill(img, (0.0, 0.0, 0.0, 0.0))
        kept = _swap(sources, pick)
        _bake(body, sources, img, kind)
        for slot, m in kept:
            slot.material = m
        out[kind] = rb._read(img)[..., :3]
        bpy.data.images.remove(img)
    out['hit'] = out['hit'][..., 0] > 0.5
    # Tripo's normal map on its skin (its caps flat), cast as a tangent normal bake.
    skin = bpy.data.materials.new('_bake_skin_n')
    skin.use_nodes = True
    nt = skin.node_tree
    bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
    nmap = nt.nodes.new('ShaderNodeNormalMap')
    nmap.uv_map = 'UVMap'
    nt.links.new(_texture(nt.nodes, nt.links, maps['N'], 'Non-Color').outputs['Color'], nmap.inputs['Color'])
    nt.links.new(nmap.outputs['Normal'], bsdf.inputs['Normal'])
    flat = bpy.data.materials.new('_bake_flat_n')
    flat.use_nodes = True
    img = rb._image('_bake_N', size)
    rb._fill(img, (0.5, 0.5, 1.0, 1.0))
    kept = _swap(sources, lambda name: skin if name == '_Skin' else flat)
    _bake(body, sources, img, 'N', normal=True)
    for slot, m in kept:
        slot.material = m
    out['N'] = rb._read(img)[..., :3]
    bpy.data.images.remove(img)
    # The statue's own slots: R cap, G iron, B covered.
    flags = {'_Cap': (1.0, 0.0, 1.0, 1.0), '_Iron': (0.0, 1.0, 1.0, 1.0)}
    mats = {}
    for name in [m.name for m in body.data.materials]:
        color = flags.get(name, (0.0, 0.0, 1.0, 1.0))
        mats[name] = _emit('_bake_flag' + name, lambda n, l, into, c=color: setattr(into, 'default_value', c))
    img = rb._image('_bake_flags', size)
    rb._fill(img, (0.0, 0.0, 0.0, 0.0))
    kept = _swap([body], lambda name: mats[name])
    sc = rb._scene()
    sc.render.bake.margin = 6
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    body.select_set(True)
    bpy.context.view_layer.objects.active = body
    for slot in body.material_slots:
        node = slot.material.node_tree.nodes.new('ShaderNodeTexImage')
        node.image = img
        slot.material.node_tree.nodes.active = node
    bpy.ops.object.bake(type='EMIT', margin=6, margin_type='EXTEND', use_clear=False, target='IMAGE_TEXTURES')
    for slot, m in kept:
        slot.material = m
    flags_px = rb._read(img)
    bpy.data.images.remove(img)
    out['cap'], out['iron'] = flags_px[..., 0] > 0.5, flags_px[..., 1] > 0.5
    out['mask'] = flags_px[..., 2] > 0.5
    for m in list(mats.values()) + [tripo_bc, white, black, skin, flat]:
        bpy.data.materials.remove(m)
    # Each texel's place and normal (world, m), for moss on upward folds and rust running down.
    for kind in ('POSITION', 'NORMAL'):
        img = rb._image('_bake_' + kind, size)
        rb._fill(img, (0.5, 0.5, 0.5, 1.0))
        rb._bake(body, None, img, kind, 0.0, 0.0, False)
        out['P' if kind == 'POSITION' else 'Nw'] = (rb._read(img)[..., :3] - 0.5) * 2.0
        bpy.data.images.remove(img)
    covered = out['mask'].sum()
    log(f'bake: {covered / out["mask"].size * 100:.0f}% of the atlas covered; '
        f'{(out["mask"] & ~out["hit"] & ~out["cap"] & ~out["iron"]).sum() / max(covered, 1) * 100:.2f}% of it missed Tripo\'s skin')
    return out


# --- 3. Our look ---

def _smooth(x, lo, hi):
    t = np.clip((x - lo) / (hi - lo), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def _luma(c):
    return c @ np.array([0.2126, 0.7152, 0.0722], np.float32)


def granite(body, size=SIZE):
    """RockGranite as the plinth wears it (box-mapped at UV_DENSITY), cast onto the statue's atlas: {'BC', 'ORM'}."""
    mat = lt.material(GRANITE, name='_SentinelGranite')
    return rb.material_look(body, mat, mapper=lambda o: lt.box_uv(o, GRANITE, texel_density=UV_DENSITY, seed=3),
                            size=size)


def compose(b, g):
    """Our look from the bakes b (bake()) and the granite g (granite()): {'BC' linear, 'ORM', 'N' DirectX 0..1}."""
    mask, cap, iron = b['mask'], b['cap'], b['iron']
    known = b['hit'] & ~cap & ~iron
    tripo = rb._dilate(b['tripo'], known, steps=48)
    lum = _luma(tripo)
    # Tripo's carving as light and shade: each texel over its neighbourhood's mean (cracks, crevices and the carved lines
    # under 1), its smeared blotches clipped off above.
    local = lt.blur(lum, 10.0) + 1e-4
    shade = np.clip(lum / local, 0.42, 1.12)
    shade = shade * 0.85 + 0.15 * np.clip(lt.blur(lum, 40.0) / (lum[known].mean() + 1e-4), 0.75, 1.15)
    stone = g['BC'] * shade[..., None]
    # Moss: where Tripo grew it (green over red and blue), crisp edges broken by noise; and creeping into upward folds.
    srgb = rb._linear_to_srgb(tripo)
    green = srgb[..., 1] - 0.5 * (srgb[..., 0] + srgb[..., 2])
    n = lt.noise(lum.shape, 7, 3.0, octaves=3)
    grown = _smooth(lt.blur(green, 3.0) + 0.03 * lt.noise(lum.shape, 9, 8.0, octaves=2), 0.07, 0.085)
    up = b['Nw'][..., 2]
    fold = _smooth(up, 0.55, 0.85) * _smooth(-(shade - 1.0), 0.10, 0.25) * _smooth(n, 0.3, 0.6)
    moss = np.clip(np.maximum(grown, fold), 0.0, 1.0) * ~cap * ~iron
    var = np.clip(_luma(g['BC']) / (_luma(g['BC'])[mask].mean() + 1e-4), 0.7, 1.3)
    moss_bc = MOSS[None, None, :] * (var * np.clip(shade, 0.6, 1.1))[..., None]
    bc = stone * (1.0 - moss[..., None]) + moss_bc * moss[..., None]
    rough = g['ORM'][..., 1] * (1.0 - moss) + 0.93 * moss
    ao = np.clip(shade, 0.5, 1.0) ** 0.6
    # Caps: fresh-cut granite, a little lighter, no carving and no moss.
    cap_bc = g['BC'] * 1.06
    bc = np.where(cap[..., None], cap_bc, bc)
    ao = np.where(cap, 1.0, ao)
    # Iron: dark, pitted with rust; rust running down the stone below each staple.
    rust_n = _smooth(lt.noise(lum.shape, 11, 1.5, octaves=2), -0.2, 0.9)
    iron_bc = IRON[None, None, :] * (1.0 - rust_n[..., None]) + RUST[None, None, :] * 0.6 * rust_n[..., None]
    bc = np.where(iron[..., None], iron_bc, bc)
    rough = np.where(iron, 0.48 + 0.3 * rust_n, rough)
    metal = np.where(iron, 0.75 * (1.0 - rust_n), 0.0)
    streak = _rust_streaks(b, lum.shape) * ~iron * ~cap
    bc = bc * (1.0 - streak[..., None] * 0.55) + RUST[None, None, :] * (streak[..., None] * 0.55)
    orm = np.stack([np.where(mask, ao, 1.0), np.clip(rough, 0.05, 1.0), metal], axis=-1).astype(np.float32)
    # Normal: Tripo's carving (cast with the pieces' own shape difference), its finest grain from our granite's colour.
    nt = b['N'] * 2.0 - 1.0
    nt = np.where((cap | iron)[..., None], np.array([0.0, 0.0, 1.0], np.float32), nt)
    detail = rb.normals({'BC': g['BC']}, relief=0.6) * 2.0 - 1.0          # DirectX
    detail[..., 1] = -detail[..., 1]                                        # back to OpenGL to add
    nx, ny = nt[..., 0] + detail[..., 0], nt[..., 1] + detail[..., 1]
    nrm = np.stack([nx, ny, np.maximum(nt[..., 2], 0.2)], axis=-1)
    nrm /= np.linalg.norm(nrm, axis=-1, keepdims=True)
    nrm[..., 1] = -nrm[..., 1]                                              # DirectX for Unreal
    log(f'look: moss on {moss[mask].mean() * 100:.0f}% of the stone, rust under {len(STAPLES)} staples')
    return {'BC': np.clip(bc, 0.0, 1.0).astype(np.float32), 'ORM': orm, 'N': (nrm * 0.5 + 0.5).astype(np.float32)}


def _rust_streaks(b, shape):
    """Rust run down the stone from each staple: a narrow band under it, fading over 25 cm, on faces not facing down."""
    p, up = b['P'], b['Nw'][..., 2]
    out = np.zeros(shape, np.float32)
    wobble = 0.01 * lt.noise(shape, 17, 2.0)
    for centre, half in STAPLE_CENTRES:
        d = p - np.asarray(centre, np.float32)
        below = -d[..., 2]
        side = np.hypot(d[..., 0], d[..., 1])
        # As wide as the staple's reach at its top, narrowing as it runs.
        width = half * (1.0 - 0.5 * _smooth(below, 0.0, 0.25)) + wobble
        band = 1.0 - _smooth(side, width * 0.6, width)
        run = _smooth(below, -0.005, 0.012) * (1.0 - _smooth(below, 0.03, 0.26))
        out = np.maximum(out, band * run * (up > -0.5))
    return np.clip(out, 0.0, 1.0)


STAPLE_CENTRES = []   # (centre, half its length) per staple as posed (world m): record_staples()


def record_staples(body):
    """The staples' centres as posed (the iron faces' middles, one per staple, found by the iron islands)."""
    STAPLE_CENTRES.clear()
    me = body.data
    slot = me.materials.find('_Iron')
    if slot < 0:
        return
    bm = bmesh.new()
    bm.from_mesh(me)
    faces = [f for f in bm.faces if f.material_index == slot]
    seen = set()
    for f in faces:
        if f.index in seen:
            continue
        stack, pts = [f], []
        seen.add(f.index)
        while stack:
            g = stack.pop()
            pts.extend(v.co.copy() for v in g.verts)
            for e in g.edges:
                for h in e.link_faces:
                    if h.material_index == slot and h.index not in seen:
                        seen.add(h.index)
                        stack.append(h)
        c = sum(pts, Vector()) / len(pts)
        STAPLE_CENTRES.append((tuple(c), max((q - c).length for q in pts)))
    bm.free()


# --- 4. Write ---

def write(look, digest):
    mat = rb.write(look, look['N'], SET, MATERIAL)
    with open(lbk._uv_stamp(SET), 'w') as f:
        f.write(digest)
    return mat


def cached(digest, sources):
    """The set on disk fits this atlas (uv.digest) and is newer than every source."""
    return rb.cached(SET, sources) and lbk.uvs_match(SET, digest)


def finish(body, look_material):
    """body's slots all become the one textured material (caps and iron are painted into the atlas)."""
    me = body.data
    me.materials.clear()
    me.materials.append(look_material)
    for p in me.polygons:
        p.material_index = 0
