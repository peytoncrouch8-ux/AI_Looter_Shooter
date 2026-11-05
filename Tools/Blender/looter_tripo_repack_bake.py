"""The repack's re-cut (looter_tripo_repack uses it; Docs/Work/2026-11-04-Art/REPACK.md): the GLB's 4096 maps copied onto
a new UV layout of the same mesh, texel by texel.

transfer(): each new texel's triangle and barycentrics carry it to its old place, where the source is box-filtered over
the texel's footprint (k x k point taps, k = 2 x the source px it spans, up to MAX_TAPS; a magnified texel one bilinear
sample). Colour and ORM as they are; the tangent-space normal decoded in the old UVs' MikkTSpace frame, encoded in the
new one's (_tangent_frames: a scratch mesh of the loop triangles). source_maps() spreads a source's islands DILATE px into
dark padding first (Tripo's own maps already are spread, 1-6 px).
pad(): each island's border spread its own texels out, the rest of the square push-pull filled.
bleed(): per mip level, the share of island texels whose mip texel also holds another island's padding or the fill.
_image(): a packed byte image of an array, so write_maps / write_set treat it as they treat the GLB's.
"""
import bpy
import numpy as np

import looter_tripo_repack_pack as rpk
from looter_tripo_repack_pack import _uv_layer, loop_triangles

MAX_TAPS = 8           # taps a side at most (a texel spanning 4 source px or more)
DILATE = 6             # px at the source's size: how far its islands' texels are spread into dark padding


# --- Transfer ---

def _pixels_u8(image):
    """An image's stored pixels as uint8 RGBA rows top first (Blender's byte buffer as it is)."""
    w, h = image.size
    buf = np.empty(w * h * 4, np.float32)
    image.pixels.foreach_get(buf)
    out = np.clip(buf * 255.0 + 0.5, 0, 255).astype(np.uint8).reshape(h, w, 4)[::-1]
    del buf
    return np.ascontiguousarray(out)


def _bilinear(src, x, y):
    """src (H, W, C) uint8 rows top first, sampled at pixel coords x (col), y (row) (texel centres at +0.5), clamped:
    float32 (n, C)."""
    h, w = src.shape[:2]
    x = x - 0.5
    y = y - 0.5
    x0 = np.floor(x)
    y0 = np.floor(y)
    fx = (x - x0)[:, None].astype(np.float32)
    fy = (y - y0)[:, None].astype(np.float32)
    x0 = x0.astype(np.int64)
    y0 = y0.astype(np.int64)
    x1 = np.clip(x0 + 1, 0, w - 1)
    y1 = np.clip(y0 + 1, 0, h - 1)
    x0 = np.clip(x0, 0, w - 1)
    y0 = np.clip(y0, 0, h - 1)
    a = src[y0, x0].astype(np.float32)
    b = src[y0, x1].astype(np.float32)
    c = src[y1, x0].astype(np.float32)
    d = src[y1, x1].astype(np.float32)
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def _nearest(src, x, y):
    """src (H, W, C) uint8 rows top first, the texel under pixel coords x (col), y (row), clamped: float32 (n, C)."""
    h, w = src.shape[:2]
    xi = np.clip(np.floor(x).astype(np.int64), 0, w - 1)
    yi = np.clip(np.floor(y).astype(np.int64), 0, h - 1)
    return src[yi, xi].astype(np.float32)


