"""Life on the cliffs (the "make it alive" polish, concept B, look sheet 43): the plants, drapes and swallows' nests of
Art/Models/Vegetation/CliffLife.py, the scree fans and fallen slabs of Art/Models/Rocks/CliffScree.py, and the stain decals
(T_CliffStains, MI_CliffStain: Tools/Unreal/build_cliff_stain_material.py) on the cliff pieces build_area.py placed. For
build_area.py's full build, or on its own in the open editor (replaces only what it placed before, then saves the level):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_cliff_life.py RansomsRest" -Until "CLIFFLIFE done"
Saved/cliff_life_probe.py counts the spots per kind per cliff run without placing anything: tune level.cliffLife with it.

Opt-in: only an area whose layout.json has a level.cliffLife block gets any (its keys over DEFAULTS below; a kind's block
replaces only the keys it names, its "meshes" whole). An area with no cliff pieces places nothing.

What it dresses: every cliff piece the build placed and tagged (Cliff_<group>_<nn>[_<course>]: the faces and a pit's
panels; Outcrop_<group>: a knob's rock, looked at from six sides), unless it stands more than `reach` cm past the playable
boundary (layout_computed.json boundary) or no player can see it. A side is seen when, from a point `views` cm out in front
of it (straight out or `viewTurns` degrees off), inside the boundary, at `eye` cm over the terrain, a line to its face at
40% or 75% of its height meets no terrain and no rock first. Drapes and lip stains go only on a point's top course, fans
and slabs only at a bottom course's foot.

Spots, found by casts against the rock (the cliff pieces and outcrops, complex collision; the terrain's tiles and the other
placed models stop a cast but are never a spot). The kit's author's rules; Tools/Blender/cliff_life_sheet.py's find_* are
the reference:
- Lips (LipDrape_A/B/C): every 10 cm along a side, a level cast 40 cm under its top finds its front; from 15 cm in front of
  that, walking in 5 cm at a time (to 85 cm behind it), a cast down from over the top first meets rock facing up (normal
  Z > 0.5) within 1.1 m of the top. A drape fits where at least 80% of the lip points across its width (its own width
  times a scale drawn in `scale`, along the lip: the actor's Y) are found, 80% of those lie within 25 cm in/out and 35 cm
  up/down of their medians, none stands more than 35 cm in front of the median, and casts into the face 0.4, 1.0 and
  1.6 m under it, at its middle and 35% of its width either side, meet rock no more than 22 cm in front of the lip. The
  face under it is its hang plus 0.5 m tall or more. Pivot: the run's middle at the medians, turned square to the side.
- Ledges (LedgeSage_A/B, LedgeGrass_A/B, CliffJuniper_B/C): casts 50 degrees down from 6 m out, every `grid` cm across and
  up, keep rock facing up (normal Z > 0.8) 1 m or more over the ground and 40 cm or more under the top, with 1 m free out
  at 25 cm up and room over it (LEDGE_FIT; 2.5 m for CliffJuniper_B). Its edge (down casts 10-60 cm out) picks what fits:
  LedgeGrass_B from 15 cm, the rest from 30. Turned within 17 degrees of the side's out, leaned with the ledge up to 15
  degrees, 2 cm down.
- Cracks (CliffJuniper_A/C): level casts every 5 cm along rows at `rows` shares of the face's height (1.5 m or more over
  the ground, 1.2 m under the top); a joint is the deepest point within 30 cm either side with rock 8 cm or more proud of
  it on both sides (the sheet's rule asked one side only: the foot of a lone rib passed). Turned to its normal, within 35
  degrees of the side's out; clear room per CRACK_FIT (out, up, and no rock jutting 40 cm out over the joint at the
  crown's height).
- Feet (ScreeFan_A/B/C, FallenSlab_A/B): every fans `step` cm along a bottom course, a level cast 30 cm over the ground in
  front (60, 100, 150 where the terrain rises into the face) meets the face's foot. A fan's pivot is 15 cm into the wall
  on the terrain just in front of it, tilted to a plane through nine terrain points over its spread (up to `tilt`
  degrees; none where they stray 45 cm from it or it slopes past 28 degrees). _B where the foot is a notch (its middle 30
  cm deeper than its ends), else drawn among those whose spread allows (FANS). Slabs 3-5.5 m out from a foot (`out`), on
  ground that strays less than 35 cm under them, any turn, a `chance` per foot.
- Overhangs (SwallowNests_A/B): level casts every 15 cm up columns 40 cm apart, from `least` cm over the ground; where the
  face just above stands 15 cm further out, a cast up from 6 cm off the face meets a ceiling (normal Z < -0.6) within
  0.6 m, still overhead 25 cm further out; a level cast 3 cm under it finds the back wall. A corner takes SwallowNests_A
  (or _B 5 cm out of it), a ceiling alone _B.
- Stains (decals): under lips every `every` cm (varnish cells 0-4 and the wash 6), seeps under ledges (5: the cell's seep
  line, a quarter down, 10 cm under the ledge) and whitewash under each nest (7: its top 15 cm under the ceiling). The
  decal's X into the face, its Y up (the cell's top row at the decal's top), its top 5 cm under the lip; width and height
  drawn per cell (`cells`: [weight, [width], [height]], cm), the height cut to the face; its box as deep as nine casts
  across it find the face, 40 cm either side (50-160 cm half depth). Its colour picks the cell, R = (cell + 0.5) / 8, and
  its A is its strength (`strength`).

Choosing: the kinds in order (drapes, cracks, ledges, nests, fans, slabs, stains), each spot ranked by a draw of its own
(seeded from `seed`, the kind and its place, so a rebuild places the same), weighted toward paths (`pathBias`,
`pathReach`: the layout's roads and ramps) and sides seen from near; then taken while the kind's `cap` (the whole area)
and `perPiece` allow, `spacing` cm from the kind's others and clear of the kinds chosen before it (no plant or nest under
a drape's hang, PLANT_ROOM between plants, NEST_ROOM round a nest, SLAB_ROOM from a fan). Nothing stands in a building
players walk into (level.cliffs.keepOut); the solid pieces (fans, slabs, a juniper low on a face) also keep off the
roads, ramps, obstacles, placed models and gameplay pieces (`keepOff`, cm) and anything solid standing there already.

Placed: one AInstancedProps per mesh (CliffLife_<mesh>, folder <area>/CliffLife, tagged CliffLife and never Obstacle: an
instanced actor's bounds span the valley, and the class tags itself in play), culled per kind (`cull`, cm; Medium draws
60% of it), solid for the junipers (their trunk's hull), fans and slabs, passable for the rest, no shadow from the grass
and the nests, none of them taking decals. Each stain is an ADecalActor (CliffStain_<nn>, folder <area>/CliffLife/Stains)
wearing MI_CliffStain, with `fadeScreenSize` and `sortOrder` (under the bullet marks). Every choice is logged, and per
kind how many were turned down and why. The fans and slabs collide: run Tools/Unreal/path_probe.py after. footprints()
gives the scatter their boxes (build_island_scatter.py keeps off them through it).
"""
import copy
import importlib
import json
import math
import os
import random
import re
import sys
import time

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.append(HERE)
import build_area  # noqa: E402

CLASSES = '/Script/AI_Looter_Shooter.'
FOLDER = 'CliffLife'
STAIN_FOLDER = 'CliffLife/Stains'
TAG = 'CliffLife'
STAIN_TAG = 'CliffStain'
STAIN_MATERIAL = '/Game/Art/Materials/MI_CliffStain'
# A cliff piece's label (cliffs(), panels): Cliff_<group>_<point>[_<course>]; a knob's rock: Outcrop_<group>.
CLIFF_LABEL = re.compile(r'^Cliff_(.+?)_(\d{2,})(?:_(\d+))?$')
OUTCROP_LABEL = re.compile(r'^Outcrop_(.+)$')
UP = (0.0, 0.0, 1.0)
ALL = ('rock', 'ground', 'other')
TAN50, COS50, SIN50 = math.tan(math.radians(50.0)), math.cos(math.radians(50.0)), math.sin(math.radians(50.0))

# The kit, by mesh (SM_<name>): the kind that places it, whether it's solid (its hulls: a juniper's trunk, a fan's bed and
# blocks, a slab) and whether it casts a shadow.
KIT = {
    'LipDrape_A': ('drapes', False, True), 'LipDrape_B': ('drapes', False, True), 'LipDrape_C': ('drapes', False, True),
    'LedgeSage_A': ('ledges', False, True), 'LedgeSage_B': ('ledges', False, True),
    'LedgeGrass_A': ('ledges', False, False), 'LedgeGrass_B': ('ledges', False, False),
    'CliffJuniper_B': ('ledges', True, True),
    'CliffJuniper_A': ('cracks', True, True), 'CliffJuniper_C': ('cracks', True, True),
    'SwallowNests_A': ('nests', False, False), 'SwallowNests_B': ('nests', False, False),
    'ScreeFan_A': ('fans', True, True), 'ScreeFan_B': ('fans', True, True), 'ScreeFan_C': ('fans', True, True),
    'FallenSlab_A': ('slabs', True, True), 'FallenSlab_B': ('slabs', True, True),
}
KINDS = ('drapes', 'cracks', 'ledges', 'nests', 'fans', 'slabs')
# Drapes: width along the lip and their deepest hang (cm; CliffLife.py).
DRAPES = {'LipDrape_A': (240.0, 110.0), 'LipDrape_B': (200.0, 200.0), 'LipDrape_C': (120.0, 150.0)}
# Ledge pieces: the least distance from the pivot to the ledge's edge, and the room over it (cm).
LEDGE_FIT = {'LedgeGrass_B': (15.0, 60.0), 'LedgeGrass_A': (30.0, 100.0), 'LedgeSage_A': (30.0, 100.0),
             'LedgeSage_B': (30.0, 100.0), 'CliffJuniper_C': (30.0, 120.0), 'CliffJuniper_B': (30.0, 250.0)}
