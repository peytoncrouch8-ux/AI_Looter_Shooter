"""The fore jumbo's paint (looter_tripo_barge.rework, right after its copies are laid). The jumbo is one plain slab of
Tripo's foredeck copied, every other copy mirrored, so its joints weld exactly (no crease); its paint is then taken off
the slab's own 89 cm and read from long stretches of Tripo's own side and foredeck instead: every corner's UV is the one
of the nearest point of Tripo's surface as it came (before any cut) that faces its way, at the station its window maps
it to. The faces fall in two families, each laid in windows of whole copies (the paint's seams on the copy joints,
where the plates meet):

- 'hull': the side's outer faces (facing out, over HULL_Z), from Tripo's plain side between the house's vertical pipe
  and the cut (x 1452..2163 on both sides: the strakes and bands at the cut's heights, no frame or pipe), in windows
  ending at the cut, so no stretch of the side comes back within 7.1 m and the run flows on into the bow's own paint.
- 'deck': the deck, the bulwark's inner face and its capping (facing up or inboard, over DECK_Z), from Tripo's
  foredeck (x 2020..2285, the house's foot and the forecastle's step left out), two copies a window from starts that
  vary and from either side in turn.

Where a corner lies at the source station: the side's faces and the bulwark's inner face move out by how much fuller
Tripo's side is there at that height (the median over OFFSET_Z of the two sections' difference: the strakes stand out of
both, so a band keeps its depth), the deck and the capping's top scale with the bulwark (its centreline stays on it).
The strakes' edges sit a few cm higher or lower from station to station, so a corner takes the nearest surface facing
its way within RADIUS (a ray along its normal missed them); one with none there (a bolt or strap standing out at that
station) is read SHIFTS further along. Each corner is read once (a vertex's faces share its paint); a face whose corners
fall in two of Tripo's UV charts takes the chart under its middle for all of them (extrapolated: no corner reads another
chart's paint).
"""
import numpy as np
from mathutils.bvhtree import BVHTree
from mathutils.geometry import barycentric_transform

from looter_tripo_rework import B, D, U, ue_centre, ue_normal

HULL_Z, HULL_NY = 1100.0, 0.35         # a 'hull' face: over this height, facing out at least this much
DECK_Z, DECK_NZ, DECK_NY = 2240.0, 0.5, 0.35   # a 'deck' face: over this height, facing up or inboard this much
HULL_SRC = (1452.0, 2163.0)            # Tripo's x: the plain side every 'hull' window lies in (both sides)
DECK_SRC = (2020.0, 2285.0)            # Tripo's x: the foredeck every 'deck' window lies in
# Windows, aft to fore: (copies, Tripo's x the window starts at or 'end' (it ends at the cut), the source side: 1 the
# face's own, -1 the other's).
WINDOWS = {'hull': ((2, 'end', 1), (8, 'end', 1), (8, 'end', 1)),
           'deck': ((2, 2020.0, 1), (2, 2085.0, -1), (2, 2045.0, 1), (2, 2105.0, -1), (2, 2030.0, -1),
                    (2, 2070.0, 1), (2, 2100.0, 1), (2, 2055.0, -1), (2, 2025.0, 1))}
RADIUS = {'hull': 12.0, 'deck': 15.0}  # cm: a corner reads the nearest of Tripo's surface facing its way within this
SHIFTS = (0.0, 10.0, -10.0, 20.0, -20.0, 30.0, -30.0)    # cm along the source, where nothing faces its way near
NORMAL_MIN = 0.6                       # the surface read faces the corner's way this much
PROFILE_X = (1380.0, 2330.0, 10.0)     # Tripo's x: the stations whose half-beams map a corner's |y|
PROFILE_Z = (900.0, 2476.0, 10.0)
SMOOTH = 5                             # stations each side in the half-beams' running median (a bolt is not the side)
OFFSET_Z = 4                           # rows (PROFILE_Z) each side in the median of two sections' difference
RATIO = (0.8, 1.35)                    # the deck's |y| scale's limits
RATIO_Z = 2300.0                       # the height the deck's scale is the bulwark's at
UV_SAME = 1e-5                         # two corners' UVs this close: one chart across their edge


