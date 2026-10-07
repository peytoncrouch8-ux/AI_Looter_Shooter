"""The freestanding outcrop kit for Ransom's Rest, Den Rock with the Gravemother's den, and the far rocks: rocks of the
cliff kit's layered rock (RockCliff, moss on up-facing rock). A scripted model (see Art/README.md); built as solids with
Tools/Blender/looter_outcrops.py.

  piece            use (Docs/Areas/RansomsRest.md)
  Outcrop_TorA     squat and blocky, 4.5 m: the Rimrock, the Keeper's Gate's south rock, ridge feet; at 0.65 the
                   bluff-top rocks (3 m)
  Outcrop_TorB     tall and split, 9.8 m: the Keeper's Gate's north rock (at 0.7), Hearse Rock's head
  Outcrop_TorC     a leaning stack, 7 m: the Rimrock, Hearse Rock, ridge feet
  Outcrop_TorD     a broken slab pile, 4.6 m: slabs tipped like books against a stub: the Rimrock's gaps, the Tumble
  Outcrop_Spine    a 12 m rock spine, 5.8 m high: the Rimrock (one per piece), Hearse Rock, ridge feet
  Outcrop_Slab     a leaning slab, 7.5 m, 14 degrees off upright: the Three Widows (three, turned)
  Outcrop_Fin      a rock fin, 9 m, its crest stepping down to one end: the Stone Teeth (a row, turned and scaled)
  DenRock          the dome on the Sink's east rim, its face the Sink's wall with the den at its foot (see den_rock)
  FarRock_A..D     low-poly rocks (about 170 triangles) for the ridges past the boundary

Rocks are horizontal beds (thick massive beds and thin soft partings, the cliff kit's) split by vertical joints into
columns that stand out or back through every bed, worn round at the edges, more on top; tops step and break. Each kit
piece is finished on all sides, so any turn works, and reaches 0.9 m below its pivot (on the ground, in the middle of
the footprint) to seat on a slope. Nanite sources of 7-11k triangles with a Fallback bringing each to 2-3k. Collision:
convex column hulls whose sides bridge the beds' ledges, so the only faces a player could stand on are the tops, out of
reach (a jump reaches about 1 m); fallen blocks big enough to matter get a hull of their own.

Args after '--' naming builders (TorA, Spine, DenRock, FarRocks, ...) build only those, for quick looks.
"""
import math
import os
import random
import sys

import numpy as np

import looter_outcrops as lo
import looter_textures as lt

SINK = 0.9
ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []


# --- Building blocks of a layered rock ---

def joints(rnd, angle, spacing, extent, wander=4.0, start=0.7):
    """A set of parallel vertical joints across a plan of half size `extent`: (normal, offset) pairs."""
    lines = []
    c = -extent + rnd.uniform(*spacing) * start
    while c < extent - 0.6:
        a = math.radians(angle + rnd.uniform(-wander, wander))
        lines.append(((math.cos(a), math.sin(a)), c))
        c += rnd.uniform(*spacing)
    return lines


def centroid(poly):
    return sum(p[0] for p in poly) / len(poly), sum(p[1] for p in poly) / len(poly)


class Columns:
    """The columns between a rock's joints: each keeps its own standing-out (or back), top step and lean through every
    bed it crosses, as the cliff kit's columns do."""

    def __init__(self, rnd, lines, stand=0.3, step=(0.0, 0.0)):
        self.rnd, self.lines, self.stand, self.step = rnd, lines, stand, step
        self.table = {}

    def get(self, poly):
        cx, cy = centroid(poly)
        key = tuple(n[0] * cx + n[1] * cy > c for n, c in self.lines)
        if key not in self.table:
            rnd = self.rnd
            roll = rnd.random()
            out = (rnd.uniform(-self.stand, -self.stand * 0.4) if roll < 0.3 else
                   rnd.uniform(self.stand * 0.3, self.stand) if roll < 0.55 else rnd.uniform(-0.25, 0.25) * self.stand)
            self.table[key] = dict(out=out, step=rnd.uniform(*self.step),
                                   tilt=(rnd.uniform(-2.5, 2.5), rnd.uniform(-2.5, 2.5)))
        return self.table[key]


def min_width(poly):
    """The narrowest extent of a convex polygon across any of its sides."""
    widths = []
    for nx, ny, _ in lo.plan_planes(poly):
        s = [nx * x + ny * y for x, y in poly]
        widths.append(max(s) - min(s))
    return min(widths) if widths else 0.0


def facets(rnd, count, height):
    """Random shaving planes for a block (local normal, depth): side faces leaning in or out, tops tipped."""
    out = []
    for _ in range(count):
        az = rnd.uniform(0.0, 2.0 * math.pi)
        if rnd.random() < 0.6:
            out.append(((math.cos(az), math.sin(az), rnd.uniform(-0.45, 0.35)), rnd.uniform(0.08, 0.32)))
        else:
            slope = rnd.uniform(0.15, 0.45)
            out.append(((slope * math.cos(az), slope * math.sin(az), 1.0), rnd.uniform(0.1, min(0.4, height * 0.3))))
    return out


def on_line(a, b, lines, tolerance=1e-4):
    """True when the side a-b lies on one of the lines (n, c): n . p = c."""
    for n, c in lines:
        if abs(n[0] * a[0] + n[1] * a[1] - c) < tolerance and abs(n[0] * b[0] + n[1] * b[1] - c) < tolerance:
            return True
    return False


def bed(rock, rnd, plan, z0, z1, columns, cuts=(), gap=0.18, top=(0.0, 0.0), drop=0.0, chamfers=0.5, r_side=0.1,
        r_top=0.3, r_bottom=0.08, taper=0.0, blend=0.22, overlap=0.2, tilt=1.0, keep=None, skip=(), step=False,
        shave=(0, 1, 1, 2, 2, 3), narrow=0.8, fixed=(), lean=(0.0, 0.0)):
    """One bed of a stack: its plan cut into blocks by the joints in cuts (open by gap; a joint that would leave a
    sliver narrower than `narrow` dies out there instead). Each block's outer sides move out or back with its column
    (its joint sides, and sides on the `fixed` lines (n, c), stay put, so joints keep their width); its top is raised
    or lowered within top (plus its column's step: only ever for the top bed, step=True, so nothing is left hanging);
    a few facets shave its faces (counts drawn from shave); drop is the chance a block has fallen out. Blocks reach
    overlap down into the bed below and lean with their column (tilt scales it) plus lean (degrees about X, Y), so
    beds fuse; partings go inside beds (rock.parting), never at their boundaries. Returns the blocks."""
    cells = [lo.ccw(plan)]
    lines = list(fixed)
    for n, c in cuts:
        parts = []
        for poly in cells:
            pieces = lo.split(poly, n, c, gap)
            good = [p for p in pieces if len(p) >= 3 and abs(lo.area2d(p)) > 0.3 and min_width(p) > narrow]
            empty = [p for p in pieces if len(p) < 3 or abs(lo.area2d(p)) < 1e-3]
            if len(good) == 2 or (len(good) == 1 and len(empty) == 1):
                parts.extend(lo.ccw(p) for p in good)
            else:
                parts.append(poly)
        cells = parts
        lines += [(n, c - gap * 0.5), ((-n[0], -n[1]), -c - gap * 0.5)]
    blocks = []
    for i, poly in enumerate(cells):
        if i in skip or (keep is not None and i not in keep) or (keep is None and rnd.random() < drop):
            continue
        column = columns.get(poly)
        out = column['out'] + rnd.uniform(-0.06, 0.06)
        moves = [0.0 if on_line(a, b, lines) else out for a, b in zip(poly, poly[1:] + poly[:1])]
        poly = lo.chamfer(lo.offset_edges(poly, moves), rnd, chamfers, (0.1, 0.4))
        if len(poly) < 3 or abs(lo.area2d(poly)) < 0.2:
            continue
        cx, cy = centroid(poly)
        local = [(x - cx, y - cy) for x, y in poly]
        tip = (column['tilt'][0] * tilt + rnd.uniform(-0.8, 0.8) + lean[0],
               column['tilt'][1] * tilt + rnd.uniform(-0.8, 0.8) + lean[1])
        bottom = z0 - (overlap if z0 > -rock.sink + 0.05 else 0.0)
        height_top = z1 + (column['step'] if step else 0.0) + rnd.uniform(*top)
        cut = facets(rnd, rnd.choice(shave), height_top - bottom)
        blocks.append(rock.add(lo.Block(local, bottom, height_top, center=(cx, cy), tilt=tip, r_side=r_side,
                                        r_top=r_top * rnd.uniform(0.75, 1.25), r_bottom=r_bottom, taper=taper,
                                        blend=blend, facets=cut)))
    return blocks


def foot(rock, x, y, angle, z=0.35, reach=14.0):
    """How far from (x, y) along angle (degrees) the rock's outer side stands at height z (0 if there is none): found
    coming in from outside, so an open joint at (x, y) doesn't stop it."""
    a = math.radians(angle)
    t = np.arange(0.0, reach, 0.04)
    points = np.stack([x + t * math.cos(a), y + t * math.sin(a), np.full_like(t, z)], axis=1)
    inside = np.nonzero(rock.field(points) < 0.0)[0]
    return float(t[inside[-1]]) + 0.04 if len(inside) else 0.0


def corners(block):
    """A block's corner points in the rock's space (for a hull of its own)."""
    h = block.z1 - block.z0
    return np.array([(px, py, pz) for px, py in block.poly for pz in (0.0, h)]) @ block.matrix.T + block.origin


def talus(rock, rnd, spots, center=(0.0, 0.0)):
    """Blocks fallen at the foot: (angle, size) each, leaning on the side, half buried, tipped. Returns the big ones'
    hull points (the player steps over the small ones)."""
    hulls = []
    for angle, size in spots:
        r = foot(rock, center[0], center[1], angle)
        a = math.radians(angle)
        x, y = center[0] + (r + size * 0.18) * math.cos(a), center[1] + (r + size * 0.18) * math.sin(a)
        poly = lo.chamfer(lo.rect(size * 0.5, size * rnd.uniform(0.36, 0.46)), rnd, 0.8, (0.06, size * 0.2))
        h = size * rnd.uniform(0.55, 0.75)
        block = rock.add(lo.Block(poly, -h * 0.4, h * 0.6, center=(x, y), yaw=rnd.uniform(0.0, 360.0),
                                  tilt=(rnd.uniform(-25.0, 25.0), rnd.uniform(-25.0, 25.0)), r_side=size * 0.12,
                                  r_top=size * 0.16, r_bottom=size * 0.08, blend=0.1))
        if size >= 0.9:
            hulls.append(corners(block))
    return hulls


