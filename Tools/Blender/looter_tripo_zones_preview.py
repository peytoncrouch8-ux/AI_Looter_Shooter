"""Preview helpers of looter_tripo_zones: M_World's detail layer emulated on a preview material (its chip mask too), a
colour map's alpha read as a separate mask, the zone map in false colours (over its atlas and on the model), and round
2's before/after previews of the Lamplighter and the Unpaid (Saved/ArtPreviews/Rebake2/; run as a script, see below)."""
import bpy
import numpy as np

import looter_textures as lt


def straight_colour(mat):
    """A preview material's colour images read their alpha as a separate channel (a mask, as Unreal reads it), so the
    colour is never premultiplied by it."""
    if mat is None or not mat.use_nodes:
        return
    for n in mat.node_tree.nodes:
        if n.type == 'TEX_IMAGE' and n.image and n.image.filepath.endswith('_BC.png'):
            n.image.alpha_mode = 'CHANNEL_PACKED'



def detail_copy(mat, set_name, strength, scale, mean, name, bump=0.25, near=2.0, far=15.0, mask_alpha=True):
    """A copy of mat (an lt.material) with M_World's detail layer emulated for a preview: the set's luma (box-projected
    in model space, scale m a tile) over its mean, lerp(1, shade, strength x fade) on the colour, fading from near to far
    m from the camera, and a bump from it (the layer leans the normal by the set's normal map); mask_alpha: times the
    colour map's alpha (DetailMaskAlpha)."""
    m = mat.copy()
    m.name = name
    nodes, links = m.node_tree.nodes, m.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    src = bsdf.inputs['Base Color'].links[0].from_socket
    coords = nodes.new('ShaderNodeTexCoord')
    mapping = nodes.new('ShaderNodeMapping')
    mapping.inputs['Scale'].default_value = (1.0 / scale,) * 3
    links.new(coords.outputs['Object'], mapping.inputs['Vector'])
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = bpy.data.images.load(lt.texture_path(set_name, 'BC'), check_existing=True)
    tex.projection, tex.projection_blend = 'BOX', 0.25
    links.new(mapping.outputs['Vector'], tex.inputs['Vector'])
    luma = nodes.new('ShaderNodeRGBToBW')
    links.new(tex.outputs['Color'], luma.inputs['Color'])
    shade = nodes.new('ShaderNodeMath')
    shade.operation = 'DIVIDE'
    links.new(luma.outputs['Val'], shade.inputs[0])
    shade.inputs[1].default_value = mean
    camera = nodes.new('ShaderNodeCameraData')
    fade = nodes.new('ShaderNodeMapRange')
    links.new(camera.outputs['View Distance'], fade.inputs['Value'])
    fade.inputs['From Min'].default_value, fade.inputs['From Max'].default_value = near, far
    fade.inputs['To Min'].default_value, fade.inputs['To Max'].default_value = 1.0, 0.0
    amount = nodes.new('ShaderNodeMath')
    amount.operation = 'MULTIPLY'
    links.new(fade.outputs['Result'], amount.inputs[0])
    amount.inputs[1].default_value = strength
    bc_node = next((n for n in nodes if n.type == 'TEX_IMAGE' and n.image and n.image.filepath.endswith('_BC.png')), None)
    straight_colour(m)
    if mask_alpha and bc_node is not None:
        masked = nodes.new('ShaderNodeMath')
        masked.operation = 'MULTIPLY'
        links.new(amount.outputs['Value'], masked.inputs[0])
        links.new(bc_node.outputs['Alpha'], masked.inputs[1])
        amount = masked
    lerp = nodes.new('ShaderNodeMix')
    lerp.data_type = 'FLOAT'
    links.new(amount.outputs['Value'], lerp.inputs['Factor'])
    lerp.inputs['A'].default_value = 1.0
    links.new(shade.outputs['Value'], lerp.inputs['B'])
    mul = nodes.new('ShaderNodeMix')
    mul.data_type = 'RGBA'
    mul.blend_type = 'MULTIPLY'
    mul.inputs['Factor'].default_value = 1.0
    links.new(src, mul.inputs['A'])
    links.new(lerp.outputs['Result'], mul.inputs['B'])
    links.new(mul.outputs['Result'], bsdf.inputs['Base Color'])
    bmp = nodes.new('ShaderNodeBump')
    bmp.inputs['Distance'].default_value = 0.002
    bmp.inputs['Strength'].default_value = bump
    links.new(luma.outputs['Val'], bmp.inputs['Height'])
    if bsdf.inputs['Normal'].links:
        links.new(bsdf.inputs['Normal'].links[0].from_socket, bmp.inputs['Normal'])
    links.new(bmp.outputs['Normal'], bsdf.inputs['Normal'])
    return m


