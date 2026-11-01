"""Builds a story person's model script (Art/Models/Creatures/<Name>.py, with its Tripo body swapped in) and renders
check pictures to Saved/ArtPreviews/TripoPeople/: <Name>_rest.png (front, side, back, three-quarter; the props on),
<Name>_poses.png (the pose table's poses, or test turns for the Unpaid-rigged ones), <Name>_face.png.
Blender: Tools\\artrun.ps1 -Script <this> -ScriptArgs <Name>[,script] (script: the script body, for comparison)."""
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

args = [a for a in sys.argv[sys.argv.index('--') + 1:] for a in a.split(',') if a]
name = args[0]
REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
OUT = os.path.join(REPO, 'Saved', 'ArtPreviews', 'TripoPeople')
os.makedirs(OUT, exist_ok=True)
tag = '_script' if 'script' in args else ''
if 'script' in args:
    os.environ['LOOTER_PEOPLE_TRIPO'] = '0'
path = os.path.join(REPO, 'Art', 'Models', 'Creatures', f'{name}.py')
sys.argv = [sys.argv[0], '--']
G = {'__name__': '__main__', '__file__': path}
exec(compile(open(path, encoding='utf-8').read(), path, 'exec'), G)
rig, body = G['RIG'], G['BODY']
print(f'CHECK {name}: body {body.name} {sum(len(p.vertices) - 2 for p in body.data.polygons)} tris, '
      f'{len(body.data.vertices)} verts, {len(rig.data.bones)} bones, uv {[u.name for u in body.data.uv_layers]}, '
      f'hulls {len([o for o in rig.children if o.name.startswith(("UCX_", "USP_"))])}')

# --- Looks for the pictures ---
paint_png = os.path.join(REPO, 'Art', 'Textures', f'{name}Paint', f'T_{name}Paint_BC.png')


def look(mat, image=None, uv='Paint', color=None, emit=None):
    m = bpy.data.materials.new(mat)
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes['Principled BSDF']
    bsdf.inputs['Roughness'].default_value = 0.8
    if image:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = bpy.data.images.load(image, check_existing=True)
        uvn = nt.nodes.new('ShaderNodeUVMap')
        uvn.uv_map = uv
        nt.links.new(uvn.outputs[0], tex.inputs[0])
        # M_Ghost's cavity: vertex colour G darkens (1 - 0.95 G^0.6)
        attr = nt.nodes.new('ShaderNodeVertexColor')
        attr.layer_name = 'Col'
        sep = nt.nodes.new('ShaderNodeSeparateColor')
        nt.links.new(attr.outputs[0], sep.inputs[0])
        pw = nt.nodes.new('ShaderNodeMath')
        pw.operation = 'POWER'
        pw.inputs[1].default_value = 0.6
        nt.links.new(sep.outputs[1], pw.inputs[0])
        mul = nt.nodes.new('ShaderNodeMath')
        mul.operation = 'MULTIPLY_ADD'
        mul.inputs[1].default_value = -0.95
        mul.inputs[2].default_value = 1.0
        nt.links.new(pw.outputs[0], mul.inputs[0])
        mix = nt.nodes.new('ShaderNodeMix')
        mix.data_type = 'RGBA'
        mix.blend_type = 'MULTIPLY'
        mix.inputs[0].default_value = 1.0
        nt.links.new(tex.outputs[0], mix.inputs[6])
        nt.links.new(mul.outputs[0], mix.inputs[7])
        nt.links.new(mix.outputs[2], bsdf.inputs['Base Color'])
    elif color is not None:
        bsdf.inputs['Base Color'].default_value = color
    if emit is not None:
        bsdf.inputs['Emission Color'].default_value = emit
        bsdf.inputs['Emission Strength'].default_value = 4.0
    return m


if not tag and os.path.exists(paint_png):
    body.data.materials[0] = look('_paint', paint_png)
else:
    body.data.materials[0] = look('_flat', color=(0.55, 0.58, 0.6, 1))
if len(body.data.materials) > 1:
    body.data.materials[1] = look('_coal', color=(0.05, 0.02, 0.01, 1), emit=(1.0, 0.35, 0.08, 1))
props = [o for o in bpy.data.objects if o.type == 'MESH' and o.parent is None and not o.name.startswith(('_', 'UCX'))
         and o is not body and not o.hide_render and o.location.length > 1e-4]     # props in their own axes at 0: not placed
for o in props:
    for i, mt in enumerate(o.data.materials):
        if mt is not None:
            base = mt.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value[:] if mt.use_nodes else (0.5,) * 4
            o.data.materials[i] = look('_prop', color=base)
for o in bpy.data.objects:
    if o.name.startswith(('UCX_', 'USP_', 'UCP_')) or (o.type == 'MESH' and o not in props and o is not body):
        o.hide_render = True

