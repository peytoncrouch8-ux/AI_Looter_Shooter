"""The cliff faces' Nanite fallback, before and after a cut (the LODS lane, 2026-11-04: Docs/Work/2026-11-04-Art/LODS.md).
Medium draws a Nanite mesh's fallback at every distance (r.Nanite 0, no LOD chain), so a face's fallback share is all
the detail Medium ever shows of it. This builds the kit with Art/Models/Rocks/Cliffs.py (the same meshes an export
makes, at the same thread count through Tools\\artrun.ps1), cuts each CliffFace_* source to every share asked for with
Blender's Decimate (the collapse Cliffs.py uses to reach its own source: a stand-in for Nanite's simplifier, which
Unreal runs on import) and renders it in the Medium look: Eevee, a hard afternoon sun with shadows, the textured
RockCliff preview material and its baked vertex AO, no other occlusion; the game's 1080p frame with the tour's 80
degree view, from 20, 60 and 150 m, each render cropped to the piece at its real pixel size.

Writes Saved/ArtPreviews/Lods/:
  <piece>_<distance>m_<share>.png   the crops; share 100 is the source (what Nanite draws on High and Epic)
  report.json                        per piece: triangles at each share; the cut's distance from the source above the
                                     ground (cm: mean, p95, max, both ways); per distance the share of the crop's pixels
                                     that change from the source's (any channel by more than 0.08) and the mean change
  CliffFallback_sheet.jpg            the three pieces whose last share changes most at 20 m: the first share (before)
                                     beside the last (after) at each distance, the far crops enlarged (nearest pixel)

    Tools\\artrun.ps1 -Script Tools\\Blender\\cliff_fallback_sheet.py [-ScriptArgs 25,12]

Shares are percents of the source (default 25, 12: before, after). Collision isn't drawn: the faces' UCX hulls are
their simple collision and don't change; complex traces (bullets, the spiders' climbing) use the fallback, so the
report's deviation is also how far a bullet's mark or a spider's foot can sit off the rock Nanite draws on High.
"""
import json
import math
import os
import runpy
import sys

import bmesh
import bpy
import numpy as np
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_homes as homes
import looter_model as lm
import looter_textures as lt

OUT = os.path.join(lt.PREVIEW_DIR, 'Lods')
CLIFFS = os.path.join(lt.REPO, 'Art', 'Models', 'Rocks', 'Cliffs.py')
DISTANCES = (20.0, 60.0, 150.0)
# The eye over the piece's ground at each distance: a player near its foot; the valley's points and the bluff path;
# the aerial and PointOverValley's height over the far walls.
EYE = {20.0: 1.7, 60.0: 12.0, 150.0: 50.0}
AZIMUTH = 25.0          # degrees off the face's front, to the left (its relief and one end show)
RES = (1920, 1080)
FOV = 80.0              # horizontal, the tour's views
SAMPLES = 16
CHANGED = 0.08          # a pixel counts as changed when a channel moves more than this (0..1, display-referred)
SHEET_PIECES = 3
TILE_IMAGE_W = 660      # each half of a sheet tile, near this wide after a whole-pixel enlargement


def args():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    tokens = [t for a in argv for t in a.split(',') if t]
    shares = [float(t) for t in tokens if t.replace('.', '', 1).isdigit()]
    pieces = [t for t in tokens if t.startswith('CliffFace')]
    return shares or [25.0, 12.0], pieces


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def cut(obj, share, density, material_seed):
    """A copy of obj reduced to share percent of its triangles (Decimate collapse, triangulated), sharp edges from the
    same 55 degrees Cliffs.py uses, and its box UVs projected again as Cliffs.py projects the source's (same density and
    seed: the same texture at the same place). Decimate collapses across UV seams and smeared the texture in streaks
    there; Nanite's simplifier keeps the seams, so the projection stands closer to what Unreal's fallback draws."""
    copy = obj.copy()
    copy.data = obj.data.copy()
    copy.name = f'{obj.name}_{share:g}'
    copy.parent = None
    bpy.context.scene.collection.objects.link(copy)
    mod = copy.modifiers.new('Fallback', 'DECIMATE')
    mod.ratio = share / 100.0
    mod.use_collapse_triangulate = True
    with bpy.context.temp_override(object=copy, active_object=copy, selected_objects=[copy],
                                   selected_editable_objects=[copy]):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    lm.smooth(copy, 55.0)
    lt.box_uv(copy, 'RockCliff', texel_density=density, seed=material_seed)
    return copy


