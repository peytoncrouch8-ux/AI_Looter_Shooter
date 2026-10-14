"""The freestanding outcrop kit for Ransom's Rest, Den Rock with the Gravemother's den, and the far rocks: rocks of the
cliff kit's layered rock (RockCliff, moss on up-facing rock). A scripted model (see Art/README.md); built as solids with
Tools/Blender/looter_outcrops.py.

  piece            use (Docs/Areas/RansomsRest.md; level.cliffs.outcrops gives a knob another piece)
  Outcrop_TorA     a rounded tor, 4.6 m: three worn pillows of the beds, undercut: the Rimrock, the bench; at 0.65
                   the bluff top's 3 m cover rocks
  Outcrop_TorB     a split tor, 9.8 m: two pillars, one cleft, a cap rock: the Keeper's Gate's north rock (at 0.7),
                   Hearse Rock's head, the bench
  Outcrop_TorC     a leaning stack, 7 m, its top overhanging: the bench
  Outcrop_TorD     a split tor, 4.6 m: a block broken along one joint, its outer half slumped away: the Keeper's Gate's
                   south rock, the Rimrock's north rock, the bench's cover
  Outcrop_Spine    a 12 m whaleback, 5.8 m high, its crest rising and falling: the Rimrock, Hearse Rock, the bench
  Outcrop_Slab     a leaning monolith, 7.5 m, 14 degrees off upright: the Three Widows (three, turned), the bench
  Outcrop_Fin      a rock fin, 9 m, its crest stepping down to both ends: a Stone Tooth
  Outcrop_FinB     a thicker fin, 7.6 m, two summits and a window: a Stone Tooth
  Outcrop_HoodooA  a hoodoo, 8.4 m, under a cap rock: a Stone Tooth, the bench
  Outcrop_HoodooB  twin hoodoos, 6.2 m, each capped: a Stone Tooth, the bench
  DenRock          the dome on the Sink's east rim, its face the Sink's wall with the den at its foot (see den_rock)
  FarRock_A..D     low-poly rocks (about 170 triangles) for the ridges past the boundary

The kit (reworked 2026-10-14, the cliff review: the first kit's prisms between straight joints read as stacked boxes
with regular slits, its slab pile as dominoes; look sheet 45) is weathered sandstone: rounded bodies of the beds (Body:
a column or a fin, its girth set by height, its crest along it; Loaf: a pillow of rock) fused into one mass, the soft
beds washed back into undercuts and necks (partings), caps of harder rock, a few clefts, wind hollows (tafoni) and rain
flutes, rounded blocks fallen at the foot. Den Rock keeps the first kit's beds and columns (and its own outside, fused).
Each kit piece is finished on all sides, so any turn works, and reaches 0.9 m below its pivot (on the ground, in the
middle of the footprint) to seat on a slope. Nanite sources of 7-11k triangles with a Fallback bringing each to 2-3k.
Collision: convex column hulls round each mass, so the only faces a player could stand on are the tops, out of reach
(a jump reaches about 1 m; the lowest is TorD's at 2.3 m); fallen blocks big enough to matter get a hull of their own.

Args after '--' naming builders (TorA, Spine, DenRock, FarRocks, ...) build only those, for quick looks.
"""
import json
import math
import os
import random
import sys

import numpy as np

import looter_outcrops as lo
import looter_textures as lt

SINK = 0.9
ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []


# --- Helpers (Den Rock's too) ---

def centroid(poly):
    return sum(p[0] for p in poly) / len(poly), sum(p[1] for p in poly) / len(poly)


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


# --- Weathered forms (the kit since the cliff review, 2026-10-14) ---
#
# The first kit (2026-10-02) stacked the beds as prisms cut by straight joints, so its tors read as stacked boxes with
# regular vertical slits, and its slab pile as dominoes (the user's look sheet 3; the cliff review's K_ views). Sandstone
# out in the weather wears round instead: its hard beds stand out as rounded ledges and caps, its soft beds wash back
# into undercuts and necks, joints open into a few deep clefts rather than a grid, and wind scoops hollows (tafoni) in
# the soft faces. So the kit is now built from rounded bodies: Body (a column of the beds, its girth set by height: necks,
# bulges, caps; or a fin, its crest set along it) and Loaf (a pillow of rock), fused (lo.Rock's smooth union), with the
# soft beds as deep partings and the hollows as Loaf cuts.

class Body:
    """A rounded column of the beds, seen from every side: a plan ellipse of radii rx, ry, sized up its height by profile
    [(z, scale), ...] (smoothed over smooth metres: necks, bulges, a flared foot), its middle drifting by drift
    [(z, dx, dy), ...] (a lean), its outline wandering by lobes [(count, amp, phase, twist), ...] (a lobe's phase turns by
    twist radians a metre up), flat-bottomed at z0 and topped at z1 (or along its length by crest [(x, z), ...], a fin's)
    with the top rounded over r_top. In its own frame turned by yaw and tilt (degrees, about the point (center, z0)). A
    lo.Rock block: lo, hi, blend, matrix, origin, dist."""

    def __init__(self, rx, ry, z0, z1, profile, center=(0.0, 0.0), yaw=0.0, tilt=(0.0, 0.0), drift=(), lobes=(),
                 crest=None, r_top=0.45, r_bottom=0.1, blend=0.35, smooth=0.35):
        self.rx, self.ry, self.z0, self.z1 = float(rx), float(ry), float(z0), float(z1)
        top = max(z1, max(z for _, z in crest)) if crest else z1
        self.zs = np.linspace(z0 - 1.0, top + 1.0, 400)
        step = self.zs[1] - self.zs[0]
        k = max(1, int(round(smooth / step)))

        def table(knots, column):
            values = np.interp(self.zs, [p[0] for p in knots], [p[column] for p in knots])
            padded = np.pad(values, k, mode='edge')
            return np.convolve(padded, np.ones(2 * k + 1) / (2 * k + 1), mode='valid')
        self.s = table(profile, 1)
        self.ds = np.gradient(self.s, self.zs)
        drift = list(drift) or [(z0, 0.0, 0.0)]
        self.dx, self.dy = table(drift, 1), table(drift, 2)
        self.lobes = [tuple(map(float, lobe)) for lobe in lobes]
        if crest:
            xs = np.linspace(crest[0][0], crest[-1][0], 200)
            tops = np.interp(xs, [c[0] for c in crest], [c[1] for c in crest])
            kc = 4
            tops = np.convolve(np.pad(tops, kc, mode='edge'), np.ones(2 * kc + 1) / (2 * kc + 1), mode='valid')
            self.crest = (xs, tops)
        else:
            self.crest = None
        self.r_top, self.r_bottom, self.blend = r_top, r_bottom, blend
        self.origin = np.array([center[0], center[1], z0], np.float64)
        self.matrix = lo.rotation(yaw, tilt)
        reach = float(self.s.max()) * (1.0 + sum(abs(a) for _, a, _, _ in self.lobes))
        ex = self.rx * reach + float(np.abs(self.dx).max())
        ey = self.ry * reach + float(np.abs(self.dy).max())
        local = np.array([(x, y, z) for x in (-ex, ex) for y in (-ey, ey) for z in (-0.2, top - z0 + 0.2)])
        world = local @ self.matrix.T + self.origin
        margin = max(r_top, r_bottom, blend) + 0.35
        self.lo = world.min(axis=0) - margin
        self.hi = world.max(axis=0) + margin

    def dist(self, P):
        L = (P - self.origin) @ self.matrix
        z = L[:, 2] + self.z0
        s = np.interp(z, self.zs, self.s)
        ds = np.interp(z, self.zs, self.ds)
        x = L[:, 0] - np.interp(z, self.zs, self.dx)
        y = L[:, 1] - np.interp(z, self.zs, self.dy)
        u, v = x / self.rx, y / self.ry
        rho = np.sqrt(u * u + v * v)
        theta = np.arctan2(v, u)
        lobe = np.ones_like(rho)
        for count, amp, phase, twist in self.lobes:
            lobe = lobe + amp * np.cos(count * theta + phase + twist * z)
        m = min(self.rx, self.ry)
        # Metres across its short axis, the girth's change up the height taken out (an underestimate elsewhere).
        side = (rho - s * lobe) * m / np.sqrt(1.0 + (m * ds) ** 2)
        top = self.z1 if self.crest is None else np.interp(L[:, 0], *self.crest)
        d = lo.smax(side, z - top, self.r_top)
        return lo.smax(d, self.z0 - z, self.r_bottom)


