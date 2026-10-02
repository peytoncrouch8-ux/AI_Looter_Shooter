"""The packet skiff (Docs/Story.md: Leaving the tutorial island; Docs/Areas/RansomsRest.md: Skiff and jetty): a small
frontier airship, more boat than zeppelin. A 9 m flat-bottomed lapstrake hull under a patched canvas gas-bag, with a
bow lantern and mooring ropes. Two options for the user to pick between, each in two paints built from the same code:

  Skiff_A_Packet / Skiff_A_Gang   a fat cigar bag slung over the hull in a rope net, a steering sweep over the stern
  Skiff_B_Packet / Skiff_B_Gang   a tapered bag on a light timber keel frame, struts down to the gunwales, a stern
                                  fin with its rudder
  Skiff_Gangplank                 the 3 m gangplank: its origin is the hinge, modeled lowered (level); raise it by
                                  turning it 90 degrees about the hinge (its local Y, along the hull) so the free end
                                  rises. Walkable, one slim box hull.

_Packet is the tutorial's skiff: faded teal planking, cream trim, a pale patched bag, a lit bow lantern. _Gang is the
Dunne Gang's skiff in the cold open: black planking, a dark red stripe, a dark bag with pale patches, the lantern dark.
Both paints share every socket, so the cold open's match cut puts the player on the same spot.

The pivot is under the hull's middle at the keel (z = 0); the front faces -Y. The deck is 0.43 m above the keel.
Sockets: SOCKET_Deck (where the player stands for the ride, facing forward), SOCKET_Light (the bow lantern's glass),
SOCKET_Mooring_1..4 (the mooring ropes' ends: bow and stern, both sides), SOCKET_Gangplank (the gangplank's hinge, on
the +X side's opening, at the sill). Collision: a walkable deck box, side slabs up to 1.45 m over the deck so nobody
falls off (open at the gangplank's opening, which the raised gangplank closes), a wedge in the bow; nothing on the bag.

    blender -b --factory-startup --python Art/Models/Vehicles/Skiff.py -- --preview
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_textures as lt
import looter_props as lp
import looter_train as tk

DECK = 0.43                     # the deck's top above the keel
LENGTH = 9.0
GAP = (0.1, 1.1)                # the gangplank's opening in the +X side
DECK_Y = (-2.9, 4.38)           # the deck from the bow platform to the transom
PLANK = 3.0                     # gangplank length
SU = 320.0 / 1024.0             # WoodPlanks UV units per metre

# The hull's lines, by station (y): gunwale half-breadth and height, chine half-breadth and height.
G = [(-4.5, 0.0), (-4.0, 0.42), (-3.5, 0.74), (-3.0, 0.94), (-2.0, 1.12), (-1.0, 1.18), (0.5, 1.2), (2.0, 1.17),
     (3.5, 1.08), (4.5, 0.96)]
ZG = [(-4.5, 2.12), (-4.0, 1.86), (-3.0, 1.62), (-2.0, 1.52), (-0.5, 1.46), (1.0, 1.45), (2.5, 1.49), (3.5, 1.57),
      (4.5, 1.72)]
B = [(-4.5, 0.0), (-4.0, 0.0), (-3.6, 0.15), (-3.0, 0.45), (-2.0, 0.72), (-1.0, 0.82), (1.0, 0.84), (3.0, 0.8),
     (4.5, 0.7)]
ZC = [(-4.5, 1.1), (-4.0, 0.7), (-3.6, 0.48), (-3.0, 0.28), (-2.0, 0.15), (0.0, 0.12), (3.5, 0.14), (4.5, 0.22)]

PAINTS = {
    # Patches are a slightly different tone of canvas (the packet's a paler, greyer cream; the gang's a lighter brown),
    # hemmed in the bag's own canvas. Inside, the hull is bare weathered planking.
    'Packet': dict(hull='wteal', stripe=('cream', None), bag='canvas', patch=('trim', 'Plaster'), rope='canvas',
                   lamp='glow'),
    'Gang': dict(hull='wblack', stripe=('trim', 'TrimRed'), bag='canvasdark', patch='canvasdarkpatch',
                 rope='canvasdark', lamp=('trim', 'Glass')),
}
# WoodPlanks rows with a single butt joint in the set's 3.2 m repeat (from lt.PLANK_JOINTS): hull planks run on these,
# stretched to 6.4 m a repeat, so a 9 m strake shows at most one scarf joint, staggered from strake to strake.
LONG_ROWS = [i for i, joints in enumerate(lt.PLANK_JOINTS) if len(joints) == 1] or list(range(16))
PLANK_REPEAT = 6.4


def table(points, y):
    """Linear interpolation in a station table."""
    if y <= points[0][0]:
        return points[0][1]
    for (y0, v0), (y1, v1) in zip(points, points[1:]):
        if y <= y1:
            return v0 + (v1 - v0) * (y - y0) / (y1 - y0)
    return points[-1][1]


def side_point(s, y, f, out=0.0):
    """A point on the hull's side at station y, fraction f from chine (0) to gunwale (1), moved out along the normal."""
    b, g, zc, zg = table(B, y), table(G, y), table(ZC, y), table(ZG, y)
    p = Vector((s * (b + (g - b) * f), y, zc + (zg - zc) * f))
    return p + side_normal(s, y) * out if out else p


