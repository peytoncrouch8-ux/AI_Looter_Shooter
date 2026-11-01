"""Renders Tripo GLBs as they came (their own maps) from the four sides and a three-quarter view, to judge them against the
views they were made from (lane TRIPO-PISTOLS, 2026-11-01). Prints each model's bounds per axis (which way it lies).

    Tools\\artrun.ps1 -Script Tools\\Blender\\tripo_part_views.py -ScriptArgs <out dir>,<a.glb>,<b.glb>...

Writes <out dir>/<glb name>_{front,back,left,right,q}.png (512 px, EEVEE, transparent). Views assume Tripo's
align_image output in Blender: the front view's subject faces -Y (the muzzle), its left +X.
"""
import math
import os
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index('--') + 1:]
args = [a for part in args for a in part.split(',') if a]
out_dir, sources = args[0], args[1:]
os.makedirs(out_dir, exist_ok=True)
SIZE = 512


def look(ob, target):
    ob.rotation_euler = (target - ob.location).to_track_quat('-Z', 'Y').to_euler()


for src in sources:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=src)
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    pts = [o.matrix_world @ v.co for o in meshes for v in o.data.vertices]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    c, ext = (lo + hi) / 2, hi - lo
    tris = sum(sum(len(p.vertices) - 2 for p in o.data.polygons) for o in meshes)
    print(f'TRIPO_VIEWS {os.path.basename(src)}: size x {ext.x:.3f} y {ext.y:.3f} z {ext.z:.3f}, {tris} triangles')
    sc = bpy.context.scene
    engines = [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items]
    sc.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in engines else 'BLENDER_EEVEE'
    sc.render.resolution_x = sc.render.resolution_y = SIZE
    sc.render.film_transparent = True
    sc.view_settings.view_transform = 'Standard'
    world = bpy.data.worlds.new('W')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs[0].default_value = (0.7, 0.71, 0.73, 1)
    world.node_tree.nodes['Background'].inputs[1].default_value = 0.8
    sc.world = world
    sun = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN'))
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(50), 0, math.radians(30))
    sc.collection.objects.link(sun)
    cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam'))
    sc.collection.objects.link(cam)
    sc.camera = cam
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = max(ext) / 0.84
    cam.data.clip_start, cam.data.clip_end = 0.001, 100.0
    d = max(ext) * 4.0
    name = os.path.splitext(os.path.basename(src))[0]
    for view, back in (('front', Vector((0, -1, 0))), ('back', Vector((0, 1, 0))), ('left', Vector((1, 0, 0))),
                       ('right', Vector((-1, 0, 0))), ('q', Vector((0.75, -0.55, 0.38)).normalized())):
        cam.location = c + back * d
        look(cam, c)
        sun.rotation_euler = (cam.location - c).to_track_quat('Z', 'Y').to_euler()
        sun.rotation_euler.x += math.radians(25)
        sc.render.filepath = os.path.join(out_dir, f'{name}_{view}.png')
        bpy.ops.render.render(write_still=True)
print('TRIPO_VIEWS done')
