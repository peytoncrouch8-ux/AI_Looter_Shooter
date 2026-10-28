"""Helpers for AR set 3 (Windfall, scrap-built) Tripo parts, beside looter_tripo (Art/Models/Weapons/Tripo/parts_set3.py
uses them). Set 3's paint is not a sand shell on its own piece but painted panels welded into one-piece meshes, so:
- is_teal / is_sky pick its sea-teal and sky-blue paint (the galvanized tin, grey and low in saturation, stays out);
- paint_faces() takes the faces under a paint, closed over the face graph so rust chips inside a panel go with it;
- paint_sets() writes one paintable copy of the colour map for every paint (each normalised by its own brightest texels,
  the rest of the map dark grey) and returns one material per paint slot, <slot>_<set> (the game paints a slot named
  <paint>_<anything> with that paint), all on the one texture;
- shift(), recentre() and the probes move and measure a fitted mesh (gun space, cm), so joints land on set 1's interfaces.
"""
import os
import shutil

import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_guns as lg
import looter_textures as lt
import looter_tripo as tripo


def _hsv(rgb):
    rgb = np.asarray(rgb, dtype=np.float32)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    hi, lo = rgb.max(axis=-1), rgb.min(axis=-1)
    d = np.maximum(hi - lo, 1e-4)
    h = np.where(hi == r, ((g - b) / d) % 6.0, np.where(hi == g, (b - r) / d + 2.0, (r - g) / d + 4.0)) * 60.0
    return h, (hi - lo) / np.maximum(hi, 1e-4), hi


def is_teal(rgb):
    """Windfall's sea-teal paint (#4f8f8a; Tripo's about #44847b): green-blue, well saturated."""
    h, s, v = _hsv(rgb)
    return (h >= 145.0) & (h < 193.0) & (s > 0.3) & (v > 0.3)


def is_cell(rgb):
    """The sight's teal cell (#34635f): as is_teal but greener, away from its dark blue-teal glass (hue ~190)."""
    h, s, v = _hsv(rgb)
    return (h >= 145.0) & (h < 184.0) & (s > 0.3) & (v > 0.3)


def is_sky(rgb):
    """Windfall's sky-blue paint (#7d9fb5; Tripo's about #7192a5): blue, half saturated, light."""
    h, s, v = _hsv(rgb)
    return (h >= 190.0) & (h < 222.0) & (s > 0.16) & (s < 0.5) & (v > 0.47)


# --- Moving and measuring (gun space, cm) ---

def shift(obj, du=0.0, dv=0.0, dw=0.0):
    obj.data.transform(Matrix.Translation(lg.at(du, dv, dw)))
    obj.data.update()


def points(obj):
    return np.array([tripo.gun_uvw(v.co) for v in obj.data.vertices])


def extent(obj, u=None, v=None, w=None):
    """Bounds (lo, hi) of the vertices inside the given (lo, hi) ranges of u, v, w; None if none."""
    p = points(obj)
    keep = np.ones(len(p), bool)
    for axis, rng in enumerate((u, v, w)):
        if rng is not None:
            keep &= (p[:, axis] >= rng[0]) & (p[:, axis] <= rng[1])
    if not keep.any():
        return None
    return p[keep].min(axis=0), p[keep].max(axis=0)


def rescale(obj, centre, sv=1.0, sw=1.0):
    """Scales v and w about centre (v, w) (u untouched)."""
    cv, cw = centre
    for vert in obj.data.vertices:
        u, v, w = tripo.gun_uvw(vert.co)
        vert.co = lg.at(u, cv + (v - cv) * sv, cw + (w - cw) * sw)
    obj.data.update()


def _tree(obj):
    me = obj.data
    return BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])


def probe(obj, origin, direction, reach=40.0):
    """The first hit (u, v, w) of a ray from origin (u, v, w cm) along direction ((du, dv, dw)), or None."""
    d = lg.at(*direction)
    d.normalize()
    hit = _tree(obj).ray_cast(lg.at(*origin), d, reach * lg.CM)[0]
    return tripo.gun_uvw(hit) if hit is not None else None


