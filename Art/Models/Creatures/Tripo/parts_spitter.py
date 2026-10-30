"""The Venom Spitter from the user's Tripo model (Art/Models/Creatures/Tripo/Spitter.glb, a copy of
Art/Concepts/Creatures/Spitter Spider/Spitter.glb): its shape. Not a model: Tools\\models.ps1 skips parts_*.py;
Spitter.py runs it (LOOTER_SPITTER_TRIPO, default on), parts_spitter_legs.py builds the legs on the rig's bones and
parts_spitter_look.py textures everything.

What Tripo made (measured 2026-10-29): one watertight shell (28.5k triangles, its front -Y, its left +X) on one 4K atlas,
SIX legs (three a side, not mirror images: the right middle leg's claw came off as a loose piece, dropped here), a
compact cephalothorax (eyes, chelicerae with ivory fangs, pedipalps) under a huge lime venom sac that is the whole
abdomen, brown collar tongues and vein ridges painted on it. It stands nose-down: its back feet 7 units over its front
ones (about 6 degrees).

1. load(): the shell, the loose claw dropped; level(): turned so the plane through its six feet is level and its two
   big eyes lie square across it, centred on x = 0.
2. tube(): a leg, palp or fang measured from its tip: geodesic rings up the mesh give its centre line, rays from that
   line its radius, until the rings widen into the body. The legs only lend their profile (the rig wants eight legs on
   its own bones: parts_spitter_legs.py); the palps and fangs are rebuilt where they are (clean lathes).
3. body_shell(): the cephalothorax rebuilt as a quad cube-sphere cast from its middle onto Tripo's (rays that meet a
   leg's stump, a palp or fang root or the sac filled smoothly from their neighbours: no lumps where those came off);
   sac_shell(): the sac the same way from its middle (an ellipsoid fitted to it where Tripo's shell turns into the
   carapace). Both mirrored across x and smoothed a little. (QuadriFlow on the cut, hole-filled shell was tried first:
   its filled hips stood out as lumps and horns.)
Space here: Tripo's units (about 3.4 cm), Blender axes (front -Y, left +X, up +Z). parts_spitter_legs.place() takes it
to creature space.
"""
import heapq
import math
import os

import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree

import looter_textures as lt
import looter_tripo as tripo

GLB = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Tripo', 'Spitter.glb')
BODY_GRID = 24               # cube-sphere cells per cube edge on the cephalothorax: 6 x 24 x 24 quads
BODY_SMOOTH = 1              # smoothing passes over its radii (Tripo's lumps)
SAC_GRID = 14                # the sac's: 6 x 14 x 14 quads (it is a smooth bulb)
SAC_SMOOTH = 2
JUNCTION_SHARE = 0.95        # the junction plane lies this share of the sac's radius in front of its middle
EYE_LINK = 1.0               # units: eye-coloured vertices this close are one eye
LEG_EXTRA = 3.5              # units past a leg's base also cut away (Tripo's coxae: the rig's legs bring their own)
RING_STEP = 1.0              # units between a tube's geodesic rings (Tripo's leg rings lie about 1.1 apart)


# --- Loading and levelling ---

def load():
    """Tripo's shell (looter_tripo.load: no custom normals, seams merged) with its loose claw dropped, and its maps."""
    obj, maps = tripo.load(GLB)
    pieces = tripo.islands(obj, gun_space=False)
    loose = [f for piece in pieces[1:] for f in piece.faces]
    if loose:
        tripo._delete_faces(obj, loose)
    lt._log(f'Spitter Tripo: {len(obj.data.polygons)} triangles, {len(pieces) - 1} loose piece(s) dropped')
    return obj, maps


def arrays(obj):
    me = obj.data
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    return co.reshape(-1, 3)


def graph(obj):
    """Each vertex's neighbours and edge lengths (for geodesic distances)."""
    co = arrays(obj)
    nbr = [[] for _ in range(len(co))]
    for e in obj.data.edges:
        a, b = e.vertices
        d = float(np.linalg.norm(co[a] - co[b]))
        nbr[a].append((b, d))
        nbr[b].append((a, d))
    return co, nbr


def geodesic(nbr, sources, limit=math.inf):
    d = np.full(len(nbr), np.inf)
    heap = []
    for s in sources:
        d[s] = 0.0
        heap.append((0.0, s))
    heapq.heapify(heap)
    while heap:
        dd, u = heapq.heappop(heap)
        if dd > d[u] or dd > limit:
            continue
        for w, length in nbr[u]:
            nd = dd + length
            if nd < d[w]:
                d[w] = nd
                heapq.heappush(heap, (nd, w))
    return d