scene = bpy.context.scene
scene.render.engine = 'BLENDER_EEVEE_NEXT' if 'BLENDER_EEVEE_NEXT' in [e.identifier for e in
    bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE'
scene.view_settings.view_transform = 'AgX' if 'AgX' in [v.identifier for v in
    scene.view_settings.bl_rna.properties['view_transform'].enum_items] else 'Filmic'
world = bpy.data.worlds.new('W')
world.use_nodes = True
world.node_tree.nodes['Background'].inputs[0].default_value = (0.55, 0.62, 0.75, 1)
world.node_tree.nodes['Background'].inputs[1].default_value = 0.6
scene.world = world
sun = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN'))
sun.data.energy = 4.0
sun.data.color = (1.0, 0.9, 0.75)
sun.rotation_euler = (math.radians(50), 0, math.radians(-35))
scene.collection.objects.link(sun)
cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam'))
scene.collection.objects.link(cam)
scene.camera = cam


def shot(path, yaw, target=(0.0, 0.0, 1.0), dist=4.2, lens=50, size=(560, 900), height=0.0):
    a = math.radians(yaw)
    tgt = Vector(target)
    cam.location = tgt + Vector((math.sin(a) * dist, -math.cos(a) * dist, height))
    cam.rotation_euler = (tgt - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens = lens
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    return path


def join(paths, out):
    imgs = [bpy.data.images.load(p) for p in paths]
    w, h = imgs[0].size
    sheet = bpy.data.images.new('sheet', w * len(imgs), h)
    px = np.zeros((h, w * len(imgs), 4), np.float32)
    for k, im in enumerate(imgs):
        a = np.empty(w * h * 4, np.float32)
        im.pixels.foreach_get(a)
        px[:, k * w:(k + 1) * w] = a.reshape(h, w, 4)
    sheet.pixels.foreach_set(px.ravel())
    sheet.filepath_raw = out
    sheet.file_format = 'PNG'
    sheet.save()
    for p in paths:
        os.remove(p)
    print('CHECK wrote', out)


lo = np.min([np.array(body.matrix_world @ Vector(c)) for c in body.bound_box], axis=0)
hi = np.max([np.array(body.matrix_world @ Vector(c)) for c in body.bound_box], axis=0)
mid = (0.0, 0.0, 0.95)
join([shot(os.path.join(OUT, f'_{name}_r{k}.png'), yaw, mid) for k, yaw in enumerate((0, 90, 180, 35))],
     os.path.join(OUT, f'{name}{tag}_rest.png'))
head = np.array(rig.data.bones['head'].head_local) + np.array([0.0, -0.04, 0.08])
join([shot(os.path.join(OUT, f'_{name}_f{k}.png'), yaw, head, dist=1.1, size=(700, 700)) for k, yaw in enumerate((0, 40, 90))],
     os.path.join(OUT, f'{name}{tag}_face.png'))
hand = np.array(rig.data.bones['hand_r'].head_local)
join([shot(os.path.join(OUT, f'_{name}_h{k}.png'), yaw, hand, dist=0.9, size=(600, 600)) for k, yaw in enumerate((0, -60))],
     os.path.join(OUT, f'{name}{tag}_hand.png'))

# The poses: the pose table's (Poser.apply or the script's apply), else the Unpaid rig's test turns.
poses = G.get('POSES') or {}
apply = G['POSER'].apply if 'POSER' in G else G.get('apply')
paths = []
if poses and apply:
    for pose in [p for p in poses if p not in ('idle',)][:5]:
        try:
            apply(poses[pose])
        except Exception as e:      # noqa: BLE001
            print('CHECK pose', pose, 'failed', e)
            continue
        bpy.context.view_layer.update()
        paths.append(shot(os.path.join(OUT, f'_{name}_p{len(paths)}.png'), 35, mid))
else:
    for k, (bone, axis, deg) in enumerate((('upperarm_r', 'X', -80), ('upperarm_l', 'Y', 60), ('jaw', 'X', 25),
                                           ('spine_02', 'X', 25))):
        for pb in rig.pose.bones:
            pb.matrix_basis = Matrix()
        pb = rig.pose.bones[bone]
        pb.rotation_mode = 'XYZ'
        setattr(pb.rotation_euler, 'xyz'['XYZ'.index(axis)], math.radians(deg))
        bpy.context.view_layer.update()
        paths.append(shot(os.path.join(OUT, f'_{name}_p{k}.png'), 35, mid))
if paths:
    join(paths, os.path.join(OUT, f'{name}{tag}_poses.png'))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(REPO, 'Intermediate', 'TripoPeople', f'{name}{tag}_check.blend'))
