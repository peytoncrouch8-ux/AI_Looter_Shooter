"""Helpers for fitting the Bullpup's set 4 (Meridian: satin-white and graphite polymer, machined aluminium, brass, a cyan
line) Tripo parts (Art/Models/Weapons/Tripo/parts_set4.py), beside looter_tripo's.

Meridian's shell is white, not set 1's sand, so looter_tripo.is_sand misses it: shell() and fittings() pick the white
shell and the graphite fittings by colour (the brushed aluminium, a little bluer, stays textured), and paint() makes a
paintable copy of the part's texture set for a slot, so a gun's shell paint (GunPolymerSand) and fittings paint
(GunPolymerGrey) recolour them like the scripted parts. surface_w() and line() place scripted glow pieces on Tripo's
surface (the painted cyan line and ember slits are too thin in the 2048 atlas to read as glow faces on their own).
"""
import os

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_guns as lg
import looter_textures as lt
import looter_tripo as tripo


# --- Colour tests: (r, g, b) sRGB 0..1 ---

def is_white(rgb):
    """Satin-white shell: light, grey, warm or neutral (the brushed aluminium is as light but bluer, b - r > 0.012)."""
    r, g, b = rgb
    hi, lo = max(r, g, b), min(r, g, b)
    return hi > 0.6 and (hi - lo) / max(hi, 1e-4) < 0.12 and b - r < 0.012


def is_graphite(rgb):
    """Graphite polymer: dark grey (the black rubber, under 0.17, and the brass and ember stay out)."""
    r, g, b = rgb
    hi, lo = max(r, g, b), min(r, g, b)
    return 0.17 < hi < 0.45 and (hi - lo) / max(hi, 1e-4) < 0.22 and b - r > -0.02


def is_alu(rgb):
    """Brushed aluminium (or any light grey): light, nearly colourless."""
    r, g, b = rgb
    hi, lo = max(r, g, b), min(r, g, b)
    return hi > 0.5 and (hi - lo) / max(hi, 1e-4) < 0.18


def is_lens(rgb):
    """A coated lens: blue, saturated."""
    r, g, b = rgb
    hi, lo = max(r, g, b), min(r, g, b)
    return b - r > 0.1 and (hi - lo) / max(hi, 1e-4) > 0.35 and b >= g


def is_cyan_line(rgb):
    """The cyan accent line (a thin painted line, blended with what's round it in the atlas)."""
    r, g, b = rgb
    return b > 0.45 and g > 0.4 and b - r > 0.2 and r < 0.6 * b


def _np_tests():
    """The tests above on arrays (H, W, 3) for paint()."""
    def white(px):
        hi, lo = px.max(-1), px.min(-1)
        return (hi > 0.6) & ((hi - lo) / np.maximum(hi, 1e-4) < 0.12) & (px[..., 2] - px[..., 0] < 0.012)

    def graphite(px):
        hi, lo = px.max(-1), px.min(-1)
        return (hi > 0.17) & (hi < 0.45) & ((hi - lo) / np.maximum(hi, 1e-4) < 0.22) & (px[..., 2] - px[..., 0] > -0.02)
    return {is_white: white, is_graphite: graphite}


# --- Faces by colour ---

def shares(obj, image, tests, size=1024):
    """For each face, the share of its samples (UV centre, corners pulled a third in) passing each test: {test: array}."""
    px = tripo.pixels(image, min(size, image.size[0]))[..., :3]
    h, w = px.shape[:2]
    me = obj.data
    uv = me.uv_layers.active.data
    out = {t: np.zeros(len(me.polygons)) for t in tests}
    for poly in me.polygons:
        pts = [uv[li].uv for li in poly.loop_indices]
        cu = sum(p[0] for p in pts) / len(pts)
        cv = sum(p[1] for p in pts) / len(pts)
        samples = [(cu, cv)] + [(cu + (p[0] - cu) * 0.67, cv + (p[1] - cv) * 0.67) for p in pts]
        cols = [px[min(h - 1, max(0, int((1.0 - (s[1] % 1.0)) * h))), min(w - 1, max(0, int((s[0] % 1.0) * w)))]
                for s in samples]
        for t in tests:
            out[t][poly.index] = sum(1 for c in cols if t(c)) / len(cols)
    return out


