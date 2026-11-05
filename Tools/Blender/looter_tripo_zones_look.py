"""Helpers of looter_tripo_zones (Tripo models repainted in our looks zone by zone): colour spaces, image filters on a
mask, our tiled texture sets sampled in model space (tri-planar, at the mip a texel's footprint asks for), each face's UV
stretch, the Cycles bakes on the mesh's atlas (face values, each texel's place, normal and pointiness), face thickness, each
texel's tangent frame, 3D zone votes, wear and relief, and writing texels into a map image so a
copy of it (write_maps shrinks one) keeps them (and a mask beside it for its alpha)."""
import bpy
import numpy as np

import looter_rebake as rb
import looter_textures as lt
import looter_tripo as tripo

LUMA = np.array([0.2126, 0.7152, 0.0722], np.float32)


def lin(c):
    c = np.asarray(c, np.float32)
    return np.where(c <= 0.04045, c / 12.92, ((np.maximum(c, 0.0) + 0.055) / 1.055) ** 2.4).astype(np.float32)


def srgb(c):
    return rb._linear_to_srgb(c).astype(np.float32)


def hex_rgb(v):
    """0xRRGGBB as sRGB 0..1."""
    return np.array([(v >> 16) & 255, (v >> 8) & 255, v & 255], np.float32) / 255.0


def oklab(s):
    """sRGB (..., 3) to OKLab (L, a, b)."""
    r, g, b = np.moveaxis(lin(s), -1, 0)
    l = np.cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b)
    m = np.cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b)
    q = np.cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b)
    return np.stack([0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * q,
                     1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * q,
                     0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * q], -1).astype(np.float32)


def box(a, r):
    """Sums over a (2r+1)^2 window (zero outside the image)."""
    c = np.pad(a, ((r + 1, r), (r + 1, r))).cumsum(0).cumsum(1)
    k = 2 * r + 1
    return c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]


def masked_blur(a, mask, sigma):
    """a blurred inside mask only (each texel the mean of the mask's texels round it), a's own value outside."""
    m = mask.astype(np.float32)
    w = np.maximum(lt.blur(m, sigma), 1e-6)
    if a.ndim == 3:
        s = lt.blur(a * m[..., None], sigma) / w[..., None]
        return np.where(mask[..., None], s, a).astype(np.float32)
    return np.where(mask, lt.blur(a * m, sigma) / w, a).astype(np.float32)


class Set:
    """A tiled texture set's maps as mip pyramids (BC linear, ORM, N with +y up the picture), sampled by texel."""

    def __init__(self, name):
        self.name = name
        self.levels = {}
        for kind in ('BC', 'ORM', 'N'):
            img = bpy.data.images.load(lt.texture_path(name, kind), check_existing=False)
            img.colorspace_settings.name = 'sRGB' if kind == 'BC' else 'Non-Color'
            a = np.ascontiguousarray(tripo.pixels(img)[..., :3], dtype=np.float32)
            bpy.data.images.remove(img)
            if kind == 'BC':
                a = lin(a)
            elif kind == 'N':
                a = a * 2.0 - 1.0
                a[..., 1] = -a[..., 1]              # the sets' maps are DirectX (green down)
            pyr = [a]
            while pyr[-1].shape[0] > 4:
                b = pyr[-1]
                pyr.append((b[0::2, 0::2] + b[1::2, 0::2] + b[0::2, 1::2] + b[1::2, 1::2]) * 0.25)
            self.levels[kind] = pyr
        self.mean = self.levels['BC'][0].reshape(-1, 3).mean(0)
        self.size = self.levels['BC'][0].shape[0]

    def sample(self, kind, u, v, lod):
        """Trilinear: u, v in the set's pixels at level 0 (wrapping), lod the fractional mip."""
        pyr = self.levels[kind]
        lod = np.clip(lod, 0.0, len(pyr) - 1.001)
        l0 = np.floor(lod).astype(np.int64)
        t = (lod - l0)[:, None].astype(np.float32)
        out = np.zeros((len(u), 3), np.float32)
        for l in np.unique(l0):
            sel = l0 == l
            a = _bilinear(pyr[l], u[sel] / 2.0 ** l, v[sel] / 2.0 ** l)
            b = _bilinear(pyr[l + 1], u[sel] / 2.0 ** (l + 1), v[sel] / 2.0 ** (l + 1))
            out[sel] = a * (1.0 - t[sel]) + b * t[sel]
        return out


