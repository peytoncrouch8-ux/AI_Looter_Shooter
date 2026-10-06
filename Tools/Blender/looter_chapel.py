"""Helpers for the Chapel family of Ransom's Rest (Art/Models/Buildings/Chapel.py, Art/Models/Props/Graves.py):

    lettering(body, size, ...)   painted lettering in the Western font Rye, as a flat low-poly mesh (bmesh)
    Paint, lap, box_block,       old white paint (the trim sheet's strip G, squeezed to its clean stretch, or its
    opening_block                stained one) and lap siding or inside boards as geometry, row by row round the
                                 openings and trims (the clapboard chapel)
    surface_of, contour_loops,   a thin shell draped over a mesh inside the contour of a field (Abel's rime): the
    tidy_loop, draped_shell,     loops traced and softened, filled by a constrained triangulation, lifted clear of
    fade_open_edges              the facets; its edge's occlusion darkened so it reads soft
    render(objects, out, ...)    a preview from a camera placed by hand (lt.preview frames from outside; this one can
                                 stand inside the chapel), with the same sky, sun and color settings as lt.preview
    vignette()                   the churchyard picture: the chapel behind an iron fence with its gate and a dozen
                                 headboards and crosses, Saved/ArtPreviews/RansomsRest/Chapel_overview.png, and the
                                 same with option A (timber frame), RansomsRest/Chapel/Chapel_A_overview.png
    NullModel                    a kit.Model stand-in that keeps nothing (option A when not previewing)

The vignette runs both scripts in one scene, so render it on its own (it starts from an empty scene):

    blender -b --factory-startup --python Tools/Blender/looter_chapel.py -- vignette

Tools/Blender is on sys.path for model scripts (models.ps1), so they `import looter_chapel as lc`.
"""
import math
import os
import random
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

import looter_textures as lt

REPO = lt.REPO
# The Western display font for painted lettering (SIL Open Font License, Art/Fonts/Rye-OFL.txt), with a fallback.
RYE = os.path.join(REPO, 'Art', 'Fonts', 'Rye-Regular.ttf').replace('\\', '/')
FALLBACK_FONT = 'C:/Windows/Fonts/georgiab.ttf'


# --- Lettering ---

def _font():
    for path in (RYE, FALLBACK_FONT):
        if os.path.exists(path):
            try:
                return bpy.data.fonts.load(path, check_existing=True)
            except RuntimeError:
                continue
    return None


def _boundary_loops(bm):
    """The outline loops of a flat filled mesh (vertices with exactly two boundary edges, chained)."""
    links = {}
    for e in bm.edges:
        if e.is_boundary:
            a, b = e.verts
            links.setdefault(a, []).append(b)
            links.setdefault(b, []).append(a)
    seen, loops = set(), []
    for start in links:
        if start in seen or len(links[start]) != 2:
            continue
        loop, prev, cur = [start], None, start
        seen.add(start)
        while True:
            nxt = [v for v in links[cur] if v is not prev and v not in seen and len(links[v]) == 2]
            if not nxt:
                break
            prev, cur = cur, nxt[0]
            seen.add(cur)
            loop.append(cur)
        loops.append(loop)
    return loops


def _keep(points, tol, keep, lo, hi):
    """Douglas-Peucker: marks the indices between lo and hi that the outline needs to stay within tol."""
    a, b = points[lo].co.xy, points[hi].co.xy
    ab = b - a
    length = ab.length
    best, index = -1.0, None
    for i in range(lo + 1, hi):
        p = points[i].co.xy
        d = (p - a).length if length < 1e-12 else abs(ab.x * (a.y - p.y) - ab.y * (a.x - p.x)) / length
        if d > best:
            best, index = d, i
    if index is not None and best > tol:
        keep.add(index)
        _keep(points, tol, keep, lo, index)
        _keep(points, tol, keep, index, hi)


