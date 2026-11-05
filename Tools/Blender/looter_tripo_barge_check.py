"""Checks of the reworked dredge barge (looter_tripo_barge.py): its previews, and its layout measured with rays on the
built mesh, written to Docs/Work/2026-11-04-Art/BARGE_LAYOUT.md (the walkable kit's contract: the decks and their
heights, the walks' clear widths, the rails, the berth gap, the hatch, the hall's cavity, what to keep clear of).

    $env:LOOTER_TRIPO_ONLY='DredgeBarge'; Tools\\artrun.ps1 -Script Art\\Models\\Tripo\\Wardens.py -Preview   previews
    Tools\\artrun.ps1 -Script Tools\\Blender\\looter_tripo_barge_check.py                                     layout
The rework's constants only say where to look; every number written is a ray's hit or a vertex (UE frame, cm, pivot
bottom-centre, bow +X, berth side -Y).
"""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_textures as lt
import looter_tripo_barge as barge
import looter_tripo_barge_collision as bc
import looter_tripo_barge_floors as floors
import looter_tripo_barge_route as stern_route
from looter_tripo_barge_previews import preview  # noqa: F401 (Wardens.py -Preview calls barge_check.preview)
from looter_tripo_rework import B, D, U

LAYOUT = os.path.join(lt.REPO, 'Docs', 'Work', '2026-11-04-Art', 'BARGE_LAYOUT.md')
FAR = 9000.0                  # cm: longer than the barge
STEP = 100.0                  # cm between stations along the walks and the well deck
FEET, HIP, HEAD = 20.0, 100.0, 180.0    # heights over a floor where a walk's clear width is measured
MIN_WALK = 150.0              # the kit's rule: a walk at least this wide
CAPSULE = (34.0, 88.0)        # cm: the pawns' capsule (radius, half-height), swept along each side walk's middle
CAPSULE_STEP = 8.0            # cm between the points along its core tested against the collision hulls
CAPSULE_SLACK = 0.5           # cm: a hull this near the capsule's skin still lets it by (the floor boxes it stands on)
MIDDLE_MEDIAN = 3             # stations each side a walk's edges are the median over for its middle line
MIN_HEAD = 200.0              # headroom reported where lower (the player stands ~150 tall, the camera above)
WALK_RULE = {name: (barge.WALK_HEAD, run[2]) for name, run in barge.WALK_RUNS.items()}   # the side walks' headroom and
                              # clear width end to end (looter_tripo_barge): the script fails under either
CROSS_WALK = (-670.0, -350.0) # Tripo's x: the casing's fore face, the house's aft face (the walks' runs end there)
SEG_REPORT = 300.0            # cm between the forecastle's reported widths
CLEAR_STEP = 100.0            # cm between the forecastle's reported clear strips
RAIL_TRACE_Z = 2295.0         # the kit's trace across the -Y rail at the berth
SLOT_SAMPLES = (('well deck', (3200.0, 0.0, 2400.0), (0.0, 0.0, -1.0), 'Deck'),
                ('forecastle floor', (4300.0, 0.0, 2390.0), (0.0, 0.0, -1.0), 'Deck'),
                ('cross walk', (-650.0, 400.0, 2400.0), (0.0, 0.0, -1.0), 'Deck'),
                ('hull below the strake', (3000.0, -2000.0, 1600.0), (0.0, 1.0, 0.0), 'Hull'),
                ('house wall', (1000.0, -2000.0, 2900.0), (0.0, 1.0, 0.0), 'Walls'),
                ('casing wall', (-2000.0, -2000.0, 2800.0), (0.0, 1.0, 0.0), 'Walls'),
                ('stack', (-1107.0, -2000.0, 4000.0), (0.0, 1.0, 0.0), 'Super'),
                ('the well deck bulwark', (2700.0, -2000.0, 2320.0), (0.0, 1.0, 0.0), 'Super'))
HATCH_DROP = 1880.0           # the kit's catwalk under the hatch: nothing may stand in the hole above it
HALL_RINGS, HALL_SPOKES, HALL_LEVELS = (0.0, 0.5, 0.97), 24, 6     # the hall cylinder's sample points
ZONES = ('well', 'forecastle', 'walk', 'cross', 'stern', 'coaming', 'stairs')
PIECES = ('house', 'cabin', 'casing', 'stack', 'tank', 'scoop', 'cradle', 'stairs', 'coaming')


