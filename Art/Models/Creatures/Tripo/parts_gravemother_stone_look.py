"""The texture set GravemotherStone for the Tripo headstone (parts_gravemother_stone.py; GravemotherStone.py paints it):
looter_barrow_stone's painted granite on its own layout (front, back and edge regions, intact and cracked), its carved
border, cross and lettering left out: the carving is Tripo's own, baked from its dense surface onto the clean slab
(Cycles, selected to active): its relief as a tangent normal map, its grooves (Cycles' pointiness under half) darkened,
dirt and moss in their bottoms as the scripted carving had. Not a model: Tools\\models.ps1 skips parts_*.py."""
import math

import bpy
import numpy as np

import looter_barrow as lb
import looter_barrow_stone as lbs
import looter_textures as lt
import parts_gravemother_stone as gst

TEX = lbs.TEX


def _image(name):
    old = bpy.data.images.get(name)
    if old:
        bpy.data.images.remove(old)
    img = bpy.data.images.new(name, TEX, TEX, alpha=True, float_buffer=True)
    img.colorspace_settings.name = 'Non-Color'
    img.pixels.foreach_set(np.tile(np.array([0.5, 0.5, 1.0, 1.0], np.float32), TEX * TEX))
    return img


def _read(img):
    buf = np.empty(TEX * TEX * 4, np.float32)
    img.pixels.foreach_get(buf)
    return buf.reshape(TEX, TEX, 4)[::-1].copy()          # rows top first


def _target_material(img):
    mat = bpy.data.materials.new('_stone_bake_target')
    mat.use_nodes = True
    node = mat.node_tree.nodes.new('ShaderNodeTexImage')
    node.image = img
    mat.node_tree.nodes.active = node
    return mat


def _source_material(kind, maps):
    """Tripo's stone as the bake sees it: 'POINTINESS' its convexity, 'COLOUR' its colour map, 'NORMAL' a surface with
    its normal map (the bake reads the shading normal, Tripo's fine carving with it)."""
    mat = bpy.data.materials.new('_stone_src_' + kind)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    if kind == 'NORMAL':
        bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
        if maps.get('N') is not None:
            tex = nt.nodes.new('ShaderNodeTexImage')
            tex.image = maps['N']
            tex.image.colorspace_settings.name = 'Non-Color'
            nm = nt.nodes.new('ShaderNodeNormalMap')
            nt.links.new(tex.outputs['Color'], nm.inputs['Color'])
            nt.links.new(nm.outputs['Normal'], bsdf.inputs['Normal'])
        nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
        return mat
    em = nt.nodes.new('ShaderNodeEmission')
    if kind == 'POINTINESS':
        geo = nt.nodes.new('ShaderNodeNewGeometry')
        nt.links.new(geo.outputs['Pointiness'], em.inputs['Color'])
    else:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = maps['BC']
        nt.links.new(tex.outputs['Color'], em.inputs['Color'])
    nt.links.new(em.outputs['Emission'], out.inputs['Surface'])
    return mat


def bake(low, tripo):
    """Tripo's stone on the slab's intact regions: its relief (a tangent normal, OpenGL: its mesh and its own normal
    map), its colour, its convexity (pointiness, 0.5 flat)."""
    import looter_tripo
    import parts_gravemother_shape as gs
    obj, maps = looter_tripo.load(gs.GLB)
    mesh = obj.data
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = 4
    target = gst.mesh_object('_stone_low', *low)
    src = gst.mesh_object('_stone_src', tripo['V'], tripo['F'], tripo['uv'])
    for poly in src.data.polygons:
        poly.use_smooth = True
    out = {}
    for kind in ('NORMAL', 'COLOUR', 'POINTINESS'):
        img = _image('_stone_' + kind)
        tmat = _target_material(img)
        target.data.materials.clear()
        target.data.materials.append(tmat)
        src.data.materials.clear()
        src.data.materials.append(_source_material(kind, maps))
        for o in bpy.context.view_layer.objects:
            o.select_set(False)
        target.select_set(True)
        src.select_set(True)
        bpy.context.view_layer.objects.active = target
        bpy.ops.object.bake(type='NORMAL' if kind == 'NORMAL' else 'EMIT', normal_space='TANGENT',
                            use_selected_to_active=True, cage_extrusion=0.03, max_ray_distance=0.06, margin=24,
                            margin_type='EXTEND', use_clear=False, target='IMAGE_TEXTURES')
        out[kind] = _read(img)[..., :3]
        bpy.data.images.remove(img)
    for o in (target, src):
        data = o.data
        bpy.data.objects.remove(o)
        bpy.data.meshes.remove(data)
    return out


def _block(region):
    u0, u1, v0, v1 = lbs.REGIONS[region]
    return slice(int(round((1.0 - v1) * TEX)), int(round((1.0 - v0) * TEX))), \
        slice(int(round(u0 * TEX)), int(round(u1 * TEX)))


