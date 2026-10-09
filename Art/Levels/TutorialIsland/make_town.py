"""Crossroads Town on Skyreach (the tutorial island): town.json, the town's dressing, made from the concept.

The concept viewer drew Crossroads Town piece by piece (crossroads_town.json: Tools/ConceptViewer's dump, 784
placements plus its roads, fences, hedges, pools, webs and no-tree areas). This script turns that drawing into
town.json, the shared contract's file (Main's town round 1) that Tools/Unreal/build_area_dressing.py places: the
pieces it instances (props, lamps, trees, bushes, reeds, rocks, Web Hollow's dressing), the fence and wall lines it
runs with its kits, the webs strung in Web Hollow, the no-tree areas the scatter keeps out of, and, by kind, the concept
pieces that wait for a model of their own (deferred).

It reads, besides the concept:
- layout.json: the buildings, roads, ponds and zones Main places (the town's buildings are Main's, never pieces here);
- layout_computed.json: today's orchard rows, the pond, the creek, the ramp and the bridge;
- Saved/MeshBounds.json (written from the editor): every game mesh's real bounds, so every clearance is measured;
- Art/Models/Props/Sink.py's DEAD_TREE_LIMBS: DeadTree_A's trunk and main limbs (measured for Web_Crown), where a
  cocoon can hang.

What it leaves out (and says so): what layout.json places (the town's buildings, the second well, the outhouses); what
today's island already has (the farmstead's buildings, windmill, lookout, jetty and skiff, bridge, gun rack, dummies,
the orchard's apple rows, and what the scatter grows: the pond's shore reeds and lily pads, grass, the meadow rocks);
the fences the concept drew piece by piece (they are lines here) and its hedge pieces (rows of bushes here); concept
the concept's woods round Web Hollow are kept as pieces (the scatter's forest grew on the rise's middle, now the clearing).

What it fixes (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "Fix while building"): the farm road's farmhouse and
its whole yard and garden move 10 m back along the road (layout.json's farmhouse_farmroad), the garden made narrower
where it then met the farm road cabin's, the washing line hung on the house's other side; the north-west and
south-east corner lamps and what stands with them move in along their diagonals; the north-east corner lamp steps off
the forest road; the saloon's hitch rail stands parallel to its front beside the doors, the trough before it. Lines
keep 4 m inside the island's rim (the orchard fence ran past it) and gardens off the paths (their far side made
shallower).

Then every piece is checked, highest priority first (the memorial and notice board, the dead trees and the tor, lamps
and posts, hedges, big props, small props, trees, bushes, reeds, lily pads), against the island's rim (4 m for its
core, 3 m for every part) and the plateau's cliffs, water, buildings (their real bounds, porches included) and the ways
to their doors, the gameplay spots (spawn, gun rack, the gunsmith's bench, the dummies, the jetty, the notice board's
front: 1.5 m), road surfaces (lamps and posts may stand on a verge; the roads are ignored only for what the concept
stood round the memorial, where they meet), fence lines, their gates and the gardens inside them, Web Hollow's strands,
and every piece already placed (trees keep apart by their crowns; the tor keeps out from under them). A piece in the
way is nudged to the nearest free spot within its reach, or dropped; every nudge and drop is printed, and the result is
verified again on its own at the end.

Run with Blender's bundled Python (the standard library only; Blender itself isn't needed):
    "C:\\Program Files\\Blender Foundation\\Blender 4.4\\4.4\\python\\bin\\python.exe" Art/Levels/TutorialIsland/make_town.py
The same inputs always give the same town.json: the only randomness is seeded streams, ids follow the concept's
order, and the output is sorted.
"""
import ast
import json
import math
import os
import random
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
CONCEPT_PATH = os.path.join(HERE, 'crossroads_town.json')
LAYOUT_PATH = os.path.join(HERE, 'layout.json')
COMPUTED_PATH = os.path.join(HERE, 'layout_computed.json')
BOUNDS_PATH = os.path.join(REPO, 'Saved', 'MeshBounds.json')
SINK_PATH = os.path.join(REPO, 'Art', 'Models', 'Props', 'Sink.py')
OUT_PATH = os.path.join(HERE, 'town.json')

# ---------------------------------------------------------------------------
# What the concept's pieces become
# ---------------------------------------------------------------------------

# Concept kit pieces with a game model (the contract's round 1 mapping), by kit name and the concept's variant.
KIT_MESH = {
    'oak': ('Oak_A', 'Oak_B', 'Oak_C'), 'birch': ('Birch_A', 'Birch_B'), 'pine': ('Pine_A', 'Pine_B'),
    'apple': ('Apple_A',), 'bush': ('Bush_A', 'Bush_B', 'Bush_C'), 'reeds': ('Reeds_A', 'Reeds_B'), 'shed': ('Woodshed',),
}
# Kit pieces that wait for a model (round 2): town.json's deferred, with their counts. k:burrow's sheet webs are made.
DEFERRED_KITS = ('stall', 'scarecrow', 'coop', 'haystack', 'sandbags', 'barricade', 'target', 'dock', 'mush', 'burrow')
# The concept's piece-by-piece fences and walls: its `fences` lines drawn section by section, so they are lines here.
LINE_MODELS = ('FenceRail', 'FencePost', 'FenceBroken', 'StoneWall', 'k:picket')
# Models today's island already places (build_area.py's gameplay: the jetty with its bell and slate, the skiff; the
# computed bridge).
EXISTING_MODELS = ('Bridge', 'SkiffJetty', 'Skiff_A_Packet', 'JettyBellPost', 'JettySlate')
# Skyreach's own model for a concept piece, written to town.json in place of the concept's mesh. Everything before the
# write (the checks, the ids, the footprint in MeshBounds, the centre rule) keeps the concept's name: NoticeBoard_Town
# is the same board, same footprint, with NOTICES on its header (Ransom's Rest's NoticeBoard says RIM RANGERS).
OUTPUT_MESH = {'NoticeBoard': 'NoticeBoard_Town'}
ROCKS = ('Rock_A', 'Rock_B', 'Rock_C', 'Rock_D', 'Boulder_A', 'Boulder_B', 'Boulder_C')
TREES = ('Oak_A', 'Oak_B', 'Oak_C', 'Birch_A', 'Birch_B', 'Pine_A', 'Pine_B', 'Apple_A')
BUSHES = ('Bush_A', 'Bush_B', 'Bush_C')
REEDS = ('Reeds_A', 'Reeds_B')
# The island pond's lily pads: the scatter already floats three drifts of them (build_island_scatter.shore_points),
# so the concept's seven are left out. True puts them in as well (onWater "pond").
POND_PADS = False

# Places (cm): the concept's creature grounds and the square.
HOLLOW = (5800.0, 5600.0)        # Web Hollow, the spiders' ground (the concept's spiderHollow)
HOLLOW_R = 2000.0
WALLOW = (2400.0, -5600.0)       # the Wallow, the slimes' bog
WALLOW_R = 2100.0
SQUARE = 1300.0                  # the cobbled square's half side, centred on the crossroads
PLAZA_CORE = 300.0               # where the roads meet: only the memorial itself stands on them (its benches at 5 m sat on the forest and farm roads, and the path probe found one blocking)

# ---------------------------------------------------------------------------
# How pieces stand and collide
# ---------------------------------------------------------------------------

# Pieces the dressing stands plumb (the contract's "upright": trees, sheds, stacks, lamps, posts, and what hangs or is
# fitted to them); lily pads lie flat on the water; the rest are props that tilt a little with the ground.
UPRIGHT = TREES + ('DeadTree_A', 'Woodshed', 'FirewoodStack', 'LampPost', 'LanternPost', 'Signpost', 'HitchRail',
                   'NoticeBoard', 'TownMemorial', 'LaundryLine', 'Web_Crown', 'Cocoon_Hung', 'Outcrop_TorB')
# No collision (the contract's bSolid false): reeds, lily pads, bushes, webs, cocoons. The Bush meshes have no collision
# hulls, so a hedge's bushes aren't solid either (solid would only put them on the minimap as obstacles players walk
# through: T2's note); the checks still keep them off everything solid.
NONSOLID = REEDS + ('LilyPads_A', 'Web_Crown', 'Cocoon_Hung', 'Cocoon_Lying') + BUSHES
# The part of a piece that stands on the ground, where its mesh bounds reach far over it (cm, its own space: x0, x1,
# y0, y1): a lamp's arm and a signpost's boards are overhead, only the post is in the way.
POSTS = {'LampPost': (-12.5, 12.5, -12.5, 12.5), 'LanternPost': (-18.0, 18.0, -18.0, 18.0),
         'Signpost': (-15.0, 15.0, -15.0, 15.0)}
# A tree's trunk at its foot (cm across, at scale 1): what the scatter's trees and props must not run into. MeshBounds
# has the crowns only; these follow the trees' collision hulls round the lower trunk (Trees.py, Pines.py, DeadTree.py,
# build_area_dressing.PIECES' 80 cm for the dead tree).
TRUNKS = {'Oak_A': 90.0, 'Oak_B': 90.0, 'Oak_C': 120.0, 'Birch_A': 50.0, 'Birch_B': 50.0, 'Pine_A': 70.0,
          'Pine_B': 70.0, 'Apple_A': 60.0, 'DeadTree_A': 80.0}
# Bushes and reeds are loose shapes: their core (this share of their bounds) is what may not touch a solid piece.
CORE = 0.6
HEDGE_CORE = 0.7
ROCK_CORE = 0.7

# The ways to buildings' doors, kept clear (cm, the model's own space, unscaled): (side, y0, y1) for each door, measured
# from the model scripts (Art/Models/Buildings; Blender x becomes Unreal -Y): 'front' runs from the front wall out past
# the porch or stoop (the mesh's max X) by DOOR_REACH, 'back' from the back wall out past the mesh's min X.
DOORS = {
    'Cottage': [('front', 320.0, 51.0, 151.0)],                  # Cottage.py: DOOR at 0.89-1.89 m from X0 = -2.4
    'LogCabin': [('front', 280.0, 50.0, 150.0)],                 # LogCabin.py: DOOR at 1.9-2.9 m from -OUT_X
    'Farmhouse': [('front', 310.0, -50.0, 50.0)],                # Farmhouse.py: the door in the middle of the front
    'FalseFront_Saloon': [('front', 700.0, -65.0, 65.0), ('back', -700.0, 100.0, 200.0)],
    'FalseFront_Store': [('front', 650.0, -65.0, 65.0), ('back', -650.0, 120.0, 225.0)],
    'FalseFront_Sheriff': [('front', 500.0, -50.0, 50.0), ('back', -500.0, 55.0, 155.0)],
    'FalseFront_Undertaker': [('front', 600.0, -55.0, 55.0), ('back', -600.0, 70.0, 175.0)],
    'Outhouse': [('front', 65.0, -37.0, 37.0)],
    'Barn': [('front', 600.0, -160.0, 160.0), ('back', -600.0, -60.0, 60.0)],
    'LookoutTower': [('front', 280.0, -80.0, 80.0)],
}
DOOR_REACH = 150.0
DOOR_SIDE = 40.0

# Clearances (cm).
SPOT_CLEAR = 150.0       # from the gameplay spots (the contract)
RIM_CLEAR = 400.0        # inside the island's rim: the meadow rolls over its edge within 3.2 m (area_shape.RIM_ROLL)
RIM_HARD = 300.0         # and never less, for any part of any piece or line (Main's trial build: a fence past the rim
                         # stood on nothing)
RIM_STEP = 20.0          # how finely a line is walked against the rim (cm)
CLIFF_OUT = 550.0        # off the plateau's cliffs: its polygon is the cliff's top, the face falls 3.5 m out from it
CLIFF_IN = 150.0         # ...and back from its lip on top
CREEK_HALF = 175.0       # the creek's valley (layout.json's creek width 350)
GATE_DEPTH = 120.0       # a gate's way through, kept clear either side of its line
GARDEN_GAP = 200.0       # between two gardens' fences: room to walk between them
STRAND_CLEAR = 60.0      # off a web strand's line (it sags and sways a little)
TALL = 300.0             # a piece taller than this (cm) keeps out from under tree crowns
ROAD_MARGIN = {'verge': 10.0, 'tree': 120.0, 'bush': 15.0, 'reed': 10.0, 'hedge': 30.0, 'prop': 40.0}
LINE_THICK = {'rail': 20.0, 'picket': 12.0, 'stone': 74.0}