def vertex_colours(obj, image, size=1024):
    """Each vertex's base colour (sRGB 0..1), the mean over its corners' texels."""
    px = tripo.pixels(image, min(size, image.size[0]))[..., :3]
    h, w = px.shape[:2]
    me = obj.data
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers.active.data.foreach_get('uv', uv)
    uv = uv.reshape(-1, 2)
    vi = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get('vertex_index', vi)
    rows = np.clip(((1.0 - uv[:, 1] % 1.0) * h).astype(int), 0, h - 1)
    cols = np.clip(((uv[:, 0] % 1.0) * w).astype(int), 0, w - 1)
    out = np.zeros((len(me.vertices), 3))
    count = np.zeros(len(me.vertices))
    np.add.at(out, vi, px[rows, cols])
    np.add.at(count, vi, 1.0)
    return out / np.maximum(count, 1.0)[:, None]


def is_sac(col):
    r, g, b = col[..., 0], col[..., 1], col[..., 2]
    return (g > r * 1.15) & (g > b * 1.4) & (g > 0.35)


def is_eye(col):
    r, g, b = col[..., 0], col[..., 1], col[..., 2]
    return (g > r * 1.2) & (col.max(axis=-1) < 0.35)


def find_tips(obj, nbr, co):
    """The leg tips: geodesic maxima from the carapace's middle, low on the ground (the sac's crown is one too)."""
    middle = int(np.argmin(np.linalg.norm(co - np.array([0.0, -10.0, 36.0]), axis=1)))
    d = geodesic(nbr, [middle])
    tips = []
    for i in range(len(co)):
        if d[i] < 40.0 or co[i, 2] > 15.0:
            continue
        ring = [w for w, _ in nbr[i]]
        if all(d[i] >= d[w] for w in ring) and all(d[i] >= d[x] for w in ring for x, _ in nbr[w]):
            if all(np.linalg.norm(co[i] - co[t]) > 8.0 for t in tips):
                tips.append(i)
    if len(tips) != 6:
        raise RuntimeError(f'Spitter Tripo: expected six leg tips, found {len(tips)}')
    return tips


def level(obj, maps):
    """Turns the shell so the plane through its six feet is level and its big eyes lie square across it, x = 0 between
    them. Returns (vertex colours, the tips' vertex indices)."""
    co, nbr = graph(obj)
    tips = find_tips(obj, nbr, co)
    feet = co[tips]
    a = np.c_[feet[:, 0], feet[:, 1], np.ones(len(feet))]
    sx, sy, _ = np.linalg.lstsq(a, feet[:, 2], rcond=None)[0]
    normal = Vector((-sx, -sy, 1.0)).normalized()
    tilt = normal.rotation_difference(Vector((0.0, 0.0, 1.0))).to_matrix().to_4x4()
    obj.data.transform(tilt)
    co = arrays(obj)
    col = vertex_colours(obj, maps['BC'])
    eyes = is_eye(col) & (co[:, 1] < -10.0) & (co[:, 2] > 33.0)
    left, right = co[eyes & (co[:, 0] > 0.0)], co[eyes & (co[:, 0] < 0.0)]
    across = Vector(left.mean(axis=0) - right.mean(axis=0))
    turn = Matrix.Rotation(-math.atan2(across.y, across.x), 4, 'Z')
    obj.data.transform(turn)
    co = arrays(obj)
    mid = (co[eyes & (co[:, 0] > 0.0)].mean(axis=0) + co[eyes & (co[:, 0] < 0.0)].mean(axis=0)) * 0.5
    obj.data.transform(Matrix.Translation((-mid[0], 0.0, -co[:, 2].min())))
    obj.data.update()
    lt._log(f'Spitter Tripo: levelled by {math.degrees(math.acos(min(1.0, normal.z))):.1f} deg (feet plane), '
            f'turned {math.degrees(math.atan2(across.y, across.x)):.1f} deg (eyes), moved {mid[0]:.2f} across')
    return col, tips


# --- Tubes: legs, palps, fangs ---