class Loaf:
    """A pillow of rock: a superellipsoid of half sizes a, b, c (exponent n: 2 an ellipsoid, 4 a rounded box) about
    center, turned by yaw and tilt. A lo.Rock block (or a cut: a hollow)."""

    def __init__(self, a, b, c, center=(0.0, 0.0, 0.0), yaw=0.0, tilt=(0.0, 0.0), n=3.0, blend=0.3):
        self.a, self.b, self.c, self.n = float(a), float(b), float(c), float(n)
        self.origin = np.array(center, np.float64)
        self.matrix = lo.rotation(yaw, tilt)
        self.blend = blend
        local = np.array([(x, y, z) for x in (-a, a) for y in (-b, b) for z in (-c, c)])
        world = local @ self.matrix.T + self.origin
        margin = blend + 0.35
        self.lo = world.min(axis=0) - margin
        self.hi = world.max(axis=0) + margin
        # For corners() and hulls: the box it fills, in its frame (z from its bottom).
        self.poly = lo.rect(a, b)
        self.z0, self.z1 = -c, c

    def dist(self, P):
        L = (P - self.origin) @ self.matrix
        q = (np.abs(L[:, 0] / self.a) ** self.n + np.abs(L[:, 1] / self.b) ** self.n
             + np.abs(L[:, 2] / self.c) ** self.n) ** (1.0 / self.n)
        return (q - 1.0) * min(self.a, self.b, self.c)


def loaf_corners(loaf, shrink=0.85):
    """A loaf's box corners in the rock's space, a little in (for a hull of its own)."""
    a, b, c = loaf.a * shrink, loaf.b * shrink, loaf.c * shrink
    local = np.array([(x, y, z) for x in (-a, a) for y in (-b, b) for z in (-c, c)])
    return local @ loaf.matrix.T + loaf.origin


def weathered(seed, sink=SINK, **extra):
    """The kit's solid: lo.Rock with sandstone's weathering (lumps, rain runnels, a gentle warp so no outline is
    drawn with a compass)."""
    options = dict(lumps=(0.26, 2.6), detail=(0.065, 1.0), grain=(0.024, 0.42), rain=0.05, warp=(0.35, 4.5), fold=0.12)
    options.update(extra)
    return lo.Rock(seed=seed, sink=sink, **options)


def boulders(rock, rnd, spots, center=(0.0, 0.0), z=0.0):
    """Rounded blocks fallen at the foot: (angle, distance past the rock's side, size) each, half buried and tipped.
    Returns the big ones' hull points (the player steps over the small ones)."""
    hulls = []
    for angle, out, size in spots:
        r = foot(rock, center[0], center[1], angle, z + 0.3)
        a = math.radians(angle)
        x, y = center[0] + (r + out + size * 0.3) * math.cos(a), center[1] + (r + out + size * 0.3) * math.sin(a)
        loaf = Loaf(size * 0.55, size * rnd.uniform(0.38, 0.48), size * rnd.uniform(0.3, 0.38),
                    center=(x, y, z + size * 0.08), yaw=rnd.uniform(0.0, 180.0),
                    tilt=(rnd.uniform(-18.0, 18.0), rnd.uniform(-18.0, 18.0)), n=rnd.uniform(2.4, 3.2), blend=0.12)
        rock.add(loaf)
        if size >= 0.9:
            hulls.append(loaf_corners(loaf))
    return hulls


def hollow(rock, rnd, center, angle, z, size, depth=0.5):
    """A tafoni hollow: a rounded scoop size across (metres) into the side facing angle (degrees) at height z, depth
    into it, its roof overhanging (the soft rock under a hard bed goes first)."""
    r = foot(rock, center[0], center[1], angle, z)
    if r < 0.3:
        lo.log(f'hollow at {angle} degrees, {z} m: no rock there')
        return
    a = math.radians(angle)
    x, y = center[0] + (r + depth * 0.2) * math.cos(a), center[1] + (r + depth * 0.2) * math.sin(a)
    rock.cut(Loaf(size * 0.5, depth, size * rnd.uniform(0.3, 0.4), center=(x, y, z), yaw=angle + 90.0,
                  tilt=(rnd.uniform(-8.0, 8.0), 0.0), n=2.3, blend=0.12))


def lobes(rnd, count=3):
    """A body's outline wandering: a few lobes of their own phase and twist."""
    return [(k, rnd.uniform(0.03, 0.07), rnd.uniform(0.0, 6.28), rnd.uniform(-0.25, 0.25))
            for k in range(2, 2 + count)]


# --- The kit ---