# Crack junipers: clear out from the joint, clear up over its crown, the crown's height over the joint (cm).
CRACK_FIT = {'CliffJuniper_A': (130.0, 180.0, 130.0), 'CliffJuniper_C': (90.0, 105.0, 80.0)}
# Fans: width along the wall, reach out from it, and how far its foot may wander in and out across that width (cm).
FANS = {'ScreeFan_A': (550.0, 260.0, 80.0), 'ScreeFan_B': (340.0, 200.0, 120.0), 'ScreeFan_C': (650.0, 180.0, 40.0)}
# Slabs: half their length and width as they lie (cm; CliffScree.py's 2.9 x 2.1 and 2.2 x 1.4 m).
SLABS = {'FallenSlab_A': (145.0, 105.0), 'FallenSlab_B': (110.0, 70.0)}
# Room between kinds (cm): between two plants of different kinds, round a nest, between a slab and a fan.
PLANT_ROOM = 150.0
NEST_ROOM = 100.0
SLAB_ROOM = 400.0
# A juniper whose pivot stands lower than this over the ground keeps off the roads and ramps (it collides).
LOW_JUNIPER = 250.0

DEFAULTS = {
    'seed': 43,
    'reach': 6000.0,
    'views': [1500.0, 3000.0, 6000.0, 10000.0],
    'viewTurns': [0.0, -40.0, 40.0],
    'eye': 170.0,
    'pathBias': 3.0,
    'pathReach': 4000.0,
    'drapes': {'spacing': 600.0, 'cap': 70, 'perPiece': 2, 'cull': 20000.0, 'scale': [0.7, 1.2],
               'meshes': {'LipDrape_A': 3, 'LipDrape_B': 2, 'LipDrape_C': 2}},
    'cracks': {'spacing': 900.0, 'cap': 36, 'perPiece': 1, 'cull': 25000.0, 'rows': [0.35, 0.55, 0.75],
               'scale': [0.9, 1.1], 'meshes': {'CliffJuniper_A': 2, 'CliffJuniper_C': 1}},
    'ledges': {'spacing': 260.0, 'cap': 150, 'perPiece': 4, 'cull': 12000.0, 'grid': 40.0, 'scale': [0.85, 1.15],
               'meshes': {'LedgeSage_A': 3, 'LedgeSage_B': 2, 'LedgeGrass_A': 3, 'LedgeGrass_B': 2,
                          'CliffJuniper_B': 1.5, 'CliffJuniper_C': 1}},
    'nests': {'spacing': 2500.0, 'cap': 12, 'perPiece': 1, 'cull': 7000.0, 'least': 300.0, 'scale': [0.9, 1.1],
              'meshes': {'SwallowNests_A': 2, 'SwallowNests_B': 1}},
    'fans': {'spacing': 1000.0, 'cap': 50, 'perPiece': 1, 'cull': 20000.0, 'step': 100.0, 'scale': [0.9, 1.1],
             'tilt': 12.0, 'meshes': {'ScreeFan_A': 3, 'ScreeFan_B': 1, 'ScreeFan_C': 2}},
    'slabs': {'spacing': 3500.0, 'cap': 12, 'perPiece': 1, 'cull': 25000.0, 'out': [300.0, 550.0], 'chance': 0.35,
              'scale': [0.85, 1.15], 'tilt': 8.0, 'meshes': {'FallenSlab_A': 1, 'FallenSlab_B': 1}},
    'stains': {'cap': 70, 'perPiece': 3, 'gap': 120.0, 'every': 120.0, 'fadeScreenSize': 0.02, 'sortOrder': -1,
               'strength': [0.8, 1.0],
               # cell: [weight, [least, most width], [least, most height]] (cm); 5 only under ledges, 7 under nests.
               'cells': {'0': [2, [180, 240], [350, 500]], '1': [3, [100, 140], [300, 500]],
                         '2': [2, [120, 180], [300, 480]], '3': [2, [140, 200], [300, 400]],
                         '4': [2, [180, 240], [300, 450]], '5': [1, [120, 200], [300, 400]],
                         '6': [1, [160, 240], [300, 450]], '7': [1, [60, 90], [180, 260]]}},
    'keepOff': {'road': 150.0, 'ramp': 200.0, 'obstacle': 150.0, 'building': 150.0, 'gameplay': 400.0,
                'placement': 300.0},
}


# ---------------------------------------------------------------------------------------------------------------------
# Plain geometry (cm, X north, Y east, Z up; points and directions as tuples)
# ---------------------------------------------------------------------------------------------------------------------

def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def mul(a, k):
    return (a[0] * k, a[1] * k, a[2] * k)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def length(a):
    return math.sqrt(dot(a, a))


def unit(a):
    n = length(a)
    return (a[0] / n, a[1] / n, a[2] / n) if n > 1e-9 else (0.0, 0.0, 0.0)


def level(a):
    return unit((a[0], a[1], 0.0))


def turned(v, degrees):
    c, s = math.cos(math.radians(degrees)), math.sin(math.radians(degrees))
    return (v[0] * c - v[1] * s, v[0] * s + v[1] * c, v[2])


def dist(a, b):
    return length(sub(a, b))