def side_normal(s, y):
    b, g, zc, zg = table(B, y), table(G, y), table(ZC, y), table(ZG, y)
    n = Vector((s * (zg - zc), 0.0, -(g - b)))
    return n.normalized() if n.length > 1e-6 else Vector((s, 0.0, 0.0))


def f_at(y, z):
    """The side fraction at height z on station y."""
    zc, zg = table(ZC, y), table(ZG, y)
    return min(max((z - zc) / max(zg - zc, 1e-3), 0.0), 1.0)


def stations(y0, y1, step=0.5):
    ys = sorted({round(y, 4) for y in [y0, y1] + [-4.5 + step * i for i in range(int(LENGTH / step) + 1)] if y0 <= y <= y1})
    return ys


# --- The hull (shared by every option and paint) ---

def strake(k, s, f0, f1, y0, y1, mat, row, joint_y, inside_mat):
    """One lapstrake plank of the side from y0 to y1: its lower edge stands out over the plank below. It runs on one
    long WoodPlanks row (see LONG_ROWS), its one joint near joint_y; painted outside, bare boards (inside_mat) inside."""
    sections, uvs = [], []
    v0, v1 = row / 16.0 + 0.002, (row + 1) / 16.0 - 0.002
    joint = lt.PLANK_JOINTS[row][0] / 3.2 if lt.PLANK_JOINTS[row] else 0.0
    u0 = joint - joint_y / PLANK_REPEAT
    for y in stations(y0, y1):
        n = side_normal(s, y)
        ib, it = side_point(s, y, f0), side_point(s, y, f1)
        sections.append([ib, ib + n * 0.055, it + n * 0.035, it])
        u = y / PLANK_REPEAT + u0
        uvs.append([(u, v0), (u, v0 + 0.004), (u, v1 - 0.004), (u, v1)])
    obj = tk.loft(sections, uvs)
    lt.assign(obj, tk.material(mat))
    inside = [p.index for p in obj.data.polygons
              if p.normal.dot(side_normal(s, p.center.y)) < -0.6 and p.area > 1e-4]
    if inside:
        lt.assign(obj, tk.material(inside_mat), inside)
    return k.add(obj)


def rail_along(k, s, f, y0, y1, profile, out, mat, strip=None, lift=0.0):
    """A rail (gunwale cap, rubbing strake) along the side at fraction f from y0 to y1."""
    pts = [side_point(s, y, f, out) + Vector((0.0, 0.0, lift)) for y in stations(y0, y1)]
    part = lp.sweep(pts, profile, up=(0.0, 0.0, 1.0))
    if mat == 'trim':
        lp.grain(part, strip, axis=(0.0, 1.0, 0.0), seed=k.seed())
        return k.add(part)
    return k.map(part, mat)


