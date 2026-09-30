"""The tutorial island's terrain (Docs/TutorialIsland.md), built from Art/Levels/TutorialIsland/layout.json by the
generator next to it (island_shape.py, island_mesh.py). A scripted model (Art/README.md) with several models:

- TutorialIsland_Tile_<i>_<j>: the walkable top in a 4 x 4 grid of tiles (i from south to north, j from west to east;
  tiles outside the island are left out). Every tile has its origin at the world origin, so all line up when placed
  at the origin. Material TutorialIslandMacro (master Terrain): UV 0 maps the whole island onto the macro color map
  T_TutorialIslandMacro_BC (U = (Y + 10240) / 20480, V = (X + 10240) / 20480, Unreal cm), UV 1 is world meters
  (U = Y / 100, V = X / 100) for the detail textures, and vertex color alpha is baked ambient occlusion (RGB white).
- TutorialIsland_Underside_<n>: the rock under the rim (RockCliff), in four sectors.
- TutorialIsland_Water: the pond and the creek's surface (material Water, master Water; UV 0 in world meters).

    blender -b --factory-startup --python Art/Models/Terrain/TutorialIsland.py -- [--macro] [--computed] [--preview]

  --macro     paints the macro color map and the PCG scatter mask (T_TutorialIslandScatter_BC: tree, grass, flower
              and pebble densities) into Art/Textures/TutorialIslandMacro (a couple of minutes)
  --computed  writes Art/Levels/TutorialIsland/layout_computed.json (heights and points the placement script needs)
  --preview   renders Saved/ArtPreviews/Terrain (macro.png, plan.png and three views)
models.ps1 runs it without flags: the meshes only.
"""
import argparse
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
for path in (os.path.join(REPO, 'Art', 'Levels', 'TutorialIsland'), os.path.join(REPO, 'Tools', 'Blender')):
    if path not in sys.path:
        sys.path.append(path)

import bpy  # noqa: E402
import numpy as np  # noqa: E402

import island_mesh  # noqa: E402
import island_shape  # noqa: E402
import looter_textures as lt  # noqa: E402
from island_math import MAP_HALF, sample  # noqa: E402

MACRO_DIR = os.path.join(REPO, 'Art', 'Textures', 'TutorialIslandMacro')
MACRO_BC = os.path.join(MACRO_DIR, 'T_TutorialIslandMacro_BC.png')
SCATTER = os.path.join(MACRO_DIR, 'T_TutorialIslandScatter_BC.png')  # PCG densities (island_scatter.py)
DETAIL_SETS = ('GroundGrass', 'RockCliff')  # M_Terrain's detail textures: grass/soil (macro alpha 0), rock (alpha 1)
TILES = 4
TOP_TRIANGLES = 118000
UNDERSIDE_REPEAT = 9.6  # meters per RockCliff repeat under the island: a third of the usual density, seen from afar
STARTED = time.time()


def log(message):
    print(f'LOOTER: [{time.time() - STARTED:5.0f} s] {message}', flush=True)


def options():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    parser = argparse.ArgumentParser()
    for flag in ('--macro', '--computed', '--preview'):
        parser.add_argument(flag, action='store_true')
    return parser.parse_known_args(argv)[0]


def clear_scene():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj)


# --- Materials ---

def sock(sockets, identifier):
    """A node socket by identifier (the Mix node has an A and a B per data type, all with the same name)."""
    return next(s for s in sockets if s.identifier == identifier)


def macro_material():
    """The top's material: M_Terrain's macro map and detail sets. Its Blender nodes preview what M_Terrain does."""
    mat = bpy.data.materials.get('TutorialIslandMacro') or bpy.data.materials.new('TutorialIslandMacro')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Roughness'].default_value = 0.9
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x6b7a3e)
    mat.diffuse_color = lt.hex_color(0x6b7a3e)
    if os.path.exists(MACRO_BC):
        uv0 = nodes.new('ShaderNodeUVMap')
        uv0.uv_map = 'UVMap'
        macro = nodes.new('ShaderNodeTexImage')
        macro.image = bpy.data.images.load(MACRO_BC, check_existing=True)
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
    mat['TextureSet'] = 'TutorialIslandMacro'
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
    vb = island_mesh.to_blender(verts)
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