def tube(obj, nbr, co, tip, base_count=40, base_radius=2.3, after=0.0, step=RING_STEP, rays=12):
    """A thin part measured from its tip vertex: {'line': (n, 3) its centre line from the tip, 's': arc length there,
    'radius': (n,), 'base': geodesic distance where it meets the body, 'dist': every vertex's geodesic distance}.
    The base is the first ring past `after` (units from the tip) with over base_count vertices and a radius over
    base_radius."""
    d = geodesic(nbr, [tip], 200.0)
    centres, base = [], None
    k = 0
    reach = float(d[np.isfinite(d)].max())
    while k * step < reach:
        sel = (d >= k * step) & (d < (k + 1) * step)
        if not sel.any():      # rings farther apart than a step
            k += 1
            continue
        p = co[sel]
        c = p.mean(axis=0)
        r = float(np.linalg.norm(p - c, axis=1).mean())
        if k * step > after and sel.sum() > base_count * step and r > base_radius:
            base = k * step
            break
        centres.append(c)
        k += 1
    if base is None:
        raise RuntimeError(f'Spitter Tripo: the tube from vertex {tip} never met the body')
    line = np.array([co[tip]] + centres)
    # Smoothed (the rings tilt where the tube bends), then resampled evenly.
    kernel = np.array([1.0, 2.0, 3.0, 2.0, 1.0])
    kernel /= kernel.sum()
    padded = np.vstack([line[:1]] * 2 + [line] + [line[-1:]] * 2)
    smooth = np.stack([np.convolve(padded[:, i], kernel, mode='valid') for i in range(3)], axis=1)
    smooth[0] = line[0]
    seg = np.linalg.norm(np.diff(smooth, axis=0), axis=1)
    s = np.r_[0.0, np.cumsum(seg)]
    even = np.arange(0.0, s[-1], step * 0.5)
    line = np.stack([np.interp(even, s, smooth[:, i]) for i in range(3)], axis=1)
    me = obj.data
    tree = BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])
    radius = np.zeros(len(line))
    for i in range(len(line)):
        a, b = line[max(i - 1, 0)], line[min(i + 1, len(line) - 1)]
        t = Vector(b - a).normalized()
        side = t.orthogonal().normalized()
        up = t.cross(side)
        hits = []
        for j in range(rays):
            ang = 2.0 * math.pi * j / rays
            ray = side * math.cos(ang) + up * math.sin(ang)
            hit = tree.ray_cast(Vector(line[i]), ray, 8.0)[0]
            if hit is not None:
                hits.append((hit - Vector(line[i])).length)
        radius[i] = float(np.median(hits)) if hits else 0.0
    radius[0] = 0.0
    # A median over five stations takes out single rays that slipped through a seam.
    radius = np.array([np.median(radius[max(0, i - 2):i + 3]) for i in range(len(radius))])
    radius[0] = 0.0
    return {'line': line, 's': even, 'radius': radius, 'base': base, 'dist': d}


def head_tips(co, col, legs):
    """The left fang's and left palp's tip vertices (x > 0; the right ones are their mirrors): the lowest ivory vertex
    under the face, and the lowest vertex of the head's sides that no leg owns."""
    on_leg = np.zeros(len(co), bool)
    for t in legs:
        on_leg |= t['dist'] < t['base']
    left = co[:, 0] > 0.0
    ivory = (col.min(axis=1) > 0.6) & (co[:, 1] < -12.0) & left
    fang = np.where(ivory)[0]
    fang = int(fang[np.argmin(co[fang, 2])])
    side = (co[:, 1] < -8.0) & (co[:, 0] > 3.0) & (co[:, 0] < 14.0) & (co[:, 2] < 28.0) & ~on_leg & ~ivory
    palp = np.where(side)[0]
    palp = int(palp[np.argmin(co[palp, 2])])
    return fang, palp


def knee(t):
    """Index of the knee on a leg's centre line: the point farthest from the line between its tip and its hip end."""
    line = t['line']
    a, b = line[0], line[-1]
    ab = (b - a) / np.linalg.norm(b - a)
    off = line - a
    dist = np.linalg.norm(off - np.outer(off @ ab, ab), axis=1)
    return int(np.argmax(dist))


# --- Radial shells: the cephalothorax and the sac rebuilt as quad cube-spheres ---

