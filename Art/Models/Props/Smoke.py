"""Chimney smoke for the tutorial island's houses (it sits on a chimney's SOCKET_Smoke). A scripted model (Art/README.md)
for M_Smoke:

  SmokePlume   four crossed cards round a curving column, 8 m tall: it leaves the chimney 0.6 m wide and nearly upright,
               then bends over toward the front (the level turns it downwind), 3 m out at the top, spreading to
               4.5 m wide and twisting a little as it rises. A straight, even column read as a white streak from the
               street; a plume that spreads and bends reads as smoke.

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

HEIGHT = 8.0
LEAN = 3.0          # meters toward the front (-Y) at the top
BASE, TOP = 0.6, 4.5  # the plume's width at the chimney and at the top (meters)
ROWS = (0.0, 0.04, 0.09, 0.16, 0.25, 0.36, 0.5, 0.66, 0.83, 1.0)


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
    """The column's middle at height fraction t: it rises nearly straight out of the chimney, then bends over downwind
    more and more as it rises (about 35 degrees from upright at the top), with a slight sideways S."""
    return Vector((0.3 * math.sin(t * math.pi * 1.2), -LEAN * t ** 1.9, HEIGHT * t))


def plume(mat):
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    col = bm.loops.layers.color.new('Col')
    for k in range(4):
        rows = []
        for t in ROWS:
            angle = math.radians(45.0 * k + 25.0 * t)          # the cards twist a little as the smoke rises
            across = Vector((math.cos(angle), math.sin(angle), 0.0))
            half = 0.5 * (BASE + (TOP - BASE) * t ** 0.85)
            alpha = smoothstep(0.0, 0.05, t) * (1.0 - smoothstep(0.35, 1.0, t))   # in at the chimney's lip, then thins
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