def _bilinear(img, x, y):
    h, w = img.shape[:2]
    x, y = x - 0.5, y - 0.5
    x0, y0 = np.floor(x).astype(np.int64), np.floor(y).astype(np.int64)
    fx, fy = (x - x0)[:, None].astype(np.float32), (y - y0)[:, None].astype(np.float32)
    x0, y0 = x0 % w, y0 % h
    x1, y1 = (x0 + 1) % w, (y0 + 1) % h
    return ((img[y0, x0] * (1 - fx) + img[y0, x1] * fx) * (1 - fy) + (img[y1, x0] * (1 - fx) + img[y1, x1] * fx) * fy)


# Tri-planar projections in model space (Blender metres): per axis the picture's u and v (v runs down its rows) axes, so a
# side's picture stands upright (MetalRust's runs fall down the wall), and offsets (in tiles) so the three never line up.
PROJ = [((0.0, 1.0, 0.0), (0.0, 0.0, -1.0), (0.37, 0.11)), ((1.0, 0.0, 0.0), (0.0, 0.0, -1.0), (0.71, 0.53)),
        ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.19, 0.83))]


def triplanar(s, kind, pos, nrm, foot, density, axes=None):
    """The set's kind at each texel (pos m, nrm unit, foot m per texel; density px per m): (texels, 3); for 'N' a
    model-space normal. axes: three (u, v, offset) projections in place of PROJ (a wood's grain along a hull)."""
    tile_m = s.size / density
    w = np.abs(nrm) ** 4
    w /= np.maximum(w.sum(1, keepdims=True), 1e-9)
    lod = np.log2(np.maximum(foot * density, 1.0))
    out = np.zeros((len(pos), 3), np.float32)
    for a, (ua, va, off) in enumerate(axes or PROJ):
        sel = w[:, a] > 0.01
        if not sel.any():
            continue
        uax, vax = np.array(ua, np.float32), np.array(va, np.float32)
        u = (pos[sel] @ uax + off[0] * tile_m) * density
        v = (pos[sel] @ vax + off[1] * tile_m) * density
        t = s.sample(kind, u, v, lod[sel])
        if kind == 'N':
            # The map's x leans the normal along +u, its y up the picture (-v), its z along the texel's own normal.
            t = nrm[sel] * t[:, 2:3] + uax[None] * t[:, 0:1] - vax[None] * t[:, 1:2]
        out[sel] += t * w[sel, a:a + 1]
    if kind == 'N':
        out /= np.maximum(np.linalg.norm(out, axis=1, keepdims=True), 1e-9)
    return out


# --- The mesh ---

def stretch(me):
    """Each face's UV stretch: the ratio of the singular values of its first triangle's 3D -> UV map (1: none)."""
    n = len(me.polygons)
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers.active.data.foreach_get('uv', uv)
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    lv = np.empty(len(me.loops), np.int64)
    me.loops.foreach_get('vertex_index', lv)
    ls = np.empty(n, np.int64)
    me.polygons.foreach_get('loop_start', ls)
    loops = ls[:, None] + np.arange(3)
    p, u = co.reshape(-1, 3)[lv[loops]], uv.reshape(-1, 2)[loops]
    e1, e2 = p[:, 1] - p[:, 0], p[:, 2] - p[:, 0]
    ax = e1 / np.maximum(np.linalg.norm(e1, axis=1, keepdims=True), 1e-9)
    nrm = np.cross(e1, e2)
    ay = np.cross(nrm / np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-12), ax)
    q = np.stack([np.stack([(e1 * ax).sum(1), (e1 * ay).sum(1)], 1), np.stack([(e2 * ax).sum(1), (e2 * ay).sum(1)], 1)], 1)
    t = np.stack([u[:, 1] - u[:, 0], u[:, 2] - u[:, 0]], 1)
    det = q[:, 0, 0] * q[:, 1, 1] - q[:, 0, 1] * q[:, 1, 0]
    ok = np.abs(det) > 1e-12
    d = np.where(ok, det, 1.0)
    qi = np.stack([np.stack([q[:, 1, 1], -q[:, 0, 1]], 1), np.stack([-q[:, 1, 0], q[:, 0, 0]], 1)], 1) / d[:, None, None]
    sv = np.linalg.svd(np.einsum('nij,njk->nik', qi, t), compute_uv=False)
    return np.where(ok, sv[:, 0] / np.maximum(sv[:, 1], 1e-12), 99.0)