def lettering(body, size, tolerance=0.04, align='CENTER', spacing=1.0):
    """Lettering as a new bmesh: flat faces in the XZ plane facing -Y (front), centered on the origin (align 'LEFT'
    starts it there), size the font size in metres (capitals come out about 0.64 x size tall). Rye's outlines carry
    hundreds of points per word, so each outline is thinned (Douglas-Peucker, within tolerance x size) and the filled
    faces around the dropped points merged and filled again: about 20 triangles a letter at the default, still
    clearly Rye at headboard distance. Returns None if no font loads."""
    font = _font()
    if font is None:
        lt._log('lettering: neither Rye nor the fallback font could be loaded')
        return None
    curve = bpy.data.curves.new('_lettering', 'FONT')
    curve.body, curve.font, curve.size = body, font, size
    curve.resolution_u = 2
    curve.space_character = spacing
    curve.align_x, curve.align_y = align, 'CENTER'
    src = bpy.data.objects.new('_lettering', curve)
    bpy.context.scene.collection.objects.link(src)
    bpy.context.view_layer.update()
    mesh = bpy.data.meshes.new_from_object(src.evaluated_get(bpy.context.evaluated_depsgraph_get()))
    bpy.data.objects.remove(src)
    bpy.data.curves.remove(curve)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bpy.data.meshes.remove(mesh)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
    tol = tolerance * size
    drop = []
    for loop in _boundary_loops(bm):
        n = len(loop)
        if n < 6:
            continue
        far = max(range(n), key=lambda i: (loop[i].co - loop[0].co).length)
        points = loop + [loop[0]]
        keep = {0, far, n}
        _keep(points, tol, keep, 0, far)
        _keep(points, tol, keep, far, n)
        drop += [loop[i] for i in range(n) if i not in keep]
    if drop:
        bmesh.ops.dissolve_verts(bm, verts=drop, use_face_split=False, use_boundary_tear=False)
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method='BEAUTY', ngon_method='BEAUTY')
    # Two outline points a fraction of a millimetre apart, in line with a third, fill as a sliver with no area (and no
    # tangents in Unreal): collapse each sliver's shortest edge and fill again until none is left.
    for _ in range(8):
        thin = [f for f in bm.faces if f.calc_area() < 1e-8]
        if not thin:
            break
        bmesh.ops.collapse(bm, edges=list({min(f.edges, key=lambda e: e.calc_length()) for f in thin}), uvs=False)
        bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 3], quad_method='BEAUTY',
                              ngon_method='BEAUTY')
    # The curve lies in XY facing +Z: stand it up facing -Y.
    bm.transform(Matrix.Rotation(math.radians(90.0), 4, 'X'))
    bm.normal_update()
    for f in bm.faces:
        if f.normal.y > 0.0:
            f.normal_flip()
    out = bmesh.new()
    out.loops.layers.uv.new('UVMap')
    verts = {v: out.verts.new(v.co) for v in bm.verts}
    for f in bm.faces:
        try:
            out.faces.new([verts[v] for v in f.verts])
        except ValueError:
            pass
    bm.free()
    return out


# --- A model that keeps nothing ---

class NullModel:
    """A stand-in for a kit.Model that builds nothing: a model's code run into it still works out the shapes and
    numbers it defines (openings, roof lines, what another model shares) but makes no mesh, so it is never exported.
    Every building call (emit, box, board, cylinder, hull, socket, section, finish...) does nothing and returns None."""

    def __init__(self, name, seed=1):
        self.name = name
        self.rng = random.Random(seed)
        self.slots, self.hulls, self.sockets = {}, [], []

    def __getattr__(self, attr):
        return lambda *args, **kwargs: None


# --- White paint and lap siding (the clapboard chapel) ---

class Paint:
    """Old white paint, for kit.Model.emit: the house trim's plaster strip G, the only white on the sheet (the town's
    cream clapboards and the fresh headboards use it too), mapped face by face along each face's longer side.
    'clean' squeezes a face's length into the strip's clean stretch, so a long board is one even coat whatever its
    length and nothing of the plaster's cracks shows; 'weathered' maps it at full scale from a random place in the
    stained and flaking stretches (grey where the paint has gone, crazing); 'bare' puts it on the weathered siding
    strip (A): a board that has lost its paint."""
    CLEAN = (4.2, 4.85)                                  # metres along strip G
    STAINED = ((0.0, 0.5), (2.3, 4.15), (5.2, 6.3))

    def __init__(self, mode='clean'):
        self.mode = mode

    def apply(self, obj, rng):
        if self.mode == 'bare':
            import looter_buildings as kit
            kit.Trim('A').apply(obj, rng)
            return
        v0, v1 = lt.TRIM_STRIPS['G']
        strip = (v1 - v0) * lt.TRIM_SIZE / lt.TRIM_DENSITY
        per_m = lt.TRIM_DENSITY / lt.TRIM_SIZE
        inset = 1.0 / lt.TRIM_SIZE
        mesh = obj.data
        uvs = mesh.uv_layers.active.data
        for poly in mesh.polygons:
            u_axis, v_axis = lt._face_axes(poly.normal)
            pts = [mesh.vertices[mesh.loops[i].vertex_index].co for i in poly.loop_indices]
            s, t = [p.dot(u_axis) for p in pts], [p.dot(v_axis) for p in pts]
            if max(t) - min(t) > max(s) - min(s):   # the face runs up: its length along U (turned, not mirrored)
                s, t = t, [-x for x in s]
            s_lo, s_span = min(s), max(max(s) - min(s), 1e-6)
            t_lo, t_span = min(t), max(max(t) - min(t), 1e-6)
            if self.mode == 'clean':
                length = min(s_span, 0.6)
                u0, ku = rng.uniform(self.CLEAN[0], self.CLEAN[1] - length), length / s_span
            else:
                a, b = rng.choice(self.STAINED)
                u0, ku = rng.uniform(a, b), 1.0
            height = min(t_span, strip - 0.06)
            v_start, kv = rng.uniform(0.03, strip - 0.03 - height), height / t_span
            for index, si, ti in zip(poly.loop_indices, s, t):
                u = (u0 + (si - s_lo) * ku) * per_m
                v = v0 + inset + (v_start + (ti - t_lo) * kv) / strip * (v1 - v0 - 2.0 * inset)
                uvs[index].uv = (u, v)


