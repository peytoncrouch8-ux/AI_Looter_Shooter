"""Orthographic front and side views of a Tripo GLB (oriented as parts_people orients it: front -Y, feet on z 0) with a
10 cm grid behind it and height labels, plus optional joint markers, for measuring joints by eye.
Blender: artrun.ps1 -Script <this> -ScriptArgs <Name>[,markers]
Writes Saved/ArtPreviews/TripoPeople/<Name>_measure.png"""
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Art', 'Models', 'Creatures', 'Tripo'))
args = [a for a in sys.argv[sys.argv.index('--') + 1:] for a in a.split(',') if a]
name = args[0]
print('GLB_VIEWS args', args, flush=True)
import parts_people_src as ps  # noqa: E402

obj, V, F = ps.source_object(name)
grey = bpy.data.materials.new('grey')
grey.use_nodes = True
grey.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.5, 0.5, 0.52, 1)
grey.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = 0.7
obj.data.materials.clear()
obj.data.materials.append(grey)
lo, hi = V.min(0), V.max(0)
crop = None
for a in args:
    if a.startswith('crop='):
        crop = [float(v) for v in a[5:].split(':')]   # x0:x1:z0:z1 (front) / y0:y1:z0:z1 (side)
step = 0.02 if crop else 0.1
print(f'GLB_VIEWS {name}: bounds {np.round(lo, 3)} .. {np.round(hi, 3)}, {len(V)} vertices, {len(F)} faces')
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'TripoPeople')
os.makedirs(OUT, exist_ok=True)

scene = bpy.context.scene
scene.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in [e.identifier for e in
    bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE'
world = bpy.data.worlds.new('W')
world.use_nodes = True
world.node_tree.nodes['Background'].inputs[0].default_value = (0.8, 0.8, 0.8, 1)
world.node_tree.nodes['Background'].inputs[1].default_value = 0.35
scene.world = world
scene.view_settings.view_transform = 'Standard'


def emit(color):
    m = bpy.data.materials.new('e')
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    o = nt.nodes.new('ShaderNodeOutputMaterial')
    e = nt.nodes.new('ShaderNodeEmission')
    e.inputs['Color'].default_value = color
    nt.links.new(e.outputs[0], o.inputs[0])
    return m


def box(c, size, mat):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=c)
    b = bpy.context.active_object
    b.scale = size
    b.data.materials.append(mat)
    return b


thin, thick, red = emit((0.55, 0.6, 0.65, 1)), emit((0.15, 0.2, 0.3, 1)), emit((1, 0, 0, 1))
markers = {}
if 'markers' in args:
    markers = ps.joints(name, 'raw')
    for k, p in markers.items():
        print('JOINT', name, k, np.round(p, 3).tolist())