def build_top(island):
    verts, tris, ring = island_mesh.top_surface(island, TOP_TRIANGLES, log=log)
    log(f'terrain: top surface {len(verts)} vertices, {len(tris)} triangles')
    normals = island_mesh.vertex_normals(island_mesh.to_blender(verts), tris)
    if np.mean(normals[:, 2]) < 0.0:  # the triangulation runs counter-clockwise; make sure the top faces up
        tris = tris[:, [0, 2, 1]]
        normals = -normals
    ao_raster = island_mesh.terrain_ao(island)
    island.ao = ao_raster
    ao = sample(ao_raster, verts[:, 0], verts[:, 1])
    colors = np.column_stack([np.ones((len(verts), 3)), np.clip(ao, 0.0, 1.0)])
    macro_uv = np.column_stack([(verts[:, 1] + MAP_HALF), (verts[:, 0] + MAP_HALF)]) / (2.0 * MAP_HALF)
    detail_uv = np.column_stack([verts[:, 1], verts[:, 0]])
    mat = macro_material()

    # Tiles: a 4 x 4 grid over the island's extent, each triangle going with the tile its center falls in.
    lo, hi = verts[:, :2].min(axis=0), verts[:, :2].max(axis=0)
    center = verts[tris].mean(axis=1)
    ti = np.minimum(((center[:, 0] - lo[0]) / (hi[0] - lo[0]) * TILES).astype(int), TILES - 1)
    tj = np.minimum(((center[:, 1] - lo[1]) / (hi[1] - lo[1]) * TILES).astype(int), TILES - 1)
    tiles = []
    for i in range(TILES):
        for j in range(TILES):
            chosen = tris[(ti == i) & (tj == j)]
            if len(chosen) == 0:
                continue
            used, inverse = np.unique(chosen.ravel(), return_inverse=True)
            local = inverse.reshape(-1, 3)
            obj = make_object(f'TutorialIsland_Tile_{i}_{j}', verts[used], local, mat,
                              uvs={'UVMap': macro_uv[used], 'UVDetail': detail_uv[used]},
                              normals=normals[used], colors=colors[used])
            tiles.append((obj.name, len(local)))
    log('terrain: tiles ' + ', '.join(f'{n[len("TutorialIsland_Tile_"):]} {c}' for n, c in tiles))
    return verts, tris, ring, tiles


def build_underside(island, ring_xyz):
    rim_drop = island.layout['island']['rimDrop'] / 100.0
    depth = island.layout['island']['undersideDepth'] / 100.0
    pieces = island_mesh.underside(island, ring_xyz, rim_drop, depth)
    # The same shared instance as the cliff kit's, with its moss (Art/Models/Rocks).
    rock = lt.material('RockCliff', MossAmount=0.6)
    made = []
    for n, (verts, tris, extra) in enumerate(pieces):
        obj = make_object(f'TutorialIsland_Underside_{n}', verts, tris, rock,
                          uvs={'UVMap': _wrap_uv(verts, tris, extra[:, :2])}, normals=extra[:, 2:5])
        made.append(obj)
    for obj in made:
        _underside_ao(obj, rim_drop, depth)
    log('terrain: underside ' + ', '.join(f'{o.name[len("TutorialIsland_"):]} {len(o.data.polygons)}' for o in made))
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


def _underside_ao(obj, rim_drop, depth):
    """Baked occlusion (its own crevices and spires), darkening toward the bottom: the sky light reaches the
    underside mostly from the side."""
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
    down = np.clip(-(z + rim_drop) / (depth - rim_drop), 0.0, 1.0)
    raw[:, :3] = 1.0
    raw[:, 3] = np.clip(raw[:, 3] * (1.0 - 0.3 * down), 0.0, 1.0)
    col.data.foreach_set('color_srgb', raw.ravel())
    mesh.color_attributes.active_color = col


def build_water(island):
    verts, tris = island_mesh.water(island)
    uv = np.column_stack([verts[:, 1], verts[:, 0]])
    normals = np.tile([0.0, 0.0, 1.0], (len(verts), 1))
    obj = make_object('TutorialIsland_Water', verts, tris, water_material(), uvs={'UVMap': uv}, normals=normals)
    # Players wade through the pond and the creek: the water has no collision (their beds do).
    obj['Collision'] = 'None'
    vb = island_mesh.to_blender(verts)
    a, b, c = vb[tris[:, 0]], vb[tris[:, 1]], vb[tris[:, 2]]
    if np.mean(np.cross(b - a, c - a)[:, 2]) < 0.0:
        obj.data.flip_normals()
        obj.data.normals_split_custom_set_from_vertices(normals.tolist())
    log(f'terrain: water {len(tris)} triangles')
    return obj


def main():
    args = options()
    clear_scene()
    island = island_shape.Island().build(log=log)
    verts, tris, ring, tiles = build_top(island)
    build_underside(island, verts[ring])
    build_water(island)
    total = sum(c for _, c in tiles) + sum(len(o.data.polygons) for o in bpy.data.objects
                                           if o.name.startswith('TutorialIsland_Underside'))
    log(f'terrain: {total} triangles in the tiles and the underside')
    if args.computed:
        import island_computed
        island_computed.write(island, log=log)
    if args.macro:
        import island_macro
        island_macro.paint(island, MACRO_BC, log=log)
        import island_scatter
        island_scatter.paint(island, SCATTER, log=log)
        macro_material()  # now with the texture
    if args.preview:
        import island_preview
        island_preview.render_all(island, REPO, MACRO_BC, log=log)
    return island


ISLAND = main()
