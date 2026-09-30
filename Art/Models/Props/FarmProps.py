"""Farm props for the tutorial island: carts, hay and firewood. A scripted model (see Art/README.md); the props are
built from parts, UV-mapped on the house trim sheet or a tileable, and merged.

  Cart            two-wheeled hand cart (1.5 m bed, 0.5 m wheels), resting forward on its shafts; the shafts point
                  to the front (-Y)
  Wheelbarrow     wooden wheelbarrow, its tray in faded oxide-red paint, wheel at the front (-Y), resting on its legs
  HayBale_Round   1.4 m round bale lying on its side (axis along X), net-wrap grooves, a spiral on its ends
  HayBale_Square  0.95 m small square bale with two twine grooves (no Nanite: a small prop)
  FirewoodStack   1.6 m cord of split logs between stakes, on two sleepers (bark and split wood)

The pivot is on the ground at the middle of the footprint.
"""
import math
import random

import bmesh
import bpy
from mathutils import Vector

import looter_textures as lt
import looter_props as lp


HAY = lt.material('Hay')
BARK = lt.material('BarkOak')


def wheel(radius, width, spokes, seed, hub=0.075, felloe=0.06):
    """A spoked wooden wheel in the XY plane around the origin (turn it onto its axle with lp.place()): an iron tyre on
    a wooden felloe, spokes, a hub."""
    rnd = random.Random(seed)
    inner = radius - felloe
    rim, bands = lp.lathe([(inner, -width * 0.5), (radius, -width * 0.5), (radius, width * 0.5), (inner, width * 0.5)],
                       segments=16, closed=True)
    lt.assign(rim, lp.trim_material())
    lp.lathe_uv(rim, 'Beams', [i for i, b in enumerate(bands) if b != 1], grain='around', seed=seed)
    lp.lathe_uv(rim, 'Iron', [i for i, b in enumerate(bands) if b == 1], grain='around', seed=seed + 1)
    boss, _ = lp.lathe([(0.0, -width * 0.9), (hub * 0.75, -width * 0.9), (hub, -width * 0.3), (hub, width * 0.3),
                     (hub * 0.75, width * 0.9), (0.0, width * 0.9)], segments=8)
    lt.assign(boss, lp.trim_material())
    lp.lathe_uv(boss, 'Beams', grain='around', seed=seed + 2)
    parts = [rim, boss]
    start = rnd.uniform(0.0, math.pi)
    for k in range(spokes):
        a = start + 2.0 * math.pi * k / spokes
        d = Vector((math.cos(a), math.sin(a), 0.0))
        spoke = lp.sweep([d * (hub * 0.8), d * (inner + 0.015)],
                      [(-0.017, -0.012), (0.017, -0.012), (0.017, 0.012), (-0.017, 0.012)])
        parts.append(lp.grain(spoke, 'Beams', axis=d, seed=rnd.randint(0, 999)))
    return lp.join('_wheel', parts)


def pole(points, radius, seed, strip='Beams', sides=6):
    """A round-ish pole (shaft, handle, rail) along points."""
    part = lp.sweep(points, lp.ngon(radius, sides, jitter=0.06, seed=seed))
    return lp.grain(part, strip, axis=Vector(points[-1]) - Vector(points[0]), seed=seed)


def board(size, center, axis, seed, strip='Siding', rotation=(0.0, 0.0, 0.0)):
    """A board (a box) with its grain along axis ('x', 'y' or 'z', before rotation)."""
    part = lp.block(size, center)
    vector = {'x': (1.0, 0.0, 0.0), 'y': (0.0, 1.0, 0.0), 'z': (0.0, 0.0, 1.0)}[axis]
    lp.grain(part, strip, axis=vector, seed=seed)
    if any(rotation):
        # Turn about the board's own centre.
        lp.place(part, [-c for c in center])
        lp.place(part, center, rotation)
    return part


def points_of(objects, keep=lambda co: True):
    """The vertices of objects (parts in their final place) that keep() accepts: points for a hull, taken before
    lp.join() merges the parts."""
    return [v.co.copy() for o in objects for v in o.data.vertices if keep(v.co)]


