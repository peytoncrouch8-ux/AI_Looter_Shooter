"""The Tripo Unpaid's own paint (Art/Textures/UnpaidPaint/T_UnpaidPaint_BC.png) on a second UV channel, 'Paint', of
SK_Unpaid's body and SM_UnpaidHat: parts_unpaid.swap() runs it once both are built. Not a model (Tools\\models.ps1 skips
parts_*.py).

Why (the user, 2026-10-31: the Unpaid "didn't import right", washed out pale, the clothes near white): the Tripo body kept
only four flat tint zones from Tripo's paint (skin and shroud #BAC4C6, shirt #93A5B2), under M_Ghost's near-white rim
(RimStrength 2.2), its own glow and its painted top light; in daylight it read as a white ghost, not the user's model (a
blue-grey dead homesteader in a cream shirt, dark brown waistcoat and tie, a brown tattered robe, a brown hat).

How: a copy of the body and the hat where they lay on Tripo's surface (before the rig's warp) is unwrapped as one atlas
(Blender's smart projection, the islands packed into the lower-left 97%, so the upper-right corner is free); Cycles casts
Tripo's colour map onto it (selected to active, a short cast: the remeshed surface lies within a few mm of Tripo's); the
mouth's teeth and pocket and the coal take the free corner (SOLID: a dull bone colour; M_Ghost's cavity darkens the
pocket and the coal slot draws no paint). The UVs go onto the real body and hat as their second channel (UV 1), so
UV 0 keeps what M_Ghost reads there (the Polymer box projection, the shroud's strip UVs that its fade drifts up).

M_Ghost draws the paint when PaintAmount is 1 (Tools/Unreal/build_creature_materials.py, the 2026-10-31 patch): its
BaseColorMap is then this map on UV 1, recoloured per tint zone by Zone<N>Color / PaintZone<N> (both the zone's mean
paint colour on Ghost_A, so the model's own clothes are Tripo's paint exactly; MI_Ghost_B and _C keep the paint's
detail in their own colours). zone_means() gives those means."""
import os

import bmesh
import bpy
import numpy as np

import looter_textures as lt
import looter_tripo as tripo
import parts_unpaid as pu

SET = 'UnpaidPaint'
SIZE = 2048
LAYER = 'Paint'
PACK = 0.97                  # the islands' share of the atlas (lower left); the rest is the solid corner
SOLID_UV = (0.985, 0.985)    # where the teeth, the mouth's pocket and the coal sit
SOLID = 0xa49a86             # a dull, dirty bone (teeth); the pocket is darkened by its cavity, the coal draws no paint
CAGE, REACH = 0.006, 0.016   # m: the cast starts this far outside the copy and looks this far in: short, so it never
                             # reaches through a hole in Tripo's robe to the dark cloth behind (it painted dark squares
                             # on the robe); a texel it misses takes its island's neighbours' colour (FILL steps)
FILL = 64
TORN = 0.5                   # a robe texel under this share of the robe's median brightness is a tear's inside


def log(message):
    print(f'UNPAID PAINT: {message}', flush=True)


def _copy(obj, V, name):
    """obj's mesh (faces, loops) with its vertices at V (Blender space, meters), as a new object at the origin."""
    me = obj.data.copy()
    me.name = name
    me.vertices.foreach_set('co', np.asarray(V, np.float32).ravel())
    for layer in list(me.uv_layers):
        me.uv_layers.remove(layer)
    me.update()
    out = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(out)
    return out


def _unwrap(objects):
    """One atlas over objects (joined into a scratch copy for the unwrap): their per-loop UVs, in order."""
    bpy.ops.object.select_all(action='DESELECT')
    joined = [o.copy() for o in objects]
    for o, src in zip(joined, objects):
        o.data = src.data.copy()
        bpy.context.scene.collection.objects.link(o)
        o.data.uv_layers.new(name=LAYER)
        o.select_set(True)
    bpy.context.view_layer.objects.active = joined[0]
    bpy.ops.object.join()
    work = bpy.context.view_layer.objects.active
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.4, island_margin=0.006, area_weight=0.0, correct_aspect=True,
                             scale_to_bounds=False)
    bpy.ops.uv.pack_islands(margin=0.006, rotate=True)
    bpy.ops.object.mode_set(mode='OBJECT')
    uv = np.empty(len(work.data.loops) * 2)
    work.data.uv_layers[LAYER].data.foreach_get('uv', uv)
    uv = uv.reshape(-1, 2) * PACK
    me = work.data
    bpy.data.objects.remove(work)
    bpy.data.meshes.remove(me)
    out, at = [], 0
    for o in objects:
        n = len(o.data.loops)
        out.append(uv[at:at + n].copy())
        at += n
    return out


