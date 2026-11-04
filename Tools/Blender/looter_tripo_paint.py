"""Painting for the reworked Tripo models (looter_tripo_rework re-exports these): new faces (caps, fills, slits) take
Tripo's own paint, from the triangle nearest them, from the face beside them, or as one surface per piece, and any face
left without UV area gets a small patch (Unreal needs a tangent frame on every corner)."""
from mathutils import Vector
from mathutils.bvhtree import BVHTree

CHART_SLACK, CHART_SHARE = 0.15, 0.3    # a cap within its neighbour's triangle (+ this) keeps that map; else this share
UV_AREA_MIN = 1e-9                    # UV area (atlas units squared) under which a triangle has no tangent frame


class Nearest:
    """Tripo's surface as it came (its triangles and their UVs), so a fill's face can take the paint of the triangle
    nearest it: a small parallelogram inside that triangle's UV chart, the face's outline mapped onto it."""

    def __init__(self, bm, uv):
        tris = []
        self.uvs = []
        self.verts = []
        for f in bm.faces:
            loops = list(f.loops)
            for k in range(1, len(loops) - 1):
                tri = (loops[0], loops[k], loops[k + 1])
                u = [l[uv].uv.copy() for l in tri]
                if abs((u[1] - u[0]).cross(u[2] - u[0])) < UV_AREA_MIN:    # no paint to give (a zero-area chart)
                    continue
                self.verts.append([l.vert.co.copy() for l in tri])
                self.uvs.append(u)
                tris.append(len(self.verts) - 1)
        pts = [p for t in self.verts for p in t]
        self.tree = BVHTree.FromPolygons(pts, [(3 * i, 3 * i + 1, 3 * i + 2) for i in range(len(self.verts))])
        self.uv = uv

    def paint(self, f, share=0.35):
        loc, nrm, idx, dist = self.tree.find_nearest(f.calc_center_median())
        if idx is None:
            return
        u0, u1, u2 = self.uvs[idx]
        c = (u0 + u1 + u2) / 3.0
        du, dv = (u1 - u0) * share, (u2 - u0) * share
        n = f.normal
        a = n.orthogonal().normalized()
        b = n.cross(a).normalized()
        ps = [(v.co.dot(a), v.co.dot(b)) for v in f.verts]
        lo = [min(p[i] for p in ps) for i in (0, 1)]
        hi = [max(p[i] for p in ps) for i in (0, 1)]
        for loop, p in zip(f.loops, ps):
            s = (p[0] - lo[0]) / max(hi[0] - lo[0], 1e-5) - 0.5
            t = (p[1] - lo[1]) / max(hi[1] - lo[1], 1e-5) - 0.5
            loop[self.uv].uv = c + du * s + dv * t
        # A sliver maps to a line: its corners take the parallelogram's corners in turn (a tangent frame for Unreal).
        us = [loop[self.uv].uv for loop in f.loops]
        area = sum((us[k] - us[0]).cross(us[k + 1] - us[0]) for k in range(1, len(us) - 1))
        if abs(area) < UV_AREA_MIN:
            for k, loop in enumerate(f.loops):
                loop[self.uv].uv = c + (du, dv, -du, -dv)[k % 4] * 0.5


def paint_like(uv, faces):
    """Fill faces painted as the face beside them that lies most like them (its 3D-to-UV map carried on over theirs):
    a cap in a deck takes on the deck's own paint, no patch. Each connected piece of faces on its own."""
    faces = [f for f in faces if f.is_valid]
    pool, pieces, seen = set(faces), [], set()
    for f in faces:
        if f in seen:
            continue
        piece, todo = [], [f]
        seen.add(f)
        while todo:
            g = todo.pop()
            piece.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h in pool and h not in seen:
                        seen.add(h)
                        todo.append(h)
        pieces.append(piece)
    return all([_paint_like(uv, piece) for piece in pieces]) if pieces else False