def hull(k, paint):
    """The lapstrake hull: bottom, four strakes a side (the upper three open at the gangplank's opening on +X), keel,
    stem, transom, gunwale caps, the painted stripe, ribs, the deck, thwarts and cargo, cleats and mooring ropes."""
    p = PAINTS[paint]
    rnd = k.rnd
    for s in (-1.0, 1.0):
        for i in range(4):
            f0, f1 = max(0.0, i / 4.0 - 0.035), (i + 1) / 4.0
            spans = [(-4.47, 4.5)]
            if s > 0 and i > 0:
                spans = [(-4.47, GAP[0]), (GAP[1], 4.5)]
            row = LONG_ROWS[(i * 3 + (2 if s > 0 else 0)) % len(LONG_ROWS)]
            joint_y = (-2.6, 1.4, -0.6, 2.8)[i] + (0.9 if s > 0 else 0.0)
            for a, b in spans:
                strake(k, s, f0, f1, a, b, p['hull'], row, joint_y, 'planks')
    # Bottom planks (fore and aft) and the keel skeg.
    sections, uvs = [], []
    for y in stations(-3.95, 4.5):
        b, zc = table(B, y), table(ZC, y)
        sections.append([Vector((-b, y, zc - 0.05)), Vector((b, y, zc - 0.05)), Vector((b, y, zc)), Vector((-b, y, zc))])
        u = y / PLANK_REPEAT
        uvs.append([(u, -b * SU * 1.6), (u, b * SU * 1.6), (u + 0.003, b * SU * 1.6), (u + 0.003, -b * SU * 1.6)])
    bottom = tk.loft(sections, uvs)
    lt.assign(bottom, tk.material(p['hull']))
    lt.assign(bottom, tk.material('planks'), [f.index for f in bottom.data.polygons if f.normal.z > 0.5])
    k.add(bottom)
    keel = [(0.0, y, table(ZC, y) - 0.085) for y in stations(-3.9, 4.4)]
    part = lp.sweep(keel, [(-0.05, -0.035), (0.05, -0.035), (0.05, 0.035), (-0.05, 0.035)], up=(0.0, 0.0, 1.0))
    lp.grain(part, 'Beams', axis=(0.0, 1.0, 0.0), seed=k.seed())
    k.add(part)
    # Stem and transom.
    stem = [(0.0, -3.95, 0.55), (0.0, -4.25, 0.95), (0.0, -4.45, 1.5), (0.0, -4.53, 2.05), (0.0, -4.56, 2.3)]
    part = lp.sweep(stem, [(-0.055, -0.06), (0.055, -0.06), (0.055, 0.06), (-0.055, 0.06)], up=(1.0, 0.0, 0.0))
    lp.grain(part, 'Beams', axis=(0.0, -0.3, 1.0), seed=k.seed())
    k.add(part)
    b, g, zc, zg = table(B, 4.5), table(G, 4.5), table(ZC, 4.5), table(ZG, 4.5)
    outline = [(-b, zc - 0.05), (b, zc - 0.05), (g + 0.05, zg + 0.02), (-g - 0.05, zg + 0.02)]
    k.panel([[(-u, v) for u, v in reversed(outline)]], 0.06, p['hull'], tk.frame('+y', (0.0, 4.53, 0.0)))
    # Gunwale caps, the stripe, ribs.
    cap = [(-0.06, 0.0), (0.06, 0.0), (0.06, 0.055), (-0.06, 0.055)]
    stripe = [(-0.014, -0.045), (0.014, -0.045), (0.014, 0.045), (-0.014, 0.045)]
    for s in (-1.0, 1.0):
        spans = [(-4.4, GAP[0]), (GAP[1], 4.5)] if s > 0 else [(-4.4, 4.5)]
        for a, b2 in spans:
            rail_along(k, s, 1.0, a, b2, cap, 0.015, 'trim', 'Beams')
            rail_along(k, s, 0.83, a, b2, stripe, 0.06, p['stripe'][0], p['stripe'][1])
        for y in (-2.6, -1.6, -0.5, 1.6, 2.6, 3.6):
            if s > 0 and GAP[0] - 0.15 < y < GAP[1] + 0.15:
                continue
            n = side_normal(s, y)
            a, c = side_point(s, y, f_at(y, DECK), -0.03), side_point(s, y, 0.98, -0.03)
            k.beam(a, c, 0.07, 0.05, 'trim', 'Beams', up=tuple(n))
    # The opening: posts each side of it and a sill.
    for y in GAP:
        a, c = side_point(1.0, y, f_at(y, DECK), 0.02), side_point(1.0, y, 1.0, 0.02)
        k.beam(a, c + Vector((0.0, 0.0, 0.08)), 0.09, 0.09, 'trim', 'Beams', up=tuple(side_normal(1.0, y)))
    sill_a, sill_b = side_point(1.0, GAP[0], 0.25, 0.03), side_point(1.0, GAP[1], 0.25, 0.03)
    k.beam(sill_a, sill_b, 0.12, 0.04, 'trim', 'Beams')
    deck(k)
    fittings(k, paint)


