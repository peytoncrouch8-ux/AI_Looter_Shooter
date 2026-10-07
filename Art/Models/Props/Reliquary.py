"""The smashed Reliquary of Saint Ada (Docs/Areas/RansomsRest.md: the Chapel of Saint Ada, Main 4 "Hallowed Ground",
and the art needs table): the reliquary held the saint's ember, until the gang smashed it, and now the bell can't call
the dead to rest. Inside the chapel it is a story prop, dark and empty. A scripted model file (Art/README.md).

The design is the backlog Reliquary's (Art/Backlog/Loot/Chests.py, reliquary(): the legendary loot chest, which stays
the future loot chest): a 1.02 m granite chest on a stepped plinth, gold corner pillars and bands, a rune diamond on
the front, a two-slab lid with a gold rim and ridge and a gem in its middle. This file copies the parts of that design
it needs and the helpers that build them (part, block, oriented, ring, disc, wrap, the sizes) instead of importing
Chests.py, which builds all three chests whenever it runs.

  Reliquary_Smashed
              the same chest, violently broken. The lid has been prised askew (turned 10 degrees and shoved 10 cm off
              to the left, overhanging the left end) and its front right corner smashed off, upper slab and all,
              the bits on the plinth and in the chest; a crack runs across what is left. The ember's setting in the
              middle of the lid is empty: the gilt boss with its claws prised open and one snapped, a few teeth of the
              glass that covered the ember still standing in its rim and shards on the lid and the plinth. On the
              front, the rune diamond is wrenched askew in its frame and its gem prised out; the rune lines are dead
              (dark stone on the granite, pale stone on the dark inlay), and a bite is knocked out of the front
              wall's top left, with a crack running down from it. The front left pillar is snapped (its stump on the
              base, a stub under the capital, the shaft lying beside the plinth), the upper front band is torn off the
              wall and hangs curled over the bite, the upper left end band and part of the plinth's band are gone,
              the hinges torn off. The plinth's corners are chipped and cracked. Inside it is empty and dark: soot
              on the granite and on the gilt lining.

Pivot: its foot, at the middle of the plinth's footprint, front (-Y) toward the nave. It stands on the chapel's
SOCKET_Reliquary (Art/Models/Buildings/Chapel.py: the top of the stone plinth in the apse, 1.56 x 1.02 m): everything,
the fallen shaft and the fragments included, stays on that top (within 0.76 m of the pivot across, 0.50 m front to
back) and nothing lies below it.
  SOCKET_Ember     the lid's empty setting, on the seat where the ember lay; its +Z points up out of the setting (the
                   lid's up), its front the lid's front: where the Grave Sight flash shows the ember lifting off
  SOCKET_Interact  on the front, in the middle of the rune diamond (the chest's middle height), facing out
Collision: three convex hulls (the plinth, the chest, the lid). No Nanite (a small prop), LODs at 50% and 25%.
Materials (4 slots), all dark or dull, nothing glows:
  ChestGranite    RockGranite, untinted: the backlog chest's granite (no moss: it stands indoors)
  ChestRuneStone  RockGranite tinted 0x4a423b: the backlog's dark rune stone; here the dead rune lines, the inlay, the
                  empty settings' seats and the cracks
  BrassWorn       the brief's shared brass: the gold, tarnished
  HouseTrim       the trim sheet's dark window glass (strip H4): the broken glass
Vertex occlusion: the usual contact pass (0.3 m), then the chapel's interior pass (Chapel.py bakes its inside the same
way) against a plain stand-in of the apse and nave around the socket, so that on Medium, where nothing else shades the
sky light indoors, the chest is as dim as the apse's own walls. Moved out of the chapel it would look too dark. Soot
darkens the inside further.

    blender -b --factory-startup --python-expr "import sys; sys.path.insert(0, 'Tools/Blender')" \\
        --python Art/Models/Props/Reliquary.py -- --preview
renders Saved/ArtPreviews/RansomsRest/Reliquary/: Reliquary.png and Reliquary_Back.png (outdoor preview light), and
in the chapel (Intermediate/ArtExport_RR_Chapel/SM_Chapel.fbx, imported, with the chest on its SOCKET_Reliquary)
under the golden late afternoon sun (west-southwest, 15 degrees up, as on Ransom's Rest): Reliquary_Nave.png (from
the middle of the nave), Reliquary_ThreeQuarter.png, Reliquary_Ember.png (the lid's setting, SOCKET_Ember marked by a
cyan arrow along its +Z) and Reliquary_BeforeAfter.png (the backlog Reliquary intact and glowing, then smashed).
"""
import json
import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_model as lm
import looter_props as lp
import looter_textures as lt

# --- The backlog Reliquary's sizes (Chests.py, reliquary()) ---
SX, SY, SZ, WALL, BASE = 1.02, 0.62, 0.42, 0.07, 0.2
H = BASE + SZ                       # the top of the walls, where the lid sits
OX, OY = SX * 0.5, SY * 0.5
FY, CZ = -(OY + 0.004), BASE + SZ * 0.5   # the rune's plane and height
LOW, UP = 0.07, 0.09                # the lid's lower and upper slabs
LID_X, LID_Y = OX + 0.035, OY + 0.035     # the lower slab's half extents
UP_X, UP_Y = (SX - 0.14) * 0.5, (SY - 0.16) * 0.5   # the upper slab's
SEAT = H + LOW + UP + 0.038         # the empty setting's seat, where the ember lay

# The prised lid: turned about its middle, then shoved back and to the left.
LID_TURN, LID_SHOVE = 10.0, (-0.10, 0.035, 0.002)
# The lid's broken edge, from its front edge to its right end: the front right corner is gone.
BREAK = [(0.20, -LID_Y), (0.235, -0.285), (0.205, -0.19), (0.285, -0.15), (0.33, -0.10), (0.39, -0.095),
         (0.49, -0.04), (LID_X, -0.02)]

# --- Materials: four slots ---
GRANITE = lt.material('RockGranite', name='ChestGranite')
RUNE = lt.material('RockGranite', name='ChestRuneStone', tint=0x4a423b)
GILT = lt.material('MetalWorn', name='BrassWorn', tint=0xc49c56)
# The glass is HouseTrim's strip H4 (shard() maps it with lp.trim).
TILEABLE = {GRANITE.name: 'RockGranite', RUNE.name: 'RockGranite', GILT.name: 'MetalWorn'}


# --- Parts (Chests.py's helpers) ---

def part(obj, mat, seed=0):
    """Puts mat on a part and maps it at world scale (small parts: the texture's own density)."""
    lt.assign(obj, mat)
    if mat.name in TILEABLE:
        lt.box_uv(obj, TILEABLE[mat.name], seed=seed)
    return obj


def block(size, center, mat, seed=0, bevel=0.0, rotation=(0.0, 0.0, 0.0)):
    return part(lp.block(size, center, rotation, bevel=bevel), mat, seed)


def oriented(obj, center, normal):
    """Turns a part built round +Z to face normal, then moves it to center."""
    q = Vector(normal).normalized().to_track_quat('Z', 'Y')
    obj.data.transform(q.to_matrix().to_4x4())
    return lp.place(obj, center)


