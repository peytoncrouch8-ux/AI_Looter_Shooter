"""Bakes procedural looks (looter_looks.py) into a texture set the game's textured masters read: T_<Set>_BC.png (sRGB
colour), T_<Set>_N.png (tangent-space normal, DirectX: green down, as Unreal expects) and T_<Set>_ORM.png (linear: R
ambient occlusion, G roughness, B metallic), in Art/Textures/<Set>/ (looter_textures' convention).

    pack(objects)                                  # one atlas: every object's 'UVMap' islands, even density, packed
    maps = bake_set('EllisGear', objects, 2048)   # bakes every material on them (each must be a look)

Each pass swaps each look's output for an emission of one of its OUT_ reroutes (exact values, no lighting), except the
normal (Cycles' normal bake of the look's shading normal) and the occlusion (Cycles' AO bake, which sees every object in
the scene, so a belt shades the coat under it). With sources, the passes are cast from the game parts onto detailed
ones (selected to active): a light game mesh wears the detailed model's folds, creases and seams. Bakes are
deterministic for a given Blender build.
"""
import hashlib
import os
import time

import bpy
import numpy as np

import looter_textures as lt

PASSES = (('COLOR', 'OUT_COLOR', 'sRGB'), ('ROUGH', 'OUT_ROUGH', 'Non-Color'), ('METAL', 'OUT_METAL', 'Non-Color'))


def log(message):
    print(f'BAKE {message}', flush=True)


def select(objects, active=None):
    bpy.ops.object.mode_set(mode='OBJECT') if bpy.context.object and bpy.context.object.mode != 'OBJECT' else None
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objects:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = active or objects[0]


def smart_uv(ob, angle=60.0, margin=0.01):
    """A fresh 'UVMap' by smart projection (for parts made without UVs, or with overlapping ones)."""
    me = ob.data
    if 'UVMap' not in me.uv_layers:
        me.uv_layers.new(name='UVMap')
    me.uv_layers.active = me.uv_layers['UVMap']
    select([ob])
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=np.radians(angle), island_margin=margin)
    bpy.ops.object.mode_set(mode='OBJECT')


def pack(objects, margin=0.004, weights=None):
    """Packs the 'UVMap' islands of all objects into one 0..1 atlas at an even texel density. weights {object: k}
    scales an object's islands before packing (k > 1 gives it more texels)."""
    for ob in objects:
        ob.data.uv_layers.active = ob.data.uv_layers['UVMap']
    select(objects)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.select_all(action='SELECT')
    bpy.ops.uv.average_islands_scale()
    bpy.ops.object.mode_set(mode='OBJECT')
    if weights:
        for ob, k in weights.items():
            layer = ob.data.uv_layers['UVMap']
            uv = np.empty(len(layer.data) * 2, dtype=np.float32)
            layer.data.foreach_get('uv', uv)
            layer.data.foreach_set('uv', uv * k)
    select(objects)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.select_all(action='SELECT')
    try:
        bpy.ops.uv.pack_islands(margin=margin, rotate=True)
    except TypeError:
        bpy.ops.uv.pack_islands(margin=margin)
    bpy.ops.object.mode_set(mode='OBJECT')


def _image(name, size, colorspace, float_buffer=False):
    img = bpy.data.images.get(name)
    if img is not None:
        bpy.data.images.remove(img)
    img = bpy.data.images.new(name, size, size, alpha=False, float_buffer=float_buffer)
    img.colorspace_settings.name = colorspace
    return img


def _materials(objects):
    mats = []
    for ob in objects:
        for slot in ob.material_slots:
            if slot.material is not None and slot.material not in mats:
                mats.append(slot.material)
    return mats


def _target(mat, img):
    nt = mat.node_tree
    nd = nt.nodes.get('BAKE_TARGET') or nt.nodes.new('ShaderNodeTexImage')
    nd.name = 'BAKE_TARGET'
    nd.image = img
    nt.nodes.active = nd
    nd.select = True


def _output(mat):
    return next(n for n in mat.node_tree.nodes if n.type == 'OUTPUT_MATERIAL')


def _swap_emission(mat, reroute):
    """Wires reroute (an OUT_ node) through an Emission into the output; returns the link to restore."""
    nt = mat.node_tree
    out = _output(mat)
    old = out.inputs['Surface'].links[0].from_socket if out.inputs['Surface'].links else None
    em = nt.nodes.get('BAKE_EMIT') or nt.nodes.new('ShaderNodeEmission')
    em.name = 'BAKE_EMIT'
    em.inputs['Strength'].default_value = 1.0
    for ln in list(em.inputs['Color'].links):
        nt.links.remove(ln)
    nt.links.new(nt.nodes[reroute].outputs[0], em.inputs['Color'])
    nt.links.new(em.outputs[0], out.inputs['Surface'])
    return old