def deck(k):
    """The floorboards (fore and aft, on the trim sheet's siding), two thwarts and the cargo aft."""
    outline = []
    ys = stations(*DECK_Y)
    for y in ys:
        outline.append((side_point(1.0, y, f_at(y, DECK)).x - 0.02, y))
    for y in reversed(ys):
        outline.append((-(side_point(-1.0, y, f_at(y, DECK)).x * -1.0) + 0.02, y))
    # Floorboards: long WoodPlanks running fore and aft (stretched along the grain, so joints are few).
    obj = tk.slab(k, [(x, y) for x, y in outline], DECK - 0.06, DECK, 'planks')
    uv = obj.data.uv_layers.active.data
    for poly in obj.data.polygons:
        if poly.normal.z > 0.5:
            for loop in poly.loop_indices:
                co = obj.data.vertices[obj.data.loops[loop].vertex_index].co
                uv[loop].uv = (co.y / PLANK_REPEAT + 0.37, co.x * SU + 0.11)
    for y in (-2.25, 3.0):
        z = DECK + 0.42
        half = side_point(1.0, y, f_at(y, z)).x - 0.03
        k.box((2.0 * half, 0.26, 0.05), (0.0, y, z), 'trim', strip='Siding', axis='x')
        for x in (-0.5, 0.5):
            k.box((0.07, 0.07, 0.4), (x, y, DECK + 0.2), 'trim', strip='Beams', axis='z')
    rnd = k.rnd
    for x, y, size in ((-0.55, 3.75, 0.55), (0.0, 3.85, 0.48), (-0.5, 3.75, 0.0)):
        if size:
            k.box((size, size, size * 0.8), (x, y, DECK + size * 0.4), 'trim', strip='Beams', bevel=0.012,
                  rot=(0.0, 0.0, rnd.uniform(-8.0, 8.0)))
    k.lathe([(0.0, 0.0), (0.22, 0.0), (0.26, 0.3), (0.22, 0.6), (0.0, 0.6)], 'trim', (0.55, 3.75, DECK), sides=10,
            strip='Beams', grain='up')                                                    # a water barrel
    for z in (DECK + 0.12, DECK + 0.48):
        k.cyl((0.55, 3.75, z), (0.55, 3.75, z + 0.04), 0.255, 'trim', sides=10, strip='Iron', caps=(False, False))


def fittings(k, paint):
    """Iron cleats and the mooring ropes' ends (SOCKET_Mooring_1..4), rope coils, the bow lantern (SOCKET_Light)."""
    p = PAINTS[paint]
    n = 1
    for y in (-3.55, 4.05):
        for s in (1.0, -1.0):
            top = side_point(s, y, 1.0) + Vector((0.0, 0.0, 0.055))
            inner = top - Vector((s * 0.12, 0.0, 0.0))
            k.box((0.06, 0.24, 0.05), inner + Vector((0.0, 0.0, 0.025)), 'trim', strip='Iron', axis='y')
            end = top + Vector((s * 0.32, -0.08 if y < 0 else 0.08, -0.42))
            k.tube([inner + Vector((0.0, 0.0, 0.05)), top + Vector((s * 0.08, 0.0, 0.03)),
                    top + Vector((s * 0.2, 0.0, -0.15)), end], 0.022, p['rope'], sides=5)
            k.socket(f'Mooring_{n}', end, (0.0, 0.0, -s * 90.0))
            n += 1
    for x, y in ((0.3, -2.6), (-0.45, 2.55)):
        coil = k.lathe([(0.16 + 0.035 * math.cos(a), 0.035 * math.sin(a)) for a in (2.0 * math.pi * i / 5 for i in range(5))],
                       p['rope'], (x, y, DECK + 0.035), sides=12, closed=True)
        lp.place(coil, (0.0, 0.0, 0.0))
        k.lathe([(0.11 + 0.03 * math.cos(a), 0.03 * math.sin(a)) for a in (2.0 * math.pi * i / 5 for i in range(5))],
                p['rope'], (x + 0.03, y, DECK + 0.1), sides=10, closed=True)
    # The bow lantern on an iron bracket over the stem head.
    k.tube([(0.0, -4.55, 2.2), (0.0, -4.7, 2.38), (0.0, -4.9, 2.4), (0.0, -4.94, 2.3)], 0.02, 'trim', strip='Iron')
    tk.oil_lamp(k, (0.0, -4.94, 1.9), facing=(0.0, -1.0, 0.0), size=0.26, body='trim', strip='Iron', socket='Light',
                glass=p['lamp'])


# --- The bags ---

def cigar(d, length, radius):
    """Option A's bag: a soft, full cigar (round shoulders, blunt rounded ends), radius at distance d from the nose."""
    x = (2.0 * d / length) - 1.0
    return radius * max(0.0, 1.0 - abs(x) ** 2.6) ** (1.0 / 2.2)


def teardrop(d, length, radius, widest=0.3):
    """Option B's bag: a blunt nose, its widest point a third back, tapering to a point at the tail."""
    w = length * widest
    if d <= w:
        return radius * max(0.0, 1.0 - ((w - d) / w) ** 2) ** 0.5
    t = (d - w) / (length - w)
    return max(radius * (1.0 - t ** 1.7), 0.05)


def bag_profile(shape, length, radius, count=15):
    ds = [length * (0.5 - 0.5 * math.cos(math.pi * i / (count - 1))) for i in range(count)]
    return [(shape(d, length, radius), d) for d in ds]