def chip(rock, rnd, center, angle, z, size):
    """An edge broken off: a tipped block carved out where the side stands at height z along angle from center."""
    r = foot(rock, center[0], center[1], angle, z)
    if r < 0.3:
        lo.log(f'chip at {angle} degrees, {z} m: no rock there')
        return
    a = math.radians(angle)
    x, y = center[0] + (r + size * 0.12) * math.cos(a), center[1] + (r + size * 0.12) * math.sin(a)
    rock.cut(lo.Block(lo.rect(size * 0.55, size * 0.45), z - size * 0.45, z + size * 0.45, center=(x, y),
                      yaw=angle + rnd.uniform(-35.0, 35.0), tilt=(rnd.uniform(-35.0, 35.0), rnd.uniform(-35.0, 35.0)),
                      r_side=0.06, r_top=0.06, r_bottom=0.06, blend=0.08))


def face_crack(rock, center, angle, z, slope, length=3.0, width=0.13, depth=0.2, shift=0.08):
    """A fracture across the side facing angle (degrees) at height z, running at slope degrees from level; the rock
    below it slumped back by shift."""
    r = foot(rock, center[0], center[1], angle, z)
    if r < 0.3:
        lo.log(f'crack at {angle} degrees, {z} m: no rock there')
        return
    a, s = math.radians(angle), math.radians(slope)
    out = np.array([math.cos(a), math.sin(a), 0.0])
    along = np.array([-math.sin(a) * math.cos(s), math.cos(a) * math.cos(s), math.sin(s)])
    normal = np.cross(out, along)
    point = np.array([center[0], center[1], z]) + out * (r - 0.1)
    rock.crack(point, normal, width=width, depth=depth, radius=length * 0.5, shift=shift)


def frames_of(blocks, rnd):
    """For per-part mapping: each face takes the frame of the block it is nearest (strata follow a tilted slab)."""
    shifts = [(rnd.uniform(0.0, 5.0), rnd.uniform(0.0, 5.0)) for _ in blocks]

    def frames(center):
        d = [b.dist(center[None, :])[0] for b in blocks]
        i = int(np.argmin(d))
        return blocks[i].matrix, blocks[i].origin, shifts[i]
    return frames


# --- The kit ---

def tor_a():
    """Squat and blocky, 4.6 m: a wide base bed, a middle bed set in, a thin soft bed and a cap broken into blocks of
    different heights, one of them fallen to the foot."""
    rnd = random.Random(4101)
    rock = lo.Rock(seed=4101, sink=SINK, lumps=(0.22, 3.2), detail=(0.07, 1.1))
    plan = lo.chamfer(lo.rect(3.8, 3.05), rnd, 0.9, (0.5, 1.3))
    set_a = joints(rnd, 16.0, (1.6, 3.0), 3.8)
    set_b = joints(rnd, 104.0, (1.9, 3.2), 3.2)
    columns = Columns(rnd, set_a + set_b, stand=0.4, step=(-0.6, 0.25))
    bed(rock, rnd, plan, -SINK, 1.55, columns, cuts=set_a[1:3] + set_b[:1], top=(0.0, 0.35), taper=0.03, tilt=0.3,
        r_top=0.24)
    middle = lo.offset(lo.transform2d(plan, 4.0, (0.4, -0.25)), -0.2)
    bed(rock, rnd, middle, 1.55, 2.95, columns, cuts=set_a + set_b[:1], top=(0.0, 0.2), taper=-0.03)
    rock.parting(1.95, depth=0.3, width=0.12)
    thin = lo.offset(lo.transform2d(plan, 2.0, (0.15, 0.0)), -0.6)
    bed(rock, rnd, thin, 2.95, 3.38, columns, cuts=set_a + set_b, r_top=0.12, r_bottom=0.08, blend=0.15, tilt=0.5,
        overlap=0.12)
    cap = lo.offset(lo.transform2d(plan, -3.0, (-0.3, 0.3)), -0.45)
    bed(rock, rnd, cap, 3.38, 4.45, columns, cuts=set_a + set_b, top=(-0.15, 0.15), drop=0.15, r_top=0.26,
        taper=-0.05, tilt=1.6, overlap=0.12, step=True)
    chip(rock, rnd, (0.0, 0.0), 300.0, 4.1, 0.9)
    chip(rock, rnd, (0.0, 0.0), 75.0, 2.7, 0.8)
    face_crack(rock, (0.0, 0.0), 250.0, 1.0, 38.0, length=2.6)
    face_crack(rock, (0.0, 0.0), 20.0, 2.3, -48.0, length=2.2)
    extra = talus(rock, rnd, [(205.0, 1.4), (226.0, 0.6), (38.0, 0.8)])
    obj = lo.build('Outcrop_TorA', rock, cell=0.07, source=9000, fallback=30, uv_seed=11)
    lo.column_hulls(obj, [(-1.9, -1.2), (1.6, -1.4), (-1.4, 1.5), (1.9, 1.3)], extra=extra)
    return obj


def tor_b():
    """Tall and split, 9.8 m: two pillars on a shared base, parted by an open joint that widens upward; the shorter
    pillar's top broken off."""
    rnd = random.Random(4202)
    rock = lo.Rock(seed=4202, sink=SINK, lumps=(0.2, 3.0), detail=(0.07, 1.1))
    plan = lo.chamfer(lo.rect(2.75, 2.2), rnd, 0.9, (0.35, 0.9))
    a = math.radians(7.0)
    n = (math.cos(a), math.sin(a))
    set_b = joints(rnd, 97.0, (1.5, 2.4), 2.2)
    columns = Columns(rnd, [(n, 0.05)] + set_b, stand=0.28, step=(-0.35, 0.1))
    bed(rock, rnd, plan, -SINK, 2.3, columns, cuts=set_b[:1], gap=0.16, top=(0.0, 0.3), taper=0.035, tilt=0.2,
        r_top=0.2)
    rock.parting(1.1, depth=0.2, width=0.1, fade=-0.2)
    levels = [(2.3, 4.4), (4.4, 6.4), (6.4, 8.2), (8.2, 9.8)]
    for k, (z0, z1) in enumerate(levels):
        gap = 0.18 + 0.08 * (z0 - 2.3)
        p = lo.offset(lo.transform2d(plan, rnd.uniform(-3.0, 3.0), (rnd.uniform(-0.12, 0.12), rnd.uniform(-0.1, 0.1))),
                      -0.1 * (k + 1))
        west, east = lo.split(p, n, 0.05, gap)
        part = 0.05 * (z0 - 2.3)            # the pillars part as they rise
        west = lo.transform2d(west, 0.0, (-n[0] * part, -n[1] * part))
        east = lo.transform2d(east, 0.0, (n[0] * part, n[1] * part))
        w_line = [(n, 0.05 - gap * 0.5 - part)]
        e_line = [((-n[0], -n[1]), -(0.05 + gap * 0.5 + part))]
        last = k == len(levels) - 1
        cuts = set_b[k % 2:k % 2 + 1]
        if not last:
            bed(rock, rnd, west, z0, z1 + (0.35 if k == 2 else 0.0), columns, cuts=cuts, gap=0.16, top=(0.0, 0.12),
                tilt=0.6, r_top=0.3 if k == 2 else 0.22, step=k == 2, fixed=w_line, lean=(0.0, -0.8 * k))
        bed(rock, rnd, east, z0, z1, columns, cuts=[] if last else cuts, gap=0.16, top=(0.0, 0.12), tilt=0.6,
            r_top=0.36 if last else 0.22, taper=0.04 if last else 0.0, step=last, fixed=e_line, lean=(0.0, 0.6 * k))
    rock.parting(5.3, depth=0.26, width=0.11)
    rock.parting(7.5, depth=0.2, width=0.1, fade=-0.1)
    chip(rock, rnd, (-1.3, 0.0), 160.0, 8.3, 1.1)
    chip(rock, rnd, (1.3, 0.0), 20.0, 6.0, 0.8)
    face_crack(rock, (0.0, 0.0), 265.0, 3.2, 42.0, length=2.4)
    face_crack(rock, (0.0, 0.0), 85.0, 5.0, -35.0, length=2.0)
    extra = talus(rock, rnd, [(100.0, 1.3), (118.0, 0.55), (282.0, 0.9), (300.0, 0.5)])
    obj = lo.build('Outcrop_TorB', rock, cell=0.07, source=10000, fallback=28, uv_seed=12)
    lo.column_hulls(obj, [(-1.35 * n[0], -1.35 * n[1]), (1.45 * n[0], 1.45 * n[1])], extra=extra)
    return obj