# --- Cart ---

def cart(name, seed):
    rnd = random.Random(seed)
    radius, track = 0.5, 0.64
    parts = []
    wheels = []
    for side in (-1.0, 1.0):
        w = wheel(radius, 0.07, 10, seed + int(side + 2))
        lp.place(w, (0.0, 0.0, 0.0), (0.0, 0.0, rnd.uniform(0.0, 36.0)))
        lp.place(w, (0.0, 0.0, 0.0), (0.0, 90.0, 0.0))
        lp.place(w, (side * track, 0.0, radius))
        wheels.append(w)
    axle = board((2.0 * track + 0.12, 0.08, 0.08), (0.0, 0.0, radius), 'x', seed, strip='Beams')
    # The body is built level around the axle, then tipped forward until the shaft tips rest on the ground.
    body = []
    for side in (-1.0, 1.0):
        body.append(pole([(side * 0.4, 0.78, 0.07), (side * 0.4, -0.78, 0.07), (side * 0.3, -1.95, 0.1)], 0.042,
                         rnd.randint(0, 999)))
    body.append(pole([(-0.33, -1.78, 0.155), (0.33, -1.78, 0.155)], 0.028, rnd.randint(0, 999)))   # the pull bar
    for k in range(5):
        x = -0.36 + 0.18 * k
        body.append(board((0.17, 1.52, 0.03), (x + rnd.uniform(-0.004, 0.004), 0.0, 0.13), 'y', rnd.randint(0, 999)))
    for side in (-1.0, 1.0):
        for z in (0.24, 0.4):
            body.append(board((0.025, 1.52, 0.14), (side * 0.47, 0.0, z + rnd.uniform(-0.005, 0.005)), 'y',
                              rnd.randint(0, 999), rotation=(rnd.uniform(-1.0, 1.0), 0.0, 0.0)))
        for y in (-0.64, 0.0, 0.64):
            body.append(board((0.05, 0.05, 0.44), (side * 0.5, y, 0.32), 'z', rnd.randint(0, 999), strip='Beams'))
        for z in (0.24, 0.4):
            body.append(board((0.94, 0.025, 0.14), (0.0, side * 0.775, z), 'x', rnd.randint(0, 999)))
    bed_parts = body[3:]
    shafts = body[:3]
    # Tip forward about the axle so the shafts' tips (1.95 m ahead, 5 cm under the pole's axis) touch the ground.
    tip_y, tip_z = -1.95, 0.1 - 0.042
    angle = math.asin((radius + tip_z) / math.hypot(tip_y, tip_z)) + math.atan2(tip_z, -tip_y)
    for part in body:
        lp.place(part, (0.0, 0.0, 0.0), (math.degrees(angle), 0.0, 0.0))
        lp.place(part, (0.0, 0.0, radius))
    bed_points = points_of(bed_parts)
    shaft_points = points_of(shafts, keep=lambda co: co.y < -0.6)
    obj = lp.join(name, wheels + [axle] + body)
    lp.hull_points(obj, bed_points)
    lp.hull_points(obj, shaft_points)
    for side in (-1.0, 1.0):
        ring = [Vector((side * track + dx, math.cos(a) * radius, radius + math.sin(a) * radius))
                for a in (k * math.pi / 4 for k in range(8)) for dx in (-0.04, 0.04)]
        lp.hull_points(obj, ring)
    return lp.finish(obj, ao=0.5, fallback=100)


# --- Wheelbarrow ---

