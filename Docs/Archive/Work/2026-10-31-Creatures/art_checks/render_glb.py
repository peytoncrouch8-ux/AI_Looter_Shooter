"""Renders a Tripo GLB with its own materials from a few angles. Args: <glb> <out prefix>"""
import math
import os
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index('--') + 1:]
glb, out = os.path.abspath(args[0]), os.path.abspath(args[1])
bpy.ops.import_scene.gltf(filepath=glb)
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
for o in meshes:
    for m in o.data.materials:
        if m and m.use_nodes:
            for n in m.node_tree.nodes:
                if n.type == 'TEX_IMAGE' and n.image:
                    print('IMG', m.name, n.image.name, n.image.size[:], n.image.colorspace_settings.name)
                if n.type == 'BSDF_PRINCIPLED':
                    for k in ('Base Color', 'Roughness', 'Metallic', 'Normal'):
                        print('BSDF', k, [l.from_node.type for l in n.inputs[k].links], n.inputs[k].default_value if not n.inputs[k].links and hasattr(n.inputs[k], 'default_value') else '')
bpy.context.view_layer.update()
lo = Vector((1e9,) * 3)
hi = Vector((-1e9,) * 3)
for o in meshes:
    for c in o.bound_box:
        p = o.matrix_world @ Vector(c)
        lo = Vector(map(min, lo, p))
        hi = Vector(map(max, hi, p))
print('BOUNDS', lo, hi)
center = (lo + hi) / 2
size = max(hi - lo)
sc = bpy.context.scene
sc.render.engine = 'BLENDER_EEVEE_NEXT'
sc.render.resolution_x, sc.render.resolution_y = 1000, 1000
sc.view_settings.view_transform = 'Standard'
world = bpy.data.worlds.new('w')
world.use_nodes = True
world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.55, 0.6, 0.68, 1.0)
world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.8
sc.world = world
sun = bpy.data.objects.new('sun', bpy.data.lights.new('sun', 'SUN'))
sun.data.energy = 3.5
sun.rotation_euler = (math.radians(50), 0, math.radians(-35))
sc.collection.objects.link(sun)
cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.lens = 50
for name, d in (('front', (0.0, -1.0, 0.25)), ('q', (0.8, -1.0, 0.5)), ('side', (1.0, 0.0, 0.15)), ('back', (-0.5, 1.0, 0.4))):
    d = Vector(d).normalized()
    cam.location = center + d * size * 2.0
    cam.rotation_euler = (center - cam.location).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = f'{out}_{name}.png'
    bpy.ops.render.render(write_still=True)
    print('WROTE', sc.render.filepath)