def _paint_like(uv, faces):
    inside = set(faces)
    if not faces:
        return False
    n = sum((f.normal * f.calc_area() for f in faces), Vector())
    n = n.normalized() if n.length > 1e-9 else Vector((0.0, 0.0, 1.0))
    beside = {g for f in faces for e in f.edges for g in e.link_faces if g not in inside}
    beside = [g for g in beside if g.calc_area() > 1e-6]
    if not beside:
        return False
    src = max(beside, key=lambda g: abs(g.normal.dot(n)) * g.calc_area() ** 0.5)
    ls = list(src.loops)[:3]
    p0, e1, e2 = ls[0].vert.co, ls[1].vert.co - ls[0].vert.co, ls[2].vert.co - ls[0].vert.co
    u0, d1, d2 = ls[0][uv].uv, ls[1][uv].uv - ls[0][uv].uv, ls[2][uv].uv - ls[0][uv].uv
    g11, g12, g22 = e1.dot(e1), e1.dot(e2), e2.dot(e2)
    det = g11 * g22 - g12 * g12
    if abs(det) < 1e-12 or abs(d1.cross(d2)) < UV_AREA_MIN:
        return False
    # The neighbour's own map carried on, where the cap stays within its chart (seamless); a bigger cap would run off
    # it into other charts' paint, so it takes a parallelogram inside the neighbour's triangle instead.
    coords = {}
    for v in {v for f in faces for v in f.verts}:
        q = v.co - p0
        b1, b2 = q.dot(e1), q.dot(e2)
        coords[v] = ((g22 * b1 - g12 * b2) / det, (g11 * b2 - g12 * b1) / det)
    if all(-CHART_SLACK <= a and -CHART_SLACK <= b and a + b <= 1.0 + CHART_SLACK for a, b in coords.values()):
        for f in faces:
            for loop in f.loops:
                a, b = coords[loop.vert]
                loop[uv].uv = u0 + d1 * a + d2 * b
        return not any(uv_flat(uv, f) for f in faces)
    lo = [min(c[i] for c in coords.values()) for i in (0, 1)]
    hi = [max(c[i] for c in coords.values()) for i in (0, 1)]
    mid = u0 + (d1 + d2) / 3.0
    for f in faces:
        for loop in f.loops:
            a, b = coords[loop.vert]
            s_ = (a - lo[0]) / max(hi[0] - lo[0], 1e-6) - 0.5
            t_ = (b - lo[1]) / max(hi[1] - lo[1], 1e-6) - 0.5
            loop[uv].uv = mid + d1 * (s_ * CHART_SHARE) + d2 * (t_ * CHART_SHARE)
    return not any(uv_flat(uv, f) for f in faces)          # a cap across its neighbour's plane maps to a line


def uv_flat(uv, f):
    """True when face f has no UV area (no tangent frame for Unreal)."""
    us = [l[uv].uv for l in f.loops]
    return abs(sum((us[k] - us[0]).cross(us[k + 1] - us[0]) for k in range(1, len(us) - 1))) < UV_AREA_MIN


def paint_group(nearest, faces, share=0.6):
    """Faces painted as one surface (a cap, a sill): one parallelogram of the chart of the Tripo triangle nearest their
    middle over their common plane, so the cap reads whole (each face its own patch made it look blobby)."""
    faces = [f for f in faces if f.is_valid]
    if not faces:
        return
    verts = list({v for f in faces for v in f.verts})
    mid = sum((v.co for v in verts), Vector()) / len(verts)
    loc, nrm, idx, dist = nearest.tree.find_nearest(mid)
    if idx is None:
        return
    u0, u1, u2 = nearest.uvs[idx]
    c, du, dv = (u0 + u1 + u2) / 3.0, (u1 - u0) * share, (u2 - u0) * share
    n = sum((f.normal for f in faces), Vector())
    n = n.normalized() if n.length > 1e-9 else Vector((0.0, 0.0, 1.0))
    a = n.orthogonal().normalized()
    b = n.cross(a).normalized()
    ps = {v: (v.co.dot(a), v.co.dot(b)) for v in verts}
    lo = [min(p[i] for p in ps.values()) for i in (0, 1)]
    hi = [max(p[i] for p in ps.values()) for i in (0, 1)]
    for f in faces:
        for loop in f.loops:
            p = ps[loop.vert]
            s = (p[0] - lo[0]) / max(hi[0] - lo[0], 1e-5) - 0.5
            t = (p[1] - lo[1]) / max(hi[1] - lo[1], 1e-5) - 0.5
            loop[nearest.uv].uv = c + du * s + dv * t


def repaint(bm, tag, fill, nearest, keel=()):
    """After all the cuts and moves: every fill (tag == fill) painted again on its final shape, each connected piece as
    one surface (faces of one piece painted each its own way met with opposite tangents: Unreal's zero binormals), the
    keel's faces as one strip (the same, where copies, slits and plates meet in one level), and any face still without
    UV area its own small patch."""
    uv = nearest.uv
    fills, seen = [f for f in bm.faces if f[tag] == fill], set()
    for f in fills:
        if f in seen:
            continue
        piece, todo = [], [f]
        seen.add(f)
        while todo:
            g = todo.pop()
            piece.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h[tag] == fill and h not in seen:
                        seen.add(h)
                        todo.append(h)
        paint_like(uv, piece) or paint_group(nearest, piece)
    if keel:
        paint_group(nearest, keel)
    for f in bm.faces:
        if uv_flat(uv, f):
            nearest.paint(f)
