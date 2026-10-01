"""Chimney smoke for the tutorial island's houses (it sits on a chimney's SOCKET_Smoke). A scripted model (Art/README.md)
for M_Smoke:

  SmokePlume   four crossed cards round a gently curving column, 1 m wide at the chimney widening to 3 m, 7 m tall,
               leaning toward the front (the level turns it downwind) and twisting a little as it rises.

Pivot: the middle of the base. UV0: U 0..1 across each card, V 0..1 from bottom to top. Vertex color (linear): white,
A fading in over the bottom 10% and out toward the top. Material Master = 'Smoke' (no texture set). No Nanite, no
collision.

    blender -b --factory-startup --python Art/Models/Props/Smoke.py -- --preview
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_textures as lt

HEIGHT = 7.0
LEAN = 1.6          # meters toward the front (-Y) at the top
ROWS = (0.0, 0.05, 0.1, 0.22, 0.38, 0.56, 0.76, 1.0)


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def material(name):
    """A material for the Smoke master; the nodes only preview it (grey, vertex alpha), the export reads Master."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    col = nodes.new('ShaderNodeVertexColor')
    col.layer_name = 'Col'
    bsdf.inputs['Base Color'].default_value = lt.hex_color(0xb9b6b0)
    links.new(col.outputs['Alpha'], bsdf.inputs['Alpha'])
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    mat.use_backface_culling = False
    mat['Master'] = 'Smoke'
    return mat


def center(t):
    """The column's middle at height fraction t: leaning forward more as it rises, with a slight sideways S."""
    return Vector((0.25 * math.sin(t * math.pi * 1.2), -LEAN * t ** 1.5, HEIGHT * t))


def plume(mat):
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    col = bm.loops.layers.color.new('Col')
    for k in range(4):
        rows = []
        for t in ROWS:
            angle = math.radians(45.0 * k + 25.0 * t)          # the cards twist a little as the smoke rises
            across = Vector((math.cos(angle), math.sin(angle), 0.0))
            half = 0.5 + 1.0 * t ** 0.8
            alpha = smoothstep(0.0, 0.1, t) * (1.0 - smoothstep(0.4, 1.0, t))
            c = center(t)
            rows.append([(bm.verts.new(c + across * (s * half)), ((s + 1.0) * 0.5, t), alpha) for s in (-1.0, 0.0, 1.0)])
        for r0, r1 in zip(rows, rows[1:]):
            for j in range(2):
                quad = (r0[j], r0[j + 1], r1[j + 1], r1[j])
                face = bm.faces.new([q[0] for q in quad])
                for loop, (_, (u, v), a) in zip(face.loops, quad):
                    loop[uv].uv = (u, v)
                    loop[col] = (1.0, 1.0, 1.0, a)
    mesh = bpy.data.meshes.new('SmokePlume')
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new('SmokePlume', mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.materials.append(mat)
    for p in mesh.polygons:
        p.use_smooth = True
    # Written as stored bytes; rewrite through the linear API, which is what the exporter reads back.
    attr = lt._col_attribute(mesh)
    raw = [tuple(d.color_srgb) for d in attr.data]
    for d, value in zip(attr.data, raw):
        d.color = value
    obj['Nanite'] = 0
    obj['Collision'] = 'None'
    return obj


smoke = plume(material('Smoke'))
lt._log(f'SmokePlume: {sum(len(p.vertices) - 2 for p in smoke.data.polygons)} triangles')
if lt.want_preview():
    lt.preview([smoke], lt.preview_path('Props', 'SmokePlume'), view=(-1.0, -0.6, 0.15), ground=False, fit=1.4)
