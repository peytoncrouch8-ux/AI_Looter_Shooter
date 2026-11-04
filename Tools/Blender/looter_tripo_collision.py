"""Collision that follows the Tripo looks (the art audit's finding 6, plan rows 6-7: Docs/Work/2026-11-03-ArtAudit/AUDIT.md;
measured before and after in Docs/Work/2026-11-04-Art/COLLISION.md). The visible meshes are never touched: only the UCX_
hulls under a model change.

Two ways, by kind:
- Props and ships (looter_tripo_wardens SPECS `collision=`): the mesh's vertices split into convex pieces, each split the
  cut plane (along the piece's own axes or the world's) that leaves the two smallest hulls, the worst piece first, until
  every hull's surface lies close to the look or the model's hull budget is spent. A Tripo prop is one closed shell (no
  islands to go by), so its parts (a table's top and legs, a winch's drum and posts) are found by shape. Each hull is cut
  down to at most HULL_VERTS corners along fixed directions in its own frame (a tilted slab stays a slab).
- Buildings (looter_tripo_buildings.swap, after the trim boards are placed): every scripted hull's box is a region of the
  Tripo shell, read by rays from outside as the player and bullets meet it; each region's walls are planes fitted to the
  faces the rays hit (each row's outermost wall-wide cluster, so trim, sills and posts don't push a wall out: the old
  refit took the shell's every vertex and a wall stood 25-50 cm proud of the look); where a wall steps out or bends (an
  overhanging loft, a gambrel's two pitches, an eave) the region is cut into bands, each its own hull; a deck's or porch
  roof's top and underside are planes fitted to the readings from above and below. Then cover_leftovers: whatever of the
  shell and its trim still stands 10 cm or more outside every hull (rakes, fascias, a stovepipe, foundation stones) gets
  small hulls of its own. Trim under 10 cm deep stays outside: shots meet the wall behind it.

    import looter_tripo_collision as tc
    tc.fit_prop(obj, report, max_hulls=8)                      # a Wardens prop (SPECS 'collision': tc.barricade_table, ...)
    tc.refit_building(obj, [shell, trim], name, keep, drop)    # looter_tripo_buildings.refit_hulls
    tc.cover_leftovers(obj, [shell, trim])
"""
import heapq
import math

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

# A hull face stands this far proud of the surface it was fitted to (the audit's rule: at most 5 cm).
MARGIN = 0.02
# Unreal keeps hulls cheap with few corners; every hull here is cut down to at most this many.
HULL_VERTS = 48
# A hull is good when this share of its outer surface at most lies more than FAR from the look.
FAR = 0.10
GOOD = 0.12


def log(message):
    print(f'LOOTER: collision: {message}', flush=True)


# --- Hull objects ---

def hulls_of(obj):
    return [c for c in obj.children if c.name.startswith('UCX_')]


def clear_hulls(obj):
    for hull in hulls_of(obj):
        mesh = hull.data
        bpy.data.objects.remove(hull)
        if not mesh.users:
            bpy.data.meshes.remove(mesh)


def new_hull(obj, points):
    """A UCX_ hull under obj round points (model space, metres): their convex hull, its corners only."""
    mesh = bpy.data.meshes.new('UCX_' + obj.name)
    hull = bpy.data.objects.new('UCX_' + obj.name, mesh)
    bpy.context.scene.collection.objects.link(hull)
    hull.parent = obj
    hull.display_type = 'WIRE'
    inverse = hull.matrix_world.inverted()
    bm = bmesh.new()
    verts = [bm.verts.new(inverse @ Vector(p)) for p in points]
    result = bmesh.ops.convex_hull(bm, input=verts)
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    bm.to_mesh(mesh)
    bm.free()
    return hull


# --- Convex pieces ---

def _frame(P):
    """The principal axes of points P (N, 3): rows of a rotation, the longest first."""
    if len(P) < 4:
        return np.eye(3)
    c = P - P.mean(axis=0)
    _, _, vt = np.linalg.svd(c, full_matrices=False)
    if np.linalg.det(vt) < 0:
        vt[2] = -vt[2]
    return vt


def _directions(count):
    """The 26 directions of a cube's faces, edges and corners, then a Fibonacci sphere's to make up count."""
    out = [np.array(d, float) / np.linalg.norm(d) for d in
           [(x, y, z) for x in (-1, 0, 1) for y in (-1, 0, 1) for z in (-1, 0, 1) if (x, y, z) != (0, 0, 0)]]
    k = max(0, count - len(out))
    for i in range(k):
        z = 1.0 - 2.0 * (i + 0.5) / k
        r = math.sqrt(max(0.0, 1.0 - z * z))
        a = i * math.pi * (3.0 - math.sqrt(5.0))
        out.append(np.array((r * math.cos(a), r * math.sin(a), z)))
    return np.array(out)


_DIRS = _directions(64)


def _planes(T):
    """(normals (K, 3), offsets (K,)) of a convex hull's triangles T, pointing out."""
    n = np.cross(T[:, 1] - T[:, 0], T[:, 2] - T[:, 0])
    length = np.linalg.norm(n, axis=1)
    keep = length > 1e-12
    n = n[keep] / length[keep, None]
    centre = T.reshape(-1, 3).mean(axis=0)
    d = np.einsum('ij,ij->i', n, T[keep, 0])
    flip = (n @ centre - d) > 0.0
    n[flip], d[flip] = -n[flip], -d[flip]
    return n, d