def ring(major, minor, center, normal, mat, segments=20, sides=6):
    profile = [(major + math.cos(a) * minor, math.sin(a) * minor) for a in (2.0 * math.pi * k / sides for k in range(sides))]
    obj = lp.lathe(profile, segments=segments, closed=True)[0]
    return oriented(part(obj, mat), center, normal)


def disc(radius, depth, center, normal, mat, segments=20):
    obj = lp.lathe([(0.0, 0.0), (radius, 0.0), (radius, depth), (0.0, depth)], segments=segments)[0]
    return oriented(part(obj, mat), center, normal)


def wrap(outer_x, outer_y, z, h, t, mat, gap_x=0.0, gap_y=0.0, bevel=0.0, seed=0, skip=()):
    """Four plates hugging a box of half extents outer_x, outer_y: front and back along X, the ends along Y. skip
    leaves plates out: 'front', 'back', 'left', 'right'."""
    parts = []
    for s, along, end in ((-1.0, 'front', 'left'), (1.0, 'back', 'right')):
        if along not in skip:
            parts.append(block((2.0 * outer_x + 2.0 * t - gap_x, t, h), (0.0, s * (outer_y + t * 0.5), z), mat, seed, bevel))
        if end not in skip:
            parts.append(block((t, 2.0 * outer_y - gap_y, h), (s * (outer_x + t * 0.5), 0.0, z), mat, seed, bevel))
    return parts


# --- Broken pieces ---

def inset(outline, offsets):
    """The outline (counterclockwise, XY) with each edge moved inward by its offset (edge i runs from point i to i+1)."""
    n = len(outline)
    pts = [Vector(p) for p in outline]
    lines = []
    for i in range(n):
        d = (pts[(i + 1) % n] - pts[i]).normalized()
        lines.append((pts[i] + Vector((-d.y, d.x)) * offsets[i], d))
    out = []
    for i in range(n):
        (p1, d1), (p2, d2) = lines[i - 1], lines[i]
        cross = d1.x * d2.y - d1.y * d2.x
        if abs(cross) < 1e-6:
            out.append(p2.copy())
        else:
            t = ((p2 - p1).x * d2.y - (p2 - p1).y * d2.x) / cross
            out.append(p1 + d1 * t)
    return out


def on_rect(p, q, rect, eps=1e-6):
    """True when the edge p-q lies along a side of rect (x0, y0, x1, y1): an outer edge, not a break."""
    x0, y0, x1, y1 = rect
    return any(abs(p[i] - v) < eps and abs(q[i] - v) < eps for i, v in ((0, x0), (0, x1), (1, y0), (1, y1)))


def clip_rect(poly, rect):
    """The part of a polygon (counterclockwise, maybe concave) inside a rectangle (Sutherland-Hodgman)."""
    x0, y0, x1, y1 = rect
    for axis, value, keep in ((0, x0, 1.0), (0, x1, -1.0), (1, y0, 1.0), (1, y1, -1.0)):
        out = []
        for i, e in enumerate(poly):
            s = poly[i - 1]
            e_in, s_in = (e[axis] - value) * keep >= -1e-9, (s[axis] - value) * keep >= -1e-9
            if e_in != s_in:
                t = (value - s[axis]) / (e[axis] - s[axis])
                hit = [s[0] + (e[0] - s[0]) * t, s[1] + (e[1] - s[1]) * t]
                hit[axis] = value
                out.append(tuple(hit))
            if e_in:
                out.append(tuple(e))
        poly = [p for k, p in enumerate(out) if (Vector(p) - Vector(out[k - 1])).length > 1e-6]
    return poly


def prism(outline, z0, z1, chamfer=0.0, rect=None, bottom=True):
    """A slab of the outline (counterclockwise, XY) from z0 to z1. Its outer edges (those along a side of rect; all of
    them without one) are chamfered top and bottom; the others are breaks: square and sharp, as stone snaps. The top
    and bottom are triangulated (a broken outline is concave)."""
    n = len(outline)
    outer = [rect is None or on_rect(outline[i], outline[(i + 1) % n], rect) for i in range(n)]
    bm = lp.new_bmesh()

    def loop(points, z):
        return [bm.verts.new((p[0], p[1], z)) for p in points]
    if chamfer > 0.0:
        small = inset(outline, [chamfer if o else 0.0 for o in outer])
        rings = [loop(small, z0), loop(outline, z0 + chamfer), loop(outline, z1 - chamfer), loop(small, z1)]
    else:
        rings = [loop(outline, z0), loop(outline, z1)]
    for r0, r1 in zip(rings, rings[1:]):
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((r0[i], r0[j], r1[j], r1[i]))
    caps = [bm.faces.new(rings[-1])]
    if bottom:
        caps.append(bm.faces.new(list(reversed(rings[0]))))
    bmesh.ops.triangulate(bm, faces=caps, quad_method='BEAUTY', ngon_method='BEAUTY')
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return lp.mesh_object(bm)


def wall_xz(outline, y_out, depth):
    """A wall from an outline in X (across) and Z (up), counterclockwise as seen from the front: its outer face at
    y_out facing -Y, its inner face depth behind it."""
    obj = prism(outline, 0.0, depth)
    return lp.place(obj, (0.0, y_out + depth, 0.0), (90.0, 0.0, 0.0))


