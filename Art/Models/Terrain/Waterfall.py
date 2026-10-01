"""The tutorial island's waterfall, where the creek runs off the rim (Docs/TutorialIsland.md; Art/Levels/TutorialIsland
layout_computed.json "waterfall"). A scripted model (Art/README.md) for M_Waterfall:

  Waterfall       a sheet that lies on the creek's last meter, arcs off the lip and falls, then breaks into four
                  ragged strands that thin out 40-58 m down (the island's rock retreats below about 6 m, so the arc
                  clears it). Each strand is a bowed ribbon with a narrower crossed one, so it reads from the side.
  WaterfallMist   soft crossed cards of spray hanging at the lip.

Pivot: the middle of the lip, at the water surface; front (-Y) is the flow direction (the level turns it to the
creek's yaw). UV0: U across each ribbon (0..1), V along the flow in meters from the sheet's upstream end (the material
scrolls V). Vertex color (linear): R foam/whiteness, strong at the lip and where strands break off; A opacity, fading in
over the creek, out down the strands and toward their edges. Material Master = 'Waterfall' (no texture set). No Nanite,
no collision.

    blender -b --factory-startup --python Art/Models/Terrain/Waterfall.py -- --preview
"""
import math
import random

import bmesh
import bpy
from mathutils import Vector

import looter_textures as lt

V0 = 2.2          # m/s off the lip: enough to clear the rock face below it
G = 9.81
WIDTH = 2.4       # the creek's water width at the lip
LIFT = 0.015      # above the creek's surface, so the two don't fight