def log(message):
    print(f'LOOTER: DredgeBarge rework: {message}', flush=True)


class Source:
    """Tripo's surface as it came: its triangles with their UVs and UV charts, a tree for lookups, the side's
    half-beams."""

    def __init__(self, bm, uv):
        bm.faces.index_update()                  # the charts are kept by face index
        chart = _charts(bm, uv)
        self.pts, self.uvs, self.chart, tris = [], [], [], []
        for f in bm.faces:
            loops = list(f.loops)
            for k in range(1, len(loops) - 1):
                tri = (loops[0], loops[k], loops[k + 1])
                base = len(self.pts)
                self.pts += [l.vert.co.copy() for l in tri]
                self.uvs.append([l[uv].uv.to_3d() for l in tri])
                self.chart.append(chart[f.index])
                tris.append((base, base + 1, base + 2))
        self.tree = BVHTree.FromPolygons(self.pts, tris)
        x0, x1, dx = PROFILE_X
        self.xs = np.arange(x0, x1 + dx / 2.0, dx)
        self.zs = np.arange(*PROFILE_Z)
        near = [i for i in range(len(tris)) if x0 - 50.0 < -self.pts[3 * i].y * 100.0 < x1 + 50.0]
        ue = np.array([[(-p.y * 100.0, -p.x * 100.0, p.z * 100.0) for p in self.pts[3 * i:3 * i + 3]] for i in near])
        self.hb = {}
        for s in (-1, 1):
            h = np.array([_section(ue, x + 0.5, self.zs, s) for x in self.xs])           # off the vertices' planes
            self.hb[s] = np.array([np.median(h[max(i - SMOOTH, 0):i + SMOOTH + 1], axis=0) for i in range(len(h))])
        self.off = {}

    def _bilinear(self, h, x, z):
        i = min(max((x - self.xs[0]) / (self.xs[1] - self.xs[0]), 0.0), len(self.xs) - 1.0)
        j = min(max((z - self.zs[0]) / (self.zs[1] - self.zs[0]), 0.0), len(self.zs) - 1.0)
        i0, j0 = int(i), int(j)
        i1, j1 = min(i0 + 1, len(self.xs) - 1), min(j0 + 1, len(self.zs) - 1)
        fi, fj = i - i0, j - j0
        return float((h[i0, j0] * (1 - fi) + h[i1, j0] * fi) * (1 - fj) + (h[i0, j1] * (1 - fi) + h[i1, j1] * fi) * fj)

    def beam(self, s, x, z):
        return self._bilinear(self.hb[s], x, z)

    def ratio(self, s_from, x_from, s_to, x_to, z):
        """The |y| scale taking a point of side s_from's section at x_from to side s_to's at x_to."""
        here, there = self.beam(s_from, x_from, z), self.beam(s_to, x_to, z)
        return min(max(there / here, RATIO[0]), RATIO[1]) if here > 50.0 else 1.0

    def prepare(self, cut):
        """The offsets from the section at cut to every station's (both sides each way): per height, the median over
        OFFSET_Z rows of their half-beams' difference."""
        for s_from in (-1, 1):
            at_cut = np.array([self.beam(s_from, cut, z) for z in self.zs])
            for s_to in (-1, 1):
                d = self.hb[s_to] - at_cut[None, :]
                pad = np.pad(d, ((0, 0), (OFFSET_Z, OFFSET_Z)), mode='edge')
                win = np.lib.stride_tricks.sliding_window_view(pad, 2 * OFFSET_Z + 1, axis=1)
                self.off[(s_from, s_to)] = np.median(win, axis=-1)

    def offset(self, s_from, s_to, x, z):
        """How much fuller side s_to's section at x is than side s_from's at the prepared cut, at height z (cm)."""
        return self._bilinear(self.off[(s_from, s_to)], x, z)

    def nearest(self, p, n, radius, chart=None):
        """The nearest point of Tripo's surface to p (cm) facing n's way (in chart, when given), within radius cm:
        (triangle, Blender point), or None."""
        nb = D(n)
        best = None
        for loc, nrm, i, dist in self.tree.find_nearest_range(B(p), radius / 100.0):
            if i is None or nrm.dot(nb) < NORMAL_MIN or (chart is not None and self.chart[i] != chart):
                continue
            if best is None or dist < best[2]:
                best = (i, loc, dist)
        return best[:2] if best else None

    def uv_at(self, i, loc):
        """Triangle i's UV at the Blender point loc (outside the triangle: its map carried on)."""
        a, b, c = self.pts[3 * i:3 * i + 3]
        u0, u1, u2 = self.uvs[i]
        return barycentric_transform(loc, a, b, c, u0, u1, u2).to_2d()