def tor_c():
    """A leaning stack, 7 m: thick slabs each set over a little further the same way and turned a little, the top one
    overhanging, worn round where they meet."""
    rnd = random.Random(4303)
    rock = lo.Rock(seed=4303, sink=SINK, lumps=(0.2, 3.0), detail=(0.07, 1.1))
    plan = lo.chamfer(lo.rect(2.6, 2.15), rnd, 0.9, (0.45, 1.0))
    lean = math.radians(165.0)
    set_b = joints(rnd, 112.0, (1.9, 2.8), 2.2)
    columns = Columns(rnd, set_b, stand=0.22, step=(-0.25, 0.15))
    levels = [(-SINK, 1.9, 0.0, 0.0, 0.0, 1.0), (1.9, 3.3, 0.62, 0.12, 9.0, 0.9),
              (3.3, 5.3, 1.32, 0.34, -4.0, 1.0), (5.3, 6.9, 2.1, 0.02, 8.0, 0.82)]
    for k, (z0, z1, shift, inset, yaw, size) in enumerate(levels):
        p = lo.offset(lo.transform2d(plan, yaw, (shift * math.cos(lean), shift * math.sin(lean)), size), -inset)
        tip = 2.6 * k                       # each slab tipped a little more the way the stack leans
        bed(rock, rnd, p, z0, z1, columns, cuts=set_b[:1] if k == 2 else [], gap=0.14, top=(0.0, 0.08), r_top=0.34,
            r_bottom=0.0 if k == 0 else 0.24, blend=0.1, overlap=0.05, tilt=0.8, taper=0.03 if k == 0 else -0.03,
            step=k == len(levels) - 1, lean=(-tip * math.sin(lean), tip * math.cos(lean)))
    rock.parting(0.85, depth=0.22, width=0.1, fade=-0.15)
    rock.parting(2.6, depth=0.2, width=0.09, fade=-0.1)
    rock.parting(4.4, depth=0.18, width=0.09, fade=-0.2)
    chip(rock, rnd, (0.0, 0.0), 340.0, 6.4, 0.9)
    face_crack(rock, (0.0, 0.0), 240.0, 0.9, 30.0, length=2.6)
    extra = talus(rock, rnd, [(165.0, 1.2), (190.0, 0.5), (330.0, 0.8), (60.0, 0.6)])
    obj = lo.build('Outcrop_TorC', rock, cell=0.07, source=9000, fallback=30, uv_seed=13)
    for z_range in ((-SINK, 3.8), (3.1, 5.6), (4.8, 7.6)):
        lo.column_hulls(obj, [(0.0, 0.0)], z_range=z_range)
    lo.column_hulls(obj, [], extra=extra)
    return obj


def tipped(rock, rnd, base, thickness, width, length, lean, yaw=0.0, cut=2):
    """A slab of the beds standing on its foot at base (x, y), leaning `lean` degrees from upright toward -X."""
    return rock.add(lo.Block(lo.chamfer(lo.rect(thickness * 0.5, width * 0.5), rnd, 0.9, (0.12, 0.4)), -SINK,
                             length - SINK, center=base, yaw=yaw, tilt=(rnd.uniform(-2.0, 2.0), -lean), r_side=0.1,
                             r_top=0.22, r_bottom=0.05, taper=0.02, blend=0.12, facets=facets(rnd, cut, length)))


def tor_d():
    """A broken slab pile, 4.7 m: slabs split off the beds that tipped over like books on a shelf, each leaning on
    the next against a stub that stayed standing, the last one at 36 degrees off upright, with chunks at the foot."""
    rnd = random.Random(4404)
    rock = lo.Rock(seed=4404, sink=SINK, lumps=(0.16, 2.6), detail=(0.06, 1.0), rain=0.02, warp=(0.22, 4.0))
    stop = rock.add(lo.Block(lo.chamfer(lo.rect(1.0, 1.65), rnd, 0.9, (0.25, 0.6)), -SINK, 3.2, center=(-2.75, 0.1),
                             yaw=8.0, r_side=0.12, r_top=0.3, r_bottom=0.05, taper=0.03, blend=0.12,
                             facets=facets(rnd, 2, 4.1)))
    slabs = [tipped(rock, rnd, (-1.3, 0.0), 1.0, 3.6, 5.6, 8.0, 4.0),
             tipped(rock, rnd, (0.6, 0.25), 0.95, 3.2, 4.9, 22.0, -5.0),
             tipped(rock, rnd, (2.3, -0.15), 0.9, 3.0, 4.3, 36.0, 7.0)]
    chip(rock, rnd, (-2.0, 0.0), 90.0, 4.0, 0.8)
    chip(rock, rnd, (0.0, 0.25), 270.0, 3.1, 0.7)
    face_crack(rock, (-2.75, 0.1), 270.0, 1.4, 30.0, length=2.0)
    extra = talus(rock, rnd, [(0.0, 1.0), (25.0, 0.55), (300.0, 0.7), (150.0, 0.6), (205.0, 0.8)])
    parts = [stop] + slabs
    frames = frames_of(parts + rock.blocks[len(parts):], rnd)
    obj = lo.build('Outcrop_TorD', rock, cell=0.065, source=9500, fallback=30, uv_seed=14, frames=frames)
    lo.block_hulls(obj, rock, [[block] for block in parts])
    lo.column_hulls(obj, [], extra=extra)
    return obj


def spine():
    """A 12 m rock spine: a long ridge of beds set in toward a stepped, broken crest, cut across by joints into
    columns of different heights, its ends stepping down."""
    rnd = random.Random(4505)
    rock = lo.Rock(seed=4505, sink=SINK, lumps=(0.24, 3.4), detail=(0.07, 1.15))
    base = lo.hull2d([(-6.2, -0.9), (-5.6, -1.9), (-2.5, -2.25), (2.0, -2.15), (5.4, -1.7), (6.2, -0.6), (6.0, 0.9),
                      (4.8, 1.95), (1.2, 2.2), (-3.0, 2.1), (-5.5, 1.5)])
    set_a = joints(rnd, 4.0, (1.5, 2.8), 6.2, wander=9.0, start=0.6)
    set_b = [((0.0, 1.0), 0.25)]
    columns = Columns(rnd, set_a + set_b, stand=0.38, step=(-1.1, 0.3))
    beds = [(-SINK, 1.7, 1.0, 0.0, set_a[1::2]), (1.7, 3.1, 0.94, 0.35, set_a),
            (3.1, 4.3, 0.85, 0.72, set_a + set_b), (4.3, 5.5, 0.73, 1.0, set_a + set_b)]
    for k, (z0, z1, length, inset, cuts) in enumerate(beds):
        p = [(x * length, y) for x, y in base]
        p = lo.offset(lo.transform2d(p, rnd.uniform(-2.0, 2.0), (rnd.uniform(-0.6, 0.6), rnd.uniform(-0.15, 0.15))),
                      -inset)
        last = k == len(beds) - 1
        bed(rock, rnd, p, z0, z1, columns, cuts=cuts, top=(-0.1, 0.1) if last else (0.0, 0.25),
            taper=0.03 if k == 0 else -0.02, tilt=0.3 if k == 0 else 1.0, r_top=0.3 if last else 0.24, step=last)
    rock.parting(0.8, depth=0.22, width=0.1, fade=-0.1)
    rock.parting(2.4, depth=0.32, width=0.12)
    rock.parting(3.75, depth=0.2, width=0.09, fade=-0.25)
    chip(rock, rnd, (1.0, 0.0), 95.0, 4.9, 1.0)
    face_crack(rock, (-1.0, 0.0), 270.0, 1.2, 35.0, length=3.0)
    face_crack(rock, (2.5, 0.0), 90.0, 2.0, -40.0, length=2.4)
    extra = talus(rock, rnd, [(250.0, 1.3), (262.0, 0.5), (75.0, 1.0), (98.0, 0.6), (180.0, 0.8)])
    obj = lo.build('Outcrop_Spine', rock, cell=0.075, source=11000, fallback=25, uv_seed=15)
    lo.column_hulls(obj, [(-4.6, 0.0), (-1.6, 0.0), (1.5, 0.0), (4.6, 0.0)], extra=extra)
    return obj


def slab():
    """A leaning slab for the Three Widows, 7 m: a buttress of the cliff's beds stood on end, narrowing to a bowed
    head, leaning 13 degrees back. Built upright and mapped, then leaned, so its beds lean with it."""
    rnd = random.Random(4606)
    rock = lo.Rock(seed=4606, sink=1.7, lumps=(0.18, 2.8), detail=(0.06, 1.0))
    plan = lo.chamfer(lo.rect(1.8, 0.82), rnd, 0.9, (0.2, 0.5))
    columns = Columns(rnd, [], stand=0.0)
    levels = [(-1.7, 1.9, 0.0, 0.0), (1.9, 3.6, -0.06, 0.1), (3.6, 5.2, 0.04, 0.16), (5.2, 6.5, -0.1, 0.24)]
    for k, (z0, z1, shift, inset) in enumerate(levels):
        p = lo.offset(lo.transform2d(plan, rnd.uniform(-2.0, 2.0), (shift, 0.0)), -inset)
        bed(rock, rnd, p, z0, z1, columns, top=(0.0, 0.15), taper=0.015, tilt=0.4, r_top=0.26, shave=(1, 2, 2, 3))
    head = lo.chamfer(lo.rect(1.05, 0.62, -0.28, 0.06), rnd, 1.0, (0.2, 0.35))
    rock.add(lo.Block(head, 6.3, 7.45, tilt=(0.0, 5.0), r_side=0.22, r_top=0.5, r_bottom=0.1, taper=0.06, blend=0.3))
    rock.parting(4.35, depth=0.2, width=0.1, fade=-0.15)
    rock.parting(1.1, depth=0.16, width=0.08, fade=-0.2)
    face_crack(rock, (0.0, 0.0), 270.0, 2.6, 34.0, length=2.4)
    face_crack(rock, (0.0, 0.0), 90.0, 4.8, -28.0, length=2.0)
    chip(rock, rnd, (0.0, 0.0), 0.0, 5.9, 0.7)
    lean = lo.Matrix.Rotation(math.radians(-14.0), 4, 'X')
    obj = lo.build('Outcrop_Slab', rock, cell=0.06, source=7000, fallback=30, uv_seed=16, matrix=lean)
    for z_range in ((-2.0, 3.0), (2.4, 5.4), (4.8, 7.6)):
        lo.column_hulls(obj, [(0.0, 0.0)], z_range=z_range)
    return obj