def tor_a():
    """A rounded tor, 4.6 m: three pillows of the beds, each set back and turned on the one below, worn round; the soft
    beds between them washed back into overhangs with hollows under them, one cleft down its side (no grid of joints).
    The Rimrock's and the bench's tors, and (at 0.65) the bluff top's 3 m cover rocks: steep and solid all round."""
    rnd = random.Random(5101)
    rock = weathered(5101)
    rock.add(Loaf(3.5, 2.85, 1.75, center=(0.0, 0.0, 0.55), yaw=6.0, n=3.4, blend=0.3))
    rock.add(Loaf(1.9, 1.7, 1.3, center=(2.35, 1.0, 0.45), yaw=34.0, n=3.0, blend=0.45))          # a buttress
    rock.add(Loaf(2.75, 2.25, 1.15, center=(0.35, -0.25, 2.65), yaw=-9.0, tilt=(2.0, -3.0), n=3.2, blend=0.3))
    rock.add(Loaf(1.55, 1.35, 0.72, center=(-0.6, 0.3, 3.9), yaw=21.0, tilt=(-4.0, 3.0), n=3.0, blend=0.22))
    rock.parting(1.55, depth=0.42, width=0.26, fade=0.2)
    rock.parting(3.35, depth=0.3, width=0.2, fade=0.0)
    rock.parting(0.55, depth=0.16, width=0.12, fade=-0.2)
    for angle, z, size in ((205.0, 1.75, 1.3), (300.0, 1.7, 0.9), (60.0, 3.2, 0.8)):
        hollow(rock, rnd, (0.0, 0.0), angle, z, size, depth=0.45)
    face_crack(rock, (0.0, 0.0), 250.0, 2.0, 70.0, length=3.4, width=0.14, depth=0.5, shift=0.06)
    extra = boulders(rock, rnd, [(215.0, 0.2, 1.3), (232.0, 0.9, 0.55), (40.0, 0.3, 0.8), (130.0, 0.1, 0.5)])
    obj = build('Outcrop_TorA', rock, cell=0.07, source=9000, fallback=30, uv_seed=11)
    lo.column_hulls(obj, [(-1.9, -1.1), (1.7, -1.3), (-1.3, 1.4), (2.2, 1.2)], extra=extra)
    return obj


def flutes(rnd, count=9):
    """Rain flutes: fine vertical ribs down a column (a lobe of high count, barely turning)."""
    return [(count, rnd.uniform(0.02, 0.03), rnd.uniform(0.0, 6.28), rnd.uniform(-0.03, 0.03))]