def cube_sphere(n):
    """A quad cube-sphere: unit directions (m, 3), quads (index tuples) and each vertex's neighbours, n cells along each
    cube edge, spread evenly (tan warp). All quads, no poles."""
    verts, index, faces = [], {}, []

    def vid(p):
        key = tuple(np.round(p, 6))
        if key not in index:
            index[key] = len(verts)
            verts.append(p)
        return index[key]

    g = [math.tan((k / n - 0.5) * math.pi / 2.0) for k in range(n + 1)]
    for axis in range(3):
        for sign in (-1.0, 1.0):
            u_ax, v_ax = [a for a in range(3) if a != axis]
            grid = []
            for i in range(n + 1):
                row = []
                for j in range(n + 1):
                    p = np.zeros(3)
                    p[axis], p[u_ax], p[v_ax] = sign, g[i], g[j]
                    row.append(vid(p / np.linalg.norm(p)))
                grid.append(row)
            for i in range(n):
                for j in range(n):
                    quad = (grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1])
                    faces.append(quad if (sign > 0) == (axis != 1) else quad[::-1])
    adj = [set() for _ in verts]
    for q in faces:
        for a, b in zip(q, q[1:] + q[:1]):
            adj[a].add(b)
            adj[b].add(a)
    return np.array(verts), faces, [sorted(a) for a in adj]


def _mean_over(values, adj):
    return np.array([np.mean(values[a]) for a in adj])


def _mirror_index(dirs):
    flipped = dirs * np.array([-1.0, 1.0, 1.0])
    return np.argmax(flipped @ dirs.T, axis=1)


def tree_of(obj):
    me = obj.data
    return BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])


def radial(tree, face_ok, centre, dirs, adj, reach=80.0, fallback=None, smooth=1, fill_passes=400):
    """Each direction's radius from centre to the first surface, where face_ok(face, distance, i) takes it; the rest (a
    ray into a cut leg's stump, into the sac) filled smoothly from their neighbours (harmonic) or, given a fallback,
    blended onto it. Mirrored across x, then smoothed `smooth` times (Tripo's lumps softened). Returns (radius, trust)."""
    radius = np.zeros(len(dirs))
    trust = np.zeros(len(dirs))
    for i, d in enumerate(dirs):
        hit, _n, face, dist = tree.ray_cast(Vector(centre), Vector(d), reach)
        if hit is not None and face_ok(face, dist, i):
            radius[i], trust[i] = dist, 1.0
    if fallback is not None:
        soft = trust.copy()
        for _ in range(3):
            soft = np.where(soft >= 1.0, 1.0, _mean_over(soft, adj))
        radius = np.where(trust > 0, radius, fallback) * soft + fallback * (1.0 - soft)
    else:
        known = trust > 0
        radius[~known] = radius[known].mean()
        for _ in range(fill_passes):
            radius[~known] = _mean_over(radius, adj)[~known]
    mirror = _mirror_index(dirs)
    radius = 0.5 * (radius + radius[mirror])
    for _ in range(smooth):
        radius = 0.5 * radius + 0.5 * _mean_over(radius, adj)
    return radius, trust


def sac_shell(obj, col, tree=None):
    """The sac: {'centre', 'dirs', 'radius', 'raw', 'quads', 'adj'}. Each direction's radius is where a ray from the sac's
    middle meets Tripo's sac; where it meets carapace or collar instead (the front underside), an ellipsoid fitted to
    the rest. 'raw' is Tripo's own (unmirrored, unsmoothed): what the body gives up to the sac."""
    co = arrays(obj)
    sac = is_sac(col)
    lo, hi = co[sac].min(axis=0), co[sac].max(axis=0)
    centre = (lo + hi) * 0.5
    centre[0] = 0.0
    half = (hi - lo) * 0.5
    dirs, quads, adj = cube_sphere(SAC_GRID)
    tree = tree or tree_of(obj)
    sac_face = np.array([sac[list(p.vertices)].mean() > 0.5 for p in obj.data.polygons])
    ell = 1.0 / np.sqrt(((dirs / half) ** 2).sum(axis=1))
    first = {}

    def on_sac(face, dist, i):
        first[i] = dist
        return sac_face[face] and 0.8 * ell[i] < dist < 1.2 * ell[i]

    _probe, trust = radial(tree, on_sac, centre, dirs, adj, fallback=ell, smooth=0)
    # The ellipsoid scaled onto the measured part, so its fill meets it; then the cast again with it.
    k = float(np.median([first[i] / ell[i] for i in range(len(dirs)) if trust[i] > 0]))
    ell = ell * k
    radius, trust = radial(tree, on_sac, centre, dirs, adj, fallback=ell, smooth=SAC_SMOOTH)
    # (A ray that runs on through the sac's mouth into the carapace meets the head's inside: only sac hits count.)
    raw = np.array([max(first[i], ell[i]) if trust[i] > 0 else ell[i] for i in range(len(dirs))])
    lt._log(f'Spitter Tripo: sac centre {np.round(centre, 2).tolist()}, half {np.round(half * k, 2).tolist()}, '
            f'{int(trust.sum())}/{len(dirs)} rays on the sac')
    return {'centre': centre, 'dirs': dirs, 'radius': radius, 'raw': raw, 'quads': quads, 'adj': adj}