def wheelbarrow(name, seed):
    rnd = random.Random(seed)
    radius, axle_y = 0.23, -0.62
    w = wheel(radius, 0.06, 8, seed, hub=0.05, felloe=0.045)
    lp.place(w, (0.0, 0.0, 0.0), (0.0, 0.0, rnd.uniform(0.0, 45.0)))
    lp.place(w, (0.0, 0.0, 0.0), (0.0, 90.0, 0.0))
    lp.place(w, (0.0, axle_y, radius))
    parts = [w]
    rails = []
    for side in (-1.0, 1.0):
        rails.append(pole([(side * 0.07, axle_y - 0.03, radius), (side * 0.2, -0.1, 0.4), (side * 0.27, 0.55, 0.52),
                           (side * 0.29, 0.98, 0.6)], 0.024, rnd.randint(0, 999)))
        leg = lp.sweep([(side * 0.215, 0.3, 0.49), (side * 0.235, 0.36, 0.0)],
                    [(-0.022, -0.022), (0.022, -0.022), (0.022, 0.022), (-0.022, 0.022)])
        parts.append(lp.grain(leg, 'Beams', axis=(0.0, 0.1, -1.0), seed=rnd.randint(0, 999)))
    parts += rails
    parts.append(pole([(-0.1, axle_y, radius), (0.1, axle_y, radius)], 0.02, seed, strip='Iron', sides=6))
    # The tray: a plank bottom, sides flaring out, a front sloping forward, all in faded red paint outside.
    bottom_z, tray_h = 0.44, 0.3
    parts.append(board((0.46, 0.66, 0.025), (0.0, -0.05, bottom_z), 'y', seed, strip='Siding'))
    for side in (-1.0, 1.0):
        for k in range(2):
            b = lp.block((0.025, 0.7, 0.15), (0.0, 0.0, 0.075 + 0.15 * k))
            lp.grain(b, 'TrimRed', axis=(0.0, 1.0, 0.0), seed=rnd.randint(0, 999))
            for v in b.data.vertices:            # the side follows the sloped front
                if v.co.y < 0.0:
                    v.co.y -= v.co.z * 0.85
            lp.place(b, (0.0, 0.0, 0.0), (0.0, side * 22.0, 0.0))
            lp.place(b, (side * 0.235, -0.02, bottom_z))
            parts.append(b)
    for k in range(2):
        f = lp.block((0.5, 0.025, 0.155), (0.0, 0.0, 0.0775 + 0.15 * k))
        lp.grain(f, 'TrimRed', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999))
        for v in f.data.vertices:                # wider at the top, like the tray
            v.co.x *= 1.0 + v.co.z * 1.25
        lp.place(f, (0.0, 0.0, 0.0), (40.0, 0.0, 0.0))    # the front slopes forward
        lp.place(f, (0.0, -0.37, bottom_z))
        parts.append(f)
        g = lp.block((0.5, 0.025, 0.155), (0.0, 0.0, 0.0775 + 0.15 * k))
        lp.grain(g, 'TrimRed', axis=(1.0, 0.0, 0.0), seed=rnd.randint(0, 999))
        for v in g.data.vertices:
            v.co.x *= 1.0 + v.co.z * 1.25
        lp.place(g, (0.0, 0.0, 0.0), (-12.0, 0.0, 0.0))   # the back leans back a little
        lp.place(g, (0.0, 0.31, bottom_z))
        parts.append(g)
    obj = lp.join(name, parts)
    lp.hull_points(obj, [Vector((x, y, z)) for x in (-0.36, 0.36) for y, z in ((-0.62, 0.0), (-0.62, 0.47), (-0.62, bottom_z + 0.2),
                                                                            (-0.58, bottom_z + 0.27), (0.4, 0.0), (0.4, 0.75))])
    lp.hull_points(obj, [Vector((x, y, z)) for x in (-0.33, 0.33) for y, z in ((0.4, 0.45), (0.4, 0.62), (1.02, 0.55), (1.02, 0.65))])
    return lp.finish(obj, ao=0.45, fallback=100)


# --- Hay ---