class Rays:
    def __init__(self, obj):
        me = obj.data
        self.tree = BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [tuple(p.vertices) for p in me.polygons])

    def cast(self, p, d, far=FAR):
        """The first hit from p (cm) along d: (point, distance cm) or None."""
        loc, _, _, dist = self.tree.ray_cast(B(p), D(d).normalized(), far / 100.0)
        return None if loc is None else (U(loc), dist * 100.0)

    def down(self, x, y, z):
        hit = self.cast((x, y, z), (0.0, 0.0, -1.0))
        return hit[0][2] if hit else None

    def up(self, x, y, z):
        hit = self.cast((x, y, z), (0.0, 0.0, 1.0))
        return hit[0][2] if hit else None

    def reach(self, p, d):
        hit = self.cast(p, d)
        return hit[1] if hit else FAR


def r0(v):
    return 'none' if v is None else f'{v:.0f}'


def stations(a, b, step=STEP):
    return list(np.arange(a + step / 2.0, b, step))


def measure(obj):
    """The layout as markdown lines, every number measured; FAILS lists what breaks a rule (the script fails after
    writing the layout)."""
    bpy.context.view_layer.update()          # the UCX_ and SOCKET_ children's matrices
    _HULLS.clear()
    FAILS.clear()
    r = Rays(obj)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    V = np.array([U(v.co) for v in obj.data.vertices])
    lo, hi = V.min(0), V.max(0)
    out = ['# Dredge barge layout (measured)', '',
           'SM_DredgeBarge after the round-5 rework (Tools/Blender/looter_tripo_barge.py), measured with rays by '
           'Tools/Blender/looter_tripo_barge_check.py. UE frame, cm: pivot bottom-centre, bow +X, berth side -Y, Z up.',
           f'Bounds: x {lo[0]:.0f}..{hi[0]:.0f}, y {lo[1]:.0f}..{hi[1]:.0f}, z {lo[2]:.0f}..{hi[2]:.0f} '
           f'({(hi - lo)[0]:.0f} x {(hi - lo)[1]:.0f} x {(hi - lo)[2]:.0f}). Triangles '
           f'{sum(len(p.vertices) - 2 for p in obj.data.polygons)}.', '']
    well = _well(r, bm, out)
    _walks(r, bm, out, well)
    _route(out)
    bm.free()
    _cross(r, out)
    _gap(r, out)
    _hall(r, obj, out)
    _slots(obj, r, out)
    _zones(out)
    _avoid(out)
    _sockets(obj, out)
    return out