# --- Bakes ---

def bake_faces(obj, values, size, margin):
    """values (faces, 3) as an emission bake on UV0 (rows top first, RGB); margin px of Cycles' extend round islands."""
    me = obj.data
    attr = me.attributes.new('_paint_bake', 'FLOAT_COLOR', 'FACE')
    rgba = np.concatenate([values.astype(np.float32), np.ones((len(values), 1), np.float32)], 1)
    attr.data.foreach_set('color', rgba.ravel())
    img = rb._image('_paint_bake', size)
    rb._fill(img, (0.0, 0.0, 0.0, 0.0))
    mat = bpy.data.materials.new('_paint_bake')
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    at = nodes.new('ShaderNodeAttribute')
    at.attribute_name = '_paint_bake'
    em = nodes.new('ShaderNodeEmission')
    links.new(at.outputs['Color'], em.inputs['Color'])
    links.new(em.outputs['Emission'], out.inputs['Surface'])
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = img
    nodes.active = tex
    kept = [s.material for s in obj.material_slots]
    if not kept:
        me.materials.append(mat)
    for s in obj.material_slots:
        s.material = mat
    rb._scene().cycles.samples = 1
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.bake(type='EMIT', margin=margin, margin_type='EXTEND', use_clear=False, target='IMAGE_TEXTURES')
    if kept:
        for s, m in zip(obj.material_slots, kept):
            s.material = m
    else:
        me.materials.clear()
    out = rb._read(img)[..., :3]
    bpy.data.images.remove(img)
    bpy.data.materials.remove(mat)
    me.attributes.remove(me.attributes['_paint_bake'])
    return out


def bake_own(obj, size):
    """Each texel's place (m) and normal (model space), and Cycles' pointiness; rows top first."""
    had = len(obj.material_slots) > 0
    out = []
    for kind in ('POSITION', 'NORMAL'):
        img = rb._image('_paint_' + kind, size)
        rb._fill(img, (0.5, 0.5, 0.5, 1.0))
        rb._bake(obj, None, img, kind, 0.0, 0.0, False)
        out.append(((rb._read(img)[..., :3] - 0.5) * 2.0).astype(np.float32))
        bpy.data.images.remove(img)
    out.append(rb.pointiness(obj, size))
    if not had:
        obj.data.materials.clear()   # rb._bake leaves an empty slot on a mesh that had none
    return out


# --- Writing into a map image ---

def faces_only(obj, keep):
    """A temporary copy of obj holding only the faces keep marks (bool per face), linked to the scene: what bakes see
    when those faces have a UV square of their own (other faces' UVs would land on the same texels)."""
    import bmesh
    me = obj.data.copy()
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if not keep[f.index]], context='FACES')
    bm.to_mesh(me)
    bm.free()
    tmp = bpy.data.objects.new('_PaintFacesOnly', me)
    bpy.context.scene.collection.objects.link(tmp)
    tmp.matrix_world = obj.matrix_world.copy()
    return tmp


def grow(seed, free, steps):
    """seed (an int image: each texel's source index, -1 none) grown steps rings into the free texels, each taking a
    source next to it (the nearest within a ring)."""
    src = seed.copy()
    for _ in range(steps):
        grown = src.copy()
        for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0), (1, 1), (1, -1), (-1, 1), (-1, -1)):
            nb = np.roll(np.roll(src, dy, 0), dx, 1)
            take = (grown < 0) & free & (nb >= 0)
            grown[take] = nb[take]
        src = grown
    return src