def dist2d(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def yaw_of(v):
    return math.degrees(math.atan2(v[1], v[0]))


def leaned(normal, most):
    """normal (a ledge's, the ground's), leaned back toward up so it stands no more than most degrees off it."""
    n = unit(normal)
    if n[2] <= 0.0:
        return UP
    if math.degrees(math.acos(min(n[2], 1.0))) <= most:
        return n
    h, t = level(n), math.radians(most)
    return (h[0] * math.sin(t), h[1] * math.sin(t), math.cos(t))


def segment_distance(p, a, b):
    ax, ay = b[0] - a[0], b[1] - a[1]
    l2 = ax * ax + ay * ay
    t = 0.0 if l2 < 1e-6 else max(0.0, min(1.0, ((p[0] - a[0]) * ax + (p[1] - a[1]) * ay) / l2))
    return math.hypot(p[0] - (a[0] + ax * t), p[1] - (a[1] + ay * t))


def path_distance(p, path):
    if len(path) == 1:
        return math.hypot(p[0] - path[0][0], p[1] - path[0][1])
    return min(segment_distance(p, a, b) for a, b in zip(path, path[1:]))


def inside(p, polygon):
    """Whether p (x, y) is inside the polygon (even-odd)."""
    result = False
    for (x0, y0), (x1, y1) in zip(polygon, list(polygon[1:]) + list(polygon[:1])):
        if (y0 > p[1]) != (y1 > p[1]) and p[0] < x0 + (p[1] - y0) * (x1 - x0) / (y1 - y0):
            result = not result
    return result


def outline_distance(p, polygon):
    return path_distance(p, list(polygon) + [polygon[0]])


def plane(points):
    """The plane z = a + b x + c y through points (least squares): (its up normal, how far the points stray from it at
    most, a function giving its z), or None for fewer than three points or a degenerate spread."""
    n = len(points)
    if n < 3:
        return None
    mx = sum(p[0] for p in points) / n
    my = sum(p[1] for p in points) / n
    mz = sum(p[2] for p in points) / n
    sxx = sum((p[0] - mx) ** 2 for p in points)
    syy = sum((p[1] - my) ** 2 for p in points)
    sxy = sum((p[0] - mx) * (p[1] - my) for p in points)
    sxz = sum((p[0] - mx) * (p[2] - mz) for p in points)
    syz = sum((p[1] - my) * (p[2] - mz) for p in points)
    det = sxx * syy - sxy * sxy
    if abs(det) < 1e-6:
        return None
    b = (sxz * syy - syz * sxy) / det
    c = (syz * sxx - sxz * sxy) / det

    def z_at(x, y):
        return mz + b * (x - mx) + c * (y - my)
    stray = max(abs(p[2] - z_at(p[0], p[1])) for p in points)
    return unit((-b, -c, 1.0)), stray, z_at


def steps(lo, hi, step):
    """lo, lo + step, ... up to hi (from the middle's side when the span is shorter than a step)."""
    if hi < lo:
        return [(lo + hi) * 0.5]
    count = int(math.floor((hi - lo) / step)) + 1
    start = lo + ((hi - lo) - (count - 1) * step) * 0.5
    return [start + i * step for i in range(count)]


def draw(spec, kind, point, extra=''):
    """A random stream of a spot's own: from the seed, the kind and its place rounded to the centimetre."""
    return random.Random(f'{spec["seed"]} {kind} {point[0]:.0f} {point[1]:.0f} {point[2]:.0f} {extra}')


def weighted(rnd, weights):
    """One name drawn by weight from {name: weight} (None when all weigh nothing)."""
    items = [(name, float(w)) for name, w in weights.items() if float(w) > 0.0]
    total = sum(w for _, w in items)
    if total <= 0.0:
        return None
    left = rnd.uniform(0.0, total)
    for name, w in items:
        left -= w
        if left <= 0.0:
            return name
    return items[-1][0]


def order_by_weight(rnd, weights):
    """Every name with weight, in an order drawn by weight (the first is most often the heaviest)."""
    left = dict(weights)
    out = []
    while True:
        name = weighted(rnd, left)
        if name is None:
            return out
        out.append(name)
        left.pop(name)


# ---------------------------------------------------------------------------------------------------------------------
# What is found and chosen
# ---------------------------------------------------------------------------------------------------------------------

class Hit:
    """Where a cast first met something: the point, its normal, what it was ('rock', 'ground' or 'other') and its label."""
    __slots__ = ('point', 'normal', 'kind', 'label', 'distance')

    def __init__(self, point, normal, kind, label, distance):
        self.point, self.normal, self.kind, self.label, self.distance = point, normal, kind, label, distance


class Piece:
    """A cliff piece as placed: its label, cliff run (group), kind ('face', 'panel', 'outcrop'), whether it is its point's
    top course and bottom course, its world bounds and the sides a placer looks at."""

    def __init__(self, label, group, kind, top_course, bottom_course, lo, hi):
        self.label, self.group, self.kind = label, group, kind
        self.top_course, self.bottom_course = top_course, bottom_course
        self.lo, self.hi = lo, hi
        self.sides = []
        self.region = None


class Side:
    """One side of a piece in its own frame: o a point on its axis, f level and out of the rock, r along it (f turned a
    quarter left: the actor's right); s from s0 to s1 along r (cm from o), d out along f; its most forward rock stands
    `front` out, the casts start `out` out; bottom and top its height."""

    def __init__(self, piece, o, f, s0, s1, front, out, bottom, top, name=''):
        self.piece = piece
        self.o = (o[0], o[1], 0.0)
        self.f = level(f)
        self.r = (-self.f[1], self.f[0], 0.0)
        self.s0, self.s1, self.front, self.out = s0, s1, front, out
        self.bottom, self.top = bottom, top
        self.name = piece.label + name
        self.yaw = yaw_of(self.f)
        self.seen = None        # the nearest view distance it's seen from (cm), or None
        self._ground = {}

    def at(self, s, d, z):
        return (self.o[0] + self.r[0] * s + self.f[0] * d, self.o[1] + self.r[1] * s + self.f[1] * d, z)

    def local(self, p):
        q = (p[0] - self.o[0], p[1] - self.o[1], 0.0)
        return dot(q, self.r), dot(q, self.f)


class Spot:
    """A candidate (and, once chosen, a placement): its kind, mesh, pivot, turn (yaw, and the up it leans to or None),
    scale, the piece and side it is on, its rank and what the choosing needs (info: width, hang, lip; reach)."""

    def __init__(self, kind, mesh, point, yaw, side, up=None, scale=(1.0, 1.0, 1.0), info=None):
        self.kind, self.mesh, self.point, self.yaw, self.side = kind, mesh, point, yaw, side
        self.up, self.scale = up, scale
        self.piece = side.piece
        self.info = info or {}
        self.rank = 0.0
        self.path = 0.0

    def describe(self):
        x, y, z = self.point
        lean = ''
        if self.up is not None:
            lean = f', leaning {math.degrees(math.acos(max(min(self.up[2], 1.0), -1.0))):.0f} deg'
        size = (f'scale {self.scale[0]:.2f}' if self.scale[0] == self.scale[1] == self.scale[2]
                else f'scale {self.scale[0]:.2f} x {self.scale[1]:.2f} x {self.scale[2]:.2f}')
        return (f'{self.mesh} on {self.piece.label} at ({x:.0f}, {y:.0f}, {z:.0f}) yaw {self.yaw:.0f}{lean}, {size}'
                f'{", " + self.info["why"] if self.info.get("why") else ""}, {self.path / 100.0:.0f} m from a path')


class Stain:
    """A stain decal: the side it lies on, its middle along it (s), its top, width and height (cm), its cell, strength, the
    kind of spot it came from, and once fitted its box (centre, half depth)."""

    def __init__(self, side, s, top, width, height, cell, strength, source):
        self.side, self.s, self.top, self.width, self.height = side, s, top, width, height
        self.cell, self.strength, self.source = cell, strength, source
        self.piece = side.piece
        self.centre = side.at(s, side.front, top - height * 0.5)
        self.depth = 0.0
        self.rank = 0.0
        self.path = 0.0

    def describe(self):
        x, y, z = self.centre
        return (f'cell {self.cell} ({self.source}) on {self.piece.label} at ({x:.0f}, {y:.0f}, {z:.0f}), '
                f'{self.width:.0f} x {self.height:.0f} cm, {self.depth:.0f} cm deep, strength {self.strength:.2f}, '
                f'{self.path / 100.0:.0f} m from a path')


# ---------------------------------------------------------------------------------------------------------------------
# The level (casts, ground, solids): Unreal
# ---------------------------------------------------------------------------------------------------------------------

def editor_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def level_actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def vec(p):
    return unreal.Vector(float(p[0]), float(p[1]), float(p[2]))


def tup(v):
    return (v.x, v.y, v.z)


def trace(component, start, end):
    """Where the line start-end first meets one component (complex collision): (point, normal), or None."""
    hit = component.line_trace_component(vec(start), vec(end), True, False, False)
    if not hit:
        return None
    if isinstance(hit, tuple):
        vectors = [h for h in hit if isinstance(h, unreal.Vector)]
        if len(vectors) >= 2:
            return tup(vectors[0]), tup(vectors[1])
        result = next((h for h in hit if isinstance(h, unreal.HitResult)), None)
        if result is None:
            return (tup(vectors[0]), UP) if vectors else None
        hit = result
    t = hit.to_tuple()
    return tup(t[5]), tup(t[7])


def in_folder(actor, folder):
    path = str(actor.get_folder_path())
    return path == folder or path.startswith(folder + '/')


class World:
    """The built level as casts see it: the terrain's tiles (tagged Ground: 'ground'), the cliff pieces and outcrops
    ('rock') and the other placed models ('other'), each with its world box; and the pieces themselves."""

    def __init__(self, build):
        self.build = build
        self.parts = []      # (component, lo, hi, kind, label)
        self.tiles = []
        self.rock_actors = []
        self.casts = 0
        mine = f'{build.folder}/{FOLDER}'
        tag = unreal.Name(build.tag)
        for actor in level_actors():
            if tag not in actor.tags or not isinstance(actor, unreal.StaticMeshActor) or in_folder(actor, mine):
                continue
            component = actor.static_mesh_component
            if component is None or component.static_mesh is None:
                continue
            label = str(actor.get_actor_label())
            origin, extent = actor.get_actor_bounds(False)
            lo = (origin.x - extent.x, origin.y - extent.y, origin.z - extent.z)
            hi = (origin.x + extent.x, origin.y + extent.y, origin.z + extent.z)
            if unreal.Name('Ground') in actor.tags:
                # Its own box: a cast high on a face over the tile's highest ground never traces the tile.
                part = (component, lo, hi, 'ground', label)
                self.tiles.append(part)
            elif CLIFF_LABEL.match(label) or OUTCROP_LABEL.match(label):
                part = (component, lo, hi, 'rock', label)
                self.rock_actors.append((actor, component, label, lo, hi))
            elif unreal.Name('Beyond') in actor.tags or in_folder(actor, f'{build.folder}/Terrain'):
                continue     # past the boundary, or the terrain's underside and water
            else:
                part = (component, lo, hi, 'other', label)
            self.parts.append(part)
        self.ground_cache = {}

    def near(self, lo, hi, margin):
        """The parts whose boxes come within margin of the box lo-hi (a piece's own region for its casts)."""
        return [p for p in self.parts if p[1][0] <= hi[0] + margin and p[2][0] >= lo[0] - margin and
                p[1][1] <= hi[1] + margin and p[2][1] >= lo[1] - margin and
                p[1][2] <= hi[2] + margin and p[2][2] >= lo[2] - margin]

    def cast(self, start, end, region=None, kinds=ALL):
        """The nearest hit on the line start-end among region's parts (all of them without one) of those kinds."""
        self.casts += 1
        lo = (min(start[0], end[0]), min(start[1], end[1]), min(start[2], end[2]))
        hi = (max(start[0], end[0]), max(start[1], end[1]), max(start[2], end[2]))
        way = unit(sub(end, start))
        best = None
        for component, plo, phi, kind, label in (self.parts if region is None else region):
            if kind not in kinds or plo[0] > hi[0] or phi[0] < lo[0] or plo[1] > hi[1] or phi[1] < lo[1] or \
                    plo[2] > hi[2] or phi[2] < lo[2]:
                continue
            got = trace(component, start, end)
            if got is None:
                continue
            point, normal = got
            if dot(normal, way) > 0.05:
                continue     # the back of a face the cast started inside
            d = dist(start, point)
            if best is None or d < best.distance:
                best = Hit(point, normal, kind, label, d)
        return best

    def ground(self, x, y):
        """The terrain's height under (x, y) (its tiles alone), or None off them."""
        key = (round(x / 10.0), round(y / 10.0))
        if key not in self.ground_cache:
            hit = self.cast((x, y, 50000.0), (x, y, -50000.0), self.tiles, ('ground',))
            self.ground_cache[key] = None if hit is None else hit.point[2]
        return self.ground_cache[key]

    def solid(self, point, radius):
        """The label of something solid (blocking a player) within radius of point that isn't the terrain, a cliff piece,
        a volume or the scatter, or None."""
        objects = [unreal.ObjectTypeQuery.ECC_WORLD_STATIC, unreal.ObjectTypeQuery.ECC_WORLD_DYNAMIC]
        try:
            found = unreal.SystemLibrary.sphere_overlap_components(editor_world(), vec(point), radius, objects, None, [])
        except Exception as error:  # noqa: BLE001 - a failed check never stops the build: the layout's checks still hold
            unreal.log_warning(f'cliff life: ({point[0]:.0f}, {point[1]:.0f}) not checked for solids ({error})')
            return None
        components = found[1] if isinstance(found, tuple) else found
        skip = tuple(c for c in (getattr(unreal, 'Volume', None), getattr(unreal, 'PCGVolume', None)) if c is not None)
        for c in components or []:
            owner = c.get_owner()
            if owner is None or (skip and isinstance(owner, skip)) or unreal.Name('Ground') in owner.tags:
                continue
            label = str(owner.get_actor_label())
            # Not the rock it stands against, nor what a run before placed (the probe looks with it still standing).
            if CLIFF_LABEL.match(label) or OUTCROP_LABEL.match(label) or \
                    in_folder(owner, f'{self.build.folder}/Terrain') or in_folder(owner, f'{self.build.folder}/{FOLDER}'):
                continue
            if c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN) != \
                    unreal.CollisionResponseType.ECR_BLOCK:
                continue
            return label
        return None

    def pieces(self, panelled):
        """Every cliff piece and outcrop as a Piece with its sides, in label order."""
        found = []
        tops = {}
        for actor, component, label, lo, hi in self.rock_actors:
            m = CLIFF_LABEL.match(label)
            if m:
                group, base, course = m.group(1), f'Cliff_{m.group(1)}_{m.group(2)}', int(m.group(3) or 0)
                tops[base] = max(tops.get(base, 0), course)
                found.append((label, actor, component, group, 'panel' if group in panelled else 'face', base, course,
                              lo, hi))
            else:
                found.append((label, actor, component, OUTCROP_LABEL.match(label).group(1), 'outcrop', label, 0, lo, hi))
        pieces = []
        for label, actor, component, group, kind, base, course, lo, hi in sorted(found, key=lambda f: f[0]):
            piece = Piece(label, group, kind, course == tops.get(base, 0), course <= 1, lo, hi)
            if kind == 'outcrop':
                origin, extent = actor.get_actor_bounds(False)
                reach, half = max(extent.x, extent.y), min(extent.x, extent.y) * 0.55
                yaw = actor.get_actor_rotation().yaw
                for k in range(6):
                    f = turned((1.0, 0.0, 0.0), yaw + 60.0 * k)
                    piece.sides.append(Side(piece, (origin.x, origin.y), f, -half, half, reach * 0.9, reach + 300.0,
                                            origin.z - extent.z, origin.z + extent.z, f'#{k}'))
            else:
                at, forward, scale = actor.get_actor_location(), actor.get_actor_forward_vector(), actor.get_actor_scale3d()
                box = component.static_mesh.get_bounding_box()
                across = sorted((box.min.y * scale.y, box.max.y * scale.y))
                front = box.max.x * abs(scale.x)
                piece.sides.append(Side(piece, (at.x, at.y), (forward.x, forward.y, 0.0), across[0] + 30.0,
                                        across[1] - 30.0, front, front + 300.0, at.z, hi[2]))
            piece.region = self.near(lo, hi, 1500.0)
            pieces.append(piece)
        return pieces