def material(name):
    """A material for the Waterfall master; its nodes only preview it in Blender (foam white over water, vertex
    alpha), the export reads Master."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    col = nodes.new('ShaderNodeVertexColor')
    col.layer_name = 'Col'
    split = nodes.new('ShaderNodeSeparateColor')
    links.new(col.outputs['Color'], split.inputs['Color'])
    mix = nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    mix.inputs['A'].default_value = lt.hex_color(0x5f8f97)
    mix.inputs['B'].default_value = lt.hex_color(0xf2f6f6)
    links.new(split.outputs['Red'], mix.inputs['Factor'])
    links.new(mix.outputs['Result'], bsdf.inputs['Base Color'])
    links.new(col.outputs['Alpha'], bsdf.inputs['Alpha'])
    bsdf.inputs['Roughness'].default_value = 0.2
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    mat.use_backface_culling = False
    mat['Master'] = 'Waterfall'
    return mat


def fall(t, v0=V0):
    """Where the water is t seconds after the lip (local y, z)."""
    return -v0 * t, LIFT - 0.5 * G * t * t


def path_length(t, v0=V0, steps=24):
    """Meters traveled from the lip in t seconds."""
    total, prev = 0.0, fall(0.0, v0)
    for k in range(1, steps + 1):
        p = fall(t * k / steps, v0)
        total += math.hypot(p[0] - prev[0], p[1] - prev[1])
        prev = p
    return total


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


class Ribbons:
    """Grids of quads; each vertex carries a UV and a color (foam, alpha)."""

    def __init__(self):
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')
        self.col = self.bm.loops.layers.color.new('Col')

    def grid(self, rows):
        """rows: [[(position, (u, v), (foam, alpha)), ...across], ...along]; quads between neighbours."""
        verts = [[(self.bm.verts.new(p), uv, c) for p, uv, c in row] for row in rows]
        for r0, r1 in zip(verts, verts[1:]):
            for j in range(len(r0) - 1):
                quad = (r0[j], r0[j + 1], r1[j + 1], r1[j])
                face = self.bm.faces.new([q[0] for q in quad])
                for loop, (_, uv, (foam, alpha)) in zip(face.loops, quad):
                    loop[self.uv].uv = uv
                    loop[self.col] = (foam, 0.0, 0.0, alpha)

    def finish(self, name, mat):
        # Faces look away from the cliff (local -Y) and down-flow; the material draws both sides anyway.
        for f in self.bm.faces:
            f.normal_update()
            if f.normal.y > 0.0:
                f.normal_flip()
        mesh = bpy.data.meshes.new(name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        mesh.materials.append(mat)
        for p in mesh.polygons:
            p.use_smooth = True
        # The values went in as stored (sRGB) bytes; write them again through the linear API, which is what the
        # exporter reads back (colors_type='LINEAR'), so the material gets the foam and alpha numbers as given.
        col = lt._col_attribute(mesh)
        raw = [tuple(d.color_srgb) for d in col.data]
        for d, value in zip(col.data, raw):
            d.color = value
        obj['Nanite'] = 0
        obj['Collision'] = 'None'
        return obj


def waterfall(mat):
    rib = Ribbons()
    rng = random.Random(54)
    # The sheet: over the creek's last meter (along its surface), off the lip, and down to about 6 m.
    upstream = [1.0, 0.7, 0.4, 0.2, 0.08]
    times = [0.0, 0.07, 0.14, 0.22, 0.31, 0.41, 0.52, 0.64, 0.77, 0.92, 1.08]
    cols = 9
    rows = []
    for y in upstream:
        v = 1.0 - y
        a = smoothstep(0.0, 0.5, v)
        foam = 0.12 + 0.35 * smoothstep(0.6, 1.0, v)
        rows.append([(Vector(((j / (cols - 1) - 0.5) * WIDTH, y, LIFT)), (j / (cols - 1), v),
                      (foam, a * edge(j, cols, 0.35))) for j in range(cols)])
    for t in times:
        y, z = fall(t)
        drop = LIFT - z
        v = 1.0 + path_length(t)
        width = WIDTH * (1.0 + 0.04 * drop)
        a = 1.0 - smoothstep(2.5, 5.8, drop)
        foam = min(1.0, 0.55 + 0.45 * smoothstep(0.0, 1.2, drop))
        rows.append([(Vector(((j / (cols - 1) - 0.5) * width, y, z)), (j / (cols - 1), v),
                      (foam, a * edge(j, cols, 0.3 + 0.4 * smoothstep(0.0, 3.0, drop)) * (0.9 + 0.1 * rng.random())))
                     for j in range(cols)])
    rib.grid(rows)

    # Four strands break off the sheet a meter down and fall apart, each a little faster or slower, swaying.
    strands = [(-0.85, 58.0, 2.3, 0.0), (-0.28, 46.0, 2.15, 1.7), (0.3, 52.0, 2.25, 3.1), (0.88, 40.0, 2.05, 4.4)]
    for x0, depth, v0, phase in strands:
        t_start, t_end = 0.45, math.sqrt(2.0 * depth / G)
        n = 22
        rows_a, rows_b = [], []
        for k in range(n + 1):
            f = (k / n) ** 1.6                      # rows bunch up near the top, where the shape changes fastest
            t = t_start + (t_end - t_start) * f
            y, z = fall(t, v0)
            drop = LIFT - z
            d = drop / depth
            x = x0 * (1.0 + 0.45 * d) + 0.35 * d * math.sin(phase + drop * 0.22) + 0.12 * math.sin(phase * 2.0 + drop * 0.7)
            half = 0.38 + 0.32 * d
            v = 1.0 + path_length(t, v0)
            alpha = smoothstep(0.0, 1.0, (drop - 1.0) / 2.0) * (1.0 - d ** 1.4) * (0.8 + 0.2 * math.sin(phase + drop * 1.3))
            foam = 1.0 - 0.35 * smoothstep(1.0, 10.0, drop)
            bow = 0.25 * half
            row_a, row_b = [], []
            for j, s in enumerate((-1.0, -0.35, 0.35, 1.0)):
                # The wide ribbon bows outward (toward -Y); the crossed one is narrower and runs along the flow.
                pa = Vector((x + s * half, y - bow * (1.0 - s * s), z))
                pb = Vector((x + 0.15 * s * half, y + s * half * 0.6, z))
                e = 1.0 - abs(s) ** 2 * 0.95
                row_a.append((pa, ((s + 1.0) * 0.5, v), (foam, alpha * e)))
                row_b.append((pb, ((s + 1.0) * 0.5, v), (foam, alpha * e * 0.8)))
            rows_a.append(row_a)
            rows_b.append(row_b)
        rib.grid(rows_a)
        rib.grid(rows_b)
    return rib.finish('Waterfall', mat)


def edge(j, cols, softness):
    """Alpha across a ribbon: 1 in the middle, falling to 1 - softness at its edges."""
    s = abs(j / (cols - 1) - 0.5) * 2.0
    return 1.0 - softness * s ** 2


def mist(mat):
    """Spray at the lip: four crossed cards, each a 3x3 grid whose middle is opaque and border clear."""
    rib = Ribbons()
    for k, (cx, angle, w, h, top) in enumerate([(0.0, 0.0, 3.4, 3.2, 0.6), (0.0, 60.0, 3.0, 3.0, 0.4),
                                                 (0.0, -60.0, 3.0, 3.0, 0.4), (0.4, 90.0, 2.4, 2.6, 0.2)]):
        a = math.radians(angle)
        across = Vector((math.cos(a), math.sin(a), 0.0))
        center = Vector((cx, -0.9, top - h * 0.5))
        rows = []
        for r in range(3):
            row = []
            for c in range(3):
                p = center + across * ((c - 1) * w * 0.5) + Vector((0.0, 0.0, (1 - r) * h * 0.5))
                inside = 1.0 if (r == 1 and c == 1) else 0.0
                row.append((p, (c * 0.5, r * 0.5), (0.9, 0.55 * inside)))
            rows.append(row)
        rib.grid(rows)
    return rib.finish('WaterfallMist', mat)


mat = material('Waterfall')
fall_obj = waterfall(mat)
mist_obj = mist(mat)
mist_obj.location.x = 5.0   # beside it in the scene; positions don't matter to the export
for obj in (fall_obj, mist_obj):
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    lt._log(f'{obj.name}: {tris} triangles')

if lt.want_preview():
    lt.preview([fall_obj], lt.preview_path('Terrain', 'Waterfall'), view=(-1.0, -1.2, 0.2), ground=False, fit=0.7)
