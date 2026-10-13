"""The Providence's rock (Art/Models/Mine/Mine.py; Docs/Areas/RansomsRest_Expansion.md, section 6, "The Providence"):
the mine's rooms carved out of one signed distance field in the mine's own frame, meshed once and cut into pieces.

The frame ("mouth-local"): metres, x east, y north (into the ridge), z up; the origin is the adit's mouth on its floor,
at the portal (design (74, 230), +58 m). Every room, timber and socket below is in this frame.

How the rock is made, so every piece meets its neighbours without a crack or a seam of light:
  1. The void is a smooth union of rooms (Room: a rounded rectangle with a domed roof) and tunnels (Corridor: a
     straight run with a flat or sloping floor and an arched roof), each floor exactly flat (or an exact ramp): the
     weathering noise fades out over the first 0.8 m above the floor, so the fights' floors are level, one height each.
     Over it: bedded-rock noise (stretched level, so ledges run along the beds), soft partings every metre or so,
     the timber that must not be buried (each set's posts and lagging slab, each pillar and prop, the barricade) cut
     back out of the rock, the roof brought down onto every pillar's and prop's cap, and the wall smoothed flat where
     the chalk is.
  2. It is meshed once over the whole mine (looter_outcrops' surface nets on a 15 cm lattice, each vertex pulled onto
     the surface) and the quads are dealt out to the pieces by where they lie (REGION). A piece's border vertices are
     shared with its neighbour's, bit for bit.
  3. Each piece is reduced (Blender's quadric collapse) with its border vertices locked, so the border stays exactly
     where it was, then all are welded back into one mesh for the shading: materials (RockCliffDen on walls and roofs,
     DenFloor on floors), world-scale UVs in the mine's frame, smooth normals and baked occlusion computed across the
     seams, the dressing (office lining, stairs, chalk) joined in, and only then split again into the pieces, each
     keeping its normals as custom normals. So the shading carries straight across every seam.

The mesh faces into the void (the rock is never seen from outside). The shell (Mine.py's Mine_Shell) closes it from the
outside for shadows.
"""
import math
import random
import time

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.kdtree import KDTree

import looter_outcrops as lo
import looter_textures as lt

CELL = 0.15
LATTICE = np.array([0.075, 0.075, 0.075])     # lattice points at LATTICE + k * CELL: floors (0, -8) fall mid-cell
UNION_K = 0.6                                  # the fillet where rooms and tunnels meet
ROCK_DENSITY = 300                             # RockCliff px per metre on the mine's walls (close up underground)
FLOOR_DENSITY = 256


def log(message):
    print(f'MINE: {message}', flush=True)


# --- Primitives (vectorized over (N, 3) points; distances about metres, negative inside the void) ---

def smin(a, b, k):
    return lo.smin(a, b, k)


def smax(a, b, k):
    return lo.smax(a, b, k)


class Corridor:
    """A straight tunnel from a to b (plan points), W wide: its floor at knots [(s, z)] along it (np.interp: flat
    beyond the ends), its walls Hw tall, its roof arched to H at the middle. amp: its rock's roughness."""

    def __init__(self, name, a, b, knots, W, Hw, H, amp=0.18, cap=0.3):
        self.name = name
        self.a, self.b = np.array(a, float), np.array(b, float)
        d = self.b - self.a
        self.length = float(np.hypot(*d))
        self.u = d / self.length
        self.n = np.array([-self.u[1], self.u[0]])
        self.ks = np.array([k[0] for k in knots], float)
        self.kz = np.array([k[1] for k in knots], float)
        self.W, self.Hw, self.H, self.amp, self.cap = W, Hw, H, amp, cap
        lo_ = np.minimum(self.a, self.b) - W
        hi_ = np.maximum(self.a, self.b) + W
        self.box = (np.array([lo_[0], lo_[1], self.kz.min() - 1.0]), np.array([hi_[0], hi_[1], self.kz.max() + H + 1.0]))

    def dist(self, P):
        """(walls and roof, open 0.6 m down past the floor; the plan's distance; the floor's height) at P."""
        s = (P[:, 0] - self.a[0]) * self.u[0] + (P[:, 1] - self.a[1]) * self.u[1]
        t = (P[:, 0] - self.a[0]) * self.n[0] + (P[:, 1] - self.a[1]) * self.n[1]
        zf = np.interp(s, self.ks, self.kz)
        half = self.W * 0.5
        side = np.abs(t) - half
        q = np.clip(np.abs(t) / half, 0.0, 1.0)
        zc = zf + self.Hw + (self.H - self.Hw) * (1.0 - q * q)
        ends = np.maximum(-s, s - self.length)
        plan = smax(side, ends, self.cap)
        d = smax(plan, P[:, 2] - zc, 0.35)
        return np.maximum(d, zf - 0.6 - P[:, 2]), plan, zf