def _well(r, bm, out):
    mid = barge.final_x(barge.FORE_CUT) + barge.FORE_DX / 2.0     # the well deck's middle
    house = max(h[0][0] for y in range(-600, 601, 100) for z in (HIP, HEAD)
                if (h := r.cast((mid, float(y), barge.WELL + z), (-1.0, 0.0, 0.0))))
    face = {y: r.cast((mid, float(y), barge.WELL + 40.0), (1.0, 0.0, 0.0)) for y in range(-650, 651, 50)}
    fore = {y: h[0][0] for y, h in face.items() if h}
    scoop = fore[0]
    xs = stations(house + 20.0, min(fore.values()) - 20.0)
    widths = [(x, r.reach((x, 0.0, barge.WELL + 50.0), (0.0, 1.0, 0.0)),
               r.reach((x, 0.0, barge.WELL + 50.0), (0.0, -1.0, 0.0))) for x in xs]
    rim = [r.down(x, s * (w + 40.0), 2700.0) for x, wp, wn in widths for s, w in ((1, wp), (-1, wn))]
    zs, bumps = [], []
    for x in xs:
        for y in range(-650, 651, 100):
            z = r.down(x, float(y), barge.WELL + 150.0)
            if z is not None:
                zs.append(z)
                if z > barge.WELL + 8.0:
                    bumps.append((x, y, z))
    yin = min(min(wp, wn) for _, wp, wn in widths)
    out += ['## Well deck', '',
            f'- Deck z {min(zs):.0f}..{max(zs):.0f} (rays down on a 100 cm grid, |y| <= 650); one level.',
            f'- Clear box: x {house:.0f} (the house\'s fore face, its foremost point at z +{HIP:.0f}..+{HEAD:.0f}) .. '
            f'{min(fore.values()):.0f} (the stairs\' foot), |y| <= {yin:.0f} (the bulwark\'s inner face, nearest of '
            f'{len(widths)} stations); {min(fore.values()) - house:.0f} x {2 * yin:.0f}.',
            f'- Bulwark inner face |y|: +Y {min(w for _, w, _ in widths):.0f}..{max(w for _, w, _ in widths):.0f}, '
            f'-Y {min(w for _, _, w in widths):.0f}..{max(w for _, _, w in widths):.0f}; its rim top z '
            f'{min(z for z in rim if z):.0f}..{max(z for z in rim if z):.0f}.',
            '- Things standing on it (ray hits over z +8): ' + (', '.join(
                f'({x:.0f}, {y}, {z:.0f})' for x, y, z in bumps[:12]) if bumps else 'none') + '.', '']
    out += ['## Forecastle and its stairs', '',
            f'- Aft face x {scoop:.0f} at y 0 (rays forward at z +40 across |y| <= 650: '
            + ', '.join(f'y {y} x {x:.0f}' for y, x in sorted(fore.items()) if y % 150 == 0) + ').']
    step = floors.WALK_STEP
    floor = {k: t for k, t in floors.trenches(bm, z_range=bc.SCOOP_FLOOR_Z, y_range=bc.SCOOP_Y).items()
             if k[1] * step > scoop - 100.0}
    if floor:
        xs = sorted({k[1] * step for k in floor})
        half = {x: min(t[1] for k, t in floor.items() if k[1] * step == x) for x in xs}
        top = float(np.median([t[2] for t in floor.values()]))
        frame = [(c[2] - sz[2] / 2.0, n) for n, c, sz, _ in barge.UCX if n.startswith('scoop frame')]
        tubes = [b - top for b, _ in frame if b > top + 25.0]
        out += [f'- Floor (the scoop\'s, up-facing faces at its level): x {xs[0]:.0f}..{xs[-1]:.0f}, z {top:.0f} '
                f'(median), between the scoop\'s walls at |y| ' + ', '.join(
                    f'{half[x]:.0f} (x {x:.0f})' for x in xs[::int(SEG_REPORT / step)]) + '.',
                f'- The scoop\'s frame over it: {len(frame)} collision chunks ({bc.LATTICE_CELL:.0f} cm, fitted to the '
                f'tubes) up to head height; the lowest tube {min(tubes):.0f} over the floor.' if tubes
                else '- The scoop\'s frame: no tube under head height.']
        out.append(f'- Clear of the frame to {MIN_HEAD:.0f} over the floor (rays up on a 25 cm grid; per x, the |y| runs): '
                   + '; '.join(_clear_strips(r, floor, top, x) for x in xs[::int(CLEAR_STEP / step)]) + '.')
    for y0, y1 in barge.STAIR_Y:
        yc = (y0 + y1) / 2.0
        steps = []
        for x in np.arange(scoop - 220.0, scoop + 20.0, 10.0):
            z = r.down(float(x), yc, barge.FORECASTLE + 100.0)
            if z is not None and (not steps or abs(z - steps[-1][1]) > 3.0):
                steps.append((float(x), z))
        out.append(f'- Stairs |y| {min(abs(y0), abs(y1)):.0f}..{max(abs(y0), abs(y1)):.0f} on {"+Y" if y0 > 0 else "-Y"} '
                   f'(rays down at y {yc:.0f}): ' + ', '.join(f'from x {x:.0f} z {z:.0f}' for x, z in steps) + '.')
    out.append('')
    return {'house': house, 'scoop': scoop}


def _clear_strips(r, floor, top, x):
    """Where at x the forecastle's floor has MIN_HEAD clear over it: its runs of y, as 'x X: y0..y1, ...'."""
    step = floors.WALK_STEP
    half = max((t[1] for k, t in floor.items() if abs(k[1] * step - x) < step), default=0.0)
    ys = list(np.arange(-half + 12.5, half, 25.0))
    free = [(r.up(x, float(y), top + 5.0) or 9999.0) - top >= MIN_HEAD for y in ys]
    runs, start, prev = [], None, None
    for y, ok in zip(ys + [None], free + [False]):
        if ok and start is None:
            start = y
        elif not ok and start is not None:
            runs.append(f'{start - 12.5:.0f}..{prev + 12.5:.0f}')
            start = None
        prev = y
    return f'x {x:.0f}: ' + (', '.join(runs) if runs else 'none')


