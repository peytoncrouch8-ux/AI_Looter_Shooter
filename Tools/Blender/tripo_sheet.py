"""A contact sheet of Tripo models: each GLB rendered once (a three-quarter view from its front, +X), labelled, in a grid,
so the user and Main can judge a batch at a glance. Run by Blender:
  blender -b --factory-startup --python Tools/Blender/tripo_sheet.py -- <out.png> <a.glb> <b.glb> ...
"""

import math
import os
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index('--') + 1:]
out, sources = args[0], args[1:]
CELL = 420
tiles = []

for src in sources:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=src)
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    points = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
    lo = Vector((min(p.x for p in points), min(p.y for p in points), min(p.z for p in points)))
    hi = Vector((max(p.x for p in points), max(p.y for p in points), max(p.z for p in points)))
    centre, size = (lo + hi) / 2, max((hi - lo).x, (hi - lo).y, (hi - lo).z)
    scene = bpy.context.scene
    engines = [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items]
    scene.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in engines else 'BLENDER_EEVEE'
    scene.render.resolution_x = scene.render.resolution_y = CELL
    world = bpy.data.worlds.new('W')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs[0].default_value = (0.32, 0.34, 0.37, 1)
    world.node_tree.nodes['Background'].inputs[1].default_value = 0.9
    scene.world = world
    sun = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN'))
    sun.data.energy = 3.5
    sun.rotation_euler = (math.radians(50), 0, math.radians(30))
    scene.collection.objects.link(sun)
    cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam'))
    cam.data.lens = 50
    angle = math.radians(35.0)
    distance = size * 2.2
    cam.location = centre + Vector((math.cos(angle) * distance, math.sin(angle) * distance, size * 0.35))
    cam.rotation_euler = (centre - cam.location).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam
    path = out.replace('.png', f'_{len(tiles)}.png')
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    tiles.append((path, os.path.splitext(os.path.basename(src))[0], hi - lo))
    print(f'TRIPO_SHEET {os.path.basename(src)} size {tuple(round(v, 2) for v in (hi - lo))} m')

columns = min(5, len(tiles))
rows = (len(tiles) + columns - 1) // columns
sheet = bpy.data.images.new('Sheet', CELL * columns, CELL * rows)
pixels = [0.2] * (CELL * columns * CELL * rows * 4)
for index, (path, _, _) in enumerate(tiles):
    tile = bpy.data.images.load(path)
    tp = list(tile.pixels)
    column, row = index % columns, rows - 1 - index // columns
    for y in range(CELL):
        dst = ((row * CELL + y) * CELL * columns + column * CELL) * 4
        pixels[dst:dst + CELL * 4] = tp[y * CELL * 4:(y + 1) * CELL * 4]
sheet.pixels = pixels
sheet.filepath_raw = out
sheet.file_format = 'PNG'
sheet.save()
for path, _, _ in tiles:
    os.remove(path)
print('TRIPO_SHEET done', out, 'order:', ', '.join(name for _, name, _ in tiles))
