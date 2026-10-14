"""The waterfalls (layout_computed.json "waterfalls"; Docs/TutorialIsland.md, Docs/Areas/RansomsRest.md). A scripted
model (Art/README.md) for M_Waterfall, M_WaterMist and M_Water (the water contract, 2026-10-14):

  Waterfall          the creek's water rolling over a lip: it lies on the creek's last meter (fading in over it), bends
                     over the edge within about a meter (thrown off at V0, falling: no flat plank), clear and dark at
                     first, thinning and whitening as it breaks into four ragged strands that thin out 40-58 m down;
                     each strand a bowed ribbon with a narrower crossed one, so it reads from the side. The tutorial
                     island's falls off the rim (the island's rock retreats below about 6 m, so the arc clears it);
                     Mill Falls places it unscaled 14 m into Mill Gorge, whose floor hides the rest (squashed in Z to
                     its drop, the bend lay flat across the creek: the white plank).
  WaterfallMist      soft spray where the sheet breaks, 1-7 m below the lip.
  WaterfallTall      a fall off a hanging valley's lip down a canyon wall (Ransom's Rest's Larkspur Falls, 115 m off
                     Larkspur Bench's west end into Gravewind Canyon): the sheet rolling off the lip as Waterfall's,
                     then a widening cascade down the wall's lean (0.17 m out per m down), whitening, fraying into
                     spray over its lowest 25 m, with veils across it for side views. Designed for a 115 m drop: the
                     level scales it in Z to its own (layout_computed.json waterfalls with "canyon": true).
  WaterfallFootMist  the spray where a fall lands: soft crossed cards up to about 14 m and a low skirt of six petals
                     spreading 13 m out and rising as it goes (so it reads from above too). Pivot: the foot, on the
                     water; the level scales it to the fall.
  WaterfallFootFoam  a flat disc of churning foam where a fall lands, 3 m across its radius (the level scales it per
                     fall), heaped a little where the water lands, its rim ragged. Pivot: its middle, on the water.
  WaterfallPool      a plunge pool's water, 8 m across its radius: flat, its last 0.8 m curving 0.5 m down, so its rim
                     tucks under uneven ground. Pivot: its middle, at the water's level (Larkspur Falls' foot).

Pivot of the falls and the lip's mist: the middle of the lip, at the water surface; front (-Y) is the flow direction
(the level turns it to the creek's yaw). Vertex colours (linear: the numbers the materials read):
  the falls (material Waterfall): R foam/whiteness, G the meters below the lip in the model's own space / 128 (the
    material may scroll in meters however the level stretches it), A opacity: fading in over the creek, out down the
    strands and to zero at every ribbon's edge (ragged, row by row). UV0: U across each ribbon (0..1), V along the
    flow in meters from the sheet's upstream end (the material scrolls V).
  WaterfallFootFoam (material WaterfallFoam: the Waterfall master with its own Speed, Length, Across and
    DepthFadeDistance, so a card lying on the pool doesn't fade out): R foam, A opacity. U around the ring (FOAM_AROUND
    repeats), V meters out from the middle, so the foam streams outward.
  the mist (WaterfallMist, WaterfallFootMist; material WaterMist, master WaterMist): white, A opacity, a soft ragged
    falloff that is zero all round every card's border (no shape of its own: no diamonds). UV0 as SmokePlume's: U 0..1
    across each card, V 0..1 from where the spray comes from to its far end (up a standing card, outward along a skirt
    petal): M_WaterMist billows along V.
  WaterfallPool (material Water): as the terrain's water (Art/Levels/area_mesh.py water()): R the shore share (1 from
    where the rim starts down, 0 from 1.5 m in), G the depth share (rising to 1 in the middle), B = A = 0.5 (still).
    UV0 in meters as the terrain's (U along the model's Unreal Y, V along its X).
No Nanite, no collision.

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
SHEET_HALF = 1.55  # the sheet's half width at the lip: the creek's water and its soft edges, which lie over the banks
EDGE_SOFT = 0.26  # share of a sheet's half width its alpha eases in over at the lip (more lower down)
LIFT = 0.015      # above the creek's surface, so the two don't fight
UPSTREAM = (1.0, 0.7, 0.45, 0.25, 0.1, 0.0)  # meters before the lip the sheet's rows lie over the creek...
CREEK_SLOPE = 0.04  # ...rising upstream as the creek's water does over its last 9 m (area_shape.py: 3.5-4.6 cm a meter)
FOAM_RADIUS = 3.0  # WaterfallFootFoam's radius (m)
FOAM_AROUND = 8    # U repeats around the foam ring (about a sheet's width each)
POOL_RADIUS = 8.0  # WaterfallPool's radius (m)
POOL_EDGE = 0.8    # its rim curves down over this many meters...
POOL_DIP = 0.5     # ...this far


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def _sock(sockets, identifier):
    """A node socket by identifier (the Mix node has an A and a B per data type, all with the same name)."""
    return next(s for s in sockets if s.identifier == identifier)


def _color_mix(nodes, links, factor, a, b):
    """A Mix node (RGBA) from colour a to b by the factor socket; returns its result socket."""
    mix = nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    _sock(mix.inputs, 'A_Color').default_value = lt.hex_color(a)
    _sock(mix.inputs, 'B_Color').default_value = lt.hex_color(b)
    links.new(factor, _sock(mix.inputs, 'Factor_Float'))
    return _sock(mix.outputs, 'Result_Color')


def _effect(name, master, base, foam=None, **numbers):
    """A material for an effect master (vertex alpha the opacity); its nodes only preview it in Blender (foam white
    over the body by vertex R), the export reads Master and the numbers (the master's parameters)."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    col = nodes.new('ShaderNodeVertexColor')
    col.layer_name = 'Col'
    if foam is None:
        bsdf.inputs['Base Color'].default_value = lt.hex_color(base)
    else:
        split = nodes.new('ShaderNodeSeparateColor')
        links.new(col.outputs['Color'], split.inputs['Color'])
        links.new(_color_mix(nodes, links, split.outputs['Red'], base, foam), bsdf.inputs['Base Color'])
    links.new(col.outputs['Alpha'], bsdf.inputs['Alpha'])
    bsdf.inputs['Roughness'].default_value = 0.2
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED'
    mat.use_backface_culling = False
    mat['Master'] = master
    for key, value in numbers.items():
        mat[key] = float(value)
    return mat


def water_material():
    """The terrain's water material (Art/Levels/area_model.py water_material(): MI_Water, master Water); the same
    properties, so importing the pool leaves MI_Water as it is. Its nodes preview the depth tint by G."""
    mat = bpy.data.materials.get('Water') or bpy.data.materials.new('Water')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Roughness'].default_value = 0.08
    col = nodes.new('ShaderNodeVertexColor')
    col.layer_name = 'Col'
    split = nodes.new('ShaderNodeSeparateColor')
    links.new(col.outputs['Color'], split.inputs['Color'])
    links.new(_color_mix(nodes, links, split.outputs['Green'], 0x4fa39a, 0x1d5560), bsdf.inputs['Base Color'])
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    mat.diffuse_color = lt.hex_color(0x2f6f73)
    mat['Master'] = 'Water'
    mat['Kind'] = 'Surface'
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


class Ribbons:
    """Grids of quads; each vertex carries a UV and a colour (r, g, b, a)."""

    def __init__(self):
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')
        self.col = self.bm.loops.layers.color.new('Col')

    def grid(self, rows, closed=False):
        """rows: [[(position, (u, v), (r, g, b, a)), ...across], ...along]; quads between neighbours."""
        verts = [[(self.bm.verts.new(p), uv, c) for p, uv, c in row] for row in rows]
        for r0, r1 in zip(verts, verts[1:]):
            for j in range(len(r0) - 1):
                quad = (r0[j], r0[j + 1], r1[j + 1], r1[j])
                face = self.bm.faces.new([q[0] for q in quad])
                for loop, (_, uv, color) in zip(face.loops, quad):
                    loop[self.uv].uv = uv
                    loop[self.col] = color
        return verts

    def fan(self, center, outer):
        """Triangles from one new vertex (position, uv, colour) to a row grid() made (its return's first row)."""
        mid = (self.bm.verts.new(center[0]), center[1], center[2])
        for a, b in zip(outer, outer[1:]):
            face = self.bm.faces.new([mid[0], a[0], b[0]])
            for loop, (_, uv, color) in zip(face.loops, (mid, a, b)):
                loop[self.uv].uv = uv
                loop[self.col] = color

    def finish(self, name, mat, up=False, normals_up=False):
        """The mesh object. Faces look away from the cliff (local -Y) and down-flow, or with up, upward (the
        materials draw both sides of the falls and the mist anyway). normals_up: every normal straight up (water)."""
        for f in self.bm.faces:
            f.normal_update()
            if (f.normal.z < 0.0) if up else (f.normal.y > 0.0):
                f.normal_flip()
        mesh = bpy.data.meshes.new(name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        mesh.materials.append(mat)
        for p in mesh.polygons:
            p.use_smooth = True
        if normals_up:
            mesh.normals_split_custom_set_from_vertices([(0.0, 0.0, 1.0)] * len(mesh.vertices))
        # The values went in as stored (sRGB) bytes; write them again through the linear API, which is what the
        # exporter reads back (colors_type='LINEAR'), so the material gets the numbers as given.
        col = lt._col_attribute(mesh)
        raw = [tuple(d.color_srgb) for d in col.data]
        for d, value in zip(col.data, raw):
            d.color = value
        obj['Nanite'] = 0
        obj['Collision'] = 'None'
        return obj


def fall_color(foam, drop, alpha):
    """A falling ribbon's vertex colour: foam, the meters below the lip / 128, opacity."""
    return (min(max(foam, 0.0), 1.0), min(max(drop / 128.0, 0.0), 1.0), 0.0, min(max(alpha, 0.0), 1.0))


def ragged_edge(s, soft, wobble):
    """Alpha across a ribbon at s (-1..1): 1 in the middle, easing in from 0 at its edges over soft (a share of the
    half width), the ease moved in or out by wobble; exactly 0 on the edge itself."""
    if abs(s) >= 1.0 - 1e-6:
        return 0.0
    return smoothstep(0.0, soft, 1.0 - abs(s) + wobble)


def lip_rows(rng, cols, width_at, rows_after):
    """A sheet's rows: over the creek's last meter (UPSTREAM, on its surface, fading in) and then over the lip and down
    (rows_after: [(local y, local z, V meters, half width, alpha, foam, gaps, softness)]). Each row's edges wobble
    (ragged), and lower down its columns thin unevenly (gaps: the share an alpha may lose)."""
    rows = []
    across = [j / (cols - 1) * 2.0 - 1.0 for j in range(cols)]
    for y in UPSTREAM:
        v = 1.0 - y
        alpha = 0.9 * smoothstep(0.0, 1.0, v) ** 1.5
        half = width_at(0.0)
        rows.append([(Vector((s * half, y, LIFT + CREEK_SLOPE * y)), ((s + 1.0) * 0.5, v),
                      fall_color(0.02 * v, 0.0, alpha * ragged_edge(s, EDGE_SOFT, 0.0))) for s in across])
    for y, z, v, half, alpha, foam, gaps, soft in rows_after:
        drop = LIFT - z
        wobble = (rng.uniform(-0.06, 0.06), rng.uniform(-0.06, 0.06))
        stretch = (1.0 + rng.uniform(-0.04, 0.04) * smoothstep(0.0, 2.0, drop),
                   1.0 + rng.uniform(-0.04, 0.04) * smoothstep(0.0, 2.0, drop))
        row = []
        for s in across:
            side = 0 if s < 0.0 else 1
            x = s * half * (1.0 + (stretch[side] - 1.0) * s * s)
            a = alpha * ragged_edge(s, soft, wobble[side] * smoothstep(0.0, 1.5, drop)) * (1.0 - gaps * rng.random())
            row.append((Vector((x, y, z)), ((s + 1.0) * 0.5, v), fall_color(foam, drop, a)))
        rows.append(row)
    return rows


def waterfall(mat):
    rib = Ribbons()
    rng = random.Random(54)
    # The sheet: off the lip and down to about 6 m, rows close together over the bend.
    times = [0.03, 0.06, 0.1, 0.15, 0.21, 0.28, 0.36, 0.45, 0.55, 0.66, 0.78, 0.92, 1.08]
    after = []
    for t in times:
        y, z = fall(t)
        drop = LIFT - z
        after.append((y, z, 1.0 + path_length(t), SHEET_HALF * (1.0 + 0.04 * drop),
                      0.92 * (1.0 - smoothstep(2.2, 5.7, drop)) * (1.0 - 0.12 * smoothstep(0.4, 2.0, drop)),
                      0.03 + 0.87 * smoothstep(0.25, 4.5, drop), 0.1 + 0.3 * smoothstep(0.5, 4.0, drop),
                      EDGE_SOFT + 0.2 * smoothstep(0.0, 3.0, drop)))
    rib.grid(lip_rows(rng, 15, lambda d: SHEET_HALF, after))

    # Four strands break off the sheet a meter down and fall apart, each a little faster or slower, swaying.
    strands = [(-0.85, 58.0, 2.3, 0.0), (-0.28, 46.0, 2.15, 1.7), (0.3, 52.0, 2.25, 3.1), (0.88, 40.0, 2.05, 4.4)]
    wide = (-1.0, -0.67, -0.33, 0.0, 0.33, 0.67, 1.0)
    crossed = (-1.0, -0.5, 0.0, 0.5, 1.0)
    for x0, depth, v0, phase in strands:
        t_start, t_end = 0.45, math.sqrt(2.0 * depth / G)
        n = 26
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
            foam = 0.45 + 0.4 * smoothstep(1.0, 12.0, drop)
            bow = 0.25 * half
            wobble = rng.uniform(-0.08, 0.08)
            row_a, row_b = [], []
            for s in wide:
                # The wide ribbon bows outward (toward -Y), its edges ragged row by row.
                pa = Vector((x + s * half * (1.0 + wobble * abs(s)), y - bow * (1.0 - s * s), z))
                e = 0.0 if abs(s) > 0.99 else (1.0 - abs(s) ** 2) * (0.85 + 0.15 * rng.random())
                row_a.append((pa, ((s + 1.0) * 0.5, v), fall_color(foam, drop, alpha * e)))
            for s in crossed:
                # The crossed one is narrower and runs along the flow.
                pb = Vector((x + 0.15 * s * half, y + s * half * 0.6, z))
                e = 0.0 if abs(s) > 0.99 else (1.0 - abs(s) ** 2) * (0.85 + 0.15 * rng.random())
                row_b.append((pb, ((s + 1.0) * 0.5, v), fall_color(foam, drop, 0.8 * alpha * e)))
            rows_a.append(row_a)
            rows_b.append(row_b)
        rib.grid(rows_a)
        rib.grid(rows_b)
    return rib.finish('Waterfall', mat)


def soft_card(rib, rng, center, across, up, width, height, peak, profile, bulge=0.0, cols=9, rows=7):
    """A mist card: a grid of cols x rows over width x height (across, up: its unit axes; center: its bottom middle),
    alpha peak times a soft falloff to zero at every border vertex, ragged by a noise per vertex: across (1 - its
    distance from the middle, eased) and up by profile(v). bulge pushes its middle out along its normal (meters), so
    crossed cards look rounder. UV: U 0..1 across, V 0..1 up."""
    normal = across.cross(up).normalized()
    out = []
    for r in range(rows):
        v = r / (rows - 1)
        row = []
        for c in range(cols):
            u = c / (cols - 1)
            h = abs(u - 0.5) * 2.0
            border = r in (0, rows - 1) or c in (0, cols - 1)
            noise = rng.uniform(-0.12, 0.12)
            alpha = 0.0 if border else peak * (1.0 - smoothstep(0.35, 1.0, h + noise)) * profile(v + 0.5 * noise)
            p = (center + across * ((u - 0.5) * width) + up * (v * height)
                 + normal * (bulge * (1.0 - h * h) * math.sin(math.pi * v)))
            row.append((p, (u, v), (1.0, 1.0, 1.0, max(alpha, 0.0))))
        out.append(row)
    rib.grid(out)


def mist(mat):
    """WaterfallMist: spray where the sheet breaks, 1-7 m below the lip, densest 2-4 m down (the sheet is still clear
    at the lip). Crossed soft cards in front of the falling water."""
    rib = Ribbons()
    rng = random.Random(71)

    def profile(v):
        return smoothstep(0.0, 0.3, v) * (1.0 - smoothstep(0.55, 1.0, v))

    for cx, cy, top, angle, w, h, peak in [(0.0, -1.7, -0.8, 0.0, 4.4, 6.0, 0.45), (0.0, -1.5, -1.0, 60.0, 3.6, 5.2, 0.4),
                                           (0.0, -1.5, -1.0, -60.0, 3.6, 5.2, 0.4), (0.3, -2.0, -1.4, 90.0, 3.2, 5.0, 0.35)]:
        a = math.radians(angle)
        across = Vector((math.cos(a), math.sin(a), 0.0))
        soft_card(rib, rng, Vector((cx, cy, top - h)), across, Vector((0.0, 0.0, 1.0)), w, h, peak, profile,
                  bulge=0.12 * w)
    return rib.finish('WaterfallMist', mat)


TALL_DROP = 115.0  # WaterfallTall's design drop (m): Larkspur Falls off the Bench's west end; the level scales it in Z
BATTER = 0.17      # the canyon wall under an escarpment's lip leans out this far per meter down (area_region.py)
CLEAR = 2.2        # meters the tall fall keeps off that lean (the wall's ledges and ribs reach out about that far)


def tall_offset(drop):
    """How far out from the lip (local -Y, meters) the tall fall is, drop meters down: thrown off the lip, then down
    the canyon wall's lean, a little clear of it."""
    t = math.sqrt(2.0 * max(drop, 0.0) / G)
    wall = 0.3 + BATTER * drop + CLEAR * smoothstep(0.0, 8.0, drop)
    return -max(V0 * t, wall)


def waterfall_tall(mat):
    """WaterfallTall: a stream off a hanging valley's lip and TALL_DROP m down the canyon wall (Larkspur Falls): the
    sheet over the creek's last meter, rolling off the lip, then a widening cascade down the wall's lean, whitening as
    it goes, that frays into spray over its lowest 25 m; three veils across it (in the flow's plane) so it reads from
    the side, as from the burial deck to its south-west. Pivot, axes, UVs and colours as Waterfall's; the level scales
    it in Z to the drop."""
    rib = Ribbons()
    rng = random.Random(115)
    drops = [0.005, 0.05, 0.15, 0.3, 0.5, 0.8, 1.2, 1.8, 2.6, 3.6, 5.0, 7.0, 9.5, 13.0, 18.0, 24.0, 31.0, 39.0, 48.0,
             57.0, 66.0, 75.0, 84.0, 92.0, 99.0, 105.0, 110.0, 113.0, 115.0]
    length, prev = 1.0, Vector((0.0, 0.0, LIFT))
    after, rows_v = [], []
    for drop in drops:
        p = Vector((0.0, tall_offset(drop), LIFT - drop))
        length += (p - prev).length
        prev = p
        share = drop / TALL_DROP
        half = SHEET_HALF + 3.25 * share ** 0.8
        alpha = 0.92 * (1.0 - smoothstep(TALL_DROP - 25.0, TALL_DROP, drop)) * (1.0 - 0.12 * smoothstep(0.4, 2.0, drop))
        foam = 0.03 + 0.87 * smoothstep(0.3, 9.0, drop)
        after.append((p.y, p.z, length, half, alpha, foam, 0.1 + 0.3 * smoothstep(1.0, 20.0, drop),
                      EDGE_SOFT + 0.25 * smoothstep(0.0, 30.0, drop)))
        rows_v.append((p, 2.0 * half, length, alpha, foam, share))
    rows = lip_rows(rng, 13, lambda d: SHEET_HALF, after)
    # The cascade ripples a little out of its plane as it goes down the wall.
    for k, row in enumerate(rows[len(UPSTREAM):]):
        drop = drops[k]
        for j, (p, uv, c) in enumerate(row):
            p.y -= 0.25 * (drop / TALL_DROP) * math.sin(j * 1.3 + drop * 0.1)
    rib.grid(rows)
    # The veils: spray standing out from the wall across the cascade, so a view along the wall still sees it.
    for x_share in (-0.3, 0.0, 0.3):
        veil = []
        for p, width, length, alpha, foam, share in rows_v:
            depth = 0.6 + 3.2 * share
            drop = LIFT - p.z
            veil.append([(Vector((x_share * width, p.y + s * depth, p.z)), ((s + 1.0) * 0.5, length),
                          fall_color(foam, drop, 0.0 if abs(s) > 0.99 else
                                     0.55 * alpha * (1.0 - abs(s) ** 2 * 0.9) * smoothstep(1.0, 6.0, -p.y)
                                     * (0.85 + 0.15 * rng.random())))
                         for s in (-1.0, -0.67, -0.33, 0.0, 0.33, 0.67, 1.0)])
        rib.grid(veil)
    return rib.finish('WaterfallTall', mat)


def foot_mist(mat):
    """WaterfallFootMist: the spray where a tall fall lands, billowing out rather than falling: soft crossed cards up
    to about 14 m, densest low down, and a skirt of six overlapping petals 13 m out, rising from 0.3 to 2.5 m as it
    spreads. Pivot: the foot, on the water."""
    rib = Ribbons()
    rng = random.Random(288)

    def profile(v):
        return smoothstep(0.0, 0.12, v) * (1.0 - smoothstep(0.25, 1.0, v))

    for angle, w, h, peak in [(0.0, 20.0, 14.0, 0.5), (60.0, 16.0, 12.0, 0.42), (-60.0, 16.0, 12.0, 0.42),
                              (90.0, 12.0, 10.0, 0.38)]:
        a = math.radians(angle)
        soft_card(rib, rng, Vector((0.0, 0.0, -0.5)), Vector((math.cos(a), math.sin(a), 0.0)),
                  Vector((0.0, 0.0, 1.0)), w, h, peak, profile, bulge=0.1 * w)
    radii = (0.8, 2.0, 3.5, 5.0, 6.8, 8.6, 10.6, 13.0)
    spread = math.radians(45.0)
    for petal in range(6):
        middle = math.radians(60.0 * petal + 15.0)
        rows = []
        for r_index, r in enumerate(radii):
            share = r / radii[-1]
            row = []
            for c in range(7):
                u = c / 6.0
                a = middle + (u - 0.5) * 2.0 * spread
                noise = rng.uniform(-0.1, 0.1)
                border = r_index in (0, len(radii) - 1) or c in (0, 6)
                alpha = 0.0 if border else (0.42 * smoothstep(0.0, 0.2, share)
                                            * (1.0 - smoothstep(0.35, 1.0, share + noise))
                                            * (1.0 - smoothstep(0.5, 1.0, abs(u - 0.5) * 2.0 + noise)))
                row.append((Vector((math.cos(a) * r, math.sin(a) * r, 0.3 + 2.2 * share ** 1.3)),
                            (u, r_index / (len(radii) - 1)), (1.0, 1.0, 1.0, max(alpha, 0.0))))
            rows.append(row)
        rib.grid(rows)
    return rib.finish('WaterfallFootMist', mat)


def _around(rng, count, terms=3):
    """A smooth random closed curve's values (about -1..1) at count points around a circle."""
    waves = [(rng.randint(2, 7), rng.uniform(0.0, 2.0 * math.pi), rng.uniform(0.5, 1.0)) for _ in range(terms)]
    total = sum(w for _, _, w in waves)
    return [sum(w * math.sin(k * 2.0 * math.pi * i / count + ph) for k, ph, w in waves) / total for i in range(count)]


def foot_foam(mat):
    """WaterfallFootFoam: churning foam on the water where a fall lands, FOAM_RADIUS across its radius, its rim ragged
    (a smooth wobble round it and a little per vertex), densest in a ring round the middle, heaped 13 cm where the
    water lands (3 cm at the rim). Pivot: its middle, on the water's surface."""
    rib = Ribbons()
    rng = random.Random(300)
    segments = 40
    reach = [0.82 + 0.18 * w for w in _around(rng, segments)]
    reach.append(reach[0])
    radii = (0.12, 0.35, 0.7, 1.1, 1.5, 1.9, 2.3, 2.65, 3.0)
    rows = []
    for r_index, r in enumerate(radii):
        row = []
        for k in range(segments + 1):
            a = 2.0 * math.pi * k / segments
            rn = r / (FOAM_RADIUS * reach[k]) + rng.uniform(-0.04, 0.04)
            outer = r_index == len(radii) - 1
            alpha = 0.0 if outer else ((0.7 + 0.25 * smoothstep(0.0, 0.35, rn)) * (1.0 - smoothstep(0.55, 1.0, rn)))
            foam = 1.0 - 0.35 * smoothstep(0.2, 1.0, rn)
            z = 0.03 + 0.1 * (1.0 - min(r / FOAM_RADIUS, 1.0)) ** 2
            row.append((Vector((math.cos(a) * r, math.sin(a) * r, z)), (FOAM_AROUND * k / segments, r),
                        (foam, 0.0, 0.0, max(alpha, 0.0))))
        rows.append(row)
    made = rib.grid(rows)
    # The middle: a triangle per segment, each with a point of its own there (U the segment's), so U stays unbroken.
    first = made[0]
    for k in range(segments):
        u = FOAM_AROUND * (k + 0.5) / segments
        rib.fan((Vector((0.0, 0.0, 0.13)), (u, 0.0), (1.0, 0.0, 0.0, 0.7)), first[k:k + 2])
    return rib.finish('WaterfallFootFoam', mat, up=True)


def pool(mat):
    """WaterfallPool: a plunge pool's water, POOL_RADIUS across its radius, flat to POOL_EDGE from its rim, then
    curving POOL_DIP down, so the rim tucks under the ground round it (the canyon's floor is uneven and has no
    collision to fit to). Colours as the terrain's water: the shore share from where the rim starts down, the depth
    share rising to the middle, still. Pivot: its middle at the water's level."""
    rib = Ribbons()
    segments = 48
    flat = POOL_RADIUS - POOL_EDGE
    radii = (0.8, 1.8, 3.0, 4.2, 5.4, 6.4, flat, flat + 0.4, flat + 0.65, POOL_RADIUS)

    def vertex(r, a):
        z = -POOL_DIP * max(0.0, (r - flat) / POOL_EDGE) ** 2
        x, y = math.cos(a) * r, math.sin(a) * r
        shore = 1.0 - min(max((flat - r) / 1.5, 0.0), 1.0)
        depth = 1.0 - smoothstep(1.0, flat, r)
        # UV in meters as the terrain's water: U along the model's Unreal Y (-Blender X), V along its X (-Blender Y).
        return Vector((x, y, z)), (-x, -y), (shore, depth, 0.5, 0.5)

    # Each ring closes on a copy of its first vertex (the same place, UV and colour; every normal is straight up).
    rows = [[vertex(r, 2.0 * math.pi * k / segments) for k in range(segments + 1)] for r in radii]
    made = rib.grid(rows)
    rib.fan(vertex(0.0, 0.0), made[0])
    return rib.finish('WaterfallPool', mat, up=True, normals_up=True)


fall_mat = _effect('Waterfall', 'Waterfall', 0x6f9fa3, foam=0xeef5f2)
foam_mat = _effect('WaterfallFoam', 'Waterfall', 0x6f9fa3, foam=0xeef5f2, Speed=1.2, Length=3.0, Across=4.0,
                   DepthFadeDistance=6.0)
mist_mat = _effect('WaterMist', 'WaterMist', 0xe4ecea)
fall_obj = waterfall(fall_mat)
mist_obj = mist(mist_mat)
mist_obj.location.x = 5.0   # beside it in the scene; positions don't matter to the export
tall_obj = waterfall_tall(fall_mat)
tall_obj.location.x = 15.0
foot_obj = foot_mist(mist_mat)
foot_obj.location.x = 40.0
foam_obj = foot_foam(foam_mat)
foam_obj.location.x = 70.0
pool_obj = pool(water_material())
pool_obj.location.x = 90.0
for obj in (fall_obj, mist_obj, tall_obj, foot_obj, foam_obj, pool_obj):
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    lt._log(f'{obj.name}: {tris} triangles')


def _cropped(obj, below):
    """A temporary copy of obj without its vertices under the height below (m): a falls cut where a gorge's floor
    would hide it, for the previews."""
    copy = bpy.data.objects.new('PreviewCut' + obj.name, obj.data.copy())
    bpy.context.scene.collection.objects.link(copy)
    bm = bmesh.new()
    bm.from_mesh(copy.data)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.z < below], context='VERTS')
    bm.to_mesh(copy.data)
    bm.free()
    return copy