def shell_and_fittings(obj, image, taken=(), white_share=0.2, graphite_share=0.34):
    """(white shell faces, graphite fitting faces), not in taken. A face a fifth white goes to the shell: on the parting
    line its graphite part, painted in the shell's colour divided by white, reads as a dark seam, and at the aluminium
    panels' ragged edges its aluminium part takes the paint, where a white part left textured would show as white flecks
    on a painted gun. A face a third graphite goes to the fittings (its aluminium part shows in the paint's brightest
    colour, an edge highlight). Both sets are then cleaned by their neighbours (looter_tripo.majority) so single faces
    don't speckle."""
    s = shares(obj, image, (is_white, is_graphite))
    taken = set(taken)
    shell, fit = [], []
    for i in range(len(obj.data.polygons)):
        if i in taken:
            continue
        if s[is_white][i] >= white_share:
            shell.append(i)
        elif s[is_graphite][i] >= graphite_share:
            fit.append(i)
    first = set(shell)
    shell = [i for i in tripo.majority(obj, shell) if i not in taken and (i in first or s[is_white][i] > 0.0)]
    have = set(shell)
    fit = [i for i in tripo.majority(obj, fit) if i not in taken and i not in have]
    return shell, fit


# --- Paintable copies of a part's texture set ---

def _read_png(path):
    """A PNG's stored values as floats (H, W, C), rows top first."""
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = 'Non-Color'
    w, h = im.size
    buf = np.empty(w * h * im.channels, dtype=np.float32)
    im.pixels.foreach_get(buf)
    bpy.data.images.remove(im)
    return buf.reshape(h, w, -1)[::-1]


def paint(maps, set_name, slot, test, source, tag, master='Gun', size=tripo.MAX_SIZE):
    """A paintable copy of set_name (looter_tripo.write_set's, written first): <set_name><tag>, its colour map in linear
    light divided by the brightest (98th percentile, per channel) of the texels test picks, so they come out near white
    and keep Tripo's light and dark. Its tint (its own colour, which a gun's paint for slot replaces) is that brightest
    colour, so unpainted it looks as Tripo made it. Polymer: its ORM is the set's with the metal taken out (Tripo's maps
    called parts of the white shell metal). Returns the material <slot>_<set_name> (UWeaponModelComponent paints a slot
    named <paint>_<anything> with that paint)."""
    paint_set = set_name + tag
    bc, orm = lt.texture_path(paint_set, 'BC'), lt.texture_path(paint_set, 'ORM')
    stamp = max(os.path.getmtime(tripo._abs(source)), os.path.getmtime(__file__))
    rgb = tripo.pixels(maps['BC'], min(size, maps['BC'].size[0]))[..., :3]
    lin = tripo._linear(rgb)
    mask = _np_tests()[test](rgb)
    top = np.percentile(lin[mask], 98.0, axis=0) if mask.any() else np.ones(3)
    if not (os.path.exists(bc) and os.path.getmtime(bc) >= stamp):
        os.makedirs(os.path.dirname(bc), exist_ok=True)
        out = tripo._srgb(np.clip(lin / np.maximum(top, 1e-3), 0.0, 1.0))
        lt.write_png(bc, lt.to8(out))
        lt._log(f'{paint_set}: wrote T_{paint_set}_BC ({rgb.shape[0]} px; {test.__name__} texels divided by linear '
                f'{np.round(top, 3).tolist()})')
    src = lt.texture_path(set_name, 'ORM')
    if os.path.exists(src) and not (os.path.exists(orm) and os.path.getmtime(orm) >= max(stamp, os.path.getmtime(src))):
        data = _read_png(src)[..., :3].copy()
        data[..., 2] = 0.0
        data[..., 1] = np.maximum(data[..., 1], 0.45)   # satin, not gloss (Tripo's graphite came out mirror-like)
        lt.write_png(orm, lt.to8(data))
    nsrc, ndst = lt.texture_path(set_name, 'N'), lt.texture_path(paint_set, 'N')
    if os.path.exists(nsrc) and not (os.path.exists(ndst) and os.path.getmtime(ndst) >= os.path.getmtime(nsrc)):
        import shutil
        shutil.copyfile(nsrc, ndst)
    elif not os.path.exists(nsrc) and os.path.exists(ndst):
        os.remove(ndst)
    s = tripo._srgb(np.clip(top, 0.0, 1.0))
    tint = (int(round(s[0] * 255)) << 16) | (int(round(s[1] * 255)) << 8) | int(round(s[2] * 255))
    return lt.material(paint_set, name=f'{slot}_{set_name}', master=master, tint=tint)