class Room:
    """A room: a rectangle x0..x1, y0..y1 with corners rounded rc, a level floor, walls Hw tall and the roof rising to
    H over `ramp` metres in from the walls."""

    def __init__(self, name, x0, x1, y0, y1, floor, Hw, H, ramp=3.0, rc=1.0, amp=0.25):
        self.name = name
        self.c = np.array([(x0 + x1) * 0.5, (y0 + y1) * 0.5])
        self.h = np.array([(x1 - x0) * 0.5, (y1 - y0) * 0.5])
        self.z = floor
        self.Hw, self.H, self.ramp, self.rc, self.amp = Hw, H, ramp, rc, amp
        self.box = (np.array([x0 - 1.5, y0 - 1.5, floor - 1.0]), np.array([x1 + 1.5, y1 + 1.5, floor + H + 1.0]))

    def dist(self, P):
        """(walls and roof, open 0.6 m down past the floor; the plan's distance; the floor's height) at P."""
        q = np.abs(P[:, :2] - self.c) - (self.h - self.rc)
        outside = np.linalg.norm(np.maximum(q, 0.0), axis=1)
        plan = outside + np.minimum(np.maximum(q[:, 0], q[:, 1]), 0.0) - self.rc
        inner = np.clip(-plan / self.ramp, 0.0, 1.0)
        zc = self.z + self.Hw + (self.H - self.Hw) * (1.0 - (1.0 - inner) ** 2)
        d = smax(plan, P[:, 2] - zc, 0.6)
        return np.maximum(d, self.z - 0.6 - P[:, 2]), plan, np.full(len(P), self.z)


def obox(P, center, axes, half):
    """Distance to an oriented box (center, 3x3 axes as columns, half sizes): exact outside, a bound inside."""
    L = (P - center) @ axes
    q = np.abs(L) - half
    return np.linalg.norm(np.maximum(q, 0.0), axis=1) + np.minimum(q.max(axis=1), 0.0)


def frame(yaw=0.0, pitch=0.0):
    """The 3x3 axes (columns) of a placement turned pitch degrees about its own X, then yaw about Z."""
    return np.array(Matrix.Rotation(math.radians(yaw), 3, 'Z') @ Matrix.Rotation(math.radians(pitch), 3, 'X'))


