"""Tools for reshaping a fitted Tripo mesh by script, keeping Tripo's own faces (looter_tripo_barge, the dredge barge's
rework, uses them). Everything is in the Unreal frame: points in cm, X front, Y right, Z up; B() turns one into the
model's Blender metres (front -Y: Unreal +X is Blender -Y, Unreal +Y is Blender -X), U() back.

- Cutting: bisect() cuts faces along a plane, spanning() finds the faces a plane crosses that need the cut (tests over(),
  above(), reaching(), steep()); tidy() afterwards collapses short edges, triangulates cut polygons the best way round
  and drops hairline slivers (Unreal's "nearly zero tangents" come from them).
- Copying: copy() duplicates faces with their UVs and moves the copies' corners by a function; trim() cuts a part's
  faces to a box; patch() is a trimmed copy of a Tripo surface (a deck plate, a bulwark band) to lay elsewhere.
- Filling: cap() fills the cut a deletion left on a plane (its boundary chains, open ones closed along a base height);
  Nearest gives a fill's faces the paint of the Tripo surface next to them (a small piece of that triangle's UV chart).
- Measuring: outline() the skin's half-beam at a height, profile() a section's half-beam by height, box_of() a part's box.
- Collision: ucx() a box hull, hull() a convex hull round points (their extremes along 26 directions).
"""
import math

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

import looter_model as lm
from looter_tripo_paint import UV_AREA_MIN, Nearest, paint_group, paint_like, repaint, uv_flat  # noqa: F401

EPS = 0.006                           # m: on a cut plane (BISECT_DIST and a margin)
BISECT_DIST = 0.005                   # m: a vertex this near a cut plane counts as on it (no sliver split off)
ON_BASE = BISECT_DIST * 100.0 + 0.5   # cm: a cut chain's end this near the base height ends on it
LEVEL_FACE = 4.0                      # cm: a face rising less than this is not cut level (no slivers)
SLIVER = 0.003                        # m: edges shorter than this collapse after the cuts, faces thinner go


LOOSE_JOIN = 6.0                      # cm: a cut chain's loose ends this near each other are joined before its cap
KDOP = [Vector(d).normalized() for d in
        [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)] +
        [(a, b, 0) for a in (1, -1) for b in (1, -1)] + [(a, 0, b) for a in (1, -1) for b in (1, -1)] +
        [(0, a, b) for a in (1, -1) for b in (1, -1)] + [(a, b, c) for a in (1, -1) for b in (1, -1) for c in (1, -1)]]


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


# --- Cutting ---

def bisect(bm, faces, axis, value):
    """Cuts faces along the Unreal plane axis = value (axis 0 x, 1 y, 2 z). Returns the faces after the cut."""
    co = B(tuple(value if i == axis else 0.0 for i in range(3)))
    no = Vector((0.0, 1.0, 0.0)) if axis == 0 else (Vector((1.0, 0.0, 0.0)) if axis == 1 else Vector((0.0, 0.0, 1.0)))
    faces = list(faces)
    if not faces:
        return []
    edges = {e for f in faces for e in f.edges}
    verts = {v for f in faces for v in f.verts}
    made = bmesh.ops.bisect_plane(bm, geom=faces + list(edges) + list(verts), dist=BISECT_DIST, plane_co=co, plane_no=no)
    return [g for g in made['geom'] if isinstance(g, bmesh.types.BMFace)]


def spanning(bm, tag, part, axis, value, test=None, faces=None):
    """Faces of part (tag; None for any) whose vertices lie on both sides of axis = value and pass test(their points,
    cm): only the faces a cut must split (each split near a corner leaves slivers Unreal warns about)."""
    out = []
    for f in (bm.faces if faces is None else faces):
        if part is not None and f[tag] != part:
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


def reaching(z):
    """A spanning() test: the face reaches above z."""
    return lambda ps: max(p[2] for p in ps) > z + 1.0


def steep(ps):
    """A spanning() test: not a nearly level face (a level cut through one leaves slivers; it is kept or cut away
    whole instead)."""
    return max(p[2] for p in ps) - min(p[2] for p in ps) > LEVEL_FACE


