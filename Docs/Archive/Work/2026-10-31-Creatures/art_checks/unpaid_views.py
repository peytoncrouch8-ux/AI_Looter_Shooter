"""Builds SK_Unpaid (Unpaid.py, Tripo body) and renders it in an approximation of M_Ghost with the paint, at rest and in
the game's poses and extremes. Out: Saved/ArtPreviews/Creatures/Fix1031/unpaid/."""
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector

import looter_textures as lt

ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
OUT = os.path.join(lt.REPO, 'Saved', 'ArtPreviews', 'Creatures', 'Fix1031', 'unpaid')
os.makedirs(OUT, exist_ok=True)
PATH = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Unpaid.py')
ns = {'__file__': PATH, '__name__': '__unpaid__'}
sys.argv = [a for a in sys.argv if a != '--preview']
with open(PATH, encoding='utf-8') as f:
    exec(compile(f.read(), PATH, 'exec'), ns)
rig, body, hat = ns['rig'], ns['body'], ns['hat']
for o in list(bpy.data.objects):
    if o.name.startswith(('UCX_', 'USP_', 'UCP_')):
        o.hide_render = True
ghost = bpy.data.materials['Ghost_A']
print('GHOST', {k: ghost[k] for k in ghost.keys() if not k.startswith('_')})

paint_path = lt.texture_path('UnpaidPaint', 'BC')
paint_img = bpy.data.images.load(paint_path) if os.path.exists(paint_path) else None
CLAY = '--clay' in ARGS


def preview_material():
    mat = bpy.data.materials.new('_GhostPaintPreview')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes['Principled BSDF']
    bsdf.inputs['Roughness'].default_value = 0.86
    col = nodes.new('ShaderNodeVertexColor')
    col.layer_name = 'Col'
    sep = nodes.new('ShaderNodeSeparateColor')
    links.new(col.outputs['Color'], sep.inputs['Color'])
    # Cavity = 1 - 0.95 G^0.6
    pw = nodes.new('ShaderNodeMath'); pw.operation = 'POWER'; pw.inputs[1].default_value = 0.6
    links.new(sep.outputs['Green'], pw.inputs[0])
    cav = nodes.new('ShaderNodeMath'); cav.operation = 'MULTIPLY_ADD'
    cav.inputs[1].default_value = -0.95; cav.inputs[2].default_value = 1.0
    links.new(pw.outputs['Value'], cav.inputs[0])
    if CLAY or paint_img is None:
        base = nodes.new('ShaderNodeRGB'); base.outputs[0].default_value = (0.55, 0.55, 0.55, 1)
        base_out = base.outputs[0]
    else:
        uv = nodes.new('ShaderNodeUVMap'); uv.uv_map = 'Paint'
        tex = nodes.new('ShaderNodeTexImage'); tex.image = paint_img
        links.new(uv.outputs['UV'], tex.inputs['Vector'])
        base_out = tex.outputs['Color']
    mul = nodes.new('ShaderNodeMix'); mul.data_type = 'RGBA'; mul.blend_type = 'MULTIPLY'
    mul.inputs['Factor'].default_value = 1.0
    links.new(base_out, mul.inputs['A'])
    links.new(cav.outputs['Value'], mul.inputs['B'])
    links.new(mul.outputs['Result'], bsdf.inputs['Base Color'])
    # A thin cold rim (RimStrength 0.8, power 4) as emission.
    lw = nodes.new('ShaderNodeLayerWeight'); lw.inputs['Blend'].default_value = 0.5
    rp = nodes.new('ShaderNodeMath'); rp.operation = 'POWER'; rp.inputs[1].default_value = 4.0
    links.new(lw.outputs['Facing'], rp.inputs[0])
    rs = nodes.new('ShaderNodeMath'); rs.operation = 'MULTIPLY'; rs.inputs[1].default_value = 0.8
    links.new(rp.outputs['Value'], rs.inputs[0])
    links.new(rs.outputs['Value'], bsdf.inputs['Emission Strength'])
    bsdf.inputs['Emission Color'].default_value = (0.55, 0.78, 0.86, 1)
    # The robe's fade (vertex A) as dithered alpha.
    links.new(col.outputs['Alpha'], bsdf.inputs['Alpha'])
    try:
        mat.surface_render_method = 'DITHERED'
    except Exception:
        pass
    return mat


prev = preview_material()
coal = bpy.data.materials.new('_coal'); coal.use_nodes = True
cb = coal.node_tree.nodes['Principled BSDF']
cb.inputs['Base Color'].default_value = (0.02, 0.01, 0.01, 1)
cb.inputs['Emission Color'].default_value = (0.9, 0.15, 0.05, 1); cb.inputs['Emission Strength'].default_value = 4
body.data.materials[0] = prev
body.data.materials[1] = coal
hat.data.materials[0] = prev

sc = bpy.context.scene
sc.render.engine = 'BLENDER_EEVEE_NEXT'
sc.render.resolution_x, sc.render.resolution_y = 900, 1100
sc.view_settings.view_transform = 'AgX'
world = bpy.data.worlds.new('w'); world.use_nodes = True
world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.5, 0.62, 0.8, 1)
world.node_tree.nodes['Background'].inputs['Strength'].default_value = 1.2
sc.world = world
sun = bpy.data.objects.new('sun', bpy.data.lights.new('sun', 'SUN'))
sun.data.energy = 4.5
sun.data.color = (1.0, 0.95, 0.86)
sun.rotation_euler = (math.radians(45), 0, math.radians(-40))
sc.collection.objects.link(sun)
gm = bpy.data.materials.new('ground'); gm.use_nodes = True
gm.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.38, 0.28, 0.16, 1)
bpy.ops.mesh.primitive_plane_add(size=12, location=(0, 0, 0))
bpy.context.active_object.data.materials.append(gm)
cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.lens = 50