def corners(P, limit=HULL_VERTS, tol=0.015):
    """At most limit points of P whose convex hull is P's, nearly: the exact hull's corners when they are few enough,
    else its extreme points along the world's 26 directions and the piece's own (a tilted board keeps its faces, a keel
    its edge), then the corner standing farthest outside the hull so far, one at a time, until none is more than tol out."""
    P = np.asarray(P, float)
    if len(P) <= limit:
        return P
    hull = _hull(P)
    if hull is None:
        return P[:limit]
    H = hull[0]
    if len(H) <= limit:
        return H
    R = _frame(H)
    world = list(np.unique(np.argmax(H @ _DIRS[:26].T, axis=0)))
    own = [k for k in np.unique(np.argmax(H @ (_DIRS[:26] @ R).T, axis=0)) if k not in world]
    chosen = (world + own)[:limit]
    while len(chosen) < limit:
        sub = _hull(H[chosen])
        if sub is None:
            break
        n, d = _planes(sub[2])
        out = (H @ n.T - d).max(axis=1)
        k = int(np.argmax(out))
        if out[k] <= tol or k in chosen:
            break
        chosen.append(k)
    return H[chosen]


def _hull(P):
    """(corners (M, 3), volume, triangles (K, 3, 3)) of points P's convex hull, or None when flat or too few."""
    if len(P) < 4:
        return None
    bm = bmesh.new()
    for p in P:
        bm.verts.new(p)
    try:
        result = bmesh.ops.convex_hull(bm, input=bm.verts)
    except Exception:
        bm.free()
        return None
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    bmesh.ops.triangulate(bm, faces=bm.faces)
    if not bm.faces:
        bm.free()
        return None
    vol = bm.calc_volume(signed=False)
    V = np.array([tuple(v.co) for v in bm.verts])
    T = np.array([[tuple(v.co) for v in f.verts] for f in bm.faces])
    bm.free()
    return V, vol, T


def _surface_samples(T, count, rng):
    a = T[:, 1] - T[:, 0]
    b = T[:, 2] - T[:, 0]
    area = 0.5 * np.linalg.norm(np.cross(a, b), axis=1)
    if area.sum() <= 0:
        return np.zeros((0, 3)), 0.0
    pick = rng.choice(len(T), size=count, p=area / area.sum())
    u = rng.random(count)
    v = rng.random(count)
    flip = u + v > 1.0
    u[flip], v[flip] = 1.0 - u[flip], 1.0 - v[flip]
    pts = T[pick, 0] + a[pick] * u[:, None] + b[pick] * v[:, None]
    return pts, float(area.sum())


def _far_share(T, tree, rng, count=240, far=FAR):
    """Of a hull's surface (triangles T), the share more than far from the look, and that area (m2)."""
    pts, area = _surface_samples(T, count, rng)
    if not len(pts):
        return 0.0, 0.0
    bad = 0
    for p in pts:
        hit = tree.find_nearest(Vector(p))
        if hit[0] is None or (hit[0] - Vector(p)).length > far:
            bad += 1
    share = bad / len(pts)
    return share, share * area


def _best_split(P, rng, sample=1400):
    """The cut of points P (along P's principal axes or the world's, at a decile) leaving the least hull volume:
    (each point's signed distance from the cut, volume after, volume before) or None."""
    S = P if len(P) <= sample else P[rng.choice(len(P), size=sample, replace=False)]
    whole = _hull(S)
    if whole is None:
        return None
    R = _frame(S)
    axes = [R[0], R[1], R[2], np.array((1.0, 0, 0)), np.array((0, 1.0, 0)), np.array((0, 0, 1.0))]
    best = None
    for axis in axes:
        proj = S @ axis
        for q in np.linspace(0.1, 0.9, 9):
            t = np.quantile(proj, q)
            a, b = S[proj <= t], S[proj > t]
            if len(a) < 12 or len(b) < 12:
                continue
            ha, hb = _hull(a), _hull(b)
            vol = (ha[1] if ha else 0.0) + (hb[1] if hb else 0.0)
            if best is None or vol < best[0]:
                best = (vol, axis, t)
    if best is None:
        return None
    vol, axis, t = best
    return P @ axis - t, vol, whole[1]


def decompose(P, tree, max_hulls=8, good=GOOD, far=FAR, min_points=24, seed=5, min_gain=0.08, seam=0.03):
    """Convex pieces (lists of points) covering points P: the piece with the most hull area far from the look is cut in
    two (_best_split) until every piece is good (share far <= good), the budget max_hulls is spent, or a cut no longer
    saves min_gain of its volume. The two halves of a cut share the points within seam of it, so they overlap there (no
    crack between them, and the face where they meet lies inside the other)."""
    rng = np.random.default_rng(seed)
    P = np.asarray(P, float)
    heap, done, counter = [], [], 0

    def push(points):
        nonlocal counter
        h = _hull(points)
        if h is None:
            return
        share, bad_area = _far_share(h[2], tree, rng, far=far)
        counter += 1
        heapq.heappush(heap, (-bad_area, counter, points, share))

    push(P)
    while heap:
        neg, _, points, share = heapq.heappop(heap)
        if share <= good or len(points) < 2 * min_points or len(done) + len(heap) + 2 > max_hulls:
            done.append(points)
            continue
        split = _best_split(points, rng)
        if split is None or split[1] > split[2] * (1.0 - min_gain):
            done.append(points)
            continue
        side = split[0]
        push(points[side <= seam])
        push(points[side > -seam])
    return done