def spans_at(outline, z):
    """The x intervals inside a polygon outline [(x, z), ...] at height z."""
    xs = []
    for (xa, za), (xb, zb) in zip(outline, outline[1:] + outline[:1]):
        if (za <= z < zb) or (zb <= z < za):
            xs.append(xa + (z - za) * (xb - xa) / (zb - za))
    xs.sort()
    return [(xs[i], xs[i + 1]) for i in range(0, len(xs) - 1, 2)]


def _cut(spans, cut):
    c0, c1 = cut
    out = []
    for a, b in spans:
        if c1 <= a or c0 >= b:
            out.append((a, b))
            continue
        if c0 > a:
            out.append((a, c0))
        if c1 < b:
            out.append((c1, b))
    return out


def box_block(x0, x1, z0=-1e9, z1=1e9):
    """A block for lap(): the boards stay out of x0..x1 between z0 and z1 (a corner board, what another wall covers)."""
    return lambda z: [(x0, x1)] if z0 <= z <= z1 else None


def opening_block(o, margin=0.0, sill=0.0):
    """A block for lap() where an opening (a kit Opening: x, z, w, h; opts head, round) and its casing are: the
    opening widened by margin at its sides and over its head, from sill below its foot (doors stand on the wall's
    foot). With sill 0 the boards under a window run on behind its sill."""
    xm = o.x + o.w * 0.5
    if o.opts.get('round'):
        r, zc = o.w * 0.5 + margin, o.z + o.w * 0.5

        def round_block(z):
            d = z - zc
            if abs(d) >= r:
                return None
            half = math.sqrt(r * r - d * d)
            return [(xm - half, xm + half)]
        return round_block
    z_lo = o.z - sill if o.z > 0.05 else -1e9
    zs, head = o.z + o.h, o.opts.get('head', 0.0)
    if head:
        apex = zs + head + margin * 1.3     # the casing's peak over the head's

        def pointed_block(z):
            if z < z_lo or z > apex:
                return None
            half = o.w * 0.5 + margin
            if z > zs:
                half *= (apex - z) / (apex - zs)
            return [(xm - half, xm + half)]
        return pointed_block
    return box_block(o.x - margin, o.x + o.w + margin, z_lo, zs + margin)