def _tangent_frames(me, L, P, uvs):
    """MikkTSpace frames per triangle corner for each per-loop UV array in uvs ({name: uv}): {name: (tangent (T, 3, 3),
    bitangent sign (T, 3), normal (T, 3, 3))}, on a scratch mesh made of the loop triangles L (corner for corner, so
    n-gons are fine; Unreal runs MikkTSpace on the triangulated mesh too), the polygons' smooth flags and sharp edges
    kept by vertex pair."""
    nT = len(L)
    co = np.empty(len(me.vertices) * 3, np.float32)
    me.vertices.foreach_get('co', co)
    lv = np.empty(len(me.loops), np.int64)
    me.loops.foreach_get('vertex_index', lv)
    smooth = np.ones(len(me.polygons), bool)
    if 'sharp_face' in me.attributes:
        sf = np.empty(len(me.polygons), bool)
        me.attributes['sharp_face'].data.foreach_get('value', sf)
        smooth = ~sf
    tm = bpy.data.meshes.new('_RepackTangents')
    tm.vertices.add(len(me.vertices))
    tm.vertices.foreach_set('co', co)
    tm.loops.add(nT * 3)
    tm.loops.foreach_set('vertex_index', lv[L].ravel().astype(np.int32))
    tm.polygons.add(nT)
    tm.polygons.foreach_set('loop_start', (np.arange(nT) * 3).astype(np.int32))
    tm.update(calc_edges=True)
    tm.polygons.foreach_set('use_smooth', smooth[P])
    if 'sharp_edge' in me.attributes:
        ev = np.empty(len(me.edges) * 2, np.int64)
        me.edges.foreach_get('vertices', ev)
        se = np.empty(len(me.edges), bool)
        me.attributes['sharp_edge'].data.foreach_get('value', se)
        sharp = {tuple(sorted(e)) for e in ev.reshape(-1, 2)[se].tolist()}
        if sharp:
            tv = np.empty(len(tm.edges) * 2, np.int64)
            tm.edges.foreach_get('vertices', tv)
            flags = np.array([tuple(sorted(e)) in sharp for e in tv.reshape(-1, 2).tolist()], bool)
            attr = tm.attributes.get('sharp_edge') or tm.attributes.new('sharp_edge', 'BOOLEAN', 'EDGE')
            attr.data.foreach_set('value', flags)
    out = {}
    for name, uv in uvs.items():
        lay = tm.uv_layers.new(name=name)
        lay.data.foreach_set('uv', np.asarray(uv, np.float32)[L].ravel())
    for name in uvs:
        tm.calc_tangents(uvmap=name)
        T = np.empty(nT * 9, np.float32)
        S = np.empty(nT * 3, np.float32)
        N = np.empty(nT * 9, np.float32)
        tm.loops.foreach_get('tangent', T)
        tm.loops.foreach_get('bitangent_sign', S)
        tm.loops.foreach_get('normal', N)
        tm.free_tangents()
        out[name] = (T.reshape(nT, 3, 3), S.reshape(nT, 3), N.reshape(nT, 3, 3))
    bpy.data.meshes.remove(tm)
    return out


def _frame(T, S, N, t, bary):
    """The orthonormal tangent frame at texels (triangle t, barycentrics): (tangent, bitangent, normal) (n, 3) each."""
    n = np.einsum('ij,ijk->ik', bary, N[t])
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-9)
    tg = np.einsum('ij,ijk->ik', bary, T[t])
    tg -= n * np.sum(tg * n, axis=1, keepdims=True)
    tg /= np.maximum(np.linalg.norm(tg, axis=1, keepdims=True), 1e-9)
    sign = np.where(S[t, 0] < 0.0, -1.0, 1.0).astype(np.float32)[:, None]
    bt = sign * np.cross(n, tg)
    return tg, bt, n


def source_maps(maps, uv_tris, dilate=DILATE):
    """The GLB's maps as uint8 arrays (rows top first) at the source size, each island's texels spread dilate px into
    the black padding round it (uv_tris: the old UV triangles of what is being re-cut): {'BC', 'RM', 'N'}: (S, S, 3)."""
    out = {}
    mask = None
    for key in ('BC', 'RM', 'N'):
        image = maps.get(key)
        if image is None:
            continue
        a = _pixels_u8(image)[..., :3]
        if mask is None or mask.shape != a.shape[:2]:
            mask, _ = rpk.coverage(uv_tris, a.shape[0])
            ring = _ring(mask, 3)
        # Tripo spreads its islands into its padding itself (the 2026-11-04 survey: 1-6 px out, 0.9 of the islands'
        # brightness); a map whose first 3 px out are dark gets it done here.
        inside = float(a[mask].mean()) if mask.any() else 0.0
        near = float(a[ring].mean()) if ring.any() else inside
        out[key] = _dilate_u8(a, mask, dilate) if near < 0.8 * inside else a
    return out, mask


def _ring(mask, steps):
    grown = mask.copy()
    for _ in range(steps):
        grown = grown | np.roll(grown, 1, 0) | np.roll(grown, -1, 0) | np.roll(grown, 1, 1) | np.roll(grown, -1, 1)
    return grown & ~mask


def _dilate_u8(a, known, steps):
    """uint8 image a: texels not known take the mean of their known 8 neighbours, steps rings deep (in strips, so a
    4096 map never needs more than a few hundred MB)."""
    h = a.shape[0]
    out = a.copy()
    k = known.copy()
    for _ in range(steps):
        new_k = k.copy()
        strip = 512
        for y0 in range(0, h, strip):
            y1 = min(h, y0 + strip)
            lo, hi = max(0, y0 - 1), min(h, y1 + 1)
            sub = out[lo:hi].astype(np.float32)
            sk = k[lo:hi]
            acc = np.zeros_like(sub)
            cnt = np.zeros(sk.shape, np.float32)
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    if dx or dy:
                        kk = np.roll(np.roll(sk, dy, 0), dx, 1)
                        acc += np.roll(np.roll(sub, dy, 0), dx, 1) * kk[..., None]
                        cnt += kk
            fresh = (~sk) & (cnt > 0)
            r0, r1 = y0 - lo, y0 - lo + (y1 - y0)
            f = fresh[r0:r1]
            vals = acc[r0:r1][f] / cnt[r0:r1][f][:, None]
            block = out[y0:y1]
            block[f] = np.clip(vals + 0.5, 0, 255).astype(np.uint8)
            new_k[y0:y1] |= f
        k = new_k
    return out