def verts_of(obj):
    me = obj.data
    V = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', V)
    return V.reshape(-1, 3)


def look_tree(obj):
    me = obj.data
    return BVHTree.FromPolygons([v.co for v in me.vertices], [list(p.vertices) for p in me.polygons], epsilon=0.0)


def fit_prop(obj, report=None, max_hulls=8, good=GOOD, regions=None, keep_below=None, label=None, limit=HULL_VERTS,
             overlap=0.05, exclude=()):
    """obj's hulls replaced by convex pieces of its own mesh (decompose), each at most limit corners. regions: optional
    (lo, hi, budget) boxes in the model's Blender frame, each decomposed on its own (budget hulls at most) and grown by
    overlap so the faces where two meet lie inside each other; vertices outside every region get no collision. keep_below:
    only vertices under this height; exclude: boxes whose vertices get no collision (a flying ship's wheels). Returns the hull
    objects."""
    clear_hulls(obj)
    V = verts_of(obj)
    if keep_below is not None:
        V = V[V[:, 2] <= keep_below]
    for lo, hi in exclude:
        V = V[~np.all((V >= np.array(lo)) & (V <= np.array(hi)), axis=1)]
    tree = look_tree(obj)
    groups = []
    if regions:
        for lo, hi, budget in regions:
            sel = np.all((V >= np.array(lo) - overlap) & (V <= np.array(hi) + overlap), axis=1)
            if sel.sum() >= 8:
                groups += decompose(V[sel], tree, max_hulls=budget, good=good)
    else:
        groups = decompose(V, tree, max_hulls=max_hulls, good=good)
    hulls = [new_hull(obj, corners(g, limit)) for g in groups if len(g) >= 4]
    name = label or obj.name
    log(f'{name}: {len(hulls)} hulls fitted to its parts ({len(V)} vertices)')
    if report is not None:
        report['hulls'] = len(hulls)
    return hulls


def top_at(obj, x, y, above=50.0):
    """The highest hull top under (x, y) (model space, metres), or None: what a body standing there rests on."""
    best = None
    for hull in hulls_of(obj):
        me = hull.data
        tree = BVHTree.FromPolygons([hull.matrix_world @ v.co for v in me.vertices], [list(p.vertices) for p in me.polygons])
        hit = tree.ray_cast(Vector((x, y, above)), Vector((0.0, 0.0, -1.0)), above + 10.0)
        if hit[0] is not None and (best is None or hit[0].z > best):
            best = hit[0].z
    return best


# --- The Wardens props and ships: looter_tripo_wardens SPECS 'collision' (called with the built model and its report) ---
# Budgets: the most hulls a model may get. A prop is cover and a thing in the way: its top, legs, drum, post and arm each
# their own hull, the air between them open to shots.

def barricade_table(obj, report):
    """Delia's overturned kitchen table (build_area_farm.py, the porch barricade): its top, the apron and each leg."""
    fit_prop(obj, report, max_hulls=8)


def barricade_boards(obj, report):
    fit_prop(obj, report, max_hulls=6)


def barricade_sacks(obj, report):
    fit_prop(obj, report, max_hulls=6)


def hose_intake(obj, report):
    """The iron intake mouth on the Sink's floor: the bell and the neck the hose comes into (its open mouth rests on the
    floor, so the hull's underside spans it)."""
    fit_prop(obj, report, max_hulls=4, limit=64)


def anchor_winch(obj, report):
    """The anchor winch: its drum, its two posts and the crank."""
    fit_prop(obj, report, max_hulls=6)


def lantern_post(obj, report):
    """The roost's lantern post (RoostKit::PostPath): the crossed foot, the post, the arm and its brace, the lantern."""
    fit_prop(obj, report, max_hulls=12)


# The company tender (Blender frame: its bow -Y, UE x = -y): the iron hull under its top walk (to 3.08 m: the hull's crown
# is 3.02-3.05 fore of the bag), the bag with its nozzle and the bow lamp above it, the tail fins, the wheels. Each part is
# split on its own, so no hull spans from the bag down onto the walk the riflemen stand on.
TENDER_REGIONS = (
    # the hull body and its top walk, in slices along its length (each part of a cigar is nearly its own hull)
    [((-2.4, y0, -0.1), (2.4, y1, 3.08), 1) for y0, y1 in zip((-6.2, -4.8, -3.6, -2.4, -1.2, 0.0, 1.2, 2.4, 3.6),
                                                               (-4.8, -3.6, -2.4, -1.2, 0.0, 1.2, 2.4, 3.6, 4.6))]
    + [((-2.4, -2.6, 3.08), (2.4, 3.4, 5.0), 6),     # the bag, its nozzle and hose
       ((-0.6, -4.4, 3.06), (0.6, -3.3, 4.0), 1),    # the bow lamp on its post
       ((-2.4, 4.6, 0.0), (2.4, 6.2, 5.0), 3)])     # the tail and its fins