def fin():
    """A rock fin for the Stone Teeth, 9 m: a thin wall of beds, thick at the foot and thin at the top, its crest
    stepping down from its tall end, cut across by joints."""
    rnd = random.Random(4707)
    rock = lo.Rock(seed=4707, sink=SINK, lumps=(0.2, 3.0), detail=(0.065, 1.05))
    base = lo.hull2d([(-3.4, -0.5), (-2.9, -1.15), (0.0, -1.25), (2.9, -1.0), (3.4, -0.3), (3.3, 0.5), (2.6, 1.1),
                      (-0.4, 1.25), (-3.0, 1.0)])
    set_a = joints(rnd, 3.0, (1.3, 2.2), 3.4, wander=10.0, start=0.8)
    columns = Columns(rnd, set_a, stand=0.22, step=(-0.6, 0.15))
    levels = [(-SINK, 2.0, (-3.7, 3.7), 0.0), (2.0, 3.7, (-3.6, 3.5), 0.12), (3.7, 5.4, (-3.5, 3.0), 0.22),
              (5.4, 6.9, (-3.4, 2.2), 0.32), (6.9, 8.1, (-3.25, 1.1), 0.4), (8.1, 9.0, (-3.0, -0.2), 0.46)]
    for k, (z0, z1, (x0, x1), inset) in enumerate(levels):
        span = x1 - x0
        p = [(x0 + (x + 3.4) / 6.8 * span, y) for x, y in base]
        p = lo.offset(lo.transform2d(p, rnd.uniform(-1.5, 1.5), (0.0, rnd.uniform(-0.08, 0.08))), -inset)
        last = k == len(levels) - 1
        cuts = set_a[::2] if k in (0, 2) else set_a if k != 1 else set_a[1::2]
        bed(rock, rnd, p, z0, z1, columns, cuts=cuts, gap=0.16, top=(0.0, 0.18) if not last else (-0.1, 0.1),
            taper=0.02 if k == 0 else 0.0, tilt=0.3 if k == 0 else 0.8, r_top=0.32 if k >= 4 else 0.22,
            step=last)
    rock.parting(1.15, depth=0.2, width=0.09, fade=-0.1)
    rock.parting(4.55, depth=0.3, width=0.11)
    rock.parting(7.6, depth=0.18, width=0.08, fade=-0.2)
    chip(rock, rnd, (1.0, 0.0), 0.0, 5.1, 0.9)
    chip(rock, rnd, (-1.5, 0.0), 180.0, 8.4, 0.7)
    face_crack(rock, (0.0, 0.0), 270.0, 2.8, 40.0, length=2.6)
    face_crack(rock, (0.0, 0.0), 90.0, 5.8, -45.0, length=2.2)
    extra = talus(rock, rnd, [(280.0, 0.9), (300.0, 0.45), (95.0, 0.7), (10.0, 0.6)])
    obj = lo.build('Outcrop_Fin', rock, cell=0.065, source=8000, fallback=30, uv_seed=17)
    lo.column_hulls(obj, [(-2.4, 0.0), (0.0, 0.0), (2.4, 0.0)], extra=extra)
    return obj


# --- Den Rock and the Gravemother's den ---

SINK_R = 18.0                     # the Sink's radius at the rim (Docs/Areas/RansomsRest.plan.json)
DEN_FLOOR = (-11.95, -11.72)      # the den's floor at its mouth and at its back: it rises gently, so it drains out
DEN_BACK = 8.3                    # how far behind the lip the den's floor rises (its pocket ends about 9 m back)
DEN_CLEAR = 3.0                   # rubble keeps out of this either side of the den's line: a clear way 6 m wide
# The den's cave as rounded lobes melted into one: x, y, the centre's height over the floor, radii in x, y and z. The
# mouth (an arch whose left shoulder stands highest), the chamber (bulging left in its middle, its roof coming down
# toward the back) and the pocket, wider again where she rests. Sized for the Gravemother (the spider at 1.8x: legs
# spanning 6.4 m, 2 m high), who walks in and out: at least 7 m clear inside the collision from the mouth to her place,
# at her knees.
DEN_LOBES = ((0.0, -1.2, 0.3, 4.25, 2.6, 4.6),
             (-1.6, -0.4, 2.4, 2.3, 2.2, 2.6),
             (0.2, 1.7, 0.6, 4.1, 2.9, 4.1),
             (-0.2, 4.3, 0.5, 3.85, 2.6, 3.8),
             (0.2, 6.1, 0.3, 4.1, 2.2, 3.5),
             (0.0, 7.0, 0.2, 4.4, 2.3, 3.3))
DEN_BOX = (np.array([-8.0, -5.0, -13.0]), np.array([8.0, 11.6, -4.0]))    # everything the den changes lies in here


def polar(theta, r):
    """A plan point at angle theta (radians from the den's line, positive toward +X) and radius r from the Sink's
    centre, which is at (0, -SINK_R): the lip above the den's mouth is the pivot."""
    return (r * math.sin(theta), -SINK_R + r * math.cos(theta))


def sector(t0, t1, r0, r1):
    return lo.hull2d([polar(t0, r0), polar(t1, r0), polar(t1, r1), polar(t0, r1)])


def ellipse(cx, cy, a, b, sides=28):
    return [(cx + a * math.cos(2.0 * math.pi * i / sides), cy + b * math.sin(2.0 * math.pi * i / sides))
            for i in range(sides)]


def clip_to(poly, region):
    for nx, ny, c in lo.plan_planes(region):
        poly = lo.clip(poly, (nx, ny), c)
        if len(poly) < 3:
            return []
    return poly


def den_floor(y):
    return DEN_FLOOR[0] + (DEN_FLOOR[1] - DEN_FLOOR[0]) * min(max(y / DEN_BACK, 0.0), 1.0)


def floor_height(y):
    """den_floor for an array of y."""
    return DEN_FLOOR[0] + (DEN_FLOOR[1] - DEN_FLOOR[0]) * np.clip(y / DEN_BACK, 0.0, 1.0)


def ellipsoid(px, py, pz, rx, ry, rz):
    """About the distance from (px, py, pz) to an ellipsoid of radii rx, ry, rz round the origin: exact on its surface,
    a close bound off it (Inigo Quilez's)."""
    k0 = np.sqrt((px / rx) ** 2 + (py / ry) ** 2 + (pz / rz) ** 2)
    k1 = np.sqrt((px / (rx * rx)) ** 2 + (py / (ry * ry)) ** 2 + (pz / (rz * rz)) ** 2)
    return k0 * (k0 - 1.0) / np.maximum(k1, 1e-6)


def inside_box(P, block):
    return np.all((P >= block.lo) & (P <= block.hi), axis=1)


