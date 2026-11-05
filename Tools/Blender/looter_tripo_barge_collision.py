"""The reworked dredge barge's collision (looter_tripo_barge.rework calls it): the points of each piece of the final
mesh, and the UCX_ hulls fitted to them. Everything that stands is a convex hull of its own vertices (26-direction
hulls, looter_tripo_rework.hull): the house and casing from their skirts' feet up, split at HOUSE_SPLIT_Z so a roof's
overhang cannot slope into a walk; the stacks, tanks and cradle; the bulwark in SEG lengths (split at the berth gap);
the scoop's walls in SEG lengths beside the forecastle's floor, its bow plate, and its frame tubes in LATTICE_CELL
chunks up to head height (a hull of the whole frame would roof the forecastle). The hull below the decks is sliced
every HULL_SLICE and, where the hall stands, keeps only its sides beyond the hall's radius; a keel box lies under the
hall's floor. The floors are boxes on the floor levels measured on the mesh (looter_tripo_barge_floors.trenches), the stairs
ramps. Tools/Blender/looter_tripo_barge_check.py measures the result into BARGE_LAYOUT.md.
"""
import math

import numpy as np

import looter_tripo_barge as barge
import looter_tripo_rework as rw
from looter_tripo_barge import (COAMING_H, COAMING_T, CRADLE, CROSS, FORECASTLE, GAP_SILL, GAP_W, GAP_X, HALL, HATCH, SQ_X,
                                SQUEEZE_CASING, STACK_FOOT, STACK_R, STACKS, STAIR_STEPS, STAIR_Y, TANKS, TREAD, UCX,
                                final_x)
from looter_tripo_barge_floors import WALK_STEP, trench_at, trenches
from looter_tripo_rework import U

FLOOR_CLEAR = 3.0            # points this far over a floor belong to what stands on it
WALL_MARGIN = 5.0            # a wall's points: inboard of its walk floor's inner edge + this
HOUSE_SPLIT_Z = 2700.0       # the house's and casing's hulls split here (over head height, 2257 + 180, with margin)
HOUSE_BINS, CABIN_BINS, CASING_BIN = 3, 2, 900.0
LOW_BIN = 150.0              # cm: the low hulls' lengths (a hull of 9 m filled the walls' feet's bends: it stood up to 1.6
                             # m into the walks where a foot runs inboard, the capsule test stopped there)
LOW_CLEAR = 10.0             # cm: the low hulls stay this much more than the walk's width rule off its bulwark
FOOT_MEDIAN = 2              # stations each side a wall's foot is the median over
CABIN_Z, STACK_HEAD_Z, CRADLE_TOP = 3200.0, 4450.0, 950.0
HOUSE_X, CASING_FORE = (-600.0, 2000.0), -640.0  # Tripo's x (final_x() moves them)
HOUSE_Y = 1100.0             # |y| within: the house's and casing's widest (the cabin's walk)
HULL_Z, HULL_SLICE, HULL_ORIGIN = (900.0, 2225.0), 600.0, -4000.0
HALL_X_MARGIN, HALL_R_MARGIN = 40.0, 30.0
KEEL_Y, KEEL_BOTTOM = 400.0, 1000.0
SEG, SEG_OVERLAP = 300.0, 15.0         # bulwark and scoop-wall hulls along x; how far each reaches into the next
FLOOR_NEAR = 2                         # stations either way the forecastle floor's width is the widest of
BULWARK_Z = (2225.0, 2460.0) # the bulwark's hulls: from the hull's top to over the rails' tops
BULWARK_EDGE = 2.0           # a bulwark point: outboard of its floor's outer edge - this
SCOOP_FLOOR_Z, SCOOP_Y = (FORECASTLE - 8.0, FORECASTLE + 20.0), (0.0, 900.0)   # the scoop's floor (Tripo's, 2365-2387)
SCOOP_BOW = 3250.0           # Tripo's x: the scoop's floor ends, its bow plate rises
LATTICE_CELL, LATTICE_HEAD = 40.0, 220.0          # frame tubes: hulls per 40 cm cell (fitted to the tubes, the floor
                                                  # beside them free), up to this over the floor