def bag(k, shape, nose_y, length, radius, cz, paint, seed, sides=24, count=15, patches=None):
    """The gas-bag along Y (nose toward -Y), and its patches (see patch_on)."""
    p = PAINTS[paint]
    profile = [(r, d) for r, d in bag_profile(shape, length, radius, count)]
    profile[0] = (0.0, profile[0][1])
    profile[-1] = (0.0, profile[-1][1])
    k.lathe(profile, p['bag'], (0.0, nose_y, cz), (0.0, nose_y + length, cz), sides=sides)
    rnd = random.Random(seed)
    if patches is None:
        patches = [(0.22, 70.0, 0.6, 0.45, 0.0, 0.015), (0.38, 128.0, 0.6, 0.45, 0.0, 0.015),
                   (0.55, 35.0, 0.6, 0.45, 0.0, 0.015), (0.7, 95.0, 0.6, 0.45, 0.0, 0.015)]
    for t, phi, w, h, turn, lift in patches:
        patch_on(k, shape, nose_y, length, radius, cz, length * t, phi, w, h, turn, lift, p['patch'], p['bag'],
                 rnd.randint(0, 10 ** 6))


# Option A's patches: (place along the bag 0..1, angle round it from +X toward +Z, width along it, height round it,
# turn in degrees, lift off the bag). Few and small, of uneven size, turned a little; two pairs overlap.
A_PATCHES = [(0.24, 42.0, 0.62, 0.46, 8.0, 0.012), (0.27, 55.0, 0.34, 0.3, -14.0, 0.02),
             (0.5, 18.0, 0.7, 0.42, -6.0, 0.012), (0.66, 66.0, 0.4, 0.34, 16.0, 0.012),
             (0.76, 30.0, 0.52, 0.38, 3.0, 0.012), (0.785, 40.0, 0.28, 0.26, -22.0, 0.02),
             (0.42, 138.0, 0.36, 0.48, -18.0, 0.012)]


def paint_part(k, obj, mat):
    """Maps a part with a material key, or a (trim, strip) pair mapped face by face."""
    if isinstance(mat, tuple):
        return k.map(obj, mat[0], mat[1], faces=True)
    return k.map(obj, mat)


def patch_on(k, shape, nose_y, length, radius, cz, dc, ac, w, h, turn, lift, mat, hem, seed):
    """A canvas patch sewn onto the bag: an uneven eight-sided piece w along the bag by h round it, turned turn degrees,
    lift off the bag, in a slightly different canvas (mat), with a thin stitched hem just inside its edge in the bag's
    own canvas (hem) and a narrow edge down onto the bag."""
    rnd = random.Random(seed)
    c, s_ = math.cos(math.radians(turn)), math.sin(math.radians(turn))

    def point(u, v, off):
        """u metres along the bag, v metres round it, from the patch's middle; off metres out from the bag."""
        d = dc + u
        r = shape(d, length, radius)
        a = math.radians(ac) + v / max(r, 0.2)
        return Vector(((r + off) * math.cos(a), nose_y + d, cz + (r + off) * math.sin(a)))
    outline = []
    count = 7
    angles = sorted(2.0 * math.pi * (i + rnd.uniform(-0.3, 0.3)) / count for i in range(count))
    for ang in angles:
        # A rough, squarish piece of cloth: a superellipse with every corner pulled in or out.
        cu, cv = math.cos(ang), math.sin(ang)
        mag = (abs(cu) ** 3 + abs(cv) ** 3) ** (-1.0 / 3.0) * rnd.uniform(0.72, 1.1)
        u, v = cu * mag * w * 0.5, cv * mag * h * 0.5
        outline.append((u * c - v * s_, u * s_ + v * c))
    bm = lp.new_bmesh()
    middle = bm.verts.new(point(0.0, 0.0, lift))
    ring = [bm.verts.new(point(u, v, lift)) for u, v in outline]
    for i in range(count):
        bm.faces.new((middle, ring[i], ring[(i + 1) % count]))
    outward(bm, cz)
    paint_part(k, lp.mesh_object(bm), mat)
    # The edge: a narrow strip from the patch down onto the bag.
    bm = lp.new_bmesh()
    top = [bm.verts.new(point(u, v, lift)) for u, v in outline]
    low = [bm.verts.new(point(u, v, -0.004)) for u, v in outline]
    for i in range(count):
        j = (i + 1) % count
        bm.faces.new((top[i], top[j], low[j], low[i]))
    centre = sum((v.co for v in top), Vector()) / count
    for f in bm.faces:
        f.normal_update()
        if f.normal.dot(f.calc_center_median() - centre) < 0.0:
            f.normal_flip()
    paint_part(k, lp.mesh_object(bm), mat)
    # The hem: a fine cord of stitching just inside the edge, close in tone to the patch.
    hem_line = [point(u * 0.84, v * 0.84, lift + 0.003) for u, v in outline]
    k.tube(hem_line + hem_line[:1], 0.0045, hem, sides=3)


