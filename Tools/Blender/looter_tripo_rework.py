"""Tools for reshaping a fitted Tripo mesh by script (looter_tripo_barge, the dredge barge's rework, uses them), all in the
Unreal frame: points in cm, X front, Y right, Z up; B() turns one into the model's Blender metres (front -Y: Unreal +X is
Blender -Y, Unreal +Y is Blender -X).

- bisect() cuts faces along a plane, spanning() finds the faces a plane crosses that need the cut (tests over(), above(),
  reaching()), cap() fills the cut a deletion left on a plane (its boundary chains, open ones closed along a base height),
  outline() the skin's half-beam at a height; tidy() afterwards collapses short edges, triangulates cut polygons the best
  way round and drops hairline slivers (Unreal's "nearly zero tangents" come from them).
- Build makes new faces (flat shaded, tagged, every loop's colour white when the mesh has colours) as tiled rectangles,
  boxes or quads; ucx() adds a box collision hull.
- Paint gives new faces Tripo's own paint: a Tripo atlas has no tiling area, so a new face's UVs are a tiny square
  (PATCH px) inside the UV charts of a matching Tripo surface, where colour and normal map are flattest and the colour is
  the surface's usual one; a tile shows that spot's colour stretched (a soft gradient), and tiles differ a little in tone.
"""
import random

import bmesh
import numpy as np
from mathutils import Vector

import looter_model as lm
import looter_tripo as tripo

ATLAS, PATCH, FLAT_WIN = 1024, 2, 3   # px: patches are picked at this atlas size; a patch spans PATCH texel centres
                                      # (4 px of a 2K map); flatness is measured over FLAT_WIN px
PATCHES, FLATTEST = 8, 0.3            # patches per paint, from the flattest share of its charts
COLOUR_BIN, COLOUR_TOL = 0.04, 0.01   # sRGB: a paint's usual colour (the fullest bin), how far a patch may stray
TILE = 300.0                          # cm: a new surface's paint tiles
EPS = 0.006                           # m: on a cut plane (BISECT_DIST and a margin)
BISECT_DIST = 0.005                   # m: a vertex this near a cut plane counts as on it (no sliver split off)
ON_BASE = BISECT_DIST * 100.0 + 0.5   # cm: a cut chain's end this near the base height ends on it
LEVEL_FACE = 4.0                      # cm: a face rising less than this is not cut level (no slivers)
SLIVER = 0.003                        # m: edges shorter than this collapse after the cuts, faces thinner go (Unreal's
                                      # zero tangents come from them)


def log(message):
    print(f'LOOTER: rework: {message}', flush=True)


def B(p):
    return Vector((-p[1] / 100.0, -p[0] / 100.0, p[2] / 100.0))


def D(d):
    return Vector((-d[1], -d[0], d[2]))


def U(co):
    return (-co.y * 100.0, -co.x * 100.0, co.z * 100.0)


def ue_centre(f):
    return U(f.calc_center_median())


def ue_normal(f):
    return (-f.normal.y, -f.normal.x, f.normal.z)


# --- Paint: plain patches of Tripo's atlas ---