def zone_preview(obj, folder, zone, core, colours, keep, close, own):
    """The zone map (zone: int8 image, -1 off the walls) in false colours over its atlas (zones_atlas.png) and on the
    ship: on the walls' slot when they have their own set (own; the rest grey), else on every slot."""
    import os
    import looter_train as tk
    size = zone.shape[0]
    rows = np.full((size, size, 4), 0.25, np.float32)
    rows[..., 3] = 1.0
    rows[core, :3] = colours[np.clip(zone[core], 0, keep)]
    lt.write_png(os.path.join(folder, 'zones_atlas.png'), lt.to8(rows[::2, ::2, :3]))
    img = bpy.data.images.new('_paint_zones', size, size, alpha=True, float_buffer=True)
    img.colorspace_settings.name = 'Non-Color'
    img.pixels.foreach_set(rows[::-1].ravel())
    del rows
    mat = bpy.data.materials.new('_paint_zones')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = img
    tex.interpolation = 'Closest'
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    grey = bpy.data.materials.new('_paint_grey')
    grey.diffuse_color = (0.25, 0.25, 0.25, 1.0)
    kept = [sl.material for sl in obj.material_slots]
    for sl in obj.material_slots:
        on_walls = sl.material is not None and sl.material.name.endswith('_Walls')
        sl.material = mat if (on_walls or not own) else grey
    tk.render([obj], 'zones_tripo_angle', view=(1.05, 1.0, 0.75), lens=50.0, fit=0.95, folder=folder)
    tk.render([obj], 'zones_side', view=(1.0, -0.08, 0.02), lens=70.0, fit=0.62, resolution=(1920, 1080), folder=folder)
    target, way, dist, lens = close
    tk.closeup([obj], 'zones_house_3m', target, way, dist, lens=lens, folder=folder)
    for sl, m in zip(obj.material_slots, kept):
        sl.material = m
    bpy.data.materials.remove(mat)
    bpy.data.materials.remove(grey)
    bpy.data.images.remove(img)


def zone_material(zone, core, colours, keep, uv=None, name='_paint_zones'):
    """The zone map (int8 image, -1 off the painted faces) in false colours as a material (on UV map uv, default the
    active one), and the colour rows; the caller removes the material and its image."""
    size = zone.shape[0]
    rows = np.full((size, size, 4), 0.25, np.float32)
    rows[..., 3] = 1.0
    rows[core, :3] = colours[np.clip(zone[core], 0, keep)]
    img = bpy.data.images.new(name, size, size, alpha=True, float_buffer=True)
    img.colorspace_settings.name = 'Non-Color'
    img.pixels.foreach_set(rows[::-1].ravel())
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = img
    tex.interpolation = 'Closest'
    if uv:
        uvn = nodes.new('ShaderNodeUVMap')
        uvn.uv_map = uv
        links.new(uvn.outputs['UV'], tex.inputs['Vector'])
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    return mat, rows


# --- Round 2's previews (Saved/ArtPreviews/Rebake2/): the Lamplighter and the Unpaid, before and after ---
#   Tools\artrun.ps1 -Script Tools\Blender\looter_tripo_zones_preview.py -ScriptArgs '--model=Lamplighter'
#   with LOOTER_REBAKE=0, then =1: the same with '--model=Unpaid' (one Unpaid build a run: the before views, then
#   the after views and the zone map)

FOLDER = 'Rebake2'
LAMP_ZONES = np.array([[0.1, 0.5, 0.55], [0.6, 0.62, 0.5], [0.4, 0.45, 0.5], [0.45, 0.25, 0.1], [0.85, 0.2, 0.12],
                       [0.95, 0.9, 0.8], [0.8, 0.6, 0.3], [0.95, 0.75, 0.2], [0.05, 0.05, 0.05], [0.1, 0.9, 0.3]],
                      np.float32)
UNPAID_ZONES = np.array([[0.45, 0.55, 0.7], [0.95, 0.9, 0.75], [0.3, 0.18, 0.1], [0.7, 0.45, 0.2], [0.2, 0.5, 0.25],
                         [0.1, 0.9, 0.3]], np.float32)