# Its wheels (under the hull either side of the keel): no collision, so the keel's slices stay narrow (it flies).
TENDER_WHEELS = [((-2.4, -6.2, -0.1), (-0.45, 6.2, 0.45)), ((0.45, -6.2, -0.1), (2.4, 6.2, 0.45))]
# Where its riflemen stand (UE cm, tender frame; CompanyTender.cpp RiflemanDeck): logged with the hull top under each.
TENDER_SPOTS = [(300.0, 0.0), (-360.0, 0.0)]


def company_tender(obj, report):
    fit_prop(obj, report, regions=TENDER_REGIONS, exclude=TENDER_WHEELS)
    tops = []
    for ux, uy in TENDER_SPOTS:
        z = top_at(obj, -uy / 100.0, -ux / 100.0)
        tops.append(None if z is None else round(z * 100.0, 1))
    report['rifleman tops cm'] = tops
    log(f'{obj.name}: the hull tops under its riflemen (UE {TENDER_SPOTS}): {tops} cm')


# --- Buildings: walls fitted to the faces the rays meet ---

def _first_hit(tree, origin, direction, lo, hi, reach, facing=0.2, exclude=()):
    """The first point where a ray meets the surface inside the box lo..hi (passing what lies in the exclude boxes: other
    parts with hulls of their own), if the face there looks back along the ray (a face looking away means the ray started
    inside the shell: no reading). None when it meets nothing in the box."""
    o = Vector(origin)
    d = Vector(direction)
    left = reach
    for _ in range(32):
        hit = tree.ray_cast(o, d, left)
        if hit[0] is None:
            return None
        p, n = hit[0], hit[1]
        if all(lo[k] - 1e-4 <= p[k] <= hi[k] + 1e-4 for k in range(3)) and not any(
                all(b0[k] <= p[k] <= b1[k] for k in range(3)) for b0, b1 in exclude):
            return p if n.dot(d) < -facing else None
        left -= hit[3] + 1e-4
        o = p + d * 1e-4
        if left <= 0.0:
            return None
    return None


def _steps(a, b, count_max, step_min):
    """Sample positions across a..b: at most count_max, no closer than step_min, half a step in from each end."""
    n = max(1, min(count_max, int((b - a) / step_min)))
    step = (b - a) / n
    return [a + step * (i + 0.5) for i in range(n)]


def _side_profile(tree, lo, hi, axis, sign, exclude=()):
    """Rays inward across one side of the box (axis 0 or 1, outward sign): per height row, the outwardness (sign x the
    coordinate) of every reading. Returns [(z, [t, ...])] and the column count."""
    u_axis = 1 - axis
    start = (hi[axis] if sign > 0 else lo[axis]) + sign * 0.3
    d = [0.0, 0.0, 0.0]
    d[axis] = -sign
    reach = hi[axis] - lo[axis] + 0.6
    us = _steps(lo[u_axis], hi[u_axis], 140, 0.05)
    mid = (lo[axis] + hi[axis]) * 0.5
    rows = []
    for z in _steps(lo[2], hi[2], 120, 0.05):
        ts = []
        for u in us:
            o = [0.0, 0.0, z]
            o[axis] = start
            o[u_axis] = u
            p = _first_hit(tree, o, d, lo, hi, reach, exclude=exclude)
            # A reading past the box's middle is the far side seen from inside (through an open edge: a porch deck whose
            # skirt is trim), not this side's wall.
            if p is not None and sign * (p[axis] - mid) > -0.05:
                ts.append(sign * p[axis])
        rows.append((z, ts))
    return rows, len(us)


def _cap_hits(tree, lo, hi, sign, exclude=()):
    """Rays down (sign +1: the top) or up (-1: the bottom) over the box: the readings (x, y, z)."""
    start = (hi[2] if sign > 0 else lo[2]) + sign * 0.3
    reach = hi[2] - lo[2] + 0.6
    out = []
    for x in _steps(lo[0], hi[0], 120, 0.05):
        for y in _steps(lo[1], hi[1], 120, 0.05):
            p = _first_hit(tree, (x, y, start), (0.0, 0.0, -sign), lo, hi, reach, exclude=exclude)
            if p is not None:
                out.append((p[0], p[1], p[2]))
    return np.array(out).reshape(-1, 3)


def _line(zs, ws, tol=0.04, passes=3):
    """A robust straight line w = a + b z through (zs, ws): least squares, refitted without points more than tol off.
    Returns (a, b, share of the points within tol)."""
    zs, ws = np.asarray(zs, float), np.asarray(ws, float)
    use = np.ones(len(zs), bool)
    a, b = float(np.median(ws)), 0.0
    for _ in range(passes):
        if use.sum() >= 2 and np.ptp(zs[use]) > 1e-6:
            b, a = np.polyfit(zs[use], ws[use], 1)
        elif use.any():
            a, b = float(np.median(ws[use])), 0.0
        use = np.abs(ws - (a + b * zs)) <= tol
    return float(a), float(b), float(use.mean()) if len(use) else 0.0