# ---------------------------------------------------------------------------------------------------------------------
# What the solid pieces keep off, and how far a spot is from a path
# ---------------------------------------------------------------------------------------------------------------------

class Site:
    """The layout's roads and ramps (paths), obstacles and placements, the buildings players walk into (level.cliffs.
    keepOut, their boxes as placed), the gameplay pieces, and the playable boundary."""

    def __init__(self, build, spec, buildings, gameplay):
        self.keep = spec['keepOff']
        self.paths = []      # (points, half width, 'road' or 'ramp', box)
        for road in build.layout.get('roads', {}).values():
            self.add_path([tuple(p[:2]) for p in road.get('points', [])], road.get('width', 300.0) * 0.5, 'road')
        for ramp in build.layout.get('ramps', {}).values():
            self.add_path([tuple(p[:2]) for p in ramp.get('points', [])], ramp.get('width', 400.0) * 0.5, 'ramp')
        self.obstacles = []  # ('path' or 'area', points, id)
        for obstacle in build.source.get('obstacles', []):
            if obstacle.get('path'):
                self.obstacles.append(('path', [tuple(p[:2]) for p in obstacle['path']], obstacle.get('id', '?')))
            elif obstacle.get('polygon'):
                self.obstacles.append(('area', [tuple(p[:2]) for p in obstacle['polygon']], obstacle.get('id', '?')))
        flat = {f['id']: f for f in build.layout.get('footprints', [])}
        self.places = []     # (x, y, radius, key)
        for key, spot in build.layout.get('placements', {}).items():
            radius = flat[key]['flatRadius'] if key in flat else self.keep['placement']
            self.places.append((spot['location'][0], spot['location'][1], radius, key))
        self.buildings = buildings
        self.gameplay = gameplay
        self.boundary = [tuple(c[:2]) for c in build.layout.get('boundary', {}).get('corners', [])]

    def add_path(self, points, half, what):
        if len(points) < 2:
            return
        xs, ys = [p[0] for p in points], [p[1] for p in points]
        self.paths.append((points, half, what, (min(xs), max(xs), min(ys), max(ys))))

    def path_distance(self, p, reach=1e9):
        """How far (x, y) is from the nearest road's or ramp's edge (0 on one), looking no further than reach."""
        best = reach
        for points, half, _, (x0, x1, y0, y1) in self.paths:
            grow = best + half
            if p[0] < x0 - grow or p[0] > x1 + grow or p[1] < y0 - grow or p[1] > y1 + grow:
                continue
            best = min(best, max(path_distance(p, points) - half, 0.0))
        return best

    def past_boundary(self, p):
        """How far (x, y) lies outside the playable boundary (0 inside it, or without one)."""
        if len(self.boundary) < 3 or inside(p, self.boundary):
            return 0.0
        return outline_distance(p, self.boundary)

    def in_bounds(self, p):
        return len(self.boundary) < 3 or inside(p, self.boundary)

    def in_building(self, point, grow=None):
        g = self.keep['building'] if grow is None else grow
        for lo, hi, label in self.buildings:
            if lo[0] - g <= point[0] <= hi[0] + g and lo[1] - g <= point[1] <= hi[1] + g and \
                    lo[2] - g <= point[2] <= hi[2] + g:
                return label
        return None

    def blocked(self, points, solid, reach=0.0):
        """Why points (x, y, z) won't do (a few words), or None: inside a building players walk into, and for a solid
        piece (reach: its own half size, cm) on a road, a ramp, an obstacle, a placed model or by a gameplay piece."""
        for p in points:
            label = self.in_building(p)
            if label:
                return f'in {label}'
            if not solid:
                continue
            for path, half, what, (x0, x1, y0, y1) in self.paths:
                margin = half + self.keep['road' if what == 'road' else 'ramp'] + reach
                if x0 - margin <= p[0] <= x1 + margin and y0 - margin <= p[1] <= y1 + margin and \
                        path_distance(p, path) < margin:
                    return f'on a {what}'
            for kind, shape, oid in self.obstacles:
                margin = self.keep['obstacle'] + reach
                if kind == 'path' and path_distance(p, shape) < margin:
                    return f'on {oid}'
                if kind == 'area' and (inside(p, shape) or outline_distance(p, shape) < margin):
                    return f'on {oid}'
            for x, y, radius, key in self.places:
                if math.hypot(p[0] - x, p[1] - y) < radius + reach:
                    return f'on {key}'
            for x, y, label in self.gameplay:
                if math.hypot(p[0] - x, p[1] - y) < self.keep['gameplay'] + reach:
                    return f'by {label}'
        return None


def level_site(build, spec):
    """The Site with the level's buildings (keepOut) and gameplay pieces (the Gameplay folder) as they stand."""
    keys = set(build.cliff_look.get('keepOut', []))
    tag = unreal.Name(build.tag)
    buildings, gameplay = [], []
    for actor in level_actors():
        if tag not in actor.tags:
            continue
        label = str(actor.get_actor_label())
        if label in keys:
            origin, extent = actor.get_actor_bounds(False)
            buildings.append(((origin.x - extent.x, origin.y - extent.y, origin.z - extent.z),
                              (origin.x + extent.x, origin.y + extent.y, origin.z + extent.z), label))
        elif in_folder(actor, f'{build.folder}/Gameplay') and actor.get_class().get_name() not in \
                ('StaticMeshActor', 'InstancedProps', 'InstancedScenery'):
            at = actor.get_actor_location()
            gameplay.append((at.x, at.y, label))
    return Site(build, spec, buildings, gameplay)


# ---------------------------------------------------------------------------------------------------------------------
# Finding the spots on one side
# ---------------------------------------------------------------------------------------------------------------------