def _section(tris, x, zs, s):
    """The skin's section at x on side s: its outermost |y| at each height zs (cm), read off the segments the plane cuts
    out of each triangle (exact between Tripo's sparse edges; a height it misses takes its neighbours')."""
    d = tris[:, :, 0] - x
    side = d > 0.0
    hit = side.any(1) & ~side.all(1)
    t, dd = tris[hit], d[hit]
    ends = []
    for a, b in ((0, 1), (1, 2), (2, 0)):
        cross = (dd[:, a] > 0.0) != (dd[:, b] > 0.0)
        w = np.where(cross, dd[:, a] / np.where(cross, dd[:, a] - dd[:, b], 1.0), 0.0)
        ends.append((cross, t[:, a, 1:] + (t[:, b, 1:] - t[:, a, 1:]) * w[:, None]))
    # Each cut triangle's two crossing edges: its segment (y, z) to (y, z).
    first = np.where(ends[0][0][:, None], ends[0][1], ends[1][1])
    second = np.where((ends[0][0] & ends[1][0])[:, None], ends[1][1], ends[2][1])
    y0, z0, y1, z1 = first[:, 0], first[:, 1], second[:, 0], second[:, 1]
    lo, hi = np.minimum(z0, z1), np.maximum(z0, z1)
    out = np.zeros(len(zs))
    for j, z in enumerate(zs):
        m = (lo <= z) & (hi >= z) & (hi > lo)
        if not m.any():
            continue
        y = y0[m] + (y1[m] - y0[m]) * (z - z0[m]) / (z1[m] - z0[m])
        y = y[y * s > 0.0]
        out[j] = np.abs(y).max() if len(y) else 0.0
    for _ in range(4):
        out = np.where(out == 0.0, np.maximum(np.roll(out, 1), np.roll(out, -1)), out)
    return out


def _charts(bm, uv):
    """Each face's UV chart (faces joined across every edge whose two sides' UVs agree at both ends)."""
    parent = list(range(len(bm.faces)))

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a
    for e in bm.edges:
        loops = list(e.link_loops)
        if len(loops) != 2:
            continue
        at = [{l.vert: l[uv].uv, l.link_loop_next.vert: l.link_loop_next[uv].uv} for l in loops]
        if all((at[0][v] - at[1][v]).length < UV_SAME for v in e.verts):
            parent[find(loops[0].face.index)] = find(loops[1].face.index)
    return [find(i) for i in range(len(parent))]


def windows(cut, length):
    """Each family's windows: (from, to: cm from the cut; Tripo's x at 'from'; the source side)."""
    out = {}
    for name, spec in WINDOWS.items():
        pos, rows = 0.0, []
        lo, hi = HULL_SRC if name == 'hull' else DECK_SRC
        for copies, start, side in spec:
            span = copies * length
            src = cut - span if start == 'end' else start
            if src < lo - 1.0 or src + span > hi + 1.0:
                raise ValueError(f'{name} window from x {src:.0f} ({span:.0f} cm) leaves the stretch {lo:.0f}..{hi:.0f}')
            rows.append((pos, pos + span, src, side))
            pos += span
        out[name] = rows
    return out