def _profile_rows(rows, columns, share=0.35, width=0.06):
    """Each row's wall: the outermost cluster of readings (within width of each other) that holds share of the row's
    columns, its median (trim and sills are too few to count, a dormer or a chimney in front of a roof too narrow; a row
    without such a cluster is left out). Returns [(z, w)]."""
    need = max(3, int(columns * share))
    out = []
    for z, ts in rows:
        if len(ts) < need:
            continue
        t = np.sort(np.asarray(ts))[::-1]
        for i in range(len(t)):
            near = t[(t <= t[i]) & (t >= t[i] - width)]
            if len(near) >= need:
                out.append((z, float(np.median(near))))
                break
    return out


def _bands(profiles, z0, z1, tol=0.07, min_height=0.15):
    """Cuts z0..z1 into bands within each of which every side's profile is one straight line (within tol): where a wall
    steps out (an overhang, an eave) or bends (a roof's pitch), a new band. Returns [(za, zb)]."""
    levels = sorted({z for rows in profiles for z, _ in rows})
    if not levels:
        return [(z0, z1)]

    def fits(za, zb):
        # Most rows on the line, and the newest three too (a bend at the band's top end is a new band, not a tail).
        for rows in profiles:
            pts = [(z, w) for z, w in rows if za - 1e-6 <= z <= zb + 1e-6]
            if len(pts) < 3:
                continue
            zs, ws = zip(*pts)
            a, b, share = _line(zs, ws, tol)
            if share < 0.8 or any(abs(w - (a + b * z)) > tol for z, w in pts[-3:]):
                return False
        return True

    cuts = [z0]
    start = levels[0]
    for i, z in enumerate(levels):
        if z - start < min_height:
            continue
        if not fits(start, z):
            # The band ends halfway between the last level that still fitted and this one.
            cuts.append((levels[i - 1] + z) * 0.5)
            start = z
    cuts.append(z1)
    bands = [(a, b) for a, b in zip(cuts, cuts[1:]) if b - a > 1e-3]
    # A band too thin to stand on its own goes into the one below (the first into the one above).
    merged = []
    for a, b in bands:
        if merged and b - a < min_height:
            merged[-1] = (merged[-1][0], b)
        else:
            merged.append((a, b))
    if len(merged) > 1 and merged[0][1] - merged[0][0] < min_height:
        merged[1] = (merged[0][0], merged[1][1])
        merged.pop(0)
    return merged


def _cap_plane(hits, sign, tol=0.04):
    """The top (sign +1) or bottom (-1) of a region from its cap readings: a plane z = a + b x + c y when most readings
    lie on one (a deck, a porch roof), else level at the outermost readings (a ridge). Returns (normal, offset) of the
    halfspace n . p <= d, or None without readings."""
    if len(hits) < 12:
        return None
    X, Y, Z = hits[:, 0], hits[:, 1], hits[:, 2]
    A = np.stack([np.ones(len(Z)), X, Y], axis=1)
    # The plane most readings lie on (a stoop's boards, not the door sill and post feet on them): the best of a few
    # hundred through three readings each, then least squares on its readings.
    rng = np.random.default_rng(3)
    best, coef = -1, np.array([np.median(Z), 0.0, 0.0])
    for _ in range(300):
        k = rng.choice(len(Z), 3, replace=False)
        try:
            c = np.linalg.solve(A[k], Z[k])
        except np.linalg.LinAlgError:
            continue
        if abs(c[1]) > 1.5 or abs(c[2]) > 1.5:
            continue
        count = int((np.abs(Z - A @ c) <= tol * 0.75).sum())
        if count > best:
            best, coef = count, c
    use = np.abs(Z - A @ coef) <= tol
    for _ in range(3):
        if use.sum() >= 3:
            coef = np.linalg.lstsq(A[use], Z[use], rcond=None)[0]
        use = np.abs(Z - A @ coef) <= tol
    # A top with readings standing well over it (a roof's other slope) is no top: level over them all instead.
    beyond = (sign * (Z - A @ coef) > tol * 2).mean()
    if use.mean() >= 0.5 and beyond <= 0.12 and abs(coef[1]) < 1.5 and abs(coef[2]) < 1.5:
        # sign (z - b x - c y) <= sign (a + sign MARGIN): what lies over the top plane (under the bottom one) is cut off.
        n = Vector((-coef[1], -coef[2], 1.0)) * sign
        length = n.length
        return n / length, sign * (coef[0] + sign * MARGIN) / length
    z = np.percentile(Z, 98) if sign > 0 else np.percentile(Z, 2)
    return Vector((0.0, 0.0, sign)), sign * (z + sign * MARGIN)


def polytope(planes):
    """The corners of the convex polytope {p : n . p <= d for every (n, d) in planes} (empty when they don't close)."""
    N = np.array([tuple(n) for n, _ in planes], float)
    D = np.array([d for _, d in planes], float)
    pts = []
    k = len(planes)
    for i in range(k):
        for j in range(i + 1, k):
            for m in range(j + 1, k):
                A = N[[i, j, m]]
                if abs(np.linalg.det(A)) < 1e-9:
                    continue
                p = np.linalg.solve(A, D[[i, j, m]])
                if np.all(N @ p <= D + 1e-5):
                    pts.append(p)
    if not pts:
        return np.zeros((0, 3))
    return np.unique(np.round(np.array(pts), 5), axis=0)