def tree(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    result = BVHTree.FromBMesh(bm)
    bm.free()
    return result


def deviation(source, low):
    """How far the cut lies from the source above the ground (cm), both ways: source vertices to the cut's surface and
    the cut's vertices to the source's."""
    d = []
    for a, b in ((source, low), (low, source)):
        t = tree(b)
        d += [t.find_nearest(v.co)[3] for v in a.data.vertices if v.co.z >= 0.0]
    d = np.array(d) * 100.0
    return dict(mean=round(float(d.mean()), 2), p95=round(float(np.percentile(d, 95)), 2), max=round(float(d.max()), 2))


def camera_for(obj, distance):
    """The camera distance metres from the middle of the face's front, at the eye height for it, AZIMUTH to the left."""
    lo = Vector([min(c[i] for c in obj.bound_box) for i in range(3)])
    hi = Vector([max(c[i] for c in obj.bound_box) for i in range(3)])
    target = Vector((0.0, lo.y, max(hi.z, 1.0) * 0.45))
    eye = EYE[distance]
    flat = math.sqrt(max(distance ** 2 - (eye - target.z) ** 2, 1.0))
    a = math.radians(AZIMUTH)
    location = Vector((target.x - flat * math.sin(a), target.y - flat * math.cos(a), eye))
    cam = bpy.data.objects.new(f'_LodCam_{distance:g}', bpy.data.cameras.new(f'_LodCam_{distance:g}'))
    cam.data.sensor_fit = 'HORIZONTAL'
    cam.data.sensor_width = 36.0
    cam.data.lens = 18.0 / math.tan(math.radians(FOV * 0.5))
    cam.data.clip_start = 0.1
    cam.data.clip_end = 5000.0
    cam.location = location
    cam.rotation_euler = (target - location).to_track_quat('-Z', 'Y').to_euler()
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.view_layer.update()     # its matrix_world, for the crop's projection
    return cam


def border(scene, cam, obj):
    """The render region around obj's part above the ground (normalized, 0 at the bottom), a little margin round it."""
    points = [world_to_camera_view(scene, cam, obj.matrix_world @ v.co) for v in obj.data.vertices if v.co.z >= 0.0]
    x0, x1 = min(p.x for p in points), max(p.x for p in points)
    y0, y1 = min(p.y for p in points), max(p.y for p in points)
    mx, my = (x1 - x0) * 0.06 + 6.0 / RES[0], (y1 - y0) * 0.06 + 6.0 / RES[1]
    return max(x0 - mx, 0.0), min(x1 + mx, 1.0), max(y0 - my, 0.0), min(y1 + my, 1.0)


def setup(scene):
    world = lt._preview_world(scene)
    scene.world = world
    sun = bpy.data.objects.new('_LodSun', bpy.data.lights.new('_LodSun', 'SUN'))
    sun.data.energy = 4.0
    sun.data.color = (1.0, 0.9, 0.78)
    sun.data.angle = math.radians(2.0)
    # From the front left of the views, 45 degrees up: the face's front lit, its joints and ledges shadowed.
    toward = Vector((-0.55, -0.7, 0.0)).normalized() * math.cos(math.radians(45.0)) + Vector((0, 0, math.sin(math.radians(45.0))))
    sun.rotation_euler = (-toward).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    size = 4000.0
    mesh = bpy.data.meshes.new('_LodGround')
    mesh.from_pydata([(-size, -size, 0.0), (size, -size, 0.0), (size, size, 0.0), (-size, size, 0.0)], [], [(0, 1, 2, 3)])
    mat = bpy.data.materials.new('_LodGround')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0x77766c)
    bsdf.inputs['Roughness'].default_value = 1.0
    mesh.materials.append(mat)
    ground = bpy.data.objects.new('_LodGround', mesh)
    scene.collection.objects.link(ground)
    scene.render.resolution_x, scene.render.resolution_y = RES
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.render.use_border = True
    scene.render.use_crop_to_border = True
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    try:
        scene.render.engine = 'BLENDER_EEVEE_NEXT'
        scene.eevee.taa_render_samples = SAMPLES
    except TypeError:
        scene.render.engine = 'BLENDER_EEVEE'
        scene.eevee.taa_render_samples = SAMPLES
    return sun, ground


def render(scene, cam, shown, region, path):
    for o in scene.objects:
        o.hide_render = o not in shown
    scene.camera = cam
    scene.render.border_min_x, scene.render.border_max_x, scene.render.border_min_y, scene.render.border_max_y = region
    scene.render.filepath = path
    try:
        bpy.ops.render.render(write_still=True)
    except Exception as error:  # no GPU context: Workbench, as lt.preview does
        lt._log(f'cliff sheet: Eevee failed ({error}); rendering with Workbench')
        scene.render.engine = 'BLENDER_WORKBENCH'
        shading = scene.display.shading
        shading.light, shading.color_type, shading.show_shadows, shading.show_cavity = 'STUDIO', 'TEXTURE', True, True
        bpy.ops.render.render(write_still=True)


def load(path):
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = 'Non-Color'
    w, h = img.size
    a = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4)       # rows bottom first


def save(a, path):
    img = bpy.data.images.new('_tile', a.shape[1], a.shape[0], alpha=True)
    img.colorspace_settings.name = 'Non-Color'
    img.pixels.foreach_set(np.ascontiguousarray(a, dtype=np.float32).ravel())
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)


def change(a, b):
    d = np.abs(a[..., :3] - b[..., :3]).max(axis=-1)
    return dict(changed=round(float((d > CHANGED).mean()) * 100.0, 2), mean=round(float(d.mean()) * 255.0, 2))


