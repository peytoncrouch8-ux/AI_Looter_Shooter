"""Exports the game's scripted models as flat-colored GLBs for the island concept viewer (Screen Print Wash palette
baked into vertex colors: alpha 1 = a normal fill, 0.5 = a saturated accent, 0 = a glow).
Runs under plain Python with the `bpy` module (pip install bpy==4.5.*, numpy, pillow), not inside Blender.
Usage: python3 web_export.py <outdir> [models|terrain|all] [Art/Models-relative scripts, comma-separated]"""
import colorsys
import json
import math
import os
import runpy
import sys
import time
import traceback

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
for p in (os.path.join(REPO, 'Tools', 'Blender'), os.path.join(REPO, 'Art', 'Levels')):
    sys.path.insert(0, p)
OUT = sys.argv[1]
WHAT = sys.argv[2] if len(sys.argv) > 2 else 'all'
ONLY = sys.argv[3].split(',') if len(sys.argv) > 3 else None
os.makedirs(os.path.join(OUT, 'models'), exist_ok=True)

import bpy  # noqa: E402
import numpy as np  # noqa: E402
from mathutils import Vector  # noqa: E402
from mathutils.bvhtree import BVHTree  # noqa: E402
from PIL import Image  # noqa: E402

import looter_textures as lt  # noqa: E402
RYE = os.path.join(REPO, 'Art', 'Fonts', 'Rye-Regular.ttf')
for _kit in ('looter_rail', 'looter_train', 'looter_town'):
    try:
        _m = __import__(_kit)
        for _attr in ('RYE', 'FONT'):
            if hasattr(_m, _attr):
                setattr(_m, _attr, RYE)
        if hasattr(_m, 'FALLBACK_FONT'):
            _m.FALLBACK_FONT = '/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf'
    except Exception as _e:
        print(f'EXPORT: could not patch {_kit}: {_e}', flush=True)

CREAM = (0xf6 / 255, 0xee / 255, 0xdc / 255)
SKIP_PREFIX = ('UCX_', 'USP_', 'UCP_', '_')

# --- the palette: average colors of the trim sheet's strips and of every texture set, lifted by the print rule ---
def texture_mean(path, alpha=False):
    a = np.asarray(Image.open(path).convert('RGBA'), np.float32) / 255.0
    m = a[..., 3] > 0.5 if alpha else np.ones(a.shape[:2], bool)
    if not m.any():
        m[:] = True
    return tuple(a[..., :3][m].mean(0))


def strip_means():
    im = np.asarray(Image.open(lt.texture_path('HouseTrim', 'BC')).convert('RGB'), np.float32) / 255.0
    h = im.shape[0]
    out = {}
    for key, (v0, v1) in lt.TRIM_STRIPS.items():
        r0, r1 = int(round((1 - v1) * h)), int(round((1 - v0) * h))
        out[key] = tuple(im[r0:r1].reshape(-1, 3).mean(0))
    return out


TRIM = strip_means()
SET_MEAN = {}
for set_name in list(lt.SETS) + ['TutorialIslandMacro']:
    path = lt.texture_path(set_name, 'BC')
    if os.path.exists(path):
        SET_MEAN[set_name] = texture_mean(path, alpha=bool(lt.SETS.get(set_name, {}).get('alpha')))
    elif set_name in lt.SETS:
        SET_MEAN[set_name] = tuple(c for c in lt.rgb(lt.SETS[set_name]['color']))


def lin_to_srgb(c):
    return tuple(12.92 * v if v <= 0.0031308 else 1.055 * v ** (1 / 2.4) - 0.055 for v in c)


def hsv_adjust(c, sat=1.0, val=1.0):
    h, s, v = colorsys.rgb_to_hsv(*c)
    return colorsys.hsv_to_rgb(h, min(1.0, s * sat), min(1.0, v * val))