def _walks(r, bm, out, well):
    """Each side walk at every trench station: the floor's level and edges, the clear width across it (rays from the
    floor's middle at FEET, HIP, HEAD over it), the headroom."""
    out += ['## Side walks (casing and house)', '']
    table, step = floors.trenches(bm), floors.WALK_STEP
    runs = (('casing', barge.SQ_X[0] + 20.0, barge.final_x(CROSS_WALK[0])),
            ('house', barge.final_x(CROSS_WALK[1]), well['house'] - 20.0))
    for s in (1, -1):
        for name, x0, x1 in runs:
            # End to end: a station where no floor is found (something low lies over it: a strap's cut foot) is measured
            # on its neighbours' floor, from their highest (over the stub).
            found = {k: t for (sd, k), t in table.items() if sd == s and x0 < k * step < x1}
            gaps = [k for k in range(math.ceil(x0 / step), math.floor(x1 / step) + 1)
                    if x0 < k * step < x1 and k not in found]
            for k in gaps:
                near = [found[k + d] for d in (1, -1, 2, -2) if k + d in found]
                if near:
                    found[k] = (max(t[0] for t in near), min(t[1] for t in near), max(t[2] for t in near))
            rows = []
            keys = sorted(found)
            for i, k in enumerate(keys):
                lo, hi, top = found[k]
                x = k * step
                # The walk's middle line: its edges the median of its neighbours' (where a wall's foot steps in, a door
                # or the casing's corner at the cross walk, the floor's run reaches inboard: no pawn keeps to that).
                near = [found[j] for j in keys[max(i - MIDDLE_MEDIAN, 0):i + MIDDLE_MEDIAN + 1]]
                ym = s * floors.walk_middle(float(np.median([t[0] for t in near])), float(np.median([t[1] for t in near])),
                                            WALK_RULE[name][1])
                across = [r.reach((x, ym, top + h), (0.0, s, 0.0)) + r.reach((x, ym, top + h), (0.0, -s, 0.0))
                          for h in (FEET, HIP, HEAD)]
                rows.append((x, lo, hi, top, min(across), (r.up(x, ym, top + floors.STEP_OVER) or 9999.0) - top, across,
                             ym))
            if not rows:
                continue
            col = lambda i: [q[i] for q in rows]
            worst = min(rows, key=lambda q: q[4])
            head_min, wide_min = WALK_RULE[name]
            narrow = [q for q in rows if q[4] < wide_min]
            low = [q for q in rows if q[5] < head_min]
            FAILS.extend(f'{"+Y" if s > 0 else "-Y"} {name} walk at x {q[0]:.0f}: headroom {q[5]:.0f}, clear width '
                         f'{q[4]:.0f} (the rule: {head_min:.0f}, {wide_min:.0f})' for q in rows
                         if q[5] < head_min or q[4] < wide_min)
            unmeasured = [k * step for k in gaps if k not in found]
            if unmeasured:
                FAILS.append(f'{"+Y" if s > 0 else "-Y"} {name} walk: no floor found near x '
                             + ', '.join(f'{x:.0f}' for x in unmeasured))
            # The collision: the pawns' capsule along the walk's middle, at every station and between them.
            path = [(q[0], q[7], q[3]) for q in rows]
            path += [tuple((a[i] + b[i]) / 2.0 for i in range(3)) for a, b in zip(path, path[1:]) if b[0] - a[0] < 2 * step]
            blocked = {}
            for x, y, base in path:
                for hull in _capsule_hits(x, y, base):
                    blocked.setdefault(hull, []).append(x)
            if blocked:
                FAILS.append(f'{"+Y" if s > 0 else "-Y"} {name} walk: the capsule is stopped by ' + '; '.join(
                    f'{h} (x {min(xs):.0f}..{max(xs):.0f})' for h, xs in sorted(blocked.items())))
            by_h = ', '.join(f'+{h:.0f}: {min(q[6][i] for q in rows):.0f}' for i, h in enumerate((FEET, HIP, HEAD)))
            out.append(f'- {"+Y" if s > 0 else "-Y"} {name} walk, x {rows[0][0]:.0f}..{rows[-1][0]:.0f} ({len(rows)} '
                       f'stations): floor z {min(col(3)):.0f}..{max(col(3)):.0f}, from the wall\'s foot |y| '
                       f'{min(col(1)):.0f}..{max(col(1)):.0f} to the bulwark\'s inner face |y| {min(col(2)):.0f}..'
                       f'{max(col(2)):.0f}; clear width (z +{FEET:.0f}..+{HEAD:.0f}) {worst[4]:.0f} at least (x '
                       f'{worst[0]:.0f}; by height {by_h}), median {np.median(col(4)):.0f}; headroom {min(col(5)):.0f} at '
                       f'least, under {head_min:.0f} at {len(low)} stations'
                       + (': ' + ', '.join(f'x {q[0]:.0f} ({q[5]:.0f})' for q in low[:10]) if low else '') + '. '
                       + (f'Under {wide_min:.0f} wide at {len(narrow)} stations: ' + ', '.join(
                           f'x {q[0]:.0f} ({q[4]:.0f})' for q in narrow[:8]) if narrow else f'Nowhere under {wide_min:.0f}.')
                       + (f' Measured on their neighbours\' floor (none found under a low cut stub): x '
                          + ', '.join(f'{k * step:.0f}' for k in gaps) + '.' if gaps else '')
                       + f' A capsule (radius {CAPSULE[0]:.0f}, half-height {CAPSULE[1]:.0f}) along its middle: ' + (
                           '; '.join(f'stopped by {h} at x {min(xs):.0f}..{max(xs):.0f}' for h, xs in sorted(blocked.items()))
                           if blocked else 'no collision hull in its way') + '.')
    out.append('')