def family(c, n, s):
    """'hull', 'deck' or None (the bottom, the keel: they keep the copies' paint) for a face at c facing n on side s."""
    if c[2] > HULL_Z and n[1] * s > HULL_NY:
        return 'hull'
    if c[2] > DECK_Z and (n[2] > DECK_NZ or n[1] * s < -DECK_NY):
        return 'deck'
    return None


def paint(bm, tag, part, src, cut, length, total):
    """Every face of part (the fore jumbo's copies, laid from the cut for total cm) painted from its family's windows.
    Returns the tally."""
    uv = bm.loops.layers.uv.active
    rows = windows(cut, length)
    if any(abs(r[-1][1] - total) > 0.5 for r in rows.values()):
        raise ValueError(f'the paint windows do not cover the jumbo ({total:.0f} cm)')
    src.prepare(cut)
    bm.normal_update()
    tally = {k: 0 for k in ('hull', 'deck', 'shifted', 'two charts', 'kept hull', 'kept deck', 'other')}
    read = {}
    for f in list(bm.faces):
        if f[tag] != part:
            continue
        c, n = ue_centre(f), ue_normal(f)
        s = 1 if c[1] >= 0.0 else -1
        name = family(c, n, s)
        if name is None:
            tally['other'] += 1
            continue
        k = next((k for k, w in enumerate(rows[name]) if w[0] - 0.5 <= c[0] - cut <= w[1] + 0.5), len(rows[name]) - 1)
        how = (name, k, s, n[2] > DECK_NZ)
        # The face's chart is the one under its middle; its corners read that chart (the nearest of it facing their
        # way), so faces of one chart share their corners' paint and a chart's border is a seam, as on Tripo's own.
        mid = _corner(src, c, n, rows[name][k], how, cut)
        if mid is None:
            tally['kept ' + name] += 1
            continue
        chart = src.chart[mid[0]]
        hits = []
        for loop in f.loops:
            key = (loop.vert, how, chart)
            if key not in read:
                v = loop.vert.normal
                read[key] = _corner(src, U(loop.vert.co), (-v.y, -v.x, v.z), rows[name][k], how, cut, chart)
            hits.append(read[key])
        tally[name] += 1
        tally['shifted'] += mid[2] or any(h is not None and h[2] for h in hits)
        if any(h is None for h in hits):
            tally['two charts'] += 1               # a corner with none of the chart near: the middle's map carried on
            hits = [h if h is not None else (mid[0], B(_place(src, U(l.vert.co), rows[name][k], how, cut, 0.0)), False)
                    for h, l in zip(hits, f.loops)]
        for loop, h in zip(f.loops, hits):
            loop[uv].uv = src.uv_at(h[0], h[1])
    log('jumbo paint: ' + ', '.join(f'{k} {v}' for k, v in tally.items()) + ' faces (the hull side from x '
        f'{HULL_SRC[0]:.0f}..{HULL_SRC[1]:.0f}, the deck from x {DECK_SRC[0]:.0f}..{DECK_SRC[1]:.0f})')
    return tally


def _place(src, p, window, how, cut, shift):
    """Where on Tripo's side the point p (cm) of the jumbo lies at its window's station (shifted)."""
    _, _, s, flat = how
    w0, _, x_src, side = window
    s_src = s * side
    xs = x_src + (p[0] - cut - w0) + shift
    if flat:
        return (xs, p[1] * side * src.ratio(s, cut, s_src, xs, RATIO_Z), p[2])
    return (xs, s_src * (abs(p[1]) + src.offset(s, s_src, xs, p[2])), p[2])


def _corner(src, p, n, window, how, cut, chart=None):
    """Where the point p (cm) facing n reads its paint (in chart, when given): (triangle, Blender point, shifted), or
    None."""
    n_src = (n[0], n[1] * window[3], n[2])
    for shift in SHIFTS:
        hit = src.nearest(_place(src, p, window, how, cut, shift), n_src, RADIUS[how[0]], chart)
        if hit is not None:
            return hit[0], hit[1], shift != 0.0
    return None