def lap(m, space, outline, z0, z1, blocks=(), reveal=0.18, out=0.026, thick=0.014, inner=None, coat=None,
        features=0.0, butt=True):
    """Lap siding (or boards inside) over one face of a wall in kit space `space`: the outer face at y = 0 with the
    boards standing out toward -y, or with inner=y the inner face at that y and the boards toward +y. Rows of about
    `reveal` fill z0..z1 inside the outline (x, z) and outside the blocks (functions z -> [(x0, x1), ...] or None:
    casings, corner boards, openings, what another wall covers). A board is one face tilted so its lower edge stands
    off by `out`, its top tucked under the row above, and (butt) its lower edge, `thick` deep: two quads. Its ends
    follow the outline and the blocks from its foot to its top, so it meets a rake or a pointed head without a gap.
    coat(row, z) gives a row's Paint mode ('clean' by default). butt may be a height: only rows starting below it get
    their butt (high up, seen from far below, the tilt and the baked shading carry the rows). In a long run, `features`
    is the chance of one board of 0.7-1.3 m standing out from it: bare of paint, or sprung (an end standing off).
    Returns the number of boards."""
    import looter_buildings as kit
    rng = m.rng
    coat = coat or (lambda row, z: 'clean')

    def free(z):
        spans = spans_at(outline, z)
        for block in blocks:
            for cut in block(z) or ():
                spans = _cut(spans, cut)
        return [s for s in spans if s[1] - s[0] > 0.03]

    rows = max(1, int(round((z1 - z0) / reveal)))
    h = (z1 - z0) / rows
    sign = 1.0 if inner is None else -1.0
    face_y = 0.0 if inner is None else inner
    made = 0
    for k in range(rows):
        zb, zt = z0 + k * h, z0 + (k + 1) * h
        lo, hi = free(zb + 0.002), free(zt - 0.002)
        if lo and len(lo) == len(hi) and all(max(a[0], b[0]) < min(a[1], b[1]) for a, b in zip(lo, hi)):
            pairs = list(zip(lo, hi))
        else:
            pairs = []
            for a0, a1 in lo:
                for b0, b1 in hi:
                    c0, c1 = max(a0, b0), min(a1, b1)
                    if c1 - c0 > 0.03:
                        pairs.append(((c0, c1), (c0, c1)))
        over = 0.014 if k < rows - 1 else 0.0
        mode = coat(k, zb)
        with_butt = butt if isinstance(butt, bool) else zb < butt
        for (a0, b0), (a1, b1) in pairs:
            if min(b0 - a0, b1 - a1) < 0.03:
                continue
            # Pieces along the run: (foot left, foot right, top left, top right, mode, spring).
            pieces = [(a0, b0, a1, b1, mode, 0.0)]
            lo_x, hi_x = max(a0, a1) + 0.3, min(b0, b1) - 0.3
            if mode == 'clean' and hi_x - lo_x > 1.4 and rng.random() < features:
                length = rng.uniform(0.7, 1.3)
                c = rng.uniform(lo_x, hi_x - length)
                odd = ('bare', 0.0) if rng.random() < 0.6 else ('clean', rng.uniform(0.015, 0.03))
                pieces = [(a0, c, a1, c, mode, 0.0), (c, c + length, c, c + length) + odd,
                          (c + length, b0, c + length, b1, mode, 0.0)]
            for fa, fb, ta, tb_, piece_mode, spring in pieces:
                a_top, b_top = ta + (ta - fa) * over / h, tb_ + (tb_ - fb) * over / h
                y_lo, y_hi = face_y - sign * out, face_y - sign * 0.003
                bm = kit._new_bmesh()
                corners = ((fa, y_lo, zb), (fb, y_lo - sign * spring, zb),
                           (b_top, y_hi - sign * spring * 0.5, zt + over), (a_top, y_hi, zt + over))
                front = [bm.verts.new(c) for c in corners]
                face = bm.faces.new(front)
                if with_butt:
                    back = [bm.verts.new((fa, y_lo + sign * thick, zb)),
                            bm.verts.new((fb, y_lo + sign * thick - sign * spring, zb))]
                    lower = bm.faces.new((front[0], back[0], back[1], front[1]))
                bm.normal_update()
                if face.normal.y * sign > 0.0:
                    face.normal_flip()
                if with_butt and lower.normal.z > 0.0:
                    lower.normal_flip()
                m.emit(bm, Paint(piece_mode), 'trim', None, space)
                made += 1
    return made


# --- Draped shells (rime lying on a mound) ---

def surface_of(obj):
    """z(x, y): the top of obj's mesh there (a ray straight down onto it), or None off it. z.normal(x, y) is the mesh's
    smooth-shaded normal there, and z.points its vertices (all in world space)."""
    from mathutils.bvhtree import BVHTree
    from mathutils import interpolate
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.transform(obj.matrix_world)
    bm.normal_update()
    bm.faces.ensure_lookup_table()
    tree = BVHTree.FromBMesh(bm)
    down = Vector((0.0, 0.0, -1.0))

    def z_at(x, y):
        hit = tree.ray_cast(Vector((x, y, 50.0)), down, 100.0)[0]
        return hit.z if hit is not None else None

    def normal_at(x, y):
        hit, _, index, _ = tree.ray_cast(Vector((x, y, 50.0)), down, 100.0)
        if hit is None:
            return Vector((0.0, 0.0, 1.0))
        verts = bm.faces[index].verts
        weights = interpolate.poly_3d_calc([v.co for v in verts], hit)
        return sum((v.normal * w for v, w in zip(verts, weights)), Vector()).normalized()
    z_at.normal = normal_at
    z_at.points = [v.co.copy() for v in bm.verts]
    z_at.keep = bm   # the tree is built on it
    return z_at