def _route(out):
    """The stern push's way up each side walk to the well deck, as Looter.World.Barge.Fight walks it (its cells, its
    Dredger, on the UCX hulls): the script fails where it is shut."""
    out += ["## The stern push's way (as Looter.World.Barge.Fight walks it: looter_tripo_barge_route)", '']
    for side in (-1, 1):
        ok, far, why = stern_route.route(_hulls(), side)
        name = '+Y' if side > 0 else '-Y'
        out.append(f'- {name}: ' + ('open to the well deck' if ok else f'shut: it gets to x {far:.0f}, then {why}') + '.')
        if not ok:
            FAILS.append(f'{name} stern route shut past x {far:.0f}: {why}')
    out.append('')


def _cross(r, out):
    hx0, hx1, hy0, hy1 = barge.HATCH
    cx, cy = (hx0 + hx1) / 2.0, (hy0 + hy1) / 2.0
    aft = cx - r.reach((cx, 0.0, barge.CROSS + 120.0), (-1.0, 0.0, 0.0))
    fore = cx + r.reach((cx, 0.0, barge.CROSS + 120.0), (1.0, 0.0, 0.0))
    zs = [z for x in np.arange(aft + 20.0, fore - 10.0, 20.0) for y in range(-800, 801, 100)
          if not (hx0 - 20 < x < hx1 + 20 and hy0 - 20 < y < hy1 + 20)
          and (z := r.down(float(x), float(y), barge.CROSS + 40.0)) is not None]
    # Rays inside the casing or the house fall through them too (no deck inside): only between their faces.
    holes = [(float(x), float(y)) for x in np.arange(max(hx0 - 60.0, aft + 5.0), min(hx1 + 61.0, fore - 5.0), 5.0)
             for y in np.arange(hy0 - 60.0, hy1 + 61.0, 5.0)
             if (r.down(float(x), float(y), barge.CROSS + 200.0) or 0.0) < barge.CROSS - 100.0]
    rim = [z for x in np.arange(hx0 - 14.0, hx1 + 15.0, 7.0) for y in (hy0 - 7.0,)
           if (z := r.down(float(x), float(y), barge.CROSS + 200.0)) is not None]
    rim += [z for x in (hx0 - 7.0, hx1 + 7.0) for y in np.arange(hy0, hy1, 10.0)
            if (z := r.down(float(x), float(y), barge.CROSS + 200.0)) is not None]
    edge = [z for x in np.arange(hx0 + 10.0, hx1 - 9.0, 10.0)
            if (z := r.down(float(x), hy1 + 7.0, barge.CROSS + 200.0)) is not None]
    below = r.down(cx, cy, barge.CROSS - 20.0)
    column = [(float(x), float(y), float(z)) for x in np.arange(hx0 + 15.0, hx1 - 14.0, 20.0)
              for y in np.arange(hy0 + 15.0, hy1 - 14.0, 20.0) for z in np.arange(HATCH_DROP, barge.CROSS + 60.0, 20.0)]
    blocked = _inside_ucx(column)
    out += ['## Cross walk and hatch', '',
            f'- Cross walk: x {aft:.0f} (the casing\'s fore face at z +120, y 0) .. {fore:.0f} (the house\'s aft face), '
            f'deck z {min(zs):.0f}..{max(zs):.0f}.',
            f'- Hatch hole (rays down on a 5 cm grid fall past z {barge.CROSS - 100:.0f}): x '
            f'{min(h[0] for h in holes):.0f}..{max(h[0] for h in holes):.0f}, y {min(h[1] for h in holes):.0f}..'
            f'{max(h[1] for h in holes):.0f} ({len(holes)} rays); centre ({cx:.0f}, {cy:.0f}). Coaming top z '
            f'{min(rim):.0f}..{max(rim):.0f} on its -Y, aft and fore sides; its +Y edge open (the deck beside it z '
            f'{min(edge):.0f}..{max(edge):.0f}: where the kit puts its stair); through the hole the first thing '
            f'below is at z {r0(below)}. Collision in the hole from z {HATCH_DROP:.0f} up: '
            + (', '.join(sorted(blocked)) if blocked else 'none') + '.', '']


