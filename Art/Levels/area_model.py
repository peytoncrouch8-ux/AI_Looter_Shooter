"""An area's terrain as Blender objects, built from Art/Levels/<Area>/layout.json by the area generator next to this
file (area_shape.py, area_mesh.py). Each area's scripted model (Art/README.md), Art/Models/Terrain/<Area>.py, is a
wrapper that calls main('<Area>'). It builds:

- <Area>_Tile_<i>_<j>: the walkable top in a grid of tiles (layout.json mesh.tiles a side; i from south to north, j
  from west to east; tiles with nothing in them are left out). Every tile has its origin at the world origin, so all
  line up when placed at the origin. Material <Area>Macro (master Terrain): UV 0 maps the map square onto the macro
  color map T_<Area>Macro_BC (on the tutorial island U = (Y + 10240) / 20480, V = (X + 10240) / 20480, Unreal cm;
  layout_computed.json macroMap), UV 1 is world meters (U = Y / 100, V = X / 100) for the detail textures, and vertex
  color alpha is baked ambient occlusion (RGB white).
- <Area>_Underside_<n>: the rock under the rim (RockCliff), in four sectors (island setting).
- <Area>_Water: the ponds' and creeks' surface (material Water, master Water; UV 0 in world meters).
- Grounded (area_beyond.py): <Area>_Ring_<n>, the surround ring in sectors (material <Area>RingMacro, master Terrain,
  UV 0 over the ring's square onto T_<Area>RingMacro_BC; the upland's sectors collide, the canyon's don't);
  <Area>_CanyonWall_<n>, the generated wall under the escarpment's lip (RockCliff, no collision); <Area>_Backdrop_<n>,
  the unlit silhouettes (material Backdrop<layer>, master Backdrop; no collision, no Nanite). The core's tiles share
  their seam vertices with the ring exactly; the log prints the seam's gap and normal difference and each piece's
  triangles against its budget.

    blender -b --factory-startup --python Art/Models/Terrain/<Area>.py -- [--macro] [--computed] [--preview]
                                                                         [--save-to DIR]
    blender -b --factory-startup --python Art/Levels/area_model.py -- --layout PATH [the same flags]

  --macro     paints the macro color map and the PCG scatter mask (T_<Area>Scatter_BC: tree, grass, flower and
              pebble densities) into Art/Textures/<Area>Macro (a couple of minutes)
  --computed  writes Art/Levels/<Area>/layout_computed.json (heights and points the placement scripts need)
  --preview   renders Saved/ArtPreviews/Terrain/<Area> (macro.png, scatter.png, plan.png and the layout's views)
  --save-to   writes all of that into DIR instead (layout_computed.json, the two maps, previews/) and leaves the
              repository alone: Tools/terrain_identity.py and dry runs use it
  --layout    builds this layout instead of the area's own (a scratch copy, say)
models.ps1 runs a wrapper without flags: the meshes only. The log ends with the build's time and peak memory.
"""
import argparse
import ctypes
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..'))
for path in (HERE, os.path.join(REPO, 'Tools', 'Blender')):
    if path not in sys.path:
        sys.path.append(path)

import bpy  # noqa: E402
import numpy as np  # noqa: E402

import area_mesh  # noqa: E402
import area_shape  # noqa: E402
import looter_textures as lt  # noqa: E402

DETAIL_SETS = ('GroundGrass', 'RockCliff')  # M_Terrain's detail textures: grass/soil (macro alpha 0), rock (alpha 1)
TILES = 4               # the top's tiles a side, unless the layout's mesh.tiles says otherwise
TOP_TRIANGLES = 118000  # the top's triangle budget, unless the layout's mesh.triangles says otherwise
UNDERSIDE_REPEAT = 9.6  # meters per RockCliff repeat under the island: a third of the usual density, seen from afar
# Triangle budgets per piece (Docs/Areas/RansomsRest.md, the performance plan): the core's tiles, the ring, the canyon
# wall, the backdrop.
BUDGETS = {'core': 150000, 'ring': 40000, 'canyon wall': 20000, 'backdrop': 6000}
# The backdrop's layers, nearest first: tints that fade toward the sky's haze (a material parameter collection takes
# over in the game, step 13).
BACKDROP_TINTS = (0x5b6b5e, 0x7d8a8f, 0xa3adb5)
STARTED = time.time()