def fit_region(tree, lo, hi, verts=None, tol=0.04, exclude=(), covered=(None, None)):
    """Hulls for one region (box lo..hi) of a building's shell: its walls read by rays (each row's outermost wall-wide
    cluster, _profile_rows, so trim and sills don't count), cut into bands where a wall steps or bends, each band a hull of six planes: its four sides fitted
    to the walls MARGIN proud, its top and bottom the band's, or for the region's own top and bottom fitted to the
    readings from above and below (a deck, a porch roof, a ridge). verts: the shell's vertices in the box, for a side the
    rays can't read; exclude: boxes of the parts inside this one with hulls of their own (a dormer on a roof, a porch
    roof against a wall), which the rays pass; covered: (top, bottom), the bottom of the region over this one's top (the
    top of the one under its bottom: a wall under its roof), so that end reaches into it whatever it read (no crack
    between the two). A region standing on the ground keeps the box's bottom (a Tripo house is open underneath: rays from
    below meet its floor). Returns a list of corner arrays."""
    lo, hi = Vector(lo), Vector(hi)
    sides = [(0, 1.0), (0, -1.0), (1, 1.0), (1, -1.0)]
    top = _cap_plane(_cap_hits(tree, lo, hi, 1.0, exclude), 1.0)
    # A region standing on another (a roof on its walls) keeps the box's bottom, deep in the one under it: what it reads
    # from below is that one's inside.
    bottom = None if lo.z < 0.15 or covered[1] is not None else _cap_plane(_cap_hits(tree, lo, hi, -1.0, exclude), -1.0)
    corners4 = [Vector((x, y, 0.0)) for x in (lo.x, hi.x) for y in (lo.y, hi.y)]

    def heights(plane):
        n, d = plane
        return [(d - n.x * c.x - n.y * c.y) / n.z for c in corners4] if abs(n.z) > 1e-6 else [0.0]

    if covered[0] is not None and (top is None or min(heights(top)) < covered[0] + 0.05):
        top = (Vector((0.0, 0.0, 1.0)), min(hi.z, covered[0] + 0.05))
    # The region's own top and bottom as read (a deck's boards, a porch roof's underside): rows over the top or under the
    # bottom read what stands behind it (a porch's rows over its boards meet the house's wall), so they are left out.
    footprint = [Vector((x, y, 0.0)) for x in (lo.x, hi.x) for y in (lo.y, hi.y)]
    z_top, z_bottom = hi.z, lo.z
    if top is not None:
        n, d = top
        z_top = max((d - n.x * c.x - n.y * c.y) / n.z for c in footprint) if abs(n.z) > 1e-6 else hi.z
    if bottom is not None:
        n, d = bottom
        z_bottom = min((d - n.x * c.x - n.y * c.y) / n.z for c in footprint) if abs(n.z) > 1e-6 else lo.z
    profiles = {}
    for axis, sign in sides:
        rows, columns = _side_profile(tree, lo, hi, axis, sign, exclude)
        rows = [(z, ts) for z, ts in rows if z_bottom - 0.01 <= z <= z_top + 0.01]
        profiles[(axis, sign)] = _profile_rows(rows, columns)
    bands = _bands(list(profiles.values()), lo.z, hi.z)
    # Each band's side planes from its rows; a side a band has no rows for (hidden behind another part: a wall's foot
    # behind a porch) takes the nearest band's plane that side, and only without one the shell's own extent there.
    lines = []
    for za, zb in bands:
        row = {}
        for axis, sign in sides:
            pts = [(z, w) for z, w in profiles[(axis, sign)] if za - 1e-6 <= z <= zb + 1e-6]
            if len(pts) >= 3:
                zs, ws = zip(*pts)
                a_, b_, _ = _line(zs, ws, tol)
                if len(pts) < 5 or abs(b_) > 6.0:
                    # Too few rows to tell a lean from noise (or a lean no wall or roof has): upright, round them all.
                    a_, b_ = max(ws), 0.0
                row[(axis, sign)] = (a_, b_)
        lines.append(row)
    pieces = []
    for index, (za, zb) in enumerate(bands):
        planes = []
        for axis, sign in sides:
            e = Vector((0.0, 0.0, 0.0))
            e[axis] = sign
            line = lines[index].get((axis, sign))
            if line is None:
                for step in range(1, len(bands)):
                    for near in (index + step, index - step):
                        if 0 <= near < len(bands) and (axis, sign) in lines[near]:
                            a_, b_ = lines[near][(axis, sign)]
                            # Its plane at the edge nearest this band, upright.
                            z = bands[near][0] if near > index else bands[near][1]
                            line = (a_ + b_ * z, 0.0)
                            break
                    if line is not None:
                        break
            if line is not None:
                a_, b_ = line
                # sign p[axis] <= a + b z + MARGIN, written (sign e - b z^) . p <= a + MARGIN
                n = e - Vector((0.0, 0.0, b_))
                planes.append((n / n.length, (a_ + MARGIN) / n.length))
            else:
                # Nothing to read from this side in any band: the shell's own extent there, else the box.
                band = None if verts is None else verts[(verts[:, 2] >= za) & (verts[:, 2] <= zb)]
                if band is not None and len(band):
                    planes.append((e, float(np.percentile(sign * band[:, axis], 99.5)) + MARGIN))
                else:
                    planes.append((e, sign * (hi[axis] if sign > 0 else lo[axis])))
        last, first = index == len(bands) - 1, index == 0
        planes.append(top if last and top else (Vector((0.0, 0.0, 1.0)), zb + (0.03 if not last else MARGIN)))
        planes.append(bottom if first and bottom else (Vector((0.0, 0.0, -1.0)), -(za - (0.03 if not first else MARGIN))))
        # Never past the region's own box (a leaning side fitted on a few rows would carry its band out of it).
        for k in range(3):
            e = Vector((0.0, 0.0, 0.0))
            e[k] = 1.0
            planes.append((e, hi[k] + MARGIN))
            planes.append((-e, -(lo[k] - MARGIN)))
        P = polytope(planes)
        if len(P) >= 4:
            pieces.append(P)
    return pieces