def tidy(bm, cut):
    """After the cuts and fills: short edges collapsed; cut faces (test cut(face)) and any of more than four corners
    triangulated the best way round (a cut can leave a corner on a straight edge, which Unreal's own split would make a
    zero-area triangle); then the slivers dropped. Returns (faces triangulated, slivers dropped)."""
    bmesh.ops.dissolve_degenerate(bm, dist=SLIVER, edges=list(bm.edges))
    big = [f for f in bm.faces if len(f.verts) > 4 or (len(f.verts) > 3 and cut(f))]
    bmesh.ops.triangulate(bm, faces=big, quad_method='BEAUTY', ngon_method='BEAUTY')
    thin = [f for f in bm.faces if 2.0 * f.calc_area() < SLIVER * max(e.calc_length() for e in f.edges)]
    bmesh.ops.delete(bm, geom=thin, context='FACES_ONLY')
    bmesh.ops.delete(bm, geom=[e for e in bm.edges if not e.link_faces], context='EDGES')
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    return len(big), len(thin)


def split_on(bm, faces, axis, value):
    """Parts the mesh along the plane axis = value (after a bisect there): the edges in the plane get their own copies,
    so the two sides can move apart."""
    on = lambda v: abs(U(v.co)[axis] - value) < EPS * 100.0
    edges = [e for e in {e for f in faces for e in f.edges} if on(e.verts[0]) and on(e.verts[1])
             and len(e.link_faces) == 2]
    if edges:
        bmesh.ops.split_edges(bm, edges=edges)
    return len(edges)


# --- Copying ---

def separate(bm, faces):
    """faces parted from the rest of the mesh (copies in their place, the originals deleted: bmesh.ops.split crashes
    Blender 4.4 when its maps come back to Python). Returns the parted faces."""
    faces = list(faces)
    if not faces:
        return []
    geom = faces + list({e for f in faces for e in f.edges}) + list({v for f in faces for v in f.verts})
    out = bmesh.ops.duplicate(bm, geom=geom)['geom']
    bmesh.ops.delete(bm, geom=faces, context='FACES')
    return [g for g in out if isinstance(g, bmesh.types.BMFace)]


def copy(bm, faces, tag, part, move=None, flip=False):
    """Copies of faces (Tripo's UVs with them), tagged part, their corners moved by move(point cm) -> point cm;
    flip reverses them (a mirroring move). Returns the copies."""
    faces = list(faces)
    if not faces:
        return []
    geom = faces + list({e for f in faces for e in f.edges}) + list({v for f in faces for v in f.verts})
    out = bmesh.ops.duplicate(bm, geom=geom)['geom']
    new = [g for g in out if isinstance(g, bmesh.types.BMFace)]
    if move is not None:
        for g in out:
            if isinstance(g, bmesh.types.BMVert):
                g.co = B(move(U(g.co)))
    if flip:
        bmesh.ops.reverse_faces(bm, faces=new)
    for f in new:
        f[tag] = part
        f.normal_update()
    return new


def trim(bm, faces, box):
    """Cuts faces to the box (x0, x1, y0, y1, z0, z1, cm; None leaves an axis open), deleting what lies outside.
    Returns the faces left."""
    faces = list(faces)
    for axis in range(3):
        for k in (0, 1):
            value = box[2 * axis + k]
            if value is None:
                continue
            cut = spanning(bm, None, None, axis, value, faces=faces)
            if cut:
                rest = [f for f in faces if f not in set(cut)]
                faces = rest + bisect(bm, cut, axis, value)
    keep, gone = [], []
    for f in faces:
        c = ue_centre(f)
        inside = all((box[2 * a] is None or c[a] >= box[2 * a]) and (box[2 * a + 1] is None or c[a] <= box[2 * a + 1])
                     for a in range(3))
        (keep if inside else gone).append(f)
    bmesh.ops.delete(bm, geom=gone, context='FACES')
    return keep


def in_box(f, box, margin=0.0):
    ps = [U(v.co) for v in f.verts]
    return all((box[2 * a] is None or max(p[a] for p in ps) > box[2 * a] - margin) and
               (box[2 * a + 1] is None or min(p[a] for p in ps) < box[2 * a + 1] + margin) for a in range(3))


def patch(bm, source, box, tag, part, move=None, flip=False):
    """A piece of a Tripo surface: the faces of source that reach into box, copied, trimmed to the box and moved
    (move(point) -> point, flip for a mirroring move). Returns its faces."""
    faces = [f for f in source if f.is_valid and in_box(f, box)]
    new = copy(bm, faces, tag, part)
    new = trim(bm, new, box)
    if move is not None:
        for v in {v for f in new for v in f.verts}:
            v.co = B(move(U(v.co)))
    if flip:
        bmesh.ops.reverse_faces(bm, faces=new)
    for f in new:
        f.normal_update()
    return new


