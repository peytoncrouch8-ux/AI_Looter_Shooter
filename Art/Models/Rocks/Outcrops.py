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
DEN_BACK = 8.3                    # how far behind the lip the den's pocket reaches
DEN_HALF = 3.35                   # half the den's width, where the collision of its sides begins


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


def den_rock():
    """Den Rock: a dome of the beds on the Sink's east rim. Its west face is the Sink's wall, a 19 m face from the
    floor (z = -12) up past the rim into the dome, curved on the Sink's circle; at its foot, under an overhang, the
    Gravemother's den: a mouth 7 m wide and 4.5 m high, a chamber narrowing to a closed pocket 8 m back, its floor
    flat and walkable. The pivot is at rim level on the lip above the mouth; the mouth faces -Y (the Sink's centre is
    18 m out that way)."""
    rnd = random.Random(4808)
    rock = lo.Rock(seed=4808, sink=12.8, lumps=(0.26, 4.2), detail=(0.08, 1.4), grain=(0.025, 0.5),
                   warp=(0.5, 7.5), fold=0.3)
    rock.calm_box((-3.9, -3.7, -12.6), (3.9, DEN_BACK + 0.4, -11.2), 0.12)

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
    # The wall below the rim, 4.5 m thick, in three thick beds that fuse; over the den the middle one juts out 2 m as
    # one overhanging mass with a worn underside.
    # Its top stays just under the rim (the terrain's edge meets it; the dome stands on it).
    wall = [(-12.8, -7.6), (-7.6, -4.9), (-4.9, -0.35)]
    for k, (z0, z1) in enumerate(wall):
        for column in columns:
            r_in = face_r(column, z0) - (0.25 if k == 2 else 0.0)
            if k == 1 and abs(column['mid']) < 0.33:
                r_in = SINK_R - 2.0 - 0.3 * column['out']
            poly = sector(column['t0'] + gap, column['t1'] - gap, r_in, SINK_R + 4.5)
            add_block(poly, z0 - (0.0 if k == 0 else 0.35), z1 + rnd.uniform(0.0, 0.2), column, r_side=0.16,
                      r_top=0.2, r_bottom=0.7 if k == 1 else 0.05, tilt=0.6, blend=0.45)
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
    rock.add(lo.Block(lo.rect(4.6, 1.9), -12.8, DEN_FLOOR[0], center=(0.0, -1.3), r_side=0.4, r_top=0.12,
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

    # The den: a mouth (an uneven arch, higher on one side), a chamber and a closed pocket carved out of the foot of
    # the face.
    rock.cut(lo.Block(lo.rect(3.5, 3.4), DEN_FLOOR[0], -7.5, center=(0.0, 0.1), r_side=0.8, r_top=1.7,
                      r_bottom=0.1, blend=0.2, taper=0.2))
    rock.cut(lo.Block(lo.rect(1.9, 2.2), -10.0, -7.0, center=(-1.2, 0.4), yaw=8.0, tilt=(0.0, 9.0), r_side=0.8,
                      r_top=1.0, r_bottom=0.3, blend=0.25))
    rock.cut(lo.Block(lo.rect(3.15, 1.6), den_floor(4.6), -7.8, center=(0.0, 4.6), r_side=0.9, r_top=1.6,
                      r_bottom=0.1, blend=0.2))
    rock.cut(lo.Block(lo.rect(2.55, 1.45), DEN_FLOOR[1], -8.5, center=(0.0, 6.9), r_side=1.2, r_top=1.4,
                      r_bottom=0.1, blend=0.2))

    for z, depth, width, fade in ((-10.3, 0.3, 0.16, 0.1), (-8.6, 0.2, 0.14, -0.2), (-6.0, 0.26, 0.15, 0.0),
                                  (-3.3, 0.34, 0.16, 0.2), (-1.4, 0.22, 0.14, -0.1), (1.0, 0.24, 0.15, 0.0),
                                  (3.0, 0.22, 0.14, -0.1), (4.7, 0.2, 0.14, -0.15)):
        rock.parting(z, depth=depth, width=width, fade=fade)
    face_crack(rock, (5.6, 6.0), 270.0, -3.6, 38.0, length=3.4, width=0.18, depth=0.28)
    face_crack(rock, (-6.8, 6.0), 270.0, -1.6, -32.0, length=3.0, width=0.18, depth=0.28)
    face_crack(rock, (2.0, 6.0), 270.0, 2.2, 44.0, length=2.6)
    for angle, z, size in ((300.0, 6.0, 1.3), (60.0, 5.6, 1.1), (200.0, 4.8, 1.2), (120.0, 3.6, 1.1)):
        chip(rock, rnd, (0.0, 6.0), angle, z, size)
    extra = []
    for x, y, size in ((-5.4, -1.4, 1.3), (5.7, -1.2, 0.9), (6.6, -0.6, 0.5), (-8.2, -2.2, 1.0), (9.0, -2.6, 0.7)):
        corners_ = talus_at(rock, rnd, (x, y), size, base=-12.0)
        if corners_ is not None:
            extra.append(corners_)

    obj = lo.mesh_rock('DenRock', rock, cell=0.1, source=18000)
    lo.finish(obj, fallback=28, uv_seed=18, ao_distance=2.4)
    lo.darken(obj, den_shade)
    den_hulls(obj, extra)
    lo.lm.socket(obj, 'Den', (0.0, DEN_BACK - 1.0, den_floor(DEN_BACK - 1.0)))
    lo.lm.socket(obj, 'DenMouth', (0.0, 0.0, DEN_FLOOR[0]))
    return obj


def talus_at(rock, rnd, at, size, base=0.0):
    """A fallen block at (x, y) on ground at height base, half buried and tipped; its corners if big enough."""
    poly = lo.chamfer(lo.rect(size * 0.5, size * rnd.uniform(0.36, 0.46)), rnd, 0.8, (0.06, size * 0.2))
    h = size * rnd.uniform(0.55, 0.75)
    block = rock.add(lo.Block(poly, base - h * 0.4, base + h * 0.6, center=at, yaw=rnd.uniform(0.0, 360.0),
                              tilt=(rnd.uniform(-25.0, 25.0), rnd.uniform(-25.0, 25.0)), r_side=size * 0.12,
                              r_top=size * 0.16, r_bottom=size * 0.08, blend=0.1))
    return corners(block) if size >= 0.9 else None


def den_shade(P):
    """The den grows dim toward its back: a multiplier on the baked AO (ambient light) inside it."""
    inside = (np.abs(P[:, 0]) < 4.2) & (P[:, 2] < -6.9) & (P[:, 1] > -0.8)
    depth = np.clip((P[:, 1] + 0.3) / 6.8, 0.0, 1.0)
    shade = 1.0 - 0.8 * depth * depth * (3.0 - 2.0 * depth)
    return np.where(inside, shade, 1.0)


def den_hulls(obj, extra):
    """The den's collision, which must leave its pocket open: two boxes under its floor and the threshold (the floor
    a player and the Gravemother walk on), the face beside and above the den in lateral columns, the rock behind its
    pocket, and the dome in two bands of three."""
    hull = lo.hull_object
    floor = []
    for y in (-0.6, DEN_BACK - 0.1):
        for x in (-DEN_HALF, DEN_HALF):
            floor += [(x, y, -12.8), (x, y, den_floor(y) - 0.02)]
    hull(obj, floor)
    hull(obj, [(x, y, z) for x in (-4.3, 4.3) for y, z in ((-3.1, -12.28), (-0.6, DEN_FLOOR[0] - 0.02))] +
              [(x, y, -12.8) for x in (-4.3, 4.3) for y in (-3.1, -0.6)])
    V = lo.vertices(obj)
    dirs = lo._directions(2)
    below = V[:, 2] < 0.3
    x = V[:, 0]
    groups = [below & (x < -7.6), below & (x >= -7.9) & (x < -DEN_HALF), below & (x > DEN_HALF) & (x <= 7.9),
              below & (x > 7.6),
              below & (np.abs(x) < DEN_HALF + 0.25) & (V[:, 2] > -7.75),
              below & (np.abs(x) < DEN_HALF + 0.25) & (V[:, 2] <= -7.75) & (V[:, 1] > DEN_BACK - 0.1)]
    # The dome bed by bed, so its collision steps as its beds do (1.7-2 m each, more than a jump) instead of making
    # a ramp up it.
    for lo_z, hi_z in ((-1.5, 2.1), (1.9, 4.0), (3.8, 5.7), (5.5, 7.8)):
        band = (V[:, 2] >= lo_z) & (V[:, 2] <= hi_z)
        groups += [band & (x < 0.4), band & (x > -0.4)]
    for mask in groups:
        if mask.sum() >= 4:
            hull(obj, lo.extremes(V[mask], dirs))
    for points in extra:
        hull(obj, lo.extremes(np.asarray(points), dirs))


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