class Paint:
    def __init__(self, bm, uv, maps, paints, seed):
        self.bc = tripo.pixels(maps['BC'], ATLAS)[..., :3].astype(np.float32)
        self.nm = tripo.pixels(maps['N'], ATLAS)[..., :3].astype(np.float32) if maps.get('N') is not None else None
        self.rng = random.Random(seed)
        self.sets = {}
        self.colour = None
        for key, rule in paints.items():
            faces = [f for f in bm.faces if self._wanted(f, rule)]
            self.sets[key] = self._patches(faces, uv, rule.get('dark'))
            log(f'paint {key}: {len(faces)} Tripo faces, {len(self.sets[key])} patches near {self.colour}')

    @staticmethod
    def _wanted(f, rule):
        x, y, z = ue_centre(f)
        n = ue_normal(f)
        if 'up' in rule and n[2] < rule['up']:
            return False
        if 'side' in rule and abs(n[2]) > rule['side']:
            return False
        return rule['x'][0] <= x <= rule['x'][1] and rule['z'][0] <= z <= rule['z'][1] and \
            rule['y'][0] <= abs(y) <= rule['y'][1]

    def _patches(self, faces, uv, dark=None):
        mask = np.zeros((ATLAS, ATLAS), bool)
        for f in faces:
            pts = [((l[uv].uv.x % 1.0) * ATLAS, (1.0 - l[uv].uv.y % 1.0) * ATLAS) for l in f.loops]
            for k in range(1, len(pts) - 1):
                _raster(mask, pts[0], pts[k], pts[k + 1])
        mask[:2, :] = mask[-2:, :] = mask[:, :2] = mask[:, -2:] = False
        for _ in range(PATCH // 2 + 2):         # the whole square and 2 px round it inside the charts
            mask &= np.roll(mask, 1, 0) & np.roll(mask, -1, 0)
            mask &= np.roll(mask, 1, 1) & np.roll(mask, -1, 1)
        if not mask.any():
            return []
        luma = self.bc @ np.array([0.299, 0.587, 0.114], np.float32)
        # Flat: the colour and the normal map vary least round the patch, and the normal map faces straight out.
        flat = np.sqrt(_local_var(luma, FLAT_WIN))
        if self.nm is not None:
            for c in (0, 1):
                flat += np.sqrt(_local_var(self.nm[..., c], FLAT_WIN)) + np.abs(_local_mean(self.nm[..., c], FLAT_WIN) - 0.5)
        ys, xs = np.nonzero(mask)
        sel = flat[ys, xs] <= np.quantile(flat[ys, xs], FLATTEST)
        ys, xs = ys[sel], xs[sel]
        cols = np.stack([_local_mean(self.bc[..., c], FLAT_WIN)[ys, xs] for c in range(3)], axis=1)
        if dark is not None:
            # A dark paint (window glass): only spots darker than dark (sRGB luma).
            sel = cols @ np.array([0.299, 0.587, 0.114], np.float32) < dark
            if not sel.any():
                return []
            ys, xs, cols = ys[sel], xs[sel], cols[sel]
        # The paint's usual colour: the fullest bin of the flat patches' colours; the tiles keep near it.
        bins, counts = np.unique(np.round(cols / COLOUR_BIN).astype(int), axis=0, return_counts=True)
        target = bins[np.argmax(counts)] * COLOUR_BIN
        keep = np.abs(cols - target).max(axis=1) < COLOUR_TOL
        ys, xs = ys[keep], xs[keep]
        order = np.argsort(flat[ys, xs])
        chosen = []
        for i in order:
            if all((xs[i] - cx) ** 2 + (ys[i] - cy) ** 2 > (4 * PATCH) ** 2 for cx, cy in chosen):
                chosen.append((int(xs[i]), int(ys[i])))
                if len(chosen) >= PATCHES:
                    break
        self.colour = '#%02x%02x%02x' % tuple(int(round(min(max(c, 0.0), 1.0) * 255)) for c in target)
        return chosen

    def square(self, key):
        """The four UV corners (u0 v0, u1 v0, u1 v1, u0 v1) of one patch of paint key."""
        pool = self.sets.get(key) or next(s for s in self.sets.values() if s)
        cx, cy = pool[self.rng.randrange(len(pool))]
        # From texel centre to texel centre, so a tile is a soft gradient between them (no spot in its middle).
        u0, u1 = (cx + 0.5) / ATLAS, (cx + 0.5 + PATCH - 1) / ATLAS
        v0, v1 = 1.0 - (cy + 0.5 + PATCH - 1) / ATLAS, 1.0 - (cy + 0.5) / ATLAS
        return [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]


def _raster(mask, a, b, c):
    x0, x1 = int(max(min(a[0], b[0], c[0]), 0)), int(min(max(a[0], b[0], c[0]) + 1, ATLAS))
    y0, y1 = int(max(min(a[1], b[1], c[1]), 0)), int(min(max(a[1], b[1], c[1]) + 1, ATLAS))
    if x1 <= x0 or y1 <= y0:
        return
    gx, gy = np.meshgrid(np.arange(x0, x1) + 0.5, np.arange(y0, y1) + 0.5)
    d = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
    if abs(d) < 1e-9:
        return
    l1 = ((b[1] - c[1]) * (gx - c[0]) + (c[0] - b[0]) * (gy - c[1])) / d
    l2 = ((c[1] - a[1]) * (gx - c[0]) + (a[0] - c[0]) * (gy - c[1])) / d
    mask[y0:y1, x0:x1] |= (l1 >= 0) & (l2 >= 0) & (l1 + l2 <= 1)


def _local_mean(a, k):
    s = np.pad(np.cumsum(np.cumsum(a, 0), 1), ((1, 0), (1, 0)))
    h = k // 2
    out = np.zeros_like(a)
    y0, y1 = h, a.shape[0] - h
    out[y0:y1, h:a.shape[1] - h] = (s[y0 + h + 1:y1 + h + 1, k:] - s[y0 - h:y1 - h, k:] - s[y0 + h + 1:y1 + h + 1, :-k]
                                    + s[y0 - h:y1 - h, :-k])[:, :a.shape[1] - 2 * h] / float(k * k)
    return out


def _local_var(a, k):
    return np.maximum(_local_mean(a * a, k) - _local_mean(a, k) ** 2, 0.0)


# --- New faces ---

class Build:
    """New faces in the Unreal frame, flat shaded, tagged NEW, each a tile of plain paint."""

    def __init__(self, bm, uv, paint, tag):
        self.bm, self.uv, self.paint, self.tag = bm, uv, paint, tag
        self.colours = list(bm.loops.layers.color) + list(bm.loops.layers.float_color)
        self.faces = 0

    def face(self, pts, out, key, uvs=None):
        verts = [self.bm.verts.new(B(p)) for p in pts]
        f = self.bm.faces.new(verts)
        f.normal_update()
        if f.normal.dot(D(out)) < 0.0:
            f.normal_flip()
        if uvs is None:
            sq = self.paint.square(key)
            uvs = dict(zip(verts, sq)) if len(verts) == 4 else fit(f, sq)
        self.finish(f, key, uvs)
        return f

    def finish(self, f, key, uvs):
        f.smooth = False
        f[self.tag] = 2
        for loop in f.loops:
            loop[self.uv].uv = uvs[loop.vert] if loop.vert in uvs else uvs['all']
            for layer in self.colours:
                loop[layer] = (1.0, 1.0, 1.0, 1.0)
        self.faces += 1

    def rect(self, origin, eu, ev, lu, lv, out, key, tile=TILE):
        """A rectangle origin + eu*u + ev*v (u in 0..lu, v in 0..lv; unit axes), tiled about every tile cm, every other
        row of tiles half a tile along (laid like plating)."""
        o, eu, ev = Vector(origin), Vector(eu), Vector(ev)
        nu, nv = max(1, round(lu / tile)), max(1, round(lv / tile))
        for j in range(nv):
            c, d = lv * j / nv, lv * (j + 1) / nv
            us = [lu * i / nu for i in range(nu + 1)]
            if j % 2 and nu > 1:
                us = [0.0] + [lu * (i + 0.5) / nu for i in range(nu)] + [lu]
            for a, b in zip(us, us[1:]):
                self.face([o + eu * a + ev * c, o + eu * b + ev * c, o + eu * b + ev * d, o + eu * a + ev * d], out, key)

    def box_x(self, x0, x1, y0, y1, z0, z1, key, faces='+x -x +y -y +z'):
        """An axis-aligned box's faces named in faces (no tiling: small parts)."""
        X, Y, Z = (x0, x1), (y0, y1), (z0, z1)
        quads = {'-x': ([(X[0], Y[0], Z[0]), (X[0], Y[1], Z[0]), (X[0], Y[1], Z[1]), (X[0], Y[0], Z[1])], (-1, 0, 0)),
                 '+x': ([(X[1], Y[0], Z[0]), (X[1], Y[1], Z[0]), (X[1], Y[1], Z[1]), (X[1], Y[0], Z[1])], (1, 0, 0)),
                 '-y': ([(X[0], Y[0], Z[0]), (X[1], Y[0], Z[0]), (X[1], Y[0], Z[1]), (X[0], Y[0], Z[1])], (0, -1, 0)),
                 '+y': ([(X[0], Y[1], Z[0]), (X[1], Y[1], Z[0]), (X[1], Y[1], Z[1]), (X[0], Y[1], Z[1])], (0, 1, 0)),
                 '-z': ([(X[0], Y[0], Z[0]), (X[1], Y[0], Z[0]), (X[1], Y[1], Z[0]), (X[0], Y[1], Z[0])], (0, 0, -1)),
                 '+z': ([(X[0], Y[0], Z[1]), (X[1], Y[0], Z[1]), (X[1], Y[1], Z[1]), (X[0], Y[1], Z[1])], (0, 0, 1))}
        for name in faces.split():
            pts, out = quads[name]
            self.face(pts, out, key)


def fit(f, sq):
    """UVs for face f's corners: its outline projected on its own plane, stretched over the patch sq."""
    n = f.normal
    u_axis = n.orthogonal().normalized()
    v_axis = n.cross(u_axis).normalized()
    ps = [(v.co.dot(u_axis), v.co.dot(v_axis)) for v in f.verts]
    lo = [min(p[i] for p in ps) for i in (0, 1)]
    hi = [max(p[i] for p in ps) for i in (0, 1)]
    return {v: (sq[0][0] + (sq[1][0] - sq[0][0]) * (p[0] - lo[0]) / max(hi[0] - lo[0], 1e-5),
                sq[0][1] + (sq[2][1] - sq[0][1]) * (p[1] - lo[1]) / max(hi[1] - lo[1], 1e-5))
            for v, p in zip(f.verts, ps)}


def box_of(bm, tag, part, test=None):
    """The box (x0, y0, z0, x1, y1, z1, cm) round part's vertices that pass test(point), or None."""
    pts = [U(v.co) for f in bm.faces if f[tag] == part for v in f.verts]
    pts = [p for p in pts if test is None or test(p)]
    if not pts:
        return None
    a = np.array(pts)
    return [round(float(v), 1) for v in (*a.min(0), *a.max(0))]


def strip(build, stations, line, z, y_out, out, key, step, span=None):
    """Flat quads at height z between the hull line (per station, |y|) and |y| y_out on both sides over x span, a paint
    patch every three stations, mapped across the group."""
    lo, hi = span or (stations[0], stations[-1])
    ks = [k for k in range(len(stations) - 1) if stations[k + 1] > lo and stations[k] < hi]
    for s in (-1, 1):
        for g in range(0, len(ks), 3):
            group = ks[g:g + 3]
            sq = build.paint.square(key)
            xa, xb = max(stations[group[0]], lo), min(stations[group[-1] + 1], hi)
            ymin = min(min(line[s][k], line[s][k + 1]) for k in group)
            for k in group:
                x0, x1 = max(stations[k], lo), min(stations[k + 1], hi)
                ya = line[s][k] + (line[s][k + 1] - line[s][k]) * (x0 - stations[k]) / step
                yb = line[s][k] + (line[s][k + 1] - line[s][k]) * (x1 - stations[k]) / step
                y0, y1 = min(ya, y_out - 1.0), min(yb, y_out - 1.0)
                pts = [(x0, s * y0, z), (x1, s * y1, z), (x1, s * y_out, z), (x0, s * y_out, z)]
                uvs = [(sq[0][0] + (sq[1][0] - sq[0][0]) * (p[0] - xa) / max(xb - xa, 1.0),
                        sq[0][1] + (sq[2][1] - sq[0][1]) * (abs(p[1]) - ymin) / max(y_out - ymin, 1.0)) for p in pts]
                verts = [build.bm.verts.new(B(p)) for p in pts]
                f = build.bm.faces.new(verts)
                f.normal_update()
                if f.normal.dot(D(out)) < 0.0:
                    f.normal_flip()
                build.finish(f, key, dict(zip(verts, uvs)))


def ucx(obj, boxes, name, x0, x1, y0, y1, z0, z1):
    """A box collision hull (cm, the Unreal frame) on obj, noted in boxes as (name, centre, size)."""
    (x0, x1), (y0, y1), (z0, z1) = sorted((x0, x1)), sorted((y0, y1)), sorted((z0, z1))
    centre = ((x0 + x1) / 2.0, (y0 + y1) / 2.0, (z0 + z1) / 2.0)
    lm.hull_box(obj, ((y1 - y0) / 100.0, (x1 - x0) / 100.0, (z1 - z0) / 100.0), B(centre))
    boxes.append((name, tuple(round(c, 1) for c in centre), (round(x1 - x0, 1), round(y1 - y0, 1), round(z1 - z0, 1))))


# --- Cutting ---

def bisect(bm, faces, axis, value):
    """Cuts faces along the Unreal plane axis = value (axis 0 x, 1 y, 2 z)."""
    co = B(tuple(value if i == axis else 0.0 for i in range(3)))
    no = Vector((0.0, 1.0, 0.0)) if axis == 0 else (Vector((1.0, 0.0, 0.0)) if axis == 1 else Vector((0.0, 0.0, 1.0)))
    faces = list(faces)
    edges = {e for f in faces for e in f.edges}
    verts = {v for f in faces for v in f.verts}
    bmesh.ops.bisect_plane(bm, geom=faces + list(edges) + list(verts), dist=BISECT_DIST, plane_co=co, plane_no=no)


def tidy(bm, cut):
    """After the cuts and fills: short edges collapsed; cut faces (polygons left by splitting a mesh's triangles: test
    cut(face)) and any of more than four corners triangulated, the best way round (a cut can leave a corner on a straight
    edge, which Unreal's own split would make a zero-area triangle); then the slivers dropped."""
    bmesh.ops.dissolve_degenerate(bm, dist=SLIVER, edges=list(bm.edges))
    big = [f for f in bm.faces if len(f.verts) > 4 or (len(f.verts) > 3 and cut(f))]
    bmesh.ops.triangulate(bm, faces=big, quad_method='BEAUTY', ngon_method='BEAUTY')
    # Slivers (a cut passing a corner, a fill along a straight run): hairline faces nobody sees; Unreal would warn.
    thin = [f for f in bm.faces if 2.0 * f.calc_area() < SLIVER * max(e.calc_length() for e in f.edges)]
    bmesh.ops.delete(bm, geom=thin, context='FACES_ONLY')
    bmesh.ops.delete(bm, geom=[e for e in bm.edges if not e.link_faces], context='EDGES')
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    return len(big), len(thin)


def spanning(bm, tag, part, axis, value, test=None):
    """Faces of part (tag) whose vertices lie on both sides of axis = value and pass test(their points, cm): only
    the faces a cut must split (each split near a corner leaves slivers Unreal warns about)."""
    out = []
    for f in bm.faces:
        if f[tag] != part:
            continue
        ps = [U(v.co) for v in f.verts]
        cs = [p[axis] for p in ps]
        if min(cs) < value - 0.05 and max(cs) > value + 0.05 and (test is None or test(ps)):
            out.append(f)
    return out


def over(axis, lo, hi):
    """A spanning() test: the face reaches into lo..hi along axis."""
    return lambda ps: max(p[axis] for p in ps) > lo and min(p[axis] for p in ps) < hi


def above(z):
    """A spanning() test: the face lies above z (an earlier cut there split it)."""
    return lambda ps: min(p[2] for p in ps) > z - 1.0


def steep(ps):
    """A spanning() test: not a nearly level face (a level cut through one leaves slivers; it is kept or cut away
    whole instead)."""
    return max(p[2] for p in ps) - min(p[2] for p in ps) > LEVEL_FACE


def reaching(z):
    """A spanning() test: the face reaches above z."""
    return lambda ps: max(p[2] for p in ps) > z + 1.0


def cap(bm, build, tag, part, axis, value, base_z, out, key):
    """Fills the cut left on the plane axis = value by part's faces: its boundary chains (closed, or open with both ends
    on z = base_z, closed along it) triangle-filled, facing out. Returns the new faces' count."""
    on = lambda v: abs(U(v.co)[axis] - value) < EPS * 100.0
    edges = [e for e in bm.edges if e.is_boundary and e.link_faces[0][tag] == part and on(e.verts[0]) and on(e.verts[1])
             and min(U(v.co)[2] for v in e.verts) > base_z - ON_BASE]
    links = {}
    for e in edges:
        for v in e.verts:
            links.setdefault(v, []).append(e)
    ends = [v for v, es in links.items() if len(es) == 1]
    loose = [v for v in ends if abs(U(v.co)[2] - base_z) > ON_BASE]
    closing = []
    seen, used = set(), set()
    for v in ends:
        if v in seen or v in loose:
            continue
        # Walk the chain from this end (each edge once) to the other end.
        cur, edge = v, links[v][0]
        while True:
            used.add(edge)
            seen.add(cur)
            nxt = edge.other_vert(cur)
            es = [e for e in links[nxt] if e not in used]
            if not es:
                break
            cur, edge = nxt, es[0]
        seen.add(nxt)
        if len(links[nxt]) == 1 and nxt not in loose and nxt is not v:
            closing.append(bm.edges.get((v, nxt)) or bm.edges.new((v, nxt)))
    if not edges:
        return 0
    normal = D(out)
    made = bmesh.ops.triangle_fill(bm, use_beauty=True, use_dissolve=False, edges=edges + closing, normal=normal)
    new = [g for g in made['geom'] if isinstance(g, bmesh.types.BMFace)]
    if not new:
        return 0
    a, b = [i for i in range(3) if i != axis]
    sq = build.paint.square(key)
    for f in new:
        f.normal_update()
        if f.normal.dot(normal) < 0.0:
            f.normal_flip()
        # Each face spans the whole patch on its own (the patch is one flat colour): a fill's small triangles keep a
        # UV area mikktspace can use.
        ps = [U(v.co) for v in f.verts]
        lo = [min(p[i] for p in ps) for i in (a, b)]
        hi = [max(p[i] for p in ps) for i in (a, b)]
        uvs = {}
        for v, p in zip(f.verts, ps):
            s = (p[a] - lo[0]) / max(hi[0] - lo[0], 1e-3)
            t = (p[b] - lo[1]) / max(hi[1] - lo[1], 1e-3)
            uvs[v] = (sq[0][0] + (sq[1][0] - sq[0][0]) * s, sq[0][1] + (sq[2][1] - sq[0][1]) * t)
        build.finish(f, key, uvs)
    log(f'cap at {"xyz"[axis]} {value:.0f}: {len(edges)} boundary edges, {len(closing)} closed along z {base_z:.0f}, '
        f'{len(loose)} loose ends, {len(new)} faces')
    return len(new)


def outline(bm, z, side, stations, step):
    """The hull skin's half-beam (cm, |y|) on one side at height z per station x: the outermost edge crossing there."""
    pts = {}
    for e in bm.edges:
        a, b = U(e.verts[0].co), U(e.verts[1].co)
        if (a[2] - z) * (b[2] - z) > 0.0 or a[2] == b[2]:
            continue
        t = (z - a[2]) / (b[2] - a[2])
        x, y = a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
        if y * side <= 0.0:
            continue
        k = int(round((x - stations[0]) / step))
        if 0 <= k < len(stations):
            pts[k] = max(pts.get(k, 0.0), abs(y))
    line = [pts.get(k, 0.0) for k in range(len(stations))]
    # Gaps (no edge crossing near a station) take the neighbours' value.
    for k in range(len(line)):
        if line[k] == 0.0:
            line[k] = max(line[max(k - 1, 0)], line[min(k + 1, len(line) - 1)])
    return line