def turn(angle, src, dst, scale=(1.0, 1.0, 1.0)):
    """A move: about src's vertical axis by angle (degrees), scaled (in the turned frame) and put at dst."""
    c, s = math.cos(math.radians(angle)), math.sin(math.radians(angle))

    def move(p):
        x, y, z = p[0] - src[0], p[1] - src[1], p[2] - src[2]
        x, y = x * c - y * s, x * s + y * c
        return (dst[0] + x * scale[0], dst[1] + y * scale[1], dst[2] + z * scale[2])
    return move


# --- Filling ---

def cap(bm, tag, part, axis, value, base_z, out, nearest, new_part, region=None):
    """Fills the cut left on the plane axis = value by part's faces (region(point) limits it): its boundary chains
    (closed, or open with both ends on z = base_z, closed along it; base_z None for closed loops only) triangle-filled,
    facing out, painted by nearest, tagged new_part. Returns the new faces."""
    on = lambda v: abs(U(v.co)[axis] - value) < EPS * 100.0
    edges = [e for e in bm.edges if e.is_boundary and (part is None or e.link_faces[0][tag] == part) and on(e.verts[0])
             and on(e.verts[1])
             and (base_z is None or min(U(v.co)[2] for v in e.verts) > base_z - ON_BASE)
             and (region is None or all(region(U(v.co)) for v in e.verts))]
    if not edges:
        return []
    links = {}
    for e in edges:
        for v in e.verts:
            links.setdefault(v, []).append(e)
    ends = [v for v, es in links.items() if len(es) == 1]
    loose = [v for v in ends if base_z is None or abs(U(v.co)[2] - base_z) > ON_BASE]
    # Chains broken by a hair (a joint's filled slit meets the cut a few mm off): joined across the break.
    for v in list(loose):
        if v not in loose:
            continue
        w = min((u for u in loose if u is not v), key=lambda u: (u.co - v.co).length, default=None)
        if w is not None and (w.co - v.co).length * 100.0 < LOOSE_JOIN:
            e = bm.edges.get((v, w)) or bm.edges.new((v, w))
            edges.append(e)
            for u in (v, w):
                links[u].append(e)
                loose.remove(u)
                ends.remove(u)
    closing = []
    seen, used = set(), set()
    for v in ends:
        if v in seen or v in loose:
            continue
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
    normal = D(out)
    closing = [e for e in dict.fromkeys(closing) if e not in set(edges)]     # a one-edge chain closes on itself
    made = bmesh.ops.triangle_fill(bm, use_beauty=True, use_dissolve=False, edges=edges + closing, normal=normal)
    new = [g for g in made['geom'] if isinstance(g, bmesh.types.BMFace)]
    for f in new:
        f.normal_update()
        if f.normal.dot(normal) < 0.0:
            f.normal_flip()
        f[tag] = new_part
        f.smooth = False
        nearest.paint(f)
    log(f'cap at {"xyz"[axis]} {value:.0f}: {len(edges)} boundary edges, {len(closing)} closed, {len(loose)} loose ends, '
        f'{len(new)} faces' + (f'; loose at {[tuple(round(c) for c in U(v.co)) for v in loose[:6]]}' if loose else ''))
    return new


def close_slot(bm, faces, z, tag, new_part, nearest, x_ends):
    """Closes the slot a cut at height z left in faces (a copied slab's keel where a strut was): its boundary chains on
    z, their ends joined across each end plane x_ends, filled facing down, painted by nearest. Returns the new faces."""
    on = lambda v: abs(U(v.co)[2] - z) < EPS * 100.0
    edges = [e for e in {e for f in faces if f.is_valid for e in f.edges} if e.is_boundary and on(e.verts[0])
             and on(e.verts[1])]
    if not edges:
        return []
    links = {}
    for e in edges:
        for v in e.verts:
            links.setdefault(v, []).append(e)
    ends = [v for v, es in links.items() if len(es) == 1]
    closing = []
    for xe in x_ends:
        at = sorted((v for v in ends if abs(U(v.co)[0] - xe) < 2.0), key=lambda v: U(v.co)[1])
        for v0, v1 in zip(at[0::2], at[1::2]):
            closing.append(bm.edges.get((v0, v1)) or bm.edges.new((v0, v1)))
    made = bmesh.ops.triangle_fill(bm, use_beauty=True, use_dissolve=False, edges=edges + closing,
                                   normal=Vector((0.0, 0.0, -1.0)))
    new = [g for g in made['geom'] if isinstance(g, bmesh.types.BMFace)]
    for f in new:
        f.normal_update()
        if f.normal.z > 0.0:
            f.normal_flip()
        f[tag] = new_part
        f.smooth = False
        nearest.paint(f)
    return new