def probe_u_back(obj, pts, reach=40.0, start=-30.0):
    """The rear surface (u, cm), seen along +u from u = start at each (v, w); the median of the hits."""
    tree = _tree(obj)
    hits = []
    for v, w in pts:
        hit = tree.ray_cast(lg.at(start, v, w), Vector((0.0, -1.0, 0.0)), reach * 3 * lg.CM)[0]
        if hit is not None:
            hits.append(tripo.gun_uvw(hit)[0])
    return sorted(hits)[len(hits) // 2] if hits else None


def probe_w(obj, u, v, side, reach=10.0):
    """The outer surface's w at (u, v) on side (+1 the gun's left, -1 its right), seen from outside."""
    hit = _tree(obj).ray_cast(lg.at(u, v, side * reach), lg.at(0.0, 0.0, -side), 2 * reach * lg.CM)[0]
    return tripo.gun_uvw(hit)[2] if hit is not None else None


def faces_seen(obj, u0, u1, v_range, w_range, step=0.05):
    """Every face a ray along +u meets between u0 and u1 (cm) through a grid over v_range x w_range: what stands in a
    sight's window (each ray goes on past each hit)."""
    tree = _tree(obj)
    found = set()
    for v in np.arange(v_range[0], v_range[1] + 1e-6, step):
        for w in np.arange(w_range[0], w_range[1] + 1e-6, step):
            u = u0
            for _ in range(32):
                loc, _n, index, dist = tree.ray_cast(lg.at(u, v, w), Vector((0.0, -1.0, 0.0)), (u1 - u) * lg.CM)
                if loc is None:
                    break
                found.add(index)
                u = tripo.gun_uvw(loc)[0] + 0.002
    return sorted(found)


def faces_in(obj, u=None, v=None, w=None, normal=None, min_dot=0.0):
    """Faces whose centre lies in the ranges (u, v, w) and, with normal ((du, dv, dw)), whose normal's |dot| with it is
    at least min_dot."""
    out = []
    n = lg.at(*normal).normalized() if normal else None
    for p in obj.data.polygons:
        c = tripo.gun_uvw(p.center)
        if all(r is None or r[0] <= x <= r[1] for x, r in zip(c, (u, v, w))):
            if n is None or abs(p.normal.dot(n)) >= min_dot:
                out.append(p.index)
    return out


def delete_faces(obj, faces):
    tripo._delete_faces(obj, faces)


# --- Painted panels ---

def _face_graph(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    nbrs = [set() for _ in bm.faces]
    for e in bm.edges:
        fs = [f.index for f in e.link_faces]
        for a in fs:
            for b in fs:
                if a != b:
                    nbrs[a].add(b)
    bm.free()
    return nbrs


def is_trim(rgb):
    """Tape, brass wire, hazard yellow (warm, saturated, not dark): pieces that stay unpainted inside a panel. Rust
    (redder, hue under 30) is not trim: its chips go with the panel."""
    h, s, v = _hsv(rgb)
    return (h >= 30.0) & (h < 70.0) & (s > 0.25) & (v > 0.35)


def paint_faces(obj, image, test, share=0.5, close=2, keep=None, exclude=is_trim):
    """Faces under a paint: those whose samples pass test (share of them), then closed over the face graph close rings
    (grown, then shrunk), so chips and rivets inside a panel join it while its outline stays; faces mostly exclude's
    colour (tape, wire, a hazard tag) stay out. keep(face index) may veto faces (a region the paint must stay in)."""
    hit = set(tripo.faces_where(obj, image, test, share=share))
    if keep:
        hit = {f for f in hit if keep(f)}
    if not hit or close <= 0:
        return sorted(hit)
    nbrs = _face_graph(obj)
    grown = set(hit)
    for _ in range(close):
        grown |= {n for f in grown for n in nbrs[f]}
    for _ in range(close):
        grown = {f for f in grown if nbrs[f] <= grown}
    if exclude is not None:
        grown -= set(tripo.faces_where(obj, image, exclude, share=0.5)) - hit
    grown |= hit
    if keep:
        grown = {f for f in grown if keep(f)}
    return sorted(grown)


def remap_u(obj, a0, a1, b0, b1):
    """Moves u linearly so a0 lands on b0 and a1 on b1 (a slight stretch that puts two joints exactly)."""
    k = (b1 - b0) / (a1 - a0)
    for vert in obj.data.vertices:
        u, v, w = tripo.gun_uvw(vert.co)
        vert.co = lg.at(b0 + (u - a0) * k, v, w)
    obj.data.update()
    return k


def centre_w(obj, faces):
    """The area-weighted centre of faces (u, v, w)."""
    return tripo.centroid(obj, faces)


def _hex(rgb_srgb):
    c = np.clip(np.round(np.asarray(rgb_srgb) * 255.0), 0, 255).astype(int)
    return (int(c[0]) << 16) | (int(c[1]) << 8) | int(c[2])


def paint_sets(maps, set_name, paints, source, master='Gun', size=tripo.MAX_SIZE):
    """paints: [(slot, test, tint)]. Writes <set_name>Paint (unless newer than source and this file): the colour map in
    linear light, each paint's texels divided by that paint's own brightest colour (98th percentile, per channel) so
    they come out near white with Tripo's light and dark kept, every other texel dark grey (its brightness) so chips and
    stains read as worn paint. Its ORM and normal are the set's. Returns {slot: material <slot>_<set_name>}, each tinted
    its paint's default colour (the game's paint of that slot replaces it)."""
    paint = set_name + 'Paint'
    bc = lt.texture_path(paint, 'BC')
    stamp = max(os.path.getmtime(tripo._abs(source)), os.path.getmtime(__file__))
    if not (os.path.exists(bc) and os.path.getmtime(bc) >= stamp):
        os.makedirs(os.path.dirname(bc), exist_ok=True)
        rgb = tripo.pixels(maps['BC'], min(size, maps['BC'].size[0]))[..., :3]
        lin = tripo._linear(rgb)
        lum = lin @ np.array([0.2126, 0.7152, 0.0722], dtype=np.float32)
        out = None
        refs = []
        masks = []
        for slot, test, tint in paints:
            mask = np.asarray(test(rgb), bool)
            top = np.percentile(lin[mask], 98.0, axis=0) if mask.any() else np.ones(3)
            masks.append((mask, top))
            refs.append(float(top @ np.array([0.2126, 0.7152, 0.0722])))
            lt._log(f'{paint}: {slot} on {mask.mean() * 100:.1f}% of the map, brightest linear {np.round(top, 3).tolist()}')
        ref = max(sum(refs) / max(len(refs), 1), 1e-3)
        grey = np.clip(lum / ref * 0.55, 0.0, 1.0)
        out = np.repeat(grey[..., None], 3, axis=-1)
        for mask, top in masks:
            out[mask] = np.clip(lin[mask] / np.maximum(top, 1e-3), 0.0, 1.0)
        lt.write_png(bc, lt.to8(tripo._srgb(out)))
        lt._log(f'{paint}: wrote T_{paint}_BC ({rgb.shape[0]} px)')
    for kind in ('ORM', 'N'):
        src, dst = lt.texture_path(set_name, kind), lt.texture_path(paint, kind)
        if os.path.exists(src) and not (os.path.exists(dst) and os.path.getmtime(dst) >= os.path.getmtime(src)):
            shutil.copyfile(src, dst)
        elif not os.path.exists(src) and os.path.exists(dst):
            os.remove(dst)
    return {slot: lt.material(paint, name=f'{slot}_{set_name}', master=master, tint=tint) for slot, test, tint in paints}


def add(bp, m, obj, maps, mat, glows=None, paints=None, drop=()):
    """Adds the fitted Tripo mesh to model m: glows {material: faces} first, then each paint's faces (paints: [(slot,
    test, tint, keep or None)]) as its paintable slot, the rest textured; faces in drop are deleted."""
    glows = {k: list(v) for k, v in (glows or {}).items() if v}
    taken = set(f for faces in glows.values() for f in faces) | set(drop)
    keep_uv = [mat]
    if paints:
        set_name = mat.name[len('Gun'):]
        mats = paint_sets(maps, set_name, [(s, t, c) for s, t, c, _ in paints], obj['TripoFile'])
        for slot, test, tint, keep in paints:
            faces = [f for f in paint_faces(obj, maps['BC'], test, keep=keep) if f not in taken]
            lt._log(f'Tripo {m}: {len(faces)} faces paintable as {slot}')
            if faces:
                glows[mats[slot]] = faces
                keep_uv.append(mats[slot])
                taken.update(faces)
    rest = [i for i in range(len(obj.data.polygons)) if i not in taken]
    bp.g.add(m, *tripo.split(obj, {mat: rest, **glows}, keep_uv=tuple(keep_uv)))