class DenRockSolid(lo.Rock):
    """Den Rock's solid, with the den carved after the beds are joined: the joints and faces behind the den's face are
    filled (so the den is cut out of whole rock: no open joint shows in its walls or floor), the cave is cut out (its
    lobes melted into one, bulging with noise, over a calm floor), the arch's top is broken, and rubble lies along the
    walls. All of it inside DEN_BOX and before the weathering, so the den's walls weather as the face does. Until den is
    set (open_den), the field is the plain rock's: the face's cracks and chips are placed by tracing it."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.den = False
        self.fill = None
        self.breaks = []
        self.rubble = []
        self.with_rubble = True
        self.cave_noise = lo.Noise(48081)

    def cave(self, Q):
        """The cave at (warped) points Q: negative inside the open den, about metres."""
        x, y, z = Q[:, 0], Q[:, 1], Q[:, 2]
        h = z - floor_height(y)
        d = None
        for cx, cy, ch, rx, ry, rz in DEN_LOBES:
            e = ellipsoid(x - cx, y - cy, h - ch, rx, ry, rz)
            d = e if d is None else lo.smin(d, e, 0.9)
        d = d + 0.35 * self.cave_noise.fbm(x / 2.4, y / 2.4, z / 2.4, 2)
        return lo.smax(d, -h, 0.3)

    def _carve(self, P, d):
        # P is the warped point here (Rock.field carves at its warped points).
        if not self.den:
            return super()._carve(P, d)
        box = np.all((P >= DEN_BOX[0]) & (P <= DEN_BOX[1]), axis=1)
        fill = box & inside_box(P, self.fill)
        if fill.any():
            d[fill] = lo.smin(d[fill], self.fill.dist(P[fill]), self.fill.blend)
        d = super()._carve(P, d)
        if not box.any():
            return d
        Q = P[box]
        e = lo.smax(d[box], -self.cave(Q), 0.25)
        for block in self.breaks:
            near = inside_box(Q, block)
            if near.any():
                e[near] = lo.smax(e[near], -block.dist(Q[near]), block.blend)
        if self.with_rubble:
            for block in self.rubble:
                near = inside_box(Q, block)
                if near.any():
                    e[near] = lo.smin(e[near], block.dist(Q[near]), block.blend)
        d[box] = e
        return d


def den_rock():
    """Den Rock: a dome of the beds on the Sink's east rim. Its west face is the Sink's wall, a 19 m face from the
    floor (z = -12) up past the rim into the dome, curved on the Sink's circle; at its foot, under a brow of the middle
    bed, the Gravemother's den, sized for her to walk in and out: a cave mouth 7.7 m wide at the floor and 7 m at 2 m
    up, its broken arch rising to 4.5 m clear on its left (its rim near 5 m on the face) and 4 m on its right, opening
    into a chamber over 7 m wide all the way back, whose walls bulge in and out and whose roof comes down toward the
    back, widening to a pocket 8.4 m across and 3.8 m high where she rests (SOCKET_Den) that rounds off 9.3 m in.
    Its floor is flat, walkable trodden dirt (DenFloor), clear 6 m wide down the middle, with rubble along the walls
    and on the threshold's corners. Inside it the rock is RockCliffDen: no moss, a little darker, growing dim toward
    the back. The pivot is at rim level on the lip above the mouth; the mouth faces -Y (the Sink's centre is 18 m out
    that way). Writes the mouth's outline for the Sink's web funnel (den_mouth)."""
    rnd = random.Random(4808)
    rock = DenRockSolid(seed=4808, sink=12.8, lumps=(0.26, 4.2), detail=(0.08, 1.4), grain=(0.025, 0.5),
                        warp=(0.5, 7.5), fold=0.3)
    rock.calm_box((-3.7, -3.7, -12.6), (3.7, DEN_BACK + 0.6, -11.4), 0.12)

    # Columns of the face: wide masses between radial joints 3.5-6 m apart at the rim (the cliff kit's columns of
    # uneven width); narrower joints are carved into them below as grooves that die out.
    edges = [-0.63]
    while edges[-1] < 0.6:
        edges.append(edges[-1] + rnd.uniform(3.5, 6.0) / SINK_R)
    columns = []
    for t0, t1 in zip(edges, edges[1:]):
        roll = rnd.random()
        out = rnd.uniform(0.3, 0.7) if roll < 0.3 else rnd.uniform(-0.7, -0.25) if roll < 0.6 else rnd.uniform(-0.12, 0.12)
        columns.append(dict(t0=t0, t1=t1, mid=(t0 + t1) * 0.5, out=out, step=rnd.uniform(-0.7, 0.25),
                            tilt=(rnd.uniform(-2.0, 2.0), rnd.uniform(-2.0, 2.0))))

    def face_r(column, z):
        """Where the face stands for a column at height z: its own stand, leaning back 1.5 degrees, and curving back
        at the ends so neighbouring cliff pieces can overlap it."""
        end = max(0.0, abs(column['mid']) - 0.42) / 0.2
        return SINK_R - column['out'] + 0.026 * (z + 12.0) + 2.6 * min(end, 1.0) ** 1.5

    def add_block(poly, z0, z1, column, r_top=0.3, r_side=0.12, r_bottom=0.08, cut=None, tilt=1.0, blend=0.2):
        if len(poly) < 3 or abs(lo.area2d(poly)) < 0.25:
            return None
        cx, cy = centroid(poly)
        local = [(x - cx, y - cy) for x, y in poly]
        tip = (column['tilt'][0] * tilt + rnd.uniform(-0.6, 0.6), column['tilt'][1] * tilt + rnd.uniform(-0.6, 0.6))
        return rock.add(lo.Block(local, z0, z1, center=(cx, cy), tilt=tip, r_side=r_side, r_top=r_top,
                                 r_bottom=r_bottom, blend=blend, facets=cut if cut is not None else
                                 facets(rnd, rnd.choice((0, 1, 1, 2)), z1 - z0)))

    gap = 0.22 / SINK_R
    # The wall below the rim, 4.5 m thick, in three thick beds that fuse; over the den the middle one juts out half a
    # metre as a brow with a worn underside, which the den's arch breaks up into.
    # Its top stays just under the rim (the terrain's edge meets it; the dome stands on it).
    wall = [(-12.8, -7.6), (-7.6, -4.9), (-4.9, -0.35)]
    for k, (z0, z1) in enumerate(wall):
        for column in columns:
            r_in = face_r(column, z0) - (0.25 if k == 2 else 0.0)
            brow = k == 1 and abs(column['mid']) < 0.33
            if brow:
                r_in = face_r(column, z0) - 0.45 - 0.5 * column['out']
            poly = sector(column['t0'] + gap, column['t1'] - gap, r_in, SINK_R + 4.5)
            add_block(poly, z0 - (0.0 if k == 0 else 0.35), z1 + rnd.uniform(0.0, 0.2), column, r_side=0.16,
                      r_top=0.2, r_bottom=0.45 if brow else 0.7 if k == 1 else 0.05, tilt=0.6, blend=0.45)
    # Joints scored into the face as grooves that die out up or down (the cliff kit's), at their own leans.
    for _ in range(9):
        theta = rnd.uniform(-0.55, 0.55)
        if abs(theta) < 0.21:
            continue                        # none across the den's mouth
        x, y = polar(theta, SINK_R)
        z = rnd.uniform(-10.0, 3.0)
        lean = math.radians(rnd.uniform(-10.0, 10.0))
        normal = (math.cos(theta) * math.cos(lean), -math.sin(theta) * math.cos(lean), math.sin(lean))
        rock.crack((x, y, z), normal, width=rnd.uniform(0.14, 0.24), depth=rnd.uniform(0.4, 0.7),
                   radius=rnd.uniform(3.0, 5.5))
    # Behind the den, the rock that closes it: below the terrain, seen only from inside.
    rock.add(lo.Block(lo.rect(6.4, 3.9), -12.8, -4.0, center=(0.0, 6.9), r_side=0.3, r_top=0.4, blend=0.3))
    # The threshold: the den's floor running out and down under the Sink's floor.
    threshold = rock.add(lo.Block(lo.rect(4.6, 1.9), -12.8, DEN_FLOOR[0], center=(0.0, -1.3), r_side=0.4, r_top=0.12,
                                  blend=0.2, facets=[((0.0, -0.1, 1.0), 0.33)]))

    # The dome above the rim: beds of big loaves in two rings, set in toward a rounded top; the face carries on up and
    # rounds over the top two.
    rings = [23.5, 28.2, 33.5]
    dome = [(0.0, 2.0, 10.4, 14.5, -4.0), (2.0, 3.9, 9.0, 12.6, -4.0), (3.9, 5.6, 7.0, 9.9, 0.4),
            (5.6, 7.1, 4.6, 6.8, 2.1)]
    for k, (z0, z1, half, east, west) in enumerate(dome):
        outline = ellipse(0.0, (east + west) * 0.5, half, (east - west) * 0.5)
        last = k == len(dome) - 1
        for column in columns:
            radii = [face_r(column, z0) - 0.25] + rings
            for j, (r0, r1) in enumerate(zip(radii, radii[1:])):
                poly = clip_to(sector(column['t0'] + gap, column['t1'] - gap, r0 + (0.09 if j else 0.0), r1 - 0.09),
                               outline)
                if not poly or min_width(poly) < 1.0:
                    continue                # a sliver clipped off by the outline would stand like a plate
                bottom = -1.3 if (k == 0 and j > 0) else z0 - 0.3
                top = z1 + (column['step'] if last else 0.0) + rnd.uniform(0.0, 0.2)
                add_block(poly, bottom, top, column, r_side=0.22, r_top=0.95 if last else 0.6 if k else 0.35,
                          tilt=1.2 if k else 0.4, blend=0.35)

    for z, depth, width, fade in ((-10.3, 0.3, 0.16, 0.1), (-8.6, 0.2, 0.14, -0.2), (-6.0, 0.26, 0.15, 0.0),
                                  (-3.3, 0.34, 0.16, 0.2), (-1.4, 0.22, 0.14, -0.1), (1.0, 0.24, 0.15, 0.0),
                                  (3.0, 0.22, 0.14, -0.1), (4.7, 0.2, 0.14, -0.15)):
        rock.parting(z, depth=depth, width=width, fade=fade)
    face_crack(rock, (5.6, 6.0), 270.0, -3.6, 38.0, length=3.4, width=0.18, depth=0.28)
    face_crack(rock, (-6.8, 6.0), 270.0, -1.6, -32.0, length=3.0, width=0.18, depth=0.28)
    face_crack(rock, (2.0, 6.0), 270.0, 2.2, 44.0, length=2.6)
    for angle, z, size in ((300.0, 6.0, 1.3), (60.0, 5.6, 1.1), (200.0, 4.8, 1.2), (120.0, 3.6, 1.1)):
        chip(rock, rnd, (0.0, 6.0), angle, z, size)
    extra, fallen = [], []
    for x, y, size in ((-5.4, -1.4, 1.3), (5.7, -1.2, 0.9), (6.6, -0.6, 0.5), (-8.2, -2.2, 1.0), (9.0, -2.6, 0.7)):
        corners_ = talus_at(rock, rnd, (x, y), size, base=-12.0)
        fallen.append(rock.blocks[-1])
        if corners_ is not None:
            extra.append(corners_)

    # The den, carved last: the face's cracks and chips above were placed by tracing the face without it.
    open_den(rock)
    extra += den_rubble(rock, random.Random(4828))

    obj = lo.mesh_rock('DenRock', rock, cell=0.1, source=18000)
    lo.finish(obj, fallback=28, uv_seed=18, ao_distance=2.4)
    inside = den_faces(obj, rock)
    lt.assign(obj, den_material(), inside)
    floor = den_floor_faces(obj, rock, inside)
    lt.assign(obj, den_floor_material(), floor)
    lt.box_uv(obj, 'GroundDirt', faces=floor, seed=18)
    lo.darken(obj, lambda P: den_shade(rock, P))
    den_hulls(obj, rock, extra, fallen + [threshold])
    lo.lm.socket(obj, 'Den', (0.0, DEN_BACK - 1.0, den_floor(DEN_BACK - 1.0)))
    lo.lm.socket(obj, 'DenMouth', (0.0, 0.0, DEN_FLOOR[0]))
    den_mouth(rock, os.path.join(lt.REPO, 'Intermediate', 'DenRock', 'den_mouth.json'))
    return obj


def talus_at(rock, rnd, at, size, base=0.0):
    """A fallen block at (x, y) on ground at height base, half buried and tipped; its corners if big enough."""
    poly = lo.chamfer(lo.rect(size * 0.5, size * rnd.uniform(0.36, 0.46)), rnd, 0.8, (0.06, size * 0.2))
    h = size * rnd.uniform(0.55, 0.75)
    block = rock.add(lo.Block(poly, base - h * 0.4, base + h * 0.6, center=at, yaw=rnd.uniform(0.0, 360.0),
                              tilt=(rnd.uniform(-25.0, 25.0), rnd.uniform(-25.0, 25.0)), r_side=size * 0.12,
                              r_top=size * 0.16, r_bottom=size * 0.08, blend=0.1))
    return corners(block) if size >= 0.9 else None


def open_den(rock):
    """Opens the den in Den Rock's solid (a DenRockSolid): fills the joints and faces behind the den's face, so it is
    cut from whole rock, and breaks the arch's rim, so its top is broken rock, not a curve: chunks knocked out of it,
    each straddling the rim where the cave meets the face, at its own lean. They are shallow (under a metre into the
    face), so they rag the outline without hollowing a box out of the face above the mouth."""
    rock.fill = lo.Block(lo.rect(6.6, 5.15, 0.0, 5.45), -12.8, -5.2, r_side=0.3, r_top=0.3, r_bottom=0.1, blend=0.3)
    rnd = random.Random(4838)
    hs = np.arange(1.0, 7.0, 0.02)

    def rim(x, y):
        """The cave's top at plan point (x, y) of the solid's warped space, over the floor."""
        Q = np.stack([np.full_like(hs, x), np.full_like(hs, y), floor_height(np.full_like(hs, y)) + hs], axis=1)
        shut = np.nonzero(rock.cave(Q) > 0.0)[0]
        return float(hs[shut[0]]) if len(shut) else 7.0

    # (x, y) on the rim, half sizes, how far the chunk reaches below the rim and above it, yaw, tilt: three clusters of
    # overlapping chunks at different leans (the left shoulder, the crown, the right shoulder), each eating the rim's
    # edge more than the face above it, so the outline breaks into facets without holes cut above the arch.
    for at, half, below, above, yaw, tilt in (((-2.9, -0.95), (0.5, 0.45), 0.55, 0.3, 20.0, (-18.0, -12.0)),
                                              ((-2.2, -0.9), (0.45, 0.4), 0.45, 0.35, -25.0, (10.0, 14.0)),
                                              ((-1.5, -0.85), (0.4, 0.4), 0.4, 0.3, 35.0, (-24.0, -6.0)),
                                              ((0.1, -0.8), (0.5, 0.42), 0.5, 0.25, -18.0, (-14.0, 10.0)),
                                              ((0.75, -0.75), (0.38, 0.38), 0.4, 0.3, 28.0, (16.0, -12.0)),
                                              ((2.3, -0.6), (0.45, 0.4), 0.5, 0.3, -30.0, (-10.0, 12.0))):
        top = DEN_FLOOR[0] + rim(*at)
        poly = lo.chamfer(lo.rect(*half), rnd, 0.8, (0.08, min(half) * 0.6))
        facet = (rnd.uniform(-1.0, 1.0), rnd.uniform(-1.0, 1.0), rnd.uniform(0.2, 1.0))     # a fracture across it
        rock.breaks.append(lo.Block(poly, top - below, top + above, center=at, yaw=yaw, tilt=tilt, r_side=0.16,
                                    r_top=0.18, r_bottom=0.14, blend=0.1, facets=[(facet, 0.2)]))
    rock.den = True


def den_rubble(rock, rnd):
    """Rubble in the den, placed in the solid's warped space (where its cave is): stones fallen from the walls and roof
    lying along them, never within DEN_CLEAR of the den's line where they land (after the warp), so the way down the
    middle stays clear, and none in the mouth's throat (the Gravemother's legs pass there); two big blocks fallen from
    the roof; two stones on the threshold's corners. Returns the big ones' corners for hulls of their own (the player
    steps over the rest)."""
    hulls = []

    def wall(y, side):
        """How far from the den's line its wall stands at y, 0.35 m over the floor."""
        xs = side * np.arange(0.0, 6.0, 0.05)
        Q = np.stack([xs, np.full_like(xs, y), floor_height(np.full_like(xs, y)) + 0.35], axis=1)
        solid = np.nonzero(rock.cave(Q) > 0.0)[0]
        return float(abs(xs[solid[0]])) if len(solid) else 6.0

    def stone(x, y, size, sink=0.35, tip=20.0, base=None, clear=True):
        """A stone at (x, y), sunk a share `sink` of its height into the floor (at base, or the den's). Its random
        draws come first, so moving it clear of the way changes nothing after it."""
        poly = lo.chamfer(lo.rect(size * 0.5, size * rnd.uniform(0.36, 0.46)), rnd, 0.8, (0.05, size * 0.2))
        h = size * rnd.uniform(0.5, 0.7)
        yaw = rnd.uniform(0.0, 360.0)
        tilt = (rnd.uniform(-tip, tip), rnd.uniform(-tip, tip))
        side = 1.0 if x > 0.0 else -1.0
        for _ in range(8):
            ground = float(floor_height(np.array([y]))[0]) if base is None else base
            block = lo.Block(poly, ground - h * sink, ground + h * (1.0 - sink), center=(x, y), yaw=yaw, tilt=tilt,
                             r_side=size * 0.12, r_top=size * 0.16, r_bottom=size * 0.08, blend=0.06)
            # Where it lands: the solid is looked up at warped points, so a stone placed at Q shows at about Q - warp.
            at = np.array([[x, y, ground]])
            points = corners(block) - (rock.warped(at)[0] - at[0])
            inner = float(np.min(side * points[:, 0]))
            if not clear or inner >= DEN_CLEAR:
                break
            x += side * (DEN_CLEAR - inner + 0.02)
        rock.rubble.append(block)
        if size >= 0.9:
            hulls.append(points)

    # Along each wall, heaps: a stone lying out from the wall's foot (not sunk into it, so it reads from the mouth) with
    # one or two small ones round it.
    for side in (-1.0, 1.0):
        y = rnd.uniform(1.6, 2.2)
        while y < 7.6:
            size = rnd.uniform(0.6, 0.98)
            stone(side * max(wall(y, side) - size * 0.45, DEN_CLEAR + size * 0.5), y, size)
            for _ in range(rnd.choice((1, 1, 2))):
                small, y2 = rnd.uniform(0.25, 0.5), y + rnd.uniform(-0.8, 0.8)
                stone(side * max(wall(y2, side) - small * 0.3, DEN_CLEAR + small * 0.5), y2, small)
            y += rnd.uniform(1.3, 1.9)
    # The pocket's back corners, behind the Gravemother's place; two big blocks fallen from the roof; the threshold.
    stone(-2.3, 8.4, 0.5, clear=False)
    stone(2.4, 8.2, 0.4, clear=False)
    stone(-max(wall(2.4, -1.0) - 0.4, DEN_CLEAR + 0.75), 2.4, 1.3, sink=0.3, tip=15.0)
    stone(max(wall(6.0, 1.0) - 0.35, DEN_CLEAR + 0.65), 6.0, 1.1, sink=0.3, tip=15.0)
    stone(4.4, -2.1, 1.0, base=-12.0)
    stone(-4.3, -2.7, 0.6, base=-12.0)
    return hulls