def chip(obj, size, center, chips):
    """Knocks corners off a box part (as Ruins.py's dressed blocks): each chip is (corner signs, depth)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for corner, depth in chips:
        n = Vector(corner).normalized()
        point = Vector(center) + Vector([c * s * 0.5 for c, s in zip(corner, size)]) - n * depth
        result = bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=point, plane_no=n,
                                        clear_outer=True)
        edges = [e for e in result['geom_cut'] if isinstance(e, bmesh.types.BMEdge)]
        if edges:
            bmesh.ops.edgeloop_fill(bm, edges=edges)
    bm.to_mesh(obj.data)
    bm.free()
    return obj


def tube(radius, length, center, mat, segments=12):
    """An upright open cylinder (a pillar shaft whose ends are hidden in its blocks)."""
    obj = lp.lathe([(radius, 0.0), (radius, length)], segments=segments)[0]
    return lp.place(part(obj, mat), center)


def inlay(size, center, normal, mat, seed=0, turn=0.0):
    """A flat strip (size: across, up) a hair proud of a face whose normal is normal: a dead rune line."""
    bm = lp.new_bmesh()
    a, b = size[0] * 0.5, size[1] * 0.5
    bm.faces.new([bm.verts.new((x, y, 0.0)) for x, y in ((-a, -b), (a, -b), (a, b), (-a, b))])
    obj = lp.mesh_object(bm)
    obj.data.transform(Matrix.Rotation(math.radians(turn), 4, 'Z'))
    q = Vector(normal).normalized().to_track_quat('Z', 'Y')
    obj.data.transform(q.to_matrix().to_4x4())
    return part(lp.place(obj, center), mat, seed)


def broken_tube(radius, length, seed, ends=(False, True), jag=0.016, segments=12, closed=True):
    """A cylinder along +Z from 0 to length whose broken ends (bottom, top) are jagged: the rim at uneven heights,
    capped by a fan to a point a little inside. closed=False leaves its whole end open (hidden in a block)."""
    rnd = random.Random(seed)
    bm = lp.new_bmesh()
    rims = []
    for z, broken in ((0.0, ends[0]), (length, ends[1])):
        rims.append([bm.verts.new((math.cos(2.0 * math.pi * k / segments) * radius,
                                   math.sin(2.0 * math.pi * k / segments) * radius,
                                   z + (rnd.uniform(-jag, jag) if broken else 0.0))) for k in range(segments)])
    for k in range(segments):
        k1 = (k + 1) % segments
        bm.faces.new((rims[0][k], rims[0][k1], rims[1][k1], rims[1][k]))
    for rim, broken, z, sign in ((rims[0], ends[0], 0.0, 1.0), (rims[1], ends[1], length, -1.0)):
        if broken:
            tip = bm.verts.new((rnd.uniform(-0.3, 0.3) * radius, rnd.uniform(-0.3, 0.3) * radius, z + sign * jag * 0.4))
            for k in range(segments):
                bm.faces.new((rim[k], rim[(k + 1) % segments], tip))
        elif closed:
            bm.faces.new(rim)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return lp.mesh_object(bm)


def strap(points, width, thick, mat, seed=0):
    """A bent strip along points: width across it (upright), thick through it; swap them for a strip lying flat."""
    obj = lp.sweep(points, [(-thick * 0.5, -width * 0.5), (thick * 0.5, -width * 0.5), (thick * 0.5, width * 0.5),
                            (-thick * 0.5, width * 0.5)], up=(0.0, 0.0, 1.0))
    return part(obj, mat, seed)


def crack(points, normal, width=0.007, seed=0, lift=0.0008):
    """A dark hairline crack: a thin ribbon along points on a surface facing normal, a hair proud of it, its ends
    tapered."""
    rnd = random.Random(seed)
    n = Vector(normal).normalized()
    bm = lp.new_bmesh()
    pairs = []
    last = len(points) - 1
    for i, p in enumerate(points):
        t = (Vector(points[min(i + 1, last)]) - Vector(points[max(i - 1, 0)])).normalized()
        side = t.cross(n).normalized()
        w = width * 0.5 * (0.3 if i in (0, last) else rnd.uniform(0.7, 1.2))
        base = Vector(p) + n * lift
        pairs.append((bm.verts.new(base - side * w), bm.verts.new(base + side * w)))
    for (a0, b0), (a1, b1) in zip(pairs, pairs[1:]):
        bm.faces.new((a0, b0, b1, a1))
    return part(lp.mesh_object(bm), RUNE, seed)


def shard(points, thick, center, rotation=(0.0, 0.0, 0.0), seed=0):
    """A sliver of broken glass: a triangle (points in its own XY) thick through, turned and moved into place."""
    obj = prism(points, -thick * 0.5, thick * 0.5)
    lp.place(obj, center, rotation)
    return lp.trim(obj, 'Glass', seed=seed)


def fragment(size, center, turn, seed):
    """A broken-off bit of granite: the hull of seeded points in size (x, y, z), flat underneath, lying on a surface
    at center (its bottom), turned about Z."""
    rnd = random.Random(seed)
    sx, sy, sz = size
    points = [(rnd.uniform(-0.5, 0.5) * sx, rnd.uniform(-0.5, 0.5) * sy, rnd.uniform(0.35, 1.0) * sz) for _ in range(6)]
    points += [(math.cos(a) * sx * 0.5 * rnd.uniform(0.75, 1.0), math.sin(a) * sy * 0.5 * rnd.uniform(0.75, 1.0), 0.0)
               for a in (2.0 * math.pi * (k + rnd.uniform(-0.15, 0.15)) / 5 for k in range(5))]
    bm = lp.new_bmesh()
    for p in points:
        bm.verts.new(p)
    hull = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in hull['geom_interior'] + hull['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(dict.fromkeys(inside)), context='VERTS')
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    obj = lp.mesh_object(bm)
    lp.place(obj, center, (0.0, 0.0, turn))
    return part(obj, GRANITE, seed)


def moved(parts, matrix):
    """Moves parts already mapped (their textures go with them)."""
    for obj in parts:
        obj.data.transform(matrix)
        obj.data.update()
    return parts


# --- The smashed Reliquary ---

def plinth():
    """The stepped granite plinth, its corners chipped and a crack up its front, its gilt band partly torn away."""
    parts = []
    lower = chip(lp.block((1.32, 0.88, 0.13), (0.0, 0.0, 0.065), bevel=0.02), (1.32, 0.88, 0.13), (0.0, 0.0, 0.065),
                 [((1, -1, 1), 0.075), ((-1, 1, 1), 0.04), ((-1, -1, 0.4), 0.03)])
    upper = chip(lp.block((1.2, 0.76, 0.07), (0.0, 0.0, 0.165), bevel=0.015), (1.2, 0.76, 0.07), (0.0, 0.0, 0.165),
                 [((-1, -1, 1), 0.035), ((1, 1, 1), 0.025)])
    parts += [part(lower, GRANITE, 1), part(upper, GRANITE, 2)]
    # The band round the upper step: its front torn off from the left corner, the torn end sprung out.
    parts += wrap(0.6, 0.38, 0.19, 0.02, 0.008, GILT, skip=('front',))
    y = -(0.38 + 0.004)
    parts.append(block((0.858, 0.008, 0.02), (0.179, y, 0.19), GILT))
    parts.append(strap([(-0.25, y, 0.19), (-0.29, y - 0.012, 0.186), (-0.322, y - 0.03, 0.178)], 0.02, 0.008, GILT))
    parts.append(crack([(0.31, -0.44, 0.024), (0.292, -0.44, 0.05), (0.318, -0.44, 0.08), (0.296, -0.44, 0.106)],
                       (0.0, -1.0, 0.0), seed=3))
    parts.append(crack([(-0.58, 0.17, 0.2), (-0.556, 0.212, 0.2), (-0.53, 0.2, 0.2)], (0.0, 0.0, 1.0), seed=4))
    return parts


# The bite out of the front wall's top left, from right to left.
BITE = [(-0.12, H), (-0.165, H - 0.035), (-0.205, H - 0.06), (-0.232, H - 0.112), (-0.275, H - 0.098),
        (-0.31, H - 0.142), (-0.348, H - 0.118), (-0.382, H - 0.07), (-0.418, H - 0.045), (-0.45, H)]


def chest():
    """The granite box (the front wall bitten at its top left), its floor and soot-dark gilt lining."""
    outline = [(-OX, BASE), (OX, BASE), (OX, H)] + BITE + [(-OX, H)]
    parts = [part(wall_xz(outline, -OY, WALL), GRANITE, 21),
             block((SX, WALL, SZ), (0.0, (SY - WALL) * 0.5, BASE + SZ * 0.5), GRANITE, 22)]
    for s, seed in ((-1.0, 23), (1.0, 24)):
        parts.append(block((WALL, SY - 2 * WALL, SZ), (s * (SX - WALL) * 0.5, 0.0, BASE + SZ * 0.5), GRANITE, seed))
    parts.append(block((SX - 2 * WALL, SY - 2 * WALL, 0.07), (0.0, 0.0, BASE + 0.035), GILT, 8))
    # A crack down the front from the bite, across the dead rune line, to the lower band.
    parts.append(crack([(-0.306, -OY, H - 0.15), (-0.285, -OY, 0.44), (-0.296, -OY, 0.418), (-0.258, -OY, 0.38),
                        (-0.27, -OY, 0.33), (-0.232, -OY, 0.29), (-0.246, -OY, 0.252)], (0.0, -1.0, 0.0), seed=5,
                       lift=0.0016))   # over the dead line it splits
    # And one down the right end, under the lid's lost corner.
    parts.append(crack([(OX, -0.17, H - 0.004), (OX, -0.135, H - 0.06), (OX, -0.16, H - 0.11), (OX, -0.112, H - 0.17),
                        (OX, -0.125, H - 0.22)], (1.0, 0.0, 0.0), seed=6))
    return parts


def pillars():
    """The gilt corner pillars on their blocks. The front left one is snapped: a stump on its base, a stub under its
    capital, the shaft fallen beside the plinth."""
    parts = []
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            cx, cy = x * (OX + 0.005), y * (OY + 0.005)
            parts.append(block((0.11, 0.11, 0.045), (cx, cy, BASE + 0.0225), GILT))   # low down: no chamfer needed
            parts.append(block((0.11, 0.11, 0.045), (cx, cy, H - 0.0225), GILT, bevel=0.006))
            if x < 0.0 and y < 0.0:
                stump = part(broken_tube(0.038, 0.075, 31, ends=(False, True), jag=0.02, closed=False), GILT, 31)
                stub = part(broken_tube(0.038, 0.032, 32, ends=(True, False), jag=0.014, closed=False), GILT, 32)
                parts += [lp.place(stump, (cx, cy, BASE + 0.045)), lp.place(stub, (cx, cy, H - 0.045 - 0.032))]
            else:
                parts.append(tube(0.038, SZ - 0.09, (cx, cy, BASE + 0.045), GILT))
    # The shaft, rolled to the foot of the plinth on the left: lying on the chapel's plinth top, along Y.
    shaft = part(broken_tube(0.038, 0.235, 33, ends=(True, True), jag=0.018), GILT, 33)
    parts.append(lp.place(shaft, (-0.718, -0.26, 0.038), (-90.0, 0.0, -4.0)))
    return parts


def bands():
    """The gilt bands round the box. The upper front one is torn off over the bite and hangs curled; the upper left
    end's is gone."""
    parts = wrap(OX, OY, BASE + 0.035, 0.025, 0.008, GILT, gap_x=0.1, gap_y=0.1)
    z, y = H - 0.035, -(OY + 0.004)
    parts += wrap(OX, OY, z, 0.025, 0.008, GILT, gap_x=0.1, gap_y=0.1, skip=('front', 'left'))
    parts.append(block((0.518, 0.008, 0.025), (0.209, y, z), GILT))
    parts.append(strap([(-0.05, y, z), (-0.12, y - 0.004, z - 0.002), (-0.19, y - 0.021, z - 0.011),
                        (-0.25, y - 0.048, z - 0.029), (-0.295, y - 0.078, z - 0.057), (-0.322, y - 0.108, z - 0.095)],
                       0.025, 0.008, GILT))
    # The hinge leaves on the back, their knuckles torn off.
    for x in (-0.3, 0.3):
        parts.append(block((0.1, 0.006, 0.06), (x, OY + 0.003, H - 0.04), GILT))
    return parts


