"""Barrels, crates and a water trough for the tutorial island's farm and village. A scripted model (see
Art/README.md); the props are built from parts, UV-mapped on the house trim sheet or a tileable, and merged.

  Barrel_A     0.92 m wooden barrel: weathered staves (each its own board), four riveted iron hoops, a board head
  Barrel_B     0.88 m steel drum in faded teal paint and rust (MetalRust), rolling hoops, dents, two bungs
  Crate_A      0.9 m slatted crate: slats with gaps on corner posts, a diagonal brace, big nail heads
  Crate_B      1.0 m plank crate, its lid pushed back askew; packing straw (Hay) inside
  WaterTrough  1.8 m plank trough on two sleepers, iron straps, dark water inside

The pivot is on the ground at the middle of the footprint; the front faces -Y.
"""
import math
import random

import bmesh
from mathutils import Matrix, Vector, noise

import looter_textures as lt
import looter_props as lp


# --- Barrels ---

def barrel_a(name, seed):
    rnd = random.Random(seed)
    height, r_end, r_mid, proud = 0.92, 0.27, 0.33, 0.009

    def radius(z):
        return r_end + (r_mid - r_end) * math.sin(math.pi * min(max(z / height, 0.0), 1.0)) ** 0.8

    rows = [(0.0, 0.0, None), (radius(0.0), 0.0, 'end')]
    z = 0.0
    for center in (0.07, 0.27, 0.65, 0.85):
        z0, z1 = center - 0.028, center + 0.028
        while z0 - z > 0.13:
            z += min(0.12, (z0 - z) * 0.5)
            rows.append((radius(z), z, 'stave'))
        rows += [(radius(z0), z0, 'stave'), (radius(z0) + proud, z0, 'iron'), (radius(z1) + proud, z1, 'iron'),
                 (radius(z1), z1, 'iron')]
        z = z1
    rows += [(radius(height), height, 'stave'), (radius(height) - 0.025, height, 'stave'),
             (radius(height) - 0.025, height - 0.03, 'stave'), (0.0, height - 0.03, 'head')]
    body, bands = lp.lathe([(r, z) for r, z, _ in rows], segments=16)
    kinds = [rows[b + 1][2] for b in bands]
    # Staves are never quite even.
    for v in body.data.vertices:
        if math.hypot(v.co.x, v.co.y) > 0.1:
            k = round(math.atan2(v.co.y, v.co.x) / (2.0 * math.pi / 16))
            wobble = 1.0 + 0.012 * noise.noise(Vector((k * 1.7, seed, 0.3)))
            v.co.x *= wobble
            v.co.y *= wobble
    staves = [i for i, k in enumerate(kinds) if k in ('stave', 'end')]
    hoops = [i for i, k in enumerate(kinds) if k == 'iron']
    head = [i for i, k in enumerate(kinds) if k == 'head']
    lt.assign(body, lp.trim_material())
    lp.lathe_uv(body, 'Siding', staves, grain='up', seed=seed, per_column=True)
    lp.lathe_uv(body, 'Iron', hoops, grain='around', seed=seed + 1)
    lt.trim_uv(body, head, 'Siding', align='world', v_offset=0.3)
    # A bung in the belly.
    bung = lp.lathe([(0.0, 0.0), (0.028, 0.0), (0.026, 0.018), (0.0, 0.018)], segments=8)[0]
    lp.grain(bung, 'Beams', axis=(1.0, 0.0, 0.0), seed=seed)
    lp.place(bung, (0.0, 0.0, 0.0), (90.0, 0.0, 0.0))
    lp.place(bung, (0.0, -radius(0.46) + 0.004, 0.46), (0.0, 0.0, rnd.uniform(-20.0, 20.0)))
    obj = lp.join(name, [body, bung])
    ring = [Vector((math.cos(a) * r, math.sin(a) * r, z)) for a in (k * math.pi / 4 for k in range(8))
            for r, z in ((r_end, 0.0), (r_mid + proud, height * 0.5), (r_end, height))]
    lp.hull_points(obj, ring)
    return lp.finish(obj, ao=0.4, fallback=100, smooth=40.0)