def outward(bm, cz):
    """Turns a bmesh's faces to face away from the bag's axis (the line x = 0, z = cz)."""
    for f in bm.faces:
        f.normal_update()
        c = f.calc_center_median()
        if f.normal.dot(c - Vector((0.0, c.y, cz))) < 0.0:
            f.normal_flip()


def netting(k, shape, nose_y, length, radius, cz, rope):
    """Option A's rope net: four bands round the bag and five lines along it, and the load lines down to the
    gunwales (clear of the gangplank's opening)."""
    def point(d, a, off=0.03):
        r = shape(d, length, radius) + off
        return Vector((r * math.cos(math.radians(a)), nose_y + d, cz + r * math.sin(math.radians(a))))
    for t in (0.2, 0.38, 0.62, 0.8):
        d = length * t
        ring = [point(d, 360.0 * i / 28) for i in range(29)]
        # Oriented round the bag's axis: with the default up (+Z) the rope's section turned over where the ring runs
        # straight down its side, and that twisted segment mapped with no UV area.
        k.tube(ring, 0.022, rope, sides=3, up=(0.0, 1.0, 0.0))
    for a in (90.0, 38.0, 142.0, -22.0, -158.0):
        k.tube([point(length * (0.04 + 0.92 * i / 17), a) for i in range(18)], 0.022, rope, sides=3)
    # Each load point fans down to two gunwale eyes; none lands in the gangplank's opening (+X, y 0.1..1.1).
    fans = [(-3.4, (-3.0, -2.2)), (-1.6, (-2.0, -1.1)), (-0.2, (-0.5, 0.0)), (1.6, (1.3, 2.1)), (3.0, (2.7, 3.5)),
            (4.2, (3.9, 4.35))]
    for a, s in ((-22.0, 1.0), (-158.0, -1.0)):
        for y_bag, targets in fans:
            top = point(y_bag - nose_y, a, 0.03)
            for y_hull in targets:
                k.tube([top, side_point(s, y_hull, 1.0) + Vector((0.0, 0.0, 0.05))], 0.015, rope, sides=3)


def keel_frame(k, shape, nose_y, length, radius, cz, paint):
    """Option B's light timber keel under the bag: a keel beam, four spars, struts down to the gunwales, straps round
    the bag and rope stays to the bow and stern."""
    p = PAINTS[paint]
    def bottom(d):
        return cz - shape(d, length, radius) - 0.07
    keel = [(0.0, nose_y + d, bottom(d)) for d in [length * (0.12 + 0.76 * i / 9) for i in range(10)]]
    part = lp.sweep(keel, [(-0.06, -0.06), (0.06, -0.06), (0.06, 0.06), (-0.06, 0.06)], up=(0.0, 0.0, 1.0))
    lp.grain(part, 'Beams', axis=(0.0, 1.0, 0.0), seed=k.seed())
    k.add(part)
    for y in (-2.8, -0.6, 1.8, 3.8):
        d = y - nose_y
        z = bottom(d)
        k.beam((-0.8, y, z), (0.8, y, z), 0.09, 0.1, 'trim', 'Beams')
        for s in (-1.0, 1.0):
            k.beam((s * 0.74, y, z - 0.04), side_point(s, y, 1.0) + Vector((0.0, 0.0, 0.05)), 0.07, 0.07, 'trim',
                   'Beams', up=(0.0, 1.0, 0.0))
        r = shape(d, length, radius) + 0.02
        k.lathe([(r, 0.0), (r, 0.14)], 'trim', (0.0, y - 0.07, cz), (0.0, y + 0.07, cz), sides=24, strip='Iron')
    for y_bag, y_hull in ((-4.6, -4.4), (5.6, 4.4)):
        top = (0.0, y_bag, bottom(y_bag - nose_y) + 0.05)
        for s in (-1.0, 1.0):
            k.tube([top, side_point(s, y_hull, 1.0) + Vector((0.0, 0.0, 0.06))], 0.016, p['rope'], sides=3)