class Mine:
    """The field: rooms (Corridor/Room), clear boxes (oriented boxes the rock is cut back from: timber), caps (where the
    roof comes down onto a cap's top: (x, y, top, r0, r1)) and flats (a wall smoothed to a plane: (point, normal, half
    sizes across and up, feather)). field(P) is negative in the rock (looter_outcrops' sign)."""

    def __init__(self, rooms, clears=(), caps=(), flats=(), seed=5807):
        self.rooms = list(rooms)
        self.clears = list(clears)
        self.caps = list(caps)
        self.flats = list(flats)
        self.noise = [lo.Noise(seed * 13 + i) for i in range(6)]
        self.partings = [(-7.35, 0.07), (-6.15, 0.05), (-4.95, 0.08), (-3.7, 0.06), (-2.55, 0.07), (-1.3, 0.05),
                         (-0.35, 0.06), (0.95, 0.07), (2.05, 0.05), (3.25, 0.08), (4.45, 0.06), (5.6, 0.05)]

    def cave(self, P):
        """The void's distance (negative inside), its floor's height and its rock's roughness at P. The walls and roofs
        of the rooms are joined smoothly (fillets where they meet); the floor comes after, so it stays exactly level
        (or an exact ramp) where rooms overlap: inside a room's plan its floor counts, and the lowest one counts (a
        sump in a landing); outside every plan the floors rise steeply."""
        ds, plans, fs = [], [], []
        for room in self.rooms:
            # Only the points in the room's box (with room for the fillets) are worked out; the rest are far from it.
            inside = np.all((P >= room.box[0] - 1.0) & (P <= room.box[1] + 1.0), axis=1)
            d, plan, f = np.full(len(P), 50.0), np.full(len(P), 50.0), np.zeros(len(P))
            if inside.any():
                d[inside], plan[inside], f[inside] = room.dist(P[inside])
            ds.append(d)
            plans.append(plan)
            fs.append(f)
        D = np.stack(ds, axis=1)
        dmin = D.min(axis=1)
        w = np.exp(-(D - dmin[:, None]) / 0.3)
        amp = (w * np.array([r.amp for r in self.rooms])[None, :]).sum(axis=1) / w.sum(axis=1)
        floor = np.min(np.stack([f + 4.0 * np.maximum(p, 0.0) for f, p in zip(fs, plans)], axis=1), axis=1)
        d = ds[0]
        for e in ds[1:]:
            d = smin(d, e, UNION_K)
        d = smax(d, floor - P[:, 2], 0.1)
        return d, floor, amp

    def void(self, P):
        P = np.asarray(P, np.float64)
        d, floor, amp = self.cave(P)
        x, y, z = P[:, 0], P[:, 1], P[:, 2]
        h = z - floor
        mask = lo.smoothstep(0.03, 0.8, h)
        near = (np.abs(d) < 1.2) & (mask > 0.0) & (amp > 0.0)
        if near.any():
            n = self.noise
            Q = P[near]
            qx, qy, qz = Q[:, 0], Q[:, 1], Q[:, 2]
            rough = 1.45 * (0.66 * n[0].fbm(qx / 2.6, qy / 2.6, qz / 1.2, 3) + 0.3 * n[1].fbm(qx / 0.9, qy / 0.9, qz / 0.75, 2)
                            + 0.14 * n[2](qx / 0.33, qy / 0.33, qz / 0.33))
            groove = np.zeros(len(Q))
            for level, depth in self.partings:
                lvl = level + 0.12 * n[3](qx / 3.0, qy / 3.0, level)
                open_ = 0.2 + 0.8 * lo.smoothstep(-0.25, 0.3, n[4](qx / 3.6, qy / 3.6, level * 0.7))
                groove += depth * open_ * lo.bell(qz - lvl, 0.09)
            d[near] = d[near] + mask[near] * (amp[near] * rough - groove * np.minimum(amp[near] / 0.18, 1.5))
        for cx, cy, top, r0, r1 in self.caps:
            r = np.hypot(x - cx, y - cy)
            wgt = 1.0 - lo.smoothstep(r0, r1, r)
            pick = wgt > 0.0
            if pick.any():
                target = np.maximum(d[pick], z[pick] - top)
                d[pick] = d[pick] * (1.0 - wgt[pick]) + target * wgt[pick]
        for point, normal, half, feather in self.flats:
            p, nrm = np.array(point, float), np.array(normal, float)
            nrm = nrm / np.linalg.norm(nrm)
            across = np.cross([0.0, 0.0, 1.0], nrm)
            across /= np.linalg.norm(across)
            R = P - p
            a, v, s = R @ across, R[:, 2], R @ nrm
            wgt = (1.0 - lo.smoothstep(half[0], half[0] + feather, np.abs(a))) * \
                  (1.0 - lo.smoothstep(half[1], half[1] + feather, np.abs(v)))
            wgt *= 1.0 - lo.smoothstep(0.4, 1.2, np.abs(s))
            pick = wgt > 0.0
            if pick.any():
                d[pick] = d[pick] * (1.0 - wgt[pick]) + (-s[pick]) * wgt[pick]
        for center, axes, half in self.clears:
            d = np.minimum(d, obox(P, np.asarray(center, float), axes, np.asarray(half, float)))
        return d

    def field(self, P):
        """looter_outcrops' convention: negative inside the rock."""
        return -self.void(P)

    def bounds(self):
        lo_ = np.min([r.box[0] for r in self.rooms], axis=0)
        hi_ = np.max([r.box[1] for r in self.rooms], axis=0)
        return lo_, hi_


# --- Meshing ---