def print_lift(c):
    """The Screen Print Wash fill rule on an sRGB color (Docs/Art/ScreenPrintWash.md). Returns (rgb, accent)."""
    accent = min(c[1], c[2]) - c[0] > 0.05
    if accent:
        c = hsv_adjust(c, sat=1.7, val=1.12)
    c = hsv_adjust(c, sat=1.22)
    c = tuple(min(1.0, 0.83 * a + 0.17 * b) for a, b in zip(c, CREAM))
    return c, accent


def strip_for_v(v):
    v = min(max(v, 0.0), 0.99999)
    for key, (v0, v1) in lt.TRIM_STRIPS.items():
        if v0 <= v < v1:
            return key
    return 'A'


def material_color(mat, v_mean):
    """(sRGB rgb, alpha flag) for a face of this material: the set's or strip's average, tinted, print-lifted."""
    if mat is None:
        return print_lift((0.6, 0.6, 0.6))[0], 1.0
    set_name = mat.get('TextureSet')
    glow = float(mat.get('Glow', 0.0) or 0.0) > 0.0 or mat.get('Kind') == 'Glow' or 'Glow' in mat.name
    if set_name == 'HouseTrim':
        base = TRIM[strip_for_v(v_mean)]
    elif set_name and set_name in SET_MEAN:
        base = SET_MEAN[set_name]
    else:
        base = lin_to_srgb(tuple(mat.diffuse_color)[:3])
    tint = mat.get('Tint')
    if tint:
        t = lt.rgb(lt._hex_int(tint))
        base = tuple(min(1.0, b * tt) for b, tt in zip(base, t))
    if glow:
        c = hsv_adjust(base, sat=1.3, val=1.1)
        return c, 0.0
    c, accent = print_lift(base)
    return c, (0.5 if accent else 1.0)


def bake_flat_colors(obj):
    mesh = obj.data
    uv = mesh.uv_layers.active
    uv_data = None
    if uv is not None:
        uv_data = np.empty(len(mesh.loops) * 2, np.float32)
        uv.data.foreach_get('uv', uv_data)
        uv_data = uv_data.reshape(-1, 2)
    # Only now add the attribute (adding one moves the mesh's layers; a handle taken before it reads stale data).
    attr = mesh.color_attributes.get('Flat') or mesh.color_attributes.new('Flat', 'BYTE_COLOR', 'CORNER')
    colors = np.empty((len(mesh.loops), 4), np.float32)
    cache = {}
    for poly in mesh.polygons:
        mat = mesh.materials[poly.material_index] if poly.material_index < len(mesh.materials) else None
        loops = range(poly.loop_start, poly.loop_start + poly.loop_total)
        v_mean = float(uv_data[list(loops), 1].mean()) if uv_data is not None else 0.5
        key = (poly.material_index, strip_for_v(v_mean) if (mat is not None and mat.get('TextureSet') == 'HouseTrim') else None)
        if key not in cache:
            rgb, a = material_color(mat, v_mean)
            cache[key] = (*rgb, a)
        colors[poly.loop_start:poly.loop_start + poly.loop_total] = cache[key]
    attr.data.foreach_set('color_srgb', colors.reshape(-1))
    mesh.color_attributes.active_color = attr
    mesh.color_attributes.render_color_index = mesh.color_attributes.find('Flat')


def sockets_of(root):
    from mathutils import Matrix
    M = Matrix(((1, 0, 0), (0, 0, 1), (0, -1, 0)))
    inv = root.matrix_world.inverted()
    out = {}
    for o in root.children_recursive:
        if o.type == 'EMPTY' and o.name.startswith('SOCKET_'):
            name = o.name[len('SOCKET_'):].split('.')[0]
            mw = inv @ o.matrix_world
            p = mw.to_translation()
            r = (M @ mw.to_3x3().normalized() @ M.transposed()).to_quaternion()
            out.setdefault(name, []).append(dict(pos=[round(p.x, 4), round(p.z, 4), round(-p.y, 4)],
                                                 quat=[round(r.x, 5), round(r.y, 5), round(r.z, 5), round(r.w, 5)]))
    return out