# --- Measuring ---

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
    for k in range(len(line)):
        if line[k] == 0.0:
            line[k] = max(line[max(k - 1, 0)], line[min(k + 1, len(line) - 1)])
    return line


def edge_arrays(bm, faces=None):
    """Every edge's two ends (cm), for profile()."""
    edges = list(bm.edges) if faces is None else list({e for f in faces for e in f.edges})
    a = np.array([U(e.verts[0].co) for e in edges])
    b = np.array([U(e.verts[1].co) for e in edges])
    return a, b


def profile(arrays, x, zs, side):
    """The section at x: its half-beam (cm) on one side at each height zs (empty heights take their neighbours')."""
    a, b = arrays
    d0, d1 = a[:, 0] - x, b[:, 0] - x
    m = (d0 > 0) != (d1 > 0)
    t = (d0[m] / (d0[m] - d1[m]))[:, None]
    p = a[m] + (b[m] - a[m]) * t
    p = p[p[:, 1] * side > 0]
    out = np.zeros(len(zs))
    for j, z in enumerate(zs):
        sel = np.abs(p[:, 2] - z) < (zs[1] - zs[0]) * 0.6
        out[j] = np.abs(p[sel, 1]).max() if sel.any() else 0.0
    for _ in range(4):
        out = np.where(out == 0.0, np.maximum(np.roll(out, 1), np.roll(out, -1)), out)
    return out


def box_of(bm, tag, part, test=None):
    """The box (x0, y0, z0, x1, y1, z1, cm) round part's vertices that pass test(point), or None."""
    pts = [U(v.co) for f in bm.faces if f[tag] == part for v in f.verts]
    pts = [p for p in pts if test is None or test(p)]
    if not pts:
        return None
    a = np.array(pts)
    return [round(float(v), 1) for v in (*a.min(0), *a.max(0))]


# --- Collision ---

def ucx(obj, boxes, name, x0, x1, y0, y1, z0, z1):
    """A box collision hull (cm, the Unreal frame) on obj, noted in boxes as (name, centre, size, object)."""
    (x0, x1), (y0, y1), (z0, z1) = sorted((x0, x1)), sorted((y0, y1)), sorted((z0, z1))
    centre = ((x0 + x1) / 2.0, (y0 + y1) / 2.0, (z0 + z1) / 2.0)
    ob = lm.hull_box(obj, ((y1 - y0) / 100.0, (x1 - x0) / 100.0, (z1 - z0) / 100.0), B(centre))
    boxes.append((name, tuple(round(c, 1) for c in centre), (round(x1 - x0, 1), round(y1 - y0, 1), round(z1 - z0, 1)),
                  ob))


def hull(obj, boxes, name, pts):
    """A convex collision hull round pts (cm): the hull of their extremes along 26 directions (a 26-DOP's corners)."""
    if len(pts) < 4:
        return
    P = np.array(pts)
    picks = {int(np.argmax(P @ np.array(d))) for d in KDOP}
    bm = bmesh.new()
    for i in picks:
        bm.verts.new(B(P[i]))
    try:
        made = bmesh.ops.convex_hull(bm, input=list(bm.verts))
    except Exception:  # noqa: BLE001 - flat point sets have no hull
        bm.free()
        return
    for g in made.get('geom_interior', []) + made.get('geom_unused', []):
        if isinstance(g, bmesh.types.BMVert) and g.is_valid:
            bm.verts.remove(g)
    if not bm.faces:
        bm.free()
        return
    mesh = bpy.data.meshes.new('UCX_' + obj.name)
    bm.to_mesh(mesh)
    bm.free()
    ob = bpy.data.objects.new('UCX_' + obj.name, mesh)
    bpy.context.scene.collection.objects.link(ob)
    ob.parent = obj
    ob.display_type = 'WIRE'
    lo, hi = P[list(picks)].min(0), P[list(picks)].max(0)
    boxes.append((name, tuple(round(float(c), 1) for c in (lo + hi) / 2.0),
                  tuple(round(float(c), 1) for c in hi - lo), ob))