def drop_crumbs(obj, smallest=0.35):
    """Removes loose bits of the mesh under smallest metres across: the weathering can pinch a crumb off a thin top.
    The fallen boulders, bigger, stay. Faces are walked in index order (sequences, never sets), so it repeats."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    seen = [False] * len(bm.faces)
    doomed = []
    for face in bm.faces:
        if seen[face.index]:
            continue
        seen[face.index] = True
        stack, part = [face], []
        while stack:
            f = stack.pop()
            part.append(f)
            for edge in f.edges:
                for g in edge.link_faces:
                    if not seen[g.index]:
                        seen[g.index] = True
                        stack.append(g)
        points = np.array([v.co[:] for f in part for v in f.verts])
        if np.linalg.norm(points.max(axis=0) - points.min(axis=0)) < smallest:
            doomed.extend(part)
    if doomed:
        bmesh.ops.delete(bm, geom=doomed, context='FACES')
        lo.log(f'{obj.name}: {len(doomed)} faces of loose crumbs removed')
    bm.to_mesh(obj.data)
    bm.free()
    lo.refresh(obj)


def build(name, rock, **options):
    """lo.build, then the crumbs dropped."""
    obj = lo.build(name, rock, **options)
    drop_crumbs(obj)
    return obj


def tor_b():
    """A split tor, 9.8 m: two pillars on one worn base, parted by a single cleft that opens upward; their girth swells
    and narrows a little with the beds, the taller one under a cap rock that overhangs it, the shorter one broken off
    and rounded. The Keeper's Gate's north rock, the bench's tall tors, Hearse Rock's head."""
    rnd = random.Random(5202)
    rock = weathered(5202)
    rock.add(Loaf(2.9, 2.35, 1.5, center=(0.0, 0.0, 0.35), yaw=8.0, n=3.2, blend=0.35))
    rock.add(Body(1.3, 1.5, -0.5, 9.0, [(-0.5, 1.2), (1.6, 1.06), (2.8, 0.9), (4.4, 0.97), (5.9, 0.86), (7.3, 0.9),
                                        (8.3, 0.8), (9.0, 0.74)],
                  center=(-1.2, 0.1), drift=[(0.0, 0.0, 0.0), (4.5, -0.1, 0.15), (9.0, -0.4, 0.2)],
                  lobes=lobes(rnd) + flutes(rnd), r_top=0.4, blend=0.4))
    # The shorter pillar broken off at a slant, its top falling away from the cleft (a dome read as a capsule).
    rock.add(Body(1.15, 1.3, -0.5, 7.4, [(-0.5, 1.2), (1.4, 1.04), (2.7, 0.9), (4.2, 0.98), (5.6, 0.88), (6.6, 0.9),
                                         (7.6, 0.86)],
                  center=(1.25, -0.15), drift=[(0.0, 0.0, 0.0), (7.4, 0.45, -0.1)], lobes=lobes(rnd) + flutes(rnd),
                  crest=[(-1.6, 7.7), (-0.4, 7.5), (0.6, 6.9), (1.6, 6.3)], r_top=0.3, blend=0.4))
    rock.add(Loaf(1.55, 1.7, 0.55, center=(-1.55, 0.35, 9.3), yaw=14.0, tilt=(5.0, -6.0), n=3.6, blend=0.14))
    for z, depth, width in ((2.75, 0.24, 0.3), (5.9, 0.14, 0.32), (8.4, 0.12, 0.2), (1.3, 0.1, 0.12)):
        rock.parting(z, depth=depth, width=width, fade=0.1)
    hollow(rock, rnd, (-1.2, 0.0), 150.0, 3.6, 1.0, depth=0.4)
    hollow(rock, rnd, (1.25, 0.0), 330.0, 4.9, 0.8, depth=0.35)
    extra = boulders(rock, rnd, [(100.0, 0.3, 1.25), (118.0, 1.1, 0.5), (282.0, 0.2, 0.9), (300.0, 0.9, 0.45)])
    obj = build('Outcrop_TorB', rock, cell=0.07, source=10000, fallback=28, uv_seed=12)
    lo.column_hulls(obj, [(-1.35, 0.0), (1.45, 0.0)], extra=extra)
    return obj


def tor_c():
    """A leaning stack, 7 m: a column of the beds leaning as it rises, its hard beds rounded ledges over washed-back soft
    ones, the top overhanging; a short stump fused at its foot."""
    rnd = random.Random(5303)
    rock = weathered(5303)
    rock.add(Body(1.85, 1.55, -0.9, 6.9, [(-0.9, 1.18), (1.0, 1.0), (2.0, 0.86), (3.1, 0.97), (4.1, 0.88), (5.2, 1.0),
                                          (6.1, 0.9), (6.9, 0.96)],
                  center=(0.3, 0.0), drift=[(0.0, 0.0, 0.0), (3.0, -0.45, 0.1), (6.9, -1.55, 0.35)],
                  lobes=lobes(rnd) + flutes(rnd, 8), r_top=0.55, blend=0.35))
    rock.add(Body(1.2, 1.05, -0.9, 2.3, [(-0.9, 1.15), (1.2, 0.95), (2.3, 0.85)], center=(1.95, 0.9),
                  lobes=lobes(rnd, 2), r_top=0.7, blend=0.5))
    for z, depth, width in ((2.0, 0.26, 0.3), (4.1, 0.2, 0.28), (6.1, 0.16, 0.22)):
        rock.parting(z, depth=depth, width=width, fade=0.1)
    hollow(rock, rnd, (0.0, 0.0), 210.0, 3.4, 0.9, depth=0.35)
    face_crack(rock, (0.0, 0.0), 20.0, 1.2, 64.0, length=2.6, width=0.12, depth=0.4)
    extra = boulders(rock, rnd, [(165.0, 0.3, 1.2), (190.0, 1.0, 0.5), (330.0, 0.2, 0.8), (60.0, 0.6, 0.55)])
    obj = build('Outcrop_TorC', rock, cell=0.07, source=9000, fallback=30, uv_seed=13)
    for z_range in ((-SINK, 3.8), (3.1, 5.6), (4.8, 7.6)):
        lo.column_hulls(obj, [(0.0, 0.0)], z_range=z_range)
    lo.column_hulls(obj, [], extra=extra)
    return obj


def tor_d():
    """A split tor, 4.6 m: a rounded block of the beds broken in two along one joint, the outer half slumped 14 degrees
    away from the other, so the cleft between them opens upward; its beds tip with it, rounded blocks at its foot. Low
    and solid: the Keeper's Gate's south rock (the lookout sees the deck over it), the Rimrock's north rock, the bench's
    cover. (The first kit's TorD was three slabs tipped like dominoes against a stub.)"""
    rnd = random.Random(5404)
    rock = weathered(5404, warp=(0.3, 4.0))
    standing = [rock.add(Loaf(1.75, 2.4, 1.85, center=(-1.5, 0.0, 1.0), yaw=4.0, n=3.3, blend=0.3)),
                rock.add(Loaf(1.35, 1.95, 0.85, center=(-1.65, 0.15, 3.35), yaw=-6.0, tilt=(3.0, -2.0), n=3.0,
                              blend=0.25))]
    slumped = [rock.add(Loaf(1.6, 2.3, 1.75, center=(1.6, 0.05, 0.85), yaw=-5.0, tilt=(-3.0, 14.0), n=3.3,
                             blend=0.3)),
               rock.add(Loaf(1.15, 1.7, 0.72, center=(2.2, 0.0, 2.95), yaw=9.0, tilt=(-3.0, 16.0), n=3.0, blend=0.25))]
    rock.parting(2.75, depth=0.22, width=0.24, fade=0.1)
    rock.parting(0.6, depth=0.12, width=0.12, fade=-0.1)
    hollow(rock, rnd, (-1.5, 0.0), 180.0, 1.6, 1.0, depth=0.4)
    hollow(rock, rnd, (1.6, 0.0), 300.0, 1.2, 0.8, depth=0.35)
    face_crack(rock, (1.6, 0.0), 0.0, 1.8, 58.0, length=2.4, width=0.1, depth=0.35)
    extra = boulders(rock, rnd, [(0.0, 0.3, 1.0), (25.0, 1.0, 0.55), (300.0, 0.4, 0.7), (150.0, 0.2, 0.6),
                                 (205.0, 0.3, 0.85)], z=0.0)
    parts = standing + slumped
    frames = frames_of(parts + rock.blocks[len(parts):], rnd)
    obj = build('Outcrop_TorD', rock, cell=0.065, source=9500, fallback=30, uv_seed=14, frames=frames)
    lo.block_hulls(obj, rock, [standing, slumped])
    lo.column_hulls(obj, [], extra=extra)
    return obj


def spine():
    """A rock spine, 12 m long and 5.8 m high: a whaleback of the beds whose crest rises and falls (a saddle, a high
    knob, a long fall to its ends), a soft bed washed back along its foot into an overhang, hollows in its sides."""
    rnd = random.Random(5505)
    rock = weathered(5505)
    rock.add(Body(6.2, 2.05, -0.9, 5.8, [(-0.9, 1.06), (1.2, 1.0), (2.0, 0.9), (3.0, 0.94), (4.0, 0.84), (4.9, 0.74),
                                         (5.8, 0.56)],
                  crest=[(-6.6, 1.6), (-5.4, 3.4), (-3.6, 4.9), (-2.0, 5.1), (-0.9, 4.3), (0.6, 5.8), (2.2, 5.6),
                         (3.8, 4.4), (5.3, 3.5), (6.6, 1.5)],
                  drift=[(0.0, 0.0, 0.0), (5.8, 0.0, 0.3)], lobes=lobes(rnd), r_top=0.55, blend=0.35))
    rock.add(Body(1.2, 1.1, -0.9, 5.9, [(-0.9, 1.15), (2.0, 0.92), (3.4, 0.86), (4.6, 0.9), (5.9, 0.72)],
                  center=(1.1, 0.2), lobes=lobes(rnd, 2) + flutes(rnd, 7), r_top=0.6, blend=0.6))
    for z, depth, width in ((2.0, 0.34, 0.32), (3.4, 0.14, 0.24), (4.6, 0.12, 0.18)):
        rock.parting(z, depth=depth, width=width, fade=0.1)
    for x, angle, z, size in ((-3.0, 270.0, 2.6, 1.1), (2.6, 270.0, 3.2, 0.9), (-1.0, 90.0, 2.2, 1.2),
                              (3.8, 90.0, 1.4, 0.8)):
        hollow(rock, rnd, (x, 0.0), angle, z, size, depth=0.4)
    extra = boulders(rock, rnd, [(250.0, 0.3, 1.3), (262.0, 1.2, 0.5), (75.0, 0.3, 1.0), (98.0, 1.0, 0.6),
                                 (180.0, 0.2, 0.8)], center=(0.0, 0.0))
    obj = build('Outcrop_Spine', rock, cell=0.075, source=11000, fallback=25, uv_seed=15)
    lo.column_hulls(obj, [(-4.6, 0.0), (-1.6, 0.0), (1.5, 0.0), (4.6, 0.0)], extra=extra)
    return obj


def slab():
    """A leaning monolith, 7.5 m, 14 degrees off upright (the Three Widows: three, turned): one tapering slab of the
    beds, flared at its foot, a little waisted at a soft bed, its shoulders falling to a rounded, hooded head. Built
    upright and mapped, then leaned, so its beds lean with it."""
    rnd = random.Random(5606)
    rock = weathered(5606, sink=1.7)
    rock.add(Body(1.75, 0.88, -1.7, 6.9, [(-1.7, 1.12), (0.4, 1.0), (1.6, 0.9), (3.0, 0.96), (4.3, 0.9), (5.3, 0.78),
                                          (6.1, 0.7), (6.9, 0.5)],
                  drift=[(0.0, 0.0, 0.0), (6.9, -0.2, 0.0)], lobes=lobes(rnd) + flutes(rnd, 7), r_top=0.85,
                  blend=0.3))
    rock.add(Loaf(1.0, 0.72, 0.6, center=(-0.25, -0.1, 6.25), yaw=-8.0, tilt=(-6.0, 4.0), n=2.8, blend=0.45))
    for z, depth, width in ((1.6, 0.2, 0.3), (5.3, 0.14, 0.24), (3.1, 0.08, 0.1)):
        rock.parting(z, depth=depth, width=width, fade=0.15)
    hollow(rock, rnd, (0.0, 0.0), 270.0, 2.2, 0.9, depth=0.3)
    lean = lo.Matrix.Rotation(math.radians(-14.0), 4, 'X')
    obj = build('Outcrop_Slab', rock, cell=0.06, source=7000, fallback=30, uv_seed=16, matrix=lean)
    for z_range in ((-2.0, 3.0), (2.4, 5.4), (4.8, 7.6)):
        lo.column_hulls(obj, [(0.0, 0.0)], z_range=z_range)
    return obj


def fin_body(rnd, length, height, thick, crest, profile):
    return Body(length * 0.5, thick, -SINK, height, profile, crest=crest,
                drift=[(0.0, 0.0, 0.0), (height, rnd.uniform(-0.3, 0.3), rnd.uniform(-0.25, 0.25))],
                lobes=lobes(rnd) + flutes(rnd, 11), r_top=0.38, blend=0.35)


def fin():
    """A rock fin, 9 m (the Stone Teeth): a thin wall of the beds, thick at its foot, its crest rising to a high point
    off its middle and falling in rounded steps to both ends; a soft bed washed back along its foot, hollows."""
    rnd = random.Random(5707)
    rock = weathered(5707)
    rock.add(fin_body(rnd, 7.4, 9.0, 1.0,
                      [(-3.9, 3.0), (-3.2, 4.6), (-2.5, 6.6), (-1.6, 7.6), (-0.9, 8.3), (-0.2, 9.0), (0.5, 8.7),
                       (1.1, 7.6), (1.8, 7.0), (2.6, 6.3), (3.2, 4.6), (3.9, 2.6)],
                      [(-0.9, 1.12), (1.2, 1.0), (2.2, 0.9), (3.4, 0.95), (4.8, 0.86), (5.8, 0.8), (6.8, 0.82),
                       (7.8, 0.68), (9.0, 0.52)]))
    for z, depth, width in ((2.2, 0.28, 0.3), (5.8, 0.14, 0.26), (7.8, 0.12, 0.18), (1.0, 0.08, 0.1)):
        rock.parting(z, depth=depth, width=width, fade=0.1)
    for x, angle, z, size in ((-1.6, 270.0, 3.6, 1.1), (1.4, 270.0, 1.6, 0.8), (0.2, 90.0, 4.4, 1.0)):
        hollow(rock, rnd, (x, 0.0), angle, z, size, depth=0.32)
    extra = boulders(rock, rnd, [(280.0, 0.3, 0.9), (300.0, 0.9, 0.45), (95.0, 0.3, 0.7), (10.0, 0.4, 0.6)])
    obj = build('Outcrop_Fin', rock, cell=0.065, source=8000, fallback=30, uv_seed=17)
    lo.column_hulls(obj, [(-2.4, 0.0), (0.0, 0.0), (2.4, 0.0)], extra=extra)
    return obj


def fin_b():
    """A broken fin, 7.6 m (one of the Stone Teeth): shorter and thicker, two summits with a saddle between, and a
    window worn through it under the saddle."""
    rnd = random.Random(5808)
    rock = weathered(5808)
    rock.add(fin_body(rnd, 6.6, 7.6, 1.2,
                      [(-3.6, 2.6), (-2.8, 5.6), (-1.9, 7.2), (-1.0, 6.9), (0.1, 5.4), (0.9, 6.1), (1.8, 7.6),
                       (2.6, 6.8), (3.6, 3.0)],
                      [(-0.9, 1.12), (1.0, 1.0), (1.9, 0.88), (3.0, 0.96), (4.2, 0.88), (5.2, 0.8), (6.4, 0.84),
                       (7.6, 0.62)]))
    rock.cut(Loaf(0.85, 2.2, 1.05, center=(-0.35, 0.0, 3.55), yaw=0.0, tilt=(0.0, 8.0), n=2.2, blend=0.3))
    for z, depth, width in ((1.9, 0.28, 0.3), (5.2, 0.14, 0.24), (0.8, 0.08, 0.1)):
        rock.parting(z, depth=depth, width=width, fade=0.1)
    hollow(rock, rnd, (1.7, 0.0), 270.0, 2.4, 0.9, depth=0.32)
    hollow(rock, rnd, (-2.2, 0.0), 90.0, 3.0, 0.8, depth=0.3)
    extra = boulders(rock, rnd, [(260.0, 0.3, 1.0), (75.0, 0.4, 0.75), (190.0, 0.3, 0.5)])
    obj = build('Outcrop_FinB', rock, cell=0.065, source=8000, fallback=30, uv_seed=18)
    lo.column_hulls(obj, [(-2.2, 0.0), (2.0, 0.0)], extra=extra)
    return obj


def hoodoo_a():
    """A hoodoo, 8.4 m: a column of the beds on a broad, flared foot, tapering as it rises, pinched at two soft beds and
    thinnest under a cap rock that overhangs it a little off its middle; rain flutes down it."""
    rnd = random.Random(5909)
    rock = weathered(5909)
    rock.add(Body(1.45, 1.25, -0.9, 7.35, [(-0.9, 1.8), (0.2, 1.42), (1.2, 1.1), (2.4, 0.9), (3.3, 0.94), (4.4, 0.8),
                                           (5.4, 0.82), (6.3, 0.7), (6.9, 0.56), (7.35, 0.5)],
                  drift=[(0.0, 0.0, 0.0), (2.5, 0.2, 0.1), (5.0, -0.05, 0.0), (7.35, 0.3, -0.15)],
                  lobes=lobes(rnd) + [(1, 0.06, 1.0, 0.3)] + flutes(rnd, 9), r_top=0.3, blend=0.3))
    rock.add(Loaf(1.45, 1.3, 0.55, center=(0.45, -0.25, 7.75), yaw=17.0, tilt=(3.0, -5.0), n=3.6, blend=0.14))
    for z, depth, width in ((2.4, 0.12, 0.36), (4.4, 0.16, 0.36), (6.9, 0.14, 0.22), (0.9, 0.08, 0.12)):
        rock.parting(z, depth=depth, width=width, fade=0.2)
    hollow(rock, rnd, (0.0, 0.0), 230.0, 1.2, 0.9, depth=0.35)
    extra = boulders(rock, rnd, [(140.0, 0.4, 1.1), (160.0, 1.3, 0.5), (320.0, 0.3, 0.7), (40.0, 0.8, 0.5)])
    obj = build('Outcrop_HoodooA', rock, cell=0.06, source=8000, fallback=30, uv_seed=19)
    lo.column_hulls(obj, [(0.0, 0.0)], z_range=(-SINK, 7.2))
    lo.column_hulls(obj, [(0.45, -0.25)], z_range=(6.9, 8.6), extra=extra)
    return obj


def hoodoo_b():
    """Twin hoodoos, 6.2 m: a tall and a short column rising together from one worn foot, each under its own cap rock
    (the short one's tipped), a soft bed pinching both at the same height."""
    rnd = random.Random(6010)
    rock = weathered(6010)
    rock.add(Loaf(2.6, 1.9, 1.15, center=(0.3, 0.0, 0.0), yaw=-12.0, n=3.0, blend=0.45))
    rock.add(Body(1.25, 1.1, -0.5, 5.4, [(-0.5, 1.3), (1.4, 1.04), (2.5, 0.84), (3.4, 0.9), (4.4, 0.76), (5.0, 0.64),
                                         (5.4, 0.58)],
                  center=(-0.75, 0.1), drift=[(0.0, 0.0, 0.0), (5.4, -0.2, 0.1)], lobes=lobes(rnd) + flutes(rnd, 8),
                  r_top=0.3, blend=0.4))
    rock.add(Loaf(1.25, 1.1, 0.48, center=(-0.95, 0.15, 5.75), yaw=-25.0, tilt=(-4.0, 3.0), n=3.4, blend=0.14))
    rock.add(Body(1.1, 1.0, -0.5, 3.9, [(-0.5, 1.25), (1.3, 1.02), (2.5, 0.86), (3.3, 0.8), (3.9, 0.72)],
                  center=(1.45, -0.2), drift=[(0.0, 0.0, 0.0), (3.9, 0.25, -0.1)], lobes=lobes(rnd) + flutes(rnd, 7),
                  r_top=0.3, blend=0.4))
    rock.add(Loaf(0.95, 0.85, 0.36, center=(1.75, -0.3, 4.1), yaw=30.0, tilt=(6.0, 9.0), n=3.4, blend=0.12))
    for z, depth, width in ((2.5, 0.2, 0.34), (4.4, 0.14, 0.24), (1.0, 0.08, 0.1)):
        rock.parting(z, depth=depth, width=width, fade=0.2)
    extra = boulders(rock, rnd, [(250.0, 0.3, 0.9), (80.0, 0.4, 0.6), (180.0, 0.6, 0.45)])
    obj = build('Outcrop_HoodooB', rock, cell=0.06, source=8000, fallback=30, uv_seed=20)
    lo.column_hulls(obj, [(-0.8, 0.1), (1.45, -0.2)], z_range=(-SINK, 5.5))
    lo.column_hulls(obj, [(-0.95, 0.15)], z_range=(5.2, 6.4), extra=extra)
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
# Where Den Rock keeps its first build (DenBlend): the den, its mouth with the rim round it, the joint above the arch
# that the mouth's outline (den_mouth) runs into, and the threshold with its stones; beyond these the reworked outside
# (den_exterior) takes over across DEN_BLEND metres.
DEN_KEEP = ((np.array([-4.55, 0.3, -13.0]), np.array([4.55, 11.6, -5.3])),       # the den behind its face
            (np.array([-4.55, -2.2, -13.0]), np.array([4.55, 0.3, -6.3])),       # the mouth and its broken rim
            (np.array([-1.75, -2.2, -13.0]), np.array([-0.65, 0.3, -5.3])),      # the joint the mouth's outline enters
            (np.array([-4.95, -3.5, -13.0]), np.array([4.95, 0.0, -11.0])))      # the threshold and its stones
DEN_BLEND = 1.0


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


class DenBlend:
    """Den Rock's solid: its first build (den, a DenRockSolid) inside DEN_KEEP, the reworked outside (den_exterior)
    beyond it, the two fields blended across DEN_BLEND metres. Inside DEN_KEEP the field is the first build's exactly,
    so the den, its mouth (den_mouth's outline) and the face round it stay as they are. Everything but the field and
    the bounds (the cave, the warp, the rubble) is the first build's."""

    def __init__(self, den, outside):
        object.__setattr__(self, 'den', den)
        object.__setattr__(self, 'outside', outside)

    def __getattr__(self, name):
        return getattr(self.den, name)

    def __setattr__(self, name, value):
        if name == 'with_rubble':
            self.den.with_rubble = value        # the den's checks switch its rubble off and on
        else:
            object.__setattr__(self, name, value)

    def weight(self, P):
        """0 inside DEN_KEEP, rising to 1 at DEN_BLEND from it."""
        far = None
        for lo_, hi_ in DEN_KEEP:
            d = np.linalg.norm(np.maximum(np.maximum(lo_ - P, P - hi_), 0.0), axis=1)
            far = d if far is None else np.minimum(far, d)
        return lo.smoothstep(0.0, DEN_BLEND, far)

    def field(self, P):
        P = np.asarray(P, np.float64)
        w = self.weight(P)
        d = np.zeros(len(P))
        first, second = w < 1.0, w > 0.0
        if first.any():
            d[first] = self.den.field(P[first])
        if second.any():
            e = self.outside.field(P[second])
            mix = w[second]
            d[second] = np.where(first[second], (1.0 - mix) * d[second] + mix * e, e)
        return d

    def bounds(self, margin=0.6):
        lo_a, hi_a = self.den.bounds(margin)
        lo_b, hi_b = self.outside.bounds(margin)
        if np.all(lo_b >= lo_a) and np.all(hi_b <= hi_a):
            return lo_a, hi_a                   # the first build's grid, so the den's surface samples as before
        return np.minimum(lo_a, lo_b), np.maximum(hi_a, hi_b)


def den_rock():
    """Den Rock: a dome of the beds on the Sink's east rim. Its west face is the Sink's wall, a 19 m face from the
    floor (z = -12) up past the rim into the dome, curved on the Sink's circle; at its foot, under a brow of the middle
    bed, the Gravemother's den, sized for her to walk in and out: a cave mouth 7.7 m wide at the floor and 7 m at 2 m
    up, its broken arch rising to 4.5 m clear on its left (its rim near 5 m on the face) and 4 m on its right, opening
    into a chamber over 7 m wide all the way back, whose walls bulge in and out and whose roof comes down toward the
    back, widening to a pocket 8.4 m across and 3.8 m high where she rests (SOCKET_Den) that rounds off 9.3 m in.
    Its floor is flat, walkable trodden dirt (DenFloor), clear 6 m wide down the middle, with rubble along the walls
    and on the threshold's corners. Inside it the rock is RockCliffDen: no moss, a little darker, growing dim toward
    the back. Round the den and its mouth the rock is one weathered mass (den_exterior, blended in by DenBlend): beds
    with their own setbacks, fused, joints only short shallow cracks within a bed. The pivot is at rim level on the lip
    above the mouth; the mouth faces -Y (the Sink's centre is 18 m out that way). Writes the mouth's outline for the
    Sink's web funnel (den_mouth)."""
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
    closing = rock.add(lo.Block(lo.rect(6.4, 3.9), -12.8, -4.0, center=(0.0, 6.9), r_side=0.3, r_top=0.4, blend=0.3))
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
    # Beyond the den and the face round its mouth, the outside rebuilt as one weathered mass: its first build's columns
    # stood apart as boxes, split by slots running the full height.
    solid = DenBlend(rock, den_exterior(fallen + [threshold, closing, rock.fill], (edges[0], edges[-1])))

    obj = lo.mesh_rock('DenRock', solid, cell=0.1, source=18000)
    lo.finish(obj, fallback=28, uv_seed=18, ao_distance=2.4)
    inside = den_faces(obj, solid)
    lt.assign(obj, den_material(), inside)
    floor = den_floor_faces(obj, solid, inside)
    lt.assign(obj, den_floor_material(), floor)
    lt.box_uv(obj, 'GroundDirt', faces=floor, seed=18)
    lo.darken(obj, lambda P: den_shade(solid, P))
    den_hulls(obj, solid, extra, fallen + [threshold])
    lo.lm.socket(obj, 'Den', (0.0, DEN_BACK - 1.0, den_floor(DEN_BACK - 1.0)))
    lo.lm.socket(obj, 'DenMouth', (0.0, 0.0, DEN_FLOOR[0]))
    den_mouth(solid, os.path.join(lt.REPO, 'Intermediate', 'DenRock', 'den_mouth.json'))
    return obj


def den_exterior(keep, span):
    """Den Rock's outside rebuilt as one weathered mass, for DenBlend beyond DEN_KEEP: the face, beds and dome of
    den_rock's first build, but each bed split at its own irregular joints, so no joint runs from one bed into the next;
    the pieces fused instead of standing apart, their edges worn round, their setbacks varying piece by piece and bed
    by bed. Joints show only as short, shallow cracks that die out within their bed, and a few edges of the dome are
    worn away. keep: blocks of the first build it keeps as they are (the fallen blocks, the threshold, the rock behind
    the den); span: the angles the first build's face ran between."""
    rnd = random.Random(4848)
    rock = lo.Rock(seed=4808, sink=12.8, lumps=(0.26, 4.2), detail=(0.08, 1.4), grain=(0.025, 0.5),
                   warp=(0.5, 7.5), fold=0.3)
    rock.calm_box((-3.7, -3.7, -12.6), (3.7, DEN_BACK + 0.6, -11.4), 0.12)     # den_rock's, so the warp is the same
    for block in keep:
        rock.add(block)

    def pieces(width):
        """Spans of angle along the face for one bed: irregular, its own for every bed, within span; and each piece's
        standing out, wandering from piece to piece (no great steps between neighbours)."""
        edges = [span[0]]
        while edges[-1] < span[1]:
            edges.append(min(edges[-1] + rnd.uniform(*width) / SINK_R, span[1]))
        if len(edges) > 2 and edges[-1] - edges[-2] < 1.5 / SINK_R:
            del edges[-2]                       # no sliver at the end
        outs, out = [], rnd.uniform(-0.3, 0.3)
        for _ in edges[1:]:
            out = min(max(out + rnd.uniform(-0.22, 0.22), -0.35), 0.35)
            outs.append(out)
        return [(t0, t1, o) for t0, t1, o in zip(edges, edges[1:], outs)]

    def face_r(mid, z, out):
        """As den_rock's: standing out by out, leaning back 1.5 degrees, curving back at the ends."""
        end = max(0.0, abs(mid) - 0.42) / 0.2
        return SINK_R - out + 0.026 * (z + 12.0) + 2.6 * min(end, 1.0) ** 1.5

    def add(poly, z0, z1, r_side, r_top, r_bottom):
        if len(poly) < 3 or abs(lo.area2d(poly)) < 0.25:
            return
        cx, cy = centroid(poly)
        rock.add(lo.Block([(x - cx, y - cy) for x, y in poly], z0, z1, center=(cx, cy),
                          tilt=(rnd.uniform(-1.5, 1.5), rnd.uniform(-1.5, 1.5)), r_side=r_side, r_top=r_top,
                          r_bottom=r_bottom, blend=0.45, facets=facets(rnd, rnd.choice((0, 1, 1, 2)), z1 - z0)))

    lap = 0.12 / SINK_R                         # neighbours overlap: fused, not parted by a slot
    # The wall below the rim, its three beds each with its own setback (the top one standing out a little, as before;
    # over the den the middle one is a brow).
    wall = [(-12.8, -7.6, 0.0), (-7.6, -4.9, -0.15), (-4.9, -0.35, 0.25)]
    cuts = []
    for k, (z0, z1, setback) in enumerate(wall):
        spans = pieces((3.0, 5.5))
        for t0, t1, wander in spans:
            mid = (t0 + t1) * 0.5
            out = setback + wander + (0.8 if k == 1 and abs(mid) < 0.33 else 0.0)
            add(sector(t0 - lap, t1 + lap, face_r(mid, z0, out), SINK_R + 4.5), z0 - (0.0 if k == 0 else 0.35),
                z1 + rnd.uniform(0.0, 0.2), r_side=0.4, r_top=0.35, r_bottom=0.5 if k else 0.05)
        # Its short joints: a few of its pieces' meetings, cracked within the bed only.
        for t, _, _ in rnd.sample(spans[1:], min(3, len(spans) - 1)):
            h = z1 - z0
            cuts.append((t, rnd.uniform(z0 + 0.3 * h, z1 - 0.3 * h), h * rnd.uniform(0.28, 0.4)))
    # The dome above the rim: den_rock's beds and rings of loaves, each bed with its own pieces and ring joints.
    rings = [23.5, 28.2, 33.5]
    dome = [(0.0, 2.0, 10.4, 14.5, -4.0), (2.0, 3.9, 9.0, 12.6, -4.0), (3.9, 5.6, 7.0, 9.9, 0.4),
            (5.6, 7.1, 4.6, 6.8, 2.1)]
    for k, (z0, z1, half, east, west) in enumerate(dome):
        outline = ellipse(0.0, (east + west) * 0.5, half, (east - west) * 0.5)
        last = k == len(dome) - 1
        split = [r + rnd.uniform(-0.8, 0.8) for r in rings[:-1]] + rings[-1:]
        for t0, t1, out in pieces((3.5, 6.5)):
            mid = (t0 + t1) * 0.5
            step = rnd.uniform(-0.7, 0.25) if last else 0.0
            radii = [face_r(mid, z0, out) - 0.25] + split
            for j, (r0, r1) in enumerate(zip(radii, radii[1:])):
                poly = clip_to(sector(t0 - lap, t1 + lap, r0, r1 + 0.1), outline)
                if not poly or min_width(poly) < 1.0:
                    continue                    # a sliver clipped off by the outline would stand like a plate
                add(poly, -1.3 if (k == 0 and j > 0) else z0 - 0.3, z1 + step + rnd.uniform(0.0, 0.2), r_side=0.5,
                    r_top=0.95 if last else 0.6 if k else 0.4, r_bottom=0.15)

    for z, depth, width, fade in ((-10.3, 0.3, 0.16, 0.1), (-8.6, 0.2, 0.14, -0.2), (-6.0, 0.26, 0.15, 0.0),
                                  (-3.3, 0.34, 0.16, 0.2), (-1.4, 0.22, 0.14, -0.1), (1.0, 0.24, 0.15, 0.0),
                                  (3.0, 0.22, 0.14, -0.1), (4.7, 0.2, 0.14, -0.15)):
        rock.parting(z, depth=depth, width=width, fade=fade)
    # The face's joints: short, shallow V cracks (lips worn by the weathering) where the face stands at that angle.
    rs = np.arange(SINK_R - 3.0, SINK_R + 4.0, 0.04)
    for t, z, reach in cuts:
        P = np.stack([np.sin(t) * rs, -SINK_R + np.cos(t) * rs, np.full_like(rs, z)], axis=1)
        solid = np.nonzero(rock.field(P) < 0.0)[0]
        if not len(solid):
            continue
        r = float(rs[solid[0]])
        lean = math.radians(rnd.uniform(-8.0, 8.0))
        rock.crack((math.sin(t) * r, -SINK_R + math.cos(t) * r, z),
                   (math.cos(t) * math.cos(lean), -math.sin(t) * math.cos(lean), math.sin(lean)),
                   width=rnd.uniform(0.08, 0.12), depth=rnd.uniform(0.08, 0.15), radius=reach)
    # The dome's: on its loaves' sides, and a few of its edges worn away (rounded, not chipped).
    for k, (z0, z1, _, _, _) in enumerate(dome[:3]):
        for _ in range(2):
            angle = rnd.uniform(-20.0, 200.0)
            z = rnd.uniform(z0 + 0.3, z1 - 0.3)
            r = foot(rock, 0.0, 6.0, angle, z)
            if r < 0.3:
                continue
            a, lean = math.radians(angle), math.radians(rnd.uniform(-8.0, 8.0))
            rock.crack((r * math.cos(a), 6.0 + r * math.sin(a), z),
                       (-math.sin(a) * math.cos(lean), math.cos(a) * math.cos(lean), math.sin(lean)),
                       width=rnd.uniform(0.08, 0.12), depth=rnd.uniform(0.08, 0.15), radius=rnd.uniform(0.6, 0.9))
    for angle, z, size in ((250.0, 6.8, 1.5), (300.0, 5.4, 1.3), (20.0, 5.2, 1.2), (160.0, 5.3, 1.4),
                           (215.0, 3.7, 1.3), (95.0, 3.6, 1.2)):
        r = foot(rock, 0.0, 6.0, angle, z)
        if r < 0.3:
            continue
        a = math.radians(angle)
        rock.cut(lo.Block(lo.rect(size * 0.55, size * 0.45), z - size * 0.5, z + size * 0.5,
                          center=(r * math.cos(a), 6.0 + r * math.sin(a)), yaw=angle + rnd.uniform(-30.0, 30.0),
                          tilt=(rnd.uniform(-25.0, 25.0), rnd.uniform(-25.0, 25.0)), r_side=size * 0.35,
                          r_top=size * 0.35, r_bottom=size * 0.3, blend=0.25))
    return rock


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
    'FinB': fin_b,
    'HoodooA': hoodoo_a,
    'HoodooB': hoodoo_b,
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
    by = {m.name: m for m in models}
    kit = [f'Outcrop_{n}' for n in KIT]
    if all(n in by for n in kit):
        # The kit at one scale, for the look sheet (Tools/Blender/rocks_sheet.py), and its numbers.
        rows = [[(by[f'Outcrop_{n}'], 0.0) for n in ('TorA', 'TorD', 'TorC', 'HoodooB', 'Slab')],
                [(by[f'Outcrop_{n}'], 0.0) for n in ('Spine', 'TorB', 'HoodooA', 'Fin', 'FinB')]]
        overview(rows, lt.preview_path('RansomsRest/Rocks', 'Rocks_kit'), gap=2.5, view=(-0.2, -1.6, 0.5), fit=0.7)
        report = {}
        for n in kit:
            v = lo.vertices(by[n])
            report[n] = dict(triangles=lo.triangles(by[n]), size=[round(float(x), 2) for x in v.max(axis=0) - v.min(axis=0)],
                             hulls=len([c for c in by[n].children if c.name.startswith('UCX_')]))
        with open(lt.preview_path('RansomsRest/Rocks', 'Outcrops_report').replace('.png', '.json'), 'w') as f:
            json.dump(report, f, indent=1)
    if not [a for a in ARGS if a in ALL]:
        rows = [[(by[n], 0.0) for n in ('FarRock_A', 'FarRock_B', 'FarRock_C', 'FarRock_D', 'Outcrop_Slab',
                                        'Outcrop_TorD')],
                [(by[n], 0.0) for n in ('Outcrop_TorA', 'Outcrop_TorC', 'Outcrop_Fin', 'Outcrop_Spine')],
                [(by['Outcrop_TorB'], 0.0), (by['DenRock'], -12.0)]]
        overview(rows, lt.preview_path('RansomsRest', 'Rocks_overview'), gap=3.0, view=(-0.2, -1.6, 0.62), fit=0.74)