def fin_and_rudder(k, shape, nose_y, length, radius, cz, paint):
    """Option B's stern fin on top of the bag's tail (a timber frame skinned in canvas) and its rudder, with the
    rudder's control lines down to the stern."""
    p = PAINTS[paint]
    def top(y):
        return cz + shape(y - nose_y, length, radius)
    tail = nose_y + length
    y0, y1 = tail - 3.4, tail - 0.5
    pts = [(0.0, y0, top(y0) - 0.02), (0.0, y1, top(y1) + 1.55), (0.0, tail - 0.15, top(tail - 0.15) + 1.6),
           (0.0, tail - 0.15, top(tail - 0.15) - 0.02)]
    skin = [(y, z) for _, y, z in pts]
    k.panel([skin], 0.03, p['bag'], tk.frame('+x', (0.015, 0.0, 0.0)))
    for a, b in zip(pts, pts[1:]):
        k.beam(a, b, 0.05, 0.06, 'trim', 'Beams', up=(1.0, 0.0, 0.0))
    # The rudder: a framed canvas panel hinged on the fin's trailing edge, turned a little.
    hinge_y, z0, z1 = tail - 0.12, top(tail - 0.15) + 0.05, top(tail - 0.15) + 1.5
    rud = [(0.0, 0.0, z0), (0.0, 0.0, z1), (0.0, 0.75, z1 - 0.15), (0.0, 0.75, z0 + 0.25)]
    turn = Matrix.Rotation(math.radians(12.0), 4, 'Z')
    world = [Vector((0.0, hinge_y, 0.0)) + turn @ Vector(c) for c in rud]
    side = (turn @ Vector((1.0, 0.0, 0.0))) * 0.012
    slab = tk.loft([[c - side for c in world], [c + side for c in world]])
    k.map(slab, p['bag'])
    for a, b in zip(world, world[1:] + world[:1]):
        k.beam(a, b, 0.04, 0.05, 'trim', 'Beams', up=tuple(turn @ Vector((1.0, 0.0, 0.0))))
    for s in (-1.0, 1.0):
        k.tube([world[3] + Vector((s * 0.03, 0.0, 0.0)), side_point(s, 4.2, 1.0) + Vector((0.0, 0.0, 0.06))], 0.012,
               p['rope'], sides=3)


def sweep_oar(k, paint):
    """Option A's steering sweep over the stern, resting in an iron crutch."""
    k.box((0.08, 0.08, 0.3), (0.45, 4.48, 1.7), 'trim', strip='Iron', axis='z')
    k.tube([(0.45, 3.6, 1.95), (0.5, 4.48, 1.82), (0.7, 6.3, 1.0)], 0.04, 'trim', sides=6, strip='Beams')
    k.box((0.05, 1.0, 0.34), (0.73, 6.55, 0.86), 'trim', strip='Siding', axis='y', rot=(-24.0, 0.0, 6.0))


# --- The models ---

def skiff(option, paint):
    name = f'Skiff_{option}_{paint}'
    k = tk.Kit(name, 61 if option == 'A' else 62)
    hull(k, paint)
    k.section('hull')
    if option == 'A':
        args = (cigar, -5.7, 12.0, 1.95, DECK + 3.2 + 1.95)
        bag(k, *args, paint, 71, sides=40, count=21, patches=A_PATCHES)
        k.section('bag')
        netting(k, *args, PAINTS[paint]['rope'])
        sweep_oar(k, paint)
        k.section('netting')
    else:
        args = (teardrop, -5.6, 12.8, 1.85, DECK + 3.25 + 1.85)
        bag(k, *args, paint, 72)
        k.section('bag')
        keel_frame(k, *args, paint)
        fin_and_rudder(k, *args, paint)
        k.section('frame')
    k.socket('Deck', (0.0, -1.2, DECK))
    gap_mid = (GAP[0] + GAP[1]) * 0.5
    hinge = side_point(1.0, gap_mid, 0.25, 0.05)
    k.socket('Gangplank', (hinge.x, gap_mid, DECK + 0.02))
    collision(k)
    # A lighter occlusion bake than the props': the open boat should read as weathered wood in daylight.
    return k.finish(lods='50,25,10', screens='0.5,0.25,0.08', ao=0.55, ao_strength=0.75)


def collision(k):
    """The walkable deck, the side slabs (open at the gangplank's opening), the bow wedge and the transom."""
    y0, y1 = DECK_Y
    floor = []
    for y in stations(y0, y1, 1.0):
        half = side_point(1.0, y, f_at(y, DECK)).x - 0.02
        floor += [Vector((x, y, z)) for x in (-half, half) for z in (DECK - 0.12, DECK)]
    k.hull_points(floor)
    top = DECK + 1.45
    for s in (-1.0, 1.0):
        spans = [(y0, -0.6), (-0.6, GAP[0]), (GAP[1], 4.52)] if s > 0 else [(y0, -0.6), (-0.6, 2.0), (2.0, 4.52)]
        for a, b in spans:
            pts = []
            for y in (a, b):
                inner = side_point(s, y, f_at(y, DECK), -0.03)
                outer = side_point(s, y, 1.0, 0.08)
                for z in (DECK, top):
                    pts += [Vector((inner.x, y, z)), Vector((outer.x + s * 0.02, y, z))]
            k.hull_points(pts)
    bow = [Vector((x, y, z)) for x, y in ((-0.95, y0), (0.95, y0), (0.0, -4.6)) for z in (DECK - 0.1, DECK + 1.6)]
    k.hull_points(bow)
    k.hull((2.1, 0.12, 1.6), (0.0, 4.5, DECK + 0.7))


