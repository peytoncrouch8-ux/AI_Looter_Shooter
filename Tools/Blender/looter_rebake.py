"""Re-textures an AI-made mesh (a Tripo model, loaded and fitted by looter_tripo) in our own look: the scripted model it
replaces (our texture sets, tints and crisp material borders) is baked onto the mesh's own UV atlas, so the result reads
like our hand-made parts, not like Tripo's soft, smeared paint (the user, 2026-10-28: sharper textures, cleaner shapes,
our art style; nothing should read as AI-made).

- bake_look() casts from the target's surface onto the reference (Cycles, selected to active) and returns the
  reference's base colour (its set's map times its tint, linear) and ORM at each texel of the target's atlas; texels
  whose ray found nothing are filled from their neighbours. paint= lays painted layers over the cast, texel by texel:
  bake_geometry() gives each texel's place and normal in tool space to draw their borders with, material_look() a
  material's look (a copy of the target in it, its UVs mapped by the caller) over the whole atlas.
- wear() adds edge wear and cavity grime from the target's own shape (Cycles' pointiness): metal worn bright and glossy
  on its convex edges, every surface darker in its creases.
- normals() gives the target's normal map: the fine relief of the new colour map (scratches, grain, pores) pressed in
  as a bump, plus Tripo's own at strength (0 by default: once the mesh is rounded and decimated, Tripo's map no longer
  fits it and draws streaks, and its lumps are what reads as AI-made).
- write() saves T_<set>_BC / _ORM / _N (DirectX) for the model importer and returns the material on the World master.
- glow_faces() lists the target's faces that lie on the reference's glow slots (emissive materials keep their own slot).

    import looter_rebake as rb
    look = rb.bake_look(tripo_obj, scripted_model)
    look = rb.wear(tripo_obj, look)
    mat = rb.write(look, rb.normals(look, maps['N']), 'TripoMeleeGraveSpade', 'MeleeTripoGraveSpade')
"""
import os

import bpy
import numpy as np

import looter_textures as lt

SIZE = 2048


def _srgb_to_linear(c):
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def _linear_to_srgb(c):
    c = np.clip(c, 0.0, 1.0)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(c, 1.0 / 2.4) - 0.055)


def _image(name, size):
    old = bpy.data.images.get(name)
    if old:
        bpy.data.images.remove(old)
    img = bpy.data.images.new(name, size, size, alpha=True, float_buffer=True)
    img.colorspace_settings.name = 'Non-Color'
    return img


def _read(img):
    w, h = img.size
    buf = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(buf)
    return buf.reshape(h, w, 4)[::-1].copy()   # rows top first


def _fill(img, rgba):
    w, h = img.size
    img.pixels.foreach_set(np.tile(np.asarray(rgba, np.float32), w * h))


def _scene():
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = 4
    sc.render.bake.margin = 16
    sc.render.bake.margin_type = 'EXTEND'
    return sc


def _emission_material(name, color_socket_fn):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    em = nodes.new('ShaderNodeEmission')
    em.inputs['Strength'].default_value = 1.0
    links.new(em.outputs['Emission'], out.inputs['Surface'])
    color_socket_fn(nodes, links, em.inputs['Color'])
    return mat