class Survey:
    """The spots on the pieces' sides, kind by kind (the module's docstring), through world's casts."""

    def __init__(self, world, spec, site):
        self.world, self.spec, self.site = world, spec, site

    def cast(self, side, start, end, kinds=ALL):
        return self.world.cast(start, end, side.piece.region, kinds)

    def into(self, side, s, z, start=None):
        """A level cast into the side at s, height z, from `start` out (its own `out` by default)."""
        start = side.out if start is None else start
        return self.cast(side, side.at(s, start, z), side.at(s, -max(side.out, 300.0) - 600.0, z))

    def ground(self, side, s):
        """The terrain in front of the side at s (150 cm past its most forward rock), or None."""
        if s not in side._ground:
            p = side.at(s, side.front + 150.0, 0.0)
            side._ground[s] = self.world.ground(p[0], p[1])
        return side._ground[s]

    # --- Who sees it ---

    def seen(self, side):
        """The nearest of `views` the side is seen from (cm), or None (the module's docstring)."""
        middle = (side.s0 + side.s1) * 0.5
        base = self.ground(side, middle)
        base = side.bottom if base is None else max(base, side.bottom)
        if side.top - base < 100.0:
            return None
        targets = []
        for share in (0.4, 0.75):
            z = base + (side.top - base) * share
            hit = self.into(side, middle, z)
            targets.append(add(hit.point, mul(side.f, 60.0)) if hit is not None and hit.kind == 'rock'
                           else side.at(middle, side.front + 60.0, z))
        for reach in self.spec['views']:
            for turn in self.spec['viewTurns']:
                way = turned(side.f, turn)
                x, y = targets[0][0] + way[0] * reach, targets[0][1] + way[1] * reach
                if not self.site.in_bounds((x, y)):
                    continue
                z = self.world.ground(x, y)
                if z is None:
                    continue
                eye = (x, y, z + self.spec['eye'])
                for target in targets:
                    hit = self.world.cast(eye, target, None, ('rock', 'ground'))
                    if hit is None or hit.distance >= dist(eye, target) - 40.0:
                        return reach
        return None

    # --- Lips and drapes ---

    def lips(self, side):
        """The lip line along the side's top: (s, d, point) every 10 cm where found."""
        out = []
        for s in steps(side.s0, side.s1, 10.0):
            front = self.into(side, s, side.top - 40.0)
            if front is None or front.kind != 'rock':
                continue
            d0 = side.local(front.point)[1]
            # From just in front of the face under the top (a top rounds back over its last 30 cm, so the lip lies behind
            # that front) in 5 cm at a time, down to 85 cm behind it.
            for k in range(21):
                d = d0 + 15.0 - 5.0 * k
                down = self.cast(side, side.at(s, d, side.top + 60.0), side.at(s, d, side.top - 150.0))
                if down is None:
                    continue
                if down.kind != 'rock':
                    break        # the terrain or a model over the piece's top: no lip here
                if down.normal[2] > 0.5 and down.point[2] > side.top - 110.0:
                    out.append((s, side.local(down.point)[1], down.point))
                    break
        return out

    def drapes(self, side, lips, why):
        spec = self.spec['drapes']
        found = []
        if len(lips) < 8:
            return found
        lips = sorted(lips, key=lambda p: p[0])
        for c in steps(lips[0][0], lips[-1][0], 30.0):
            rnd = draw(self.spec, 'drape', side.at(c, 0.0, side.top))
            scale = rnd.uniform(*spec['scale'])
            for mesh in order_by_weight(rnd, spec['meshes']):
                if mesh not in DRAPES:
                    continue
                width, hang = DRAPES[mesh]
                half = width * scale * 0.5
                run = [p for p in lips if c - half <= p[0] <= c + half]
                if len(run) < 0.8 * width * scale / 10.0:
                    why['lip too short'] = why.get('lip too short', 0) + 1
                    continue
                ds, zs = sorted(p[1] for p in run), sorted(p[2][2] for p in run)
                md, mz = ds[len(ds) // 2], zs[len(zs) // 2]
                near = sum(1 for p in run if abs(p[1] - md) <= 25.0 and abs(p[2][2] - mz) <= 35.0)
                if near < 0.8 * len(run) or ds[-1] > md + 35.0:
                    why['lip not straight'] = why.get('lip not straight', 0) + 1
                    continue
                ground = self.ground(side, c)
                if ground is None or mz - ground < hang + 50.0:
                    why['face under it too short'] = why.get('face under it too short', 0) + 1
                    continue
                if not self.face_below_clear(side, c, half, md, mz):
                    why['face under it juts out'] = why.get('face under it juts out', 0) + 1
                    continue
                spot = Spot('drapes', mesh, side.at(c, md, mz), side.yaw, side, scale=(1.0, scale, 1.0),
                            info={'width': width * scale, 'hang': hang, 'lip': mz})
                found.append(spot)
                break
        return found

    def face_below_clear(self, side, c, half, md, mz):
        for s in (c - half * 0.7, c, c + half * 0.7):
            for drop in (40.0, 100.0, 160.0):
                hit = self.cast(side, side.at(s, md + 400.0, mz - drop), side.at(s, md - 300.0, mz - drop))
                if hit is None or hit.kind != 'rock' or side.local(hit.point)[1] > md + 22.0:
                    return False
        return True

    # --- Ledges ---

    def ledges(self, side):
        """Up-facing rock in the face: (point, normal, s, room over it, edge distance) each."""
        spec = self.spec['ledges']
        grid = spec['grid']
        way = add(mul(side.f, -COS50), mul(UP, -SIN50))
        found, cells = [], set()
        for s in steps(side.s0, side.s1, grid):
            ground = self.ground(side, s)
            if ground is None:
                continue
            z = max(ground, side.bottom) + 100.0
            while z <= side.top - 40.0:
                origin = side.at(s, side.front + 600.0, z + 600.0 * TAN50)
                hit = self.cast(side, origin, add(origin, mul(way, 1500.0)))
                z += grid
                if hit is None or hit.kind != 'rock' or hit.normal[2] <= 0.8:
                    continue
                p = hit.point
                if p[2] < ground + 100.0 or p[2] > side.top - 40.0:
                    continue
                # One spot per 30 cm cell across, out and up: two rays meeting one ledge at its edge and deep in it are
                # two spots (a ledge plant wants 30 cm of ledge in front of it).
                s_hit, d_hit = side.local(p)
                cell = (round(s_hit / 30.0), round(d_hit / 30.0), round(p[2] / 30.0))
                if cell in cells:
                    continue
                cells.add(cell)
                over = self.cast(side, add(p, (0.0, 0.0, 5.0)), add(p, (0.0, 0.0, 265.0)))
                room = 260.0 if over is None else over.distance
                if room < 60.0:
                    continue
                if self.cast(side, add(p, (0.0, 0.0, 25.0)), add(add(p, (0.0, 0.0, 25.0)), mul(side.f, 100.0))):
                    continue
                found.append((p, hit.normal, s_hit, room, self.edge(side, p), ground))
        return found

    def edge(self, side, p):
        """How far out from p its ledge runs before the rock under it drops away (cm; 999 past 60)."""
        for e in (10.0, 20.0, 30.0, 45.0, 60.0):
            q = add(p, mul(side.f, e))
            hit = self.cast(side, add(q, (0.0, 0.0, 20.0)), add(q, (0.0, 0.0, -40.0)))
            if hit is None or hit.kind != 'rock' or abs(hit.point[2] - p[2]) > 25.0:
                return e
        return 999.0

    def ledge_spots(self, side, ledges, why):
        spec = self.spec['ledges']
        found = []
        for p, normal, s, room, edge, ground in ledges:
            rnd = draw(self.spec, 'ledge', p)
            fits = {}
            for mesh, weight in spec['meshes'].items():
                if mesh not in LEDGE_FIT:
                    continue
                least_edge, least_room = LEDGE_FIT[mesh]
                if edge < least_edge or room < least_room:
                    continue
                if edge < 30.0 and mesh != 'LedgeGrass_B':
                    continue
                if mesh == 'CliffJuniper_B' and p[2] < ground + 150.0:
                    continue
                fits[mesh] = weight
            mesh = weighted(rnd, fits)
            if mesh is None:
                why['no room on the ledge'] = why.get('no room on the ledge', 0) + 1
                continue
            yaw = side.yaw + rnd.uniform(-17.0, 17.0)
            size = rnd.uniform(*spec['scale'])
            found.append(Spot('ledges', mesh, add(p, (0.0, 0.0, -2.0)), yaw, side, up=leaned(normal, 15.0),
                              scale=(size, size, size), info={'reach': 120.0 if mesh.startswith('Cliff') else 0.0,
                                                               'height': p[2] - ground,
                                                               'why': f'ledge edge {edge:.0f} cm, room {room:.0f} cm'}))
        return found

    # --- Cracks ---

    def cracks(self, side, why):
        spec = self.spec['cracks']
        found = []
        middle = (side.s0 + side.s1) * 0.5
        ground = self.ground(side, middle)
        if ground is None:
            return found
        base = max(ground, side.bottom)
        reach = 6
        for share in spec['rows']:
            z = base + (side.top - base) * share
            if z < ground + 150.0 or z > side.top - 120.0:
                continue
            row = []
            for s in steps(side.s0, side.s1, 5.0):
                hit = self.into(side, s, z)
                row.append((s, side.local(hit.point)[1], hit) if hit is not None and hit.kind == 'rock'
                           else (s, None, None))
            last = None
            for i in range(reach, len(row) - reach):
                s, d, hit = row[i]
                if d is None or (last is not None and s - last < 30.0):
                    continue
                left = [row[j][1] for j in range(i - reach, i) if row[j][1] is not None]
                right = [row[j][1] for j in range(i + 1, i + reach + 1) if row[j][1] is not None]
                # The deepest within 30 cm, with rock standing 8 cm or more proud of it on both sides: a joint, not the
                # foot of a single rib beside it.
                if len(left) < reach // 2 or len(right) < reach // 2 or d > min(left + right) or \
                        max(left) - d < 8.0 or max(right) - d < 8.0:
                    continue
                last = s
                spot = self.crack_spot(side, hit, ground, why)
                if spot is not None:
                    found.append(spot)
        return found

    def crack_spot(self, side, hit, ground, why):
        spec = self.spec['cracks']
        p = hit.point
        facing = level(hit.normal)
        # A joint's floor facing up or down has no way out of its own: the side's.
        off = 0.0 if length(facing) < 0.5 else (yaw_of(facing) - side.yaw + 180.0) % 360.0 - 180.0
        yaw = side.yaw + max(-35.0, min(35.0, off))
        out = turned((1.0, 0.0, 0.0), yaw)
        rnd = draw(self.spec, 'crack', p)
        for mesh in order_by_weight(rnd, spec['meshes']):
            if mesh not in CRACK_FIT:
                continue
            clear_out, clear_up, crown = CRACK_FIT[mesh]
            start = add(add(p, mul(out, 15.0)), (0.0, 0.0, 30.0))
            if self.cast(side, start, add(start, mul(out, clear_out))):
                continue
            start = add(p, mul(out, 60.0))
            if self.cast(side, start, add(start, (0.0, 0.0, clear_up))):
                continue
            high = add(p, (0.0, 0.0, crown))
            over = self.cast(side, add(high, mul(out, 300.0)), add(high, mul(out, -100.0)))
            if over is not None and over.kind == 'rock' and dot(sub(over.point, p), out) > 40.0:
                continue
            size = rnd.uniform(*spec['scale'])
            return Spot('cracks', mesh, p, yaw, side, scale=(size, size, size),
                        info={'reach': clear_out, 'height': p[2] - ground})
        why['no room at the joint'] = why.get('no room at the joint', 0) + 1
        return None

    # --- Feet: fans and slabs ---

    def feet(self, side):
        """The face's foot every fans step: (s, d, point, the terrain just in front of it)."""
        found = []
        for s in steps(side.s0, side.s1, self.spec['fans']['step']):
            ground = self.ground(side, s)
            if ground is None:
                continue
            foot = None
            for lift in (30.0, 60.0, 100.0, 150.0):
                hit = self.cast(side, side.at(s, side.out + 200.0, ground + lift),
                                side.at(s, -max(side.out, 300.0) - 600.0, ground + lift))
                if hit is None or hit.kind == 'other':
                    break
                if hit.kind == 'rock':
                    foot = hit
                    break
            if foot is None:
                continue
            d = side.local(foot.point)[1]
            q = side.at(s, d + 25.0, 0.0)
            z = self.world.ground(q[0], q[1])
            if z is None or z > foot.point[2] + 20.0 or z < foot.point[2] - 200.0:
                continue
            found.append((s, d, foot.point, z))
        return found

    def fans(self, side, feet, why):
        spec = self.spec['fans']
        found = []
        step = spec['step']
        for s, d, point, z in feet:
            rnd = draw(self.spec, 'fan', point)
            for mesh in self.fan_order(rnd, feet, s, d):
                width, reach, wander = FANS[mesh]
                run = [f for f in feet if abs(f[0] - s) <= width * 0.5 + 1.0]
                if len(run) < 0.8 * (width / step + 1.0):
                    continue
                ds = sorted(f[1] for f in run)
                if ds[-1] - ds[0] > wander:
                    continue
                md = ds[len(ds) // 2]
                fitted = self.terrain_under(side, s, md, width, reach)
                if fitted is None:
                    continue
                normal, ground = fitted
                pivot = side.at(s, md - 15.0, ground)
                up = leaned(normal, spec['tilt'])
                size = rnd.uniform(*spec['scale'])
                found.append(Spot('fans', mesh, pivot, side.yaw, side, up=up, scale=(size, size, size),
                                  info={'width': width * size, 'reach': reach * size,
                                        'why': f'foot wanders {ds[-1] - ds[0]:.0f} cm'}))
                break
            else:
                why['no run of foot for a fan'] = why.get('no run of foot for a fan', 0) + 1
        return found

    def fan_order(self, rnd, feet, s, d):
        """The fans to try at a foot: _B alone in a notch (the foot 30 cm deeper here than at both quarter marks of its
        width), else drawn by weight."""
        width = FANS['ScreeFan_B'][0]
        ends = [f[1] for f in feet if width * 0.25 <= abs(f[0] - s) <= width * 0.5 + 1.0]
        if len(ends) >= 2 and d <= min(ends) - 30.0 and 'ScreeFan_B' in self.spec['fans']['meshes']:
            return ['ScreeFan_B']
        return [m for m in order_by_weight(rnd, self.spec['fans']['meshes']) if m in FANS]

    def terrain_under(self, side, s, d, width, reach):
        """The terrain under a fan's spread (a plane through nine points: its up normal, the height at its pivot), or
        None where it strays 45 cm from the plane, slopes past 28 degrees or isn't there."""
        points = []
        for a in (-width / 3.0, 0.0, width / 3.0):
            for b in (60.0, 150.0, max(reach, 160.0)):
                q = side.at(s + a, d + b, 0.0)
                z = self.world.ground(q[0], q[1])
                if z is None:
                    return None
                points.append((q[0], q[1], z))
        fitted = plane(points)
        if fitted is None:
            return None
        normal, stray, z_at = fitted
        if stray > 45.0 or normal[2] < math.cos(math.radians(28.0)):
            return None
        q = side.at(s, d + 25.0, 0.0)
        z = self.world.ground(q[0], q[1])
        return normal, (z if z is not None else z_at(q[0], q[1]))

    def slabs(self, side, feet, why):
        spec = self.spec['slabs']
        found = []
        for s, d, point, _ in feet:
            rnd = draw(self.spec, 'slab', point)
            if rnd.random() > spec['chance']:
                continue
            mesh = weighted(rnd, {m: w for m, w in spec['meshes'].items() if m in SLABS})
            if mesh is None:
                continue
            out, along, yaw = rnd.uniform(*spec['out']), rnd.uniform(-150.0, 150.0), rnd.uniform(-180.0, 180.0)
            size = rnd.uniform(*spec['scale'])
            half_x, half_y = SLABS[mesh][0] * size, SLABS[mesh][1] * size
            centre = side.at(s + along, d + out, 0.0)
            ax, ay = turned((half_x, 0.0, 0.0), yaw), turned((0.0, half_y, 0.0), yaw)
            points = []
            for a, b in ((0.0, 0.0), (1.0, 1.0), (1.0, -1.0), (-1.0, 1.0), (-1.0, -1.0)):
                x, y = centre[0] + ax[0] * a + ay[0] * b, centre[1] + ax[1] * a + ay[1] * b
                z = self.world.ground(x, y)
                if z is None:
                    points = None
                    break
                points.append((x, y, z))
            fitted = plane(points) if points else None
            if fitted is None or fitted[1] > 35.0 or fitted[0][2] < math.cos(math.radians(15.0)):
                why['ground under a slab too rough'] = why.get('ground under a slab too rough', 0) + 1
                continue
            pivot = (centre[0], centre[1], points[0][2] - 5.0)
            found.append(Spot('slabs', mesh, pivot, yaw, side, up=leaned(fitted[0], spec['tilt']),
                              scale=(size, size, size), info={'reach': max(half_x, half_y)}))
        return found

    # --- Overhangs: nests ---

    def overhangs(self, side, why):
        spec = self.spec['nests']
        found = []
        for s in steps(side.s0, side.s1, 40.0):
            ground = self.ground(side, s)
            if ground is None:
                continue
            profile = []
            z = max(ground, side.bottom) + spec['least']
            while z <= side.top - 40.0:
                hit = self.into(side, s, z)
                profile.append((z, side.local(hit.point)[1], hit) if hit is not None and hit.kind == 'rock'
                               else (z, None, None))
                z += 15.0
            i = 0
            while i < len(profile):
                z, d, hit = profile[i]
                above = profile[i + 1:i + 5]
                if d is None or dot(hit.normal, side.f) < 0.3 or \
                        not any(o[1] is not None and o[1] > d + 15.0 for o in above):
                    i += 1
                    continue
                at = add(hit.point, mul(side.f, 6.0))
                ceiling = self.cast(side, at, add(at, (0.0, 0.0, 60.0)))
                if ceiling is None or ceiling.kind != 'rock' or ceiling.normal[2] > -0.6:
                    i += 1
                    continue
                further = add(at, mul(side.f, 25.0))
                if not self.cast(side, further, add(further, (0.0, 0.0, 80.0)), ('rock',)):
                    i += 1
                    continue
                top = ceiling.point[2]
                corner = self.cast(side, side.at(s, d + 30.0, top - 3.0), side.at(s, d - 70.0, top - 3.0), ('rock',))
                found.append(self.nest_spot(side, s, d, top, corner, at, ground))
                # On past the ceiling (at least one step: a ceiling met where the cast began is no further up).
                i += 1
                while i < len(profile) and profile[i][0] < top:
                    i += 1
        return [spot for spot in found if spot is not None]

    def nest_spot(self, side, s, d, top, corner, at, ground):
        spec = self.spec['nests']
        rnd = draw(self.spec, 'nest', (at[0], at[1], top))
        weights = dict(spec['meshes']) if corner is not None else {'SwallowNests_B': 1.0}
        mesh = weighted(rnd, {m: w for m, w in weights.items() if m in ('SwallowNests_A', 'SwallowNests_B')})
        if mesh is None:
            return None
        if corner is None:
            pivot = (at[0], at[1], top)
        elif mesh == 'SwallowNests_A':
            pivot = (corner.point[0], corner.point[1], top)
        else:
            q = add(corner.point, mul(side.f, 5.0))
            pivot = (q[0], q[1], top)
        size = rnd.uniform(*spec['scale'])
        return Spot('nests', mesh, pivot, side.yaw, side, scale=(size, size, size),
                    info={'ceiling': top, 'height': top - ground, 'why': 'in a corner' if corner is not None else
                          'under a ceiling'})

    # --- Stains ---

    def lip_stains(self, side, lips):
        spec = self.spec['stains']
        cells = {k: v for k, v in spec['cells'].items() if k not in ('5', '7')}
        found = []
        if not lips:
            return found
        lips = sorted(lips, key=lambda p: p[0])
        for mark in steps(lips[0][0], lips[-1][0], spec['every']):
            s, d, point = min(lips, key=lambda p: abs(p[0] - mark))
            if abs(s - mark) > 30.0:
                continue
            ground = self.ground(side, s)
            if ground is None:
                continue
            stain = self.new_stain(side, s, point[2] - 5.0, point[2] - 5.0 - ground - 30.0, cells, 'lip', point)
            if stain is not None:
                found.append(stain)
        return found

    def seep_stains(self, side, ledges):
        spec = self.spec['stains']
        found = []
        for p, _, s, _, _, ground in ledges:
            if '5' not in spec['cells']:
                break
            seep = p[2] - 10.0
            rnd = draw(self.spec, 'seep', p)
            weight, width, height = spec['cells']['5']
            if float(weight) <= 0.0:
                break
            h = min(rnd.uniform(*height), (seep - ground - 30.0) / 0.76)
            if h < 200.0:
                continue
            found.append(Stain(side, s, seep + 0.24 * h, rnd.uniform(*width), h, 5, rnd.uniform(*spec['strength']),
                               'seep'))
        return found

    def nest_stain(self, spot):
        spec = self.spec['stains']
        if '7' not in spec['cells']:
            return None
        side = spot.side
        s = side.local(spot.point)[0]
        ground = self.ground(side, s)
        top = spot.info['ceiling'] - 15.0
        rnd = draw(self.spec, 'whitewash', spot.point)
        _, width, height = spec['cells']['7']
        h = min(rnd.uniform(*height), top - (ground if ground is not None else side.bottom) - 30.0)
        if h < 100.0:
            return None
        return Stain(side, s, top, rnd.uniform(*width), h, 7, rnd.uniform(*spec['strength']), 'whitewash')

    def new_stain(self, side, s, top, room, cells, source, point):
        spec = self.spec['stains']
        rnd = draw(self.spec, 'stain', point)
        cell = weighted(rnd, {k: v[0] for k, v in cells.items()})
        if cell is None:
            return None
        _, width, height = cells[cell]
        h = min(rnd.uniform(*height), room)
        if h < 200.0:
            return None
        return Stain(side, s, top, rnd.uniform(*width), h, int(cell), rnd.uniform(*spec['strength']), source)

    def fit_stain(self, stain):
        """The stain's box from nine casts into the face across it: its middle and half depth, or False where fewer than
        six meet rock."""
        side, ds = stain.side, []
        for a in (-stain.width / 3.0, 0.0, stain.width / 3.0):
            for share in (0.15, 0.5, 0.85):
                hit = self.into(side, stain.s + a, stain.top - stain.height * share)
                if hit is not None and hit.kind == 'rock':
                    ds.append(side.local(hit.point)[1])
        if len(ds) < 6:
            return False
        middle = (max(ds) + min(ds)) * 0.5
        stain.depth = max(50.0, min(160.0, (max(ds) - min(ds)) * 0.5 + 40.0))
        stain.centre = side.at(stain.s, middle, stain.top - stain.height * 0.5)
        return True


# ---------------------------------------------------------------------------------------------------------------------
# Choosing
# ---------------------------------------------------------------------------------------------------------------------

class Plan:
    """What a run found and chose: the pieces (seen or not, and why), the candidates and the chosen spots per kind, the
    stains, what was turned down per kind and why, the log's lines."""

    def __init__(self):
        self.pieces = []
        self.skipped = {}
        self.candidates = {kind: [] for kind in KINDS + ('stains',)}
        self.chosen = {kind: [] for kind in KINDS + ('stains',)}
        self.why = {kind: {} for kind in KINDS + ('stains',)}
        self.lips = {}
        self.lines = []
        self.casts = 0
        self.seconds = 0.0


def rank(spec, item, side, site):
    """A spot's rank: a draw of its own, raised so spots near a path and on a side seen from near win more often."""
    point = item.point if isinstance(item, Spot) else item.centre
    item.path = site.path_distance(point, spec['pathReach'] * 4.0)
    weight = 1.0 + spec['pathBias'] * math.exp(-item.path / max(spec['pathReach'], 1.0))
    if side.seen is not None:
        weight *= 1.0 + math.exp(-side.seen / 3000.0)
    u = max(draw(spec, 'rank', point, getattr(item, 'mesh', getattr(item, 'cell', ''))).random(), 1e-9)
    item.rank = u ** (1.0 / weight)


def under_drape(spot, drapes, above):
    """The drape whose hang spot stands in (within its width and 40 cm, from `above` cm over its lip down to its hang and
    a margin below), or None."""
    for drape in drapes:
        along = abs(dot(sub(spot.point, drape.point), drape.side.r))
        drop = drape.point[2] - spot.point[2]
        if along < drape.info['width'] * 0.5 + 40.0 and -above <= drop <= drape.info['hang'] + 80.0 and \
                dist2d(spot.point, drape.point) < drape.info['width'] * 0.5 + 150.0:
            return drape
    return None


def conflicts(kind, spot, chosen):
    """Why spot clashes with the kinds chosen before it, or None."""
    if kind in ('cracks', 'ledges', 'nests'):
        drape = under_drape(spot, chosen['drapes'], 50.0)
        if drape is not None:
            return 'under a drape'
    if kind in ('cracks', 'ledges'):
        for other in ('cracks', 'ledges'):
            if other != kind and any(dist(spot.point, o.point) < PLANT_ROOM for o in chosen[other]):
                return 'by another plant'
    if kind == 'nests':
        if any(dist(spot.point, o.point) < NEST_ROOM + 60.0 for o in chosen['cracks'] + chosen['ledges']):
            return 'by a plant'
    if kind == 'slabs':
        if any(dist2d(spot.point, o.point) < SLAB_ROOM for o in chosen['fans']):
            return 'by a fan'
    return None


def solid_check(kind, spot, survey):
    """Why a solid spot won't do (the Site's keep-offs, something solid standing there), or None; and for every spot,
    a building players walk into."""
    site, world = survey.site, survey.world
    if kind == 'fans':
        side = spot.side
        s, d = side.local(spot.point)
        width, reach = spot.info['width'], spot.info['reach']
        points = [side.at(s + a * width * 0.4, d + b * reach, spot.point[2] + 50.0)
                  for a, b in ((0.0, 0.1), (0.0, 0.5), (-1.0, 0.5), (1.0, 0.5), (0.0, 1.0))]
        why = site.blocked(points, True)
        if why:
            return why
        for a in (-0.33, 0.0, 0.33):
            q = side.at(s + a * width, d + reach * 0.5, spot.point[2] + 80.0)
            solid = world.solid(q, min(120.0, width / 3.0))
            if solid:
                return f'by {solid}'
        return None
    if kind == 'slabs':
        why = site.blocked([add(spot.point, (0.0, 0.0, 40.0))], True, spot.info['reach'])
        if why:
            return why
        solid = world.solid(add(spot.point, (0.0, 0.0, 70.0)), spot.info['reach'] + 30.0)
        return f'by {solid}' if solid else None
    reach = spot.info.get('reach', 0.0)
    if reach and spot.info.get('height', 1e9) < LOW_JUNIPER:
        out = add(spot.point, mul(turned((1.0, 0.0, 0.0), spot.yaw), reach))
        why = site.blocked([spot.point, out], True)
        if why:
            return why
    return site.blocked([spot.point], False)


def bucket(reason):
    """A keep-off's reason in a few words for the counts ('in the depot' -> 'in a building')."""
    if reason.startswith('in '):
        return 'in a building'
    if reason in ('on a road', 'on a ramp'):
        return reason
    if reason.startswith('on '):
        return 'on an obstacle or a placed model'
    if reason.startswith('by ') and reason[3:] in ('a road', 'a ramp'):
        return reason
    return 'by a gameplay piece or something solid'


def choose(spec, kind, candidates, plan, survey):
    """Takes the kind's candidates by rank while its cap, perPiece and spacing allow and nothing clashes (the module's
    docstring); returns the chosen."""
    own = spec[kind]
    chosen, per_piece, why = [], {}, plan.why[kind]
    for spot in sorted(candidates, key=lambda c: c.rank, reverse=True):
        if len(chosen) >= own['cap']:
            why['over the cap'] = why.get('over the cap', 0) + 1
            continue
        if per_piece.get(spot.piece.label, 0) >= own['perPiece']:
            why['piece full'] = why.get('piece full', 0) + 1
            continue
        if any(dist(spot.point, other.point) < own['spacing'] for other in chosen):
            why['spacing'] = why.get('spacing', 0) + 1
            continue
        clash = conflicts(kind, spot, plan.chosen)
        if clash:
            why[clash] = why.get(clash, 0) + 1
            continue
        reason = solid_check(kind, spot, survey)
        if reason:
            key = bucket(reason)
            why[key] = why.get(key, 0) + 1
            if kind in ('fans', 'slabs'):
                # The ground pieces' by name: what kept each off (a road, a fence, the depot) is worth reading.
                plan.lines.append(f'{kind}: {spot.mesh} at ({spot.point[0]:.0f}, {spot.point[1]:.0f}, '
                                  f'{spot.point[2]:.0f}) on {spot.piece.label} turned down: {reason}')
            continue
        chosen.append(spot)
        per_piece[spot.piece.label] = per_piece.get(spot.piece.label, 0) + 1
    plan.chosen[kind] = chosen
    return chosen


def choose_stains(spec, plan, survey):
    """Whitewash under every nest first, then the lips' and seeps' stains by rank, cap, perPiece and the gap between
    their rectangles; each fitted to the face (fit_stain) as it's taken."""
    own = spec['stains']
    chosen, per_piece, why = [], {}, plan.why['stains']
    whitewash = [st for st in (survey.nest_stain(n) for n in plan.chosen['nests']) if st is not None]
    for stain in whitewash:
        rank(spec, stain, stain.side, survey.site)
    ranked = whitewash + sorted(plan.candidates['stains'], key=lambda c: c.rank, reverse=True)
    for stain in ranked:
        if len(chosen) >= own['cap']:
            why['over the cap'] = why.get('over the cap', 0) + 1
            continue
        if stain.source != 'whitewash' and per_piece.get(stain.piece.label, 0) >= own['perPiece']:
            why['piece full'] = why.get('piece full', 0) + 1
            continue
        clash = False
        for other in chosen:
            apart = dist2d(stain.centre, other.centre) - (stain.width + other.width) * 0.5
            if apart < own['gap'] and abs(stain.centre[2] - other.centre[2]) < (stain.height + other.height) * 0.5:
                clash = True
                break
        if clash:
            why['too near another'] = why.get('too near another', 0) + 1
            continue
        if survey.site.in_building(stain.centre):
            why['in a building'] = why.get('in a building', 0) + 1
            continue
        if not survey.fit_stain(stain):
            why['no face across it'] = why.get('no face across it', 0) + 1
            continue
        chosen.append(stain)
        per_piece[stain.piece.label] = per_piece.get(stain.piece.label, 0) + 1
    plan.chosen['stains'] = chosen


def survey_and_choose(spec, world, site, pieces, log=None):
    """Finds every kind's candidates on the seen pieces and chooses among them: the Plan (nothing placed)."""
    plan = Plan()
    started = time.time()
    survey = Survey(world, spec, site)
    for piece in pieces:
        centre = ((piece.lo[0] + piece.hi[0]) * 0.5, (piece.lo[1] + piece.hi[1]) * 0.5)
        past = site.past_boundary(centre)
        if past > spec['reach']:
            plan.skipped[piece.label] = f'{past / 100.0:.0f} m past the boundary'
            continue
        sides = []
        for side in piece.sides:
            side.seen = survey.seen(side)
            if side.seen is not None:
                sides.append(side)
        if not sides:
            plan.skipped[piece.label] = 'not seen from the playable area'
            continue
        plan.pieces.append(piece)
        for side in sides:
            if piece.top_course:
                lips = survey.lips(side)
                plan.lips[side.name] = len(lips)
                plan.candidates['drapes'] += survey.drapes(side, lips, plan.why['drapes'])
                plan.candidates['stains'] += survey.lip_stains(side, lips)
            ledges = survey.ledges(side)
            plan.candidates['ledges'] += survey.ledge_spots(side, ledges, plan.why['ledges'])
            plan.candidates['stains'] += survey.seep_stains(side, ledges)
            plan.candidates['cracks'] += survey.cracks(side, plan.why['cracks'])
            plan.candidates['nests'] += survey.overhangs(side, plan.why['nests'])
            if piece.bottom_course:
                feet = survey.feet(side)
                plan.candidates['fans'] += survey.fans(side, feet, plan.why['fans'])
                plan.candidates['slabs'] += survey.slabs(side, feet, plan.why['slabs'])
        if log:
            log(f'{piece.label}: {len(sides)} of {len(piece.sides)} sides seen; '
                f'{sum(1 for k in KINDS for c in plan.candidates[k] if c.piece is piece)} spots so far')
    for kind in KINDS:
        for spot in plan.candidates[kind]:
            rank(spec, spot, spot.side, site)
        choose(spec, kind, plan.candidates[kind], plan, survey)
    for stain in plan.candidates['stains']:
        rank(spec, stain, stain.side, site)
    choose_stains(spec, plan, survey)
    plan.casts = world.casts
    plan.seconds = time.time() - started
    return plan


# ---------------------------------------------------------------------------------------------------------------------
# Settings and the run
# ---------------------------------------------------------------------------------------------------------------------

def settings(build, force=False):
    """level.cliffLife over DEFAULTS (a kind's block over the kind's defaults, key by key), or None when the layout has
    no such block (and force isn't set)."""
    own = build.source.get('level', {}).get('cliffLife')
    if own is None and not force:
        return None
    spec = copy.deepcopy(DEFAULTS)
    for key, value in (own or {}).items():
        if isinstance(value, dict) and isinstance(spec.get(key), dict):
            spec[key].update(value)
        else:
            spec[key] = value
    return spec


def plan_level(build, spec, log=None):
    """The level's pieces, surveyed and chosen among (nothing placed): the Plan, or None when the level has no cliff
    pieces."""
    world = World(build)
    pieces = world.pieces(set(build.cliff_look.get('panels', {})))
    if not pieces:
        return None
    site = level_site(build, spec)
    return survey_and_choose(spec, world, site, pieces, log)


def transform(spot):
    """The spot's world transform: turned by its yaw, leaned to its up (Z) when it has one."""
    if spot.up is None:
        rotation = unreal.Rotator(roll=0.0, pitch=0.0, yaw=spot.yaw)
    else:
        rotation = unreal.MathLibrary.make_rot_from_zx(vec(spot.up), vec(turned((1.0, 0.0, 0.0), spot.yaw)))
    return unreal.Transform(location=vec(spot.point), rotation=rotation, scale=vec(spot.scale))


def spawn(build, spec, plan, meshes):
    """The chosen spots as one AInstancedProps per mesh and the stains as decals (the module's docstring). Returns how
    many instances and decals stood."""
    by_mesh = {}
    for kind in KINDS:
        for spot in plan.chosen[kind]:
            by_mesh.setdefault(spot.mesh, []).append(spot)
    cls = unreal.load_class(None, CLASSES + 'InstancedProps')
    placed = 0
    if cls is None:
        build.warn('cliff life: no InstancedProps class (build the game module first): no plants, nests or rocks')
        by_mesh = {}
    for mesh, spots in sorted(by_mesh.items()):
        if mesh not in KIT:
            build.warn(f'cliff life: {mesh} is not a piece of the kit (level.cliffLife meshes): {len(spots)} left out')
            continue
        if mesh not in meshes:
            build.warn(f'cliff life: no SM_{mesh} yet ({len(spots)} left out): import the kit first')
            continue
        kind, solid, shadow = KIT[mesh]
        actor = build.place(cls, (0.0, 0.0, 0.0), label=f'CliffLife_{mesh}', folder=FOLDER, tags=(TAG,))
        actor.set_editor_property('cull_distance', float(spec[kind]['cull']))
        try:
            actor.set_editor_property('solid', bool(solid))
            actor.set_editor_property('cast_shadows', bool(shadow))
        except Exception as error:  # noqa: BLE001 - an old game module: solid and shadowed
            build.warn(f'CliffLife_{mesh}: {error} (build the game module first): it stays solid and shadowed')
        try:
            # The stains lie on the rock under the plants: never on the plants (a drape's strands in a stain's box).
            actor.get_editor_property('instances').set_editor_property('receives_decals', False)
        except Exception as error:  # noqa: BLE001
            build.warn(f'CliffLife_{mesh}: receives decals ({error})')
        actor.set_instances(unreal.load_asset(meshes[mesh]), [transform(spot) for spot in spots])
        placed += len(spots)
    stains = 0
    if plan.chosen['stains']:
        material = unreal.load_asset(STAIN_MATERIAL) if unreal.EditorAssetLibrary.does_asset_exist(STAIN_MATERIAL) else None
        if material is None:
            build.warn(f'cliff life: no {STAIN_MATERIAL} yet (build_cliff_stain_material.py): no stains')
        else:
            own = spec['stains']
            for n, stain in enumerate(plan.chosen['stains'], 1):
                actor = build.place(unreal.DecalActor, stain.centre, label=f'CliffStain_{n:02d}', folder=STAIN_FOLDER,
                                    tags=(TAG, STAIN_TAG))
                # Its X into the face (the way it projects), its Y up: the cell's top row at the decal's top.
                rotation = unreal.MathLibrary.make_rot_from_xy(vec(mul(stain.side.f, -1.0)), vec(UP))
                actor.set_actor_rotation(rotation, False)
                decal = actor.get_editor_property('decal')
                decal.set_editor_property('decal_material', material)
                decal.set_editor_property('decal_size', unreal.Vector(stain.depth, stain.height * 0.5, stain.width * 0.5))
                code = unreal.LinearColor((stain.cell + 0.5) / 8.0, 1.0, 1.0, stain.strength)
                try:
                    decal.set_editor_property('decal_color', code)
                except Exception:  # noqa: BLE001 - not editable as a property here: its setter
                    decal.set_decal_color(code)
                for prop, value in (('fade_screen_size', float(own['fadeScreenSize'])),
                                    ('sort_order', int(own['sortOrder']))):
                    try:
                        decal.set_editor_property(prop, value)
                    except Exception as error:  # noqa: BLE001
                        build.warn(f'CliffStain_{n:02d}: {prop} not set ({error})')
                stains += 1
    return placed, stains


def report(build, spec, plan, log):
    """The plan's lines: what was skipped, each choice, and per kind the candidates, the chosen and what was turned down."""
    skipped = {}
    for why in plan.skipped.values():
        key = 'past the boundary' if why.endswith('past the boundary') else why
        skipped[key] = skipped.get(key, 0) + 1
    log(f'cliff life: {len(plan.pieces)} pieces looked at, {len(plan.skipped)} skipped ('
        + ', '.join(f'{n} {why}' for why, n in sorted(skipped.items())) + f'); {plan.casts} casts in {plan.seconds:.0f} s')
    for label, why in sorted(plan.skipped.items()):
        log(f'cliff life: {label} skipped: {why}')
    for line in plan.lines:
        log(f'cliff life: {line}')
    for kind in KINDS:
        for spot in plan.chosen[kind]:
            log(f'cliff life: {kind}: {spot.describe()}')
    for n, stain in enumerate(plan.chosen['stains'], 1):
        log(f'cliff life: stain CliffStain_{n:02d}: {stain.describe()}')
    for kind in KINDS + ('stains',):
        why = ', '.join(f'{count} {reason}' for reason, count in sorted(plan.why[kind].items()))
        log(f'cliff life: {kind}: {len(plan.chosen[kind])} chosen of {len(plan.candidates[kind])} candidates '
            f'(cap {spec[kind]["cap"]})' + (f'; turned down: {why}' if why else ''))


def place(build, meshes=None):
    """The cliff life for build's level (the module's docstring), in its CliffLife folder. For build_area.py's full
    build, once the cliffs, models and gameplay pieces stand; the build cleared the folder first."""
    spec = settings(build)
    if spec is None:
        build.log('cliff life: no level.cliffLife block in the layout: none placed')
        return
    meshes = build_area.mesh_index() if meshes is None else meshes
    # A line per piece as it's surveyed: a whole level takes minutes of casts, and the log shows where it is.
    plan = plan_level(build, spec, lambda message: build.log(f'cliff life: {message}'))
    if plan is None:
        build.log('cliff life: no cliff pieces in the level: none placed')
        return
    report(build, spec, plan, build.log)
    placed, stains = spawn(build, spec, plan, meshes)
    build.log(f'cliff life: {placed} instances of {len({s.mesh for k in KINDS for s in plan.chosen[k]})} meshes and '
              f'{stains} stains placed')


def footprints(folder):
    """The fans' and slabs' boxes as placed, for the scatter (build_island_scatter.py's dressing footprints: (x, y, yaw,
    half along Y, half along X, line), line False: solid): from the open level's <folder>/CliffLife actors."""
    out = []
    for actor in level_actors():
        label = str(actor.get_actor_label())
        if not in_folder(actor, f'{folder}/{FOLDER}') or not label.startswith(('CliffLife_ScreeFan_',
                                                                                 'CliffLife_FallenSlab_')):
            continue
        component = actor.get_editor_property('instances')
        mesh = component.static_mesh if component is not None else None
        if mesh is None:
            continue
        box = mesh.get_bounding_box()
        cx, cy = (box.min.x + box.max.x) * 0.5, (box.min.y + box.max.y) * 0.5
        hx, hy = (box.max.x - box.min.x) * 0.5, (box.max.y - box.min.y) * 0.5
        for i in range(component.get_instance_count()):
            t = component.get_instance_transform(i, True)
            centre = t.transform_location(unreal.Vector(cx, cy, 0.0))
            s = t.scale3d
            out.append((centre.x, centre.y, t.rotation.rotator().yaw, hy * abs(s.y), hx * abs(s.x), False))
    return out


def run(area):
    """The cliff life again in the area's level, on its own: what it placed before goes first; saved."""
    importlib.reload(build_area)
    build = build_area.AreaBuild(area)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    if settings(build) is None:
        build.log('cliff life: no level.cliffLife block in the layout: nothing done (apply the patch first)')
        unreal.log('CLIFFLIFE done')
        return
    build.open_level(FOLDER)
    place(build, build_area.mesh_index())
    # The fans' and slabs' rock wears the area's own (level.swaps: MI_RockCliff_Ransom).
    build.swap_materials()
    build_area.levels.save_current_level()
    build.log('cliff life placed and saved')
    unreal.log('CLIFFLIFE done')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_cliff_life.py <Area> (a folder under Art/Levels whose layout has '
                         'level.cliffLife)')
    run(sys.argv[1])