def contour_loops(field, half_w, half_l, cell):
    """Marching squares on field(x, y) over a grid of cell spacing (x and y within +-half_w and +-half_l; keep the
    field negative along the grid's border): the closed loops where it crosses zero, as lists of 2D points, each with
    the inside (field > 0) on its left, so outlines run counterclockwise and holes clockwise."""
    nx, ny = round(2.0 * half_w / cell), round(2.0 * half_l / cell)
    xs = [-half_w + 2.0 * half_w * i / nx for i in range(nx + 1)]
    ys = [-half_l + 2.0 * half_l * j / ny for j in range(ny + 1)]
    f = {}
    for i, x in enumerate(xs):
        for j, y in enumerate(ys):
            v = field(x, y)
            f[i, j] = v if abs(v) > 1e-6 else -1e-6

    def crossing(a, b):
        t = f[a] / (f[a] - f[b])
        return Vector((xs[a[0]] + (xs[b[0]] - xs[a[0]]) * t, ys[a[1]] + (ys[b[1]] - ys[a[1]]) * t))
    nxt, pos = {}, {}
    for i in range(nx):
        for j in range(ny):
            keys = [(i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1)]
            inside = [f[k] > 0.0 for k in keys]
            if all(inside) or not any(inside):
                continue
            ring = []   # the crossings counterclockwise round the cell: (edge, leaving the inside?)
            for k in range(4):
                a, b = keys[k], keys[(k + 1) % 4]
                if inside[k] != inside[(k + 1) % 4]:
                    edge = (min(a, b), max(a, b))
                    pos.setdefault(edge, crossing(a, b))
                    ring.append((edge, inside[k]))
            # A saddle (two opposite corners in) joins them when its middle is in too, and keeps them apart if not.
            step = 1 if len(ring) == 2 or sum(f[k] for k in keys) > 0.0 else -1
            for k, (edge, leaving) in enumerate(ring):
                if leaving:
                    nxt[edge] = ring[(k + step) % len(ring)][0]
    loops, seen = [], set()
    for start in nxt:
        if start in seen:
            continue
        loop, edge = [], start
        while edge not in seen:
            seen.add(edge)
            loop.append(pos[edge])
            edge = nxt[edge]
        loops.append(loop)
    return loops


def loop_area(loop):
    """Signed area of a 2D loop: positive counterclockwise."""
    return 0.5 * sum(a.x * b.y - b.x * a.y for a, b in zip(loop, loop[1:] + loop[:1]))


def _simplify(points, tolerance):
    """Douglas-Peucker on an open 2D polyline: the points it needs to stay within tolerance (both ends kept)."""
    keep = [False] * len(points)
    keep[0] = keep[-1] = True
    stack = [(0, len(points) - 1)]
    while stack:
        i, j = stack.pop()
        a, ab = points[i], points[j] - points[i]
        length2 = ab.length_squared
        best, at = -1.0, -1
        for k in range(i + 1, j):
            t = 0.0 if length2 < 1e-12 else min(1.0, max(0.0, (points[k] - a).dot(ab) / length2))
            d = (points[k] - (a + ab * t)).length
            if d > best:
                best, at = d, k
        if best > tolerance:
            keep[at] = True
            stack += [(i, at), (at, j)]
    return [p for p, k in zip(points, keep) if k]


def tidy_loop(loop, tolerance, smooth=4, small=(12, 0.0012)):
    """A traced loop made soft and light: a small one (fewer than small[0] points, or under small[1] square metres)
    has its corners cut twice so it comes out a rounded fleck instead of a shard; a bigger one is smoothed (smooth
    passes, which rounds off the grid's steps); then both are thinned to tolerance."""
    if len(loop) < small[0] or abs(loop_area(loop)) < small[1]:
        for _ in range(2):
            loop = [q for a, b in zip(loop, loop[1:] + loop[:1]) for q in (a * 0.75 + b * 0.25, a * 0.25 + b * 0.75)]
        tolerance *= 0.7
        if len(loop) <= 10:
            return loop
    else:
        for _ in range(smooth):
            loop = [a * 0.5 + (p + q) * 0.25 for p, a, q in zip(loop[-1:] + loop[:-1], loop, loop[1:] + loop[:1])]
    far = max(range(len(loop)), key=lambda k: (loop[k] - loop[0]).length_squared)
    return _simplify(loop[:far + 1], tolerance)[:-1] + _simplify(loop[far:] + loop[:1], tolerance)[:-1]


def bounded(loops):
    """Loops paired with their bounding boxes, for inside()."""
    return [(loop, (min(p.x for p in loop), min(p.y for p in loop), max(p.x for p in loop), max(p.y for p in loop)))
            for loop in loops]


def inside(loops, x, y):
    """Even-odd: is (x, y) inside the region the bounded() loops enclose?"""
    odd = False
    for loop, (x0, y0, x1, y1) in loops:
        if not (x0 <= x <= x1 and y0 <= y <= y1):
            continue
        for a, b in zip(loop, loop[1:] + loop[:1]):
            if (a.y > y) != (b.y > y) and x < a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y):
                odd = not odd
    return odd