def _lamp_views(obj):
    """The Lamplighter's cameras (her frame: Blender metres, front -Y, deck and hull side measured on the mesh): the hull
    at 1, 3 and 10 m, her deck at eye height, a rail at 1 m, the bag from her deck and at 3 m."""
    me = obj.data
    n = len(me.polygons)
    C, Nn, A = np.empty(n * 3), np.empty(n * 3), np.empty(n)
    me.polygons.foreach_get('center', C)
    me.polygons.foreach_get('normal', Nn)
    me.polygons.foreach_get('area', A)
    C, Nn = C.reshape(-1, 3), Nn.reshape(-1, 3)
    up = (Nn[:, 2] > 0.9) & (np.abs(C[:, 0]) < 2.5) & (np.abs(C[:, 1]) < 8.0) & (C[:, 2] < 9.0)
    h, e = np.histogram(C[up, 2], bins=np.arange(0, 9.0, 0.1), weights=A[up])
    deck = float(e[np.argmax(h)] + 0.05)
    sides = (np.abs(Nn[:, 0]) > 0.8) & (C[:, 2] < deck) & (C[:, 2] > deck - 2.0)
    side = float(np.percentile(np.abs(C[sides, 0]), 90))
    return {'hull_1m': ((side, 2.0, deck - 1.0), (1.0, 0.1, 0.05), 1.0, 30.0),
            'hull_3m': ((side, 2.0, deck - 1.0), (1.0, 0.2, 0.1), 3.0, 30.0),
            'hull_10m': ((side, 0.0, deck - 0.5), (1.0, 0.25, 0.12), 10.0, 35.0),
            'deck_eye': ((0.0, -6.0, deck + 1.0), (0.05, 1.0, 0.12), 6.0, 24.0),
            'rail_1m': ((side - 0.3, -3.0, deck + 0.6), (-0.3, 1.0, 0.3), 1.0, 30.0),
            'bag_from_deck': ((0.0, -2.0, deck + 7.0), (0.3, 0.6, -0.6), 6.0, 24.0),
            'bag_3m': ((side + 1.0, 0.0, deck + 8.0), (1.0, 0.2, 0.0), 3.0, 30.0)}


def lamplighter(folder):
    import os
    import looter_train as tk
    import looter_tripo_wardens as tw
    import looter_tripo_zones as tz
    for tag in ('before', 'after'):
        os.environ['LOOTER_REBAKE'] = '0' if tag == 'before' else '1'
        obj, _ = tw.build('Lamplighter')
        for img in bpy.data.images:
            if img.filepath and 'TripoLamplighter' in img.filepath:
                img.reload()            # lt.material reuses a loaded image by its path: the first build's maps
        tk.render([obj], f'lamp_ship_34_{tag}', view=(1.05, -1.0, 0.6), lens=50.0, fit=0.9, folder=folder)
        tk.render([obj], f'lamp_ship_side_{tag}', view=(1.0, 0.0, 0.1), lens=70.0, fit=0.7, resolution=(1920, 1080),
                  folder=folder)
        for name, (target, way, dist, lens) in _lamp_views(obj).items():
            tk.closeup([obj], f'lamp_{name}_{tag}', target, way, dist, lens=lens, folder=folder)
        if tag == 'after':
            mat, rows = zone_material(tz.LAST['zone'], tz.LAST['core'], LAMP_ZONES, len(LAMP_ZONES) - 1)
            lt.write_png(os.path.join(folder, 'lamp_zones_atlas.png'), lt.to8(rows[::2, ::2, :3]))
            kept = [sl.material for sl in obj.material_slots]
            for sl in obj.material_slots:
                sl.material = mat
            tk.render([obj], 'lamp_zones_34', view=(1.05, -1.0, 0.6), lens=50.0, fit=0.9, folder=folder)
            tk.render([obj], 'lamp_zones_side', view=(1.0, 0.0, 0.1), lens=70.0, fit=0.7, resolution=(1920, 1080),
                      folder=folder)
            for sl, m in zip(obj.material_slots, kept):
                sl.material = m
        mesh = obj.data
        bpy.data.objects.remove(obj)
        bpy.data.meshes.remove(mesh)