# Priority (who stays where the concept put it when two want the same ground) and how far each may be nudged (cm).
CLASSES = ['centre', 'verge', 'hedge', 'big', 'small', 'tree', 'bush', 'reed', 'pad']
REACH = {'centre': 450.0, 'verge': 300.0, 'hedge': 60.0, 'big': 300.0, 'small': 250.0, 'tree': 500.0,
         'bush': 300.0, 'reed': 200.0, 'pad': 60.0}
# Web Hollow's tor is its landmark: the concept stood it 4.4 m from the rim, its 6 m bulk over the rim's roll, and the
# web ring is inside of it, so it may go further round the hollow rather than be lost.
REACH_MESH = {'Outcrop_TorB': 1100.0}
BIG = ('Cart', 'Woodshed', 'HitchRail', 'WaterTrough', 'LaundryLine', 'Bench', 'Wheelbarrow', 'HayBale_Round',
       'Boulder_A', 'Boulder_B', 'Boulder_C', 'Cocoon_Lying')
STEP = 25.0

# ---------------------------------------------------------------------------
# The concept's houses: what its house() put round each (Tools/ConceptViewer/web/kit.js house(), the options from
# concepts.js conceptCrossroads() and farmstead()), so each yard piece is known by its house and moves with it.
# ---------------------------------------------------------------------------

HOUSES = {
    'farmhouse': dict(group='farm', garden=(1100.0, 700.0), laundry=True, lamp='LanternPost'),
    'saloon': dict(group='square', bench=False, flowers=False, firewood=False),
    'cottage_ne': dict(group='square', garden=(800.0, 600.0), laundry=True),
    'cabin_se': dict(group='square', garden=(800.0, 600.0)),
    'sheriff': dict(group='square', flowers=False, firewood=False),
    'cottage_sw': dict(group='square', laundry=True),
    'store': dict(group='square', bench=False, firewood=False),
    'undertaker': dict(group='square', flowers=False, firewood=False, barrels=False),
    'cabin_farmroad': dict(group='farmroad', garden=(800.0, 600.0), laundry=True),
    'cottage_farmroad': dict(group='farmroad', garden=(700.0, 600.0)),
    'farmhouse_farmroad': dict(group='farmroad', garden=(900.0, 650.0), laundry=True),
}
YARD_MATCH = 40.0        # cm: how close a concept piece must be to its house's slot

# The fixes' numbers. Fix 2: each corner lamp and what stands with it moves in along its diagonal to 12.7 m from the
# centre; fix 3: the north-east corner lamp stands 90 cm off the forest road's edge, as the road lamps stand off theirs.
CORNER_MOVES = [((1150.0, -1150.0), (900.0, -900.0)), ((-1150.0, 1150.0), (-900.0, 900.0))]
CORNER_GROUP_R = 330.0
NE_LAMP = (1150.0, 1150.0)
LAMP_OFF_ROAD = 90.0

# Web Hollow: egg sac clusters (the concept's k:sacs, five small sacs) are three of the game's sacs, small: (mesh, cm
# from the cluster's spot, bearing from its yaw, scale). Cocoons hang from a limb that can hold one (Cocoon_Hung: its
# pivot SOCKET_Top, its top, 3 cm into the limb's underside, its bundle 1-2.26 m below it; lift is that pivot's height),
# else lie at the tree's foot. The concept's 2.4-3.2 m was the bundle's middle in the viewer's kit: the game's hangs
# lower, so a limb 3.3-4.2 m up keeps its bundle clear of the ground.
SAC_CLUSTER = [('EggSac_B', 0.0, 0.0, 0.34), ('EggSac_A', 50.0, 115.0, 0.28), ('EggSac_C', 48.0, 240.0, 0.26)]
COCOON_DROP = 226.0      # Cocoon_Hung's bundle bottom under its pivot (MeshBounds min Z)
COCOON_KNOT = 100.0      # its knot (the rope above it is thin)
COCOON_R = 25.0          # its bundle's radius
COCOON_LIFT = (330.0, 420.0)   # the pivot's height over the ground: the bundle's bottom 1.0-1.9 m up
COCOON_LIMB_R = 0.045    # the thinnest limb (m) that holds one
DEAD_LIMBS = (1, 6, 15, 29)


def load(path):
    with open(path, encoding='utf-8') as file:
        return json.load(file)


def dead_tree_limbs():
    """DeadTree_A's trunk (0) and main limbs as Sink.py measured them for Web_Crown (Blender metres: x, y, z, radius,
    from the trunk's foot), read from that file so there is one copy of the measurement."""
    with open(SINK_PATH, encoding='utf-8') as file:
        tree = ast.parse(file.read())
    for node in tree.body:
        if isinstance(node, ast.Assign) and any(getattr(t, 'id', None) == 'DEAD_TREE_LIMBS' for t in node.targets):
            return ast.literal_eval(node.value)
    raise SystemExit(f'{SINK_PATH} has no DEAD_TREE_LIMBS')


# ---------------------------------------------------------------------------
# Plane geometry (cm; yaw in degrees from +X toward +Y, Unreal's)
# ---------------------------------------------------------------------------

def rot(lx, ly, yaw):
    a = math.radians(yaw)
    c, s = math.cos(a), math.sin(a)
    return lx * c - ly * s, lx * s + ly * c


def to_world(x, y, yaw, lx, ly):
    dx, dy = rot(lx, ly, yaw)
    return x + dx, y + dy


def to_local(x, y, yaw, px, py):
    return rot(px - x, py - y, -yaw)


def blender_to_local(bx, by):
    """A model's Blender metres to its Unreal space (cm): Blender -Y is the actor's +X, Blender -X its +Y."""
    return -by * 100.0, -bx * 100.0


class Box:
    """An oriented rectangle: its middle, its yaw and its half sizes along its own X and Y."""
    __slots__ = ('cx', 'cy', 'yaw', 'c', 's', 'hx', 'hy')

    def __init__(self, cx, cy, yaw, hx, hy):
        self.cx, self.cy, self.yaw, self.hx, self.hy = cx, cy, yaw, hx, hy
        a = math.radians(yaw)
        self.c, self.s = math.cos(a), math.sin(a)

    def local(self, px, py):
        dx, dy = px - self.cx, py - self.cy
        return dx * self.c + dy * self.s, -dx * self.s + dy * self.c

    def corners(self):
        out = []
        for a, b in ((1, 1), (-1, 1), (-1, -1), (1, -1)):
            lx, ly = a * self.hx, b * self.hy
            out.append((self.cx + lx * self.c - ly * self.s, self.cy + lx * self.s + ly * self.c))
        return out

    def samples(self):
        """Its corners, edge middles and middle: where a curved boundary (water, the rim) is tested."""
        out = [(self.cx, self.cy)]
        for a, b in ((1, 1), (-1, 1), (-1, -1), (1, -1), (1, 0), (-1, 0), (0, 1), (0, -1)):
            lx, ly = a * self.hx, b * self.hy
            out.append((self.cx + lx * self.c - ly * self.s, self.cy + lx * self.s + ly * self.c))
        return out

    def bbox(self):
        ex = self.hx * abs(self.c) + self.hy * abs(self.s)
        ey = self.hx * abs(self.s) + self.hy * abs(self.c)
        return self.cx - ex, self.cy - ey, self.cx + ex, self.cy + ey

    def scaled(self, f):
        return Box(self.cx, self.cy, self.yaw, self.hx * f, self.hy * f)


def sat_gap(a, b):
    """How far apart two boxes are along the axis that separates them most (negative: they overlap)."""
    best = -1e18
    for ux, uy in ((a.c, a.s), (-a.s, a.c), (b.c, b.s), (-b.s, b.c)):
        d = abs((b.cx - a.cx) * ux + (b.cy - a.cy) * uy)
        ra = a.hx * abs(a.c * ux + a.s * uy) + a.hy * abs(-a.s * ux + a.c * uy)
        rb = b.hx * abs(b.c * ux + b.s * uy) + b.hy * abs(-b.s * ux + b.c * uy)
        best = max(best, d - ra - rb)
    return best


def point_box_dist(box, px, py):
    lx, ly = box.local(px, py)
    return math.hypot(max(abs(lx) - box.hx, 0.0), max(abs(ly) - box.hy, 0.0))


def seg_point(a, b, p):
    """The point of segment ab nearest p, and how far it is."""
    dx, dy = b[0] - a[0], b[1] - a[1]
    l2 = dx * dx + dy * dy
    t = 0.0 if l2 < 1e-9 else max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / l2))
    q = (a[0] + dx * t, a[1] + dy * t)
    return q, math.hypot(p[0] - q[0], p[1] - q[1])


def seg_hits_box(box, a, b):
    """Whether segment ab crosses the box (Liang-Barsky in the box's frame)."""
    ax, ay = box.local(*a)
    bx, by = box.local(*b)
    t0, t1 = 0.0, 1.0
    dx, dy = bx - ax, by - ay
    for p, q in ((-dx, ax + box.hx), (dx, box.hx - ax), (-dy, ay + box.hy), (dy, box.hy - ay)):
        if abs(p) < 1e-12:
            if q < 0.0:
                return False
            continue
        r = q / p
        if p < 0.0:
            t0 = max(t0, r)
        else:
            t1 = min(t1, r)
        if t0 > t1:
            return False
    return True


def box_seg_dist(box, a, b):
    if seg_hits_box(box, a, b):
        return 0.0
    best = min(seg_point(a, b, c)[1] for c in box.corners())
    return min(best, point_box_dist(box, *a), point_box_dist(box, *b))


def box_line_dist(box, pts, reach=1e9, closed=False):
    """The box's distance from a polyline (0 where they touch); segments further than reach are skipped."""
    x0, y0, x1, y1 = box.bbox()
    best = 1e18
    seq = list(pts) + ([pts[0]] if closed else [])
    for a, b in zip(seq, seq[1:]):
        if min(a[0], b[0]) > x1 + reach or max(a[0], b[0]) < x0 - reach or \
                min(a[1], b[1]) > y1 + reach or max(a[1], b[1]) < y0 - reach:
            continue
        best = min(best, box_seg_dist(box, a, b))
        if best <= 0.0:
            break
    return best


def nearest_on(pts, p, closed=False):
    seq = list(pts) + ([pts[0]] if closed else [])
    best, best_d = seq[0], 1e18
    for a, b in zip(seq, seq[1:]):
        q, d = seg_point(a, b, p)
        if d < best_d:
            best, best_d = q, d
    return best, best_d


def inside(poly, x, y):
    hit = False
    n = len(poly)
    for i in range(n):
        (x0, y0), (x1, y1) = poly[i], poly[(i + 1) % n]
        if (y0 > y) != (y1 > y) and x < x0 + (y - y0) * (x1 - x0) / (y1 - y0):
            hit = not hit
    return hit


def centroid(poly):
    return sum(p[0] for p in poly) / len(poly), sum(p[1] for p in poly) / len(poly)


def length(pts):
    return sum(math.dist(a, b) for a, b in zip(pts, pts[1:]))


def point_at(pts, d):
    """The point d along a polyline and the unit direction there."""
    for a, b in zip(pts, pts[1:]):
        step = math.dist(a, b)
        if d <= step or b is pts[-1]:
            t = 0.0 if step < 1e-9 else max(0.0, min(1.0, d / step))
            return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t), ((b[0] - a[0]) / step, (b[1] - a[1]) / step)
        d -= step
    a, b = pts[-2], pts[-1]
    step = math.dist(a, b)
    return b, ((b[0] - a[0]) / step, (b[1] - a[1]) / step)


def cut(pts, d0, d1):
    """The polyline from d0 along it to d1, with its corners between."""
    out = [point_at(pts, d0)[0]]
    walked = 0.0
    for a, b in zip(pts, pts[1:]):
        walked += math.dist(a, b)
        if d0 < walked < d1:
            out.append(b)
    out.append(point_at(pts, d1)[0])
    return out


def crossings(line, road):
    """Every point where a road's centre line crosses a line, with the sine of the angle between them."""
    found = []
    for a, b in zip(line, line[1:]):
        for c, d in zip(road, road[1:]):
            r = (b[0] - a[0], b[1] - a[1])
            s = (d[0] - c[0], d[1] - c[1])
            den = r[0] * s[1] - r[1] * s[0]
            if abs(den) < 1e-9:
                continue
            t = ((c[0] - a[0]) * s[1] - (c[1] - a[1]) * s[0]) / den
            u = ((c[0] - a[0]) * r[1] - (c[1] - a[1]) * r[0]) / den
            if 0.0 <= t <= 1.0 and 0.0 <= u <= 1.0:
                found.append(((a[0] + r[0] * t, a[1] + r[1] * t), abs(den) / (math.hypot(*r) * math.hypot(*s))))
    return found