def repaint_like_neighbours(obj, faces, passes=6):
    """Gives faces (a thin painted line Tripo's triangles cut into teeth) the colour round them: each takes, on every
    corner, the UV centre of a neighbouring face outside the set (passes times, so wider bands fill from their edges).
    The line itself is then drawn as geometry. Returns how many were repainted."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    uv = bm.loops.layers.uv.active
    todo = set(faces)
    done = 0
    for _ in range(passes):
        fixed = []
        for i in todo:
            f = bm.faces[i]
            near = [g for e in f.edges for g in e.link_faces if g.index not in todo]
            if not near:
                continue
            g = max(near, key=lambda x: x.calc_area())
            c = sum((l[uv].uv for l in g.loops), Vector((0.0, 0.0))) / len(g.loops)
            for l in f.loops:
                l[uv].uv = c
            fixed.append(i)
        todo.difference_update(fixed)
        done += len(fixed)
        if not todo:
            break
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return done


# --- Measuring the fitted mesh (gun space, cm) ---

def tree(obj):
    me = obj.data
    return BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])


def surface_w(bvh, u, v, side, reach=8.0):
    """The mesh's outer surface (w, cm) at (u, v) on side (+1 the gun's left, -1 its right), or None."""
    hit = bvh.ray_cast(lg.at(u, v, side * reach), Vector((-side, 0.0, 0.0)), 2.0 * reach * lg.CM)[0]
    return tripo.gun_uvw(hit)[2] if hit is not None else None


def line(obj, faces, side, u0, u1, step=0.8, min_faces=2, keep=None):
    """A polyline (u, v, w) along painted faces on one side (w's sign): per step of u, the faces' median v and w; bins
    with fewer than min_faces are skipped. keep((u, v, w)) picks the faces of one line among several."""
    pts = [tripo.gun_uvw(obj.data.polygons[i].center) for i in faces]
    pts = [p for p in pts if p[2] * side > 0.0 and u0 <= p[0] <= u1 and (keep is None or keep(p))]
    out = []
    u = u0
    while u < u1:
        b = [p for p in pts if u <= p[0] < u + step]
        if len(b) >= min_faces:
            out.append((u + step * 0.5, float(np.median([p[1] for p in b])), float(np.median([p[2] for p in b]))))
        u += step
    return out


def bounds(obj, faces):
    """(lo, hi) of faces' vertices, (u, v, w) cm."""
    me = obj.data
    pts = [tripo.gun_uvw(me.vertices[k].co) for i in faces for k in me.polygons[i].vertices]
    return (tuple(min(p[i] for p in pts) for i in range(3)), tuple(max(p[i] for p in pts) for i in range(3)))


def move(obj, du=0.0, dv=0.0, dw=0.0):
    from mathutils import Matrix
    obj.data.transform(Matrix.Translation(lg.at(du, dv, dw)))
    obj.data.update()


def delete_faces(obj, faces):
    tripo._delete_faces(obj, faces)


def boundary_loops(obj):
    """The mesh's open edges, grouped into loops: [[(u, v, w), ...], ...] (holes Tripo left)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    open_edges = [e for e in bm.edges if e.is_boundary]
    seen, loops = set(), []
    for e in open_edges:
        if e.index in seen:
            continue
        stack, verts = [e], set()
        seen.add(e.index)
        while stack:
            x = stack.pop()
            for v in x.verts:
                verts.add(v.index)
                for y in v.link_edges:
                    if y.is_boundary and y.index not in seen:
                        seen.add(y.index)
                        stack.append(y)
        loops.append([tripo.gun_uvw(obj.data.vertices[i].co) for i in verts])
    bm.free()
    return loops


def flatten_sides(obj, u0, u1, v0, v1, inward=0.35, outward=0.06, facing=0.8):
    """Presses the dents out of a flat side wall (Tripo's lumps): on each side, the vertices in u0..u1 x v0..v1 whose
    faces look sideways (|normal w| > facing) give the wall's plane (their median |w|); every vertex there from inward cm
    inside it to outward cm proud of it moves onto it (knobs and buttons standing further out stay). Returns the planes."""
    me = obj.data
    normals = [Vector() for _ in me.vertices]
    for p in me.polygons:
        for k in p.vertices:
            normals[k] += p.normal * p.area
    planes = {}
    for side in (-1.0, 1.0):
        pick = []
        for v in me.vertices:
            u, h, w = tripo.gun_uvw(v.co)
            n = normals[v.index].normalized() if normals[v.index].length else normals[v.index]
            if u0 < u < u1 and v0 < h < v1 and w * side > 0.0 and n.x * side > facing:
                pick.append(v)
        if len(pick) < 8:
            continue
        plane = float(np.median([abs(tripo.gun_uvw(v.co)[2]) for v in pick]))
        for v in pick:
            d = abs(tripo.gun_uvw(v.co)[2]) - plane
            if -inward <= d <= outward:
                v.co.x = side * plane * lg.CM
        planes[side] = plane
    me.update()
    return planes


def uv_at(obj, bvh, u, v, w):
    """The UV centre of the face nearest the gun-space point (u, v, w): a texel of the colour there."""
    hit = bvh.find_nearest(lg.at(u, v, w))
    poly = obj.data.polygons[hit[2]]
    uv = obj.data.uv_layers.active.data
    return tuple(sum(uv[li].uv[k] for li in poly.loop_indices) / poly.loop_total for k in range(2))


def patch(piece, mat, uv):
    """piece (a scripted shape) in Tripo's material, every corner on one texel: a flat patch of that colour that keeps its
    UVs (a cover over a hole in the model)."""
    import looter_textures as _lt
    _lt.assign(piece, mat)
    me = piece.data
    if not me.uv_layers:
        me.uv_layers.new(name='UVMap')
    for loop_uv in me.uv_layers.active.data:
        loop_uv.uv = uv
    piece['KeepUV'] = 1
    return piece


def fill_holes(obj, sides=16):
    """Caps Tripo's pinholes (open loops of up to sides edges); each new corner takes the UV a neighbouring face has
    there, so the cap wears the colour round it. Returns how many were filled."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    uv = bm.loops.layers.uv.active
    edges = [e for e in bm.edges if e.is_boundary]
    new = bmesh.ops.holes_fill(bm, edges=edges, sides=sides)['faces'] if edges else []
    fresh = set(new)
    for f in new:
        for loop in f.loops:
            other = next((l for l in loop.vert.link_loops if l.face not in fresh), None)
            if other is not None and uv is not None:
                loop[uv].uv = other[uv].uv
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return len(new)


def add(bp, m, obj, groups, keep):
    """Splits obj into its slots ({material: faces}) and adds them to model m; the materials in keep keep Tripo's UVs."""
    bp.g.add(m, *tripo.split(obj, {k: v for k, v in groups.items() if v}, keep_uv=tuple(keep)))