def rune():
    """The rune on the front, dead: the gilt diamond wrenched askew (turned and prised out at the top), its dark inlay
    with pale dead lines and an empty setting where the gem was prised out; dark lines out to the ends."""
    r = 0.07
    diamond = [block((0.2, 0.012, 0.2), (0.0, 0.0, 0.0), GILT, rotation=(0.0, 45.0, 0.0), bevel=0.004),
               block((0.15, 0.006, 0.15), (0.0, -0.009, 0.0), RUNE, 9, rotation=(0.0, 45.0, 0.0))]
    for xs in (-1.0, 1.0):
        for zs in (-1.0, 1.0):
            diamond.append(inlay((r * 1.414 + 0.012, 0.012), (xs * r * 0.5, -0.0126, zs * r * 0.5), (0.0, -1.0, 0.0),
                                 GRANITE, 16, turn=-45.0 * xs * zs))
    diamond.append(ring(0.019, 0.0045, (0.0, -0.0135, 0.0), (0.0, -1.0, 0.0), GILT, segments=8, sides=4))
    corner = 0.1 * math.sqrt(2.0)   # it pivots on its bottom corner, still against the wall
    moved(diamond, Matrix.Translation((0.0, FY, CZ - corner)) @ Matrix.Rotation(math.radians(9.0), 4, 'X')
          @ Matrix.Translation((0.0, 0.0, corner)) @ Matrix.Rotation(math.radians(12.0), 4, 'Y'))
    parts = diamond
    y = -OY - 0.0006
    for s in (-1.0, 1.0):
        parts.append(inlay((0.22, 0.01), (s * 0.24, y, CZ), (0.0, -1.0, 0.0), RUNE, 13))
        parts.append(inlay((0.01, 0.12), (s * 0.35, y, CZ), (0.0, -1.0, 0.0), RUNE, 14))
        parts.append(inlay((0.16, 0.01), (s * (OX + 0.0006), 0.0, CZ), (s, 0.0, 0.0), RUNE, 15))
    return parts


def lid_matrix():
    return Matrix.Translation(LID_SHOVE) @ Matrix.Rotation(math.radians(LID_TURN), 4, 'Z')