def _restore(mat, old):
    if old is not None:
        mat.node_tree.links.new(old, _output(mat).inputs['Surface'])


def _pixels(img):
    a = np.empty(img.size[0] * img.size[1] * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    return a.reshape(img.size[1], img.size[0], 4)


def bake_set(set_name, objects, size=2048, samples=1, ao_samples=32, ao_distance=0.1, margin=12, sources=None, hide=(),
             cage=0.015, reach=0.04):
    """Bakes a texture set onto objects (their 'UVMap', packed into one atlas) and writes its three PNGs. Returns
    {'BC', 'N', 'ORM': path, 'images': {COLOR, ROUGH, METAL, NORMAL (OpenGL), AO, ORM}}.

    sources: the detailed parts the game parts are baked from. Then colour, roughness, metal and normal are cast from
    each game part (rays from cage metres outside it, reaching reach) onto the sources and read from their looks, so
    the sources' folds and creases land in the normal map; without sources the objects' own looks are baked. The
    occlusion is always baked on the objects themselves, with hide (the sources) out of the render so a detailed part
    lying on its game part doesn't shade it black."""
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    bk = sc.render.bake
    bk.margin = margin
    try:
        bk.target = 'IMAGE_TEXTURES'
    except Exception:
        pass
    mats = _materials(objects)
    for ob in objects:
        ob.data.uv_layers.active = ob.data.uv_layers['UVMap']
    images = {}
    # occlusion: soft, so a quarter of the set's size, scaled up into the ORM (its rays are most of a bake's time)
    ao_size = max(512, size // 4)
    img = _image(f'{set_name}_AO', ao_size, 'Non-Color')
    for m in mats:
        _target(m, img)
    sc.cycles.samples = ao_samples
    if sc.world is None:
        sc.world = bpy.data.worlds.new('BakeWorld')
    sc.world.light_settings.distance = ao_distance
    was = [(o, o.hide_render) for o in hide]
    for o in hide:
        o.hide_render = True
    select(objects)
    t0 = time.time()
    bpy.ops.object.bake(type='AO', margin=margin, use_clear=True)
    for o, h in was:
        o.hide_render = h
    images['AO'] = img
    log(f'{set_name}: occlusion ({time.time() - t0:.0f} s)')
    for m in mats:
        nd = m.node_tree.nodes.get('BAKE_TARGET')
        if nd is not None:
            m.node_tree.nodes.remove(nd)
    if sources:
        target = _joined_copy(objects, f'_{set_name}_target')
        tmat = bpy.data.materials.new('_BakeTarget')
        tmat.use_nodes = True
        target.data.materials.clear()
        target.data.materials.append(tmat)
        for p in target.data.polygons:
            p.material_index = 0
        look_mats = _materials(sources)
        bake_from = sources
        active, writes = target, [tmat]
        kw = dict(use_selected_to_active=True, cage_extrusion=cage, max_ray_distance=reach)
    else:
        look_mats, bake_from, active, writes, kw = mats, objects, objects[0], mats, {}
    for key, reroute, cs in PASSES:
        img = _image(f'{set_name}_{key}', size, cs)
        for m in writes:
            _target(m, img)
        olds = {m.name: _swap_emission(m, reroute) for m in look_mats}
        sc.cycles.samples = samples
        select(bake_from + ([active] if sources else []), active)
        t0 = time.time()
        bpy.ops.object.bake(type='EMIT', margin=margin, use_clear=True, **kw)
        for m in look_mats:
            _restore(m, olds[m.name])
        images[key] = img
        log(f'{set_name}: {key.lower()} ({time.time() - t0:.0f} s)')
    img = _image(f'{set_name}_NORMAL', size, 'Non-Color')
    for m in writes:
        _target(m, img)
    sc.cycles.samples = 1
    select(bake_from + ([active] if sources else []), active)
    t0 = time.time()
    bpy.ops.object.bake(type='NORMAL', normal_space='TANGENT', margin=margin, use_clear=True, **kw)
    images['NORMAL'] = img
    log(f'{set_name}: normal ({time.time() - t0:.0f} s)')
    for m in set(mats) | set(look_mats):
        for name in ('BAKE_TARGET', 'BAKE_EMIT'):
            nd = m.node_tree.nodes.get(name)
            if nd is not None:
                m.node_tree.nodes.remove(nd)
    if sources:
        me = target.data
        bpy.data.objects.remove(target)
        bpy.data.meshes.remove(me)
        bpy.data.materials.remove(tmat)
    # write the set: colour as it is; normal with green flipped (DirectX); ORM packed
    paths = {}
    os.makedirs(os.path.dirname(lt.texture_path(set_name, 'BC')), exist_ok=True)
    paths['BC'] = lt.texture_path(set_name, 'BC')
    _save(_pixels(images['COLOR']), paths['BC'], 'sRGB')
    n = _pixels(images['NORMAL']).copy()
    n[..., 1] = 1.0 - n[..., 1]
    paths['N'] = lt.texture_path(set_name, 'N')
    _save(n, paths['N'], 'Non-Color')
    ao = _resize(_pixels(images['AO'])[..., 0], size)
    orm = np.stack([ao, _pixels(images['ROUGH'])[..., 0], _pixels(images['METAL'])[..., 0],
                    np.ones((size, size), dtype=np.float32)], -1)
    paths['ORM'] = lt.texture_path(set_name, 'ORM')
    _save(orm, paths['ORM'], 'Non-Color')
    images['ORM'] = _from_pixels(f'{set_name}_ORMpacked', orm, 'Non-Color')
    paths['images'] = images
    log(f'{set_name}: written to {os.path.dirname(paths["BC"])}')
    return paths


def _joined_copy(objects, name):
    """One object made of copies of objects (their meshes, UVs and all), to bake onto in one go."""
    copies = []
    for ob in objects:
        c = bpy.data.objects.new(name, ob.data.copy())
        c.matrix_world = ob.matrix_world
        bpy.context.scene.collection.objects.link(c)
        copies.append(c)
    select(copies, copies[0])
    if len(copies) > 1:
        bpy.ops.object.join()
    out = bpy.context.view_layer.objects.active
    out.name = name
    return out


def load_set(set_name):
    """A set baked before, read back from its PNGs into the images bake_set returns (the normal turned back to
    OpenGL), for previews and glTF when the bake is skipped. None if a file is missing."""
    paths = {k: lt.texture_path(set_name, k) for k in ('BC', 'N', 'ORM')}
    if not all(os.path.exists(p) for p in paths.values()):
        return None
    images = {}
    images['COLOR'] = bpy.data.images.load(paths['BC'], check_existing=False)
    images['COLOR'].colorspace_settings.name = 'sRGB'
    orm = bpy.data.images.load(paths['ORM'], check_existing=False)
    orm.colorspace_settings.name = 'Non-Color'
    images['ORM'] = orm
    n = bpy.data.images.load(paths['N'], check_existing=False)
    n.colorspace_settings.name = 'Non-Color'
    px = _pixels(n).copy()
    px[..., 1] = 1.0 - px[..., 1]
    images['NORMAL'] = _from_pixels(f'{set_name}_NORMAL', px, 'Non-Color')
    out = dict(paths)
    out['images'] = images
    return out


def digest(paths, extra=''):
    """A fingerprint of the files a bake depends on (their bytes) and any extra settings."""
    h = hashlib.sha1(extra.encode())
    for p in sorted(paths):
        with open(p, 'rb') as f:
            h.update(f.read())
    return h.hexdigest()


def _resize(a, size):
    """A square single-channel image scaled to size by bilinear interpolation (numpy only)."""
    n = a.shape[0]
    if n == size:
        return a
    x = (np.arange(size) + 0.5) * n / size - 0.5
    i0 = np.clip(np.floor(x).astype(int), 0, n - 1)
    i1 = np.clip(i0 + 1, 0, n - 1)
    f = np.clip(x - i0, 0.0, 1.0)
    rows = a[i0] * (1 - f)[:, None] + a[i1] * f[:, None]
    return rows[:, i0] * (1 - f)[None, :] + rows[:, i1] * f[None, :]


def _from_pixels(name, px, colorspace):
    h, w = px.shape[:2]
    img = _image(name, w, colorspace)
    img.pixels.foreach_set(np.ascontiguousarray(px, dtype=np.float32).ravel())
    return img


def _save(px, path, colorspace):
    """Writes pixels (as Blender holds them: bottom row first, RGBA 0..1) to an 8-bit PNG."""
    img = _from_pixels('_save', px, colorspace)
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)
