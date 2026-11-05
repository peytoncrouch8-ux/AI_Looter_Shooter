"""The repack's packer and rasterizer (looter_tripo_repack uses them; Docs/Work/2026-11-04-Art/REPACK.md).

raster(): the texels whose centres lie in UV triangles, vectorised by bounding-box size.
islands(): each polygon's UV island, joined across edges whose corners carry the same UVs on both sides (Tripo's seams).
pack(): a raster packer in the way of xatlas (Tripo's own atlases look packed by one): Tripo's islands keep their shapes
and seams and are only turned, scaled and placed.
- Each island is scaled by its 3D area (one texel density over the model), but never below the density it had in
  Tripo's layout (FLOOR: evened out, 28% of the farmhouse's surface came out softer than before, 2026-11-04), and
  turned so its smallest bounding box is axis-aligned; each may then be placed at 0, 90, 180 or 270 degrees.
- Islands are laid largest first, each at the first free place (top row, then left) where its cells miss every placed
  island's cells grown by the gap: tested row run by row run against the grid's row prefix sums in bands from the first
  free row down (a summed-area test for small boxy islands, an FFT correlation for the biggest). The grid is GRID cells across: a cell is size / GRID px (4 at 2K, 2 at
  1K), an island's cells are every cell its triangles touch, so the gap is at least 2 x pad px and a cell's rounding more.
- The scale is the largest that fits: first MIN_GAIN times Tripo's area (no fit: Tripo's layout stays), then bisection
  (FIT_STEPS tries) up to START_HIGH times its scale.
Deterministic (numpy's FFT, no random places, sorted orders): an export repeats exactly at any thread count.
"""
import math

import numpy as np

GRID = 512
FIT_STEPS = 5          # bisection tries after the first fit (scale to ~1.5%)
START_HIGH = 1.6       # the search's top: 1.6 x the scale of Tripo's layout (2.6 x its area)
SMALL_TURNS = 64       # islands of fewer cells try two turns, not four
SMALL_BOX = 36         # islands of at most this many cells in their box are placed by box (summed-area test)
RUN_FFT = 160          # islands of more row runs than this are placed by FFT correlation, the rest by their runs
BAND = 48              # rows a run test scans at once
MAX_CELLS = 1 << 22


# --- Mesh arrays ---

def _uv_layer(me, layer=None):
    if layer is None:
        return me.uv_layers.active
    return me.uv_layers[layer] if isinstance(layer, str) else layer


def loop_uvs(me, layer=None):
    lay = _uv_layer(me, layer)
    uv = np.empty(len(me.loops) * 2, np.float64)
    lay.data.foreach_get('uv', uv)
    return uv.reshape(-1, 2)


def set_loop_uvs(me, uv, layer=None):
    _uv_layer(me, layer).data.foreach_set('uv', np.asarray(uv, np.float32).ravel())
    me.update()


def loop_triangles(me):
    """(loops (T, 3), polygon (T,)): the mesh's triangulation by loop index (what the UV and tangent data are per)."""
    me.calc_loop_triangles()
    n = len(me.loop_triangles)
    L = np.empty(n * 3, np.int64)
    P = np.empty(n, np.int64)
    me.loop_triangles.foreach_get('loops', L)
    me.loop_triangles.foreach_get('polygon_index', P)
    return L.reshape(-1, 3), P


def loop_positions(me):
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    lv = np.empty(len(me.loops), np.int64)
    me.loops.foreach_get('vertex_index', lv)
    return co.reshape(-1, 3)[lv]


def _tri_area3(P):
    return 0.5 * np.linalg.norm(np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0]), axis=1)


def _tri_area2(U):
    a, b, c = U[:, 0], U[:, 1], U[:, 2]
    return 0.5 * np.abs((b[:, 0] - a[:, 0]) * (c[:, 1] - a[:, 1]) - (c[:, 0] - a[:, 0]) * (b[:, 1] - a[:, 1]))


# --- Rasterizer ---

def raster(UV, size, eps=1e-7, rows=None):
    """The texels whose centres lie in each UV triangle (UV (T, 3, 2), 0..1 across size; rows top first): arrays
    (triangle, row, col, barycentrics (n, 3)). rows: the grid's height when it isn't square (UV v then runs over rows
    px: y = (1 - v) * rows)."""
    rows = rows or size
    px = np.stack([UV[..., 0] * size, (1.0 - UV[..., 1]) * rows], -1)
    return raster_px(px, size, rows, eps)


