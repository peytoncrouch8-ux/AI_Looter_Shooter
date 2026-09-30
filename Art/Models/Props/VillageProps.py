"""Village props for the tutorial island: lamp post, laundry line, bench, campfire and signpost. A scripted model (see
Art/README.md); the props are built from parts, UV-mapped on the house trim sheet or a tileable, and merged.

  LampPost      2.75 m hewn post with an arm and a brace; an iron lantern hangs toward the front (-Y), lit glass inside
                (LanternGlow, the same glowing material as the old LanternPost). SOCKET_Light at the lantern. No
                Nanite: a small prop.
  LaundryLine   two T-posts 3.6 m apart, two sagging lines, a sheet, a shirt, a towel and a rag pegged on the front
                line. Collision on the posts only.
  Bench         1.7 m plank bench on two trestles (no Nanite: a small prop)
  CampfireRing  a ring of stones round an ash bed and charred logs (RockGranite, Charcoal). SOCKET_Fire at the fire.
  Signpost      2.4 m post with two arrow boards in faded teal and oxide-red paint (no Nanite: a small prop)

The pivot is on the ground at the middle of the footprint (the post's foot for the lamp post and the signpost).
"""
import math
import random

import bmesh
import bpy
from mathutils import Euler, Vector, noise

import looter_model as lm
import looter_textures as lt
import looter_props as lp


# Lit glass is an opaque, emissive surface in the game's stylized material, shared with the old LanternPost (Nanite
# can't draw the additive Glow kind).
GLOW = lm.material('LanternGlow', 0xffc56e, Glow=5.0, Variation=0.04)
GRANITE = lt.material('RockGranite', MossAmount=0.6)   # moss on up-facing stone (M_World), as on the boulders
CHARCOAL = lt.material('BarkOak', name='Charcoal', tint=0x4a4440)


def beam(size, center, axis, seed, strip='Beams', rotation=(0.0, 0.0, 0.0), bevel=0.0):
    """A squared timber (a box) with its grain along axis ('x', 'y' or 'z' before rotation), turned about its centre."""
    part = lp.block(size, (0.0, 0.0, 0.0), bevel=bevel)
    vector = {'x': (1.0, 0.0, 0.0), 'y': (0.0, 1.0, 0.0), 'z': (0.0, 0.0, 1.0)}[axis]
    lp.grain(part, strip, axis=vector, seed=seed)
    return lp.place(part, center, rotation)


def strut(p0, p1, width, seed, strip='Beams'):
    """A squared timber from p0 to p1."""
    part = lp.sweep([p0, p1], [(-width * 0.5, -width * 0.5), (width * 0.5, -width * 0.5), (width * 0.5, width * 0.5),
                            (-width * 0.5, width * 0.5)])
    return lp.grain(part, strip, axis=Vector(p1) - Vector(p0), seed=seed)


# --- Lamp post ---