def draped_shell(surface, loops, inner, edge_lift=0.0008, clearance=0.0005):
    """A thin shell draped over surface (a surface_of()), filling the region the loops enclose (outlines and holes):
    a constrained triangulation of the loops' points and the inner points [(Vector2, lift), ...], every point set on
    the surface, the loops' points edge_lift over it and each inner point its own lift. Wherever a fold or a corner
    of a faceted surface would poke up through a triangle, the triangle is raised to clear it by clearance.
    Returns a bmesh (no UVs yet)."""
    from mathutils import geometry
    coords, edges = [], []
    for loop in loops:
        base = len(coords)
        coords += loop
        edges += [(base + k, base + (k + 1) % len(loop)) for k in range(len(loop))]
    count = len(coords)
    coords += [p for p, _ in inner]
    verts, _, faces, orig, _, _ = geometry.delaunay_2d_cdt(coords, edges, [], 0, 1e-6)
    region = bounded(loops)
    bm = bmesh.new()
    bm.loops.layers.uv.new('UVMap')
    out = []
    for k, p in enumerate(verts):
        src = [o for o in orig[k] if o >= count]
        lift = inner[src[0] - count][1] if src else edge_lift
        out.append(bm.verts.new((p.x, p.y, (surface(p.x, p.y) or 0.0) + lift)))
    for face in faces:
        c = sum((verts[k] for k in face), Vector((0.0, 0.0))) / len(face)
        if not inside(region, c.x, c.y):
            continue
        a, b, d = (verts[k] for k in face[:3])
        ring = [out[k] for k in face]
        if (b - a).cross(d - a) < 0.0:
            ring.reverse()
        try:
            bm.faces.new(ring)
        except ValueError:
            pass
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    peaks = getattr(surface, 'points', [])
    for _ in range(4):
        moved = False
        for face in bm.faces:
            a, b, c = (v.co for v in face.verts)
            need = 0.0
            for p in [(a + b + c) / 3.0] + [p.lerp(q, t) for p, q in ((a, b), (b, c), (c, a)) for t in (0.25, 0.5, 0.75)]:
                z = surface(p.x, p.y)
                if z is not None:
                    need = max(need, z + clearance - p.z)
            det = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y)
            if abs(det) > 1e-12:
                x0, x1, y0, y1 = min(a.x, b.x, c.x), max(a.x, b.x, c.x), min(a.y, b.y, c.y), max(a.y, b.y, c.y)
                for q in peaks:
                    if not (x0 <= q.x <= x1 and y0 <= q.y <= y1):
                        continue
                    wa = ((b.x - q.x) * (c.y - q.y) - (c.x - q.x) * (b.y - q.y)) / det
                    wb = ((c.x - q.x) * (a.y - q.y) - (a.x - q.x) * (c.y - q.y)) / det
                    wc = 1.0 - wa - wb
                    if min(wa, wb, wc) >= 0.0:
                        need = max(need, q.z + clearance - (wa * a.z + wb * b.z + wc * c.z))
            if need > 1e-5:
                for v in face.verts:
                    v.co.z += need
                moved = True
        if not moved:
            break
    return bm


def fade_open_edges(obj, mat, amount):
    """Darkens the baked occlusion (the vertex alpha) along the open edges of obj's faces in material mat, to amount
    of what it was: a thin layer (rime) lets the ground show through at its edge, so it reads soft instead of cut.
    Run it after the occlusion bake (finish())."""
    import numpy as np
    mesh = obj.data
    slot = list(mesh.materials).index(mat)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    rim = {v.index for e in bm.edges if e.is_boundary and e.link_faces[0].material_index == slot for v in e.verts}
    bm.free()
    corners = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', corners)
    raw = lt._read_col(mesh)
    raw[np.isin(corners, list(rim)), 3] *= amount
    lt._write_col(mesh, raw)
    return obj


# --- Renders ---