def _shell_tree(meshes):
    """A BVH of one mesh object or several (world space) and their vertices (N, 3)."""
    meshes = meshes if isinstance(meshes, (list, tuple)) else [meshes]
    verts, polys = [], []
    for m in meshes:
        base = len(verts)
        verts += [m.matrix_world @ v.co for v in m.data.vertices]
        polys += [[base + i for i in p.vertices] for p in m.data.polygons]
    return BVHTree.FromPolygons(verts, polys, epsilon=0.0), np.array([tuple(v) for v in verts]).reshape(-1, 3)


def _hull_bounds(hull):
    pts = [hull.matrix_world @ v.co for v in hull.data.vertices]
    return (Vector((min(p[k] for p in pts) for k in range(3))), Vector((max(p[k] for p in pts) for k in range(3))))


def _set_hull(hull, points):
    inverse = hull.matrix_world.inverted()
    bm = bmesh.new()
    verts = [bm.verts.new(inverse @ Vector(p)) for p in points]
    result = bmesh.ops.convex_hull(bm, input=verts)
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    bm.to_mesh(hull.data)
    bm.free()


# Per Tripo building (looter_tripo_buildings.swap's name): its regions read as parts (decompose: a lattice tower has no
# walls for rays to read), and extra regions the scripted model had no hull for: (box, 'parts' or 'walls', budget).
# Boxes in the model's frame (metres, front -Y).
BUILDINGS = {
    # The stovepipe standing out of the roof by the ridge (parts_buildings.FARM_PIPE), from just under the ridge up.
    'FarmhouseRansom': dict(extra=[(((-0.5, -1.1, 6.97), (0.55, -0.15, 9.0)), 'parts', 1),
                                   # The cross gable's back half and its back dormer (the scripted house had only the front
                                   # dormer: its hull's box takes the front half).
                                   (((-0.8, -0.9, 4.1), (0.8, 3.6, 7.0)), 'walls', 0)]),
    'WindmillRR': dict(parts=10),
}


def _volume(box):
    return float(np.prod([box[1][k] - box[0][k] for k in range(3)]))


def _shared(a, b):
    """The volume two boxes share."""
    return float(np.prod([max(0.0, min(a[1][k], b[1][k]) - max(a[0][k], b[0][k])) for k in range(3)]))


def _children(box, boxes, inside=0.6):
    """The smaller boxes mostly inside box (inside of their volume): parts in it with hulls of their own (a dormer on a
    roof, a stool on a porch), not a neighbour it only overlaps (the walls under a roof)."""
    return [b for b in boxes if b is not box and _volume(b) < _volume(box) and _shared(box, b) >= inside * _volume(b)]


def _covered(box, boxes, share=0.8):
    """Where most of box's top (bottom) lies under (over) other boxes reaching past it (a wall under its roof): the
    lowest bottom (highest top) of those, else None. Returns (top, bottom)."""
    lo, hi = box
    out = []
    for end in (1, -1):
        z = hi.z if end > 0 else lo.z
        hits = total = 0
        reach = []
        for x in np.linspace(lo.x, hi.x, 9):
            for y in np.linspace(lo.y, hi.y, 9):
                total += 1
                over = [b for b in boxes if b is not box and b[0].x <= x <= b[1].x and b[0].y <= y <= b[1].y and
                        (b[0].z <= z < b[1].z - 0.05 if end > 0 else b[0].z + 0.05 < z <= b[1].z)]
                if over:
                    hits += 1
                    reach += [b[0].z if end > 0 else b[1].z for b in over]
        out.append((min(reach) if end > 0 else max(reach)) if hits >= share * total else None)
    return tuple(out)


def _clusters(P, cell=0.1, least=24):
    """Points P (N, 3) grouped by touching (within a cell of each other, on a grid), the groups of at least least points."""
    keys = np.floor(P / cell).astype(np.int64)
    cells = {}
    for i, k in enumerate(map(tuple, keys)):
        cells.setdefault(k, []).append(i)
    seen, groups = set(), []
    for start in cells:
        if start in seen:
            continue
        stack, members = [start], []
        seen.add(start)
        while stack:
            c = stack.pop()
            members += cells[c]
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    for dz in (-1, 0, 1):
                        n = (c[0] + dx, c[1] + dy, c[2] + dz)
                        if n in cells and n not in seen:
                            seen.add(n)
                            stack.append(n)
        if len(members) >= least:
            groups.append(P[members])
    return groups


def _outside_all(P, hull_objs, tol):
    """The points of P (N, 3) more than tol outside every hull."""
    if not len(P):
        return P
    keep = np.ones(len(P), bool)
    for hull in hull_objs:
        pts = np.array([tuple(hull.matrix_world @ v.co) for v in hull.data.vertices])
        h = _hull(pts)
        if h is None:
            continue
        n, d = _planes(h[2])
        keep &= (P @ n.T - d).max(axis=1) > tol
    return P[keep]