def lid():
    """The lid as it lies, prised askew with its front right corner smashed off: the two granite slabs with square
    breaks, what is left of the gilt rim, the ridge, the empty setting (claws prised open, one snapped, one gone;
    teeth of its glass in the rim), shards and a crack across the top."""
    low_rect, up_rect = (-LID_X, -LID_Y, LID_X, LID_Y), (-UP_X, -UP_Y, UP_X, UP_Y)
    outline = [(-LID_X, -LID_Y)] + BREAK + [(LID_X, LID_Y), (-LID_X, LID_Y)]
    top = H + LOW + UP
    parts = [part(prism(outline, H, H + LOW, chamfer=0.012, rect=low_rect), GRANITE, 11),
             part(prism(clip_rect(outline, up_rect), H + LOW, top, chamfer=0.025, rect=up_rect, bottom=False),
                  GRANITE, 12)]
    z, t = H + 0.035, 0.008
    parts += wrap(LID_X, LID_Y, z, 0.03, t, GILT, skip=('front', 'right'))
    parts.append(block((0.212 + LID_X + t, t, 0.03), ((0.212 - LID_X - t) * 0.5, -(LID_Y + t * 0.5), z), GILT))
    parts.append(block((t, LID_Y + 0.032, 0.03), (LID_X + t * 0.5, (LID_Y - 0.032) * 0.5, z), GILT))
    parts.append(block((SX - 0.4, 0.05, 0.035), (0.0, 0.0, top + 0.0175), GILT, bevel=0.008))
    # The setting: a gilt boss on the slab, the ridge running into it, its seat (dark) empty.
    boss, bands = lp.lathe([(0.047, 0.0), (0.047, 0.046), (0.041, 0.052), (0.034, 0.049), (0.033, 0.038),
                            (0.0, 0.038)], segments=12)
    lt.assign(boss, GILT)
    lt.assign(boss, RUNE, [i for i, b in enumerate(bands) if b == 4])
    lt.box_uv(boss, 'MetalWorn', faces=GILT)
    lt.box_uv(boss, 'RockGranite', faces=RUNE)
    parts.append(lp.place(boss, (0.0, 0.0, top)))
    for k, (height, bend) in enumerate(((0.026, 35.0), (0.026, 72.0), (0.008, 12.0))):
        a = math.radians(45.0 + 90.0 * k)
        claw = block((0.006, 0.011, height), (0.0, 0.0, height * 0.5), GILT, 40 + k)
        lp.place(claw, (0.0, 0.0, 0.0), (0.0, bend, 0.0))
        parts.append(lp.place(claw, (math.cos(a) * 0.04, math.sin(a) * 0.04, top + 0.049), (0.0, 0.0, math.degrees(a))))
    # Teeth of the glass that covered the ember, still standing in the rim; shards on the slab.
    for k, (deg, pts, lean) in enumerate(((20.0, [(-0.01, 0.0), (0.008, 0.0), (0.003, 0.026)], 12.0),
                                          (150.0, [(-0.007, 0.0), (0.011, 0.0), (-0.004, 0.018)], 20.0),
                                          (262.0, [(-0.009, 0.0), (0.006, 0.0), (0.0, 0.021)], 8.0))):
        a = math.radians(deg)
        parts.append(shard(pts, 0.0025, (math.cos(a) * 0.04, math.sin(a) * 0.04, top + 0.047), (90.0 + lean, 0.0, deg + 90.0),
                           seed=50 + k))
    parts.append(shard([(0.0, 0.0), (0.032, 0.006), (0.011, 0.022)], 0.002, (-0.12, -0.11, top + 0.001), (0.0, 0.0, 30.0), 54))
    parts.append(shard([(0.0, 0.0), (0.022, -0.004), (0.016, 0.017)], 0.002, (0.1, 0.09, top + 0.001), (0.0, 0.0, 115.0), 55))
    # The crack: from the break across the top, in front of the setting, and down over the step to the front.
    parts.append(crack([(0.33, -0.10, top), (0.22, -0.074, top), (0.13, -0.092, top), (0.05, -0.062, top),
                        (-0.04, -0.076, top), (-0.13, -0.112, top), (-0.22, -0.128, top), (-0.30, -0.176, top),
                        (-0.355, -0.198, top)], (0.0, 0.0, 1.0), seed=56))
    parts.append(crack([(-0.372, -0.236, H + LOW), (-0.39, -0.285, H + LOW), (-0.418, -0.333, H + LOW)],
                       (0.0, 0.0, 1.0), seed=57))
    moved(parts, lid_matrix())
    return parts, outline


def debris():
    """The smashed corner's bits (and the bite's) on the plinth's steps, beside it and in the chest; a torn piece of
    the lid's rim; shards. Each lies on its own surface within the chapel plinth's top."""
    parts = [fragment((0.095, 0.15, 0.065), (0.72, -0.17, 0.0), 4.0, 61),        # beside the plinth, right
             fragment((0.05, 0.035, 0.03), (0.36, -0.40, 0.13), 20.0, 62),        # the lower step, front
             fragment((0.045, 0.035, 0.028), (0.12, -0.343, BASE), -15.0, 63),    # the upper step, front
             fragment((0.05, 0.04, 0.03), (0.52, -0.475, 0.0), 35.0, 64),         # the chapel's plinth, front
             fragment((0.045, 0.04, 0.03), (-0.72, -0.43, 0.0), 10.0, 65),        # left, from the bite
             fragment((0.04, 0.03, 0.025), (-0.30, -0.345, BASE), 40.0, 66),      # under the bite
             fragment((0.08, 0.06, 0.04), (0.28, -0.10, BASE + 0.07), 25.0, 67),  # in the chest
             fragment((0.06, 0.05, 0.035), (0.36, -0.17, BASE + 0.07), -30.0, 68)]
    parts.append(strap([(0.705, 0.0, 0.004), (0.73, 0.06, 0.004), (0.718, 0.12, 0.004)], 0.008, 0.03, GILT, 69))
    parts.append(strap([(0.17, -0.2, BASE + 0.074), (0.23, -0.185, BASE + 0.074), (0.27, -0.205, BASE + 0.074)],
                       0.008, 0.03, GILT, 70))
    parts.append(shard([(0.0, 0.0), (0.03, 0.004), (0.012, 0.02)], 0.002, (0.05, -0.405, 0.131), (0.0, 0.0, 25.0), 71))
    parts.append(shard([(0.0, 0.0), (0.026, -0.006), (0.02, 0.015)], 0.002, (-0.15, -0.478, 0.001), (0.0, 0.0, 140.0), 72))
    parts.append(shard([(0.0, 0.0), (0.024, 0.008), (0.004, 0.019)], 0.002, (0.33, -0.05, BASE + 0.071), (0.0, 0.0, 70.0), 73))
    return parts


# --- Occlusion: the chapel round SOCKET_Reliquary ---
# Chapel.py's clapboard chapel (2026-10-07), in its own metres: x across the nave, y toward the apse, z up from the
# ground; the socket is at SOCKET_AT. Walls, floors and roofs as boxes, the smashed windows, the apse's window and arch
# left open: a stand-in for the 7 m interior pass Chapel.py bakes into the chapel itself.
SOCKET_AT = (0.0, 6.33, 1.21)


def wall_pieces(lo, hi, z0, z1, holes):
    """A wall's boxes along it (a0, a1, z0, z1) from lo to hi, round holes (a0, a1, z0, z1)."""
    pieces, a = [], lo
    for a0, a1, h0, h1 in sorted(holes):
        if a0 > a:
            pieces.append((a, a0, z0, z1))
        pieces += [(a0, a1, z0, h0), (a0, a1, h1, z1)]
        a = a1
    if hi > a:
        pieces.append((a, hi, z0, z1))
    return pieces