def render_meshes(root):
    out = []
    for o in [root] + list(root.children_recursive):
        if o.type == 'MESH' and not o.name.startswith(SKIP_PREFIX) and len(o.data.polygons):
            out.append(o)
    return out


def export_objects(objs, path):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.export_scene.gltf(filepath=path, export_format='GLB', use_selection=True, export_apply=True,
                              export_materials='NONE', export_vertex_color='ACTIVE',
                              export_active_vertex_color_when_no_material=True, export_all_vertex_colors=False,
                              export_normals=True, export_texcoords=False, export_skins=False,
                              export_animations=False, export_yup=True, export_extras=False)


def bounds(objs):
    pts = [o.matrix_world @ Vector(c) for o in objs for c in o.bound_box]
    lo = [min(p[i] for p in pts) for i in range(3)]
    hi = [max(p[i] for p in pts) for i in range(3)]
    # Blender (x, y, z) -> three.js (x, z, -y)
    return dict(min=[lo[0], lo[2], -hi[1]], max=[hi[0], hi[2], -lo[1]])


MANIFEST = dict(models={}, terrain={}, palette={k: lin_to_srgb((0, 0, 0)) for k in ()})
MANIFEST['palette'] = {'trim_' + k: print_lift(v)[0] for k, v in TRIM.items()}
MANIFEST['palette'].update({k: print_lift(v)[0] for k, v in SET_MEAN.items()})
MANIFEST['cream'] = CREAM

DECIMATE = {'Rocks/Cliffs.py': 0.25, 'Rocks/Outcrops.py': 0.35, 'Rocks/SkyIslands.py': 0.6}

SOURCES = [
    'Buildings/Barn.py', 'Buildings/Bridge.py', 'Buildings/Cottage.py', 'Buildings/Farmhouse.py', 'Buildings/GunRack.py',
    'Buildings/LogCabin.py', 'Buildings/LookoutTower.py', 'Buildings/Outhouse.py', 'Buildings/Well.py',
    'Buildings/Windmill.py', 'Buildings/FalseFronts.py', 'Buildings/Depot.py',
    'Props/Fences.py', 'Props/VillageProps.py', 'Props/Containers.py', 'Props/FarmProps.py', 'Props/LanternPost.py',
    'Props/SkiffJetty.py', 'Props/Boardwalk.py', 'Props/Ruins.py',
    'Rocks/Cliffs.py', 'Rocks/Rocks.py', 'Rocks/Outcrops.py', 'Rocks/SkyIslands.py',
    'Vegetation/DeadTree.py', 'Vegetation/Pond.py',
    'Creatures/Spider.py', 'Creatures/Slime.py', 'Vehicles/Skiff.py',
]


def export_models():
    for rel in (ONLY or SOURCES):
        path = os.path.join(REPO, 'Art', 'Models', rel)
        t = time.time()
        bpy.ops.wm.read_factory_settings(use_empty=True)
        sys.argv = ['blender', '--', '--no-ao']
        try:
            runpy.run_path(path, run_name='__main__')
        except Exception:
            print(f'EXPORT: FAILED to build {rel}\n{traceback.format_exc()}', flush=True)
            continue
        roots = [o for o in bpy.context.scene.objects if o.parent is None and o.type in ('MESH', 'ARMATURE')
                 and not o.name.startswith(SKIP_PREFIX)]
        for root in roots:
            meshes = render_meshes(root)
            if not meshes:
                continue
            for m in meshes:
                if m.data.users > 1:
                    m.data = m.data.copy()
                bake_flat_colors(m)
                if rel in DECIMATE and len(m.data.polygons) > 2000:
                    mod = m.modifiers.new('Decimate', 'DECIMATE')
                    mod.ratio = DECIMATE[rel]
            name = root.name if root.name != 'root' else os.path.splitext(os.path.basename(rel))[0]
            glb = os.path.join(OUT, 'models', f'{name}.glb')
            try:
                export_objects(meshes, glb)
            except Exception:
                print(f'EXPORT: FAILED to export {name}\n{traceback.format_exc()}', flush=True)
                continue
            tris = sum(len(p.vertices) - 2 for m in meshes for p in m.data.polygons)
            MANIFEST['models'][name] = dict(file=f'models/{name}.glb', source=rel, tris=tris, bounds=bounds(meshes),
                                            size=os.path.getsize(glb), sockets=sockets_of(root))
            print(f'EXPORT: {name:28s} {tris:7d} tris {os.path.getsize(glb) / 1024:7.0f} KB  from {rel}', flush=True)
        print(f'EXPORT: {rel} done in {time.time() - t:.1f} s', flush=True)