def junction(sac, body_centre):
    """The plane where the cephalothorax ends and the sac begins: (point, normal toward the sac). Anything of Tripo's past
    it (the collar tongues lying on the sac, the carapace under its front) is the sac's."""
    axis = sac['centre'] - np.asarray(body_centre, float)
    axis /= np.linalg.norm(axis)
    toward = int(np.argmax(sac['dirs'] @ -axis))
    return sac['centre'] - axis * sac['radius'][toward] * JUNCTION_SHARE, axis


def body_mask(obj, tubes, sac, col, body_centre):
    """Vertices kept for the body: not on a leg, palp or fang (geodesically within its base), not on or in the sac (within
    6% of Tripo's, or sac-coloured) and on the head's side of the junction."""
    co = arrays(obj)
    keep = np.ones(len(co), bool)
    for t in tubes:
        keep &= t['dist'] >= t['base'] + t.get('extra', -0.25)
    rel = co - sac['centre']
    dist = np.linalg.norm(rel, axis=1)
    unit = rel / np.maximum(dist, 1e-6)[:, None]
    nearest = np.argmax(unit @ sac['dirs'].T, axis=1)
    keep &= (dist > sac['raw'][nearest] * 1.06) & ~is_sac(col)
    point, axis = junction(sac, body_centre)
    keep &= (co - point) @ axis < 0.0
    return keep


def body_shell(obj, keep, centre, tree=None):
    """The cephalothorax as a cube-sphere cast from centre onto Tripo's: {'centre', 'dirs', 'radius', 'quads', 'adj',
    'trust'}. Rays that meet a leg's stump, a palp or fang root or the sac are filled from their neighbours, so the
    shell closes smoothly where those were."""
    dirs, quads, adj = cube_sphere(BODY_GRID)
    tree = tree or tree_of(obj)
    ok = np.array([all(keep[v] for v in p.vertices) for p in obj.data.polygons])
    radius, trust = radial(tree, lambda face, dist, i: ok[face], centre, dirs, adj, reach=40.0, smooth=BODY_SMOOTH)
    lt._log(f'Spitter Tripo: body shell {len(quads)} quads, {int(trust.sum())}/{len(dirs)} rays on the carapace')
    return {'centre': np.asarray(centre, float), 'dirs': dirs, 'radius': radius, 'quads': quads, 'adj': adj,
            'trust': trust}


def shell_points(shell, scale=1.0):
    return shell['centre'] + shell['dirs'] * (shell['radius'] * scale)[:, None]


def eyes(obj, col, keep):
    """The eyes on Tripo's head (left side; the right are their mirrors): [(centre, radius)], biggest first: clusters of
    the glossy eye colour."""
    co = arrays(obj)
    pick = np.where(is_eye(col) & keep & (co[:, 0] > 0.0) & (co[:, 1] < -10.0))[0]
    kd = KDTree(len(pick))
    for k, i in enumerate(pick):
        kd.insert(co[i], k)
    kd.balance()
    label = -np.ones(len(pick), int)
    found = []
    for k in range(len(pick)):
        if label[k] >= 0:
            continue
        label[k] = len(found)
        stack, members = [k], []
        while stack:
            m = stack.pop()
            members.append(m)
            for _p, j, _d in kd.find_range(co[pick[m]], EYE_LINK):
                if label[j] < 0:
                    label[j] = label[k]
                    stack.append(j)
        found.append(co[pick[members]])
    out = []
    for pts in found:
        if len(pts) < 3:
            continue
        c = pts.mean(axis=0)
        out.append((c, float(np.linalg.norm(pts - c, axis=1).max())))
    out.sort(key=lambda e: -e[1])
    return out