def refit_building(obj, shell, name='', keep=(), drop=(), grow=(0.25, 0.25, 0.12)):
    """obj's scripted UCX hulls refitted to the Tripo shell (one mesh object, or a list: the shell and the trim boards
    joined to it): each hull's box, grown by grow (where the shell may stand), becomes fit_region's hulls, its rays
    passing the smaller regions mostly inside it (a dormer on the roof, a stool on the porch); a hull centred in one of
    keep stays (scripted parts kept in the model) and its box counts as such a
    part; one centred in drop goes (parts the shell has elsewhere), and so does one with nothing of the shell round it.
    The building's extra regions (BUILDINGS) are fitted the same way. Returns the number of hulls made."""
    config = BUILDINGS.get(name, {})
    tree, V = _shell_tree(shell)
    regions, kept = [], []
    for hull in hulls_of(obj):
        lo, hi = _hull_bounds(hull)
        mid = (lo + hi) * 0.5
        if any(all(b[0][k] <= mid[k] <= b[1][k] for k in range(3)) for b in keep):
            kept.append((lo, hi))
            continue
        if any(all(b[0][k] <= mid[k] <= b[1][k] for k in range(3)) for b in drop):
            bpy.data.objects.remove(hull)
            continue
        regions.append(dict(hull=hull, box=(Vector([lo[k] - grow[k] for k in range(3)]),
                                            Vector([hi[k] + grow[k] for k in range(3)])), mode='walls', budget=0))
    for (lo, hi), mode, budget in config.get('extra', ()):
        regions.append(dict(hull=None, box=(Vector(lo), Vector(hi)), mode=mode, budget=budget))
    boxes = [r['box'] for r in regions] + kept
    made = 0
    for region in regions:
        lo, hi = region['box']
        where = tuple(round(v, 2) for v in (lo + hi) * 0.5)
        sel = V[np.all((V >= np.array(lo)) & (V <= np.array(hi)), axis=1)]
        if len(sel) < 12:
            log(f'{obj.name}: region at {where}: only {len(sel)} shell vertices there, no hull')
            if region['hull'] is not None:
                bpy.data.objects.remove(region['hull'])
            continue
        if config.get('parts') or region['mode'] == 'parts':
            pieces = [corners(g) for g in decompose(sel, tree, max_hulls=region['budget'] or config.get('parts', 8))]
        else:
            pieces = fit_region(tree, lo, hi, sel, exclude=_children(region['box'], boxes),
                                covered=_covered(region['box'], [r['box'] for r in regions]))
        if not pieces:
            log(f'{obj.name}: region at {where}: no walls to fit, the corners of the shell there')
            pieces = [corners(sel)]
        if region['hull'] is not None:
            _set_hull(region['hull'], pieces[0])
            pieces = pieces[1:]
            made += 1
        for P in pieces:
            new_hull(obj, P)
        made += len(pieces)
        log(f'{obj.name}: region at {where} -> {made} hulls so far')
    return made


def cover_leftovers(obj, meshes, tol=0.10, least=0.3, density=400.0):
    """What of meshes (the shell, the trim joined to it) still stands more than tol outside every one of obj's hulls (a
    rake past the gable, a porch roof's front past the scripted box, the fascia boards, a stovepipe): each touching group
    of it at least least metres across gets hulls of its own (decompose, one for every 1.5 m it runs), so nothing seen is
    left to shoot through. Read on the meshes' vertices and on points strewn over their faces (density a square metre: a
    long board has few vertices). Returns the number of hulls added."""
    bm = bmesh.new()
    for m in meshes:
        tmp = m.data.copy()
        tmp.transform(m.matrix_world)
        bm.from_mesh(tmp)
        bpy.data.meshes.remove(tmp)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    tree = BVHTree.FromBMesh(bm)
    T = np.array([[tuple(v.co) for v in f.verts] for f in bm.faces]).reshape(-1, 3, 3)
    V = np.array([tuple(v.co) for v in bm.verts]).reshape(-1, 3)
    bm.free()
    if len(T):
        rng = np.random.default_rng(9)
        area = 0.5 * np.linalg.norm(np.cross(T[:, 1] - T[:, 0], T[:, 2] - T[:, 0]), axis=1).sum()
        S, _ = _surface_samples(T, int(min(80000, max(1000, area * density))), rng)
        V = np.vstack([V, S])
    rest = _outside_all(V, hulls_of(obj), tol)
    groups = [g for g in _clusters(rest) if np.ptp(g, axis=0).max() >= least]
    added = 0
    for g in groups:
        # A hull for every 1.5 m it runs (a rake along a gable, a porch roof's front), and no more than its points allow.
        budget = int(min(10, max(1, np.ceil(np.ptp(g, axis=0).max() / 1.5)), max(1, len(g) // 40)))
        for piece in decompose(g, tree, max_hulls=budget):
            if len(piece) >= 4:
                new_hull(obj, corners(piece))
                added += 1
    log(f'{obj.name}: {len(rest)} points outside the fitted hulls, in {len(groups)} groups: {added} hulls more')
    return added