def _gap(r, out):
    xs = np.arange(barge.GAP_X - 250.0, barge.GAP_X + 251.0, 5.0)
    z = barge.WELL + 40.0
    open_ = [float(x) for x in xs if (h := r.cast((float(x), -1500.0, z), (0.0, 1.0, 0.0))) and h[0][1] > -600.0]
    side = min(r.reach((x, -1500.0, z), (0.0, 1.0, 0.0)) for x in (barge.GAP_X - 400.0, barge.GAP_X + 400.0))
    sill = r.down(barge.GAP_X, -1500.0 + side + 20.0, barge.WELL + 200.0)
    rim = r.down(barge.GAP_X - 400.0, -1500.0 + side + 20.0, barge.WELL + 200.0)
    out += ['## Rail line and the berth gap', '',
            f'- The gap in the -Y bulwark (rays inboard at z +40 pass): x {min(open_):.0f}..{max(open_):.0f} '
            f'({max(open_) - min(open_):.0f} wide); its sill z {r0(sill)}, the rim beside it z {r0(rim)}; the hull\'s side '
            f'outboard there |y| {1500.0 - side:.0f}.' if open_ else '- The berth gap: no ray passes (closed).']
    # The rail's collision either side of the gap (a trace across the -Y rail at the rail's mid height).
    held = [bool(_inside_ucx([(float(x), y, RAIL_TRACE_Z) for y in np.arange(-845.0, -644.0, 15.0)])) for x in xs]
    runs, start = [], None
    for x, ok in zip(list(xs) + [None], held + [True]):
        if not ok and start is None:
            start = x
        elif ok and start is not None:
            runs.append(f'{start:.0f}..{prev:.0f}')
            start = None
        prev = x
    out += [f'- The -Y rail\'s collision at z {RAIL_TRACE_Z:.0f} (points across y -845..-645, every 5 cm over x '
            f'{xs[0]:.0f}..{xs[-1]:.0f}): open over x ' + (', '.join(runs) if runs else 'nothing') + ', held elsewhere.', '']