def gangplank():
    """Skiff_Gangplank: 3 m of boards on three battens, anti-slip cleats, kick rails, iron hinge straps round the
    pin. Origin on the hinge; lies level along +X. One slim box hull (walkable)."""
    k = tk.Kit('Skiff_Gangplank', 81)
    w = 0.86
    for i in range(4):
        y = -w * 0.5 + w * (i + 0.5) / 4
        k.box((PLANK - 0.04, w / 4 - 0.01, 0.045), (PLANK * 0.5, y, 0.03), 'trim', strip='Siding', axis='x')
    for x in (0.35, PLANK * 0.5, PLANK - 0.3):
        k.box((0.12, w, 0.06), (x, 0.0, -0.025), 'trim', strip='Beams', axis='y')
    for i in range(7):
        x = 0.3 + (PLANK - 0.6) * i / 6
        k.box((0.04, w - 0.08, 0.025), (x, 0.0, 0.065), 'trim', strip='Beams', axis='y')
    for s in (-1.0, 1.0):
        k.box((PLANK, 0.05, 0.1), (PLANK * 0.5, s * (w * 0.5 + 0.02), 0.07), 'trim', strip='Beams', axis='x')
        k.box((0.32, 0.06, 0.012), (0.17, s * 0.26, 0.06), 'trim', strip='Iron', axis='x')
        k.cyl((0.0, s * 0.26 - 0.06, 0.0), (0.0, s * 0.26 + 0.06, 0.0), 0.035, 'trim', sides=8, strip='Iron')
    k.hull((PLANK, w + 0.06, 0.08), (PLANK * 0.5, 0.0, 0.03))
    return k.finish(lods='50,25', screens='0.3,0.1', ao=0.2, ground=False)


# The user picked option A (2026-10-02). Option B is still built for previews, to compare, but never exported.
models = {}
for option in (('A', 'B') if lt.want_preview() else ('A',)):
    for paint in ('Packet', 'Gang'):
        models[f'{option}_{paint}'] = skiff(option, paint)
plank = gangplank()


def attach_plank(body, raised=False):
    """A copy of the gangplank at the skiff's SOCKET_Gangplank, lowered or raised (for previews)."""
    bpy.context.view_layer.update()
    socket = next(c for c in body.children if c.name.startswith('SOCKET_Gangplank'))
    copy = bpy.data.objects.new('PV_Gangplank', plank.data)
    bpy.context.scene.collection.objects.link(copy)
    turn = Matrix.Rotation(math.radians(-90.0 if raised else 0.0), 4, 'Y')
    copy.matrix_world = socket.matrix_world @ turn
    return copy


if lt.want_preview() and __name__ == '__main__':
    import sys
    # Option A is rendered; add 'B' after '--' to render option B too, 'plank' for the gangplank's views.
    args = [w for a in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for w in a.split(',')]
    folder = os.path.join(lt.PREVIEW_DIR, 'RansomsRest', 'Skiff')
    plank.hide_render = True
    for key, body in models.items():
        if key.startswith('B') and 'B' not in args:
            continue
        tk.render([body], body.name + '_front', view=(1.0, -1.25, 0.42), lens=40.0, fit=0.62, folder=folder,
                  ground_at=-1.5)
        tk.render([body], body.name + '_side', view=(1.0, -0.02, 0.1), lens=45.0, fit=0.62, resolution=(1600, 800),
                  folder=folder, ground_at=-1.5)
        # The deck from the player's eyes on SOCKET_Deck (1.65 m over it), looking forward and a little down.
        eye = Vector((0.0, -1.2, DECK + 1.65))
        tk.closeup([body], f'{body.name}_deck', eye + Vector((0.0, -1.0, -0.42)), (0.0, 1.0, 0.42), 1.0, lens=18.0,
                   folder=folder)
    if 'plank' in args:
        body = models['A_Packet']
        for raised in (False, True):
            copy = attach_plank(body, raised)
            tk.closeup([body, copy], 'Skiff_gangplank_' + ('raised' if raised else 'lowered'), (1.6, 0.6, 1.2),
                       (1.0, -0.75, 0.35), 7.0, lens=35.0, folder=folder)
            tk.remove([copy])