DRUM = lt.material('MetalRust')


def barrel_b(name, seed):
    rnd = random.Random(seed)
    height, r = 0.88, 0.29
    rows = [(0.0, 0.0), (r - 0.012, 0.0), (r, 0.012)]
    for z in (0.12, 0.21):
        rows.append((r, z))
    for center in (0.29, 0.59):
        rows += [(r, center - 0.022), (r + 0.012, center - 0.008), (r + 0.012, center + 0.008), (r, center + 0.022)]
        if center < 0.5:
            rows += [(r, 0.38), (r, 0.46), (r, 0.53)]
    rows += [(r, 0.7), (r, 0.79), (r, height - 0.02), (r + 0.007, height - 0.01), (r, height), (r - 0.018, height),
             (r - 0.018, height - 0.016), (0.0, height - 0.016)]
    body, bands = lp.lathe(rows, segments=20)
    lid = [i for i, b in enumerate(bands) if b == len(rows) - 2]
    # Knocks and dents from a hard life.
    for center, depth in (((math.radians(-70.0), 0.45), 0.02), ((math.radians(35.0), 0.8), 0.014)):
        for v in body.data.vertices:
            rr = math.hypot(v.co.x, v.co.y)
            if rr < 0.2:
                continue
            a = math.atan2(v.co.y, v.co.x)
            d = math.hypot(((a - center[0] + math.pi) % (2.0 * math.pi) - math.pi) * r, v.co.z - center[1])
            if d < 0.12:
                s = 1.0 - depth * (1.0 - d / 0.12) ** 2 / rr
                v.co.x *= s
                v.co.y *= s
    lt.assign(body, DRUM)
    lp.lathe_uv(body, 'MetalRust', [i for i in range(len(bands)) if i not in set(lid)], grain='around', seed=seed)
    lt.box_uv(body, 'MetalRust', faces=lid, seed=seed)
    parts = [body]
    for a, rr in ((0.6, 0.19), (0.6 + math.pi, 0.2)):
        bung = lp.lathe([(0.0, 0.0), (0.034, 0.0), (0.034, 0.014), (0.022, 0.022), (0.0, 0.022)], segments=8)[0]
        lt.assign(bung, DRUM)
        lt.box_uv(bung, 'MetalRust', seed=seed + 2)
        lp.place(bung, (math.cos(a) * rr, math.sin(a) * rr, height - 0.018), (0.0, 0.0, rnd.uniform(0.0, 45.0)))
        parts.append(bung)
    obj = lp.join(name, parts)
    ring = [Vector((math.cos(a) * r, math.sin(a) * r, z)) for a in (k * math.pi / 4 for k in range(8))
            for z in (0.0, height)]
    lp.hull_points(obj, ring)
    return lp.finish(obj, ao=0.4, fallback=100, smooth=40.0)


# --- Crates ---