def apse_stand_in():
    boxes = [(-3.2, 3.2, -4.0, 3.5, 0.4, 0.43), (-3.2, 3.2, 3.5, 5.6, 0.4, 0.79), (-1.2, 1.2, 5.6, 7.2, 0.4, 0.79),
             (-0.68, 0.68, 5.9, 6.76, 0.79, 1.11), (-0.78, 0.78, 5.82, 6.84, 1.11, 1.2)]
    nave_windows = {-1.0: [4.065, 1.665, -0.735, -3.135], 1.0: [-3.135, -0.735, 1.665]}
    for side, starts in nave_windows.items():
        x0, x1 = sorted((side * 3.04, side * 3.2))
        for a0, a1, z0, z1 in wall_pieces(-4.0, 5.6, 0.4, 4.8, [(y, y + 0.67, 1.75, 4.0) for y in starts]):
            boxes.append((x0, x1, a0, a1, z0, z1))
    for a0, a1, z0, z1 in wall_pieces(-3.2, 3.2, 0.4, 4.8, [(-0.735, 0.735, 0.76, 3.5)]):
        boxes.append((a0, a1, 5.44, 5.6, z0, z1))
    boxes += [(-3.2, 3.2, 5.44, 5.6, 4.8, 6.0), (-2.0, 2.0, 5.44, 5.6, 6.0, 7.4), (-0.9, 0.9, 5.44, 5.6, 7.4, 8.6)]
    for side in (-1.0, 1.0):
        x0, x1 = sorted((side * 1.04, side * 1.2))
        boxes.append((x0, x1, 5.6, 7.2, 0.4, 4.3))
    for a0, a1, z0, z1 in wall_pieces(-1.2, 1.2, 0.4, 3.6, [(-0.335, 0.335, 1.95, 3.0)]):
        boxes.append((a0, a1, 7.04, 7.2, z0, z1))
    bm = bmesh.new()
    sx, sy, sz = SOCKET_AT
    for x0, x1, y0, y1, z0, z1 in boxes:
        bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.LocRotScale(
            Vector(((x0 + x1) * 0.5 - sx, (y0 + y1) * 0.5 - sy, (z0 + z1) * 0.5 - sz)), None,
            Vector((x1 - x0, y1 - y0, z1 - z0))))
    # The roofs: the nave's two slopes (eaves 4.8 m, ridge 8.61 m) and the apse's lean-to (4.3 m down to 3.6 m).
    for slab in ([(3.2, 4.8), (0.0, 8.61)], [(-3.2, 4.8), (0.0, 8.61)]):
        points = [(x, y, z + dz) for (x, z) in slab for y in (-4.0, 5.6) for dz in (0.0, 0.15)]
        _hull_into(bm, [(x - sx, y - sy, z - sz) for x, y, z in points])
    _hull_into(bm, [(x - sx, y - sy, z + dz - sz) for x in (-1.2, 1.2) for y, z in ((5.6, 4.3), (7.2, 3.6))
                    for dz in (0.0, 0.15)])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return lp.mesh_object(bm, 'ApseStandIn')


