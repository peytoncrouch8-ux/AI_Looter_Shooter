"""Renders a Tripo model (GLB, its own textures) from the front and a three-quarter view into one PNG, for the user to
judge before any more credits are spent (Tools/tripo.py). Run by Blender:
  blender -b --factory-startup --python Tools/Blender/tripo_preview.py -- <in.glb> <out.png>
"""

import math
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index('--') + 1:]
src, out = args[0], args[1]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
lo = Vector((min(min((o.matrix_world @ Vector(c)).x for c in o.bound_box) for o in meshes),
             min(min((o.matrix_world @ Vector(c)).y for c in o.bound_box) for o in meshes),
             min(min((o.matrix_world @ Vector(c)).z for c in o.bound_box) for o in meshes)))
hi = Vector((max(max((o.matrix_world @ Vector(c)).x for c in o.bound_box) for o in meshes),
             max(max((o.matrix_world @ Vector(c)).y for c in o.bound_box) for o in meshes),
             max(max((o.matrix_world @ Vector(c)).z for c in o.bound_box) for o in meshes)))
centre, size = (lo + hi) / 2, max((hi - lo).x, (hi - lo).y, (hi - lo).z)
print(f'TRIPO_PREVIEW size {hi - lo} (m)')

scene = bpy.context.scene
scene.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in [e.identifier for e in
    bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE'
scene.render.resolution_x, scene.render.resolution_y = 1400, 1000
scene.render.film_transparent = False
world = bpy.data.worlds.new('W')
world.use_nodes = True
world.node_tree.nodes['Background'].inputs[0].default_value = (0.32, 0.34, 0.37, 1)
world.node_tree.nodes['Background'].inputs[1].default_value = 0.9
scene.world = world
sun = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN'))
sun.data.energy = 3.5
sun.rotation_euler = (math.radians(50), 0, math.radians(30))
scene.collection.objects.link(sun)

# Two views side by side: two renders composited by rendering twice into halves isn't simple; render two images and join.
images = []
for i, yaw in enumerate((0.0, 40.0)):
    cam = bpy.data.objects.new(f'Cam{i}', bpy.data.cameras.new(f'Cam{i}'))
    cam.data.lens = 50
    distance = size * 2.3
    # Tripo exports facing +X (its export_orientation default), so the front view looks from +X.
    angle = math.radians(yaw)
    cam.location = centre + Vector((math.cos(angle) * distance, math.sin(angle) * distance, size * 0.12))
    direction = centre - cam.location
    cam.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = 700, 1000
    path = out.replace('.png', f'_{i}.png')
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    images.append(path)

# Join the two renders into one picture.
left, right = bpy.data.images.load(images[0]), bpy.data.images.load(images[1])
w, h = left.size
joined = bpy.data.images.new('Joined', w * 2, h)
lp, rp = list(left.pixels), list(right.pixels)
pixels = [0.0] * (w * 2 * h * 4)
for row in range(h):
    pixels[row * w * 2 * 4: row * w * 2 * 4 + w * 4] = lp[row * w * 4:(row + 1) * w * 4]
    pixels[row * w * 2 * 4 + w * 4:(row + 1) * w * 2 * 4] = rp[row * w * 4:(row + 1) * w * 4]
joined.pixels = pixels
joined.filepath_raw = out
joined.file_format = 'PNG'
joined.save()
print('TRIPO_PREVIEW done', out)