def lamp_post(name, seed):
    rnd = random.Random(seed)
    top = 2.75
    post = lp.block((0.16, 0.16, top + 0.15), (0.0, 0.0, (top - 0.15) * 0.5), bevel=0.015)
    lp.grain(post, 'Beams', axis=(0.0, 0.0, 1.0), seed=seed)
    lp.slice_at(post, (0.0, 0.0, 1.0), (0.03, 0.5))
    lp.rough(post, 0.005, 5.0, seed)
    parts = [post]
    parts.append(beam((0.1, 0.78, 0.11), (0.0, -0.33, top - 0.12), 'y', seed + 1, bevel=0.01))
    parts.append(strut((0.0, -0.06, top - 0.62), (0.0, -0.44, top - 0.18), 0.07, seed + 2))
    for y, z in ((-0.075, top - 0.12), (-0.3, top - 0.23)):   # big nails where the arm and brace meet
        parts.append(lp.nail((0.055, y, z), (1.0, 0.0, 0.0)))
    # The iron: a strap over the arm's end, a hook, and the lantern hanging from it.
    hang = Vector((0.0, -0.64, top - 0.18))
    strap = lp.block((0.12, 0.05, 0.13), (0.0, hang.y, top - 0.12))
    parts.append(lp.grain(strap, 'Iron', axis=(1.0, 0.0, 0.0), seed=seed + 3))
    parts.append(strut(hang, hang - Vector((0.0, 0.0, 0.1)), 0.016, seed + 4, strip='Iron'))
    lantern_top = hang.z - 0.1
    cap, _ = lp.lathe([(0.0, lantern_top + 0.0), (0.17, lantern_top - 0.09), (0.17, lantern_top - 0.11),
                    (0.0, lantern_top - 0.11)], segments=4)
    lt.assign(cap, lp.trim_material())
    lp.lathe_uv(cap, 'Iron', seed=seed)
    lp.place(cap, (0.0, 0.0, 0.0), (0.0, 0.0, 45.0))
    lp.place(cap, (hang.x, hang.y, 0.0))
    parts.append(cap)
    body_top, body_bottom = lantern_top - 0.11, lantern_top - 0.41
    glass = lp.block((0.19, 0.19, body_top - body_bottom - 0.02), (hang.x, hang.y, (body_top + body_bottom) * 0.5))
    lt.assign(glass, GLOW)
    lt.box_uv(glass, 'HouseTrim')    # the glow needs no texture, but tangents need UVs
    parts.append(glass)
    for dx in (-0.1, 0.1):
        for dy in (-0.1, 0.1):
            parts.append(strut((hang.x + dx, hang.y + dy, body_bottom), (hang.x + dx, hang.y + dy, body_top), 0.022,
                               rnd.randint(0, 999), strip='Iron'))
    for z in (body_top - 0.012, body_bottom + 0.012):
        ring = lp.block((0.23, 0.23, 0.024), (hang.x, hang.y, z))
        parts.append(lp.grain(ring, 'Iron', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
    base = lp.block((0.25, 0.25, 0.04), (hang.x, hang.y, body_bottom - 0.02))
    parts.append(lp.grain(base, 'Iron', axis=(1.0, 0.0, 0.0), seed=seed + 5))
    obj = lp.join(name, parts)
    lp.hull_box(obj, (0.2, 0.2, top), (0.0, 0.0, top * 0.5))
    lp.hull_box(obj, (0.28, 0.28, lantern_top - body_bottom + 0.06), (hang.x, hang.y, (lantern_top + body_bottom) * 0.5 - 0.03))
    lm.socket(obj, 'Light', location=(hang.x, hang.y, (body_top + body_bottom) * 0.5))
    return lp.finish(obj, ao=0.4, nanite=False)   # under 300 triangles


# --- Laundry line ---

def cloth_uv(obj, strip, x0, x1):
    """Maps a hanging cloth onto a trim strip: along the line at world scale, its whole drop fitted into the strip."""
    key = lt.TRIM_ALIASES.get(strip, strip)
    v_lo, v_hi = lt.TRIM_STRIPS[key]
    scale = lt.TRIM_DENSITY / lt.TRIM_SIZE
    zs = [v.co.z for v in obj.data.vertices]
    z0, z1 = min(zs), max(zs)
    uv = obj.data.uv_layers.active.data
    for p in obj.data.polygons:
        for loop in p.loop_indices:
            co = obj.data.vertices[obj.data.loops[loop].vertex_index].co
            t = (co.z - z0) / max(z1 - z0, 1e-4)
            uv[loop].uv = ((co.x - x0 + 1.2) * scale, v_lo + 0.002 + t * (v_hi - v_lo - 0.004))
    return obj


def line_height(x, span, top, sag):
    t = x / span
    return top - sag * (1.0 - 4.0 * t * t)


def cloth(x0, x1, drop, line_y, span, top, sag, seed, strip, columns=6, rows=4):
    """A cloth hanging from the line between x0 and x1: folds, a little billow, both sides (two thin layers)."""
    rnd = random.Random(seed)
    bm = lp.new_bmesh()
    grid = []
    for j in range(rows + 1):
        row = []
        for i in range(columns + 1):
            u, v = i / columns, j / rows
            x = x0 + (x1 - x0) * u
            z_top = line_height(x, span, top, sag) - 0.01
            fold = 0.035 * math.sin(u * math.pi * (columns * 0.5) + rnd.uniform(-0.3, 0.3)) * (0.3 + v)
            billow = 0.07 * v ** 1.5
            ragged = 0.03 * noise.noise(Vector((x * 7.0, seed, 0.5))) * v
            row.append(Vector((x, line_y + fold + billow, z_top - drop * v + ragged)))
        grid.append(row)
    for offset in (0.0, 0.006):
        verts = [[bm.verts.new(p + Vector((0.0, offset, 0.0))) for p in row] for row in grid]
        for j in range(rows):
            for i in range(columns):
                quad = (verts[j][i], verts[j][i + 1], verts[j + 1][i + 1], verts[j + 1][i])
                # The front layer faces the front (-Y), the back layer the back: each layer's back is hidden.
                bm.faces.new(tuple(reversed(quad)) if offset == 0.0 else quad)
    part = lp.mesh_object(bm)
    lt.assign(part, lp.trim_material())
    return cloth_uv(part, strip, x0, x1)


def laundry_line(name, seed):
    rnd = random.Random(seed)
    span, top, sag = 3.6, 2.0, 0.16
    parts = []
    for side in (-1.0, 1.0):
        x = side * span * 0.5
        post = lp.block((0.12, 0.12, 2.25), (0.0, 0.0, 0.975))
        lp.grain(post, 'Siding', axis=(0.0, 0.0, 1.0), seed=rnd.randint(0, 999))
        lp.slice_at(post, (0.0, 0.0, 1.0), (0.03, 0.5))
        lp.rough(post, 0.005, 5.0, rnd.randint(0, 999))
        lp.place(post, (x, 0.0, 0.0), (0.0, side * rnd.uniform(1.0, 2.5), rnd.uniform(-4.0, 4.0)))
        parts.append(post)
        parts.append(beam((0.08, 0.7, 0.08), (x, 0.0, top + 0.04), 'y', rnd.randint(0, 999), strip='Siding'))
        parts.append(strut((x, -0.05, top - 0.35), (x, -0.28, top - 0.01), 0.05, rnd.randint(0, 999), strip='Siding'))
        parts.append(strut((x, 0.05, top - 0.35), (x, 0.28, top - 0.01), 0.05, rnd.randint(0, 999), strip='Siding'))
    for line_y in (-0.28, 0.28):
        pts = [(-span * 0.5, line_y, top + 0.005)]
        for k in range(1, 12):
            x = -span * 0.5 + span * k / 12
            pts.append((x, line_y, line_height(x, span, top, sag)))
        pts.append((span * 0.5, line_y, top + 0.005))
        rope = lp.sweep(pts, lp.ngon(0.008, 4, start=math.pi / 4))
        parts.append(lp.grain(rope, 'Plaster', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
    line_y = -0.28
    cloths = [(-1.45, -0.55, 0.78, 'Plaster'), (-0.35, 0.15, 0.58, 'TrimTeal'), (0.35, 0.8, 0.5, 'TrimRed'),
              (1.0, 1.25, 0.32, 'Plaster')]
    for x0, x1, drop, strip in cloths:
        parts.append(cloth(x0, x1, drop, line_y - 0.012, span, top, sag, rnd.randint(0, 999), strip))
        for x in (x0 + 0.03, x1 - 0.03):
            peg = lp.block((0.018, 0.022, 0.07), (x, line_y - 0.005, line_height(x, span, top, sag) - 0.01),
                        (0.0, 0.0, rnd.uniform(-8.0, 8.0)))
            parts.append(lp.grain(peg, 'Siding', axis=(0.0, 0.0, 1.0), seed=rnd.randint(0, 999)))
    obj = lp.join(name, parts)
    for side in (-1.0, 1.0):
        lp.hull_box(obj, (0.16, 0.16, top + 0.1), (side * span * 0.5, 0.0, (top + 0.1) * 0.5))
    return lp.finish(obj, ao=0.4, fallback=100)


# --- Bench ---

def bench(name, seed):
    rnd = random.Random(seed)
    length, seat_z = 1.7, 0.46
    parts = []
    for y in (-0.092, 0.092):
        plank = lp.block((length + rnd.uniform(-0.02, 0.02), 0.175, 0.05), (rnd.uniform(-0.01, 0.01), y, seat_z - 0.025))
        lp.grain(plank, 'Siding', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999))
        lp.slice_at(plank, (1.0, 0.0, 0.0), (-0.66, -0.58, 0.58, 0.66))
        parts.append(lp.rough(plank, 0.004, 4.0, rnd.randint(0, 999)))
        for x in (-0.62, 0.62):
            parts.append(lp.nail((x, y, seat_z + 0.001), (0.0, 0.0, 1.0), size=0.022))
    for x in (-0.62, 0.62):
        for side in (-1.0, 1.0):
            parts.append(strut((x, side * 0.09, seat_z - 0.05), (x, side * 0.2, 0.0), 0.06, rnd.randint(0, 999)))
        parts.append(beam((0.05, 0.36, 0.06), (x, 0.0, 0.2), 'y', rnd.randint(0, 999)))
        parts.append(beam((0.07, 0.3, 0.05), (x, 0.0, seat_z - 0.075), 'y', rnd.randint(0, 999)))
    parts.append(beam((1.24, 0.05, 0.06), (0.0, 0.0, 0.22), 'x', rnd.randint(0, 999)))
    obj = lp.join(name, parts)
    lp.hull_box(obj, (length + 0.02, 0.44, seat_z), (0.0, 0.0, seat_z * 0.5))
    return lp.finish(obj, ao=0.35, nanite=False)


# --- Campfire ---

def fire_stone(size, seed):
    """A fist-to-head sized stone: a rounded lump, flat underneath."""
    rnd = random.Random(seed)
    bm = lp.new_bmesh()
    bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
    offset = Vector((rnd.uniform(0, 50), rnd.uniform(0, 50), rnd.uniform(0, 50)))
    sx, sy, sz = size, size * rnd.uniform(0.7, 0.9), size * rnd.uniform(0.55, 0.75)
    for v in bm.verts:
        d = v.co.normalized()
        r = (abs(d.x) ** 2.4 + abs(d.y) ** 2.4 + abs(d.z) ** 2.4) ** (-1.0 / 2.4)
        r *= 1.0 + 0.12 * noise.noise(d * 1.5 + offset)
        v.co = Vector((d.x * r * sx * 0.5, d.y * r * sy * 0.5, max(d.z * r * sz * 0.5, -sz * 0.3)))
    part = lp.mesh_object(bm)
    mod = part.modifiers.new('Reduce', 'DECIMATE')
    mod.ratio = 72.0 / 320.0
    with bpy.context.temp_override(object=part, active_object=part, selected_objects=[part],
                                   selected_editable_objects=[part]):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    lt.assign(part, GRANITE)
    return part, sz


def campfire_ring(name, seed):
    rnd = random.Random(seed)
    parts = []
    count, ring = 11, 0.56
    for k in range(count):
        a = 2.0 * math.pi * (k + rnd.uniform(-0.15, 0.15)) / count
        stone, height = fire_stone(rnd.uniform(0.2, 0.28), rnd.randint(0, 10 ** 6))
        lp.place(stone, (0.0, 0.0, 0.0), (rnd.uniform(-8.0, 8.0), rnd.uniform(-8.0, 8.0), math.degrees(a) + 90.0))
        r = ring + rnd.uniform(-0.03, 0.03)
        lp.place(stone, (math.cos(a) * r, math.sin(a) * r, height * 0.18))
        parts.append(stone)
    for p in parts:
        lt.box_uv(p, 'RockGranite', seed=rnd.randint(0, 99))
    ash, _ = lp.lathe([(0.0, 0.035), (0.2, 0.03), (0.36, 0.018), (0.46, 0.0), (0.5, -0.03), (0.0, -0.03)], segments=12)
    lp.rough(ash, 0.008, 6.0, seed)
    lp.grain(ash, 'BarkOak', axis=(1.0, 0.0, 0.0), seed=seed, material=CHARCOAL)
    parts.append(ash)
    # Charred logs, burnt thin toward the middle where they met, lying in a star.
    for k in range(4):
        a = 2.0 * math.pi * (k + rnd.uniform(-0.2, 0.2)) / 4 + 0.4
        d = Vector((math.cos(a), math.sin(a), 0.0))
        outer = d * rnd.uniform(0.42, 0.5) + Vector((0.0, 0.0, 0.05))
        inner = d * rnd.uniform(0.02, 0.08) + Vector((0.0, 0.0, 0.1 + 0.02 * k))
        log = lp.sweep([outer, outer.lerp(inner, 0.5), inner], lp.ngon(0.05, 6, jitter=0.1, seed=k + seed),
                    scales=[1.0, 0.85, 0.45])
        parts.append(lp.grain(log, 'BarkOak', axis=inner - outer, seed=rnd.randint(0, 999), material=CHARCOAL))
    obj = lp.join(name, parts)
    lp.hull_points(obj, [Vector((math.cos(a) * 0.74, math.sin(a) * 0.74, z)) for a in (k * math.pi / 4 for k in range(8))
                      for z in (0.0, 0.24)])
    lm.socket(obj, 'Fire', location=(0.0, 0.0, 0.12))
    lp.finish(obj, ao=0.3, fallback=100)
    # Stones are round: soft shading all over them (the charred logs keep their hard split edges).
    mesh = obj.data
    stone = mesh.materials.find(GRANITE.name)
    stone_edges = {key for p in mesh.polygons if p.material_index == stone for key in p.edge_keys}
    sharp = mesh.attributes.get('sharp_edge')
    if sharp is not None:
        for e in mesh.edges:
            if e.key in stone_edges:
                sharp.data[e.index].value = False
    return obj


# --- Signpost ---

def arrow_board(length, height, seed, strip):
    """A board with a pointed end, its tail at the origin, pointing along +X."""
    tip = height * 0.8
    outline = [(0.0, -height * 0.5), (length - tip, -height * 0.5), (length, 0.0), (length - tip, height * 0.5),
               (0.0, height * 0.5)]
    part = lp.sweep([(0.0, -0.015, 0.0), (0.0, 0.015, 0.0)], outline)
    return lp.grain(part, strip, axis=(1.0, 0.0, 0.0), seed=seed)


def signpost(name, seed):
    rnd = random.Random(seed)
    top = 2.3
    post = lp.block((0.12, 0.12, top + 0.15), (0.0, 0.0, (top - 0.15) * 0.5))
    lp.grain(post, 'Siding', axis=(0.0, 0.0, 1.0), seed=seed)
    lp.slice_at(post, (0.0, 0.0, 1.0), (0.03, 0.5))
    lp.rough(post, 0.004, 5.0, seed)
    cap, _ = lp.lathe([(0.0, top), (0.0849, top), (0.0, top + 0.1)], segments=4)
    lt.assign(cap, lp.trim_material())
    lp.lathe_uv(cap, 'Siding', seed=seed)
    lp.place(cap, (0.0, 0.0, 0.0), (0.0, 0.0, 45.0))
    parts = [post, cap]
    for z, yaw, strip, length in ((1.98, -12.0, 'TrimTeal', 0.78), (1.72, 158.0, 'TrimRed', 0.7)):
        board_part = arrow_board(length, 0.16, rnd.randint(0, 999), strip)
        lp.place(board_part, (-0.02, -0.075, 0.0))
        tilt = rnd.uniform(-3.0, 3.0)
        lp.place(board_part, (0.0, 0.0, z), (0.0, tilt, yaw))
        parts.append(board_part)
        front = Euler((0.0, math.radians(tilt), math.radians(yaw))).to_matrix() @ Vector((0.0, -1.0, 0.0))
        for along in (0.03, 0.11):
            spot = Euler((0.0, math.radians(tilt), math.radians(yaw))).to_matrix() @ Vector((along, -0.09, 0.0))
            parts.append(lp.nail(spot + Vector((0.0, 0.0, z)), front, size=0.02))
    obj = lp.join(name, parts)
    lp.hull_box(obj, (0.16, 0.16, top + 0.1), (0.0, 0.0, (top + 0.1) * 0.5))
    return lp.finish(obj, ao=0.35, nanite=False)


models = [
    lamp_post('LampPost', 10),
    laundry_line('LaundryLine', 20),
    bench('Bench', 30),
    campfire_ring('CampfireRing', 40),
    signpost('Signpost', 50),
]

if lt.want_preview():
    for model in models:
        lt.preview([model], lt.preview_path('RocksProps', model.name))