def _source_material(mat, kind):
    """A bake stand-in for a reference material: emits its tinted base colour (kind 'BC', linear), its ORM ('ORM') or
    white ('HIT'), read from its texture set at its own UVs."""
    set_name = mat.get('TextureSet')
    tint = mat.get('Tint')
    scale = float(mat.get('UVScale', 1.0))

    def build(nodes, links, into):
        if kind == 'HIT':
            into.default_value = (1.0, 1.0, 1.0, 1.0)
            return
        path = lt.texture_path(set_name, kind) if set_name else None
        if path and os.path.exists(path):
            coords = nodes.new('ShaderNodeTexCoord')
            mapping = nodes.new('ShaderNodeMapping')
            mapping.inputs['Scale'].default_value = (scale, scale, 1.0)
            links.new(coords.outputs['UV'], mapping.inputs['Vector'])
            tex = nodes.new('ShaderNodeTexImage')
            tex.image = bpy.data.images.load(path, check_existing=True)
            tex.image.colorspace_settings.name = 'sRGB' if kind == 'BC' else 'Non-Color'
            links.new(mapping.outputs['Vector'], tex.inputs['Vector'])
            color = tex.outputs['Color']
            if kind == 'BC' and tint:
                mix = nodes.new('ShaderNodeMix')
                mix.data_type = 'RGBA'
                mix.blend_type = 'MULTIPLY'
                mix.inputs['Factor'].default_value = 1.0
                links.new(color, mix.inputs['A'])
                mix.inputs['B'].default_value = lt.hex_color(int(tint.lstrip('#'), 16))
                color = mix.outputs['Result']
            links.new(color, into)
        elif kind == 'BC':
            bsdf = next((n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None) if mat.use_nodes else None
            into.default_value = tuple(bsdf.inputs['Base Color'].default_value) if bsdf else tuple(mat.diffuse_color)
        else:   # no ORM map: occlusion 1, the material's roughness and metal (a glow: matte)
            bsdf = next((n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None) if mat.use_nodes else None
            r = bsdf.inputs['Roughness'].default_value if bsdf else 0.6
            m = bsdf.inputs['Metallic'].default_value if bsdf else 0.0
            into.default_value = (1.0, r, m, 1.0)
    return _emission_material(f'_bake_{kind}_{mat.name}', build)


def _bake(target, source, img, kind, cage, reach, selected):
    """One Cycles bake into img: from source's stand-in materials (selected) or target's own (source None)."""
    sc = _scene()
    target_mat = bpy.data.materials.new('_bake_target')
    target_mat.use_nodes = True
    node = target_mat.node_tree.nodes.new('ShaderNodeTexImage')
    node.image = img
    target_mat.node_tree.nodes.active = node
    kept_t = [s.material for s in target.material_slots]
    if selected:
        kept_s = [s.material for s in source.material_slots]
        stand = {}
        for slot in source.material_slots:
            if slot.material and slot.material.name not in stand:
                stand[slot.material.name] = _source_material(slot.material, kind)
            slot.material = stand.get(slot.material.name) if slot.material else None
        for slot in target.material_slots:
            slot.material = target_mat
        if not target.material_slots:
            target.data.materials.append(target_mat)
    else:
        # The target's own geometry: kind 'POINTINESS' (Cycles' convexity, 0.5 flat), 'POSITION' or 'NORMAL' (stored
        # as x * 0.5 + 0.5: meters and unit vectors, the model sits at the origin).
        def build(nodes, links, into):
            geo = nodes.new('ShaderNodeNewGeometry')
            if kind == 'POINTINESS':
                links.new(geo.outputs['Pointiness'], into)
                return
            half = nodes.new('ShaderNodeVectorMath')
            half.operation = 'MULTIPLY_ADD'
            half.inputs[1].default_value = (0.5, 0.5, 0.5)
            half.inputs[2].default_value = (0.5, 0.5, 0.5)
            links.new(geo.outputs['Position' if kind == 'POSITION' else 'Normal'], half.inputs[0])
            links.new(half.outputs['Vector'], into)
        own = _emission_material('_bake_own', build)
        own.node_tree.nodes.active = own.node_tree.nodes.new('ShaderNodeTexImage')
        own.node_tree.nodes.active.image = img
        for slot in target.material_slots:
            slot.material = own
        if not target.material_slots:
            target.data.materials.append(own)
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    target.select_set(True)
    if selected:
        source.select_set(True)
    bpy.context.view_layer.objects.active = target
    bpy.ops.object.bake(type='EMIT', use_selected_to_active=selected, cage_extrusion=cage, max_ray_distance=reach,
                        margin=16, margin_type='EXTEND', use_clear=False, target='IMAGE_TEXTURES')
    for slot, m in zip(target.material_slots, kept_t):
        slot.material = m
    if selected:
        for slot, m in zip(source.material_slots, kept_s):
            slot.material = m
        for m in stand.values():
            bpy.data.materials.remove(m)
    else:
        bpy.data.materials.remove(own)
    bpy.data.materials.remove(target_mat)
    for o in bpy.context.view_layer.objects:
        o.select_set(False)


def _dilate(data, known, steps=64):
    """Fills unknown texels from known neighbours (mean of the known ones among the 8 around), steps rings deep."""
    data = data.copy()
    known = known.copy()
    for _ in range(steps):
        if known.all():
            break
        acc = np.zeros_like(data)
        cnt = np.zeros(known.shape, np.float32)
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dx or dy:
                    k = np.roll(np.roll(known, dy, 0), dx, 1)
                    acc += np.roll(np.roll(data, dy, 0), dx, 1) * k[..., None]
                    cnt += k
        new = (~known) & (cnt > 0)
        if not new.any():
            break
        data[new] = acc[new] / cnt[new][:, None]
        known = known | new
    return data


def bake_look(target, source, size=SIZE, cage=0.012, reach=0.03, paint=None):
    """The reference's look on the target's atlas (UV0): {'BC': linear RGB, 'ORM': RGB, 'hit': bool, 'mask': bool (texels
    any face covers)}, rows top first. cage and reach in meters (the cast starts cage outside the target's surface and
    looks reach inward). paint: [(where (H, W) bool, material_look()), ...] laid over the cast in order."""
    out = {}
    hit_img = _image('_bake_hit', size)
    _fill(hit_img, (0.0, 0.0, 0.0, 0.0))
    _bake(target, source, hit_img, 'HIT', cage, reach, True)
    hit_raw = _read(hit_img)
    bpy.data.images.remove(hit_img)
    for kind in ('BC', 'ORM'):
        img = _image('_bake_' + kind, size)
        _fill(img, (0.0, 0.0, 0.0, 0.0))
        _bake(target, source, img, kind, cage, reach, True)
        out[kind] = _read(img)[..., :3]
        bpy.data.images.remove(img)
    # Which texels the target's faces cover (bake margin aside): a plain bake of white with no margin.
    mask_img = _image('_bake_mask', size)
    _fill(mask_img, (0.0, 0.0, 0.0, 0.0))
    sc = _scene()
    sc.render.bake.margin = 0
    def white(nodes, links, into):
        into.default_value = (1.0, 1.0, 1.0, 1.0)
    own = _emission_material('_bake_white', white)
    own.node_tree.nodes.active = own.node_tree.nodes.new('ShaderNodeTexImage')
    own.node_tree.nodes.active.image = mask_img
    kept = [s.material for s in target.material_slots]
    for slot in target.material_slots:
        slot.material = own
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    target.select_set(True)
    bpy.context.view_layer.objects.active = target
    bpy.ops.object.bake(type='EMIT', margin=0, use_clear=False, target='IMAGE_TEXTURES')
    for slot, m in zip(target.material_slots, kept):
        slot.material = m
    bpy.data.materials.remove(own)
    out['mask'] = _read(mask_img)[..., 0] > 0.5
    bpy.data.images.remove(mask_img)
    hit = hit_raw[..., 0] > 0.5
    for where, look in (paint or []):
        # Painted layers win over the cast (where Tripo's shape strays from the reference's, the cast picks up the
        # neighbouring part); texel by texel, so their borders are as crisp as the rule that drew them.
        out['BC'] = np.where(where[..., None], look['BC'], out['BC'])
        out['ORM'] = np.where(where[..., None], look['ORM'], out['ORM'])
        hit = hit | where
        lt._log(f'rebake: a painted layer covers {(where & out["mask"]).sum() / max(out["mask"].sum(), 1) * 100:.1f}% '
                f'of the atlas in use')
    out['hit'] = hit
    missed = out['mask'] & ~hit
    lt._log(f'rebake: {out["mask"].mean() * 100:.1f}% of the atlas covered, {missed.sum() / max(out["mask"].sum(), 1) * 100:.2f}% '
            f'of it missed the reference (filled from neighbours)')
    known = hit
    out['BC'] = _dilate(out['BC'], known)
    out['ORM'] = _dilate(out['ORM'], known)
    return out


def material_look(target, mat, mapper=None, size=SIZE, offset=0.001):
    """mat's look all over the target's atlas ({'BC', 'ORM'}, as bake_look's): a copy of the target in mat, pushed offset
    meters out along its normals and UV-mapped by mapper(copy) for mat's set, baked back by a short cast."""
    import bmesh
    obj = target.copy()
    obj.data = target.data.copy()
    obj.name = obj.data.name = '_RebakeLayer'
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    for f in bm.faces:
        f.material_index = 0
    for v in bm.verts:
        v.co += v.normal * offset
    bm.to_mesh(obj.data)
    bm.free()
    if mapper:
        mapper(obj)
    out = {}
    for kind in ('BC', 'ORM'):
        img = _image('_bake_layer_' + kind, size)
        _fill(img, (0.0, 0.0, 0.0, 0.0))
        _bake(target, obj, img, kind, offset * 2.0, offset * 4.0, True)
        out[kind] = _read(img)[..., :3]
        bpy.data.images.remove(img)
    mesh = obj.data
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    return out


def bake_geometry(target, size=SIZE):
    """Each texel's place and normal in tool space (rows top first): P in cm (u along the tool, v, w), N unit (u, v, w);
    with the bake's margin they run on past the islands' edges, so rules drawn on them cover the margin too."""
    raw = {}
    for kind in ('POSITION', 'NORMAL'):
        img = _image('_bake_' + kind, size)
        _fill(img, (0.5, 0.5, 0.5, 1.0))
        _bake(target, None, img, kind, 0.0, 0.0, False)
        raw[kind] = (_read(img)[..., :3] - 0.5) * 2.0
        bpy.data.images.remove(img)
    p, n = raw['POSITION'] * 100.0, raw['NORMAL']
    to_tool = lambda a: np.stack([-a[..., 1], a[..., 2], a[..., 0]], axis=-1)
    return to_tool(p), to_tool(n)


def pointiness(target, size=SIZE):
    img = _image('_bake_point', size)
    _fill(img, (0.5, 0.5, 0.5, 1.0))
    _bake(target, None, img, 'POINTINESS', 0.0, 0.0, False)
    p = _read(img)[..., 0]
    bpy.data.images.remove(img)
    return p


def _smooth(x, lo, hi):
    t = np.clip((x - lo) / (hi - lo), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def wear(target, look, edge=0.6, grime=0.3, size=SIZE):
    """Edge wear and cavity grime from the target's shape (Cycles pointiness, softened a little): metal (ORM blue > 0.5)
    worn toward bright bare steel and glossier on convex edges, wood and paint lighter there; every surface darker and
    rougher in creases."""
    p = lt.blur(pointiness(target, size), 3.0)
    convex = _smooth(p, 0.55, 0.65)
    crease = _smooth(-p, -0.44, -0.35)
    bc, orm = look['BC'].copy(), look['ORM'].copy()
    metal = np.clip((orm[..., 2] - 0.3) / 0.4, 0.0, 1.0)
    luma = bc @ np.array([0.2126, 0.7152, 0.0722], np.float32)
    bare = np.stack([luma * 0.5 + 0.32] * 3, axis=-1) * np.array([0.98, 0.99, 1.0], np.float32)
    worn_metal = bc + (np.maximum(bare, bc) - bc) * 0.75
    worn_other = bc * 1.18 + 0.02
    worn = worn_metal * metal[..., None] + worn_other * (1.0 - metal[..., None])
    k = (convex * edge)[..., None]
    bc = bc * (1.0 - k) + worn * k
    orm[..., 1] = np.clip(orm[..., 1] - 0.18 * convex * edge * metal + 0.12 * crease * grime, 0.04, 1.0)
    bc = bc * (1.0 - crease * grime * 0.8)[..., None]
    out = dict(look)
    out['BC'], out['ORM'] = np.clip(bc, 0.0, 1.0), orm
    return out


def normals(look, tripo_normal=None, strength=0.0, relief=1.0, size=SIZE):
    """The normal map (DirectX, 0..1, rows top first): Tripo's own (OpenGL in the .glb) scaled by strength (its modeled
    detail; its lumps are why it is not used at full strength) with the colour map's fine relief as a bump (relief: the
    bump's strength; the high frequencies of the luminance: scratches, grain, pores)."""
    luma = look['BC'] @ np.array([0.2126, 0.7152, 0.0722], np.float32)
    height = luma - lt.blur(luma, 3.0)
    gx = (np.roll(height, -1, 1) - np.roll(height, 1, 1)) * 0.5
    gy = (np.roll(height, -1, 0) - np.roll(height, 1, 0)) * 0.5   # rows top first: +row is -v
    nx, ny = -gx * relief * 8.0, gy * relief * 8.0                # OpenGL: +y up the texture
    if tripo_normal is not None and strength > 0.0:
        import looter_tripo as tripo
        t = tripo.pixels(tripo_normal, size)[..., :3] * 2.0 - 1.0
        nx = nx + t[..., 0] * strength
        ny = ny + t[..., 1] * strength
    nz = np.ones_like(nx)
    n = np.stack([nx, ny, nz], axis=-1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    n[..., 1] = -n[..., 1]                                         # DirectX: green down
    return n * 0.5 + 0.5


def write(look, normal, set_name, material_name):
    """Writes the set's maps (BC sRGB, ORM and N raw) and returns its material on the World master."""
    bc_path, orm_path, n_path = (lt.texture_path(set_name, k) for k in ('BC', 'ORM', 'N'))
    os.makedirs(os.path.dirname(bc_path), exist_ok=True)
    lt.write_png(bc_path, lt.to8(_linear_to_srgb(look['BC'])))
    orm = look['ORM'].copy()
    orm[..., 0] = np.clip(orm[..., 0], 0.0, 1.0)
    lt.write_png(orm_path, lt.to8(orm))
    lt.write_png(n_path, lt.to8(normal))
    lt._log(f'{set_name}: wrote T_{set_name}_BC, _ORM, _N ({look["BC"].shape[0]} px)')
    return lt.material(set_name, name=material_name, master='World')


def cached(set_name, sources):
    """True when the set's three maps exist and are newer than every file in sources (an export then skips the bake)."""
    paths = [lt.texture_path(set_name, k) for k in ('BC', 'ORM', 'N')]
    if not all(os.path.exists(p) for p in paths):
        return False
    oldest = min(os.path.getmtime(p) for p in paths)
    return all(os.path.getmtime(s) <= oldest for s in sources if os.path.exists(s))


def glow_faces(target, source, glows):
    """The target's faces whose centre lies nearest a reference face of one of the glow materials: {material: [face]}."""
    from mathutils.bvhtree import BVHTree
    me = source.data
    tree = BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])
    names = {m.name for m in glows}
    found = {m: [] for m in glows}
    by_name = {m.name: m for m in glows}
    for poly in target.data.polygons:
        loc, _n, index, _d = tree.find_nearest(poly.center)
        if index is None:
            continue
        slot = me.polygons[index].material_index
        mat = me.materials[slot] if slot < len(me.materials) else None
        if mat is not None and mat.name in names:
            found[by_name[mat.name]].append(poly.index)
    return found