def _hall(r, obj, out):
    hx, hy, R = barge.HALL['x'], barge.HALL['y'], barge.HALL['r']
    floor, ceiling = barge.HALL['floor'], barge.HALL['ceiling']
    pts = [(hx + k * R * math.cos(a), hy + k * R * math.sin(a), z) for k in HALL_RINGS
           for a in np.linspace(0.0, 2.0 * math.pi, HALL_SPOKES, endpoint=False)
           for z in np.linspace(floor + 20.0, ceiling - 20.0, HALL_LEVELS)]
    radial = []
    for z in np.linspace(floor + 20.0, ceiling - 20.0, HALL_LEVELS):
        for a in np.linspace(0.0, 2.0 * math.pi, HALL_SPOKES, endpoint=False):
            radial.append((r.reach((hx, hy, float(z)), (math.cos(a), math.sin(a), 0.0)), float(z), math.degrees(a)))
    tight = sorted(radial)[:4]
    roof = [r.up(hx + k * R * math.cos(a), hy + k * R * math.sin(a), floor + 50.0) for k in HALL_RINGS
            for a in np.linspace(0.0, 2.0 * math.pi, HALL_SPOKES, endpoint=False)]
    bottom = [r.down(hx + k * R * math.cos(a), hy + k * R * math.sin(a), ceiling - 50.0) for k in HALL_RINGS
              for a in np.linspace(0.0, 2.0 * math.pi, HALL_SPOKES, endpoint=False)]
    inside = _inside_ucx(pts)
    out += ['## The hall\'s cavity', '',
            f'- Cylinder centre ({hx:.0f}, {hy:.0f}), R {R:.0f}, floor z {floor:.0f}, ceiling z {ceiling:.0f}.',
            f'- Nearest mesh from the axis (rays out in {HALL_SPOKES} directions at {HALL_LEVELS} heights): '
            + ', '.join(f'{d:.0f} at z {z:.0f}, {a:.0f} deg' for d, z, a in tight) + '.',
            f'- Deck underside over it (rays up from the floor + 50): z {r0(min(z for z in roof if z))}..'
            f'{r0(max(z for z in roof if z))}; the hull\'s bottom under it (rays down from the ceiling - 50): z '
            f'{r0(min(z for z in bottom if z))}..{r0(max(z for z in bottom if z))}.',
            '- Collision inside it: ' + (', '.join(sorted(inside)) if inside else 'none') + ' (sample points inside a UCX hull).',
            '- Over it the deck is not closed everywhere (Tripo made no deck inside the casing: rays up reach the stack): '
            "the hall's own ceiling closes it.",
            '']


_HULLS = []
FAILS = []


def _hulls():
    """Each UCX hull as (name, its bounds' low and high corners, its outward planes, a tree of its faces), made once
    per measure()."""
    if not _HULLS:
        for name, _, _, ob in barge.UCX:
            M = ob.matrix_local
            verts = [M @ v.co for v in ob.data.vertices]
            c = sum(verts, Vector()) / len(verts)
            planes = []
            for poly in ob.data.polygons:
                n = (M.to_3x3() @ poly.normal).normalized()
                q = verts[poly.vertices[0]]
                planes.append((n if n.dot(q - c) >= 0.0 else -n, q))
            lo = Vector([min(v[i] for v in verts) for i in range(3)])
            hi = Vector([max(v[i] for v in verts) for i in range(3)])
            tree = BVHTree.FromPolygons(verts, [tuple(p.vertices) for p in ob.data.polygons])
            _HULLS.append((name, lo, hi, planes, tree))
    return _HULLS


def _capsule_hits(x, y, base):
    """The collision hulls (not the floor boxes it stands on) a pawn's capsule standing at (x, y) on base (cm) meets."""
    r, half = CAPSULE
    core = [B((x, y, z)) for z in np.arange(base + r, base + 2.0 * half - r + 0.1, CAPSULE_STEP)]
    reach = (r - CAPSULE_SLACK) / 100.0
    hits = set()
    for name, lo, hi, planes, tree in _hulls():
        if name.split(' ')[0] in ZONES:
            continue
        for b in core:
            if not all(lo[i] - reach <= b[i] <= hi[i] + reach for i in range(3)):
                continue
            if all(n.dot(b - q) < 0.0 for n, q in planes) or tree.find_nearest(b)[3] < reach:
                hits.add(name)
                break
    return hits


def _inside_ucx(pts):
    """The names of the UCX hulls any of pts (cm) lies inside."""
    hits = set()
    bs = [B(p) for p in pts]
    for name, lo, hi, planes, _ in _hulls():
        for b in bs:
            if all(lo[i] <= b[i] <= hi[i] for i in range(3)) and all(n.dot(b - q) < -0.01 for n, q in planes):
                hits.add(name)
                break
    return hits