def den_material():
    """RockCliffDen, the rock inside the den: RockCliff without moss, a little darker (its tint), its occlusion (the
    baked AO, dimmed toward the back by den_shade) darkening its color more than outside (DiffuseAO)."""
    return lt.material('RockCliff', name='RockCliffDen', tint=0xdcd6ce, MossAmount=0.0, DiffuseAO=0.85)


def den_floor_material():
    """DenFloor, the den's trodden floor and its threshold: GroundDirt (the Sink's floor runs in) without moss, a
    little grey, dimmed by its occlusion as RockCliffDen is, so it darkens toward the back with the walls. RockCliff's
    strata would lie on a floor as stripes running in like planks."""
    return lt.material('GroundDirt', name='DenFloor', tint=0xc9bfb2, MossAmount=0.0, DiffuseAO=0.85)


def den_faces(obj, rock):
    """The faces inside the den and on its threshold: they take RockCliffDen. The outer face round the mouth (facing
    out, in front of the lip) keeps RockCliff, and so do the stones out on the threshold."""
    mesh = obj.data
    count = len(mesh.polygons)
    C, N = np.empty(3 * count), np.empty(3 * count)
    mesh.polygons.foreach_get('center', C)
    mesh.polygons.foreach_get('normal', N)
    C, N = C.reshape(-1, 3), N.reshape(-1, 3)
    Q = rock.warped(C)
    x, y, z = C[:, 0], C[:, 1], C[:, 2]
    inside = (rock.cave(Q) < 0.7) & (y > -2.5) & (np.abs(x) < 6.0) & (z < -5.5) & ((N[:, 1] > -0.55) | (y > 0.6))
    floor = (N[:, 2] > 0.45) & (np.abs(x) < 4.6) & (y > -3.6) & (y < 0.5) & (z < -11.8)
    out = np.zeros(count, bool)
    for block in rock.rubble:
        if block.origin[1] < -1.0:
            near = inside_box(Q, block)
            out[near] |= block.dist(Q[near]) < 0.12
    return [int(i) for i in np.nonzero((inside | floor) & ~out)[0]]


def den_floor_faces(obj, rock, faces):
    """Of the den's faces (den_faces), its floor and the threshold's top, which take DenFloor: facing up, at most
    0.3 m over the floor, and not on a stone."""
    mesh = obj.data
    count = len(mesh.polygons)
    C, N = np.empty(3 * count), np.empty(3 * count)
    mesh.polygons.foreach_get('center', C)
    mesh.polygons.foreach_get('normal', N)
    index = np.asarray(faces, np.int64)
    C, N = C.reshape(-1, 3)[index], N.reshape(-1, 3)[index]
    Q = rock.warped(C)
    stone = np.zeros(len(index), bool)
    for block in rock.rubble:
        near = inside_box(Q, block)
        stone[near] |= block.dist(Q[near]) < 0.1
    keep = (N[:, 2] > 0.7) & (C[:, 2] - floor_height(C[:, 1]) < 0.3) & ~stone
    return [int(i) for i in index[keep]]


def den_shade(rock, P):
    """The den grows dim toward its back: a multiplier on the baked AO (the ambient light) of the rock round its open
    space, from 1 at the lip to 0.22 at its back."""
    near = lo.smoothstep(1.2, 0.4, rock.cave(rock.warped(P)))
    return 1.0 - 0.78 * lo.smoothstep(-0.6, 8.6, P[:, 1]) * near