FRAME_MIN_POINTS = 3                # a tube's chunk of a cell may hold only a few corners
STERN_X, STERN_Y, DECK_DEPTH = -3650.0, 700.0, 100.0
HOLE_EDGES = (-611.0, -436.0, -220.0, 225.0)      # the hatch hole as rays measure it (the coaming's inner faces)
COAMING_HULL_T, COAMING_HULL_H = 20.0, 50.0        # the coaming's hulls: thick, high (their tops on the coaming's)
CROSS_X = (-700.0, -540.0)   # Tripo's x: the cross walk's boxes, overlapping the casing's and house's feet


def pieces(bm, tag):
    """The points of each piece the collision is fitted to (final coordinates, cm), and the floors measured."""
    # The walls' feet as the median of their stations' (a corridor's cap or a door's step at one station read as the
    # foot stepping out there: the low hull reaching from it stood in the walk).
    raw = trenches(bm)
    walk = {(s, k): (float(np.median([raw[(s, k + d)][0] for d in range(-FOOT_MEDIAN, FOOT_MEDIAN + 1)
                                      if (s, k + d) in raw])), hi, top) for (s, k), (lo, hi, top) in raw.items()}
    scoop_x = barge.terrace_aft(bm)
    # The forecastle's floor to the scoop's walls: its outermost faces (a post's foot breaks the run from the middle),
    # the widest of the stations near (a station where a bar's foot sits reads narrow).
    floor = trenches(bm, z_range=SCOOP_FLOOR_Z, y_range=SCOOP_Y)
    step = WALK_STEP
    floor = {k: (t[0], max(floor[(k[0], k[1] + d)][1] for d in range(-FLOOR_NEAR, FLOOR_NEAR + 1)
                           if (k[0], k[1] + d) in floor), t[2]) for k, t in floor.items()}
    house = (final_x(HOUSE_X[0]), final_x(HOUSE_X[1]))
    casing = (SQ_X[0] - 60.0, final_x(CASING_FORE))
    tx0, tx1 = final_x(TANKS[0]), final_x(TANKS[1])
    stacks = [(sx, sy * SQUEEZE_CASING) for sx, sy in STACKS]
    gap = (GAP_X - GAP_W / 2.0, GAP_X + GAP_W / 2.0)
    bow = final_x(SCOOP_BOW)
    pts = {}

    def add(name, p):
        pts.setdefault(name, []).append(p)
    for v in bm.verts:
        x, y, z = p = U(v.co)
        side = '+Y' if y > 0 else '-Y'
        w = trench_at(walk, x, y)
        f = trench_at(floor, x, y) if x > scoop_x - 30.0 else None
        if z < CRADLE_TOP and CRADLE[0] < x < CRADLE[1]:
            add('cradle', p)
        elif z > STACK_FOOT and any(math.hypot(x - sx, y - sy) < STACK_R for sx, sy in stacks):
            k = next(i for i, (sx, sy) in enumerate(stacks) if math.hypot(x - sx, y - sy) < STACK_R)
            add(f'stack {k + 1} {"head" if z > STACK_HEAD_Z else "shaft"}', p)
        elif tx0 < x < tx1 and z > TANKS[3] + 20.0 and abs(y) < TANKS[2]:
            add(f'tank {side}', p)
        elif f and z > f[2] + FLOOR_CLEAR:
            if x > bow:
                add('scoop bow', p)
            elif abs(y) > f[1] - WALL_MARGIN:
                add(f'scoop wall {side} {int((x - scoop_x) // SEG)}', p)
            elif z < f[2] + LATTICE_HEAD:
                cell = tuple(int(c // LATTICE_CELL) for c in p)
                add(f'scoop frame {cell}', p)
        elif (house[0] < x < house[1] or casing[0] < x < casing[1]) and w and z > w[2] + FLOOR_CLEAR \
                and abs(y) < reach(z, w, barge.WALK_RUNS['house' if house[0] < x < house[1] else 'casing'][2]):
            band = 'cabin' if z > CABIN_Z else ('high' if z > HOUSE_SPLIT_Z else 'low')
            if house[0] < x < house[1]:
                bins = CABIN_BINS if band == 'cabin' else HOUSE_BINS
                k = int((x - house[0]) // (LOW_BIN if band == 'low' else (house[1] - house[0]) / bins))
                add(f'cabin {k}' if band == 'cabin' else f'house {k} {band}', p)
            else:
                add(f'casing {int((x - SQ_X[0]) // (LOW_BIN if band == "low" else CASING_BIN))} {band}', p)
        elif w and x < scoop_x and BULWARK_Z[0] < z < BULWARK_Z[1] and abs(y) > w[1] - BULWARK_EDGE:
            # Split at the berth gap's ends: no hull may span it (the gap's own hull is its sill).
            k = int((x - HULL_ORIGIN) // SEG)
            # The gap's own hull is its sill: what stands over the sill there (the cut ends' caps) goes either side.
            inside = gap[0] + 1.0 < x < gap[1] - 1.0 and z < GAP_SILL + 2.0
            part = '' if y > 0 or not gap[0] - SEG < x < gap[1] + SEG else (
                ' gap' if inside else (' aft of the gap' if x < GAP_X else ' fore of the gap'))
            add(f'bulwark {side} {k}{part}', p)
            # Each length's hull reaches SEG_OVERLAP into its neighbours' (convex hulls of their own points left slits).
            u = (x - HULL_ORIGIN) % SEG
            for n in ([k - 1] if u < SEG_OVERLAP else []) + ([k + 1] if u > SEG - SEG_OVERLAP else []):
                add(f'bulwark {side} {n}{part}', p)
        elif HULL_Z[0] < z < HULL_Z[1]:
            add(f'hull {int((x - HULL_ORIGIN) // HULL_SLICE)}', p)
    return pts, {'walk': walk, 'floor': floor, 'scoop_x': scoop_x, 'bow': bow}


def reach(z, w, wide):
    """How far out a point of the house or casing may lie at height z (w: its walk's floor; wide: the walk's width
    rule): up to head height and over (the low hulls) only its own wall's foot line, so no rail post or overhang slopes
    a hull into the walk, and never into the walk's rule width + LOW_CLEAR along the bulwark (a cut's cap at the foot
    read as the foot stepping out); the high hulls to the bulwark's inner face; the cabin to its own walk round it."""
    if z < HOUSE_SPLIT_Z:
        return min(w[0] + WALL_MARGIN, w[1] - wide - LOW_CLEAR)
    return w[1] - BULWARK_EDGE if z < CABIN_Z else HOUSE_Y


def _level(table, keep=lambda k: True):
    """A floor's level: the median of its stations' tops (a fitting's corner on one station does not lift it)."""
    tops = [t[2] for k, t in table.items() if keep(k)]
    return float(np.median(tops)) if tops else None


def build(obj, found):
    pts, levels = found
    box = lambda name, *b: rw.ucx(obj, UCX, name, *b)
    hull = lambda name, p: rw.hull(obj, UCX, name, p)
    hall_x0, hall_x1 = HALL['x'] - HALL['r'] - HALL_X_MARGIN, HALL['x'] + HALL['r'] + HALL_X_MARGIN
    for name, p in sorted(pts.items()):
        if name.startswith('hull'):
            # Below the deck: slices of the skin; where the hall stands only its sides outside the hall's radius.
            a = np.array(p)
            inside = (a[:, 0] > hall_x0) & (a[:, 0] < hall_x1)
            if (~inside).sum() > 8:
                hull(name, a[~inside].tolist())
            side = a[inside]
            for s in (-1, 1):
                q = side[side[:, 1] * s > HALL['r'] + HALL_R_MARGIN]
                if len(q) > 8:
                    hull(f'{name} beside the hall {"+Y" if s > 0 else "-Y"}', q.tolist())
        elif len(p) > (FRAME_MIN_POINTS if name.startswith("scoop frame") else 8):
            hull(name, p)
    box('keel under the hall', hall_x0, hall_x1, -KEEL_Y, KEEL_Y, KEEL_BOTTOM, HALL['floor'] - 10.0)
    walk, floor, scoop_x, bow = levels['walk'], levels['floor'], levels['scoop_x'], levels['bow']
    step = WALK_STEP
    house_fore = final_x(SQ_X[1])
    # The well deck and the forecastle (by SEG lengths, as wide as its floor between the scoop's walls).
    in_well = lambda k: house_fore < k[1] * step < scoop_x
    wz = _level(walk, in_well)
    for n, x0 in enumerate(np.arange(house_fore, scoop_x, SEG)):
        # By SEG lengths, each as wide as its widest station (under the bulwark's hulls, not out past the hull).
        x1 = min(x0 + SEG, scoop_x)
        wy = max((t[1] for k, t in walk.items() if x0 <= k[1] * step <= x1), default=None)
        if wy:
            box(f'well deck {n}', x0, x1, -wy, wy, wz - DECK_DEPTH, wz)
    top = _level(floor)
    for n, x0 in enumerate(np.arange(scoop_x, bow, SEG)):
        x1 = min(x0 + SEG, bow)
        hw = [t[1] for (s, k), t in floor.items() if x0 <= k * step <= x1]
        if hw:
            box(f'forecastle {n}', x0, x1, -max(hw), max(hw), barge.WELL, top)   # under the walls' hulls
    # The side walks, casing's and house's (one box each side: the floor's level, foot to bulwark).
    for s in (-1, 1):
        on = lambda k, s=s: k[0] == s and SQ_X[0] < k[1] * step < house_fore
        run = [t for k, t in walk.items() if on(k)]
        y0, y1 = sorted((s * min(t[0] for t in run), s * max(t[1] for t in run)))
        box(f'walk {"+Y" if s > 0 else "-Y"}', SQ_X[0], house_fore, y0, y1, HALL['ceiling'], _level(walk, on))
    # The cross walk round the hatch.
    hx0, hx1, hy0, hy1 = HATCH
    cx0, cx1 = final_x(CROSS_X[0]), final_x(CROSS_X[1])
    cross = _level(walk, lambda k: cx0 < k[1] * step < cx1) or CROSS
    wy = max(t[1] for (s, k), t in walk.items() if cx0 < k * step < cx1)
    for name, b in (('cross walk aft of the hatch', (cx0, hx0, -wy, wy)),
                    ('cross walk fore of the hatch', (hx1, cx1, -wy, wy)),
                    ('cross walk -Y of the hatch', (hx0, hx1, -wy, hy0)),
                    ('cross walk +Y of the hatch', (hx0, hx1, hy1, wy))):
        box(name, *b, HALL['ceiling'], cross)
    stern = _level(walk, lambda k: k[1] * step < SQ_X[0]) or CROSS
    box('stern platform', STERN_X, SQ_X[0], -STERN_Y, STERN_Y, stern - DECK_DEPTH, stern)
    # The hatch's coaming and the stairs (ramps).
    # The coaming: three thin walls on the hole's measured edges, nothing on +Y (the kit's stair goes down there) and
    # nothing in the hole (drops fall through it to the kit's catwalk).
    (ex0, ex1, ey0, ey1), t = HOLE_EDGES, COAMING_HULL_T
    for name, b in (('coaming aft', (ex0 - t, ex0, ey0 - t, ey1 + t)), ('coaming fore', (ex1, ex1 + t, ey0 - t, ey1 + t)),
                    ('coaming -Y', (ex0 - t, ex1 + t, ey0 - t, ey0))):
        box(name, *b, CROSS + COAMING_H - COAMING_HULL_H, CROSS + COAMING_H)
    for y0, y1 in STAIR_Y:
        run = TREAD * (STAIR_STEPS - 1)
        hull(f'stairs {"+Y" if y0 > 0 else "-Y"}', [(scoop_x - run, y, barge.WELL) for y in (y0, y1)] +
             [(scoop_x, y, z) for y in (y0, y1) for z in (barge.WELL, FORECASTLE)])