def _slots(obj, r, out):
    """The material slots: faces per slot, and a face hit in each region checked against the slot it must be in (the
    script fails if one is wrong: round 3's export had every face one slot off)."""
    me = obj.data
    names = [m.name for m in me.materials]
    counts = [0] * len(names)
    for p in me.polygons:
        counts[p.material_index] += 1
    out += ['## Material slots', '', '- ' + ', '.join(f'{n} {c}' for n, c in zip(names, counts)) + '.']
    wrong = []
    for region, p, d, slot in SLOT_SAMPLES:
        loc, _, i, _ = r.tree.ray_cast(B(p), D(d).normalized(), FAR / 100.0)
        got = names[me.polygons[i].material_index] if i is not None else 'nothing'
        ok = got.endswith('_' + slot)
        out.append(f'- {region}: {got}' + ('' if ok else f' (WRONG: should be _{slot})'))
        if not ok:
            wrong.append(region)
    out.append('')
    if len(names) != len(barge.SLOTS) or wrong:
        FAILS.append(f'material slots wrong: {names}, regions {wrong}')


def _zones(out):
    """The floors as boxes: the collision's floor boxes, each laid on its floor's measured level."""
    out += ['## Deck zones (the floor boxes: x, y, top z)', '']
    for n, c, sz, _ in barge.UCX:
        if n.split(' ')[0] in ZONES:
            out.append(f'- {n}: x {c[0] - sz[0] / 2:.0f}..{c[0] + sz[0] / 2:.0f}, y {c[1] - sz[1] / 2:.0f}..'
                       f'{c[1] + sz[1] / 2:.0f}, top z {c[2] + sz[2] / 2:.0f}')
    out.append('')


def _avoid(out):
    """Footprints from the collision hulls' bounds; the scoop's frame chunks summed into one line each list."""
    frame = [(c, sz) for n, c, sz, _ in barge.UCX if n.startswith('scoop frame')]
    rest = [e for e in barge.UCX if not e[0].startswith('scoop frame')]
    span = lambda c, sz, a: f'{c[a] - sz[a] / 2:.0f}..{c[a] + sz[a] / 2:.0f}'
    out += ['## Footprints to keep clear of (the collision hulls\' bounds)', '']
    out += [f'- {n}: x {span(c, sz, 0)}, y {span(c, sz, 1)}, z {span(c, sz, 2)}' for n, c, sz, _ in rest
            if n.split(' ')[0] in PIECES]
    if frame:
        lo = np.min([[c[a] - sz[a] / 2 for a in range(3)] for c, sz in frame], 0)
        hi = np.max([[c[a] + sz[a] / 2 for a in range(3)] for c, sz in frame], 0)
        out.append(f'- scoop frame (tubes and posts over the forecastle, {len(frame)} chunks of {bc.LATTICE_CELL:.0f} cm): '
                   f'x {lo[0]:.0f}..{hi[0]:.0f}, y {lo[1]:.0f}..{hi[1]:.0f}, z {lo[2]:.0f}..{hi[2]:.0f}')
    out += ['', f'## Collision (UCX_), {len(barge.UCX)} hulls', '']
    out += [f'- {n}: centre ({c[0]:.0f}, {c[1]:.0f}, {c[2]:.0f}), size {sz[0]:.0f} x {sz[1]:.0f} x {sz[2]:.0f}'
            for n, c, sz, _ in rest]
    if frame:
        out.append(f'- scoop frame: {len(frame)} hulls of {bc.LATTICE_CELL:.0f} cm chunks (bounds above)')
    out.append('')


def _sockets(obj, out):
    out += ['## Sockets', '']
    for ob in sorted(obj.children, key=lambda o: o.name):
        if ob.name.startswith('SOCKET_'):
            x, y, z = U(ob.matrix_local.translation)
            yaw = -math.degrees(ob.matrix_local.to_euler().z)
            out.append(f'- {ob.name[7:]}: ({x:.0f}, {y:.0f}, {z:.0f}), yaw {yaw:.0f}')
    out.append('')


if __name__ == '__main__':
    import looter_tripo_wardens as tw
    built, report = tw.build('DredgeBarge')
    lines = measure(built)
    os.makedirs(os.path.dirname(LAYOUT), exist_ok=True)
    with open(LAYOUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines))
    print('\n'.join('LAYOUT ' + line for line in lines), flush=True)
    if FAILS:
        raise ValueError('the barge breaks the layout rules: ' + '; '.join(FAILS))