def den_shell(rock, apart, step=0.2, reach=2.6, tolerance=0.3, levels=5, smallest=16):
    """The collision round the den's open space: the rock within reach of it, above its floor (not the blocks in apart,
    which have hulls of their own or none), sampled on a grid and cut into convex pieces: slabs along the den, sectors
    round its line, each halved across its longest direction until no open space inside it lies further than
    tolerance from the rock (a hull bridging a hollow or a corner holds air far from the walls), or `levels` halvings.
    Neighbouring pieces share a row of samples, so they leave no gap. Returns the pieces' points and the open space's
    top, back and sides (the rest of the rock's collision keeps beyond those)."""
    import bmesh
    xs = np.arange(-7.4, 7.41, step)
    ys = np.arange(-2.8, 11.41, step)
    hs = np.arange(-0.1, 7.76, step)
    X, Y, H = np.meshgrid(xs, ys, hs, indexing='ij')
    P = np.stack([X.ravel(), Y.ravel(), (H + floor_height(Y)).ravel()], axis=1)
    h = H.ravel()
    rock.with_rubble = False
    bare = rock.field(P)
    rock.with_rubble = True
    full = rock.field(P)
    Q = rock.warped(P)
    cave = rock.cave(Q)
    loose = np.zeros(len(P), bool)
    for block in apart:
        near = inside_box(Q, block)
        loose[near] |= block.dist(Q[near]) < 0.15
    solid = (bare < 0.05) & (cave > -0.15) & (cave < reach) & (h > 0.05) & ~loose
    air = full > 0.12                          # open space clear of the rock (not its narrow grooves and cracks)
    S, A, hS, fA = P[solid], P[air], h[solid], full[air]
    phi = np.degrees(np.arctan2(hS - 1.4, S[:, 0]))
    phi = np.where(phi < -90.0, phi + 360.0, phi)
    dirs = lo._directions(2)

    def reach_into(index):
        """The hull of the samples index (their extreme points), and how far from the rock open space inside it lies."""
        ext = lo.extremes(S[index], dirs)
        bm = bmesh.new()
        for p in ext:
            bm.verts.new(p)
        bmesh.ops.convex_hull(bm, input=bm.verts[:])
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        bm.normal_update()
        planes = np.array([(*f.normal, f.normal.dot(f.verts[0].co)) for f in bm.faces if f.calc_area() > 1e-8])
        bm.free()
        box = np.all((A >= ext.min(axis=0)) & (A <= ext.max(axis=0)), axis=1)
        if not box.any() or not len(planes):
            return ext, 0.0
        held = (A[box] @ planes[:, :3].T - planes[:, 3]).max(axis=1) <= 0.0
        return ext, float(fA[box][held].max()) if held.any() else 0.0

    pieces = []

    def cut(index, level):
        ext, depth = reach_into(index)
        if depth <= tolerance or level >= levels or len(index) < smallest:
            pieces.append((ext, depth))
            return
        # Halved across its longest direction (a corner piece is cut where the walls meet, not along them); the
        # halves share the samples within half a step of the cut.
        pts = S[index] - S[index].mean(axis=0)
        t = pts @ np.linalg.eigh(pts.T @ pts)[1][:, -1]
        m = float(np.median(t))
        parts = (index[t <= m + step * 0.5], index[t >= m - step * 0.5])
        if max(len(part) for part in parts) >= len(index):
            pieces.append((ext, depth))                 # too thin to halve
            return
        for part in parts:
            cut(part, level + 1)

    slabs = (-2.8, -1.0, 0.8, 2.6, 4.4, 6.2, 8.0, 11.4)
    sectors = (-90.0, 15.0, 55.0, 90.0, 125.0, 165.0, 270.0)
    for y0, y1 in zip(slabs, slabs[1:]):
        for a0, a1 in zip(sectors, sectors[1:]):
            index = np.nonzero((S[:, 1] >= y0 - step * 0.5) & (S[:, 1] <= y1 + step * 0.5) & (phi >= a0 - 4.0) &
                               (phi <= a1 + 4.0))[0]
            if len(index) >= 4:
                cut(index, 0)
    den = P[(cave < -0.12) & (full > 0.05) & (P[:, 1] > -1.0)]
    top = float(den[:, 2].max()) + 0.15
    back = float(den[:, 1].max()) + 0.4
    left, right = float(den[:, 0].min()) - 0.15, float(den[:, 0].max()) + 0.15
    lo.log(f'den shell: {len(pieces)} pieces, holding open space at most {max(d for _, d in pieces):.2f} m from the '
           f'rock; its top {top:.2f}, back {back:.2f}, sides {left:.2f} to {right:.2f}')
    return [p for p, _ in pieces], top, back, left, right


def den_hulls(obj, rock, extra, apart):
    """The den's collision, which must leave its open space open: a box under its floor and one under the threshold
    (what a player and the Gravemother walk on, meeting without a gap), the shell round the cave (den_shell), then the
    rest of the rock beyond it in lateral columns: the face and rock either side, above the den and behind its pocket,
    and the dome bed by bed, each bed in two halves. Loose blocks (apart: the talus and the threshold; the rubble) stay
    out of those; the big ones have hulls of their own (extra)."""
    hull = lo.hull_object
    floor = []
    for y in (-0.6, DEN_BACK, DEN_BACK + 1.6):
        for x in (-5.2, 5.2):
            floor += [(x, y, -12.8), (x, y, den_floor(y) - 0.02)]
    hull(obj, floor)
    hull(obj, [(x, y, z) for x in (-4.3, 4.3) for y, z in ((-3.1, -12.28), (-0.6, DEN_FLOOR[0] - 0.02))] +
              [(x, y, -12.8) for x in (-4.3, 4.3) for y in (-3.1, -0.6)])
    shell, top, back, left, right = den_shell(rock, apart)
    for points in shell:
        hull(obj, points)
    V = lo.vertices(obj)
    Q = rock.warped(V)
    loose = np.zeros(len(V), bool)
    for block in list(apart) + rock.rubble:
        near = inside_box(Q, block)
        loose[near] |= block.dist(Q[near]) < 0.1
    dirs = lo._directions(2)
    x, y, z = V[:, 0], V[:, 1], V[:, 2]
    below = (z < 0.3) & ~loose
    over = (x > left - 0.15) & (x < right + 0.15)
    groups = [below & (x < -7.6), below & (x >= -7.9) & (x < left), below & (x > right) & (x <= 7.9),
              below & (x > 7.6), below & over & (z > top), below & over & (z <= top) & (y > back)]
    # The dome bed by bed, so its collision steps as its beds do (1.7-2 m each, more than a jump) instead of making
    # a ramp up it.
    for lo_z, hi_z in ((-1.5, 2.1), (1.9, 4.0), (3.8, 5.7), (5.5, 7.8)):
        band = (z >= lo_z) & (z <= hi_z)
        groups += [band & (x < 0.4), band & (x > -0.4)]
    for mask in groups:
        if mask.sum() >= 4:
            hull(obj, lo.extremes(V[mask], dirs))
    for points in extra:
        hull(obj, lo.extremes(np.asarray(points), dirs))


def den_mouth(rock, path):
    """Writes where the den's opening meets the rock on its outer face, for the Sink's web funnel (Sink.py's
    FUNNEL_LOOP): a closed loop in SOCKET_DenMouth's space (x right looking in, y into the den, z the height over the
    floor at the mouth), from the floor at the left up the left side, over the arch, down the right side and back along
    the floor, each point at the y where the face stands at that edge; and the den's outline at a few depths (sections)
    for the funnel's rings. The opening is what is open straight through from 2 m out to 1 m in (rubble left out)."""
    base = DEN_FLOOR[0]

    def opening(xh, depths):
        """For (x, h) points: open at every depth in depths."""
        xh = np.asarray(xh, np.float64).reshape(-1, 2)
        k = len(depths)
        P = np.stack([np.repeat(xh[:, 0], k), np.tile(depths, len(xh)), np.repeat(xh[:, 1], k) + base], axis=1)
        return (rock.field(P) > 0.0).reshape(len(xh), k).all(axis=1)

    def edge(origin, direction, depths, reach=7.0, step=0.03):
        """Where a ray from origin (x, h) along direction leaves the opening."""
        o, u = np.asarray(origin, np.float64), np.asarray(direction, np.float64)
        t = np.arange(0.0, reach, step)
        shut = np.nonzero(~opening(o + t[:, None] * u, depths))[0]
        return o + (t[shut[0]] - step * 0.5 if len(shut) else reach) * u

    def outline(depths):
        """The loop: (x, h) and the outward direction at each point."""
        loop = [(edge((0.0, hh), (-1.0, 0.0), depths), (-1.0, 0.0)) for hh in (0.1, 0.7, 1.3, 1.9)]
        for i in range(22):
            a = math.radians(180.0 - 180.0 * i / 21.0)
            u = (math.cos(a), math.sin(a))
            loop.append((edge((-0.3, 2.4), u, depths), u))
        loop += [(edge((0.0, hh), (1.0, 0.0), depths), (1.0, 0.0)) for hh in (1.9, 1.3, 0.7, 0.1)]
        right, left = loop[-1][0], loop[0][0]
        for i in range(8):
            f = (i + 1) / 9.0
            loop.append((np.array([right[0] + (left[0] - right[0]) * f, 0.03]), (0.0, -1.0)))
        return loop

    def face_y(p, u):
        """Where the face stands at the edge point p: 6 cm into the rock, the first rock coming in from outside."""
        ys = np.arange(-2.0, 1.0, 0.02)
        P = np.stack([np.full_like(ys, p[0] + 0.06 * u[0]), ys, np.full_like(ys, base + p[1] + 0.06 * u[1])], axis=1)
        solid = np.nonzero(rock.field(P) < 0.0)[0]
        return float(ys[solid[0]]) if len(solid) else 1.0

    rock.with_rubble = False
    try:
        loop = outline(np.arange(-2.0, 1.01, 0.1))
        points = [[float(p[0]), face_y(p, u), float(p[1])] for p, u in loop[:30]]
        y_right, y_left = points[-1][1], points[0][1]
        for i, (p, _) in enumerate(loop[30:]):
            f = (i + 1) / 9.0
            points.append([float(p[0]), y_right + (y_left - y_right) * f, float(p[1])])
        sections = [(y, [[float(p[0]), float(p[1])] for p, _ in outline(np.array([y]))])
                    for y in (-0.6, -0.2, 0.45, 1.25, 2.0, 2.6, 3.2)]
    finally:
        rock.with_rubble = True

    def row(values):
        return '[' + ', '.join(f'{v:.3f}' for v in values) + ']'
    text = ['{', '  "space": "SOCKET_DenMouth",', '  "units": "m",',
            '  "axes": "+X right looking in, +Y into the den, +Z up (the height over the den floor at the mouth, '
            f'z = {base})",',
            '  "closed": true,',
            '  "order": "from the floor at the left up the left side, over the arch, down the right side, back along '
            'the floor (clockwise seen from outside); x, y, z each",',
            '  "points": [', ',\n'.join('    ' + row(p) for p in points), '  ],',
            '  "sections": [']
    text.append(',\n'.join('    {"y": ' + f'{y:.2f}' + ', "points": [' + ', '.join(row(p) for p in pts) + ']}'
                           for y, pts in sections))
    text += ['  ]', '}']
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(text) + '\n')
    P = np.array(points)
    lo.log(f'den mouth: {len(points)} points, x {P[:, 0].min():.2f} to {P[:, 0].max():.2f}, up to '
           f'{P[:, 2].max():.2f} m, face y {P[:, 1].min():.2f} to {P[:, 1].max():.2f}: {path}')