def attach_alpha(img, widx, values, size):
    """values (0..1) at widx as the colour map's alpha mask: an image beside img (AlphaMask: its name, a grey Non-Color
    image of img's size, 1 where unwritten) that write_maps adds to the PNG. Not in img itself: Blender premultiplies a
    colour image's alpha when it copies, scales or saves it, and the colour went black wherever the mask is 0."""
    name = img.get('AlphaMask') or img.name + '_AlphaMask'
    mask = bpy.data.images.get(name)
    if mask is None or tuple(mask.size) != tuple(img.size):
        if mask is not None:
            bpy.data.images.remove(mask)
        mask = bpy.data.images.new(name, img.size[0], img.size[1], alpha=False)
        mask.colorspace_settings.name = 'Non-Color'
        _fill(mask, 1.0)
    rows = _rows(mask)
    at, v = _fit(img, widx, np.asarray(values, np.float32)[:, None], size)
    rows[at[0], at[1], :3] = np.clip(v, 0.0, 1.0)
    _store(mask, rows)
    img['AlphaMask'] = mask.name
    return mask


def _fill(img, value):
    img.pixels.foreach_set(np.full(img.size[0] * img.size[1] * 4, value, np.float32))


def _rows(img):
    """img's pixels at its own size, rows top first (RGBA float)."""
    w, h = img.size
    buf = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(buf)
    return buf.reshape(h, w, 4)[::-1].copy()


def _store(img, rows):
    img.pixels.foreach_set(np.ascontiguousarray(rows[::-1]).ravel())
    img.update()
    img.pack()       # write_maps reads a copy of the image, made from its packed data: pack the edited pixels


