"""Before/after views of buildings swapped for Tripo models (looter_tripo_buildings): each old export beside its new
one, from the same camera (front three-quarter and a second from the back three-quarter), with the UCX hulls drawn
as a red wire on the new one, so size, pivot, porch and collision can be judged at a glance. Run by Blender:
  blender -b --factory-startup --python Tools/Blender/tripo_buildings_check.py -- <out.png> <old.fbx> <new.fbx> [...]
(Tools/artrun.ps1 -Script Tools/Blender/tripo_buildings_check.py -ScriptArgs out.png,old.fbx,new.fbx,...)
"""
import math
import os
import sys

import bpy
from mathutils import Vector

args = [a for x in sys.argv[sys.argv.index('--') + 1:] for a in x.split(',') if a]
out, files = args[0], args[1:]
pairs = list(zip(files[0::2], files[1::2]))
W, H = 640, 520
tiles = []


def setup(frame_lo, frame_hi, view):
    scene = bpy.context.scene
    engines = [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items]
    scene.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in engines else 'BLENDER_EEVEE'
    scene.render.resolution_x, scene.render.resolution_y = W, H
    scene.view_settings.view_transform = 'Standard'
    world = bpy.data.worlds.new('W')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs[0].default_value = (0.55, 0.62, 0.72, 1)
    world.node_tree.nodes['Background'].inputs[1].default_value = 0.8
    scene.world = world
    sun = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN'))
    sun.data.energy = 4.0
    sun.data.color = (1.0, 0.9, 0.75)
    sun.rotation_euler = (math.radians(50), 0, math.radians(-35))
    scene.collection.objects.link(sun)
    ground = bpy.data.meshes.new('G')
    ground.from_pydata([(-60, -60, 0), (60, -60, 0), (60, 60, 0), (-60, 60, 0)], [], [(0, 1, 2, 3)])
    g = bpy.data.objects.new('G', ground)
    gm = bpy.data.materials.new('GM')
    gm.diffuse_color = (0.42, 0.33, 0.22, 1)
    gm.use_nodes = True
    gm.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.42, 0.33, 0.22, 1)
    g.data.materials.append(gm)
    scene.collection.objects.link(g)
    centre = (frame_lo + frame_hi) / 2
    size = max((frame_hi - frame_lo).x, (frame_hi - frame_lo).y, (frame_hi - frame_lo).z)
    cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam'))
    cam.data.lens = 40
    d = Vector(view).normalized() * size * 1.9
    cam.location = centre + d
    cam.rotation_euler = (centre - cam.location).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam


def load(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=path)
    # A model with a Fan socket and a SM_<name>Fan.fbx beside it (the windpumps): the fan hung on its socket.
    import glob
    import json
    fan = path[:-4] + 'Fan.fbx'
    model = os.path.splitext(os.path.basename(path))[0]
    if os.path.exists(fan):
        for manifest in glob.glob(os.path.join(os.path.dirname(path), '*.json')):
            for m in json.load(open(manifest))['models']:
                for s in m['sockets'] if m['name'] == model else []:
                    if s['name'] == 'Fan':
                        before = set(bpy.context.scene.objects)
                        bpy.ops.import_scene.fbx(filepath=fan)
                        loc = s['location']
                        for o in set(bpy.context.scene.objects) - before:
                            if o.parent is None:
                                o.location = Vector((-loc[1] / 100.0, -loc[0] / 100.0, loc[2] / 100.0))
    objs = list(bpy.context.scene.objects)
    hulls = [o for o in objs if o.type == 'MESH' and o.name.startswith('UCX_')]
    meshes = [o for o in objs if o.type == 'MESH' and o not in hulls]
    return meshes, hulls


def box(meshes):
    pts = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
    return (Vector((min(p[k] for p in pts) for k in range(3))), Vector((max(p[k] for p in pts) for k in range(3))))


def show_hulls(hulls):
    mat = bpy.data.materials.new('Hull')
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    nodes.clear()
    emit = nodes.new('ShaderNodeEmission')
    emit.inputs[0].default_value = (1.0, 0.1, 0.05, 1)
    outn = nodes.new('ShaderNodeOutputMaterial')
    mat.node_tree.links.new(emit.outputs[0], outn.inputs[0])
    for h in hulls:
        mod = h.modifiers.new('W', 'WIREFRAME')
        mod.thickness = 0.03
        h.data.materials.clear()
        h.data.materials.append(mat)
        h.hide_render = False


def textured(meshes, fbx):
    """FBX materials come in flat; point them at the exported texture sets (Art/Textures/<Set>/T_<Set>_BC.png) when the
    material's name names one (Tripo<Name>, HouseTrim...)."""
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
    for o in meshes:
        for slot in o.material_slots:
            m = slot.material
            if not m:
                continue
            path = os.path.join(root, 'Art', 'Textures', m.name, f'T_{m.name}_BC.png')
            if not os.path.exists(path):
                continue
            m.use_nodes = True
            nt = m.node_tree
            bsdf = next((n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'), None)
            if not bsdf:
                continue
            tex = nt.nodes.new('ShaderNodeTexImage')
            tex.image = bpy.data.images.load(path, check_existing=True)
            nt.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
            bsdf.inputs['Roughness'].default_value = 0.85


for old, new in pairs:
    frames = []
    for path in (old, new):
        meshes, hulls = load(path)
        frames.append(box(meshes))
    lo = Vector((min(f[0][k] for f in frames) for k in range(3)))
    hi = Vector((max(f[1][k] for f in frames) for k in range(3)))
    for view in ((-1.0, -1.5, 0.6), (1.2, 1.4, 0.7)):
        for path in (old, new):
            meshes, hulls = load(path)
            textured(meshes, path)
            if path == new:
                show_hulls(hulls)
            else:
                for h in hulls:
                    h.hide_render = True
            setup(lo, hi, view)
            shot = out.replace('.png', f'_{len(tiles)}.png')
            bpy.context.scene.render.filepath = shot
            bpy.ops.render.render(write_still=True)
            tiles.append(shot)
    print(f'TRIPO_CHECK {os.path.basename(old)} {tuple(round(v, 2) for v in frames[0][1] - frames[0][0])} -> '
          f'{os.path.basename(new)} {tuple(round(v, 2) for v in frames[1][1] - frames[1][0])}')

columns = 4
rows = (len(tiles) + columns - 1) // columns
sheet = bpy.data.images.new('Sheet', W * columns, H * rows)
pixels = [0.2] * (W * columns * H * rows * 4)
for index, path in enumerate(tiles):
    tile = bpy.data.images.load(path)
    tp = list(tile.pixels)
    column, row = index % columns, rows - 1 - index // columns
    for y in range(H):
        dst = ((row * H + y) * W * columns + column * W) * 4
        pixels[dst:dst + W * 4] = tp[y * W * 4:(y + 1) * W * 4]
sheet.pixels = pixels
sheet.filepath_raw = out
sheet.file_format = 'PNG'
sheet.save()
for path in tiles:
    os.remove(path)
print('TRIPO_CHECK done', out)