def _ghost(name, image):
    """A preview of M_Ghost with PaintAmount 1 (the Unpaid's paint on UV 'Paint'), lit plainly with a faint cold rim."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    uvn = nodes.new('ShaderNodeUVMap')
    uvn.uv_map = 'Paint'
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = image
    links.new(uvn.outputs['UV'], tex.inputs['Vector'])
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Roughness'].default_value = 0.85
    links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    lw = nodes.new('ShaderNodeLayerWeight')
    lw.inputs['Blend'].default_value = 0.12
    rim = nodes.new('ShaderNodeMath')
    rim.operation = 'MULTIPLY'
    links.new(lw.outputs['Facing'], rim.inputs[0])
    rim.inputs[1].default_value = 0.35
    em = nodes.new('ShaderNodeEmission')
    em.inputs['Color'].default_value = (0.55, 0.65, 0.75, 1.0)
    links.new(rim.outputs['Value'], em.inputs['Strength'])
    add = nodes.new('ShaderNodeAddShader')
    links.new(bsdf.outputs['BSDF'], add.inputs[0])
    links.new(em.outputs['Emission'], add.inputs[1])
    links.new(add.outputs['Shader'], out.inputs['Surface'])
    return mat


def unpaid(folder):
    """One Unpaid build (Art/Models/Creatures/Unpaid.py: its paint step writes T_UnpaidPaint_BC, rebaked unless
    LOOTER_REBAKE=0) and its views: front and three-quarter at 3 m, 10 m, the face and the chest at 1 m."""
    import os
    import runpy
    import sys
    from mathutils import Vector
    import looter_train as tk
    import looter_tripo_zones as tz
    tag = 'before' if os.environ.get('LOOTER_REBAKE', '1') == '0' else 'after'
    argv = list(sys.argv)
    sys.argv = [argv[0], '--']
    runpy.run_path(os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Unpaid.py'), run_name='__main__')
    sys.argv = argv
    body, hat = bpy.data.objects['Unpaid'], bpy.data.objects['UnpaidHat']
    image = bpy.data.images.load(lt.texture_path('UnpaidPaint', 'BC'), check_existing=False)
    mat = _ghost('_ghost_' + tag, image)
    for o in (body, hat):
        for k, m in enumerate(o.data.materials):
            if m is None or m.name != 'GhostCoal':
                o.data.materials[k] = mat
    pts = [o.matrix_world @ Vector(c) for o in (body, hat) for c in o.bound_box]
    lo = np.min([np.array(p) for p in pts], 0)
    hi = np.max([np.array(p) for p in pts], 0)
    mid = (lo + hi) / 2.0
    head = np.array([mid[0], mid[1], hi[2] - 0.35])
    views = {'front_3m': (mid, (0.0, -1.0, 0.08), 3.0, 40.0), 'q34_3m': (mid, (0.78, -1.0, 0.13), 3.0, 40.0),
             'front_10m': (mid, (0.2, -1.0, 0.1), 10.0, 50.0), 'face_1m': (head, (0.15, -1.0, 0.05), 1.0, 35.0),
             'chest_1m': (mid + np.array([0.0, 0.0, 0.35]), (0.3, -1.0, 0.0), 1.0, 35.0)}
    for name, (target, way, dist, lens) in views.items():
        tk.closeup([body, hat], f'unpaid_{name}_{tag}', tuple(target), way, dist, lens=lens, folder=folder)
    if tag == 'after' and tz.LAST.get('model') == 'Unpaid':
        zm, rows = zone_material(tz.LAST['zone'], tz.LAST['core'], UNPAID_ZONES, len(UNPAID_ZONES) - 1, uv='Paint')
        lt.write_png(os.path.join(folder, 'unpaid_zones_atlas.png'), lt.to8(rows[::2, ::2, :3]))
        for o in (body, hat):
            for k, m in enumerate(o.data.materials):
                if m is None or m.name != 'GhostCoal':
                    o.data.materials[k] = zm
        for name in ('front_3m', 'q34_3m'):
            target, way, dist, lens = views[name]
            tk.closeup([body, hat], f'unpaid_zones_{name}', tuple(target), way, dist, lens=lens, folder=folder)


if __name__ == '__main__':
    import os
    import sys
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    model = next((a.split('=', 1)[1] for a in args if a.startswith('--model=')), 'Lamplighter')
    out = os.path.join(lt.PREVIEW_DIR, FOLDER)
    os.makedirs(out, exist_ok=True)
    {'Lamplighter': lamplighter, 'Unpaid': unpaid}[model](out)