def _fit(img, widx, values, size):
    """values at widx (texels of the size x size work grid) on img's own grid (box-averaged when img is smaller, each
    work texel on its f x f block when img is larger)."""
    w = img.size[0]
    if w == size:
        return widx, values
    if w > size and w % size == 0:
        f = w // size
        dy, dx = np.divmod(np.arange(f * f), f)
        rows = (widx[0][:, None] * f + dy[None]).ravel()
        cols = (widx[1][:, None] * f + dx[None]).ravel()
        return (rows, cols), np.repeat(values, f * f, axis=0)
    f = size // w
    if f < 1 or size % w:
        raise ValueError(f'rebake: map {img.name} is {w} px, the work grid {size}: not a whole fraction')
    key = (widx[0] // f) * w + widx[1] // f
    uk, inv = np.unique(key, return_inverse=True)
    acc = np.zeros((len(uk), values.shape[1]), np.float64)
    np.add.at(acc, inv, values)
    return (uk // w, uk % w), (acc / np.bincount(inv)[:, None]).astype(np.float32)


def write_raw(img, widx, values, size):
    rows = _rows(img)
    at, v = _fit(img, widx, values, size)
    rows[at[0], at[1], :3] = np.clip(v, 0.0, 1.0)
    _store(img, rows)


def write_bc(img, widx, lin, size, grade, unsharp):
    """The walls' colour (linear) into the colour map (sRGB), undoing write_maps' grade: contrast and saturation
    (pointwise), then its unsharp mask (softened beforehand by amount / (1 + amount) at its radius, so the sharpening
    gives back about what was computed instead of haloing every border)."""
    sat, contrast = grade
    s = 0.5 + (srgb(lin) - 0.5) / contrast
    l601 = (s @ np.array([0.299, 0.587, 0.114], np.float32))[:, None]
    s = l601 + (s - l601) / sat
    rows = _rows(img)
    at, v = _fit(img, widx, s, size)
    rows[at[0], at[1], :3] = v
    sigma, amount = unsharp
    soft = lt.blur(rows[..., :3], sigma * img.size[0] / 1024.0)[at[0], at[1]]
    b = amount / (1.0 + amount)
    rows[at[0], at[1], :3] = np.clip(v - b * (v - soft), 0.0, 1.0)
    _store(img, rows)



# --- Texels ---

def thickness(obj, reach=2.0):
    """Each face's thickness (m, at most reach): a ray in from its middle to the far side of the skin (a rope, a rail or
    a strut is thin; a hull, a bag or a wall thick)."""
    from mathutils.bvhtree import BVHTree
    me = obj.data
    n = len(me.polygons)
    c, nn = np.empty(n * 3), np.empty(n * 3)
    me.polygons.foreach_get('center', c)
    me.polygons.foreach_get('normal', nn)
    c, nn = c.reshape(-1, 3), nn.reshape(-1, 3)
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    tree = BVHTree.FromPolygons([tuple(v) for v in co.reshape(-1, 3)], [tuple(p.vertices) for p in me.polygons])
    out = np.full(n, reach, np.float32)
    for f in range(n):
        d = -nn[f]
        hit = tree.ray_cast(tuple(c[f] + d * 0.002), tuple(d), reach)
        if hit[0] is not None:
            out[f] = hit[3]
    return out


def face_values(obj, attr, channel):
    """Each face's value of a mesh attribute (a colour attribute's channel: the stored bytes of a byte colour, as the
    exporter writes them; a corner or point attribute averaged over the face); '_thickness': thickness()."""
    me = obj.data
    if attr == '_thickness':
        return thickness(obj)
    a = me.color_attributes.get(attr) or me.attributes.get(attr)
    if a is None:
        raise KeyError(f'{obj.name}: no attribute {attr}')
    n = {'FACE': len(me.polygons), 'CORNER': len(me.loops), 'POINT': len(me.vertices)}[a.domain]
    if a.data_type in ('BYTE_COLOR', 'FLOAT_COLOR'):
        buf = np.empty(n * 4, np.float32)
        a.data.foreach_get('color_srgb' if a.data_type == 'BYTE_COLOR' else 'color', buf)
        vals = buf.reshape(-1, 4)[:, channel]
    else:
        vals = np.empty(n, np.float32)
        a.data.foreach_get('value', vals)
    if a.domain == 'FACE':
        return vals
    ls = np.empty(len(me.polygons), np.int64)
    lc = np.empty(len(me.polygons), np.int64)
    me.polygons.foreach_get('loop_start', ls)
    me.polygons.foreach_get('loop_total', lc)
    if a.domain == 'POINT':
        lv = np.empty(len(me.loops), np.int64)
        me.loops.foreach_get('vertex_index', lv)
        vals = vals[lv]
    sums = np.add.reduceat(vals, ls)
    return (sums / lc).astype(np.float32)


def upsample(a, size):
    """a (rows top first) to size px by repetition (size a whole multiple of a's)."""
    f = size // a.shape[0]
    return a if f == 1 else np.repeat(np.repeat(a, f, 0), f, 1)


def frames(pos, nrm, core, idx):
    """Each texel's place P (m), normal N, tangent T (+u) and bitangent B (+v) from its neighbours' places (the same frame
    as Blender's MikkTSpace loop tangents: dot 0.98 median on the barge), its footprint (m, its longer side; texels on
    an island's rim take the median) and that median."""
    size = core.shape[0]
    P, N = pos[idx], nrm[idx]
    N /= np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-9)

    def step(dy, dx):
        yy, xx = np.clip(idx[0] + dy, 0, size - 1), np.clip(idx[1] + dx, 0, size - 1)
        return pos[yy, xx], core[yy, xx][:, None]

    def diff(a, b):
        (pa, ka), (pb, kb) = a, b
        return np.where(ka & kb, (pa - pb) * 0.5, np.where(ka, pa - P, np.where(kb, P - pb, 0.0)))
    du = diff(step(0, 1), step(0, -1))          # a column right: +u
    dv = diff(step(-1, 0), step(1, 0))          # a row up: +v
    foot = np.maximum(np.linalg.norm(du, axis=1), np.linalg.norm(dv, axis=1))
    typical = float(np.median(foot[foot > 0])) if (foot > 0).any() else 0.04
    foot = np.where(foot > 0, foot, typical).astype(np.float32)
    T = du - (du * N).sum(1, keepdims=True) * N
    tl = np.linalg.norm(T, axis=1, keepdims=True)
    fall = np.cross(N, np.where(np.abs(N[:, 2:3]) < 0.9, np.array([[0.0, 0.0, 1.0]]), np.array([[1.0, 0.0, 0.0]])))
    T = np.where(tl > 1e-7, T / np.maximum(tl, 1e-9), fall / np.linalg.norm(fall, axis=1, keepdims=True))
    B = dv - (dv * N).sum(1, keepdims=True) * N - (dv * T).sum(1, keepdims=True) * T
    bl = np.linalg.norm(B, axis=1, keepdims=True)
    B = np.where(bl > 1e-7, B / np.maximum(bl, 1e-9), np.cross(N, T))
    return P, N.astype(np.float32), T.astype(np.float32), B.astype(np.float32), foot, typical


def vote_3d(zc, pos, good, bad, kinds, cell_m):
    """zc (a zone per texel) with each bad texel's zone the one most good texels in the 3 x 3 x 3 cells (cell_m) round
    it have (a stretched face's texels take their 3D neighbourhood's paint, not Tripo's streaks)."""
    def key(c):
        c = c + 32768
        return (c[:, 0] << 32) | (c[:, 1] << 16) | c[:, 2]
    cell = lambda q: np.floor(q / cell_m).astype(np.int64)
    uk, cnt = np.unique(key(cell(pos[good])) * kinds + zc[good], return_counts=True)
    cb = cell(pos[bad])
    votes = np.zeros((len(cb), kinds), np.int64)
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            for dz in (-1, 0, 1):
                kk = key(cb + np.array([dx, dy, dz])) * kinds
                for z_ in range(kinds):
                    i = np.minimum(np.searchsorted(uk, kk + z_), len(uk) - 1)
                    votes[:, z_] += np.where(uk[i] == kk + z_, cnt[i], 0)
    has = votes.sum(1) > 0
    zb = zc[bad]
    zb[has] = votes[has].argmax(1)
    out = zc.copy()
    out[bad] = zb
    return out


def wear(col, orm, amount, p, edge, grime):
    """rb.wear's rule on the texels (pointiness p, softened): convex edges paler, on metal bright and glossy; creases
    darker and rougher; amount: how much of each texel takes it (the set zones' weight)."""
    convex, crease = rb._smooth(p, 0.55, 0.65) * amount, rb._smooth(-p, -0.44, -0.35) * amount
    metal = np.clip((orm[:, 2] - 0.3) / 0.4, 0.0, 1.0)[:, None]
    luma = col @ LUMA
    bare = np.stack([luma * 0.5 + 0.32] * 3, axis=-1) * np.array([0.98, 0.99, 1.0], np.float32)
    worn = (col + (np.maximum(bare, col) - col) * 0.75) * metal + (col * 1.18 + 0.02) * (1.0 - metal)
    kk = (convex * edge)[:, None]
    col = np.clip((col * (1.0 - kk) + worn * kk) * (1.0 - crease * grime * 0.8)[:, None], 0.0, 1.0)
    orm[:, 1] = np.clip(orm[:, 1] - 0.18 * convex * edge * metal[:, 0] + 0.12 * crease * grime, 0.04, 1.0)
    return col, orm


def relief(nts, col, amount, core, idx, strength):
    """The new colour's fine relief (rb.normals' bump) pressed into the normals (amount: per texel); normalized."""
    size = core.shape[0]
    height = np.zeros((size, size), np.float32)
    height[idx] = (col @ LUMA) * amount
    height = height - masked_blur(height, core, 3.0)

    def grad(axis):
        # Central where both neighbours are the model's texels, one-sided on an island's rim (a step to the empty
        # atlas beside it would draw every UV seam as a groove).
        hp, hm = np.roll(height, -1, axis)[idx], np.roll(height, 1, axis)[idx]
        kp, km = np.roll(core, -1, axis)[idx], np.roll(core, 1, axis)[idx]
        h = height[idx]
        return np.where(kp & km, (hp - hm) * 0.5, np.where(kp, hp - h, np.where(km, h - hm, 0.0)))
    gx, gy = grad(1), grad(0)
    nts[:, 0] -= gx * strength * 8.0
    nts[:, 1] += gy * strength * 8.0
    nts[:, 2] = np.maximum(nts[:, 2], 0.2)
    return nts / np.linalg.norm(nts, axis=1, keepdims=True)