def seg_seg_dist(a, b, c, d):
    if crossings([a, b], [c, d]):
        return 0.0
    return min(seg_point(a, b, c)[1], seg_point(a, b, d)[1], seg_point(c, d, a)[1], seg_point(c, d, b)[1])


def poly_line_dist(poly, pts):
    """A closed polygon's edges' distance from a polyline."""
    return min(seg_seg_dist(a, b, c, d) for a, b in zip(poly, poly[1:] + poly[:1]) for c, d in zip(pts, pts[1:]))


def polys_overlap(p, q):
    """Whether two simple polygons overlap (a vertex inside the other, or edges crossing)."""
    if any(inside(q, *v) for v in p) or any(inside(p, *v) for v in q):
        return True
    for a, b in zip(p, p[1:] + p[:1]):
        for c, d in zip(q, q[1:] + q[:1]):
            if crossings([a, b], [c, d]):
                return True
    return False


def snake(name):
    return name.lower()


def rnd1(v):
    return round(float(v), 1)


# ---------------------------------------------------------------------------
# Mesh footprints
# ---------------------------------------------------------------------------

class Meshes:
    """Saved/MeshBounds.json by mesh name without SM_."""

    def __init__(self, data):
        self.bounds = {k[3:] if k.startswith('SM_') else k: v for k, v in data.items()}

    def has(self, mesh):
        return mesh in self.bounds

    def extent(self, mesh):
        """Its own-space ground rectangle (x0, x1, y0, y1), cm at scale 1."""
        if mesh in POSTS:
            return POSTS[mesh]
        if mesh in TRUNKS:
            h = TRUNKS[mesh] * 0.5
            return -h, h, -h, h
        b = self.bounds[mesh]
        return b['min'][0], b['max'][0], b['min'][1], b['max'][1]

    def box(self, mesh, x, y, yaw, scale=1.0):
        x0, x1, y0, y1 = self.extent(mesh)
        sx, sy = (scale, scale) if not isinstance(scale, (list, tuple)) else (scale[0], scale[1])
        cx, cy = to_world(x, y, yaw, (x0 + x1) * 0.5 * sx, (y0 + y1) * 0.5 * sy)
        return Box(cx, cy, yaw, (x1 - x0) * 0.5 * sx, (y1 - y0) * 0.5 * sy)

    def crown(self, mesh, scale):
        """A tree's crown radius: half its narrower spread (cm)."""
        b = self.bounds[mesh]
        s = scale if not isinstance(scale, (list, tuple)) else max(scale[0], scale[1])
        return 0.5 * min(b['max'][0] - b['min'][0], b['max'][1] - b['min'][1]) * s

    def mesh_box(self, mesh, x, y, yaw, scale=1.0):
        """The whole mesh's rectangle (a building's walls and porch)."""
        b = self.bounds[mesh]
        x0, x1, y0, y1 = b['min'][0], b['max'][0], b['min'][1], b['max'][1]
        cx, cy = to_world(x, y, yaw, (x0 + x1) * 0.5 * scale, (y0 + y1) * 0.5 * scale)
        return Box(cx, cy, yaw, (x1 - x0) * 0.5 * scale, (y1 - y0) * 0.5 * scale)


# ---------------------------------------------------------------------------
# The pieces
# ---------------------------------------------------------------------------

class Item:
    """A piece to place: where the concept (after the fixes) wants it, and what the checks may do to it."""

    def __init__(self, mesh, x, y, yaw, group, order, scale=1.0, why='', **extra):
        self.mesh, self.x, self.y, self.yaw, self.group, self.order = mesh, x, y, yaw % 360.0, group, order
        self.scale, self.why, self.extra = scale, why, dict(extra)
        self.hedge = extra.pop('hedge', None)
        self.cluster = extra.pop('cluster', None)
        self.extra.pop('hedge', None)
        self.extra.pop('cluster', None)
        self.cls = piece_class(mesh, self.hedge)
        self.id = None
        self.yard = None         # the house whose yard it belongs to (HOUSES), if any

    @property
    def solid(self):
        return self.extra.get('solid', self.mesh not in NONSOLID)


def piece_class(mesh, hedge=None):
    if hedge:
        return 'hedge'
    if mesh in ('TownMemorial', 'NoticeBoard', 'DeadTree_A', 'Outcrop_TorB'):
        return 'centre'
    if mesh in POSTS:
        return 'verge'
    if mesh in BIG:
        return 'big'
    if mesh in TREES:
        return 'tree'
    if mesh in BUSHES:
        return 'bush'
    if mesh in REEDS:
        return 'reed'
    if mesh == 'LilyPads_A':
        return 'pad'
    return 'small'


class Placed:
    """A piece where it finally stands, as the checks see it."""
    __slots__ = ('item', 'box', 'solid', 'crown')

    def __init__(self, item, box, solid, crown):
        self.item, self.box, self.solid, self.crown = item, box, solid, crown


# ---------------------------------------------------------------------------
# The world the pieces must fit in
# ---------------------------------------------------------------------------

class Obstacle:
    def __init__(self, name, box, doors=()):
        self.name, self.box, self.doors = name, box, list(doors)


class World:
    def __init__(self, concept, layout, computed, meshes):
        self.meshes = meshes
        self.outline = [tuple(p) for p in layout['outline']]
        plateau = next(f for f in layout['features'] if f['id'] == 'plateau')
        self.plateau = [tuple(p) for p in plateau['polygon']]
        self.ramp = [(p[0], p[1]) for p in computed['ramp']['points']]
        self.ramp_half = computed['ramp']['width'] * 0.5
        self.zones = {z['id']: [tuple(p) for p in z['polygon']] for z in layout['zones']}
        # Roads: the concept's (Main's split matches them) and the layout's as Main wrote it (its plateau path runs
        # from the crossroads through the square), and the ramp with its walls.
        self.roads = []
        for k, r in enumerate(concept['roads']):
            self.roads.append((f'concept road {k + 1} ({r["kind"]})', [tuple(p) for p in r['points']], r['width'] * 0.5))
        for r in layout['roads']:
            self.roads.append((r['id'], [tuple(p) for p in r['path']], r['width'] * 0.5))
        self.roads.append(('ramp', self.ramp, self.ramp_half + 150.0))
        self.ponds = {k: (tuple(v['center']), tuple(v['radii'])) for k, v in computed['ponds'].items()}
        self.creek = [(p[0], p[1]) for p in computed['creek']['points']]
        self.buildings, self.spots = [], []
        self.gone = []
        for p in layout['placements']:
            kind, (x, y) = p['kind'], p['location'][:2]
            yaw, scale = p.get('yaw', 0.0), p.get('scale', 1.0)
            if kind == 'PlayerStart':
                self.spots.append(('the spawn', Box(x, y, 0.0, 1.0, 1.0)))
                continue
            if kind == 'TargetDummy':
                self.spots.append((f'{p["id"]}', Box(x, y, 0.0, 40.0, 40.0)))
                continue
            if not meshes.has(kind):
                self.gone.append(p['id'])
                continue
            box = meshes.mesh_box(kind, x, y, yaw, scale)
            self.buildings.append(Obstacle(p['id'], box, self.door_boxes(kind, x, y, yaw, scale)))
            if kind == 'GunRack':
                self.spots.append(('the gun rack', box))
        for bench in layout['gameplay'].get('benches', []):
            (x, y), yaw = bench['at'], bench['yaw']
            box = meshes.mesh_box('GunsmithBench', x, y, yaw)
            self.buildings.append(Obstacle(bench['id'], box))
            self.spots.append((f"the gunsmith's bench {bench['id']}", box))
        jetty = layout['gameplay'].get('jetty')
        if jetty:
            self.spots.append(('the jetty', meshes.mesh_box('SkiffJetty', jetty['at'][0], jetty['at'][1], jetty['yaw'])))
        bridge = computed.get('bridge')
        if bridge:
            x, y = bridge['location'][:2]
            self.buildings.append(Obstacle('the bridge', Box(x, y, bridge['yaw'], bridge['span'] * 0.5,
                                                             bridge['width'] * 0.5)))
        self.lines = []          # (id, kit, points, closed)
        self.gates = []          # (line id, Box)
        self.gardens = []        # (line id, polygon): the kitchen gardens' fences
        self.strands = []        # (a, b): Web Hollow's strands, between its dead trees
        self.no_trees = []       # ('circle', (x, y), r) or ('polygon', poly)

    def door_boxes(self, kind, x, y, yaw, scale):
        out = []
        if kind not in DOORS:
            return out
        b = self.meshes.bounds[kind]
        for side, face, y0, y1 in DOORS[kind]:
            if side == 'front':
                x0, x1 = face, b['max'][0] + DOOR_REACH / scale
            else:
                x0, x1 = b['min'][0] - DOOR_REACH / scale, face
            y0, y1 = y0 - DOOR_SIDE / scale, y1 + DOOR_SIDE / scale
            cx, cy = to_world(x, y, yaw, (x0 + x1) * 0.5 * scale, (y0 + y1) * 0.5 * scale)
            out.append((side, Box(cx, cy, yaw, (x1 - x0) * 0.5 * scale, (y1 - y0) * 0.5 * scale)))
        return out


# ---------------------------------------------------------------------------
# The checks
# ---------------------------------------------------------------------------