def hay_round(name, seed):
    """A round bale lying on its side: rounded edges, the ends dished a little, grooves where the net wrap bites."""
    r, half = 0.7, 0.6
    rows = [(0.0, -half + 0.03), (0.3, -half + 0.01), (0.5, -half), (0.62, -half + 0.015), (0.68, -half + 0.06),
            (r, -half + 0.14)]
    for g in (-0.3, 0.0, 0.3):
        rows += [(r, g - 0.035), (r - 0.014, g), (r, g + 0.035)]
    rows += [(r, half - 0.14), (0.68, half - 0.06), (0.62, half - 0.015), (0.5, half), (0.3, half - 0.01),
             (0.0, half - 0.03)]
    part, bands = lp.lathe(rows, segments=24)
    lt.assign(part, HAY)
    lp.lathe_uv(part, 'Hay', grain='around', seed=seed)    # the straw runs round the bale, and spirals on the ends
    lp.rough(part, 0.018, 3.0, seed)
    lp.place(part, (0.0, 0.0, 0.0), (0.0, 90.0, 0.0))
    # It settles under its own weight: flatter underneath.
    for v in part.data.vertices:
        v.co.z *= 0.94
        v.co.z = -0.62 + math.sqrt((v.co.z + 0.62) ** 2 + 0.0004) if v.co.z < -0.55 else v.co.z
    lowest = min(v.co.z for v in part.data.vertices)
    for v in part.data.vertices:
        v.co.z -= lowest + 0.02
    obj = lp.join(name, [part])
    top = max(v.co.z for v in obj.data.vertices)
    ring = [Vector((x, math.cos(a) * r, top * 0.5 + math.sin(a) * top * 0.5)) for a in (k * math.pi / 4 for k in range(8))
            for x in (-half, half)]
    lp.hull_points(obj, ring)
    return lp.finish(obj, ao=0.6, fallback=100, smooth=60.0)


def hay_square(name, seed):
    """A small square bale: chamfered, a little bulging, two twine grooves round it."""
    sx, sy, sz = 0.95, 0.46, 0.36
    part = lp.block((sx, sy, sz), (0.0, 0.0, sz * 0.5))
    bm = bmesh.new()
    bm.from_mesh(part.data)
    bmesh.ops.bevel(bm, geom=list(bm.edges), offset=0.035, segments=2, affect='EDGES', clamp_overlap=True)
    bm.to_mesh(part.data)
    bm.free()
    lt.assign(part, HAY)
    lt.box_uv(part, 'Hay', seed=seed)
    lp.slice_at(part, (1.0, 0.0, 0.0), (-0.3, -0.27, -0.24, 0.24, 0.27, 0.3))
    for v in part.data.vertices:
        c = Vector((v.co.x, v.co.y, v.co.z - sz * 0.5))
        bulge = 0.02 * (1.0 - (2.0 * v.co.x / sx) ** 2)
        v.co.y += math.copysign(bulge, c.y) if abs(c.y) > sy * 0.3 else 0.0
        if abs(abs(v.co.x) - 0.27) < 0.005:      # the twine pulls the straw in
            v.co.y *= 0.965
            v.co.z = sz * 0.5 + (v.co.z - sz * 0.5) * 0.955
    lp.rough(part, 0.008, 5.0, seed)
    for v in part.data.vertices:
        v.co.z = max(v.co.z, 0.0)
    obj = lp.join(name, [part])
    lp.hull_box(obj, (sx, sy + 0.04, sz), (0.0, 0.0, sz * 0.5))
    return lp.finish(obj, ao=0.4, nanite=False, smooth=50.0)


# --- Firewood ---