def raster_px(px, width, height, eps=1e-7):
    """raster() on triangles already in pixel coordinates (x col, y row)."""
    lo = np.floor(px.min(1)).astype(np.int64)
    hi = np.ceil(px.max(1)).astype(np.int64)
    lo[:, 0] = np.clip(lo[:, 0], 0, width)
    hi[:, 0] = np.clip(hi[:, 0], 0, width)
    lo[:, 1] = np.clip(lo[:, 1], 0, height)
    hi[:, 1] = np.clip(hi[:, 1], 0, height)
    span = np.maximum(hi - lo, 0).max(1)
    a, b, c = px[:, 0], px[:, 1], px[:, 2]
    den = (b[:, 1] - c[:, 1]) * (a[:, 0] - c[:, 0]) + (c[:, 0] - b[:, 0]) * (a[:, 1] - c[:, 1])
    good = (span > 0) & (np.abs(den) > 1e-12)
    out_t, out_r, out_c, out_b = [], [], [], []
    B = 1
    top = max(width, height)
    while True:
        sel = np.nonzero(good & (span <= B) & ((span > B // 2) if B > 1 else (span >= 1)))[0]
        if len(sel):
            gy, gx = np.meshgrid(np.arange(B), np.arange(B), indexing='ij')
            gx, gy = gx.ravel(), gy.ravel()
            per = max(1, MAX_CELLS // (B * B))
            for s in range(0, len(sel), per):
                t = sel[s:s + per]
                X = lo[t, 0][:, None] + gx[None, :]
                Y = lo[t, 1][:, None] + gy[None, :]
                ok = (X < hi[t, 0][:, None]) & (Y < hi[t, 1][:, None])
                x, y = X + 0.5, Y + 0.5
                A, Bv, C, D = a[t], b[t], c[t], den[t][:, None]
                w0 = ((Bv[:, 1, None] - C[:, 1, None]) * (x - C[:, 0, None])
                      + (C[:, 0, None] - Bv[:, 0, None]) * (y - C[:, 1, None])) / D
                w1 = ((C[:, 1, None] - A[:, 1, None]) * (x - C[:, 0, None])
                      + (A[:, 0, None] - C[:, 0, None]) * (y - C[:, 1, None])) / D
                w2 = 1.0 - w0 - w1
                ok &= (w0 >= -eps) & (w1 >= -eps) & (w2 >= -eps)
                ti, ci = np.nonzero(ok)
                if len(ti):
                    out_t.append(t[ti])
                    out_r.append(Y[ti, ci])
                    out_c.append(X[ti, ci])
                    out_b.append(np.column_stack([w0[ti, ci], w1[ti, ci], w2[ti, ci]]))
        if B >= top:
            break
        B *= 2
    if not out_t:
        return np.zeros(0, np.int64), np.zeros(0, np.int64), np.zeros(0, np.int64), np.zeros((0, 3))
    return np.concatenate(out_t), np.concatenate(out_r), np.concatenate(out_c), np.concatenate(out_b)


def coverage(UV, size):
    """The square's texels whose centres some triangle covers (bool, rows top first) and how many are covered twice or
    more (UV overlap)."""
    _, r, c, _ = raster(UV, size)
    count = np.bincount(r * size + c, minlength=size * size)
    return (count > 0).reshape(size, size), int((count > 1).sum())


# --- Islands ---

def islands(me, uv, polys=None):
    """Each polygon's UV island (an int per polygon, -1 for polygons left out; uv per loop): polygons joined across an
    edge whose two corners carry the same UVs on both sides (Tripo's seams, as Blender's packer sees them)."""
    n = len(me.polygons)
    start = np.empty(n, np.int64)
    total = np.empty(n, np.int64)
    me.polygons.foreach_get('loop_start', start)
    me.polygons.foreach_get('loop_total', total)
    lv = np.empty(len(me.loops), np.int64)
    me.loops.foreach_get('vertex_index', lv)
    poly_of = np.repeat(np.arange(n), total)
    nxt = np.arange(len(me.loops)) + 1
    nxt[start + total - 1] = start
    keep = np.ones(n, bool) if polys is None else np.isin(np.arange(n), polys)
    q = np.round(uv * 1e6).astype(np.int64)
    va, vb = lv, lv[nxt]
    ua, ub = q, q[nxt]
    swap = va > vb
    key = np.column_stack([np.where(swap, vb, va), np.where(swap, va, vb),
                           np.where(swap[:, None], ub, ua), np.where(swap[:, None], ua, ub)])
    idx = np.nonzero(keep[poly_of])[0]
    order = np.lexsort(key[idx].T[::-1])
    k = key[idx][order]
    same = np.all(k[1:] == k[:-1], axis=1)
    owner = poly_of[idx][order]
    pairs = np.column_stack([owner[:-1][same], owner[1:][same]])
    parent = np.arange(n)

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    for p, r in pairs.tolist():
        a_, b_ = find(p), find(r)
        if a_ != b_:
            parent[max(a_, b_)] = min(a_, b_)
    roots = np.array([find(i) for i in range(n)])
    out = np.full(n, -1, np.int64)
    if keep.any():
        _, inv = np.unique(roots[keep], return_inverse=True)
        out[keep] = inv
    return out


# --- The raster packer ---

def _hull(P):
    """Convex hull of 2D points (monotone chain), counter-clockwise."""
    P = np.unique(np.round(P, 9), axis=0)
    if len(P) < 3:
        return P
    P = P[np.lexsort((P[:, 1], P[:, 0]))]

    def half(pts):
        out = []
        for p in pts:
            while len(out) >= 2 and ((out[-1][0] - out[-2][0]) * (p[1] - out[-2][1])
                                     - (out[-1][1] - out[-2][1]) * (p[0] - out[-2][0])) <= 0:
                out.pop()
            out.append(tuple(p))
        return out
    lower, upper = half(P), half(P[::-1])
    return np.array(lower[:-1] + upper[:-1])


def _min_box_angle(P):
    """The turn (radians) that lays P's smallest-area bounding box along the axes (rotating calipers on its hull)."""
    H = _hull(P)
    if len(H) < 3:
        return 0.0
    E = np.roll(H, -1, 0) - H
    ang = np.unique(np.round(np.mod(np.arctan2(E[:, 1], E[:, 0]), math.pi / 2.0), 9))
    best, best_a = None, 0.0
    for a in ang:
        c, s = math.cos(-a), math.sin(-a)
        R = H @ np.array([[c, s], [-s, c]])
        area = np.ptp(R[:, 0]) * np.ptp(R[:, 1])
        if best is None or area < best - 1e-12:
            best, best_a = area, -a
    return best_a


def _turn(P, k):
    """P (n, 2) turned k quarter turns."""
    for _ in range(k % 4):
        P = np.column_stack([-P[:, 1], P[:, 0]])
    return P


def _cells(tris, cell):
    """An island's cells: (mask (h, w) bool, origin cell (x, y)) for its triangles in px (T, 3, 2): every cell a
    triangle's texel centres, corners or edge middles fall in."""
    lo = np.floor(tris.reshape(-1, 2).min(0) / cell).astype(np.int64)
    hi = np.floor(tris.reshape(-1, 2).max(0) / cell).astype(np.int64) + 1
    w, h = int(hi[0] - lo[0]), int(hi[1] - lo[1])
    local = tris - lo * cell
    mask = np.zeros((h, w), bool)
    sub = min(cell, 2)
    _, r, c, _ = raster_px(local * (sub / cell), w * sub, h * sub)
    mask[r // sub, c // sub] = True
    pts = np.concatenate([local.reshape(-1, 2), ((local + np.roll(local, 1, 1)) * 0.5).reshape(-1, 2)])
    pc = np.clip((pts // cell).astype(np.int64), 0, [w - 1, h - 1])
    mask[pc[:, 1], pc[:, 0]] = True
    return mask, lo


def _grow(mask, g):
    """mask grown g cells every way (a square: the corners too)."""
    h, w = mask.shape
    out = np.zeros((h + 2 * g, w + 2 * g), bool)
    for dy in range(2 * g + 1):
        for dx in range(2 * g + 1):
            out[dy:dy + h, dx:dx + w] |= mask
    return out


class Island:
    __slots__ = ('index', 'tris', 'area3', 'pts', 'scale', 'least')

    def __init__(self, index, tris, area3):
        self.index, self.tris, self.area3 = index, tris, area3


def _prepare(groups, size=None, floor=None):
    """groups: [(UV triangles (T, 3, 2), 3D area)] -> islands in even-density, box-aligned units (UV area = 3D area);
    island.least: the px per unit below which it would hold fewer texels than it did at size px (floor x that)."""
    out = []
    areas = [(t, a3, float(0.5 * np.abs(np.cross(t[:, 1] - t[:, 0], t[:, 2] - t[:, 0])).sum())) for t, a3 in groups]
    ratios = [math.sqrt(a3 / a2) for _, a3, a2 in areas if a2 > 1e-14 and a3 > 1e-14]
    typical = float(np.median(ratios)) if ratios else 1.0
    for i, (t, a3, a2) in enumerate(areas):
        f = math.sqrt(a3 / a2) if (a2 > 1e-14 and a3 > 1e-14) else typical
        isl = Island(i, t, a3)
        P = (t.reshape(-1, 2) - t.reshape(-1, 2).mean(0)) * f
        ang = _min_box_angle(P)
        c, s = math.cos(ang), math.sin(ang)
        isl.pts = P @ np.array([[c, s], [-s, c]])
        isl.scale = f
        isl.least = (floor * size / f) if (floor and size) else 0.0
        out.append(isl)
    return out


def _fft_places(free_f, mask, G):
    """Every place (y, x) where mask's cells miss the occupied grid (free_f: rfft2 of it): bool (G, G)."""
    h, w = mask.shape
    m = np.zeros((G, G))
    m[:h, :w] = mask
    corr = np.fft.irfft2(free_f * np.conj(np.fft.rfft2(m)), s=(G, G))
    ok = corr < 0.5
    ok[G - h + 1:, :] = False
    ok[:, G - w + 1:] = False
    return ok


def _sat_places(sat, h, w, G):
    """Every place where an h x w box is empty (sat: the occupied grid's summed-area table, (G + 1, G + 1))."""
    ok = np.zeros((G, G), bool)
    if h > G or w > G:
        return ok
    box = sat[h:, w:] - sat[:-h, w:] - sat[h:, :-w] + sat[:-h, :-w]
    ok[:G - h + 1, :G - w + 1] = box == 0
    return ok


def _runs(mask):
    """The mask's cells as row runs: [(row, first col, end col)]."""
    out = []
    for i, row in enumerate(mask):
        d = np.diff(np.concatenate([[0], row.astype(np.int8), [0]]))
        out += [(i, int(a), int(b)) for a, b in zip(np.flatnonzero(d == 1), np.flatnonzero(d == -1))]
    return out


def _run_first(S, runs, longest, maxfree, h, w, G, y_from, y_limit):
    """The first place (y, x), rows y_from..y_limit, where every run of the mask misses the occupied grid (S: its row
    prefix sums, (G, G + 1)); None if there is none. Only rows whose longest free run (maxfree) holds each mask row's
    longest run (longest) are tested, BAND candidate rows at a time."""
    X = G - w + 1
    y_max = min(G - h, y_limit)
    if y_max < y_from:
        return None
    ok = np.ones(y_max - y_from + 1, bool)
    for i, need in enumerate(longest):
        if need:
            ok &= maxfree[y_from + i:y_max + 1 + i] >= need
    ys = np.flatnonzero(ok) + y_from
    for c in range(0, len(ys), BAND):
        band = ys[c:c + BAND]
        bad = np.zeros((len(band), X), bool)
        for n, (i, a, b) in enumerate(runs):
            seg = S[band + i]
            bad |= (seg[:, b:b + X] - seg[:, a:a + X]) > 0
            if n % 16 == 15 and bad.all():
                break
        free = np.flatnonzero(~bad)
        if len(free):
            r, col = divmod(int(free[0]), X)
            return int(band[r]), col
    return None


def _try(isls, s, size, cell, gap, rotations):
    """All islands laid at px-per-unit s: [(island, quarter turns, cell origin, place (x, y))] or None if one misses."""
    G = size // cell
    occ = np.zeros((G, G), bool)
    S = np.zeros((G, G + 1), np.int32)          # row prefix sums of occ
    top = 0                                    # the first row with a free cell
    maxfree = np.full(G, G, np.int64)          # each row's longest free run
    shapes = []
    for isl in isls:
        options = []
        base = None
        for k in range(rotations):
            if k == 2 and base is not None and base < SMALL_TURNS:
                break                  # a small island tries 0 and 90 degrees only
            P = _turn(isl.pts, k) * max(s, isl.least)
            P -= P.min(0)
            mask, lo = _cells(P.reshape(-1, 3, 2) + cell * 0.5, cell)
            runs = _runs(mask)
            longest = np.zeros(mask.shape[0], np.int64)
            for r, a, b in runs:
                longest[r] = max(longest[r], b - a)
            options.append((k, mask, lo, runs, longest))
            base = int(mask.sum()) if base is None else base
        shapes.append(options)
    order = sorted(range(len(isls)), key=lambda i: (-int(shapes[i][0][1].sum()), i))
    placed = [None] * len(isls)
    for i in order:
        best = None
        free_f = None
        sat = None
        for k, mask, lo, runs, longest in shapes[i]:
            h, w = mask.shape
            if h > G or w > G:
                continue
            limit = best[0] if best else G
            pos = None
            if mask.mean() > 0.75 or mask.size <= SMALL_BOX:
                if sat is None:
                    sat = np.zeros((G + 1, G + 1), np.int64)
                    sat[1:, :] = S.cumsum(0)
                ok = _sat_places(sat, h, w, G)
                ok[:top] = False
                hit = np.flatnonzero(ok)
                pos = divmod(int(hit[0]), G) if len(hit) else None
            elif len(runs) <= RUN_FFT:
                pos = _run_first(S, runs, longest, maxfree, h, w, G, top, limit)
            else:
                if free_f is None:
                    free_f = np.fft.rfft2(occ.astype(np.float64))
                ok = _fft_places(free_f, mask, G)
                ok[:top] = False
                hit = np.flatnonzero(ok)
                pos = divmod(int(hit[0]), G) if len(hit) else None
            if pos is not None and (best is None or pos < (best[0], best[1])):
                best = (pos[0], pos[1], k, mask, lo)
        if best is None:
            return None
        y, x, k, mask, lo = best
        grown = _grow(mask, gap)
        y0, x0 = y - gap, x - gap
        gy0, gx0 = max(0, -y0), max(0, -x0)
        gy1 = grown.shape[0] - max(0, y0 + grown.shape[0] - G)
        gx1 = grown.shape[1] - max(0, x0 + grown.shape[1] - G)
        r0 = max(0, y0)
        r1 = r0 + gy1 - gy0
        occ[r0:r1, max(0, x0):max(0, x0) + gx1 - gx0] |= grown[gy0:gy1, gx0:gx1]
        S[r0:r1, 1:] = np.cumsum(occ[r0:r1], axis=1)
        for r in range(r0, r1):
            taken = np.flatnonzero(occ[r])
            maxfree[r] = int((np.diff(np.concatenate([[-1], taken, [G]])) - 1).max())
        while top < G and S[top, G] == G:
            top += 1
        placed[i] = (k, lo, (x, y))
    return placed


def pack(groups, size, pad, rotations=4, start=None, min_gain=1.0, floor=1.0, log=None):
    """groups: [(UV triangles (T, 3, 2) as they are, 3D area)] per island -> (the new UV triangles per island (same
    shapes), px per unit), or (None, px per unit) when even min_gain times the area at start doesn't fit. Islands keep
    pad px each: the gap between them is at least 2 x pad, and an island keeps pad px from the square's edge too (the
    texture wraps). start: the px per unit at which the islands cover what they covered before (Tripo's layout); the
    search runs from min_gain times that area up to START_HIGH times its scale. floor: no island ends with fewer than
    floor times the texels it had in groups' layout at size px (an island Tripo gave more than the even density keeps
    them; 0: every island even). The density evens out above the floor only."""
    cell = max(1, size // GRID)
    gap = max(1, int(math.ceil(2.0 * pad / cell)))
    isls = _prepare(groups, size, floor)
    total = sum(i.area3 for i in isls)
    full = size / math.sqrt(max(total, 1e-12))          # every island's area adds up to the whole square
    start = start or full * math.sqrt(0.5)
    lo_s = start * math.sqrt(min_gain)
    res = _try(isls, lo_s, size - 2 * pad, cell, gap, rotations)
    if log:
        log(f'pack: {len(isls)} islands, {lo_s:.1f} px/unit (x{min_gain} the area) {"fits" if res else "misses"}')
    if res is None:
        return None, lo_s
    best = (lo_s, res)
    hi_s = min(full, start * START_HIGH)
    for step in range(FIT_STEPS):
        s = math.sqrt(lo_s * hi_s)
        res = _try(isls, s, size - 2 * pad, cell, gap, rotations)
        if res is None:
            hi_s = s
        else:
            lo_s, best = s, (s, res)
        if log:
            log(f'pack try {step}: {s:.1f} px/unit {"fits" if res else "misses"}')
    s, res = best
    out = []
    for isl, (k, lo, (x, y)) in zip(isls, res):
        P = _turn(isl.pts, k) * max(s, isl.least)
        P -= P.min(0)
        P = P + cell * 0.5 - lo * cell + np.array([x, y]) * cell + pad
        out.append(np.column_stack([P[:, 0] / size, 1.0 - P[:, 1] / size]).reshape(-1, 3, 2))
    return out, s