def log(message):
    print(f'LOOTER: [{time.time() - STARTED:5.0f} s] {message}', flush=True)


def peak_memory_mb():
    """The process's peak working set and peak private bytes (MB) on Windows; None elsewhere."""
    if sys.platform != 'win32':
        return None

    class Counters(ctypes.Structure):
        _fields_ = [('cb', ctypes.c_ulong), ('PageFaultCount', ctypes.c_ulong),
                    ('PeakWorkingSetSize', ctypes.c_size_t), ('WorkingSetSize', ctypes.c_size_t),
                    ('QuotaPeakPagedPoolUsage', ctypes.c_size_t), ('QuotaPagedPoolUsage', ctypes.c_size_t),
                    ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t), ('QuotaNonPagedPoolUsage', ctypes.c_size_t),
                    ('PagefileUsage', ctypes.c_size_t), ('PeakPagefileUsage', ctypes.c_size_t)]
    counters = Counters()
    counters.cb = ctypes.sizeof(Counters)
    kernel = ctypes.WinDLL('kernel32')
    kernel.GetCurrentProcess.restype = ctypes.c_void_p
    kernel.K32GetProcessMemoryInfo.argtypes = [ctypes.c_void_p, ctypes.POINTER(Counters), ctypes.c_ulong]
    if not kernel.K32GetProcessMemoryInfo(kernel.GetCurrentProcess(), ctypes.byref(counters), counters.cb):
        return None
    return counters.PeakWorkingSetSize / 2 ** 20, counters.PeakPagefileUsage / 2 ** 20


def options():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    # No abbreviations: models.ps1 passes the exporter's own --out, which must not pass for --save-to.
    parser = argparse.ArgumentParser(allow_abbrev=False)
    for flag in ('--macro', '--computed', '--preview'):
        parser.add_argument(flag, action='store_true')
    parser.add_argument('--save-to', default='')
    parser.add_argument('--layout', default='')
    return parser.parse_known_args(argv)[0]


def clear_scene():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj)


# --- Materials ---

def sock(sockets, identifier):
    """A node socket by identifier (the Mix node has an A and a B per data type, all with the same name)."""
    return next(s for s in sockets if s.identifier == identifier)


def macro_path(area):
    return os.path.join(REPO, *area.macro_texture.split('/'))


def ring_macro_path(area):
    return os.path.join(REPO, *area.ring_macro_texture.split('/'))


def macro_material(area, ring=False):
    """The top's material (or the ring's): M_Terrain's macro map and detail sets. Its Blender nodes preview what
    M_Terrain does."""
    name = area.name + ('RingMacro' if ring else 'Macro')
    # The map this run painted (--save-to puts it elsewhere), else the repository's.
    texture = getattr(area, 'painted', {}).get('ring' if ring else 'core') or (
        ring_macro_path(area) if ring else macro_path(area))
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Roughness'].default_value = 0.9
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x6b7a3e)
    mat.diffuse_color = lt.hex_color(0x6b7a3e)
    if os.path.exists(texture):
        uv0 = nodes.new('ShaderNodeUVMap')
        uv0.uv_map = 'UVMap'
        macro = nodes.new('ShaderNodeTexImage')
        macro.image = bpy.data.images.load(texture, check_existing=True)
        macro.image.colorspace_settings.name = 'sRGB'
        macro.image.alpha_mode = 'CHANNEL_PACKED'
        macro.interpolation = 'Linear'
        links.new(uv0.outputs['UV'], macro.inputs['Vector'])
        color = _detail_preview(nodes, links, macro.outputs['Color'], macro.outputs['Alpha'])
    else:
        flat = nodes.new('ShaderNodeRGB')
        flat.outputs['Color'].default_value = lt.hex_color(0x6b7a3e)
        color = flat.outputs['Color']
    # Ambient occlusion from the vertex alpha, as M_Terrain's DiffuseAO (0.45) applies it.
    ao = nodes.new('ShaderNodeVertexColor')
    ao.layer_name = 'Col'
    fade = nodes.new('ShaderNodeMapRange')
    fade.inputs['To Min'].default_value = 0.55
    links.new(ao.outputs['Alpha'], fade.inputs['Value'])
    mix = nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    mix.blend_type = 'MULTIPLY'
    sock(mix.inputs, 'Factor_Float').default_value = 1.0
    links.new(color, sock(mix.inputs, 'A_Color'))
    links.new(fade.outputs['Result'], sock(mix.inputs, 'B_Color'))
    links.new(sock(mix.outputs, 'Result_Color'), bsdf.inputs['Base Color'])
    mat['Master'] = 'Terrain'
    mat['TextureSet'] = name
    mat['DetailSets'] = ','.join(DETAIL_SETS)
    mat['UVScale'] = 1.0
    mat['Kind'] = 'Surface'
    return mat