def split_log(length, radius, seed):
    """A split log along Y: a wedge of a round, bark on its curved side, split wood on the flat ones."""
    rnd = random.Random(seed)
    kind = rnd.random()
    if kind < 0.45:        # a quarter
        arc = [(math.cos(a), math.sin(a)) for a in (0.0, math.pi / 6, math.pi / 3, math.pi / 2)]
        outline = [(0.0, 0.0)] + arc
    elif kind < 0.8:       # a half
        arc = [(math.cos(a), math.sin(a)) for a in (0.0, math.pi / 4, math.pi / 2, 3 * math.pi / 4, math.pi)]
        outline = arc
    else:                  # a third
        arc = [(math.cos(a), math.sin(a)) for a in (0.0, math.pi / 3, 2 * math.pi / 3)]
        outline = [(0.0, 0.0)] + arc
    profile = [(x * radius * rnd.uniform(0.93, 1.05), z * radius * rnd.uniform(0.93, 1.05)) for x, z in outline]
    cx = sum(p[0] for p in profile) / len(profile)
    cz = sum(p[1] for p in profile) / len(profile)
    profile = [(x - cx, z - cz) for x, z in profile]
    part = lp.sweep([(0.0, -length * 0.5, 0.0), (0.0, length * 0.5, 0.0)], profile, up=(0.0, 0.0, 1.0))
    # Bark on the faces of the round side, split wood on the flat faces and the ends.
    bark, wood = [], []
    for p in part.data.polygons:
        if abs(p.normal.y) > 0.7:
            wood.append(p.index)
            continue
        c = p.center
        # A face on the round side has its middle on the circle's outside.
        (bark if Vector((c.x + cx, c.z + cz)).length > radius * 0.75 else wood).append(p.index)
    lp.grain(part, 'Logs', axis=(0.0, 1.0, 0.0), seed=seed, faces=wood)
    if bark:
        lp.grain(part, 'BarkOak', axis=(0.0, 1.0, 0.0), seed=seed, faces=bark, material=BARK)
    return part, max(abs(z) for _, z in profile), max(abs(x) for x, _ in profile)


def firewood_stack(name, seed):
    rnd = random.Random(seed)
    length, depth, height = 1.6, 0.45, 1.0
    parts = []
    for y in (-0.12, 0.12):
        parts.append(pole([(-length * 0.5 - 0.05, y, 0.045), (length * 0.5 + 0.05, y, 0.045)], 0.045,
                          rnd.randint(0, 999), strip='BarkOak'))
    for side in (-1.0, 1.0):
        for y in (-0.16, 0.16):
            x = side * (length * 0.5 + 0.06)
            parts.append(pole([(x, y, -0.1), (x + side * rnd.uniform(0.0, 0.03), y, height + 0.12)], 0.035,
                              rnd.randint(0, 999), strip='BarkOak'))
    for p in parts:
        lt.assign(p, BARK)
    z = 0.09
    row = 0
    while z < height - 0.05:
        x = -length * 0.5 + 0.02
        tallest = 0.0
        # The top row is ragged: fewer logs, not all the way across.
        last = z > height - 0.2
        while x < length * 0.5 - 0.08:
            radius = rnd.uniform(0.075, 0.1)
            log, half_h, half_w = split_log(depth * rnd.uniform(0.9, 1.08), radius, rnd.randint(0, 10 ** 6))
            lp.place(log, (0.0, 0.0, 0.0), (0.0, rnd.uniform(0.0, 360.0), 0.0))
            size = max(max(abs(v.co.x) for v in log.data.vertices), 0.05)
            height_here = max(abs(v.co.z) for v in log.data.vertices)
            if last and rnd.random() < 0.35:
                x += size * 2.0
                bpy.data.objects.remove(log)
                continue
            lp.place(log, (x + size, rnd.uniform(-0.035, 0.035), z + height_here), (0.0, 0.0, rnd.uniform(-5.0, 5.0)))
            parts.append(log)
            x += size * 2.0 + rnd.uniform(0.0, 0.01)
            tallest = max(tallest, height_here * 2.0)
        z += tallest * 0.82
        row += 1
    obj = lp.join(name, parts)
    lp.hull_box(obj, (length + 0.2, depth + 0.1, height + 0.05), (0.0, 0.0, (height + 0.05) * 0.5))
    return lp.finish(obj, ao=0.35, fallback=100)


models = [
    cart('Cart', 10),
    wheelbarrow('Wheelbarrow', 20),
    hay_round('HayBale_Round', 30),
    hay_square('HayBale_Square', 40),
    firewood_stack('FirewoodStack', 50),
]

if lt.want_preview():
    for model in models:
        lt.preview([model], lt.preview_path('RocksProps', model.name))