def crate_a(name, seed):
    """A slatted crate: slats with gaps nailed round four corner posts, a diagonal brace front and back, a plank top,
    and a dark inside seen through the gaps."""
    rnd = random.Random(seed)
    sx, sy, sz = 0.9, 0.65, 0.6
    post, slat_t = 0.07, 0.022
    parts = []
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            p = lp.block((post, post, sz - 0.02), (x * (sx * 0.5 - post * 0.5), y * (sy * 0.5 - post * 0.5), (sz - 0.02) * 0.5))
            parts.append(lp.grain(p, 'Beams', axis=(0.0, 0.0, 1.0), seed=rnd.randint(0, 999)))
    inner = lp.block((sx - 0.06, sy - 0.06, sz - 0.06), (0.0, 0.0, (sz - 0.04) * 0.5))
    parts.append(lp.grain(inner, 'Beams', axis=(1.0, 0.0, 0.0), seed=seed))
    slat_z = (0.1, 0.3, 0.5)
    for side in (-1.0, 1.0):
        y = side * (sy * 0.5 + slat_t * 0.5)
        for z in slat_z:
            s = lp.block((sx, slat_t, 0.16), (0.0, y, z + rnd.uniform(-0.006, 0.006)), (0.0, 0.0, rnd.uniform(-0.4, 0.4)))
            parts.append(lp.grain(s, 'Siding', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
            for x in (-sx * 0.5 + 0.035, sx * 0.5 - 0.035):
                parts.append(lp.nail((x, y + side * slat_t * 0.5, z), (0.0, side, 0.0)))
        # The brace runs corner to corner over the slats.
        length = math.hypot(sx - 0.1, sz - 0.12)
        angle = math.degrees(math.atan2(sz - 0.12, sx - 0.1)) * side
        brace = lp.block((length, slat_t, 0.1), (0.0, y + side * slat_t, sz * 0.5), (0.0, -angle, 0.0))
        parts.append(lp.grain(brace, 'Siding', seed=rnd.randint(0, 999)))
    for side in (-1.0, 1.0):
        x = side * (sx * 0.5 + slat_t * 0.5)
        for z in slat_z:
            s = lp.block((slat_t, sy + 2.0 * slat_t, 0.16), (x, 0.0, z + rnd.uniform(-0.006, 0.006)))
            parts.append(lp.grain(s, 'Siding', axis=(0.0, 1.0, 0.0), seed=rnd.randint(0, 999)))
            for y in (-sy * 0.5 + 0.03, sy * 0.5 - 0.03):
                parts.append(lp.nail((x + side * slat_t * 0.5, y, z), (side, 0.0, 0.0)))
    # The top: four planks with gaps, a little uneven.
    for k in range(4):
        y = -sy * 0.5 + 0.02 + (sy + 0.004) * (k + 0.5) / 4
        plank = lp.block((sx + 0.04, 0.15, slat_t), (rnd.uniform(-0.01, 0.01), y, sz + slat_t * 0.5 - 0.02),
                      (rnd.uniform(-0.6, 0.6), 0.0, rnd.uniform(-0.8, 0.8)))
        parts.append(lp.grain(plank, 'Siding', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
        for x in (-sx * 0.5 + 0.035, sx * 0.5 - 0.035):
            parts.append(lp.nail((x, y, sz + slat_t - 0.02), (0.0, 0.0, 1.0)))
    obj = lp.join(name, parts)
    lp.hull_box(obj, (sx + 0.08, sy + 0.08, sz + 0.02), (0.0, 0.0, (sz + 0.02) * 0.5))
    return lp.finish(obj, ao=0.35, fallback=100)


HAY = lt.material('Hay')


def crate_b(name, seed):
    """A plank crate, its lid pushed back and askew, packing straw inside."""
    rnd = random.Random(seed)
    sx, sy, sz, wall = 1.0, 0.62, 0.55, 0.025
    parts = []
    # Walls of boards (the siding strip draws the boards), a floor, and a batten frame round the edges.
    for side in (-1.0, 1.0):
        w = lp.block((sx, wall, sz), (0.0, side * (sy - wall) * 0.5, sz * 0.5))
        parts.append(lp.dice(lp.trim(w, 'Siding', seed=rnd.randint(0, 999)), 0.2))
        e = lp.block((wall, sy - 2.0 * wall, sz), (side * (sx - wall) * 0.5, 0.0, sz * 0.5))
        parts.append(lp.dice(lp.trim(e, 'Siding', seed=rnd.randint(0, 999)), 0.2))
    floor = lp.block((sx - 2.0 * wall, sy - 2.0 * wall, 0.03), (0.0, 0.0, 0.05))
    parts.append(lp.dice(lp.trim(floor, 'Siding', seed=seed), 0.25))
    batten = 0.055
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            b = lp.block((batten, batten, sz), (x * (sx * 0.5 - batten * 0.5 + 0.012), y * (sy * 0.5 - batten * 0.5 + 0.012),
                                             sz * 0.5))
            parts.append(lp.grain(b, 'Beams', axis=(0.0, 0.0, 1.0), seed=rnd.randint(0, 999)))
    for z in (0.03, sz - 0.03):
        for side in (-1.0, 1.0):
            b = lp.block((sx - 2.0 * batten, 0.02, 0.06), (0.0, side * (sy * 0.5 + 0.01), z))
            parts.append(lp.grain(b, 'Beams', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
            b = lp.block((0.02, sy - 2.0 * batten, 0.06), (side * (sx * 0.5 + 0.01), 0.0, z))
            parts.append(lp.grain(b, 'Beams', axis=(0.0, 1.0, 0.0), seed=rnd.randint(0, 999)))
            for x in (-0.3, 0.3):
                parts.append(lp.nail((x, side * (sy * 0.5 + 0.02), z), (0.0, side, 0.0)))
    # Straw: a lumpy bed a little below the rim, spilling over it at one corner.
    bm = lp.new_bmesh()
    bmesh.ops.create_grid(bm, x_segments=10, y_segments=6, size=0.5,
                          matrix=Matrix.Diagonal(Vector(((sx - 0.06) / 1.0, (sy - 0.06) / 1.0, 1.0, 1.0))))
    for v in bm.verts:
        v.co.z = sz - 0.1 + 0.035 * noise.noise(Vector((v.co.x * 6.0, v.co.y * 6.0, seed)))
    straw = lp.mesh_object(bm)
    lt.assign(straw, HAY)
    lt.box_uv(straw, 'Hay', seed=seed)
    parts.append(straw)
    # The lid: boards on two battens, pushed back and turned, one corner resting on the rim.
    lid_parts = [lp.dice(lp.trim(lp.block((sx + 0.04, sy + 0.04, 0.025), (0.0, 0.0, 0.0125)), 'Siding', seed=seed + 5), 0.25)]
    for x in (-0.32, 0.32):
        b = lp.block((0.08, sy - 0.02, 0.03), (x, 0.0, 0.04))
        lid_parts.append(lp.grain(b, 'Beams', axis=(0.0, 1.0, 0.0), seed=rnd.randint(0, 999)))
        for y in (-0.2, 0.0, 0.2):
            lid_parts.append(lp.nail((x, y, 0.055), (0.0, 0.0, 1.0)))
    lid = lp.join('_lid', lid_parts)
    lp.place(lid, (0.0, 0.0, 0.0), (0.0, -3.5, 14.0))
    lp.place(lid, (0.1, 0.24, sz + 0.004))
    parts.append(lid)
    obj = lp.join(name, parts)
    lp.hull_box(obj, (sx + 0.04, sy + 0.04, sz), (0.0, 0.0, sz * 0.5))
    lp.hull_box(obj, (sx + 0.04, sy + 0.04, 0.08), (0.1, 0.24, sz + 0.05), (0.0, -3.5, 14.0))
    return lp.finish(obj, ao=0.35, fallback=100)


# --- Water trough ---

def water_trough(name, seed):
    """A plank trough on two sleepers: sides of boards leaning out, end boards, a rim, two iron straps, water."""
    rnd = random.Random(seed)
    length, width, height, wall, lean = 1.8, 0.62, 0.5, 0.035, 8.0
    base = 0.1
    parts = []
    for x in (-0.6, 0.6):
        s = lp.block((0.14, width + 0.24, 0.1), (x, 0.0, 0.05), (0.0, 0.0, rnd.uniform(-3.0, 3.0)))
        parts.append(lp.grain(s, 'Beams', axis=(0.0, 1.0, 0.0), seed=rnd.randint(0, 999)))
    bottom = lp.block((length - 0.04, width - 0.1, wall), (0.0, 0.0, base + wall * 0.5))
    parts.append(lp.dice(lp.trim(bottom, 'Siding', seed=seed), 0.3))
    side_h = height - base
    for side in (-1.0, 1.0):
        # Two boards a side, leaning out.
        for k in range(2):
            board = lp.block((length, wall, side_h * 0.5 - 0.004), (0.0, 0.0, side_h * (k + 0.5) * 0.5))
            lp.grain(board, 'Siding', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999))
            lp.slice_at(board, (1.0, 0.0, 0.0), [-0.8, -0.5, -0.2, 0.2, 0.5, 0.8])
            lp.place(board, (0.0, 0.0, 0.0), (-side * lean, 0.0, 0.0))   # leaning out
            lp.place(board, (0.0, side * (width * 0.5 - 0.06), base))
            parts.append(board)
    for side in (-1.0, 1.0):
        end = lp.block((wall, width + 0.02, side_h), (side * (length * 0.5 - wall * 0.5 - 0.03), 0.0, base + side_h * 0.5))
        for v in end.data.vertices:   # the end follows the sides' lean
            v.co.y *= 0.82 + 0.18 * (v.co.z - base) / side_h + 0.08
        parts.append(lp.grain(end, 'Siding', axis=(0.0, 1.0, 0.0), seed=rnd.randint(0, 999)))
    top_y = width * 0.5 - 0.06 + math.sin(math.radians(lean)) * side_h
    for side in (-1.0, 1.0):
        rim = lp.block((length + 0.06, 0.08, 0.035), (0.0, side * top_y, height + 0.0175))
        parts.append(lp.grain(rim, 'Beams', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999)))
    # Iron straps round the belly.
    for x in (-0.5, 0.5):
        pts = [(x, -top_y - 0.03, height - 0.02), (x, -width * 0.5 + 0.02, base - 0.005),
               (x, width * 0.5 - 0.02, base - 0.005), (x, top_y + 0.03, height - 0.02)]
        for a, b in zip(pts, pts[1:]):
            a, b = Vector(a), Vector(b)
            mid, d = (a + b) * 0.5, b - a
            strap = lp.block((0.06, d.length + 0.01, 0.008), (0.0, 0.0, 0.0))
            angle = math.degrees(math.atan2(d.z, d.y))
            lp.place(strap, mid + Vector((0.0, 0.0, 0.0)), (angle, 0.0, 0.0))
            normal = Vector((0.0, -d.z, d.y)).normalized()
            if normal.dot(mid - Vector((x, 0.0, height * 0.5))) < 0.0:
                normal = -normal
            lp.place(strap, normal * 0.022)
            parts.append(lp.grain(strap, 'Iron', axis=d, seed=rnd.randint(0, 999)))
            parts.append(lp.nail(mid + normal * 0.026, normal, size=0.02))
    # Water, dark and still, a hand below the rim (in strips, each one board of the glass strip).
    for k in range(3):
        y0 = -top_y + 0.02 + (2.0 * top_y - 0.04) * k / 3
        y1 = -top_y + 0.02 + (2.0 * top_y - 0.04) * (k + 1) / 3
        water = lp.block((length - 0.1, y1 - y0, 0.01), (0.0, (y0 + y1) * 0.5, height - 0.1))
        parts.append(lp.grain(water, 'Glass', axis=(1.0, 0.0, 0.0), seed=seed + k))
    obj = lp.join(name, parts)
    lp.hull_box(obj, (length + 0.06, 2.0 * top_y + 0.08, height + 0.035), (0.0, 0.0, (height + 0.035) * 0.5))
    return lp.finish(obj, ao=0.4, fallback=100)


models = [
    barrel_a('Barrel_A', 10),
    barrel_b('Barrel_B', 20),
    crate_a('Crate_A', 30),
    crate_b('Crate_B', 40),
    water_trough('WaterTrough', 50),
]

if lt.want_preview():
    for model in models:
        lt.preview([model], lt.preview_path('RocksProps', model.name))