def _detail_preview(nodes, links, color, select):
    """Macro * lerp(1, detail luma / mean luma, 0.6), the detail on UV 1 (grass every 2 m, rock every 4 m) picked by
    the macro alpha, like M_Terrain. Skipped while the detail textures don't exist."""
    paths = [lt.texture_path(s, 'BC') for s in DETAIL_SETS]
    if not all(os.path.exists(p) for p in paths):
        return color
    uv1 = nodes.new('ShaderNodeUVMap')
    uv1.uv_map = 'UVDetail'
    lumas = []
    for path, scale, mean in zip(paths, (0.5, 0.25), (0.3, 0.35)):
        mapping = nodes.new('ShaderNodeMapping')
        mapping.inputs['Scale'].default_value = (scale, scale, 1.0)
        links.new(uv1.outputs['UV'], mapping.inputs['Vector'])
        tex = nodes.new('ShaderNodeTexImage')
        tex.image = bpy.data.images.load(path, check_existing=True)
        links.new(mapping.outputs['Vector'], tex.inputs['Vector'])
        bw = nodes.new('ShaderNodeRGBToBW')
        links.new(tex.outputs['Color'], bw.inputs['Color'])
        ratio = nodes.new('ShaderNodeMath')
        ratio.operation = 'DIVIDE'
        ratio.inputs[1].default_value = mean
        links.new(bw.outputs['Val'], ratio.inputs[0])
        lumas.append(ratio.outputs['Value'])
    pick = nodes.new('ShaderNodeMix')
    pick.data_type = 'FLOAT'
    links.new(select, sock(pick.inputs, 'Factor_Float'))
    links.new(lumas[0], sock(pick.inputs, 'A_Float'))
    links.new(lumas[1], sock(pick.inputs, 'B_Float'))
    strength = nodes.new('ShaderNodeMix')
    strength.data_type = 'FLOAT'
    sock(strength.inputs, 'Factor_Float').default_value = 0.6
    sock(strength.inputs, 'A_Float').default_value = 1.0
    links.new(sock(pick.outputs, 'Result_Float'), sock(strength.inputs, 'B_Float'))
    mix = nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    mix.blend_type = 'MULTIPLY'
    sock(mix.inputs, 'Factor_Float').default_value = 1.0
    links.new(color, sock(mix.inputs, 'A_Color'))
    links.new(sock(strength.outputs, 'Result_Float'), sock(mix.inputs, 'B_Color'))
    return sock(mix.outputs, 'Result_Color')


def water_material():
    """A flat blue placeholder; in Unreal the instance's parent is M_Water."""
    mat = bpy.data.materials.get('Water') or bpy.data.materials.new('Water')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x2f5b63)
    bsdf.inputs['Roughness'].default_value = 0.08
    mat.diffuse_color = lt.hex_color(0x2f5b63)
    mat['Master'] = 'Water'
    mat['Kind'] = 'Surface'
    return mat


# --- Meshes ---