def transfer(obj, old_uv, new_uv, maps, size, polys=None, layer=None, src=None):
    """The source maps re-cut onto the new layout at size px: {'BC', 'RM', 'N'} float32 (size, size, 3) in the stored
    0..255 range (normals: OpenGL, tangent space of the new UVs), the covered texels (bool) and each texel's island
    triangle (int, -1 uncovered). Exact for the same mesh: barycentrics carry each texel to its old place."""
    me = obj.data
    L, P = loop_triangles(me)
    if polys is not None:
        sel = np.isin(P, polys)
        L, P = L[sel], P[sel]
    Uo, Un = old_uv[L], new_uv[L]
    if src is None:
        src, _ = source_maps(maps, Uo)
    S = next(iter(src.values())).shape[0]
    t, r, c, bary = rpk.raster(Un, size)
    # Each texel once: the last triangle claiming it (a centre on a shared edge, the same colour either side).
    flat = r * size + c
    _, first = np.unique(flat[::-1], return_index=True)
    pick = len(flat) - 1 - first
    t, r, c, bary, flat = t[pick], r[pick], c[pick], bary[pick], flat[pick]
    op = np.stack([Uo[..., 0] * S, (1.0 - Uo[..., 1]) * S], -1)          # old, source px (x col, y row)
    npx = np.stack([Un[..., 0] * size, (1.0 - Un[..., 1]) * size], -1)    # new, target px
    En = np.stack([npx[:, 1] - npx[:, 0], npx[:, 2] - npx[:, 0]], -1)   # (T, 2, 2): columns are edges
    Eo = np.stack([op[:, 1] - op[:, 0], op[:, 2] - op[:, 0]], -1)
    det = En[:, 0, 0] * En[:, 1, 1] - En[:, 0, 1] * En[:, 1, 0]
    ok = np.abs(det) > 1e-12
    inv = np.zeros_like(En)
    inv[ok, 0, 0] = En[ok, 1, 1] / det[ok]
    inv[ok, 1, 1] = En[ok, 0, 0] / det[ok]
    inv[ok, 0, 1] = -En[ok, 0, 1] / det[ok]
    inv[ok, 1, 0] = -En[ok, 1, 0] / det[ok]
    J = Eo @ inv                                                          # d(old px)/d(new px)
    scale = np.sqrt(np.abs(J[:, 0, 0] * J[:, 1, 1] - J[:, 0, 1] * J[:, 1, 0]))
    # A box filter over each new texel's footprint in the source: k x k point taps (k = 2 x the source px a texel
    # spans, so taps fall under half a source px apart), as the old 4096 -> 2048 / 1024 halving was a clean box. Taps
    # of bilinear samples instead (2 x 2 at 1.56 source px a texel) blurred each texel by about half a texel more:
    # the repacked farmhouse wall came out softer at 1 m than Tripo's own layout (2026-11-04). Magnified texels
    # (a source px or more each way) take one bilinear sample.
    taps = np.where(scale <= 1.0, 1, np.clip(np.ceil(2.0 * scale), 2, MAX_TAPS)).astype(np.int64)
    centre = np.einsum('ij,ijk->ik', bary, op[t])
    out = {k: np.zeros((size * size, 3), np.float32) for k in src}
    for k in range(1, MAX_TAPS + 1):
        idx = np.nonzero(taps[t] == k)[0]
        if not len(idx):
            continue
        offs = (np.arange(k) + 0.5) / k - 0.5
        ox, oy = np.meshgrid(offs, offs)
        ox, oy = ox.ravel(), oy.ravel()
        step = max(1, (1 << 21) // (k * k))
        for s in range(0, len(idx), step):
            i = idx[s:s + step]
            Ji = J[t[i]]
            px_ = centre[i, 0][:, None] + Ji[:, 0, 0][:, None] * ox[None, :] + Ji[:, 0, 1][:, None] * oy[None, :]
            py_ = centre[i, 1][:, None] + Ji[:, 1, 0][:, None] * ox[None, :] + Ji[:, 1, 1][:, None] * oy[None, :]
            for key, a in src.items():
                if k == 1:
                    v = _bilinear(a, px_.ravel(), py_.ravel())
                else:
                    v = _nearest(a, px_.ravel(), py_.ravel()).reshape(len(i), k * k, -1).mean(1)
                out[key][flat[i]] = v
    if 'N' in out:
        # Tangent space: decoded in the old UVs' frame, encoded in the new one's (same mesh, same normals).
        frames = _tangent_frames(me, L, P, {'old': old_uv, 'new': new_uv})
        To, So, No = frames['old']
        Tn, Sn, Nn = frames['new']
        v = out['N'][flat] / 255.0 * 2.0 - 1.0
        step = 1 << 20
        for s in range(0, len(flat), step):
            sl = slice(s, s + step)
            tg, bt, nn = _frame(To, So, No, t[sl], bary[sl])
            w = v[sl, 0:1] * tg + v[sl, 1:2] * bt + v[sl, 2:3] * nn
            w /= np.maximum(np.linalg.norm(w, axis=1, keepdims=True), 1e-9)
            tg, bt, nn = _frame(Tn, Sn, Nn, t[sl], bary[sl])
            e = np.stack([np.sum(w * tg, 1), np.sum(w * bt, 1), np.sum(w * nn, 1)], 1)
            out['N'][flat[sl]] = (e * 0.5 + 0.5) * 255.0
    known = np.zeros(size * size, bool)
    known[flat] = True
    tri = np.full(size * size, -1, np.int64)
    tri[flat] = t
    shaped = {k: v.reshape(size, size, 3) for k, v in out.items()}
    return shaped, known.reshape(size, size), tri.reshape(size, size), dict(taps=np.bincount(taps[t], minlength=5)[1:].tolist(),
                                                                           scale=round(float(np.median(scale[t])), 2))


# --- Padding ---

def _dilate_f(data, known, steps):
    data = data.copy()
    known = known.copy()
    for _ in range(steps):
        acc = np.zeros_like(data)
        cnt = np.zeros(known.shape, np.float32)
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dx or dy:
                    k = np.roll(np.roll(known, dy, 0), dx, 1)
                    acc += np.roll(np.roll(data, dy, 0), dx, 1) * k[..., None]
                    cnt += k
        new = (~known) & (cnt > 0)
        if not new.any():
            break
        data[new] = acc[new] / cnt[new][:, None]
        known |= new
    return data, known


def _push_pull(data, known):
    """Unknown texels filled smooth from the known ones (a pyramid of known-weighted means, filled coarse to fine)."""
    levels = [(data * known[..., None], known.astype(np.float32))]
    while levels[-1][1].shape[0] > 1:
        d, w = levels[-1]
        h = d.shape[0] // 2
        d2 = d.reshape(h, 2, h, 2, -1).sum((1, 3))
        w2 = w.reshape(h, 2, h, 2).sum((1, 3))
        levels.append((d2, w2))
    filled = levels[-1][0] / np.maximum(levels[-1][1][..., None], 1e-9)
    for d, w in reversed(levels[:-1]):
        up = np.repeat(np.repeat(filled, 2, 0), 2, 1)
        mean = d / np.maximum(w[..., None], 1e-9)
        filled = np.where(w[..., None] > 0, mean, up)
    out = data.copy()
    out[~known] = filled[~known]
    return out


def pad(data, known, steps):
    """Each island's border spread steps px outward from its own texels, the rest of the square filled smooth."""
    d, k = _dilate_f(data, known, steps)
    return _push_pull(d, k)


def bleed(tri_island, known, steps, levels=(1, 2, 3, 4, 5)):
    """Mip bleed: per mip level, the share of island texels whose mip texel (a 2^k block) also holds texels of another
    island's padding or of the free fill (tri_island: each texel's island, -1 uncovered)."""
    size = tri_island.shape[0]
    terr = np.where(known, tri_island, -1).astype(np.int64)
    for _ in range(steps):
        grown = terr.copy()
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)):
            nb = np.roll(np.roll(terr, dy, 0), dx, 1)
            take = (grown < 0) & (nb >= 0)
            grown[take] = nb[take]
        terr = grown
    out = {}
    for k in levels:
        f = 1 << k
        if f > size:
            break
        b = terr.reshape(size // f, f, size // f, f)
        lo = b.min((1, 3))
        hi = b.max((1, 3))
        clean = (lo == hi)
        own = known.reshape(size // f, f, size // f, f).sum((1, 3))
        out[k] = round(float((own * ~clean).sum()) / max(int(known.sum()), 1), 4)
    return out


# --- Images ---

def _image(name, data, colour):
    """A byte image of data ((H, W, 3) floats 0..255, rows top first), packed so copies keep its pixels."""
    old = bpy.data.images.get(name)
    if old:
        bpy.data.images.remove(old)
    h, w = data.shape[:2]
    img = bpy.data.images.new(name, w, h, alpha=False)
    img.colorspace_settings.name = 'sRGB' if colour else 'Non-Color'
    rgba = np.ones((h, w, 4), np.float32)
    rgba[..., :3] = np.clip(np.round(data), 0, 255) / 255.0
    img.pixels.foreach_set(rgba[::-1].ravel())
    img.pack()
    return img