class Placer:
    CELL = 800.0

    def __init__(self, world):
        self.world = world
        self.meshes = world.meshes
        self.grid = defaultdict(list)
        self.placed = []
        self.log_nudged, self.log_dropped = [], []

    def cells(self, bbox, pad=0.0):
        x0, y0, x1, y1 = bbox
        for i in range(int(math.floor((x0 - pad) / self.CELL)), int(math.floor((x1 + pad) / self.CELL)) + 1):
            for j in range(int(math.floor((y0 - pad) / self.CELL)), int(math.floor((y1 + pad) / self.CELL)) + 1):
                yield i, j

    def add(self, rec):
        self.placed.append(rec)
        for cell in self.cells(rec.box.bbox()):
            self.grid[cell].append(rec)

    def near(self, box, pad):
        seen = set()
        for cell in self.cells(box.bbox(), pad):
            for rec in self.grid.get(cell, ()):
                if id(rec) not in seen:
                    seen.add(id(rec))
                    yield rec

    def height(self, item):
        b = self.meshes.bounds[item.mesh]
        s = item.scale if not isinstance(item.scale, (list, tuple)) else item.scale[2]
        return b['max'][2] * s

    def footprint(self, item, x, y):
        box = self.meshes.box(item.mesh, x, y, item.yaw, item.scale)
        if item.cls in ('bush', 'reed'):
            return box, box.scaled(CORE)
        if item.cls == 'hedge':
            return box, box.scaled(HEDGE_CORE)
        return box, box

    def problem(self, item, x, y):
        """The first thing in the way of the item at (x, y): (what, a push away from it), or None."""
        w = self.world
        cls = item.cls
        box, core = self.footprint(item, x, y)
        test = core          # what touches things: the core for loose shapes, the whole piece otherwise
        tree = cls == 'tree' or item.mesh == 'DeadTree_A'
        crown = self.meshes.crown(item.mesh, item.scale) if tree else 0.0
        # A rock's bounds are a box round a rounded lump: against the rim, the cliffs and water its core is what sits on
        # the ground (the outcrop's corners would otherwise keep it 8 m off the rim).
        ground = box.scaled(ROCK_CORE) if item.mesh in ROCKS + ('Outcrop_TorB',) else test

        # The island's rim (the piece's core 4 m in, every part of it 3 m) and the plateau's cliffs.
        for px, py in ground.samples():
            if not inside(w.outline, px, py) or nearest_on(w.outline, (px, py), closed=True)[1] < RIM_CLEAR:
                return 'the island rim', (-x, -y)
        for px, py in box.samples():
            if not inside(w.outline, px, py) or nearest_on(w.outline, (px, py), closed=True)[1] < RIM_HARD:
                return 'the island rim', (-x, -y)
        if cls != 'pad':
            for px, py in ground.samples():
                d = nearest_on(w.plateau, (px, py), closed=True)[1]
                top = inside(w.plateau, px, py)
                if (top and d < CLIFF_IN) or (not top and d < CLIFF_OUT):
                    if nearest_on(w.ramp, (px, py))[1] < w.ramp_half + 400.0:
                        continue
                    q = nearest_on(w.plateau, (px, py), closed=True)[0]
                    return 'the plateau cliff', ((px - q[0], py - q[1]) if not top else (q[0] - px, q[1] - py))

        # Water: lily pads float inside their pool, reeds may stand at the edge, everything else keeps off.
        for pid, ((cx, cy), (rx, ry)) in w.ponds.items():
            if cls == 'pad':
                if pid != item.extra.get('onWater'):
                    continue
                grow = -(max(box.hx, box.hy) + 15.0)
                if ((x - cx) / (rx + grow)) ** 2 + ((y - cy) / (ry + grow)) ** 2 > 1.0:
                    return f'{pid} (a lily pad off its water)', (cx - x, cy - y)
                continue
            grow = {'tree': 200.0, 'reed': -60.0, 'bush': 30.0}.get(cls, 80.0)
            pts = [(x, y)] if cls in ('tree', 'reed') else ground.samples()
            for px, py in pts:
                if ((px - cx) / max(rx + grow, 1.0)) ** 2 + ((py - cy) / max(ry + grow, 1.0)) ** 2 < 1.0:
                    return f'the water of {pid}', (x - cx, y - cy)
        if cls != 'pad':
            need = {'tree': CREEK_HALF + 150.0, 'reed': 120.0, 'bush': CREEK_HALF + 20.0}.get(cls, CREEK_HALF + 50.0)
            if box_line_dist(test, w.creek, need) < need:
                q = nearest_on(w.creek, (x, y))[0]
                return 'the creek', (x - q[0], y - q[1])

        # Buildings, their porches, and the ways to their doors.
        for b in w.buildings:
            if tree:
                if point_box_dist(b.box, x, y) < 0.8 * crown or sat_gap(box, b.box) < 30.0:
                    return f'{b.name} (a crown over its roof)', (x - b.box.cx, y - b.box.cy)
            elif sat_gap(test, b.box) < (10.0 if cls in ('bush', 'reed') else 15.0):
                return f'{b.name}', (x - b.box.cx, y - b.box.cy)
            for side, door in b.doors:
                if sat_gap(test, door) < 0.0:
                    return f"the way to {b.name}'s {side} door", (x - door.cx, y - door.cy)

        # Gameplay spots.
        for name, spot in w.spots:
            if sat_gap(test, spot) < SPOT_CLEAR:
                return f'{name} (1.5 m)', (x - spot.cx, y - spot.cy)

        # Roads: off their surfaces, except for what the concept stood round the memorial, where they meet (judged by
        # where the concept put it, so a nudge never escapes a road into the middle of the square).
        if cls != 'pad' and math.hypot(item.x, item.y) > PLAZA_CORE:
            margin = ROAD_MARGIN['verge' if cls == 'verge' else cls if cls in ROAD_MARGIN else 'prop']
            for name, pts, half in w.roads:
                if box_line_dist(test, pts, half + margin) < half + margin:
                    q = nearest_on(pts, (x, y))[0]
                    return f'the road ({name})', (x - q[0], y - q[1])

        # Fences and walls, and the ways through their gates.
        if cls != 'pad':
            for lid, kit, pts, closed in w.lines:
                need = LINE_THICK[kit] * 0.5 + {'tree': 60.0, 'bush': 5.0, 'reed': 5.0}.get(cls, 15.0)
                if box_line_dist(test, pts, need, closed) < need:
                    q = nearest_on(pts, (x, y), closed)[0]
                    return f'the fence {lid}', (x - q[0], y - q[1])
            for lid, gate in w.gates:
                if sat_gap(test, gate) < 0.0:
                    return f'the gate of {lid}', (x - gate.cx, y - gate.cy)
            # Web Hollow's strands run between its dead trees, up to 4.4 m up: nothing solid or tall stands in their way.
            if item.solid or tree:
                for a, b in w.strands:
                    if box_seg_dist(test, a, b) < STRAND_CLEAR:
                        q, _ = seg_point(a, b, (x, y))
                        return 'a web strand', (x - q[0], y - q[1])
            # Nothing stands in a kitchen garden (its crops come in round 2).
            for lid, poly in w.gardens:
                if any(inside(poly, px, py) for px, py in test.samples()):
                    c = centroid(poly)
                    return f'inside the garden {lid}', (x - c[0], y - c[1])

        # Trees keep out of the no-tree areas (yards, gardens, creature grounds), the hollow's own dead trees aside.
        if cls == 'tree':
            for kind, a, b in w.no_trees:
                if kind == 'circle' and math.dist((x, y), a) < b:
                    return 'a no-tree area', (x - a[0], y - a[1])
                if kind == 'polygon' and inside(a, x, y):
                    c = centroid(a)
                    return 'a no-tree area', (x - c[0], y - c[1])

        # Everything already placed.
        solid = item.solid
        for rec in self.near(box, 600.0 if tree else 300.0):
            other = rec.item
            if item.hedge and item.hedge == other.hedge:
                continue
            if item.cluster and item.cluster == other.cluster:
                continue
            if tree and rec.crown:
                if math.dist((x, y), (other.x, other.y)) < 0.3 * (crown + rec.crown):
                    return f'the tree {other.id}', (x - other.x, y - other.y)
                continue
            # Something taller than a crown's underside (the tor) keeps out from under a tree's crown, and a tree's
            # crown off it.
            reach = 0.0
            if rec.crown and self.height(item) > TALL:
                reach = 0.8 * rec.crown
            elif tree and self.height(other) > TALL:
                reach = 0.8 * crown
            if reach and sat_gap(box, rec.box) < reach:
                return f'{other.id} ({other.mesh}: under a crown)', (x - other.x, y - other.y)
            if cls == 'verge' and other.cls == 'verge' and math.dist((x, y), (other.x, other.y)) < 150.0:
                return f'{other.id} ({other.mesh}: posts 1.5 m apart)', (x - other.x, y - other.y)
            if not solid and not rec.solid:
                continue
            a = core if not solid or cls == 'hedge' else box
            b = rec.box.scaled(CORE) if not rec.solid else (rec.box.scaled(HEDGE_CORE) if other.cls == 'hedge' else rec.box)
            need = 8.0 if (solid and rec.solid) else 0.0
            if tree or rec.crown:
                need = 80.0 if (tree and other.cls == 'verge') or (rec.crown and cls == 'verge') else 20.0
            if sat_gap(a, b) < need:
                return f'{other.id} ({other.mesh})', (x - other.x, y - other.y)
        return None

    def place(self, item):
        """Stand the item where it wants to be, or as near as its reach allows, or drop it. Returns its spot."""
        hit = self.problem(item, item.x, item.y)
        if hit is None:
            return self.accept(item, item.x, item.y)
        reason, push = hit
        base = math.atan2(push[1], push[0]) if math.hypot(*push) > 1e-6 else 0.0
        reach = REACH_MESH.get(item.mesh, REACH[item.cls])
        r = STEP
        while r <= reach + 1e-6:
            for k in range(24):
                turn = ((k + 1) // 2) * (1 if k % 2 else -1) * math.radians(15.0)
                x = item.x + math.cos(base + turn) * r
                y = item.y + math.sin(base + turn) * r
                if self.problem(item, x, y) is None:
                    self.log_nudged.append(f'{item.id} ({item.mesh}) {r:.0f} cm off {reason}')
                    return self.accept(item, x, y)
            r += STEP
        if item.cls == 'hedge' and reason.startswith('the road'):
            # A hedge leaves a gap where a road or path runs through it.
            self.log_dropped.append(f'{item.id} ({item.mesh}) at ({item.x:.0f}, {item.y:.0f}): the hedge\'s gap where '
                                    f'{reason[10:-1]} crosses it')
            return None
        self.log_dropped.append(f'{item.id} ({item.mesh}) at ({item.x:.0f}, {item.y:.0f}): {reason}, '
                                f'no free spot within {reach / 100.0:.1f} m')
        return None

    def accept(self, item, x, y):
        item.x, item.y = x, y
        box, _ = self.footprint(item, x, y)
        tree = item.cls == 'tree' or item.mesh == 'DeadTree_A'
        crown = self.meshes.crown(item.mesh, item.scale) if tree else 0.0
        self.add(Placed(item, box, item.solid, crown))
        return x, y


# ---------------------------------------------------------------------------
# Reading the concept
# ---------------------------------------------------------------------------

def kit_of(model):
    """'k:oak:2' -> ('oak', 2); a game model -> (None, None)."""
    if not model.startswith('k:'):
        return None, None
    parts = model.split(':')
    return parts[1], (int(parts[2]) if len(parts) > 2 and parts[2].isdigit() else 0)


def house_frames(concept, layout, meshes):
    """Each house's frame in the concept and in the layout (where Main stands it), with its size for the yard."""
    out = {}
    places = {p['id']: p for p in layout['placements']}
    for hid, opts in HOUSES.items():
        p = places[hid]
        kind = p['kind']
        x, y = p['location'][:2]
        best = min((c for c in concept['placements'] if c['model'] == kind),
                   key=lambda c: math.dist((c['X'], c['Y']), (x, y)))
        cs = best.get('scale', 1.0)
        b = meshes.bounds[kind]
        out[hid] = dict(kind=kind, opts=opts, concept=(best['X'], best['Y'], best['yaw'], cs),
                        layout=(x, y, p.get('yaw', 0.0), p.get('scale', 1.0)),
                        front=b['max'][0] * cs, back=-b['min'][0] * cs, half=(b['max'][1] - b['min'][1]) * 0.5 * cs)
    return out


def yard_slots(h):
    """The pieces house() put round a house, in its concept frame: (model family, forward, right)."""
    o, front, back, half = h['opts'], h['front'], h['back'], h['half']
    slots = []
    if o.get('bench', True):
        slots.append(('Bench', front + 80.0, half * 0.45))
    slots.append((o.get('lamp', 'LampPost'), front + 110.0, -half - 60.0))
    if o.get('firewood', True):
        slots.append(('FirewoodStack', 0.0, -half - 90.0))
    if o.get('barrels', True):
        slots += [('Barrel', -back - 70.0, half * 0.6), ('Barrel', -back - 70.0, half * 0.6 + 85.0),
                  ('Crate_A', -back - 60.0, half * 0.6 - 110.0)]
    if o.get('laundry'):
        slots.append(('LaundryLine', -back - 120.0, half + 260.0))
    return slots


def garden_centre(h):
    gd = h['opts']['garden'][1]
    x, y, yaw, _ = h['concept']
    return to_world(x, y, yaw, -h['back'] - 180.0 - gd * 0.5, 0.0)


def move_with(h, px, py, yaw=None):
    """A point (and yaw) of a house's yard, from where the concept stood the house to where the layout stands it."""
    cx, cy, cyaw, _ = h['concept']
    lx, ly, lyaw, _ = h['layout']
    ax, ay = to_local(cx, cy, cyaw, px, py)
    nx, ny = to_world(lx, ly, lyaw, ax, ay)
    return nx, ny, (None if yaw is None else (yaw + lyaw - cyaw) % 360.0)


def region(world, x, y):
    """The group a piece belongs to by where it stands (the contract's groups)."""
    if math.dist((x, y), HOLLOW) <= HOLLOW_R:
        return 'hollow'
    if math.dist((x, y), WALLOW) <= WALLOW_R:
        return 'wallow'
    if inside(world.plateau, x, y):
        return 'plateau'
    (cx, cy), (rx, ry) = world.ponds['pond']
    if ((x - cx) / (rx + 800.0)) ** 2 + ((y - cy) / (ry + 800.0)) ** 2 <= 1.0:
        return 'pond'
    for zone in ('farmstead', 'orchard', 'pasture'):
        poly = world.zones[zone]
        if inside(poly, x, y) or nearest_on(poly, (x, y), closed=True)[1] < 400.0:
            return 'farm'
    if inside(world.zones['range'], x, y) or nearest_on(world.zones['range'], (x, y), closed=True)[1] < 600.0 \
            or math.dist((x, y), (6200.0, -3800.0)) < 1500.0:
        return 'range'
    outside_square = abs(x) > SQUARE or abs(y) > SQUARE
    range_road = next(pts for name, pts, _ in world.roads if name == 'range_road')
    farm_road = next(pts for name, pts, _ in world.roads if name == 'farm_road')
    if outside_square and nearest_on(range_road, (x, y))[1] < 900.0 and x > 0.0:
        return 'range'
    if outside_square and nearest_on(farm_road, (x, y))[1] < 1000.0 and x < 0.0:
        return 'farmroad'
    if inside(world.zones['town'], x, y):
        return 'square'
    return 'green'


def gate_box(line, gate):
    """A gate's way through: the gap along its line (at least the picket gate unit's 2 m), GATE_DEPTH either side."""
    pts = line['points'] + ([line['points'][0]] if line['closed'] else [])
    gx, gy, gw = gate
    seg = min(zip(pts, pts[1:]), key=lambda s: seg_point(s[0], s[1], (gx, gy))[1])
    yaw = math.degrees(math.atan2(seg[1][1] - seg[0][1], seg[1][0] - seg[0][0]))
    return Box(gx, gy, yaw, max(gw, 200.0) * 0.5, GATE_DEPTH)


def rim_runs(points, closed, outline):
    """The parts of a line at least RIM_CLEAR inside the island's rim, as polylines (runs shorter than 1.5 m left
    out), or None when the whole line is. A loop's run across its seam stays one run."""
    track = list(points) + ([points[0]] if closed else [])
    total = length(track)
    count = max(1, int(math.ceil(total / RIM_STEP)))
    ok = []
    for i in range(count + 1):
        p = point_at(track, total * i / count)[0]
        ok.append(inside(outline, *p) and nearest_on(outline, p, closed=True)[1] >= RIM_CLEAR)
    if all(ok):
        return None
    spans, i = [], 0
    while i <= count:
        if ok[i]:
            j = i
            while j + 1 <= count and ok[j + 1]:
                j += 1
            spans.append([total * i / count, total * j / count])
            i = j + 1
        else:
            i += 1
    if closed and len(spans) > 1 and ok[0] and ok[count]:
        first = spans.pop(0)
        spans[-1][1] = total + first[1]
    round_track = track + track[1:] if closed else track
    return [cut(round_track, d0, d1) for d0, d1 in spans if d1 - d0 >= 150.0]


def concept_sections(points, unit):
    """The viewer's fence sections (app.js expandFences): each edge in equal pieces about unit long, numbered on."""
    out = []
    for a, b in zip(points, points[1:]):
        span = math.dist(a, b)
        if span < 40.0:
            continue
        n = max(1, int(math.floor(span / unit + 0.5)))
        for j in range(n):
            pa = (a[0] + (b[0] - a[0]) * j / n, a[1] + (b[1] - a[1]) * j / n)
            pb = (a[0] + (b[0] - a[0]) * (j + 1) / n, a[1] + (b[1] - a[1]) * (j + 1) / n)
            out.append((pa, pb))
    return out


# ---------------------------------------------------------------------------
# The build
# ---------------------------------------------------------------------------

class Town:
    """What the build works on, step by step (main() runs the steps in order): the inputs, the world the pieces must
    fit in, the houses and their yards, the pieces and lines as each step leaves them, and what the report tells."""

    def __init__(self):
        self.concept = load(CONCEPT_PATH)
        self.layout = load(LAYOUT_PATH)
        self.computed = load(COMPUTED_PATH)
        if not os.path.exists(BOUNDS_PATH):
            raise SystemExit(f'{BOUNDS_PATH} is missing: write it from the editor first (Saved/mesh_bounds.py)')
        self.meshes = Meshes(load(BOUNDS_PATH))
        self.limbs = dead_tree_limbs()
        self.world = World(self.concept, self.layout, self.computed, self.meshes)
        self.houses = house_frames(self.concept, self.layout, self.meshes)
        self.road_list = [(name, pts, h) for name, pts, h in self.world.roads if name != 'ramp']
        self.fixes, self.warnings = [], []
        self.existing, self.deferred, self.dropped_forest, self.main_placed = Counter(), Counter(), Counter(), Counter()
        self.items, self.lines, self.webs, self.no_trees, self.final = [], [], [], [], []
        self.sacs, self.cocoons, self.dead_trees, self.wallow_fence = [], [], [], []
        self.order = 0
        self.counters = Counter()
        self.placer = None


def classify(t):
    """The concept's placements sorted into what they become: Main's or today's (left out), lines, hedges, deferred,
    the forest's, or a piece to place (moved with its house's yard where the house moved)."""
    concept, houses, world, meshes = t.concept, t.houses, t.world, t.meshes
    fixes, warnings, existing, deferred = t.fixes, t.warnings, t.existing, t.deferred
    main_placed, dropped_forest = t.main_placed, t.dropped_forest
    today = {'spawn', 'farmhouse', 'barn', 'well', 'gun_rack', 'windmill', 'lookout', 'dummy_1', 'dummy_2', 'dummy_3'}
    places = t.layout['placements']
    orchard = [tuple(tree[:2]) for row in t.computed['orchardRows'] for tree in row['trees']]

    # --- Which house each yard piece belongs to (so it moves and groups with it). ---
    yard_of = {}
    for i, p in enumerate(concept['placements']):
        model = p['model']
        family = 'Barrel' if model.startswith('Barrel') else model
        for hid, h in houses.items():
            x, y, yaw, _ = h['concept']
            if any(s[0] == family and math.dist(to_world(x, y, yaw, s[1], s[2]), (p['X'], p['Y'])) < YARD_MATCH
                   for s in yard_slots(h)):
                yard_of[i] = hid
                break

    # --- The concept's placements, sorted into what they become. ---
    items = []
    sacs, cocoons, dead_trees = [], [], []
    wallow_fence = []
    order = 0
    for i, p in enumerate(concept['placements']):
        model, x, y, yaw = p['model'], float(p['X']), float(p['Y']), float(p.get('yaw', 0.0))
        scale = p.get('scale', 1.0)
        kit, variant = kit_of(model)
        order += 1
        # Buildings: Main's (layout.json) or today's island's.
        match = next((q for q in places if q['kind'] == model and math.dist(q['location'][:2], (x, y)) < 60.0), None)
        if match is None and model == 'Farmhouse':
            # The farm road's farmhouse, which the layout stands 10 m further back (fix 1).
            match = next(q for q in places if q['id'] == 'farmhouse_farmroad')
            fixes.append(f"fix 1: the farm road's farmhouse ({x:.0f}, {y:.0f}) yaw {yaw:.1f} is layout.json's "
                         f"farmhouse_farmroad at {tuple(match['location'][:2])} yaw {match['yaw']}")
        if match is not None:
            if match['id'] in today:
                existing[f'{model} (today\'s {match["id"]})'] += 1
            else:
                main_placed[f'{model} ({match["id"]})'] += 1
            continue
        if model in EXISTING_MODELS:
            existing[f'{model} (the jetty, skiff and bridge build_area.py places)'] += 1
            continue
        if kit == 'dummy':
            existing["k:dummy (today's target dummies)"] += 1
            continue
        if model in LINE_MODELS:
            near_wallow = math.dist((x, y), WALLOW) <= WALLOW_R
            if near_wallow and model in ('FenceRail', 'FenceBroken'):
                wallow_fence.append((x, y, yaw, model))
            continue
        if kit == 'hedge':
            continue
        if kit in DEFERRED_KITS:
            deferred[f'k:{kit}'] += 1
            continue
        if kit == 'apple' and any(math.dist(t, (x, y)) < 60.0 for t in orchard):
            existing["k:apple (today's orchard rows, layout_computed.json orchardRows)"] += 1
            continue
        if kit == 'reeds' or model == 'Reeds_B':
            (cx, cy), (rx, ry) = world.ponds['pond']
            if ((x - cx) / (rx + 500.0)) ** 2 + ((y - cy) / (ry + 500.0)) ** 2 <= 1.0:
                existing[f"{model} (the pond's shore reeds the scatter grows)"] += 1
                continue
        if model == 'LilyPads_A':
            if 'Z' in p and not POND_PADS:
                existing["LilyPads_A (the pond's lily pads the scatter floats)"] += 1
                continue
        if model in ROCKS:
            tinted = 'tint' in p
            if not tinted and math.dist((x, y), HOLLOW) > HOLLOW_R:
                existing[f"{model} (the viewer's meadow rocks: the scatter lays the island's)"] += 1
                continue
        if kit == 'cocoon':
            cocoons.append(dict(x=x, y=y, yaw=yaw, lift=float(p.get('lift', 240.0)), order=order))
            continue
        if kit == 'sacs':
            sacs.append(dict(x=x, y=y, yaw=yaw, order=order))
            continue
        mesh = model
        if kit:
            options = KIT_MESH.get(kit)
            if options is None:
                deferred[f'k:{kit}'] += 1
                continue
            mesh = options[min(variant, len(options) - 1)]
        if not meshes.has(mesh):
            deferred[model] += 1
            warnings.append(f'{model}: no game mesh {mesh}')
            continue
        # The concept's woods round Web Hollow stay as pieces: the scatter's forest grew densest on the rise's middle,
        # which is now the hollow's clearing (its no-tree box), so left to the scatter the woods came out bare
        # (2026-10-08: three scattered trees on the whole island). The concept puts no live tree in the clearing.
        extra = {}
        why = ''
        group = None
        if i in yard_of:
            h = houses[yard_of[i]]
            nx, ny, nyaw = move_with(h, x, y, yaw)
            if math.dist((nx, ny), (x, y)) > 1.0:
                fixes.append(f'fix 1: {model} of {yard_of[i]}\'s yard moves with the house, ({x:.0f}, {y:.0f}) yaw '
                             f'{yaw:.1f} -> ({nx:.0f}, {ny:.0f}) yaw {nyaw:.1f}')
            x, y, yaw = nx, ny, nyaw
            group = h['opts']['group']
        if mesh in TREES:
            group = 'green'
        if mesh in BUSHES:
            group = 'hollow' if math.dist((x, y), HOLLOW) <= HOLLOW_R else 'green'
            extra['solid'] = False
        if mesh in REEDS:
            extra['solid'] = False
        if p.get('sunk'):
            extra['sink'] = float(p['sunk'])
        if p.get('tilt'):
            extra['tilt'] = [0.0, 14.0]   # the bogged cart: the concept's 14.3 degree roll, as the contract asks
        if mesh == 'LilyPads_A':
            pool = min(world.ponds, key=lambda k: math.dist(world.ponds[k][0], (x, y)))
            extra['onWater'] = pool
            extra['solid'] = False
        if group is None:
            group = region(world, x, y)
        if mesh == 'DeadTree_A':
            group = 'hollow'
        item = Item(mesh, x, y, yaw, group, order, scale, why, **extra)
        item.yard = yard_of.get(i)
        if mesh == 'DeadTree_A':
            dead_trees.append(item)
        items.append(item)
    t.items, t.sacs, t.cocoons, t.dead_trees, t.wallow_fence, t.order = \
        items, sacs, cocoons, dead_trees, wallow_fence, order


def square_fixes(t):
    """Fixes 2 and 3 in the square: the corner lamp groups, the north-east lamp, the saloon's hitch rail and trough."""
    items, world, meshes, fixes = t.items, t.world, t.meshes, t.fixes
    places = t.layout['placements']

    # --- Fix 2: the corner lamps that stood in porches, and what stands with them, move in along their diagonals. ---
    for (ox, oy), (nx, ny) in CORNER_MOVES:
        dx, dy = nx - ox, ny - oy
        lamp = next(it for it in items if it.mesh == 'LampPost' and math.dist((it.x, it.y), (ox, oy)) < 1.0)
        group = [lamp] + [it for it in items if it is not lamp and it.cls == 'small'
                          and math.dist((it.x, it.y), (ox, oy)) <= CORNER_GROUP_R]
        for it in group:
            fixes.append(f'fix 2: {it.mesh} of the corner lamp group at ({ox:.0f}, {oy:.0f}) moves in along the '
                         f'diagonal, ({it.x:.0f}, {it.y:.0f}) -> ({it.x + dx:.0f}, {it.y + dy:.0f})')
            it.x, it.y = it.x + dx, it.y + dy

    # --- Fix 3: the north-east corner lamp off the forest road; the saloon's hitch rail and trough off its porch. ---
    forest_road = next(pts for name, pts, _ in world.roads if name == 'forest')
    half = next(h for name, _, h in world.roads if name == 'forest')
    lamp = next(it for it in items if it.mesh == 'LampPost' and math.dist((it.x, it.y), NE_LAMP) < 1.0)
    q, d = nearest_on(forest_road, (lamp.x, lamp.y))
    want = half + LAMP_OFF_ROAD
    ux, uy = (lamp.x - q[0]) / d, (lamp.y - q[1]) / d
    nx, ny = q[0] + ux * want, q[1] + uy * want
    fixes.append(f'fix 3: the north-east corner lamp stood {d - half:.0f} cm off the forest road\'s edge (its post on '
                 f'the surface): ({lamp.x:.0f}, {lamp.y:.0f}) -> ({nx:.0f}, {ny:.0f}), {LAMP_OFF_ROAD:.0f} cm off it')
    lamp.x, lamp.y = nx, ny
    saloon = next(p for p in places if p['id'] == 'saloon')
    sx, sy = saloon['location'][:2]
    syaw = saloon['yaw']
    porch = meshes.bounds['FalseFront_Saloon']['max'][0]
    door = DOORS['FalseFront_Saloon'][0][3] + DOOR_SIDE
    rail_half = (meshes.bounds['HitchRail']['max'][1] - meshes.bounds['HitchRail']['min'][1]) * 0.5
    trough_half = (meshes.bounds['WaterTrough']['max'][1] - meshes.bounds['WaterTrough']['min'][1]) * 0.5
    # The rail beside the doors, 1.6 m out from the porch; the trough in front of it, where a tied horse drinks.
    rail_x, rail_y = porch + DOOR_REACH + 10.0, -(door + 10.0 + rail_half)
    trough_depth = (meshes.bounds['WaterTrough']['max'][0] - meshes.bounds['WaterTrough']['min'][0]) * 0.5
    rail_depth = (meshes.bounds['HitchRail']['max'][0] - meshes.bounds['HitchRail']['min'][0]) * 0.5
    spots = {'HitchRail': (rail_x, rail_y), 'WaterTrough': (rail_x + rail_depth + 35.0 + trough_depth, rail_y)}
    saloon_box = meshes.mesh_box('FalseFront_Saloon', sx, sy, syaw)
    for mesh in ('HitchRail', 'WaterTrough'):
        it = next(t for t in items if t.mesh == mesh and math.dist((t.x, t.y), (sx, sy)) < 1600.0)
        line_x, side_y = spots[mesh]
        nx, ny = to_world(sx, sy, syaw, line_x, side_y)
        gap = sat_gap(meshes.box(mesh, it.x, it.y, it.yaw), saloon_box)
        where = 'touching its porch' if abs(gap) < 5.0 else \
            f'{-gap:.0f} cm into its porch' if gap < 0.0 else f'{gap:.0f} cm off its porch'
        fixes.append(f'fix 3: the saloon\'s {mesh} stood end-on to the porch, {where}, at ({it.x:.0f}, {it.y:.0f}) yaw '
                     f'{it.yaw:.0f}: now parallel to the front, {line_x - porch:.0f} cm out, '
                     f'{"beside the doors" if mesh == "HitchRail" else "in front of the hitch rail"}, at '
                     f'({nx:.0f}, {ny:.0f}) yaw {syaw:.0f}')
        it.x, it.y, it.yaw = nx, ny, syaw % 360.0


def concept_lines(t):
    """The concept's fences and walls as lines, with the gates its gap index leaves (gardens moved with their houses),
    and the Wallow's broken fence."""
    concept, houses, fixes, warnings = t.concept, t.houses, t.fixes, t.warnings
    lines = []
    for k, f in enumerate(concept['fences']):
        kit = f['kind']
        pts = [tuple(map(float, p)) for p in f['points']]
        closed = len(pts) > 3 and math.dist(pts[0], pts[-1]) < 1.0
        unit = 200.0 if kit == 'picket' else 300.0
        sections = concept_sections(pts, unit)
        gates = []
        if f.get('gap') is not None and kit != 'stone' and len(pts) > 3:
            a, b = sections[f['gap']]
            gates.append(((a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5, math.dist(a, b)))
        broken = round(len(f.get('broken', [])) / len(sections), 3) if f.get('broken') else 0.0
        c = centroid(pts[:-1] if closed else pts)
        owner = None
        if kit == 'picket':
            owner = min((hid for hid, h in houses.items() if h['opts'].get('garden')),
                        key=lambda hid: math.dist(garden_centre(houses[hid]), c))
            if math.dist(garden_centre(houses[owner]), c) > 60.0:
                warnings.append(f'fence {k + 1}: no house\'s garden at ({c[0]:.0f}, {c[1]:.0f})')
                owner = None
        if owner:
            h = houses[owner]
            moved = [move_with(h, *p)[:2] for p in pts]
            if math.dist(moved[0], pts[0]) > 1.0:
                fixes.append(f'fix 1: {owner}\'s garden fence moves with the house (its middle ({c[0]:.0f}, '
                             f'{c[1]:.0f}) -> ({centroid(moved[:-1])[0]:.0f}, {centroid(moved[:-1])[1]:.0f}))')
            pts = moved
            gates = [move_with(h, gx, gy)[:2] + (gw,) for gx, gy, gw in gates]
            lid, group = f'garden_{owner}', h['opts']['group']
        elif kit == 'stone':
            lid, group = 'windmill_wall', 'range'
        elif f.get('broken'):
            lid, group = 'range_fence', 'range'
        elif closed:
            lid, group = 'pasture_fence', 'farm'
        else:
            lid, group = 'orchard_fence', 'farm'
        lines.append(dict(id=lid, kit=kit, points=pts[:-1] if closed else pts, closed=closed, gates=gates,
                          broken=broken, group=group, owner=owner))
    # The Wallow's broken fence: the concept's three loose sections (two broken, one whole) along the bog's north side,
    # each its own short line between posts, broken or not as the concept drew it.
    for n, (x, y, yaw, model) in enumerate(sorted(t.wallow_fence, key=lambda f: f[1])):
        ax, ay = x, y
        bx, by = to_world(x, y, yaw, 0.0, -300.0)     # a section runs along its -Y from its first post
        lines.append(dict(id=f'wallow_fence_{n + 1}', kit='rail', points=[(ax, ay), (bx, by)], closed=False, gates=[],
                          broken=1.0 if model == 'FenceBroken' else 0.0, group='wallow', owner=None))
    t.lines = lines


def clip_lines(t):
    """Lines keep 4 m inside the island's rim, as pieces do: the concept drew the orchard fence's south run on past
    the rim, where it found no ground. A line is cut where it comes nearer; a loop cut open becomes runs."""
    lines, world, fixes = t.lines, t.world, t.fixes
    kept = []
    for line in lines:
        runs = rim_runs(line['points'], line['closed'], world.outline)
        if runs is None:
            kept.append(line)
            continue
        before = length(line['points'] + ([line['points'][0]] if line['closed'] else []))
        after = sum(length(r) for r in runs)
        fixes.append(f'rim: {line["id"]} ran within {RIM_CLEAR / 100.0:.0f} m of the island\'s rim (or past it): cut to '
                     f'{len(runs)} run(s), {before / 100.0:.1f} -> {after / 100.0:.1f} m')
        for n, run in enumerate(runs):
            gates = [g for g in line['gates'] if nearest_on(run, g[:2])[1] < 5.0]
            kept.append(dict(line, id=line['id'] if len(runs) == 1 else f'{line["id"]}_{n + 1}', points=run,
                             closed=False, gates=gates))
    t.lines = kept


def gardens_off_roads(t):
    """A garden whose far corner reaches onto a path (the farmstead's onto the orchard path, the south-east cabin's
    onto the plateau path) is made shallower from its far side, the side away from its house, until it clears every
    road."""
    houses, road_list, fixes, warnings = t.houses, t.road_list, t.fixes, t.warnings
    for line in t.lines:
        if not line['owner'] or not line['closed']:
            continue
        h = houses[line['owner']]
        hx, hy = h['layout'][:2]
        pts = line['points']
        c = centroid(pts)
        ux, uy = (c[0] - hx) / math.dist(c, (hx, hy)), (c[1] - hy) / math.dist(c, (hx, hy))
        t0 = min((p[0] - hx) * ux + (p[1] - hy) * uy for p in pts)

        def shrink(p, f):
            t = (p[0] - hx) * ux + (p[1] - hy) * uy
            return p[0] - ux * (t - t0) * (1.0 - f), p[1] - uy * (t - t0) * (1.0 - f)

        def clear(poly):
            return all(poly_line_dist(poly, rpts) >= rh + 30.0 for _, rpts, rh in road_list)

        if clear(pts):
            continue
        for k in range(1, 21):
            f = 1.0 - 0.02 * k
            moved = [shrink(p, f) for p in pts]
            if clear(moved):
                depth = max((p[0] - hx) * ux + (p[1] - hy) * uy for p in pts) - t0
                fixes.append(f'garden: {line["id"]} reached onto a path at its far side: {depth * (1.0 - f):.0f} cm '
                             f'shallower ({depth / 100.0:.1f} -> {depth * f / 100.0:.1f} m deep, the house side kept)')
                line['points'] = moved
                line['gates'] = [shrink((gx, gy), f) + (gw,) for gx, gy, gw in line['gates']]
                break
        else:
            warnings.append(f'{line["id"]} stays on a path: no depth down to 60% clears it')


def gardens_apart(t):
    """Gardens keep GARDEN_GAP apart and off each other's gates: the farm road farmhouse's garden, moved with its
    house, came within 0.3 m of the farm road cabin's, right across that garden's gate. The garden whose house moved is
    made narrower from the side facing the other one (its far side kept)."""
    houses, fixes, warnings = t.houses, t.fixes, t.warnings

    def gap_between(p, q):
        return min(seg_seg_dist(a, b, c, d) for a, b in zip(p, p[1:] + p[:1]) for c, d in zip(q, q[1:] + q[:1]))

    gardens = [line for line in t.lines if line['owner'] and line['closed']]
    for a in gardens:
        for b in gardens:
            if a is b:
                continue

            def apart(poly, other=b):
                gate = min((box_line_dist(gate_box(other, g), poly, 1e9, True) for g in other['gates']), default=1e9)
                return gap_between(poly, other['points']) >= GARDEN_GAP and gate >= 50.0

            h = houses[a['owner']]
            moved = math.dist(h['concept'][:2], h['layout'][:2]) > 1.0
            if apart(a['points']) or not moved:
                continue
            hx, hy, hyaw, _ = h['layout']
            vx, vy = rot(0.0, 1.0, hyaw)              # across the house (its right)
            pts = a['points']
            side = 1.0 if (centroid(b['points'])[0] - centroid(pts)[0]) * vx + \
                (centroid(b['points'])[1] - centroid(pts)[1]) * vy > 0.0 else -1.0
            lat = [((p[0] - hx) * vx + (p[1] - hy) * vy) * side for p in pts]
            anchor = min(lat)

            def narrow(p, f):
                s = ((p[0] - hx) * vx + (p[1] - hy) * vy) * side
                return p[0] - vx * side * (s - anchor) * (1.0 - f), p[1] - vy * side * (s - anchor) * (1.0 - f)

            for k in range(1, 21):
                f = 1.0 - 0.02 * k
                trial = [narrow(p, f) for p in pts]
                if apart(trial):
                    width = max(lat) - anchor
                    fixes.append(f'fix 1: {a["id"]}, moved with its house, came within '
                                 f'{gap_between(pts, b["points"]):.0f} cm of {b["id"]} and its gate: '
                                 f'{width * (1.0 - f):.0f} cm narrower from that side '
                                 f'({width / 100.0:.1f} -> {width * f / 100.0:.1f} m)')
                    a['points'] = trial
                    a['gates'] = [narrow((gx, gy), f) + (gw,) for gx, gy, gw in a['gates']]
                    break
            else:
                warnings.append(f'{a["id"]} stays within {GARDEN_GAP / 100.0:.0f} m of {b["id"]}')


def road_gates(t):
    """A gate wherever a road crosses a line, as wide as the road takes up along it (build_area_dressing's ROAD_GAPS
    rule), so no fence closes a road."""
    for line in t.lines:
        pts = line['points'] + ([line['points'][0]] if line['closed'] else [])
        for name, rpts, h in t.road_list:
            for p, sine in crossings(pts, rpts):
                width = max(h * 2.0, 250.0) / max(sine, 0.35) + 50.0
                if any(math.dist(p, g[:2]) < (width + g[2]) * 0.5 for g in line['gates']):
                    continue
                line['gates'].append((p[0], p[1], width))
                t.fixes.append(f'gate: {line["id"]} opens {width / 100.0:.1f} m where {name} crosses it, '
                               f'at ({p[0]:.0f}, {p[1]:.0f})')


def register_lines(t):
    """The finished lines, their gates and gardens, as obstacles the pieces keep off."""
    for line in t.lines:
        t.world.lines.append((line['id'], line['kit'], line['points'], line['closed']))
        if line['owner'] and line['closed']:
            t.world.gardens.append((line['id'], line['points']))
        for g in line['gates']:
            t.world.gates.append((line['id'], gate_box(line, g)))


def yards_off_gardens(t):
    """A moved yard's piece that now lands on another house's garden goes to the same spot on the other side of its
    own house (the farm road farmhouse's washing line stood on the farm road cabin's garden)."""
    houses, meshes, lines, fixes = t.houses, t.meshes, t.lines, t.fixes
    for it in t.items:
        if not it.yard or math.dist(houses[it.yard]['concept'][:2], houses[it.yard]['layout'][:2]) < 1.0:
            continue
        box = meshes.box(it.mesh, it.x, it.y, it.yaw, it.scale)
        hit = next((l['id'] for l in lines if l['owner'] and l['closed'] and l['owner'] != it.yard and
                    (any(inside(l['points'], *p) for p in box.samples()) or
                     box_line_dist(box, l['points'], 100.0, True) < 100.0)), None)
        if hit:
            hx, hy, hyaw, _ = houses[it.yard]['layout']
            lx, ly = to_local(hx, hy, hyaw, it.x, it.y)
            nx, ny = to_world(hx, hy, hyaw, lx, -ly)
            fixes.append(f'fix 1: the {it.mesh} of {it.yard}\'s yard landed on {hit}: moved to the other side of the '
                         f'house, ({it.x:.0f}, {it.y:.0f}) -> ({nx:.0f}, {ny:.0f})')
            it.x, it.y, it.yaw = nx, ny, (2.0 * hyaw - it.yaw) % 360.0


def check_lines(t):
    """What the lines still run into, as warnings (a line isn't moved: Main decides)."""
    lines, world, road_list, warnings = t.lines, t.world, t.road_list, t.warnings
    for line in lines:
        pts = line['points'] + ([line['points'][0]] if line['closed'] else [])
        for b in world.buildings:
            d = box_line_dist(b.box, pts, 40.0)
            if d < 40.0:
                warnings.append(f'line {line["id"]} runs {d:.0f} cm from {b.name}')
        for name, spot in world.spots:
            d = box_line_dist(spot, pts, SPOT_CLEAR)
            if d < SPOT_CLEAR:
                warnings.append(f'line {line["id"]} runs {d:.0f} cm from {name}')
        for name, rpts, h in road_list:
            total = length(pts)
            steps = int(total // 50.0) + 1
            for s in range(steps + 1):
                (px, py), _ = point_at(pts, total * s / steps)
                if nearest_on(rpts, (px, py))[1] < h + 20.0 and \
                        not any(math.dist((px, py), g[:2]) < g[2] * 0.5 + 10.0 for g in line['gates']):
                    warnings.append(f'line {line["id"]} runs on {name} at ({px:.0f}, {py:.0f})')
                    break
    for a in range(len(lines)):
        for b in range(a + 1, len(lines)):
            if lines[a]['closed'] and lines[b]['closed'] and polys_overlap(lines[a]['points'], lines[b]['points']):
                warnings.append(f'lines {lines[a]["id"]} and {lines[b]["id"]} overlap')


def no_tree_areas(t):
    """The concept's no-tree areas, the farm road farmhouse's yard circle moved with it and each garden's area where
    its fence now runs."""
    concept, houses, world, fixes = t.concept, t.houses, t.world, t.fixes
    no_trees = []
    garden_polys = {}
    for line in t.lines:
        if line['owner'] and line['closed']:
            garden_polys[line['owner']] = line['points']
    for entry in concept['noTrees']:
        if 'polygon' in entry:
            poly = [tuple(map(float, p)) for p in entry['polygon']]
            c = centroid(poly)
            for hid, h in houses.items():
                if h['opts'].get('garden') and math.dist(garden_centre(h), c) < 60.0 and hid in garden_polys:
                    # The garden's no-tree area is its fence's, where the fence now runs (moved, made shallower).
                    if math.dist(garden_polys[hid][0], poly[0]) > 1.0:
                        fixes.append(f'garden: {hid}\'s garden no-tree area follows its fence')
                    poly = list(garden_polys[hid])
                    break
            no_trees.append({'polygon': [[rnd1(px), rnd1(py)] for px, py in poly]})
            world.no_trees.append(('polygon', poly, None))
        else:
            x, y, r = float(entry['X']), float(entry['Y']), float(entry['r'])
            for hid, h in houses.items():
                if math.dist(h['concept'][:2], (x, y)) < 1.0:
                    nx, ny, _ = move_with(h, x, y)
                    if math.dist((nx, ny), (x, y)) > 1.0:
                        fixes.append(f'fix 1: {hid}\'s yard no-tree circle moves with the house, ({x:.0f}, {y:.0f}) -> '
                                     f'({nx:.0f}, {ny:.0f})')
                    x, y = nx, ny
                    break
            no_trees.append({'at': [rnd1(x), rnd1(y)], 'r': rnd1(r)})
            world.no_trees.append(('circle', (x, y), r))
    t.no_trees = no_trees


def hedges(t):
    """Hedges: rows of bushes along the concept's hedge lines (its k:hedge pieces), about every 1.6 m, each bush's
    kind, size and turn from the hedge's own seeded stream."""
    order, items = t.order, t.items
    hedge_groups = {0: 'farmroad', 1: 'range', 2: 'range'}
    for k, hedge in enumerate(t.concept['hedges']):
        pts = [tuple(map(float, p)) for p in hedge['points']]
        total = length(pts)
        count = max(2, int(round((total - 60.0) / 160.0)) + 1)
        rnd = random.Random(f'hedge {k + 1}')
        for n in range(count):
            d = 30.0 + (total - 60.0) * n / (count - 1) + rnd.uniform(-12.0, 12.0)
            (px, py), (ux, uy) = point_at(pts, max(0.0, min(total, d)))
            side = rnd.uniform(-10.0, 10.0)
            mesh = rnd.choices(BUSHES, weights=(4, 4, 2))[0]
            scale = round(rnd.uniform(0.8, 1.0), 2)
            yaw = round(rnd.uniform(0.0, 360.0), 1)
            order += 1
            items.append(Item(mesh, px - uy * side, py + ux * side, yaw, hedge_groups.get(k, 'green'), order, scale,
                              solid=False, hedge=f'hedge{k + 1}'))
    t.order = order


def name_pieces(t):
    """Ids, in the concept's order within each group and mesh: stable as long as the concept is."""
    for it in sorted(t.items, key=lambda i: i.order):
        t.counters[(it.group, it.mesh)] += 1
        it.id = f'{it.group}_{snake(it.mesh)}_{t.counters[(it.group, it.mesh)]:02d}'


def place_all(t):
    """Every piece placed, highest priority first (CLASSES); Web Hollow's egg sacs at their dead trees' feet, and its
    strands an obstacle from the moment their trees stand."""
    world, meshes, concept, counters = t.world, t.meshes, t.concept, t.counters
    sacs, dead_trees = t.sacs, t.dead_trees
    placer = t.placer = Placer(world)
    final = t.final
    by_class = defaultdict(list)
    for it in t.items:
        by_class[it.cls].append(it)
    for cls in CLASSES:
        if cls == 'small':
            # Web Hollow's egg sac clusters stand at their dead trees' feet, where the trees ended up.
            for n, s in enumerate(sorted(sacs, key=lambda s: s['order'])):
                tree = min(dead_trees, key=lambda t: math.dist((t.x, t.y), (s['x'], s['y'])))
                for k, (mesh, r, bearing, scale) in enumerate(SAC_CLUSTER):
                    a = math.radians(s['yaw'] + bearing)
                    sac = Item(mesh, s['x'] + math.cos(a) * r, s['y'] + math.sin(a) * r, s['yaw'] + bearing * 0.5,
                               'hollow', s['order'], scale, cluster=f'sacs{n + 1}')
                    counters[('hollow', mesh)] += 1
                    sac.id = f'hollow_{snake(mesh)}_{counters[("hollow", mesh)]:02d}'
                    by_class['small'].append(sac)
        for it in sorted(by_class[cls], key=lambda t: (t.order, t.id)):
            if it.cls == 'centre' and it.mesh == 'NoticeBoard':
                spot = placer.place(it)
                if spot:
                    final.append(it)
                    # Its front is a gameplay spot: where townsfolk will stand and the player reads it.
                    b = meshes.bounds['NoticeBoard']
                    fx, fy = to_world(it.x, it.y, it.yaw, b['max'][0] + DOOR_REACH * 0.5, 0.0)
                    world.spots.append(("the notice board's front", Box(fx, fy, it.yaw, DOOR_REACH * 0.5,
                                                                        (b['max'][1] - b['min'][1]) * 0.5)))
                continue
            if placer.place(it):
                final.append(it)
                if it.mesh == 'DeadTree_A':
                    # The strands between the dead trees placed so far: what the pieces after them keep out of.
                    standing = [t for t in dead_trees if t in final]
                    world.strands = []
                    for web in concept['webs']:
                        if web.get('sheet') or len(standing) < 2:
                            continue
                        a = min(standing, key=lambda t: math.dist((t.x, t.y), tuple(web['from'])))
                        b = min(standing, key=lambda t: math.dist((t.x, t.y), tuple(web['to'])))
                        if a is not b:
                            world.strands.append(((a.x, a.y), (b.x, b.y)))


def hollow(t):
    """Web Hollow on its dead trees where they stand: their crowns, the cocoons on their limbs (or at their feet), the
    strands between them and the sheet webs over the burrows."""
    final, counters, fixes, placer = t.final, t.counters, t.fixes, t.placer
    trees = [tree for tree in t.dead_trees if tree in final]
    webs = t.webs
    for tree in trees:
        # The dressing fits Web_Crown on the tree with its own transform (as the Sink's Webwood does).
        tree.extra['crown'] = True
    for c in sorted(t.cocoons, key=lambda c: c['order']):
        tree = min(trees, key=lambda tr: math.dist((tr.x, tr.y), (c['x'], c['y'])))
        spot = hang_spot(tree, c, t.limbs)
        if spot:
            hx, hy, lift, limb = spot
            it = Item('Cocoon_Hung', hx, hy, c['yaw'], 'hollow', c['order'], 1.0, solid=False, lift=round(lift, 1))
            it.extra['hangFrom'] = tree.id
            counters[('hollow', 'Cocoon_Hung')] += 1
            it.id = f'hollow_cocoon_hung_{counters[("hollow", "Cocoon_Hung")]:02d}'
            final.append(it)
            fixes.append(f'cocoon: {it.id} hangs from {tree.id}\'s limb {limb}, its pivot {lift:.0f} cm over the '
                         f'tree\'s foot (bundle {lift - COCOON_DROP:.0f}-{lift - COCOON_KNOT:.0f} cm up; the concept '
                         f'hung it at {c["lift"]:.0f}), at ({hx:.0f}, {hy:.0f})')
        else:
            it = Item('Cocoon_Lying', c['x'], c['y'], c['yaw'], 'hollow', c['order'], 1.0, solid=False)
            counters[('hollow', 'Cocoon_Lying')] += 1
            it.id = f'hollow_cocoon_lying_{counters[("hollow", "Cocoon_Lying")]:02d}'
            if placer.place(it):
                final.append(it)
                fixes.append(f'cocoon: no limb of {tree.id} holds one near ({c["x"]:.0f}, {c["y"]:.0f}): '
                             f'{it.id} lies at its foot')
    for web in t.concept['webs']:
        if web.get('sheet'):
            webs.append({'id': f'hollow_sheet_{sum(1 for w in webs if "sheet" in w) + 1}', 'sheet': True,
                         'at': [rnd1(web['X']), rnd1(web['Y'])], 'r': rnd1(web['r'])})
            continue
        # Two strands crossing between neighbours, from the concept's low and high ends: a web reads from afar where one
        # thread wouldn't. Their ends are the trees' own spots: the dressing ties each into that tree's trunk.
        a = min(trees, key=lambda tr: math.dist((tr.x, tr.y), tuple(web['from'])))
        b = min(trees, key=lambda tr: math.dist((tr.x, tr.y), tuple(web['to'])))
        k = sum(1 for w in webs if 'sheet' not in w) // 2 + 1
        for suffix, ha, hb in (('a', web['high'], web['low']), ('b', web['low'], web['high'])):
            webs.append({'id': f'hollow_web_{k}{suffix}', 'from': [rnd1(a.x), rnd1(a.y), rnd1(ha)],
                         'to': [rnd1(b.x), rnd1(b.y), rnd1(hb)]})


def deferred_counts(t):
    """The concept's pieces that wait for a model: its small things (cobbles, kerb bricks, cabbages, flowers), its folk
    and beasts, its slime trails. Its grass tufts are the scatter's grass, already on the island."""
    for kind, count in t.concept['fineCounts'].items():
        if kind == 'k:tuft':
            t.existing["k:tuft (the scatter's grass)"] += count
        else:
            t.deferred[kind] += count
    for c in t.concept['creatures']:
        if c['kind'] in ('villager', 'sheep', 'chicken'):
            t.deferred[f'creature:{c["kind"]}'] += 1
    t.deferred['slimeTrail'] += len(t.concept['slimeTrails'])


def main():
    t = Town()
    for step in (classify, square_fixes, concept_lines, clip_lines, gardens_off_roads, gardens_apart, road_gates,
                 register_lines, yards_off_gardens, check_lines, no_tree_areas, hedges, name_pieces, place_all, hollow,
                 deferred_counts):
        step(t)
    verify(t.final, t.lines, t.world, t.meshes, t.warnings)
    write(t.final, t.lines, t.webs, t.no_trees, t.deferred)
    report(t)


def verify(final, lines, world, meshes, warnings):
    """The result checked on its own, after everything moved: every piece's whole footprint and every line at least
    RIM_HARD inside the island's rim, no solid piece in a building or on a road outside the memorial's ring, no
    two solid pieces overlapping. Anything found is a warning (and should never be)."""
    def rim_ok(p):
        return inside(world.outline, *p) and nearest_on(world.outline, p, closed=True)[1] >= RIM_HARD

    for it in final:
        if it.mesh == 'Cocoon_Hung':
            continue
        box = meshes.box(it.mesh, it.x, it.y, it.yaw, it.scale)
        if not all(rim_ok(p) for p in box.samples()):
            warnings.append(f'verify: {it.id} reaches within {RIM_HARD / 100.0:.0f} m of the rim')
        if it.solid and it.cls not in ('tree',):
            for b in world.buildings:
                if sat_gap(box, b.box) < 0.0:
                    warnings.append(f'verify: {it.id} stands in {b.name}')
    for line in lines:
        track = line['points'] + ([line['points'][0]] if line['closed'] else [])
        total = length(track)
        count = max(1, int(math.ceil(total / RIM_STEP)))
        if not all(rim_ok(point_at(track, total * i / count)[0]) for i in range(count + 1)):
            warnings.append(f'verify: line {line["id"]} reaches within {RIM_HARD / 100.0:.0f} m of the rim')
    solids = [(it, meshes.box(it.mesh, it.x, it.y, it.yaw, it.scale)) for it in final
              if it.solid and it.mesh != 'Cocoon_Hung']
    for i, (a, ba) in enumerate(solids):
        for b, bb in solids[i + 1:]:
            if (a.cluster and a.cluster == b.cluster) or abs(a.x - b.x) > 2000.0 or abs(a.y - b.y) > 2000.0:
                continue
            if sat_gap(ba, bb) < 0.0:
                warnings.append(f'verify: {a.id} and {b.id} overlap')


# ---------------------------------------------------------------------------
# Web Hollow: where a dead tree's limbs and trunk are
# ---------------------------------------------------------------------------

def densify(points, step=0.1):
    """A limb's samples (Blender m: x, y, z, r) about step apart, each with how far along the limb it is."""
    out = []
    walked = 0.0
    for a, b in zip(points, points[1:]):
        seg = math.dist(a[:3], b[:3])
        n = max(1, int(math.ceil(seg / step)))
        for j in range(n):
            t = j / n
            out.append((tuple(a[i] + (b[i] - a[i]) * t for i in range(4)), walked + seg * t))
        walked += seg
    out.append((tuple(points[-1]), walked))
    return out


def hang_spot(tree, cocoon, limbs):
    """Where a Cocoon_Hung hangs from one of the tree's main limbs: (x, y, pivot height over the tree's foot, limb),
    the limb thick enough, the hanging line clear of the trunk and the other limbs, the pivot 3.3-4.2 m up; the spot
    nearest the concept's height (in that range), then nearest the concept's spot. None when no limb can hold one."""
    s = tree.scale
    dense = {k: densify(v) for k, v in limbs.items()}
    best, best_cost = None, 1e18
    for key in DEAD_LIMBS:
        for (bx, by, bz, br), along in dense[key]:
            if br * s < COCOON_LIMB_R:
                continue
            pivot = (bz - br) * s * 100.0 + 3.0
            bottom = pivot - COCOON_DROP
            if not COCOON_LIFT[0] <= pivot <= COCOON_LIFT[1]:
                continue
            ok = True
            for k2, samples in dense.items():
                for (cx, cy, cz, cr), along2 in samples:
                    z = cz * s * 100.0
                    if z < bottom - 20.0 or z > pivot - 5.0:
                        continue
                    if k2 == key and abs(along2 - along) < 0.35:
                        continue            # the limb itself, where the silk wraps it
                    horiz = math.hypot(cx - bx, cy - by) * s * 100.0
                    need = cr * s * 100.0 + (COCOON_R + 5.0 if z < pivot - COCOON_KNOT else 6.0)
                    if horiz < need:
                        ok = False
                        break
                if not ok:
                    break
            if not ok:
                continue
            lx, ly = blender_to_local(bx * s, by * s)
            wx, wy = to_world(tree.x, tree.y, tree.yaw, lx, ly)
            # The concept's heights (the viewer's bundle middles) kept in order: 2.4 m hangs lowest, 3.2 m highest.
            want = COCOON_LIFT[0] + (cocoon['lift'] - 240.0) * (COCOON_LIFT[1] - COCOON_LIFT[0]) / 80.0
            cost = abs(pivot - want) + 0.3 * math.dist((wx, wy), (cocoon['x'], cocoon['y']))
            if cost < best_cost:
                best, best_cost = (wx, wy, pivot, key), cost
    return best


# ---------------------------------------------------------------------------
# Writing and reporting
# ---------------------------------------------------------------------------

def piece_json(it):
    out = {'id': it.id, 'mesh': OUTPUT_MESH.get(it.mesh, it.mesh), 'at': [rnd1(it.x), rnd1(it.y)], 'yaw': rnd1(it.yaw % 360.0),
           'group': it.group}
    if isinstance(it.scale, (list, tuple)):
        out['scale'] = [round(v, 3) for v in it.scale]
    elif abs(it.scale - 1.0) > 1e-6:
        out['scale'] = round(it.scale, 3)
    for key in ('lift', 'sink', 'tilt', 'onWater'):
        if key in it.extra:
            out[key] = it.extra[key]
    if 'solid' in it.extra:
        out['solid'] = bool(it.extra['solid'])
    out['kind'] = 'flat' if it.mesh == 'LilyPads_A' else 'upright' if it.mesh in UPRIGHT else 'prop'
    for key in ('crown', 'hangFrom'):
        if key in it.extra:
            out[key] = it.extra[key]
    return out


ABOUT = (
    "Crossroads Town's dressing on Skyreach (the tutorial island), round 1: the pieces, fence lines and webs "
    "Tools/Unreal/build_area_dressing.py places when layout.json's level.town names this file, the no-tree areas the "
    "scatter keeps out of, and the concept pieces still waiting for a model. Made by make_town.py (beside this file) "
    "from crossroads_town.json (the concept viewer's Crossroads Town), layout.json, layout_computed.json and "
    "Saved/MeshBounds.json; edit those and run it again, never this file. Unreal cm, X north, Y east, yaw in degrees "
    "from +X (layout.json's convention). pieces: mesh is the game mesh without SM_; at is its pivot; scale uniform or "
    "[sx, sy, sz]; lift cm over the ground (a hung cocoon's: its pivot, its top, on a limb of the dead tree hangFrom "
    "names, the height measured from that tree's foot); sink cm into the ground; tilt [pitch, roll] degrees added; "
    "solid overrides the mesh's default (bushes, reeds, lily pads and cocoons have none); onWater: floats on that "
    "pond's water (layout_computed.json ponds[id].waterZ); kind prop (tilts a little with the ground), upright (plumb) "
    "or flat; crown: a dead tree wears Web_Crown with its own transform. lines: kit rail, picket or stone "
    "(build_area_dressing.KITS), points in order, closed for a loop, gates [X, Y, width] left open (the concept's gaps "
    "and every road crossing), broken the share of sections drawn broken. webs: a strand from [X, Y, up] to [X, Y, up] "
    "(the two dead trees' spots: the dressing ties each end into that tree's trunk, up cm over its foot) or a sheet web "
    "over a burrow (at, r). noTrees: circles and polygons (the concept's, the farm road farmhouse's moved with it). "
    "deferred: concept pieces with no game model yet, by kind, with counts.")


def write(final, lines, webs, no_trees, deferred):
    pieces = sorted((piece_json(it) for it in final), key=lambda p: p['id'])
    line_out = []
    for line in sorted(lines, key=lambda l: l['id']):
        entry = {'id': line['id'], 'kit': line['kit'],
                 'points': [[rnd1(x), rnd1(y)] for x, y in line['points']], 'closed': line['closed'],
                 'gates': [[rnd1(x), rnd1(y), rnd1(w)] for x, y, w in sorted(line['gates'])],
                 'broken': line['broken'], 'group': line['group']}
        line_out.append(entry)
    web_out = sorted(webs, key=lambda w: w['id'])
    dumps = lambda v: json.dumps(v, ensure_ascii=False)  # noqa: E731
    parts = ['{', f'  "about": {dumps(ABOUT)},', '  "pieces": [']
    parts.append(',\n'.join(f'    {dumps(p)}' for p in pieces))
    parts += ['  ],', '  "lines": [', ',\n'.join(f'    {dumps(l)}' for l in line_out), '  ],', '  "webs": [',
              ',\n'.join(f'    {dumps(w)}' for w in web_out), '  ],', '  "noTrees": [',
              ',\n'.join(f'    {dumps(n)}' for n in no_trees), '  ],',
              f'  "deferred": {dumps(dict(sorted(deferred.items())))}', '}']
    with open(OUT_PATH, 'w', encoding='utf-8', newline='\n') as file:
        file.write('\n'.join(parts) + '\n')


def report(t):
    """What was made, left out, fixed, nudged and dropped, for the person running it."""
    final, lines, webs, deferred, placer = t.final, t.lines, t.webs, t.deferred, t.placer
    existing, main_placed, dropped_forest, world = t.existing, t.main_placed, t.dropped_forest, t.world
    fixes, warnings = t.fixes, t.warnings
    print(f'town.json: {len(final)} pieces, {len(lines)} lines, {len(webs)} webs -> {OUT_PATH}')
    groups = Counter(it.group for it in final)
    print('pieces by group: ' + ', '.join(f'{g} {n}' for g, n in sorted(groups.items())))
    meshes = Counter(it.mesh for it in final)
    print('pieces by mesh: ' + ', '.join(f'{m} {n}' for m, n in sorted(meshes.items())))
    kits = Counter(l['kit'] for l in lines)
    print('lines by kit: ' + ', '.join(f'{k} {n}' for k, n in sorted(kits.items())) + ' (' +
          ', '.join(f'{l["id"]}: {len(l["gates"])} gate(s)' for l in sorted(lines, key=lambda l: l['id'])) + ')')
    print(f'webs: {sum(1 for w in webs if "sheet" not in w)} strands, {sum(1 for w in webs if "sheet" in w)} sheets')
    print('deferred: ' + ', '.join(f'{k} {n}' for k, n in sorted(deferred.items())))
    print("Main's (layout.json): " + ', '.join(f'{k} {n}' for k, n in sorted(main_placed.items())))
    print('already on the island: ' + ', '.join(f'{k} {n}' for k, n in sorted(existing.items())))
    print('left to the forest\'s scatter: ' + ', '.join(f'{k} {n}' for k, n in sorted(dropped_forest.items())) +
          f' ({sum(dropped_forest.values())})')
    if world.gone:
        print('layout placements without a mesh (not obstacles): ' + ', '.join(world.gone))
    print(f'fixes ({len(fixes)}):')
    for line in fixes:
        print('  ' + line)
    print(f'nudged ({len(placer.log_nudged)}):')
    for line in placer.log_nudged:
        print('  ' + line)
    print(f'dropped ({len(placer.log_dropped)}):')
    for line in placer.log_dropped:
        print('  ' + line)
    print(f'warnings ({len(warnings)}):')
    for line in warnings:
        print('  ' + line)


if __name__ == '__main__':
    main()