def pair(left, right):
    """Two crops side by side, enlarged by the same whole factor (nearest pixel) to about TILE_IMAGE_W each."""
    a, b = load(left), load(right)
    k = max(1, int(round(TILE_IMAGE_W / a.shape[1])))
    a, b = (np.repeat(np.repeat(x, k, axis=0), k, axis=1) for x in (a, b))
    gap = np.zeros((a.shape[0], 14, 4), np.float32)
    gap[..., :3] = np.array([0x16, 0x17, 0x1a], np.float32) / 255.0
    gap[..., 3] = 1.0
    return np.concatenate([a, gap, b], axis=1), k


def main():
    shares, only = args()
    os.makedirs(OUT, exist_ok=True)
    sys.argv = [sys.argv[0]]        # Cliffs.py reads --preview from the arguments: build only
    built = runpy.run_path(CLIFFS, run_name='cliffs_build')
    faces = [m for m in built['models'] if m.name.startswith('CliffFace') and (not only or m.name in only)]
    seeds = {piece[0]: piece[5] for piece in built['PIECES'] + built['TWINS']}     # name -> material seed
    scene = bpy.context.scene
    sun, ground = setup(scene)
    report = {}
    for face in faces:
        entry = report[face.name] = {'source': triangles(face), 'shares': {}, 'views': {}}
        copies = {100.0: face}
        for share in shares:
            low = copies[share] = cut(face, share, built['DENSITY'], seeds[face.name])
            entry['shares'][f'{share:g}'] = dict(triangles=triangles(low), deviation_cm=deviation(face, low))
        for distance in DISTANCES:
            cam = camera_for(face, distance)
            region = border(scene, cam, face)
            paths = {}
            for share, obj in copies.items():
                paths[share] = os.path.join(OUT, f'{face.name}_{distance:03.0f}m_{share:g}.png')
                render(scene, cam, {obj, sun, ground}, region, paths[share])
            ref = load(paths[100.0])
            view = entry['views'][f'{distance:g}'] = {'crop_px': [ref.shape[1], ref.shape[0]]}
            for share in shares:
                view[f'{share:g}'] = change(ref, load(paths[share]))
            print(f"LOOTER: {face.name} at {distance:g} m ({ref.shape[1]} x {ref.shape[0]} px): " +
                  ', '.join(f"{s:g}% {view[f'{s:g}']['changed']}% changed" for s in shares), flush=True)
        print(f"LOOTER: {face.name}: source {entry['source']} triangles; " +
              ', '.join(f"{s}% {v['triangles']} ({v['deviation_cm']['mean']} / {v['deviation_cm']['p95']} / "
                        f"{v['deviation_cm']['max']} cm)" for s, v in entry['shares'].items()), flush=True)
    with open(os.path.join(OUT, 'report.json'), 'w', encoding='utf-8') as f:
        json.dump(dict(shares=shares, distances=DISTANCES, eye_m=EYE, azimuth=AZIMUTH, fov=FOV, resolution=RES,
                       pieces=report), f, indent=2)

    if len(shares) < 2:
        return
    before, after = f'{shares[0]:g}', f'{shares[-1]:g}'
    worst = sorted(report, key=lambda n: -report[n]['views']['20'][after]['changed'])[:SHEET_PIECES]
    tiles = []
    for name in worst:
        entry = report[name]
        b, a = entry['shares'][before], entry['shares'][after]
        for distance in DISTANCES:
            image, k = pair(*(os.path.join(OUT, f'{name}_{distance:03.0f}m_{s}.png') for s in (before, after)))
            path = os.path.join(OUT, f'_tile_{name}_{distance:03.0f}m.png')
            save(image, path)
            view = entry['views'][f'{distance:g}']
            w, h = view['crop_px']
            label = f"{name} at {distance:g} m" + (f"  (x{k})" if k > 1 else '')
            caption = (f"{before}%: {b['triangles']:,} tris, {view[before]['changed']}% of pixels changed from the source  |  "
                       f"{after}%: {a['triangles']:,} tris, {view[after]['changed']}%\n"
                       f"{w} x {h} px at 1080p; off the source's surface (mean / p95): {before}% "
                       f"{b['deviation_cm']['mean']} / {b['deviation_cm']['p95']} cm, {after}% "
                       f"{a['deviation_cm']['mean']} / {a['deviation_cm']['p95']} cm")
            tiles.append((path, label, caption))
    homes.look_sheet(os.path.join(OUT, 'CliffFallback_sheet.jpg'), f'Cliff faces: fallback {before}% -> {after}%',
                     f'What Medium draws of the three faces that change most: {before}% (left) beside {after}% (right),\n'
                     f'at 20, 60 and 150 m in the 1080p frame (80 degree view), far crops enlarged by whole pixels.\n'
                     f'The cut is Blender\'s Decimate standing in for Nanite\'s fallback; High and Epic draw the source.',
                     None, tiles, width=2800, columns=2)


main()