def make_object(name, verts, tris, material, uvs=None, normals=None, colors=None):
    """A mesh object from layout-meter vertices; uvs is {layer name: per-vertex (N, 2) or per-corner (T * 3, 2)},
    colors per vertex (N, 4)."""
    vb = area_mesh.to_blender(verts)
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vb.tolist(), [], tris.tolist())
    loops = tris.ravel()
    for layer, uv in (uvs or {}).items():
        per_loop = uv if len(uv) == len(loops) and len(uv) != len(verts) else uv[loops]
        mesh.uv_layers.new(name=layer).data.foreach_set('uv', per_loop.astype(np.float32).ravel())
    if colors is not None:
        attr = mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
        attr.data.foreach_set('color_srgb', colors[loops].astype(np.float32).ravel())
        mesh.color_attributes.active_color = attr
    mesh.polygons.foreach_set('use_smooth', [True] * len(mesh.polygons))
    if normals is not None:
        mesh.normals_split_custom_set_from_vertices(normals.tolist())
    mesh.materials.append(material)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def build_top(area):
    settings = area.layout.get('mesh', {})
    verts, tris, ring = area_mesh.top_surface(area, settings.get('triangles', TOP_TRIANGLES), log=log)
    log(f'terrain: top surface {len(verts)} vertices, {len(tris)} triangles')
    normals = area_mesh.vertex_normals(area_mesh.to_blender(verts), tris)
    if np.mean(normals[:, 2]) < 0.0:  # the triangulation runs counter-clockwise; make sure the top faces up
        tris = tris[:, [0, 2, 1]]
        normals = -normals
    geometric = normals.copy()
    if area.setting == 'grounded':
        # The seam's normals come from the regional field's gradient, as the ring's do: no light crease between them.
        seam = ring[area.loop_kinds >= 1]
        normals[seam] = area_mesh.to_blender(area.region.gradient_normals(verts[seam, 0], verts[seam, 1]))
    ao_raster = area_mesh.terrain_ao(area)
    area.ao = ao_raster
    ao = area.at(ao_raster, verts[:, 0], verts[:, 1])
    colors = np.column_stack([np.ones((len(verts), 3)), np.clip(ao, 0.0, 1.0)])
    macro_uv = np.column_stack([(verts[:, 1] + area.half), (verts[:, 0] + area.half)]) / (2.0 * area.half)
    detail_uv = np.column_stack([verts[:, 1], verts[:, 0]])
    mat = macro_material(area)

    # Tiles: a grid over the top's extent, each triangle going with the tile its center falls in.
    count = settings.get('tiles', TILES)
    prefix = area.name + '_Tile_'
    lo, hi = verts[:, :2].min(axis=0), verts[:, :2].max(axis=0)
    center = verts[tris].mean(axis=1)
    ti = np.minimum(((center[:, 0] - lo[0]) / (hi[0] - lo[0]) * count).astype(int), count - 1)
    tj = np.minimum(((center[:, 1] - lo[1]) / (hi[1] - lo[1]) * count).astype(int), count - 1)
    tiles = []
    for i in range(count):
        for j in range(count):
            chosen = tris[(ti == i) & (tj == j)]
            if len(chosen) == 0:
                continue
            used, inverse = np.unique(chosen.ravel(), return_inverse=True)
            local = inverse.reshape(-1, 3)
            obj = make_object(f'{prefix}{i}_{j}', verts[used], local, mat,
                              uvs={'UVMap': macro_uv[used], 'UVDetail': detail_uv[used]},
                              normals=normals[used], colors=colors[used])
            tiles.append((obj.name, len(local)))
    log('terrain: tiles ' + ', '.join(f'{n[len(prefix):]} {c}' for n, c in tiles))
    area.top = dict(verts=verts, tris=tris, ring=ring, kinds=area.loop_kinds, normals=normals, geometric=geometric,
                    ao=np.clip(ao, 0.0, 1.0))
    return verts, tris, ring, tiles


def build_underside(area, ring_xyz):
    rim_drop = area.island['rimDrop'] / 100.0
    depth = area.island['undersideDepth'] / 100.0
    pieces = area_mesh.underside(area, ring_xyz, rim_drop, depth)
    # The same shared instance as the cliff kit's, with its moss (Art/Models/Rocks).
    rock = lt.material('RockCliff', MossAmount=0.6)
    made = []
    for n, (verts, tris, extra) in enumerate(pieces):
        obj = make_object(f'{area.name}_Underside_{n}', verts, tris, rock,
                          uvs={'UVMap': _wrap_uv(verts, tris, extra[:, :2])}, normals=extra[:, 2:5])
        made.append(obj)
    for obj in made:
        _underside_ao(obj, rim_drop, depth)
    log('terrain: underside ' + ', '.join(f'{o.name[len(area.name) + 1:]} {len(o.data.polygons)}' for o in made))
    return made