def face_look(region, front, cracked, pointy, lines):
    """looter_barrow_stone._paint_face without its painted carving: Tripo's grooves (pointy under 0.5) carry it."""
    xs, zs = lbs._face_xz(region)
    shape = xs.shape
    if not front:
        xs = -xs
    outline = [tuple(p) for p in lb.stone_outline()]
    inside_d = lbs._poly_dist(xs, zs, outline + outline[:1])
    seed = 500 + (0 if front else 50) + (7 if cracked else 0)
    color, height, fine = lbs._granite(shape, 500 + (0 if front else 50), "polished" if front else "pitched")
    groove = np.maximum(lt.smooth(0.485, 0.43, lt.blur(pointy, 1.2)), lines)
    color = lt.mix(color, lt.rgb(0x252321), groove * 0.55)
    color = lt.mix(color, lt.rgb(0x5f6046), groove * lt.smooth(0.2, 1.0, lt.noise(shape, 603, 8.0)) * 0.35)
    ridge = lt.smooth(0.53, 0.6, lt.blur(pointy, 1.2))
    color = lt.mix(color, lt.rgb(0x6c6862), ridge * 0.12)
    streak = lt.smooth(-0.2, 1.3, lt.noise(shape, seed + 10, 4.0, 90.0, octaves=2)) * lt.smooth(20.0, 95.0, zs)
    color = lt.mix(color, lt.rgb(0x302e2b), streak * 0.25)
    soil = lt.smooth(32.0, 6.0, zs + 6.0 * lt.noise(shape, seed + 11, 20.0))
    color = lt.mix(color, lt.rgb(0x6d5b47), soil * 0.7)
    color = lt.mix(color, lt.rgb(0x8d7a60), soil * lt.specks(shape, seed + 12, 0.2, 2.0) * 0.6)
    height += 0.0008 * soil * lt.noise(shape, seed + 13, 3.0)
    lichen = lbs._lichen(shape, seed + 20, 0.22 if front else 0.5) * lt.smooth(15.0, 40.0, zs)
    color = lt.mix(color, lt.mix(lt.rgb(0x868871), lt.rgb(0x989680), lt.smooth(-1, 1, lt.noise(shape, seed + 21, 3.0))),
                   lichen * 0.32)
    color = lt.mix(color, lt.rgb(0xb98a4a), lt.specks(shape, seed + 22, 0.006, 1.6) * 0.75 * lt.smooth(20.0, 50.0, zs))
    height += 0.0005 * lichen
    round_off = lt.smooth(1.6, 0.0, inside_d)
    color = lt.mix(color, lt.rgb(0x6c6862), round_off * 0.25)
    if cracked:
        color, height = lbs._cracks_on(color, height, xs, zs, shape, seed + 30, chips=16 if front else 8)
    rough = np.full(shape, 0.74 if front else 0.86, np.float32) + 0.05 * fine + 0.2 * groove + 0.12 * lichen + 0.15 * soil
    return color, height, rough, groove


def _whiteout(a, b):
    """Two tangent normals (-1..1, z out) laid over each other."""
    n = np.stack([a[..., 0] + b[..., 0], a[..., 1] + b[..., 1], a[..., 2] * b[..., 2]], axis=-1)
    return n / np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)


def paint(low, tripo):
    """Writes T_GravemotherStone_BC, _N and _ORM (Art/Textures/GravemotherStone) for the Tripo slab."""
    baked = bake(low, tripo)
    pointy = baked['POINTINESS'][..., 0]
    relief = baked['NORMAL'] * 2.0 - 1.0
    # Tripo's painted carving and cracks: where its colour runs darker than round it (its value only; its hue and smears
    # are left behind).
    luma = baked['COLOUR'] @ np.array([0.2126, 0.7152, 0.0722], np.float32)
    lines = np.clip((lt.blur(luma, 6.0) - luma) / max(float(np.median(luma)), 1e-3) * 2.2, 0.0, 1.0)
    lines = lt.smooth(0.25, 0.7, lines) * 0.8
    relief[..., 1] *= -1.0                                   # Blender's OpenGL green to DirectX
    for a, b in (('front0', 'front1'), ('back0', 'back1'), ('edge0', 'edge1')):
        pointy[_block(b)] = pointy[_block(a)]
        relief[_block(b)] = relief[_block(a)]
        lines[_block(b)] = lines[_block(a)]
    color = np.zeros((TEX, TEX, 3), np.float32)
    color[:] = lt.rgb(0x44413d)
    height = np.zeros((TEX, TEX), np.float32)
    rough = np.full((TEX, TEX), 0.8, np.float32)
    groove = np.zeros((TEX, TEX), np.float32)
    for region in ('front0', 'front1', 'back0', 'back1'):
        sl = _block(region)
        c, h, r, g = face_look(region, region.startswith('front'), region.endswith('1'), pointy[sl], lines[sl])
        color[sl], height[sl], rough[sl], groove[sl] = c, h, r, g
    for region in ('edge0', 'edge1'):
        sl = _block(region)
        c, h, r = lbs._paint_edge(region, region.endswith('1'))
        g = np.maximum(lt.smooth(0.485, 0.43, lt.blur(pointy[sl], 1.2)), lines[sl] * 0.6)
        color[sl] = lt.mix(c, lt.rgb(0x252321), g * 0.5)
        height[sl], rough[sl], groove[sl] = h, r, g
    px_m = 0.01 / lbs.PX_CM
    ao = lt.occlusion(height, px_m, (2, 5, 12), 1.0) * (1.0 - 0.35 * groove)
    color = color * (0.72 + 0.28 * ao)[..., None]
    painted = lt.normal_map(height, px_m, 1.0).astype(np.float32) / 255.0 * 2.0 - 1.0
    normal = _whiteout(relief, painted)
    lt.save_set(lbs.SET, color, height, px_m, np.clip(rough, 0.0, 1.0), ao=ao, normal=lt.to8(normal * 0.5 + 0.5))
    log_ = f'{lbs.SET}: painted for the Tripo slab ({TEX} px), Tripo\'s carving baked in'
    print('GRAVEMOTHER STONE:', log_, flush=True)