# --- Far rocks: the ridges past the playable boundary ---

def far_rock(name, seed, plan, beds, tris=170):
    """A low-poly rock of the same beds, for the ridges past the boundary: a few beds set in toward the top, reduced
    to about `tris` triangles. Instanced by the hundreds: no Nanite, no LODs, no collision (place them with shadows
    off)."""
    rnd = random.Random(seed)
    rock = lo.Rock(seed=seed, sink=0.5, lumps=(0.22, 4.0), detail=(0.0, 1.0), grain=(0.0, 1.0), rain=0.0,
                   warp=(0.4, 6.0), fold=0.15)
    for z0, z1, inset, shift, length in beds:
        p = [(x * length, y) for x, y in plan]
        p = lo.offset(lo.transform2d(p, rnd.uniform(-6.0, 6.0), shift), -inset)
        rock.add(lo.Block(lo.chamfer(p, rnd, 0.9, (0.3, 0.8)), z0 - (0.0 if z0 <= -0.5 else 0.15), z1,
                          r_side=0.22, r_top=0.35, r_bottom=0.08, blend=0.25, facets=facets(rnd, 2, z1 - z0)))
    obj = lo.mesh_rock(name, rock, cell=0.18, source=tris)
    lo.finish(obj, uv_seed=seed, ao_distance=1.5, nanite=False, smooth=50.0)
    obj['Collision'] = 'None'
    return obj


def far_rocks():
    rnd = random.Random(4909)
    return [
        far_rock('FarRock_A', 4911, lo.chamfer(lo.rect(3.0, 2.5), rnd, 0.9, (0.4, 1.0)),
                 [(-0.5, 1.8, 0.0, (0.0, 0.0), 1.0), (1.8, 3.5, 0.5, (0.35, 0.1), 1.0)]),
        far_rock('FarRock_B', 4912, lo.chamfer(lo.rect(1.8, 1.5), rnd, 0.9, (0.3, 0.7)),
                 [(-0.5, 2.6, 0.0, (0.0, 0.0), 1.0), (2.6, 5.0, 0.25, (0.15, 0.0), 1.0),
                  (5.0, 7.0, 0.5, (0.1, 0.2), 1.0)]),
        far_rock('FarRock_C', 4913, lo.chamfer(lo.rect(4.6, 2.0), rnd, 0.9, (0.4, 1.0)),
                 [(-0.5, 1.4, 0.0, (0.0, 0.0), 1.0), (1.4, 2.5, 0.55, (0.5, 0.0), 0.8)]),
        far_rock('FarRock_D', 4914, lo.hull2d([(-4.0, -0.6), (-3.0, -1.5), (1.5, -1.6), (4.0, -0.8), (3.8, 0.8),
                                              (1.0, 1.5), (-3.2, 1.3)]),
                 [(-0.5, 2.4, 0.0, (0.0, 0.0), 1.0), (2.4, 4.0, 0.2, (-1.3, 0.2), 0.62),
                  (4.0, 5.6, 0.35, (-2.4, -0.1), 0.3)]),
    ]


KIT = {
    'TorA': tor_a,
    'TorB': tor_b,
    'TorC': tor_c,
    'TorD': tor_d,
    'Spine': spine,
    'Slab': slab,
    'Fin': fin,
}
DEN = {'DenRock': den_rock}
FAR = {'FarRocks': far_rocks}


ALL = {**KIT, **DEN, **FAR}


def wanted(name):
    names = [a for a in ARGS if a in ALL]
    return not names or name in names


models = []
for _name in ALL:
    if wanted(_name):
        built = ALL[_name]()
        models += built if isinstance(built, list) else [built]
for model in models:
    lo.hull_report(model)
    _v = lo.vertices(model)
    _size = _v.max(axis=0) - _v.min(axis=0)
    lo.log(f'{model.name}: {lo.triangles(model)} triangles, fallback {model.get("Fallback")}% '
           f'(about {int(lo.triangles(model) * model.get("Fallback", 100) / 100)}); {_size[0]:.1f} x {_size[1]:.1f} m, '
           f'z {_v[:, 2].min():.1f} to {_v[:, 2].max():.1f}')

VIEWS = {   # the two angles each piece is shown from: its front three-quarter, then from behind
    'Outcrop_TorB': [(0.3, -1.6, 0.45), (1.3, 1.2, 0.35)],
}


def overview(rows, path, gap=2.5, view=(-0.22, -1.6, 0.5), fit=0.62):
    """Every piece side by side, rows front to back, as placed (cut at their ground), with the 1.8 m figure."""
    import bpy
    shown = []
    y = 0.0
    for row in rows:
        x, placed = 0.0, []
        for model, ground in row:
            copy = lo.placed_copy(model, ground)
            copy.location.z = -ground
            bpy.context.view_layer.update()
            lo_, hi_ = lo.bounds([copy])
            copy.location.x += x - lo_[0]
            copy.location.y += y - lo_[1]
            x += hi_[0] - lo_[0] + gap
            placed.append(copy)
        bpy.context.view_layer.update()
        for copy in placed:
            copy.location.x -= (x - gap) * 0.5
        bpy.context.view_layer.update()
        depth = max(lo.bounds([c])[1][1] for c in placed) - y
        y += depth + gap * 3.0
        shown += placed
    bpy.context.view_layer.update()
    lo_, hi_ = lo.bounds(shown)
    shown.append(lo.scale_figure((lo.bounds(shown[:1])[0][0] - 1.2, lo_[1] + 0.5, 0.0)))
    try:
        lt.preview(shown, path, view=view, fit=fit, lens=35.0, ground_at=0.0)
    finally:
        lo.remove(shown)


def pit_mock():
    """A stand-in for the Sink around Den Rock in its previews (removed again): the floor at -12, the rim at 0, the
    pit's edge pushed back behind the rock's face where the rock stands."""
    import bpy
    xs = np.arange(-26.0, 26.01, 0.5)
    ys = np.arange(-40.0, 24.01, 0.5)
    X, Y = np.meshgrid(xs, ys)
    r = np.hypot(X, Y + SINK_R)
    t = np.abs(np.arctan2(X, Y + SINK_R))
    edge = 17.6 + 4.6 * lo.smoothstep(0.66, 0.5, t)
    Z = -12.0 + 12.0 * lo.smoothstep(edge - 0.4, edge + 0.6, r)
    verts = np.stack([X.ravel(), Y.ravel(), Z.ravel()], axis=1)
    n = len(xs)
    faces = [(j * n + i, j * n + i + 1, (j + 1) * n + i + 1, (j + 1) * n + i)
             for j in range(len(ys) - 1) for i in range(n - 1)]
    mesh = bpy.data.meshes.new('Preview_Pit')
    mesh.from_pydata([tuple(v) for v in verts], [], faces)
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    mesh.materials.append(bpy.data.materials.get('PreviewSoil') or lo.lm.material('PreviewSoil', 0x9a8566))
    obj = bpy.data.objects.new('Preview_Pit', mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def crop(obj, low, high):
    """Cuts a preview copy down to a box (a close view frames what is left)."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for axis in range(3):
        for sign, value in ((1.0, high[axis]), (-1.0, low[axis])):
            normal = [0.0, 0.0, 0.0]
            point = [0.0, 0.0, 0.0]
            normal[axis], point[axis] = sign, value
            bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=point,
                                   plane_no=normal, clear_outer=True)
    bm.to_mesh(obj.data)
    bm.free()
    lo.refresh(obj)


def den_previews(model):
    base = lt.preview_path('RansomsRest/Rocks', model.name)
    full, pit = lo.placed_copy(model, -20.0), pit_mock()
    try:
        lt.preview([full, pit], base, view=(-0.55, -1.6, 0.8), fit=0.42, ground_at=-12.5)
    finally:
        lo.remove([full, pit])
    near = lo.placed_copy(model, -20.0)
    crop(near, (-9.0, -5.0, -12.9), (9.0, 9.0, -3.2))
    figure = lo.scale_figure((2.4, -2.2, -12.0))
    try:
        lt.preview([near, figure], base.replace('.png', '_Den.png'), view=(-0.38, -1.6, 0.1), fit=0.48,
                   ground_at=-12.0)
    finally:
        lo.remove([near, figure])
    lo.preview(model, base.replace('.png', '_B.png'), view=(1.1, 1.5, 0.5), ground=0.0)


if lt.want_preview():
    far = [m for m in models if m.name.startswith('FarRock_')]
    for model in models:
        if model.name == 'DenRock':
            den_previews(model)
            continue
        if model in far:
            continue
        base = lt.preview_path('RansomsRest/Rocks', model.name)
        front, back = VIEWS.get(model.name, [(-1.0, -1.6, 0.55), (1.3, 1.2, 0.35)])
        lo.preview(model, base, view=front)
        lo.preview(model, base.replace('.png', '_B.png'), view=back)
    if far:
        path = lt.preview_path('RansomsRest/Rocks', 'FarRocks')
        overview([[(m, 0.0) for m in far]], path, gap=1.5, view=(-0.5, -1.6, 0.45), fit=0.8)
        overview([[(m, 0.0) for m in far]], path.replace('.png', '_B.png'), gap=1.5, view=(0.6, 1.6, 0.35), fit=0.8)
    if not [a for a in ARGS if a in ALL]:
        by = {m.name: m for m in models}
        rows = [[(by[n], 0.0) for n in ('FarRock_A', 'FarRock_B', 'FarRock_C', 'FarRock_D', 'Outcrop_Slab',
                                        'Outcrop_TorD')],
                [(by[n], 0.0) for n in ('Outcrop_TorA', 'Outcrop_TorC', 'Outcrop_Fin', 'Outcrop_Spine')],
                [(by['Outcrop_TorB'], 0.0), (by['DenRock'], -12.0)]]
        overview(rows, lt.preview_path('RansomsRest', 'Rocks_overview'), gap=3.0, view=(-0.2, -1.6, 0.62), fit=0.74)