def _wrap_uv(verts, tris, wrap, repeat=UNDERSIDE_REPEAT):
    """Cylindrical UVs per triangle corner: U around the island (meters along the loop), V up (meters), so the rock's
    strata stay level all the way round. Triangles across the loop's start get their U unwrapped."""
    u = wrap[tris][:, :, 0].copy()
    across = (u.max(axis=1) - u.min(axis=1)) > 0.5
    u[across] = np.where(u[across] < 0.5, u[across] + 1.0, u[across])
    along = u * wrap[tris][:, :, 1]
    up = verts[tris][:, :, 2]
    return (np.stack([along, up], axis=2) / repeat).reshape(-1, 2)


def _underside_ao(obj, rim_drop, depth, top=0.0):
    """Baked occlusion (its own crevices and spires), darkening toward the bottom: the sky light reaches the
    underside mostly from the side. (top: the height the darkening starts below, plus rim_drop.)"""
    baked = True
    try:
        lt.bake_vertex_ao(obj, samples=16, distance=6.0, ground=False)
    except Exception as error:  # no Cycles: occlusion from the depth alone
        log(f'warning: underside AO bake failed ({error}); using depth only')
        baked = False
    mesh = obj.data
    col = mesh.color_attributes.get('Col') or mesh.color_attributes.new('Col', 'BYTE_COLOR', 'CORNER')
    raw = np.ones(4 * len(mesh.loops), dtype=np.float32)
    if baked:
        col.data.foreach_get('color_srgb', raw)
    raw = raw.reshape(-1, 4)
    loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loop_vert)
    co = np.empty(3 * len(mesh.vertices), dtype=np.float32)
    mesh.vertices.foreach_get('co', co)
    z = co.reshape(-1, 3)[loop_vert, 2]
    down = np.clip(-(z - top + rim_drop) / (depth - rim_drop), 0.0, 1.0)
    raw[:, :3] = 1.0
    raw[:, 3] = np.clip(raw[:, 3] * (1.0 - 0.3 * down), 0.0, 1.0)
    col.data.foreach_set('color_srgb', raw.ravel())
    mesh.color_attributes.active_color = col


def build_water(area):
    verts, tris = area_mesh.water(area)
    uv = np.column_stack([verts[:, 1], verts[:, 0]])
    normals = np.tile([0.0, 0.0, 1.0], (len(verts), 1))
    obj = make_object(area.name + '_Water', verts, tris, water_material(), uvs={'UVMap': uv}, normals=normals)
    # Players wade through the ponds and creeks: the water has no collision (their beds do).
    obj['Collision'] = 'None'
    # A flat surface gains nothing from Nanite (and M_Water has no Nanite permutation).
    obj['Nanite'] = 0
    vb = area_mesh.to_blender(verts)
    a, b, c = vb[tris[:, 0]], vb[tris[:, 1]], vb[tris[:, 2]]
    if np.mean(np.cross(b - a, c - a)[:, 2]) < 0.0:
        obj.data.flip_normals()
        obj.data.normals_split_custom_set_from_vertices(normals.tolist())
    log(f'terrain: water {len(tris)} triangles')
    return obj


# --- Past the core (grounded) ---

def backdrop_material(index):
    """An unlit silhouette layer: a flat tint (emission, in Blender's preview), Master Backdrop for the game."""
    name = f'Backdrop{index}'
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    emit = nodes.new('ShaderNodeEmission')
    color = lt.hex_color(BACKDROP_TINTS[min(index, len(BACKDROP_TINTS) - 1)])
    emit.inputs['Color'].default_value = color
    emit.inputs['Strength'].default_value = 1.0
    links.new(emit.outputs['Emission'], out.inputs['Surface'])
    mat.diffuse_color = color
    mat['Master'] = 'Backdrop'
    mat['Tint'] = '#%06x' % BACKDROP_TINTS[min(index, len(BACKDROP_TINTS) - 1)]
    mat['Kind'] = 'Surface'
    return mat