def render(objects, out_png, camera_at, look_at, lens=24.0, sun=None, sun_strength=4.0, resolution=(1280, 720),
           ground=True, ground_z=0.0, samples=48, world_strength=1.0, exposure=0.0):
    """Renders objects (and their children) from a camera at camera_at looking at look_at, under lt.preview's sky and
    color settings. sun is the direction toward the sun (default: lt.preview's afternoon sun for this view). Returns
    the PNG path."""
    scene = bpy.context.scene
    shown = lt._preview_objects(objects)
    out_png = os.path.normpath(out_png if os.path.isabs(out_png) else os.path.join(REPO, out_png))
    os.makedirs(os.path.dirname(out_png), exist_ok=True)
    hidden = {o: o.hide_render for o in scene.objects}
    for o in scene.objects:
        if o not in shown:
            o.hide_render = True
    added = []
    camera = bpy.data.objects.new('_RenderCamera', bpy.data.cameras.new('_RenderCamera'))
    camera.data.lens = lens
    camera.data.sensor_fit = 'HORIZONTAL'
    camera.location = Vector(camera_at)
    camera.rotation_euler = (Vector(look_at) - Vector(camera_at)).to_track_quat('-Z', 'Y').to_euler()
    camera.data.clip_start = 0.05
    camera.data.clip_end = 3000.0
    scene.collection.objects.link(camera)
    added.append(camera)
    if sun is None:
        direction = (Vector(camera_at) - Vector(look_at)).normalized()
        turn = math.atan2(direction.y, direction.x) - math.atan2(-1.6, -1.0)
        base = Vector((-0.55, -0.7, 0.0))
        sun = Vector((base.x * math.cos(turn) - base.y * math.sin(turn), base.x * math.sin(turn) + base.y * math.cos(turn),
                      0.0)).normalized()
        sun = sun * math.cos(math.radians(45.0)) + Vector((0.0, 0.0, math.sin(math.radians(45.0))))
    light = bpy.data.objects.new('_RenderSun', bpy.data.lights.new('_RenderSun', 'SUN'))
    light.data.energy = sun_strength
    light.data.color = (1.0, 0.9, 0.78)
    light.data.angle = math.radians(2.0)
    light.rotation_euler = (-Vector(sun).normalized()).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(light)
    added.append(light)
    if ground:
        size = 3000.0
        mesh = bpy.data.meshes.new('_RenderGround')
        mesh.from_pydata([(-size, -size, ground_z), (size, -size, ground_z), (size, size, ground_z),
                          (-size, size, ground_z)], [], [(0, 1, 2, 3)])
        mat = bpy.data.materials.get('_PreviewGround') or bpy.data.materials.new('_PreviewGround')
        mat.use_nodes = True
        bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
        bsdf.inputs['Base Color'].default_value = lt.hex_color(0x77766c)
        bsdf.inputs['Roughness'].default_value = 1.0
        mesh.materials.append(mat)
        plane = bpy.data.objects.new('_RenderGround', mesh)
        scene.collection.objects.link(plane)
        added.append(plane)
    saved = dict(camera=scene.camera, world=scene.world, engine=scene.render.engine, x=scene.render.resolution_x,
                 y=scene.render.resolution_y, pct=scene.render.resolution_percentage, path=scene.render.filepath,
                 view=scene.view_settings.view_transform, look=scene.view_settings.look,
                 exposure=scene.view_settings.exposure)
    world = lt._preview_world(scene)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value = world_strength
    try:
        scene.camera = camera
        scene.world = world
        scene.render.resolution_x, scene.render.resolution_y = resolution
        scene.render.resolution_percentage = 100
        scene.render.film_transparent = False
        scene.render.image_settings.file_format = 'PNG'
        scene.render.image_settings.color_mode = 'RGB'
        scene.render.filepath = out_png
        scene.view_settings.view_transform = 'AgX'
        scene.view_settings.look = 'AgX - Medium High Contrast'
        scene.view_settings.exposure = exposure
        try:
            scene.render.engine = 'BLENDER_EEVEE_NEXT'
            scene.eevee.taa_render_samples = samples
            if hasattr(scene.eevee, 'use_shadows'):
                scene.eevee.use_shadows = True
            bpy.ops.render.render(write_still=True)
        except Exception as error:
            lt._log(f'render: Eevee failed ({error}); rendering with Workbench')
            scene.render.engine = 'BLENDER_WORKBENCH'
            scene.display.shading.light = 'STUDIO'
            scene.display.shading.color_type = 'TEXTURE'
            bpy.ops.render.render(write_still=True)
    finally:
        scene.camera = saved['camera']
        scene.world = saved['world']
        scene.render.engine = saved['engine']
        scene.render.resolution_x, scene.render.resolution_y = saved['x'], saved['y']
        scene.render.resolution_percentage = saved['pct']
        scene.render.filepath = saved['path']
        scene.view_settings.view_transform = saved['view']
        scene.view_settings.look = saved['look']
        scene.view_settings.exposure = saved['exposure']
        bpy.data.worlds.remove(world)
        for o in added:
            data = o.data
            bpy.data.objects.remove(o)
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
            elif isinstance(data, bpy.types.Camera):
                bpy.data.cameras.remove(data)
            elif isinstance(data, bpy.types.Light):
                bpy.data.lights.remove(data)
        for o, value in hidden.items():
            if o.name in scene.objects:
                o.hide_render = value
    lt._log(f'render: {out_png}')
    return out_png


# --- The churchyard vignette ---

def _run(path):
    """Runs a model script into the current scene; returns the models it made (top-level meshes)."""
    import runpy
    before = set(bpy.context.scene.objects)
    runpy.run_path(path, run_name='__vignette__')
    return {o.name: o for o in bpy.context.scene.objects if o not in before and o.parent is None and o.type == 'MESH'
            and not o.name.startswith(('_', 'UCX_'))}