def shot(name, direction, target=(0.0, 0.0, 1.2), distance=4.2):
    d = Vector(direction).normalized()
    cam.location = Vector(target) + d * distance
    cam.rotation_euler = (Vector(target) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = os.path.join(OUT, name + '.png')
    bpy.ops.render.render(write_still=True)
    print('UNPAID VIEW', sc.render.filepath)


pose = ns['pose']
IB = ns['IDLE_BEND']
X, Y, Z = (1, 0, 0), (0, 1, 0), (0, 0, 1)
POSES = {
    'rest': {},
    'lunge': ns['LUNGE'],
    'shriek': ns['SHRIEK'],
    'rake': ns['RAKE'],
    # The extremes the code reaches: arms straight out to the sides 100 (spread 55 + cocked 45), elbows at 130.
    'x_spread': {'upperarm_l': [(Y, -100)], 'upperarm_r': [(Y, 100)],
                 'lowerarm_l': [(X, -130 + IB)], 'lowerarm_r': [(X, -130 + IB)]},
    # Arms swung forward 110 and up, elbows straight; head thrown back; waist bent.
    'x_reach': {'upperarm_l': [(X, -120)], 'upperarm_r': [(X, -120)], 'lowerarm_l': [(X, IB)], 'lowerarm_r': [(X, IB)],
                'spine_02': [(X, 12)], 'head': [(X, -30)], 'pelvis': [(X, 20)]},
    # Swept back 60 and cocked out 45: a wind-up.
    'x_back': {'upperarm_l': [(X, 60), (Y, -45)], 'upperarm_r': [(X, 60), (Y, 45)],
               'lowerarm_l': [(X, -90 + IB)], 'lowerarm_r': [(X, -90 + IB)], 'spine_02': [(Z, 20)]},
}
only = [a for a in ARGS if not a.startswith('--')]
STATS_ONLY = '--stats' in ARGS
for name, rot in POSES.items():
    if (only and name not in only) or STATS_ONLY:
        continue
    pose(rot)
    if name == 'rest':
        shot('rest_front', (0.0, -1.0, 0.1))
        shot('rest_q', (0.8, -1.0, 0.15))
        shot('rest_back', (-0.5, 1.0, 0.15))
        shot('rest_face', (0.25, -1.0, 0.05), target=(0.0, 0.0, 1.62), distance=1.3)
    else:
        shot(name + '_front', (0.15, -1.0, 0.12))
        shot(name + '_q', (0.9, -0.8, 0.2))
        if name.startswith('x_'):
            shot(name + '_shoulder', (0.9, -0.5, 0.35), target=(0.15, 0.0, 1.45), distance=1.4)
        if name in ('shriek', 'lunge'):
            hb = rig.pose.bones['head']
            hp = rig.matrix_world @ hb.head
            shot(name + '_face', (0.35, -1.0, 0.0), target=tuple(hp + Vector((0, -0.05, 0.0))), distance=0.9)
        if name in ('x_reach', 'rake', 'lunge'):
            for sfx in ('l', 'r'):
                hp = rig.matrix_world @ rig.pose.bones[f'hand_{sfx}'].tail
                shot(f'{name}_hand_{sfx}', (0.5, -1.0, 0.4), target=tuple(hp), distance=0.9)
pose({})


# --- Stretch: every edge's posed length over its rest length, per pose, worst places named by their main bone ---
me = body.data
E = np.array([e.vertices[:] for e in me.edges])
rest = np.array([v.co[:] for v in me.vertices])
names = [g.name for g in body.vertex_groups]
Wm = np.zeros((len(me.vertices), len(names)))
for v in me.vertices:
    for g in v.groups:
        Wm[v.index, g.group] = g.weight
main = np.array(names)[Wm.argmax(1)]
alpha = np.ones(len(me.vertices))
lr = np.linalg.norm(rest[E[:, 0]] - rest[E[:, 1]], axis=1)
for name, rot in POSES.items():
    if only and name not in only:
        continue
    pose(rot)
    ev = body.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m2 = ev.to_mesh()
    P = np.array([v.co[:] for v in m2.vertices])
    ev.to_mesh_clear()
    lp = np.linalg.norm(P[E[:, 0]] - P[E[:, 1]], axis=1)
    ratio = np.where(lr > 0.003, lp / np.maximum(lr, 1e-6), 1.0)
    grow = lp - lr
    big = np.argsort(-np.abs(grow))[:12]
    print(f'UNPAID GROW {name}: edges grown over 2 cm {(grow > 0.02).sum()}, shrunk over 2 cm {(grow < -0.02).sum()}; worst ' + ', '.join(f'{main[E[k, 0]]}/{main[E[k, 1]]} {lr[k] * 100:.1f}->{lp[k] * 100:.1f}cm at z{rest[E[k,0],2]:.2f}' for k in big[:8]))
    worst = {}
    for k in np.argsort(-np.abs(np.log(np.maximum(ratio, 1e-6))))[:400]:
        b = main[E[k, 0]]
        worst.setdefault(b, []).append(ratio[k])
    summary = ', '.join(f'{b} {min(v):.2f}-{max(v):.2f} ({len(v)})' for b, v in sorted(worst.items(), key=lambda x: -len(x[1]))[:6])
    print(f'UNPAID STRETCH {name}: edges past 1.5x {(ratio > 1.5).sum()}, under 0.6x {(ratio < 0.6).sum()}, '
          f'max {ratio.max():.2f}, min {ratio.min():.2f}; worst 400 by bone: {summary}')
pose({})
