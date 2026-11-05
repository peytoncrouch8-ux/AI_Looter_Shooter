"""The dredge barge's side walks cleared for the walkable kit (looter_tripo_barge.rework, on Tripo's mesh before the
squeeze, in Tripo's x): where Tripo's pieces cross a walk at a pawn's height they are cut where they cross, each side's
cut closed in the paint beside it, so the squeeze then carries the wall's side in with the wall and leaves the
bulwark's side where it is. Cut: the bulwark's capping lip that roofs Tripo's slot trench at the casing's aft end and
dome end (squeezed, it was a shelf 26-50 cm over the walk), the casing's band straps (a bridge 117-137 cm up) and the
horn pipes off the house's sides (from the house wall over the rail, 38-119 cm up). Cut before the squeeze, the cut
spans only Tripo's narrow trench and each side's chain is near straight; cut after, the stretched faces zigzagged and
their caps stood out into the walk. What that leaves (lips under ENV_LOW, shelves and brackets outside CLEAR_X, the
canopies' edges) the corridors take after the squeeze: each walk measured as the layout measures it, a box cut out of
what stands over every stretch that falls short (corridors). Also here: the forecastle's stub posts taken off its floor
(posts) and a copy's rail post left out of it (trim, for the cross walk's copies).
"""
import bmesh
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_tripo_barge_floors as fl
import looter_tripo_rework as rw
from looter_tripo_rework import B, D, U, bisect, spanning, ue_centre

CLEAR_X = ((-3100.0, -2990.0),         # Tripo's x: the casing's aft end (the lip)
           (-2875.0, -2675.0),         # the casing's dome end (the lip)
           (-2425.0, -2200.0),         # the casing's band straps
           (1120.0, 1400.0))           # the house's horn pipes (final x 1360..1640)
ENV_IN, ENV_OUT = 2.0, 2.0             # cm: the cut from the wall's foot + ENV_IN to the bulwark's face - ENV_OUT
ENV_LOW, ENV_HIGH = 15.0, 220.0        # cm over the walk's floor (a step is free under ENV_LOW)


def walks(bm, tag, nearest, fill):
    """Cuts what crosses the walks in CLEAR_X; returns (faces cut, cap faces)."""
    walk = fl.trenches(bm)
    doomed = []
    for f in bm.faces:
        x, y, z = ue_centre(f)
        if not any(a < x < b for a, b in CLEAR_X):
            continue
        t = fl.trench_at(walk, x, y)
        if t and t[0] + ENV_IN < abs(y) < t[1] - ENV_OUT and t[2] + ENV_LOW < z < t[2] + ENV_HIGH:
            doomed.append(f)
    # Closed side by side: a cut running across the walk closes at each side, not over the walk.
    mids = {f: (t[0] + t[1]) / 2.0 for f in doomed if (t := fl.trench_at(walk, *ue_centre(f)[:2]))}
    rim = {e: next((mids[f] for f in e.link_faces if f in mids), None) for f in doomed for e in f.edges}
    bmesh.ops.delete(bm, geom=doomed, context='FACES')
    caps = 0
    for outer in (False, True):
        edges = [e for e, m in rim.items() if e.is_valid and e.is_boundary and m is not None
                 and (abs(sum(U(v.co)[1] for v in e.verts) / 2.0) > m) == outer]
        caps += _close(bm, tag, nearest, fill, edges)
    return len(doomed), caps


POSTS_X, POST_TALL, POST_WIDE, POST_FOOT = (4080.0, 5100.0), 100.0, 160.0, 8.0   # final x; a stub post: no taller,
                                       # no wider than these (cm), standing within POST_FOOT of the floor