def evaluate(mine, lo_, shape, cell, coarse=4, lipschitz=1.8):
    """The field on the lattice lo_ + cell * (i, j, k), coarse first (looter_outcrops.evaluate), the border forced to
    rock so the surface closes round the void."""
    shape_c = tuple(int(math.ceil((n - 1) / coarse)) + 2 for n in shape)
    nx, ny, nz = shape_c
    xs = lo_[0] + cell * coarse * np.arange(nx)
    ys = lo_[1] + cell * coarse * np.arange(ny)
    zs = lo_[2] + cell * coarse * np.arange(nz)
    C = np.empty(shape_c, np.float32)
    for i in range(nx):
        Y, Z = np.meshgrid(ys, zs, indexing='ij')
        P = np.stack([np.full(Y.size, xs[i]), Y.ravel(), Z.ravel()], axis=1)
        C[i] = mine.field(P).reshape(ny, nz)
    reach = lipschitz * cell * coarse * math.sqrt(3.0)
    near = np.abs(C) < reach
    cells = np.zeros(tuple(n - 1 for n in shape_c), bool)
    for dx, dy, dz in lo._CORNERS:
        cells |= near[dx:shape_c[0] - 1 + dx, dy:shape_c[1] - 1 + dy, dz:shape_c[2] - 1 + dz]
    idx = [np.minimum(np.arange(n) // coarse, cells.shape[a] - 1) for a, n in enumerate(shape)]
    F = np.where(C[np.ix_(idx[0], idx[1], idx[2])] < 0.0, np.float32(-reach), np.float32(reach))
    todo = cells[np.ix_(idx[0], idx[1], idx[2])]
    points = np.argwhere(todo)
    del todo
    for start in range(0, len(points), 1_000_000):
        part = points[start:start + 1_000_000]
        F[part[:, 0], part[:, 1], part[:, 2]] = mine.field(lo_ + part * cell)
    F[0], F[-1], F[:, 0], F[:, -1], F[:, :, 0], F[:, :, -1] = -1.0, -1.0, -1.0, -1.0, -1.0, -1.0
    log(f'evaluated {len(points)} of {F.size} lattice points near the surface')
    return F


_DIAGONALS = (((0, 0, 0), (1, 1, 1)), ((1, 0, 0), (0, 1, 1)), ((0, 1, 0), (1, 0, 1)), ((0, 0, 1), (1, 1, 0)))


def manifold(F, passes=24):
    """Surface nets gives one vertex per cell, so a cell whose rock corners (or void corners) don't touch along a cube
    edge makes two sheets share that vertex: a checkerboard cell face puts four quads on one edge, two opposite
    corners alone pinch a vertex. Such meshes decimate into fins and the clean-up opens them into holes. In each such
    cell the sample nearest zero (the least sure one) changes side, by a hair, until no cell is ambiguous. Edits F."""
    done = set()

    def sureness(p):
        # A sample that changed side once keeps it (the flip would only come back): the next least sure goes.
        return (p in done, float(mag[p]), p)

    for _ in range(passes):
        inside = F < 0.0
        mag = np.abs(F)
        picks = set()
        # Checkerboard faces, on each of the three planes.
        for axis in range(3):
            u, v = (axis + 1) % 3, (axis + 2) % 3
            corners = []
            for du, dv in ((0, 0), (1, 0), (1, 1), (0, 1)):
                s = [slice(None)] * 3
                s[u] = slice(du, F.shape[u] - 1 + du)
                s[v] = slice(dv, F.shape[v] - 1 + dv)
                corners.append((du, dv, tuple(s)))
            a, b, c, d = (inside[s] for _, _, s in corners)
            bad = np.argwhere((a == c) & (b == d) & (a != b))
            for cell in bad:
                best = None
                for du, dv, _ in corners:
                    p = cell.copy()
                    p[u] += du
                    p[v] += dv
                    p = tuple(int(x) for x in p)
                    if best is None or sureness(p) < sureness(best):
                        best = p
                picks.add(best)
        # Two opposite corners on their own (two rock among six void, or two void among six rock).
        nx, ny, nz = F.shape
        count = np.zeros((nx - 1, ny - 1, nz - 1), np.int8)
        for dx in (0, 1):
            for dy in (0, 1):
                for dz in (0, 1):
                    count += inside[dx:nx - 1 + dx, dy:ny - 1 + dy, dz:nz - 1 + dz]
        for p0, p1 in _DIAGONALS:
            c0 = inside[p0[0]:nx - 1 + p0[0], p0[1]:ny - 1 + p0[1], p0[2]:nz - 1 + p0[2]]
            c1 = inside[p1[0]:nx - 1 + p1[0], p1[1]:ny - 1 + p1[1], p1[2]:nz - 1 + p1[2]]
            bad = np.argwhere(((count == 2) & c0 & c1) | ((count == 6) & ~c0 & ~c1))
            for cell in bad:
                a = tuple(int(x) for x in cell + np.array(p0))
                b = tuple(int(x) for x in cell + np.array(p1))
                picks.add(min(a, b, key=sureness))
        if not picks:
            break
        for p in sorted(picks):
            F[p] = 1e-4 if F[p] < 0.0 else -1e-4
        done.update(picks)
    log(f'manifold: {len(done)} samples changed side, {len(picks)} cells still ambiguous')


def islands(V, Q, keep=400):
    """The quads without the small closed bits the noise leaves (a crumb of rock floating in a room, a bubble of void
    inside the rock). Nobody can see them, and the decimator crushes them into back-to-back triangles that open holes
    when the pieces are welded."""
    parent = list(range(len(V)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for a, b, c, d in Q.tolist():
        ra = find(a)
        for o in (b, c, d):
            ro = find(o)
            if ro != ra:
                parent[max(ra, ro)] = min(ra, ro)
                ra = min(ra, ro)
    roots = np.array([find(i) for i in range(len(V))])
    quad_root = roots[Q[:, 0]]
    names, counts = np.unique(quad_root, return_counts=True)
    small = set(names[counts < keep].tolist())
    for name, n in zip(names.tolist(), counts.tolist()):
        if name in small:
            mid = V[Q[quad_root == name]].reshape(-1, 3).mean(axis=0)
            log(f'  dropped an island of {n} quads at {tuple(round(float(c), 2) for c in mid)}')
    log(f'islands: {len(names) - len(small)} kept, sizes {sorted(counts[counts >= keep].tolist())[-4:]}')
    return Q[~np.isin(quad_root, list(small))] if small else Q


def mesh(mine):
    """The whole mine's surface: vertices V (N, 3) and quads Q (M, 4), facing into the void."""
    start = time.time()
    lo_, hi_ = mine.bounds()
    lo_ = LATTICE + np.floor((lo_ - LATTICE) / CELL) * CELL
    shape = tuple(int(math.ceil((h - l) / CELL)) + 1 for l, h in zip(lo_, hi_))
    log(f'lattice {shape[0]} x {shape[1]} x {shape[2]} from {tuple(round(v, 3) for v in lo_)}')
    F = evaluate(mine, lo_, shape, CELL)
    manifold(F)
    V, Q = lo.surface_nets(F, lo_, CELL)
    del F
    V = lo.project(V, mine, CELL, iterations=2)
    Q = islands(V, Q)
    log(f'{len(V)} vertices, {len(Q)} quads ({time.time() - start:.1f} s)')
    return V, Q


# --- Pieces: dealt out, reduced with locked borders, welded back ---

def deal(V, Q, region):
    """{piece: quad indices} by each quad's middle; quads whose region is None are dropped (the mouth's outside)."""
    mids = V[Q].mean(axis=1)
    names = [region(m) for m in mids]
    pieces = {}
    for i, name in enumerate(names):
        if name is not None:
            pieces.setdefault(name, []).append(i)
    return {k: np.array(v) for k, v in pieces.items()}


def piece_object(name, V, Q, quads):
    """A mesh object of the chosen quads (triangulated along their shorter diagonal), and the indices of its border
    vertices (on an open edge)."""
    used, inverse = np.unique(Q[quads].ravel(), return_inverse=True)
    faces = inverse.reshape(-1, 4)
    verts = V[used]
    tris = []
    for a, b, c, d in faces:
        if np.linalg.norm(verts[a] - verts[c]) <= np.linalg.norm(verts[b] - verts[d]):
            tris += [(a, b, c), (a, c, d)]
        else:
            tris += [(a, b, d), (b, c, d)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts.tolist(), [], tris)
    mesh.validate()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def border(obj):
    """The vertices on open edges, in mesh order."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    out = sorted({v.index for e in bm.edges if e.is_boundary for v in e.verts})
    bm.free()
    return out


def reduce_locked(obj, target):
    """Reduces obj to about target triangles with its border vertices locked (a vertex group at weight 0 there, the
    decimator's own factor at its most), then cleans slivers away from the border. Returns how many border vertices
    moved (should be 0)."""
    locked = border(obj)
    before = {i: obj.data.vertices[i].co.copy() for i in locked}
    group = obj.vertex_groups.new(name='_free')
    group.add(range(len(obj.data.vertices)), 1.0, 'REPLACE')
    group.remove(locked)
    group.add(locked, 0.0, 'REPLACE')
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    if tris > target:
        mod = obj.modifiers.new('Reduce', 'DECIMATE')
        mod.ratio = target / tris
        mod.use_collapse_triangulate = True
        mod.vertex_group = '_free'
        mod.vertex_group_factor = 1000.0
        with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj],
                                       selected_editable_objects=[obj]):
            bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.vertex_groups.clear()
    clean_locked(obj, [tuple(c) for c in before.values()])
    defects(obj)
    # Did the border stay? Every old border vertex must still be there, exactly.
    kd = KDTree(len(obj.data.vertices))
    for i, v in enumerate(obj.data.vertices):
        kd.insert(v.co, i)
    kd.balance()
    moved = sum(1 for co in before.values() if kd.find(co)[2] > 1e-6)
    return moved


def defects(obj):
    """Logs what would break the welded mine's seal: small open loops inside a piece (its seams are long loops) and
    edges with more than two faces. A clean build logs nothing here."""
    for length, centre, count in holes(obj):
        if length < 2.0:
            log(f'  {obj.name}: a small open loop, {count} edges, {length:.3f} m, at {centre}')
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    fins = [e for e in bm.edges if len(e.link_faces) > 2]
    if fins:
        log(f'  {obj.name}: {len(fins)} edges with more than two faces, first at '
            f'{tuple(round(c, 2) for c in fins[0].verts[0].co)}')
    bm.free()


def clean_locked(obj, pinned, min_edge=0.012, thin=0.004):
    """looter_outcrops.clean, keeping the pinned positions: edges with a pinned end are never collapsed or turned."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    kd = KDTree(len(pinned))
    for i, p in enumerate(pinned):
        kd.insert(p, i)
    kd.balance()
    pin = {v.index for v in bm.verts if kd.find(v.co)[2] <= 1e-7}

    def free(e):
        return e.verts[0].index not in pin and e.verts[1].index not in pin

    for _ in range(4):
        bm.verts.index_update()
        short = [e for e in bm.edges if e.calc_length() < min_edge and free(e)]
        if not short:
            break
        bmesh.ops.collapse(bm, edges=short, uvs=False)
        bm.verts.index_update()
        pin = {v.index for v in bm.verts if kd.find(v.co)[2] <= 1e-7}
    for _ in range(6):
        bm.verts.index_update()
        bm.edges.index_update()
        slivers = [f for f in bm.faces if lo._thin(f, thin)]
        if not slivers:
            break
        # Turning an edge moves no vertex, so a sliver against the border may have its long edge turned too.
        edges = sorted({max(f.edges, key=lambda e: e.calc_length()) for f in slivers}, key=lambda e: e.index)
        edges = [e for e in edges if len(e.link_faces) == 2]
        if not edges:
            break
        bmesh.ops.rotate_edges(bm, edges=edges, use_ccw=False)
    # What is still a sliver loses its shortest edge (collapsed, so the surface stays closed), unless that edge touches
    # the border. Nothing is ever deleted: a deleted sliver is a crack (one ran 10 m down the adit's floor).
    for _ in range(4):
        bm.verts.index_update()
        pin = {v.index for v in bm.verts if kd.find(v.co)[2] <= 1e-7}
        bm.edges.index_update()
        slivers = [f for f in bm.faces if f.is_valid and lo._sliver(f)]
        short = sorted({min(f.edges, key=lambda e: e.calc_length()) for f in slivers}, key=lambda e: e.index)
        short = [e for e in short if e.is_valid and free(e)]
        if not short:
            break
        bmesh.ops.collapse(bm, edges=short, uvs=False)
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context='VERTS')
    bm.to_mesh(obj.data)
    bm.free()


def weld(objects, name):
    """One mesh of the pieces, their shared border vertices welded, each face carrying its piece's number ('piece')."""
    bm = bmesh.new()
    layer = bm.faces.layers.int.new('piece')
    for k, obj in enumerate(objects):
        part = bmesh.new()
        part.from_mesh(obj.data)
        verts = [bm.verts.new(v.co) for v in part.verts]
        for f in part.faces:
            try:
                nf = bm.faces.new([verts[v.index] for v in f.verts])
            except ValueError:
                log(f'  weld: {obj.name} skipped a face at {tuple(round(c, 2) for c in f.calc_center_median())}')
                continue
            nf[layer] = k
        part.free()
    before = len(bm.verts)
    faces_before = len(bm.faces)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
    log(f'welded {before - len(bm.verts)} border vertices, {faces_before - len(bm.faces)} faces lost')
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def holes(obj, mouth_y=-0.5):
    """The open edges' loops other than the mouth's (all of whose ends lie at y < mouth_y): (length, centre) each."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    edges = [e for e in bm.edges if e.is_boundary]
    seen, loops = set(), []
    for e in edges:
        if e.index in seen:
            continue
        stack, group = [e], []
        seen.add(e.index)
        while stack:
            cur = stack.pop()
            group.append(cur)
            for v in cur.verts:
                for nxt in v.link_edges:
                    if nxt.is_boundary and nxt.index not in seen:
                        seen.add(nxt.index)
                        stack.append(nxt)
        pts = [v.co for g in group for v in g.verts]
        if max(p.y for p in pts) < mouth_y:
            continue
        centre = sum(pts, Vector()) / len(pts)
        loops.append((sum(g.calc_length() for g in group), tuple(round(c, 2) for c in centre), len(group)))
    bm.free()
    return loops


def open_edges(obj):
    """How many open edges the mesh has, and the range of y their ends lie in (the mouth's ring, y about -1)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    edges = [e for e in bm.edges if e.is_boundary]
    ys = [v.co.y for e in edges for v in e.verts]
    bm.free()
    return f'{len(edges)}' + (f' at y {min(ys):.2f}..{max(ys):.2f}' if ys else '')


# --- Shading the welded mesh ---

def shade(obj, floor_at, seed=11):
    """Materials (RockCliffDen; DenFloor where a face looks up within 18 cm of its room's floor: floor_at(x, y, z)),
    world-scale UVs in the mine's frame, smooth normals with hard edges past 55 degrees."""
    import looter_mountain as mtn
    rock = mtn.EXTRA['rockden']()
    dirt = mtn.EXTRA['denfloor']()
    mesh = obj.data
    mesh.materials.clear()
    mesh.materials.append(rock)
    mesh.materials.append(dirt)
    n = len(mesh.polygons)
    C = np.empty(3 * n)
    N = np.empty(3 * n)
    mesh.polygons.foreach_get('center', C)
    mesh.polygons.foreach_get('normal', N)
    C, N = C.reshape(-1, 3), N.reshape(-1, 3)
    fz = floor_at(C)
    floor = (N[:, 2] > 0.8) & (C[:, 2] < fz + 0.18)
    mesh.polygons.foreach_set('material_index', floor.astype(np.int32))
    mesh.update()
    floors = [int(i) for i in np.nonzero(floor)[0]]
    walls = [int(i) for i in np.nonzero(~floor)[0]]
    rock_uv(obj, walls, ROCK_DENSITY, seed)
    lt.box_uv(obj, 'GroundDirt', texel_density=FLOOR_DENSITY, faces=floors, seed=seed + 1)
    for p in mesh.polygons:
        p.use_smooth = True
    mesh.set_sharp_from_angle(angle=math.radians(55.0))
    return floors


def rock_uv(obj, faces, density, seed, roof=0.82):
    """RockCliff at world scale in the mine's frame, by cube projection leaning to the sides: a face takes the top
    projection only when it looks within 35 degrees of straight up or down (a roof's crown), so the beds' bands (V is
    height on every side) carry on up the walls and round an arched roof instead of breaking where it turns."""
    rng = random.Random(seed)
    scale = density / float(lt.SETS['RockCliff']['size'])
    shift = (rng.uniform(0.0, 4.0), rng.uniform(0.0, 4.0))
    wanted = set(faces)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    uv = bm.loops.layers.uv.active or bm.loops.layers.uv.new('UVMap')
    for face in bm.faces:
        if face.index not in wanted:
            continue
        n = face.normal
        if abs(n.z) > roof:
            u_axis, v_axis = Vector((1.0, 0.0, 0.0)), Vector((0.0, 1.0 if n.z > 0.0 else -1.0, 0.0))
        elif abs(n.x) >= abs(n.y):
            u_axis, v_axis = Vector((0.0, 1.0 if n.x > 0.0 else -1.0, 0.0)), Vector((0.0, 0.0, 1.0))
        else:
            u_axis, v_axis = Vector((-1.0 if n.y > 0.0 else 1.0, 0.0, 0.0)), Vector((0.0, 0.0, 1.0))
        for loop in face.loops:
            co = loop.vert.co
            loop[uv].uv = ((co.dot(u_axis) + shift[0]) * scale, (co.dot(v_axis) + shift[1]) * scale)
    bm.to_mesh(obj.data)
    bm.free()


def keep_normals(obj):
    """Stores the mesh's corner normals in a corner attribute '_n' (split pieces get them back as custom normals)."""
    mesh = obj.data
    normals = np.empty(3 * len(mesh.loops), np.float32)
    mesh.corner_normals.foreach_get('vector', normals)
    attr = mesh.attributes.get('_n') or mesh.attributes.new('_n', 'FLOAT_VECTOR', 'CORNER')
    attr.data.foreach_set('vector', normals)


def split(obj, count, names):
    """The welded, shaded mesh split back into its pieces (by the 'piece' face attribute), each its own object with the
    stored normals set as custom normals."""
    out = []
    mesh = obj.data
    pieces = np.empty(len(mesh.polygons), np.int32)
    mesh.attributes['piece'].data.foreach_get('value', pieces)
    for k in range(count):
        copy = obj.copy()
        copy.data = mesh.copy()
        copy.name = names[k]
        copy.data.name = names[k]
        bpy.context.scene.collection.objects.link(copy)
        bm = bmesh.new()
        bm.from_mesh(copy.data)
        layer = bm.faces.layers.int.get('piece')
        gone = [f for f in bm.faces if f[layer] != k]
        bmesh.ops.delete(bm, geom=gone, context='FACES')
        bm.to_mesh(copy.data)
        bm.free()
        restore_normals(copy)
        out.append(copy)
    return out


def restore_normals(obj):
    mesh = obj.data
    attr = mesh.attributes.get('_n')
    if attr is None:
        return
    normals = np.empty(3 * len(mesh.loops), np.float32)
    attr.data.foreach_get('vector', normals)
    normals = normals.reshape(-1, 3)
    normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-9)
    mesh.normals_split_custom_set([tuple(n) for n in normals])
    mesh.attributes.remove(mesh.attributes['_n'])
    if 'piece' in mesh.attributes:
        mesh.attributes.remove(mesh.attributes['piece'])
    # Unused material slots go (a piece without a floor keeps only the rock).
    used = sorted({p.material_index for p in mesh.polygons})
    if len(used) < len(mesh.materials):
        remap = {old: new for new, old in enumerate(used)}
        idx = np.array([remap[p.material_index] for p in mesh.polygons], np.int32)
        mats = [mesh.materials[i] for i in used]
        mesh.materials.clear()
        for m in mats:
            mesh.materials.append(m)
        mesh.polygons.foreach_set('material_index', idx)
        mesh.update()


def raycast(obj, origin, direction, distance=20.0):
    """The first hit on obj's mesh (in its own coordinates) from origin along direction: (point, normal) or None."""
    from mathutils.bvhtree import BVHTree
    tree = getattr(raycast, '_trees', {}).get(obj.name)
    if tree is None:
        tree = BVHTree.FromObject(obj, bpy.context.evaluated_depsgraph_get())
        raycast._trees = getattr(raycast, '_trees', {})
        raycast._trees[obj.name] = tree
    hit, normal, _, _ = tree.ray_cast(Vector(origin), Vector(direction).normalized(), distance)
    return (hit, normal) if hit is not None else None