def _set_uvs(obj, uv, name=LAYER):
    layer = obj.data.uv_layers.get(name) or obj.data.uv_layers.new(name=name)
    layer.data.foreach_set('uv', np.asarray(uv, np.float32).ravel())
    obj.data.uv_layers.active_index = 0          # UV 0 stays the active and rendered channel
    return layer


def _source():
    """Tripo's model in Tripo space (parts_unpaid.T), emitting its colour map."""
    obj, maps = tripo.load(pu.GLB)
    co = np.empty(len(obj.data.vertices) * 3)
    obj.data.vertices.foreach_get('co', co)
    obj.data.vertices.foreach_set('co', pu.T(co.reshape(-1, 3)).astype(np.float32).ravel())
    obj.data.update()
    mat = bpy.data.materials.new('_UnpaidPaintSource')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    em = nodes.new('ShaderNodeEmission')
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = maps['BC']
    links.new(tex.outputs['Color'], em.inputs['Color'])
    links.new(em.outputs['Emission'], out.inputs['Surface'])
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return obj, maps


def _bake(target, source, cage=CAGE, reach=REACH):
    """Tripo's colour on the target's only UV layer: (linear RGB rows top first, hit mask)."""
    import looter_rebake as rb
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = 4
    out = {}
    white = bpy.data.images.new('_unpaid_white', 4, 4)
    white.pixels.foreach_set([1.0] * 64)
    for key, image in (('colour', None), ('hit', white)):
        img = rb._image(f'_UnpaidPaint_{key}', SIZE)
        rb._fill(img, (0.0, 0.0, 0.0, 0.0))
        if image is not None:
            node = next(n for n in source.data.materials[0].node_tree.nodes if n.type == 'TEX_IMAGE')
            kept, node.image = node.image, image
        mat = bpy.data.materials.new('_unpaid_bake_target')
        mat.use_nodes = True
        tn = mat.node_tree.nodes.new('ShaderNodeTexImage')
        tn.image = img
        mat.node_tree.nodes.active = tn
        target.data.materials.clear()
        target.data.materials.append(mat)
        bpy.ops.object.select_all(action='DESELECT')
        source.select_set(True)
        target.select_set(True)
        bpy.context.view_layer.objects.active = target
        bpy.ops.object.bake(type='EMIT', use_selected_to_active=True, cage_extrusion=cage, max_ray_distance=reach,
                            margin=3, margin_type='EXTEND', use_clear=False, target='IMAGE_TEXTURES')
        out[key] = rb._read(img)
        bpy.data.images.remove(img)
        bpy.data.materials.remove(mat)
        if image is not None:
            node.image = kept
    bpy.data.images.remove(white)
    hit = out['hit'][..., 0] > 0.5
    return out['colour'][..., :3], hit