def posts(bm, tag, nearest, fill):
    """Takes the dark stub posts (Tripo's bollards, a row of tombstones in the game) off the forecastle's floor, the
    floor closed under them; returns (posts, faces)."""
    floor = fl.trenches(bm, z_range=(2363.0, 2392.0), y_range=(0.0, 900.0))
    top = sorted(t[2] for t in floor.values())[len(floor) // 2] if floor else 2376.0
    near = set()
    for f in bm.faces:
        x, y, z = ue_centre(f)
        t = fl.trench_at(floor, x, y)
        low = top - 3.0 < z < top + POST_TALL and not (f.normal.z > fl.DECK_UP and z < top + 3.0)   # feet too
        if POSTS_X[0] < x < POSTS_X[1] and t and abs(y) < t[1] - 10.0 and low:
            near.add(f)
    found, seen = [], set()
    for f in near:
        if f in seen:
            continue
        piece, todo = [], [f]
        seen.add(f)
        while todo:
            g = todo.pop()
            piece.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h in near and h not in seen:
                        seen.add(h)
                        todo.append(h)
        ps = [U(v.co) for g in piece for v in g.verts]
        span = max(max(p[a] for p in ps) - min(p[a] for p in ps) for a in (0, 1))
        if min(p[2] for p in ps) < top + POST_FOOT and max(p[2] for p in ps) < top + POST_TALL and span < POST_WIDE:
            found.append(piece)
    faces = [g for piece in found for g in piece]
    rim = {e for g in faces for e in g.edges}
    bmesh.ops.delete(bm, geom=faces, context='FACES')
    _close(bm, tag, nearest, fill, [e for e in rim if e.is_valid and e.is_boundary])
    return len(found), len(faces)


HEAD_SPARE = 5.0                       # cm: a corridor clears its walk's middle this much over the headroom rule
WIDE_SPARE = 4.0                       # cm: a corridor this much wider than its walk's rule
CORRIDOR = (4.0, 2.0)                  # cm: a corridor clears from this over the floor (a plate's lip under it stays) to
                                       # this over the headroom rule (what hangs higher, a canopy, is left whole: cut, its
                                       # shell lay open over the walk and the floors under it read as inside it)
WALL_KEEP = 30.0                       # cm off the wall's foot a corridor stays where its walk is wide enough (the walls
                                       # lean out over their feet: no notch cut in them)
SEG_JOIN, SEG_PAD = 50.0, 10.0         # cm: failing stations this near share a corridor; it reaches this past them
TOP_JOIN = 3.0                         # cm: and their floors this level: a corridor clears from its lowest floor + CORRIDOR[0]
                                       # up (from its highest, its cap stood in a lower floor's headroom), under TOP_JOIN its
                                       # highest floor stays whole
EDGE_JOIN = 20.0                       # cm: and their walk's edges this even (a box spans its stations' narrowest walk:
                                       # where the walk widened, what hung over its outer part stayed)
CROSS_AT = (20.0, 100.0, 180.0)        # cm over the floor: the layout's heights for a walk's width
FAR = 2000.0                           # cm: a ray across a walk
CORRIDOR_ROUNDS = 1                    # the walks measured and corridors cut at most this many times (a second round's
                                       # caps broke the floors' runs the collision is fitted to)
SEAL_FLAT = 1.0                        # cm (rms off its plane): a cut's leftover open loop flat enough to fill whole
MIDDLE_MEDIAN = 3                      # stations each side a walk's edges are the median over (as the layout's check)
STUB_CUT = 2.0                         # cm over its neighbours' floor a station with none found is cut down to


def corridors(bm, tag, nearest, fill, runs, head):
    """After the squeeze: every side walk station measured as the layout measures it (its middle clear to head,
    its width across at CROSS_AT); over each stretch that falls short a box (from the wall's foot + WALL_KEEP or less,
    to the bulwark's face - ENV_OUT, CORRIDOR over the floor) cut out of whatever stands in it, the cut capped facing
    into the box in the paint beside it; measured again after each round (two corridors' cuts beside each other can
    open a shell over a station between them), up to CORRIDOR_ROUNDS. runs: {name: (Tripo's x from, to, the walk's
    width rule)}; head: the headroom rule. Returns (corridors, faces cut, cap faces)."""
    total = [0, 0, 0]
    for k in range(CORRIDOR_ROUNDS):
        got = _corridor_round(bm, tag, nearest, fill, runs, head, k == 0)
        total = [a + b for a, b in zip(total, got)]
        if not got[0]:
            break
    return tuple(total)


def _corridor_round(bm, tag, nearest, fill, runs, head, first):
    """One round of corridors (see corridors()); the stations with no floor found only in the first."""
    table = fl.trenches(bm)
    tree = BVHTree.FromBMesh(bm)
    step = fl.WALK_STEP

    def reach(p, d, far):
        hit = tree.ray_cast(B(p), D(d), far / 100.0)
        return far if hit[0] is None else hit[3] * 100.0
    failing, stubs = {}, []
    # A station with no floor found (a strap's foot over it, what the walks' clear left under ENV_LOW) is cut down to
    # STUB_CUT over its neighbours' floor, its own station's width only (a cap that high reads as the floor again).
    for name, (a, b, _) in runs.items():
        for s in (-1, 1):
            for k in range(int(np.ceil(a / step)), int(np.floor(b / step)) + 1):
                if first and (s, k) not in table and a < k * step < b:
                    near = [table[(s, k + d)] for d in (1, -1, 2, -2) if (s, k + d) in table]
                    if near:
                        stubs.append((s, name, [k], (max(r[0] for r in near), min(r[1] for r in near),
                                                     min(r[2] for r in near))))
    # Each station's edges as the median of its neighbours' (where a wall's foot steps in, a door or a corner, the
    # floor's run reaches inboard: its middle lies under the house, not on the walk, and its box cut the house).
    edges = {}
    for name, (a, b, _) in runs.items():
        for s in (-1, 1):
            keys = sorted(k for (sd, k) in table if sd == s and a < k * step < b)    # the run's own, as the check
            for i, k in enumerate(keys):
                near = [table[(s, j)] for j in keys[max(i - MIDDLE_MEDIAN, 0):i + MIDDLE_MEDIAN + 1]]
                edges[(s, k)] = (float(np.median([t[0] for t in near])), float(np.median([t[1] for t in near])))
    for (s, k), (_, _, top) in table.items():
        x = k * step
        run = next((name for name, (a, b, _) in runs.items() if a < x < b), None)
        if run is None:
            continue
        lo, hi = edges[(s, k)]
        ym = s * fl.walk_middle(lo, hi, runs[run][2])
        clear = reach((x, ym, top + fl.STEP_OVER), (0.0, 0.0, 1.0), head + HEAD_SPARE) + fl.STEP_OVER
        wide = min(reach((x, ym, top + h), (0.0, s, 0.0), FAR) + reach((x, ym, top + h), (0.0, -s, 0.0), FAR)
                   for h in CROSS_AT)
        if clear < head + HEAD_SPARE or wide < runs[run][2] + WIDE_SPARE:
            failing.setdefault((s, run), []).append(k)
    # Stretches split where their floors' level changes reach a station into each other (between them a station
    # no box reached kept a shell over its walk, its underside cut off by the box beside it).
    segments = []
    for (s, run), ks in sorted(failing.items()):
        ks.sort()
        group, pad0 = [ks[0]], 0.0
        for k in ks[1:]:
            tops = [table[(s, j)][2] for j in group + [k]]
            spread = lambda i: np.ptp([edges[(s, j)][i] for j in group + [k]])
            near = (k - group[-1]) * step <= SEG_JOIN
            if near and max(tops) - min(tops) <= TOP_JOIN and max(spread(0), spread(1)) <= EDGE_JOIN:
                group.append(k)
            else:
                segments.append((s, run, group, None, (pad0, step if near else 0.0)))
                group, pad0 = [k], step if near else 0.0
        segments.append((s, run, group, None, (pad0, 0.0)))
    segments = [(s, run, ks, stub, (0.0, 0.0)) for s, run, ks, stub in stubs] + segments   # the stubs first
    faces, known = [], set()
    lo3, hi3 = np.zeros((0, 3)), np.zeros((0, 3))

    def listed():
        # The faces near a corridor are picked from these boxes: the ones each carve makes are listed too (a corridor
        # beside one already cut missed the pieces its cuts left and left them standing in the walk).
        nonlocal lo3, hi3
        new = [f for f in bm.faces if f not in known]
        pts = [[U(v.co) for v in f.verts] for f in new]
        faces.extend(new)
        known.update(new)
        lo3 = np.vstack([lo3, np.array([[min(p[i] for p in ps) for i in range(3)] for ps in pts]).reshape(-1, 3)])
        hi3 = np.vstack([hi3, np.array([[max(p[i] for p in ps) for i in range(3)] for ps in pts]).reshape(-1, 3)])
    cut = caps = 0
    for s, run, ks, stub, (more0, more1) in segments:
        rows = [(*edges[(s, k)], table[(s, k)][2]) for k in ks] if stub is None else [stub]
        lo, hi = max(r[0] for r in rows), min(r[1] for r in rows)
        out = hi - ENV_OUT
        keep = min(max(out - lo - runs[run][2] - WIDE_SPARE, ENV_IN), WALL_KEEP)
        keep = max(keep, out - lo - (runs[run][2] + fl.PATH_IN + WIDE_SPARE))     # no deeper than the check looks
        pad = SEG_PAD if stub is None else 0.0
        x0, x1 = ks[0] * step - step / 2.0 - pad - more0, ks[-1] * step + step / 2.0 + pad + more1
        y0, y1 = (lo + keep, out) if s > 0 else (-out, -(lo + keep))
        z0 = min(r[2] for r in rows) + CORRIDOR[0] if stub is None else stub[2] + STUB_CUT   # over its lowest floor (TOP_JOIN)
        z1 = min(r[2] for r in rows) + head + CORRIDOR[1]
        box = ((x0, x1), (y0, y1), (z0, z1))
        listed()
        near = np.all((hi3 > np.array([b[0] for b in box]) - 1.0) & (lo3 < np.array([b[1] for b in box]) + 1.0), axis=1)
        n, c = _carve(bm, tag, nearest, fill, [faces[i] for i in np.nonzero(near)[0]], box)
        cut, caps = cut + n, caps + c
        rw.log(f'walk corridor {"+Y" if s > 0 else "-Y"} {run}{" (no floor)" if stub else ""}: x {x0:.0f}..{x1:.0f}, |y| {lo + keep:.0f}..{out:.0f}, '
               f'z {z0:.0f}..{z1:.0f}: {n} faces cut, {c} cap faces')
    return len(segments), cut, caps


def _carve(bm, tag, nearest, fill, faces, box):
    """Cuts the box out of faces (those near it): split on its six planes, what lies inside deleted, the cut on each
    plane capped facing into the box. Returns (faces cut, cap faces)."""
    faces = [f for f in faces if f.is_valid]
    for axis in range(3):
        for value in box[axis]:
            cut = spanning(bm, tag, None, axis, value, faces=faces)
            if cut:
                gone = set(cut)
                faces = [f for f in faces if f not in gone and f.is_valid] + bisect(bm, cut, axis, value)
                faces = list(dict.fromkeys(f for f in faces if f.is_valid))
    inside = [f for f in faces if all(box[i][0] < ue_centre(f)[i] < box[i][1] for i in range(3))]
    rim = {e for f in inside for e in f.edges}
    bmesh.ops.delete(bm, geom=inside, context='FACES')
    caps = 0
    for axis in range(3):
        for k, value in enumerate(box[axis]):
            on = lambda v, axis=axis, value=value: abs(U(v.co)[axis] - value) < rw.EPS * 100.0
            edges = [e for e in rim if e.is_valid and e.is_boundary and on(e.verts[0]) and on(e.verts[1])]
            facing = [0.0, 0.0, 0.0]
            facing[axis] = 1.0 if k == 0 else -1.0
            caps += _close(bm, tag, nearest, fill, edges, facing)
    caps += _seal(bm, tag, nearest, fill, rim)
    return len(inside), caps


def _seal(bm, tag, nearest, fill, edges):
    """Whatever the caps left open round a cut: each flat boundary loop through edges (within SEAL_FLAT of a plane)
    filled whole, in the paint beside it; a loop over two of the box's planes stays open (filled, it would span the
    corridor). Returns the faces made."""
    todo = {e for e in edges if e.is_valid and e.is_boundary}
    made = 0
    while todo:
        loop, ends = [], [todo.pop()]
        seen = set(ends)
        while ends:
            e = ends.pop()
            loop.append(e)
            for v in e.verts:
                for n in v.link_edges:
                    if n.is_boundary and n not in seen:
                        seen.add(n)
                        ends.append(n)
        todo -= seen
        pts = np.array([U(v.co) for e in loop for v in e.verts])
        if len(pts) < 3 or np.linalg.svd(pts - pts.mean(0), compute_uv=False)[-1] / np.sqrt(len(pts)) > SEAL_FLAT:
            continue
        new = bmesh.ops.holes_fill(bm, edges=loop, sides=0)['faces']
        new = bmesh.ops.triangulate(bm, faces=new)['faces'] if new else []
        for f in new:
            f[tag] = fill
            f.normal_update()
            f.smooth = False
            nearest.paint(f)
        if new:
            rw.paint_like(nearest.uv, new)
        made += len(new)
    return made


def _close(bm, tag, nearest, fill, edges, facing=None):
    """Each chain of edges closed on itself (its ends joined) and filled (facing that way, when given)."""
    links = {}
    for e in edges:
        for v in e.verts:
            links.setdefault(v, []).append(e)
    made = 0
    seen = set()
    for e0 in edges:
        if e0 in seen:
            continue
        chain, todo = [], [e0]
        seen.add(e0)
        while todo:
            e = todo.pop()
            chain.append(e)
            for v in e.verts:
                for n in links[v]:
                    if n not in seen:
                        seen.add(n)
                        todo.append(n)
        ends = [v for v in {v for e in chain for v in e.verts} if len(links[v]) == 1]
        if len(ends) == 2 and not bm.edges.get(ends):
            chain.append(bm.edges.new(ends))
        if len(chain) < 3:
            continue
        new = [g for g in bmesh.ops.triangle_fill(bm, use_beauty=True, use_dissolve=False, edges=chain)['geom']
               if isinstance(g, bmesh.types.BMFace)]
        for f in new:
            f[tag] = fill
            f.normal_update()
            if facing is not None and f.normal.dot(D(facing)) < 0.0:
                f.normal_flip()
            f.smooth = False
            nearest.paint(f)
            made += 1
        rw.paint_like(nearest.uv, new)
    return made


def _cap(bm, tag, nearest, fill, edges):
    """The closed holes along edges filled, painted like the faces beside them; returns the faces made."""
    if not edges:
        return 0
    made = bmesh.ops.holes_fill(bm, edges=list(set(edges)), sides=0)['faces']
    made = bmesh.ops.triangulate(bm, faces=made)['faces'] if made else []
    for f in made:
        f[tag] = fill
        f.normal_update()
        f.smooth = False
        nearest.paint(f)
    rw.paint_like(nearest.uv, made)
    return len(made)



def trim(bm, tag, nearest, fill, faces, trim):
    """Drops a copy's faces over trim = (z, |y|) (a rail post), the cut closed flat."""
    z0, y0 = trim
    # Cut level first, so the hole it leaves is flat and its cap lies on the rail's top (no spikes).
    faces = faces + bisect(bm, [f for f in faces if f.is_valid and min(abs(v.co.x) for v in f.verts) * 100.0 > y0
                                and min(v.co.z for v in f.verts) * 100.0 < z0 < max(v.co.z for v in f.verts) * 100.0], 2, z0)
    faces = list(dict.fromkeys(f for f in faces if f.is_valid))
    gone = [f for f in faces if ue_centre(f)[2] > z0 and abs(ue_centre(f)[1]) > y0]
    rim = {e for f in gone for e in f.edges}
    bmesh.ops.delete(bm, geom=gone, context='FACES')
    _cap(bm, tag, nearest, fill, [e for e in rim if e.is_valid and e.is_boundary])
    return [f for f in faces if f.is_valid]