def _instance(source, name, at, turn=0.0, lean=(0.0, 0.0)):
    """A linked copy of a model (its mesh, not its hulls or sockets) placed in the scene."""
    obj = bpy.data.objects.new(name, source.data)
    bpy.context.scene.collection.objects.link(obj)
    obj.location = at
    obj.rotation_euler = (math.radians(lean[0]), math.radians(lean[1]), math.radians(turn))
    return obj


def vignette(out=None, models=('Chapel', 'Chapel_A')):
    """The churchyard: the chapel on its knoll behind a run of the iron fence with the gate, and a dozen mixed
    headboards and crosses in rows in front of it. One picture for each chapel in models, from the same camera:
    Chapel (the game's, the clapboard church) as RansomsRest/Chapel_overview.png (or out), Chapel_A (the timber-frame
    option, built only for previews and this picture) as RansomsRest/Chapel/Chapel_A_overview.png."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    chapel = _run(os.path.join(REPO, 'Art', 'Models', 'Buildings', 'Chapel.py'))
    kit = _run(os.path.join(REPO, 'Art', 'Models', 'Props', 'Graves.py'))
    shown = []
    bell = chapel.get('ChapelBell')
    if bell is not None:
        shown.append(bell)
    # The kit's own objects stay where the script left them, out of the picture: everything here is a placed copy.
    for obj in kit.values():
        obj.hide_render = True
    rng = random.Random(5)
    # The fence: a run along the front of the yard with the gate in the middle, corner posts, and runs turning back
    # toward the chapel at both ends (a section turned 90 degrees runs along +Y from its post).
    front_y = -13.0
    run = ['Fence_IronSection'] * 4 + ['Fence_IronGate'] + ['Fence_IronSection'] * 4
    x = -9.0
    for k, name in enumerate(run):
        shown.append(_instance(kit[name], f'VigFence{k}', (x, front_y, 0.0)))
        x += 2.0
    for side_x in (-9.0, x):
        shown.append(_instance(kit['Fence_IronPost'], 'VigCorner', (side_x, front_y, 0.0)))
        for k in range(3):
            shown.append(_instance(kit['Fence_IronSection'], 'VigSide', (side_x, front_y + 2.0 * k, 0.0), turn=90.0))
    # Graves in two loose rows each side of the path from the gate to the chapel door, a fresh mound before one.
    boards = ['Grave_Headboard_OldA', 'Grave_Headboard_OldB', 'Grave_Headboard_OldC', 'Grave_Headboard_OldD',
              'Grave_Cross_A', 'Grave_Headboard_FreshA', 'Grave_Cross_B', 'Grave_Headboard_OldB',
              'Grave_Headboard_OldA', 'Grave_Headboard_FreshB', 'Grave_Headboard_OldD', 'Grave_Headboard_OldC']
    spots = []
    for row_y in (front_y + 3.6, front_y + 6.2):
        for gx in (-7.2, -5.4, -3.6, 3.6, 5.4, 7.2):
            spots.append((gx + rng.uniform(-0.25, 0.25), row_y + rng.uniform(-0.2, 0.2)))
    for k, (name, (gx, gy)) in enumerate(zip(boards, spots)):
        if name in kit:
            shown.append(_instance(kit[name], f'VigGrave{k}', (gx, gy, 0.0), turn=rng.uniform(-6.0, 6.0)))
    if 'Grave_MoundFresh' in kit:
        shown.append(_instance(kit['Grave_MoundFresh'], 'VigMound', (spots[5][0], spots[5][1] - 1.15, 0.0)))
    written = []
    for name in models:
        if name not in chapel:
            continue
        # Black crepe over the chapel door, on its socket.
        extra = []
        crepe = next((c for c in chapel[name].children if c.name.startswith('SOCKET_Crepe')), None)
        if crepe is not None and 'CrepeSwag' in kit:
            bpy.context.view_layer.update()
            extra.append(_instance(kit['CrepeSwag'], 'VigCrepe', crepe.matrix_world.translation.copy()))
        bpy.context.view_layer.update()
        if name == 'Chapel':
            path = out or lt.preview_path('RansomsRest', 'Chapel_overview')
        else:
            path = lt.preview_path('RansomsRest/Chapel', f'{name}_overview')
        # The camera takes in the taller steeple (the clapboard chapel's cross, 17.9 m up).
        written.append(render(shown + [chapel[name]] + extra, path, camera_at=(-17.0, -40.0, 7.0),
                              look_at=(1.0, -4.0, 7.8), lens=40.0, sun=(-0.75, -0.55, 0.42), resolution=(1600, 1000)))
        for obj in extra:
            bpy.data.objects.remove(obj)
    return written


if __name__ == '__main__':
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    if 'vignette' in args:
        vignette()