def build_beyond(area):
    """The ring, the canyon wall and the backdrop of a grounded area, with the seam's check and the budgets.
    Returns (pieces by kind: [(name, triangles)], the seam's numbers)."""
    import area_beyond
    started = time.time()
    spec = area.layout['region']
    area_beyond.ring_raster(area, log=log)
    area_beyond.ring_ao(area)
    top = area.top
    ring_spec = spec.get('ring', {})
    upland, canyon, seam = area_beyond.ring(area, top['verts'], top['ring'], top['kinds'], top['normals'], top['ao'],
                                            ring_spec.get('triangles', area_beyond.RING_TRIANGLES), log=log)
    count = ring_spec.get('sectors', 8)
    shares = [('upland', upland, count - (2 if canyon else 0))] + ([('canyon', canyon, 2)] if canyon else [])
    mat = macro_material(area, ring=True)
    made = {'ring': [], 'canyon wall': [], 'backdrop': []}
    index = 0
    for kind, piece, sectors in shares:
        for chosen in area_beyond.sectors(piece['verts'], piece['tris'], sectors):
            if not len(chosen):
                continue
            used, inverse = np.unique(piece['tris'][chosen].ravel(), return_inverse=True)
            v = piece['verts'][used]
            obj = make_object(f'{area.name}_Ring_{index}', v, inverse.reshape(-1, 3), mat,
                              uvs={'UVMap': (np.column_stack([v[:, 1], v[:, 0]]) + area.region.half)
                                   / (2.0 * area.region.half), 'UVDetail': np.column_stack([v[:, 1], v[:, 0]])},
                              normals=piece['normals'][used],
                              colors=np.column_stack([np.ones((len(v), 3)), piece['ao'][used]]))
            if kind == 'canyon':
                obj['Collision'] = 'None'  # nothing past the lip collides: a fall off it ends in the canyon's haze
            made['ring'].append((obj.name, len(chosen), kind))
            index += 1
    # The canyon wall hangs from the whole lip: the ring's south run, the core's lip, the ring's north run.
    if canyon is not None:
        kinds = top['kinds']
        core_lip = top['ring'][:int(np.nonzero(kinds == 2)[0][-1]) + 1]
        lip_xyz = np.vstack([upland['verts'][seam['south_ids']], top['verts'][core_lip],
                             upland['verts'][seam['north_ids']]])
        rock = lt.material('RockCliff', MossAmount=0.6)
        wall_spec = spec.get('canyonWall', {})
        walls = []
        for n, (verts, tris, extra) in enumerate(area_beyond.canyon_wall(area, lip_xyz, wall_spec.get('sectors', 4))):
            obj = make_object(f'{area.name}_CanyonWall_{n}', verts, tris, rock,
                              uvs={'UVMap': _wrap_uv(verts, tris, extra[:, :2])}, normals=extra[:, 2:5])
            obj['Collision'] = 'None'
            walls.append(obj)
            made['canyon wall'].append((obj.name, len(tris), 'wall'))
        for obj in walls:
            _underside_ao(obj, 0.0, area.region.drop, top=float(np.max(lip_xyz[:, 2])))
    for li, sct, verts, tris, extra in area_beyond.backdrop(area):
        obj = make_object(f'{area.name}_Backdrop_{li * spec.get("backdrop", {}).get("sectors", 4) + sct}', verts, tris,
                          backdrop_material(li), uvs={'UVMap': np.column_stack([extra[:, 1], extra[:, 0]])},
                          colors=np.column_stack([np.ones((len(verts), 3)), extra[:, 0]]))
        obj['Collision'] = 'None'
        obj['Nanite'] = 0
        obj.visible_shadow = False
        made['backdrop'].append((obj.name, len(tris), f'layer {li}'))
    numbers = area_beyond.seam(top['verts'], top['tris'], top['normals'], top['geometric'], seam, upland, area.region)
    for kind, pieces in made.items():
        log(f'terrain: {kind} ' + ', '.join(f'{n[len(area.name) + 1:]} {c}' for n, c, _ in pieces))
    log(f'terrain: past the core in {time.time() - started:.0f} s')
    return made, numbers