span = max(hi[2] - lo[2], hi[0] - lo[0]) + 0.2
for view in ('front', 'side'):
    col = bpy.data.collections.new(view)
    scene.collection.children.link(col)
    before = set(bpy.data.objects)
    # the grid on a plane behind the model
    if view == 'front':
        back, ax = hi[1] + 0.3, 0
    else:
        back, ax = lo[0] - 0.3, 1
    rng = np.arange(math.floor(min(lo[ax], lo[2]) * 10) / 10 - 0.1, max(hi[ax], hi[2]) + 0.2, step)
    for v in rng:
        heavy = abs(v * 10 - round(v * 10)) < 1e-6
        w = (0.0015 if heavy else 0.0005) if crop else (0.004 if heavy else 0.0015)
        m = thick if heavy else thin
        if view == 'front':
            box((v, back, (lo[2] + hi[2]) / 2), (w, 0.001, hi[2] - lo[2] + 0.4), m)
            box(((lo[0] + hi[0]) / 2, back, v), (hi[0] - lo[0] + 0.4, 0.001, w), m)
        else:
            box((back, v, (lo[2] + hi[2]) / 2), (0.001, w, hi[2] - lo[2] + 0.4), m)
            box((back, (lo[1] + hi[1]) / 2, v), (0.001, hi[1] - lo[1] + 0.4, w), m)
    zs = np.arange(round(crop[2], 2), crop[3] + 1e-6, 0.02) if crop else np.arange(0.0, hi[2] + 0.1, 0.1)
    for z in zs:
        lx = (crop[0] + 0.003) if crop else lo[0] - 0.15
        bpy.ops.object.text_add(location=((lx, back - 0.01, z + 0.002) if view == 'front' else
                                          (back + 0.01, (crop[1] - 0.003) if crop else hi[1] + 0.06, z + 0.002)))
        t = bpy.context.active_object
        t.data.body = f'{z:.2f}'
        t.data.size = 0.006 if crop else 0.035
        t.rotation_euler = (math.pi / 2, 0, 0) if view == 'front' else (math.pi / 2, 0, -math.pi / 2)
        t.data.materials.append(thick)
    xs = np.arange(round(crop[0], 2), crop[1] + 1e-6, 0.02) if crop else np.arange(math.floor(lo[ax] * 10) / 10, hi[ax] + 0.05, 0.1)
    for x in xs:
        bz = (crop[2] + 0.004) if crop else lo[2] - 0.08
        bpy.ops.object.text_add(location=((x + 0.001, back - 0.01, bz) if view == 'front' else
                                          (back + 0.01, x - 0.001, bz)))
        t = bpy.context.active_object
        t.data.body = f'{x:+.2f}'
        t.data.size = 0.005 if crop else 0.025
        t.rotation_euler = (math.pi / 2, 0, 0) if view == 'front' else (math.pi / 2, 0, -math.pi / 2)
        t.data.materials.append(thick)
    for key, p in markers.items():
        q = list(p)
        if view == 'front':
            q[1] = lo[1] - 0.05
        else:
            q[0] = hi[0] + 0.05
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.004 if crop else 0.008, location=tuple(q))
        bpy.context.active_object.data.materials.append(red)
    for o in set(bpy.data.objects) - before:
        for c in o.users_collection:
            c.objects.unlink(o)
        col.objects.link(o)

sun = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN'))
sun.data.energy = 3.0
sun.rotation_euler = (math.radians(60), 0, math.radians(20))
scene.collection.objects.link(sun)
cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam'))
cam.data.type = 'ORTHO'
scene.collection.objects.link(cam)
scene.camera = cam
centre = (lo + hi) / 2
images = []
for view in ('front', 'side'):
    for c in bpy.data.collections:
        if c.name in ('front', 'side'):
            c.hide_render = c.name != view
    views = [v for v in args if v in ('front', 'side')]
    if views and view not in views:
        continue
    if view == 'front':
        cx, w = (((crop[0] + crop[1]) / 2, crop[1] - crop[0]) if crop else (centre[0], hi[0] - lo[0] + 0.5))
        cz, h = (((crop[2] + crop[3]) / 2, crop[3] - crop[2]) if crop else (centre[2], hi[2] - lo[2] + 0.3))
        cam.location = (cx, lo[1] - 3, cz)
        cam.rotation_euler = (math.pi / 2, 0, 0)
    else:
        cy, w = (((crop[0] + crop[1]) / 2, crop[1] - crop[0]) if crop else (centre[1], hi[1] - lo[1] + 0.5))
        cz, h = (((crop[2] + crop[3]) / 2, crop[3] - crop[2]) if crop else (centre[2], hi[2] - lo[2] + 0.3))
        cam.location = (hi[0] + 3, cy, cz)
        cam.rotation_euler = (math.pi / 2, 0, math.pi / 2)
    cam.data.ortho_scale = max(w, h)
    px = 1600
    scene.render.resolution_x = int(px * w / max(w, h))
    scene.render.resolution_y = int(px * h / max(w, h))
    tag = next((a[5:].replace(':', '_') for a in args if a.startswith('crop=')), '')
    path = os.path.join(OUT, f'{name}_{view}{"_" + tag if tag else ""}.png')
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print('GLB_VIEWS wrote', path)