def previews():
    """Saved/ArtPreviews/Terrain/: Waterfall (the island's falls and its spray), Waterfall_Lip (the bend over the edge,
    close), Waterfall_MillFalls (cut at Mill Gorge's 14 m floor, with the foam and the foot's spray scaled as the level
    places them), WaterfallTall and WaterfallTall_Lip, and WaterfallFoot (Larkspur Falls' foot: the pool, the foam,
    the spray)."""
    out = lambda name: lt.preview_path('Terrain', name)  # noqa: E731
    mist_obj.location = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()  # the previews frame by the objects' world matrices
    lt.preview([fall_obj, mist_obj], out('Waterfall'), view=(-1.0, -1.2, 0.2), ground=False, fit=1.15)
    lip = _cropped(fall_obj, -2.5)
    lt.preview([lip], out('Waterfall_Lip'), view=(-0.8, -1.0, 0.45), ground=False, fit=0.75)
    mill = _cropped(fall_obj, -14.3)
    foam_obj.location = (0.0, -3.75, -14.25)
    foam_obj.scale = (1.0, 1.0, 1.0)
    foot_obj.location = (0.0, -3.75, -14.25)
    foot_obj.scale = (0.35, 0.35, 0.35)
    bpy.context.view_layer.update()
    lt.preview([mill, mist_obj, foam_obj, foot_obj], out('Waterfall_MillFalls'), view=(-0.6, -1.4, 0.15), fit=0.8)
    lt.preview([tall_obj], out('WaterfallTall'), view=(-1.0, -1.2, 0.2), ground=False, fit=1.15)
    tall_lip = _cropped(tall_obj, -3.0)
    lt.preview([tall_lip], out('WaterfallTall_Lip'), view=(-0.8, -1.0, 0.45), ground=False, fit=0.75)
    bpy.data.objects.remove(tall_lip)
    for obj in (pool_obj, foam_obj, foot_obj):
        obj.location = (200.0, 0.0, 0.0)
    foam_obj.scale = (2.3, 2.3, 1.0)
    foot_obj.scale = (1.0, 1.0, 1.0)
    bpy.context.view_layer.update()
    lt.preview([pool_obj, foam_obj, foot_obj], out('WaterfallFoot'), view=(-0.7, -1.2, 0.55), fit=0.8)
    for temporary in (lip, mill):
        data = temporary.data
        bpy.data.objects.remove(temporary)
        bpy.data.meshes.remove(data)
    # As they were: the export applies an object's scale.
    for obj, x in ((mist_obj, 5.0), (foot_obj, 40.0), (foam_obj, 70.0), (pool_obj, 90.0)):
        obj.location = (x, 0.0, 0.0)
        obj.scale = (1.0, 1.0, 1.0)


if lt.want_preview():
    previews()