def report_grounded(area, tiles, made, numbers):
    """Prints the seam's check and each piece's triangles against its budget; returns whether all passed."""
    totals = {'core': sum(c for _, c in tiles)}
    for kind, pieces in made.items():
        totals[kind] = sum(c for _, c, _ in pieces)
    passed = True
    gap_ok = numbers['gap_mm'] == 0.0 and numbers['missing_edges'] == 0
    normal_ok = numbers['normal_deg'] < 1.0
    passed &= gap_ok and normal_ok
    log(f"check: seam over {numbers['vertices']} shared vertices: gap {numbers['gap_mm']:.3f} mm, "
        f"{'no T-junctions' if numbers['missing_edges'] == 0 else str(numbers['missing_edges']) + ' seam edges missing'}"
        f" - {'PASS' if gap_ok else 'FAIL'}")
    log(f"check: seam normals: the core's and the ring's differ by at most {numbers['normal_deg']:.3f} degrees (both "
        f"from the regional field's gradient) - {'PASS' if normal_ok else 'FAIL'}; each side's own geometric normal "
        f"is within {numbers['core_geometric_deg']:.2f} (core) and {numbers['ring_geometric_deg']:.2f} (ring) degrees "
        f"of it, {numbers['core_geometric_mean']:.2f} and {numbers['ring_geometric_mean']:.2f} on average, "
        f"{numbers['core_geometric_p95']:.2f} and {numbers['ring_geometric_p95']:.2f} for 95% of the seam (the most "
        f"at x {numbers['worst'][0]:.1f}, y {numbers['worst'][1]:.1f} m)")
    for kind, budget in BUDGETS.items():
        ok = totals.get(kind, 0) <= budget
        passed &= ok
        log(f'check: {kind} {totals.get(kind, 0)} triangles of {budget} - {"PASS" if ok else "FAIL"}')
    return passed


def main(name=None):
    """Builds an area's terrain (its layout, or --layout) and, with the flags, its computed layout, maps and
    previews. Returns the area."""
    args = options()
    path = args.layout or area_shape.layout_path(name)
    clear_scene()
    area = area_shape.Area(path)
    log(f"terrain: {area.name}, a {2.0 * area.half:g} m square: " +
        ', '.join(f'{raster} {n} ({area.cell(raster) * 100.0:.3g} cm)' for raster, n in area.sizes.items()))
    area.build(log=log)
    verts, tris, ring, tiles = build_top(area)
    underside = build_underside(area, verts[ring]) if area.setting == 'island' else []
    build_water(area)
    total = sum(c for _, c in tiles) + sum(len(o.data.polygons) for o in underside)
    log(f'terrain: {total} triangles in the tiles and the underside')
    if area.setting == 'grounded':
        made, numbers = build_beyond(area)
        area.checks_passed = report_grounded(area, tiles, made, numbers)
    save_to = os.path.abspath(args.save_to) if args.save_to else ''
    if save_to:
        os.makedirs(save_to, exist_ok=True)
    previews = (os.path.join(save_to, 'previews') if save_to else
                os.path.join(REPO, 'Saved', 'ArtPreviews', 'Terrain', area.name))
    if args.computed:
        import area_computed
        area_computed.write(area, os.path.join(save_to, 'layout_computed.json') if save_to else None, log=log)
    if args.macro:
        import area_macro
        import area_scatter
        out = save_to or os.path.dirname(macro_path(area))
        area.painted = {'core': os.path.join(out, os.path.basename(area.macro_texture))}
        if area.setting == 'grounded':
            # The ring's map first: the core's blends into it across the seam band.
            ring_out = save_to or os.path.dirname(ring_macro_path(area))
            area.painted['ring'] = os.path.join(ring_out, os.path.basename(area.ring_macro_texture))
            area_macro.paint_ring(area, area.painted['ring'], previews, log=log)
            macro_material(area, ring=True)
        area_macro.paint(area, area.painted['core'], previews, log=log)
        area_scatter.paint(area, os.path.join(out, os.path.basename(area.scatter_texture)), previews, log=log)
        macro_material(area)  # now with the texture
    if args.preview:
        import area_preview
        area_preview.render_all(area, previews, log=log)
    memory = peak_memory_mb()
    log(f'terrain: done in {time.time() - STARTED:.0f} s' +
        (f', peak memory {memory[0]:.0f} MB (working set), {memory[1]:.0f} MB (private)' if memory else ''))
    return area


if __name__ == '__main__':
    AREA = main()
