"""The dredge barge's side walks cleared for the walkable kit (looter_tripo_barge.rework, on Tripo's mesh before the
squeeze, in Tripo's x): where Tripo's pieces cross a walk at a pawn's height they are cut where they cross, each side's
cut closed in the paint beside it, so the squeeze then carries the wall's side in with the wall and leaves the
bulwark's side where it is. Cut: the bulwark's capping lip that roofs Tripo's slot trench at the casing's aft end and
dome end (squeezed, it was a shelf 26-50 cm over the walk), the casing's band straps (a bridge 117-137 cm up) and the
horn pipes off the house's sides (from the house wall over the rail, 38-119 cm up). Cut before the squeeze, the cut
spans only Tripo's narrow trench and each side's chain is near straight; cut after, the stretched faces zigzagged and
their caps stood out into the walk. Also here: the forecastle's stub posts taken off its floor (posts) and a copy's
rail post left out of it (trim, for the cross walk's copies).
"""
import bmesh

import looter_tripo_barge_floors as fl
import looter_tripo_rework as rw
from looter_tripo_rework import U, bisect, ue_centre

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


def _close(bm, tag, nearest, fill, edges):
    """Each chain of edges closed on itself (its ends joined) and filled."""
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