def paint(body, body_tripo, hat, hat_tripo, solid_faces):
    """Bakes the set and gives the body and the hat their 'Paint' UVs. body_tripo / hat_tripo: each vertex where it
    lay on Tripo's surface (Tripo space); solid_faces: per body polygon, True for the teeth, pocket and coal."""
    import looter_rebake as rb
    tb = _copy(body, body_tripo, '_UnpaidPaintBody')
    th = _copy(hat, hat_tripo, '_UnpaidPaintHat')
    uv_body, uv_hat = _unwrap([tb, th])
    # The solid faces onto the free corner.
    loop_face = np.repeat(np.arange(len(body.data.polygons)),
                          [p.loop_total for p in body.data.polygons])
    uv_body[np.asarray(solid_faces, bool)[loop_face]] = SOLID_UV
    for obj, uv in ((tb, uv_body), (th, uv_hat)):
        layer = obj.data.uv_layers.new(name=LAYER)
        layer.data.foreach_set('uv', np.asarray(uv, np.float32).ravel())
    _set_uvs(body, uv_body)
    _set_uvs(hat, uv_hat)
    # One target for the cast (selected to active bakes onto the active object only).
    bpy.ops.object.select_all(action='DESELECT')
    tb.select_set(True)
    th.select_set(True)
    bpy.context.view_layer.objects.active = tb
    bpy.ops.object.join()
    target = bpy.context.view_layer.objects.active
    source, maps = _source()
    colour, hit = _bake(target, source)
    log(f'{hit.mean() * 100:.1f}% of the atlas cast from Tripo\'s paint')
    # Where the remesh closed Tripo's torn robe, the cast finds the tear's dark inside: blocky black squares on the robe.
    # The robe's texels far darker than the robe's own tone take their neighbours' colour instead.
    robe = _robe_texels(body, uv_body)
    luma = colour @ np.array([0.2126, 0.7152, 0.0722], np.float32)
    tone = float(np.median(luma[robe & hit])) if (robe & hit).any() else 0.0
    torn = robe & hit & (luma < TORN * tone)
    hit = hit & ~torn
    log(f'robe: {torn.sum()} texels of a tear\'s dark inside refilled ({100 * torn.sum() / max(robe.sum(), 1):.1f}% of '
        f'the robe)')
    colour = rb._dilate(colour, hit, steps=FILL)
    c0 = int(round(SIZE * PACK))
    colour[:SIZE - c0, c0:] = np.array(lt.hex_color(SOLID)[:3], np.float32)       # rows top first: the upper right
    path = lt.texture_path(SET, 'BC')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    lt.write_png(path, lt.to8(rb._linear_to_srgb(colour)))
    log(f'wrote {path} ({SIZE} px)')
    for o in (target, source):
        me = o.data
        bpy.data.objects.remove(o)
        bpy.data.meshes.remove(me)
    for image in [m for m in maps.values() if m]:
        bpy.data.images.remove(image)
    return colour


def _robe_texels(body, uv):
    """The atlas texels of the robe's faces (those the shroud chain carries most): a mask, rows top first."""
    from parts_spider_look import _raster
    names = {g.index: g.name for g in body.vertex_groups}
    tail = np.zeros(len(body.data.vertices))
    for v in body.data.vertices:
        tail[v.index] = sum(g.weight for g in v.groups if names[g.group].startswith('tail'))
    tris = []
    for p in body.data.polygons:
        if p.material_index != 0 or tail[list(p.vertices)].mean() < 0.5:
            continue
        loops = list(range(p.loop_start, p.loop_start + p.loop_total))
        for k in range(1, len(loops) - 1):
            tris.append(uv[[loops[0], loops[k], loops[k + 1]]])
    mask = np.zeros((SIZE, SIZE), bool)
    if tris:
        _, rows, cols, _ = _raster(np.array(tris), SIZE)
        mask[rows, cols] = True
    return mask


def zone_means(colour, objects):
    """Each tint zone's mean paint colour (linear RGB, area-weighted over the faces of objects that use it; the solid
    corner left out): the PaintZone<N> M_Ghost divides by, and Ghost_A's own zone colours."""
    acc = np.zeros((4, 3))
    area = np.zeros(4)
    for obj in objects:
        me = obj.data
        n = len(me.loops)
        uv = np.empty(n * 2)
        me.uv_layers[LAYER].data.foreach_get('uv', uv)
        uv = uv.reshape(-1, 2)
        col = me.color_attributes['Col']
        rgba = np.empty(n * 4, np.float32)
        col.data.foreach_get('color_srgb' if col.data_type == 'BYTE_COLOR' else 'color', rgba)
        zone = np.clip(np.round(rgba.reshape(-1, 4)[:, 0] * 3.0), 0, 3).astype(int)
        for p in me.polygons:
            if p.material_index != 0:
                continue
            loops = range(p.loop_start, p.loop_start + p.loop_total)
            c = uv[list(loops)].mean(0)
            if c[0] > PACK and c[1] > PACK:
                continue
            row = min(SIZE - 1, max(0, int((1.0 - c[1]) * SIZE)))
            col_ = min(SIZE - 1, max(0, int(c[0] * SIZE)))
            z = zone[p.loop_start]
            acc[z] += colour[row, col_] * p.area
            area[z] += p.area
    means = acc / np.maximum(area[:, None], 1e-9)
    return means


def to_hex(linear):
    s = np.clip(np.where(linear <= 0.0031308, linear * 12.92, 1.055 * np.power(np.clip(linear, 0, 1), 1 / 2.4) - 0.055),
                0.0, 1.0)
    return '#' + ''.join(f'{int(round(c * 255)):02X}' for c in s)