def export_terrain():
    blend = os.path.join(os.path.dirname(OUT), 'terrain.blend')
    bpy.ops.wm.open_mainfile(filepath=blend)
    scene = bpy.context.scene
    tiles = [o for o in scene.objects if o.type == 'MESH' and '_Tile_' in o.name]
    under = [o for o in scene.objects if o.type == 'MESH' and '_Underside_' in o.name]
    water = [o for o in scene.objects if o.type == 'MESH' and o.name.endswith('_Water')]
    print(f'EXPORT: terrain tiles {len(tiles)} underside {len(under)} water {len(water)}', flush=True)

    # Colors from the macro map, quantized to a few flat tones, per face corner.
    macro_rgba = np.asarray(Image.open(lt.texture_path('TutorialIslandMacro', 'BC')).convert('RGBA'), np.float32) / 255.0
    import cv2
    macro = cv2.GaussianBlur(np.ascontiguousarray(macro_rgba[..., :3]), (0, 0), 7.0)
    macro_alpha = macro_rgba[..., 3]
    mh, mw = macro.shape[:2]
    # k-means on a sample of the macro map for the palette
    sample = macro.reshape(-1, 3)[::37].astype(np.float32)
    crit = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 40, 0.25)
    _, _, centers = cv2.kmeans(sample, 4, None, crit, 4, cv2.KMEANS_PP_CENTERS)
    lifted = [print_lift(tuple(c))[0] for c in centers]
    MANIFEST['terrain']['palette'] = lifted
    grass_k = int(np.argmax(centers[:, 1] - centers[:, 0]))   # the greenest tone
    print(f'EXPORT: terrain palette {[tuple(round(v, 3) for v in c) for c in lifted]} grass {grass_k}', flush=True)
    for o in tiles:
        mesh = o.data
        uv = mesh.uv_layers[0]
        uv_data = np.empty(len(mesh.loops) * 2, np.float32)
        uv.data.foreach_get('uv', uv_data)
        uv_data = uv_data.reshape(-1, 2)
        attr = mesh.color_attributes.get('Flat') or mesh.color_attributes.new('Flat', 'BYTE_COLOR', 'CORNER')
        colors = np.empty((len(mesh.loops), 4), np.float32)
        colors[:, 3] = 1.0
        for poly in mesh.polygons:
            ls = slice(poly.loop_start, poly.loop_start + poly.loop_total)
            u, v = uv_data[ls].mean(0)
            px = min(max(int(u * mw), 0), mw - 1)
            py = min(max(int((1 - v) * mh), 0), mh - 1)
            c = macro[py, px]
            k = int(((centers - c) ** 2).sum(1).argmin())
            if 0.18 <= macro_alpha[py, px] <= 0.52:   # a painted road or footpath: the concepts lay their own
                k = grass_k
            colors[ls, :3] = lifted[k]
        attr.data.foreach_set('color_srgb', colors.reshape(-1))
        mesh.color_attributes.active_color = attr
        mesh.color_attributes.render_color_index = mesh.color_attributes.find('Flat')
        mod = o.modifiers.new('Decimate', 'DECIMATE')
        mod.ratio = 0.45
    for o in under:
        mesh = o.data
        attr = mesh.color_attributes.get('Flat') or mesh.color_attributes.new('Flat', 'BYTE_COLOR', 'CORNER')
        c = print_lift(SET_MEAN['RockCliff'])[0]
        colors = np.tile(np.array([*c, 1.0], np.float32), (len(mesh.loops), 1))
        attr.data.foreach_set('color_srgb', colors.reshape(-1))
        mesh.color_attributes.active_color = attr
        mesh.color_attributes.render_color_index = mesh.color_attributes.find('Flat')
        mod = o.modifiers.new('Decimate', 'DECIMATE')
        mod.ratio = 0.3
    for o in water:
        mesh = o.data
        attr = mesh.color_attributes.get('Flat') or mesh.color_attributes.new('Flat', 'BYTE_COLOR', 'CORNER')
        c = print_lift((0.55, 0.72, 0.76))[0]
        colors = np.tile(np.array([*c, 0.5], np.float32), (len(mesh.loops), 1))
        attr.data.foreach_set('color_srgb', colors.reshape(-1))
        mesh.color_attributes.active_color = attr
        mesh.color_attributes.render_color_index = mesh.color_attributes.find('Flat')
    for o in tiles + under:
        for poly in o.data.polygons:
            poly.use_smooth = True
    # Heights for the viewer, from the decimated tiles the viewer draws: a grid over the map square, in three.js coordinates (x = x_b, z = -y_b), meters.
    depsgraph = bpy.context.evaluated_depsgraph_get()
    verts, polys = [], []
    for o in tiles:
        ev = o.evaluated_get(depsgraph)
        me = ev.to_mesh()
        base = len(verts)
        verts.extend(o.matrix_world @ v.co for v in me.vertices)
        polys.extend([base + i for i in p.vertices] for p in me.polygons)
        ev.to_mesh_clear()
    tree = BVHTree.FromPolygons(verts, polys, epsilon=0.0)
    N = 256
    half = 102.4
    heights = np.full((N, N), -999.0, np.float32)
    for r in range(N):
        z3 = -half + (r + 0.5) * (2 * half / N)
        for c in range(N):
            x3 = -half + (c + 0.5) * (2 * half / N)
            hit = tree.ray_cast(Vector((x3, -z3, 200.0)), Vector((0, 0, -1)), 400.0)
            if hit[0] is not None:
                heights[r, c] = hit[0].z
    heights.tofile(os.path.join(OUT, 'heights.bin'))
    MANIFEST['terrain']['heights'] = dict(file='heights.bin', n=N, half=half, missing=-999.0)
    print(f'EXPORT: heights grid {N}x{N}, {np.isfinite(heights).sum()} cells, z {heights[heights > -900].min():.1f}..{heights.max():.1f}', flush=True)

    for name, objs in (('Terrain', tiles), ('Underside', under), ('Water', water)):
        if not objs:
            continue
        glb = os.path.join(OUT, 'models', f'{name}.glb')
        export_objects(objs, glb)
        MANIFEST['terrain'][name.lower()] = dict(file=f'models/{name}.glb', size=os.path.getsize(glb))
        print(f'EXPORT: {name} -> {os.path.getsize(glb) / 1024:.0f} KB', flush=True)


manifest_path = os.path.join(OUT, 'manifest.json')
if os.path.exists(manifest_path):
    MANIFEST.update({k: v for k, v in json.load(open(manifest_path)).items() if k in ('models', 'terrain')})
if WHAT in ('models', 'all'):
    export_models()
if WHAT in ('terrain', 'all'):
    export_terrain()
with open(manifest_path, 'w') as f:
    json.dump(MANIFEST, f, indent=1)
print('EXPORT: done', flush=True)