def _hull_into(bm, points):
    verts = [bm.verts.new(p) for p in points]
    hull = bmesh.ops.convex_hull(bm, input=verts)
    inside = [v for v in hull['geom_interior'] + hull['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(dict.fromkeys(inside)), context='VERTS')


def soot(obj, weight, strength):
    """Lowers the baked occlusion (vertex color alpha) of whole faces by strength x weight(face centre, normal): M_World
    darkens the base color with some of it and the ambient light with all of it."""
    import numpy as np
    mesh = obj.data
    raw = lt._read_col(mesh)
    for poly in mesh.polygons:
        w = weight(poly.center, poly.normal)
        if w > 0.0:
            for li in poly.loop_indices:
                raw[li, 3] *= 1.0 - strength * min(w, 1.0)
    lt._write_col(mesh, np.clip(raw, 0.0, 1.0))


def bake(obj, sooty=True):
    """The contact pass, then the interior pass against the chapel stand-in: near x (0.3 + 0.7 x far), as Chapel.py
    does for the chapel's own inside. Then soot in the chest and under the lid."""
    lp.finish(obj, ao=0.3, nanite=False)
    near = lt._read_col(obj.data)[:, 3].copy()
    room = apse_stand_in()
    room.parent = obj
    lt.bake_vertex_ao(obj, samples=40, distance=7.0, ground=False, children=False)
    raw = lt._read_col(obj.data)
    raw[:, 3] = near * (0.3 + 0.7 * raw[:, 3])
    lt._write_col(obj.data, raw)
    mesh = room.data
    bpy.data.objects.remove(room)
    bpy.data.meshes.remove(mesh)
    if sooty:
        # Faces in the chest (the walls' inner faces run on into the walls they meet).
        inside = lambda p, n: 1.0 if abs(p.x) < (OX if abs(n.y) > 0.9 else OX - WALL) + 0.004 \
            and abs(p.y) < (OY if abs(n.x) > 0.9 else OY - WALL) + 0.004 and BASE < p.z < H + 0.004 else 0.0
        under = lambda p, n: 1.0 if n.z < -0.9 and H - 0.01 < p.z < H + 0.01 else 0.0
        soot(obj, lambda p, n: max(inside(p, n), under(p, n)), 0.55)
    obj['LODs'] = '50,25'
    return obj


def build():
    lid_parts, outline = lid()
    # SM_Reliquary_Smashed: the loot chest keeps SM_Reliquary for when it comes into the game.
    model = lp.join('Reliquary_Smashed', plinth() + chest() + pillars() + bands() + rune() + lid_parts + debris())
    m = lid_matrix()
    lp.hull_box(model, (1.32, 0.88, 0.2), (0.0, 0.0, 0.1))
    lp.hull_box(model, (SX + 0.04, SY + 0.04, SZ), (0.0, 0.0, BASE + SZ * 0.5))
    lp.hull_points(model, [m @ Vector((x, y, z)) for x, y in outline for z in (H, H + LOW + UP)])
    lm.socket(model, 'Ember', m @ Vector((0.0, 0.0, SEAT)), (0.0, 0.0, LID_TURN))
    lm.socket(model, 'Interact', (0.0, FY - 0.016, CZ))
    bake(model)
    lo, hi = lt._bounds([model])
    lt._log(f'Reliquary_Smashed: {lp.tri_count(model)} triangles, {len(model.data.vertices)} vertices, '
            f'{(hi - lo).x:.3f} x {(hi - lo).y:.3f} x {(hi - lo).z:.3f} m (x {lo.x:.3f}..{hi.x:.3f}, '
            f'y {lo.y:.3f}..{hi.y:.3f}, z {lo.z:.3f}..{hi.z:.3f}), materials '
            f"{', '.join(s.material.name for s in model.material_slots)}")
    return model


# --- Previews ---
PREVIEWS = 'RansomsRest/Reliquary'
CHAPEL_EXPORT = os.path.join(lt.REPO, 'Intermediate', 'ArtExport_RR_Chapel')
# Toward the sun at Ransom's Rest's golden hour (azimuth 247.5 degrees, 15 up: west-southwest) in the chapel's frame
# (it faces south: x east, y north).
SUN = Vector((math.sin(math.radians(247.5)) * math.cos(math.radians(15.0)),
              math.cos(math.radians(247.5)) * math.cos(math.radians(15.0)), math.sin(math.radians(15.0))))


def intact():
    """The backlog Reliquary as Chests.py builds it, closed, its crystal floating: for the before and after only."""
    gold = lt.material('MetalWorn', name='ChestGold', tint=0xffc04a)
    TILEABLE[gold.name] = 'MetalWorn'
    glow = lm.material('ChestGlowLegendary', 0xffa62e, Glow=4.0, Variation=0.0)
    bsdf = next(n for n in glow.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Emission Color'].default_value = lt.hex_color(0xffa62e)
    bsdf.inputs['Emission Strength'].default_value = 4.0   # its Glow in Unreal
    rnd = random.Random(53)
    parts = [block((1.32, 0.88, 0.13), (0.0, 0.0, 0.065), GRANITE, 1, bevel=0.02),
             block((1.2, 0.76, 0.07), (0.0, 0.0, 0.165), GRANITE, 2, bevel=0.015)]
    parts += wrap(0.6, 0.38, 0.19, 0.02, 0.008, gold, bevel=0.002)
    for s in (-1.0, 1.0):
        parts.append(block((SX, WALL, SZ), (0.0, s * (SY - WALL) * 0.5, BASE + SZ * 0.5), GRANITE, rnd.randint(0, 99), bevel=0.008))
        parts.append(block((WALL, SY - 2 * WALL, SZ), (s * (SX - WALL) * 0.5, 0.0, BASE + SZ * 0.5), GRANITE,
                           rnd.randint(0, 99), bevel=0.008))
    parts.append(block((SX - 2 * WALL, SY - 2 * WALL, 0.03), (0.0, 0.0, BASE + 0.055), gold, 8, bevel=0.003))
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            cx, cy = x * (OX + 0.005), y * (OY + 0.005)
            parts.append(disc(0.038, SZ - 0.08, (cx, cy, BASE + 0.04), (0.0, 0.0, 1.0), gold, segments=14))
            parts.append(block((0.11, 0.11, 0.045), (cx, cy, BASE + 0.0225), gold, bevel=0.006))
            parts.append(block((0.11, 0.11, 0.045), (cx, cy, H - 0.0225), gold, bevel=0.006))
    for z in (BASE + 0.035, H - 0.035):
        parts += wrap(OX, OY, z, 0.025, 0.008, gold, gap_x=0.1, gap_y=0.1, bevel=0.002)
    parts.append(block((0.2, 0.012, 0.2), (0.0, FY, CZ), gold, rotation=(0.0, 45.0, 0.0), bevel=0.004))
    parts.append(block((0.15, 0.006, 0.15), (0.0, FY - 0.009, CZ), RUNE, rotation=(0.0, 45.0, 0.0)))
    r = 0.07
    for xs in (-1.0, 1.0):
        for zs in (-1.0, 1.0):
            parts.append(block((r * 1.414 + 0.012, 0.004, 0.012), (xs * r * 0.5, FY - 0.013, CZ + zs * r * 0.5), glow,
                               rotation=(0.0, 45.0 * xs * zs, 0.0)))
    gems = [((0.018, 0.03, (0.0, FY - 0.014, CZ), (0.0, -1.0, 0.0), 4)), ((0.04, 0.03, (0.0, 0.0, H + 0.2), (0.0, 0.0, 1.0), 4)),
            ((0.075, 0.19, (0.0, 0.0, H + 0.55), (0.0, 0.0, 1.0), 6))]
    for radius, height, center, normal, sides in gems:
        gem = lp.lathe([(0.0, -height), (radius, 0.0), (0.0, height)], segments=sides)[0]
        parts.append(oriented(part(gem, glow), center, normal))
    for s in (-1.0, 1.0):
        parts.append(block((0.22, 0.004, 0.01), (s * 0.24, FY - 0.001, CZ), glow))
        parts.append(block((0.01, 0.004, 0.12), (s * 0.35, FY - 0.001, CZ), glow))
        parts.append(block((0.004, 0.16, 0.01), (s * (OX + 0.001), 0.0, CZ), glow))
    for x in (-0.3, 0.3):
        parts.append(disc(0.03, 0.14, (x - 0.07, OY + 0.045, H), (1.0, 0.0, 0.0), gold, segments=12))
    parts.append(block((SX + 0.07, SY + 0.07, 0.07), (0.0, 0.0, H + 0.035), GRANITE, 11, bevel=0.012))
    parts += wrap(OX + 0.035, OY + 0.035, H + 0.035, 0.03, 0.008, gold, bevel=0.002)
    parts.append(block((SX - 0.14, SY - 0.16, 0.09), (0.0, 0.0, H + 0.115), GRANITE, 12, bevel=0.025))
    parts.append(block((SX - 0.4, 0.05, 0.035), (0.0, 0.0, H + 0.1775), gold, bevel=0.008))
    model = lp.join('IntactReliquary', parts)
    return bake(model, sooty=False)


def render(objects, out_png, camera_at, look_at, lens, resolution=(1600, 1000), sun_strength=9.0, exposure=0.6,
           samples=64):
    """Renders objects from camera_at toward look_at in the chapel's late afternoon light: a warm low sun from SUN,
    lt.preview's sky (the chapel's walls and its baked occlusion keep it dim inside), a ground outside. Returns the
    PNG path."""
    scene = bpy.context.scene
    shown = lt._preview_objects(objects)
    out_png = os.path.normpath(os.path.join(lt.REPO, out_png) if not os.path.isabs(out_png) else out_png)
    os.makedirs(os.path.dirname(out_png), exist_ok=True)
    hidden = {o: o.hide_render for o in scene.objects}
    for o in scene.objects:
        o.hide_render = o not in shown
    camera = bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    camera.data.lens, camera.data.sensor_fit = lens, 'HORIZONTAL'
    camera.data.clip_start, camera.data.clip_end = 0.02, 2000.0
    camera.location = Vector(camera_at)
    camera.rotation_euler = (Vector(look_at) - Vector(camera_at)).to_track_quat('-Z', 'Y').to_euler()
    light = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    light.data.energy, light.data.color, light.data.angle = sun_strength, (1.0, 0.8, 0.58), math.radians(1.5)
    light.rotation_euler = (-SUN).to_track_quat('-Z', 'Y').to_euler()
    mesh = bpy.data.meshes.new('_Ground')
    mesh.from_pydata([(-500.0, -500.0, 0.0), (500.0, -500.0, 0.0), (500.0, 500.0, 0.0), (-500.0, 500.0, 0.0)], [],
                     [(0, 1, 2, 3)])
    ground_mat = bpy.data.materials.get('_PreviewGround') or bpy.data.materials.new('_PreviewGround')
    ground_mat.diffuse_color = lt.hex_color(0x8c7a58)
    ground_mat.use_nodes = True
    next(n for n in ground_mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED').inputs['Base Color'].default_value = \
        lt.hex_color(0x8c7a58)
    mesh.materials.append(ground_mat)
    ground = bpy.data.objects.new('_Ground', mesh)
    added = [camera, light, ground]
    for o in added:
        scene.collection.objects.link(o)
    world = lt._preview_world(scene)
    saved = (scene.camera, scene.world, scene.render.resolution_x, scene.render.resolution_y)
    try:
        scene.camera, scene.world = camera, world
        scene.render.resolution_x, scene.render.resolution_y = resolution
        scene.render.resolution_percentage = 100
        scene.render.film_transparent = False
        scene.render.image_settings.file_format, scene.render.image_settings.color_mode = 'PNG', 'RGB'
        scene.render.filepath = out_png
        scene.view_settings.view_transform = 'AgX'
        scene.view_settings.look = 'AgX - Medium High Contrast'
        scene.view_settings.exposure = exposure
        scene.render.engine = 'BLENDER_EEVEE_NEXT'
        scene.eevee.taa_render_samples = samples
        bpy.ops.render.render(write_still=True)
    finally:
        scene.camera, scene.world, scene.render.resolution_x, scene.render.resolution_y = saved
        scene.view_settings.exposure = 0.0
        bpy.data.worlds.remove(world)
        for o, store in zip(added, (bpy.data.cameras, bpy.data.lights, bpy.data.meshes)):
            data = o.data
            bpy.data.objects.remove(o)
            store.remove(data)
        for o, value in hidden.items():
            if o.name in scene.objects:
                o.hide_render = value
    lt._log(f'render: {out_png}')
    return out_png


def chapel():
    """SM_Chapel from the chapel's export test, its materials made again by name (the FBX brings names only), and its
    SOCKET_Reliquary from the export's manifest (Unreal centimetres back to Blender metres)."""
    path = os.path.join(CHAPEL_EXPORT, 'SM_Chapel.fbx')
    if not os.path.exists(path):
        lt._log(f'note: no {path}: run the chapel export test (-Family Chapel) for the views in the chapel')
        return None, None
    import looter_buildings as kit
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path, colors_type='LINEAR')
    found = [o for o in bpy.data.objects if o not in before]
    for o in found:
        if o.name.startswith('UCX_'):
            o.hide_render = True
    building = next(o for o in found if o.type == 'MESH' and o.name.startswith('SM_Chapel'))
    makers = {'HouseTrim': lambda: lt.material('HouseTrim'), 'WoodPlanks': lambda: lt.material('WoodPlanks'),
              'IronBlack': lambda: lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a),
              'LanternGlow': lambda: kit._material('glow')}
    for slot in building.material_slots:
        name = slot.material.name.split('.')[0]
        if name in makers:
            slot.material = makers[name]()
    at = Vector(SOCKET_AT)
    manifest = os.path.join(CHAPEL_EXPORT, 'Art_Models_Buildings_Chapel.json')
    if os.path.exists(manifest):
        with open(manifest, encoding='utf-8') as file:
            for model in json.load(file)['models']:
                for s in model.get('sockets', []):
                    if s['name'] == 'Reliquary':
                        u = s['location']
                        at = Vector((-u[1], -u[0], u[2])) * 0.01
    return building, at


def marker(at, up, front):
    """A cyan arrow along a socket's +Z and a short white one along its front (-Y), for the close view only."""
    mat = bpy.data.materials.get('SocketMarker') or bpy.data.materials.new('SocketMarker')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = (0.05, 0.9, 1.0, 1.0)
    bsdf.inputs['Emission Color'].default_value = (0.05, 0.9, 1.0, 1.0)
    bsdf.inputs['Emission Strength'].default_value = 6.0
    white = bpy.data.materials.get('SocketMarkerFront') or bpy.data.materials.new('SocketMarkerFront')
    white.use_nodes = True
    wb = next(n for n in white.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    wb.inputs['Emission Color'].default_value = (1.0, 1.0, 1.0, 1.0)
    wb.inputs['Emission Strength'].default_value = 3.0
    pieces = []
    for direction, length, m in ((up, 0.16, mat), (front, 0.06, white)):
        shaft = lp.lathe([(0.0, 0.0), (0.0025, 0.0), (0.0025, length), (0.008, length), (0.0, length + 0.022)],
                         segments=10)[0]
        lt.assign(shaft, m)
        pieces.append(oriented(shaft, at, direction))
    dot = lp.lathe([(0.0, -0.006), (0.006, 0.0), (0.0, 0.006)], segments=10)[0]
    lt.assign(dot, mat)
    pieces.append(lp.place(dot, at))
    return lp.join('EmberMarker', pieces)


def side_by_side(left, right, out_png):
    """Puts two renders of the same size side by side with a dark rule between them."""
    import numpy as np
    images = [bpy.data.images.load(p) for p in (left, right)]
    w, h = images[0].size
    pixels = [np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4) for img in images]
    rule = np.zeros((h, 6, 4), dtype=np.float32)
    rule[..., :3], rule[..., 3] = 0.06, 1.0
    both = np.concatenate([pixels[0], rule, pixels[1]], axis=1)
    out = bpy.data.images.new('_BeforeAfter', both.shape[1], h)
    out.pixels[:] = both.ravel()
    out.filepath_raw, out.file_format = out_png, 'PNG'
    out.save()
    for img in images + [out]:
        bpy.data.images.remove(img)
    lt._log(f'render: {out_png}')


def previews(model):
    # The outdoor views show the shapes with the contact pass only (the chapel pass is for the chapel's light).
    baked = lt._read_col(model.data).copy()
    lt.bake_vertex_ao(model, distance=0.3)
    lt.preview([model], lt.preview_path(PREVIEWS, 'Reliquary'), view=(-0.85, -1.5, 0.62), fit=0.95)
    lt.preview([model], lt.preview_path(PREVIEWS, 'Reliquary_Back'), view=(1.2, 1.1, 0.8), fit=0.95)
    lt._write_col(model.data, baked)
    before = intact()
    before.hide_render = True
    building, at = chapel()
    if building is None:
        return
    for obj in (model, before):
        obj.location = at
    bpy.context.view_layer.update()
    scene = [building, model]
    eye = 0.43 + 0.36 + 1.55   # on the sanctuary floor
    # Standing in the aisle among the pews, halfway down the nave.
    render(scene, lt.preview_path(PREVIEWS, 'Reliquary_Nave'), (0.2, -0.3, 0.43 + 1.62), at + Vector((0.0, 0.0, 0.3)),
           lens=28.0)
    # From the sanctuary just in front of the apse's arch (its jambs frame the view), a little below standing eyes.
    three_quarter = dict(camera_at=at + Vector((-0.72, -1.58, eye - at.z - 0.25)), look_at=at + Vector((0.03, 0.0, 0.4)),
                         lens=34.0)
    render(scene, lt.preview_path(PREVIEWS, 'Reliquary_ThreeQuarter'), **three_quarter, resolution=(1300, 1000))
    ember = next(c for c in model.children if c.name.startswith('SOCKET_Ember'))
    m = ember.matrix_world
    flag = marker(m.translation, m.to_3x3() @ Vector((0.0, 0.0, 1.0)), m.to_3x3() @ Vector((0.0, -1.0, 0.0)))
    render(scene + [flag], lt.preview_path(PREVIEWS, 'Reliquary_Ember'), m.translation + Vector((-0.2, -0.42, 0.3)),
           m.translation + Vector((0.0, 0.0, 0.02)), lens=48.0, resolution=(1300, 1000))
    import tempfile
    halves = [os.path.join(tempfile.gettempdir(), f'reliquary_{k}.png') for k in ('before', 'after')]
    model.hide_render, before.hide_render = True, False
    render([building, before], halves[0], **three_quarter, resolution=(900, 1000))
    model.hide_render, before.hide_render = False, True
    render(scene, halves[1], **three_quarter, resolution=(900, 1000))
    side_by_side(halves[0], halves[1], lt.preview_path(PREVIEWS, 'Reliquary_BeforeAfter'))
    for p in halves:
        os.remove(p)


reliquary = build()
if lt.want_preview():
    previews(reliquary)
