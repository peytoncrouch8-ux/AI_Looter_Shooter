"""The heroes' melee tools from the user's Tripo remodels (2026-10-28): how MeleeTools.py swaps each scripted tool for
its Tripo model (Art/Models/Weapons/Melee/Tripo/<Tool>.glb, made from Art/Concepts/MeleeTools/<Tool>/upload). Not a
model: Tools\\models.ps1 skips parts_*.py. The user's bar (2026-10-28): crisp textures with worn metal and edge wear like
the gun parts, clean shapes (no lumps, straight edges, no holes, sharp silhouettes), our style; nothing reads as AI-made.

swap() per tool:
1. looter_tripo.load() and fit() onto the scripted model's box (tool space, cm: u up the handle, v the strike side, w
   across), so the length, size and every socket (copied from the scripted model) stay where the swings, the hand, the
   tip's smear and the effects expect them.
2. The tool's prepare() fixes what Tripo got wrong (below), measures its shape into CTX and may hand back pieces to add
   (scripted ones, or lathes rebuilt on Tripo's own profile: relathe).
3. Rounded off (one Catmull-Clark level, edges over 75 degrees creased), collapse-decimated to the spec's triangles,
   then the pick's handle made truly round (turn_true: Tripo builds turned parts as octagons); 11-14k triangles in
   all (seen big in first person), LODs 50% and 20%.
4. Our look baked onto Tripo's atlas (Tools/Blender/looter_rebake.py): the scripted model's materials (texture sets,
   tints, crisp borders: rings, bands, pips, edges) cast onto the mesh; painted layers drawn texel by texel on Tripo's
   own shape where it strays from the scripted one (*_PAINT); edge wear and crease grime from its shape; a normal map
   from the new colour map's fine relief (Tripo's own is dropped: it no longer fits the rounded mesh and its lumps read
   as AI-made). Maps 2048 px in Art/Textures/TripoMelee<Tool>/ (material MeleeTripo<Tool> on the World master);
   re-baked only when the .glb or a script is newer than the maps.
5. Faces on the scripted glow slots (MeleeEmberCyan, MeleeEmberOrange) take those materials.

Fixes (checked against the upload views and the README's picks):
  GraveSpade  the blade came out with a fin standing off both faces, a gouged crack and white streaks smeared over the
              edge: Tripo keeps the D-grip, ferrule and socket (cut at u 46.9, capped); the blade, crack, tread and
              frog are the scripted ones (clean worn-bright edge band, the cyan crack), with an iron collar closing the
              socket's top; the shaft is a lathe on Tripo's profile (its octagon left dents when rounded). Tripo's
              socket starts lower than the scripted one (painted iron from there).
  RazorFan    lumpy card faces and torn pip bumps: the faces are pressed flat (their w smoothed, the outlines kept).
  FerryGaff   the pole sat 0.2 cm off the axis (re-centred), lumpy and octagonal: a lathe on its profile (bands and
              the butt cap iron); the fare coin was paint only: the scripted coin and nail wrapped round the pole.
  PistonFist  the yoke came out as a hollow wire frame (cut away, the scripted solid yoke in); the brass rear ring came
              out a washer with a 2.8 cm bore (cut away, the scripted ring in); the front ring's bore pushed out to
              4.7 cm (Gauge's forearm); the cylinder came out lumpy with uneven bands, lower and fatter than the
              scripted one: the scripted cylinder, flanges and bands scaled to Tripo's (1.27) on Tripo's axis.
  RockPick    clean; the handle made round, its wood re-laid with a straight grain; the brass wire binding (Tripo
              painted it wood) comes from the bake.
"""
import os

import bmesh
import bpy
import numpy as np

import looter_model as lm
import looter_textures as lt

TRIPO_DIR = 'Art/Models/Weapons/Melee/Tripo/'
# The .glb's axes (Tripo stands the tool up as in the views: +Z its length, -Y the front view's side): see
# Tools/Blender/melee_views.py. The spade and the fist show +v in front; the others +w.
V_FRONT = dict(forward='+Z', left='-X', up='-Y')
W_FRONT = dict(forward='+Z', left='-Y', up='+X')
LODS = '50,20'
LOD_SCREENS = (0.3, 0.12)    # the guns' body switch sizes, scaled by the tool's length against a body's (0.522 m)
BODY_LENGTH = 0.522
CM = 0.01


def uvw(obj):
    """The mesh's vertices in tool space, cm (u, v, w)."""
    co = np.empty(len(obj.data.vertices) * 3)
    obj.data.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3) / CM
    return np.stack([-co[:, 1], co[:, 2], co[:, 0]], axis=1)


def face_centres(obj):
    c = np.empty(len(obj.data.polygons) * 3)
    obj.data.polygons.foreach_get('center', c)
    c = c.reshape(-1, 3) / CM
    return np.stack([-c[:, 1], c[:, 2], c[:, 0]], axis=1)


def delete_faces(obj, faces):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[bm.faces[i] for i in set(faces)], context='FACES')
    bm.to_mesh(obj.data)
    bm.free()


def cut_above(obj, u):
    """Cuts the mesh across at u, keeps what lies below and caps the cut."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
    res = bmesh.ops.bisect_plane(bm, geom=geom, plane_co=(0.0, -u * CM, 0.0), plane_no=(0.0, -1.0, 0.0),
                                 clear_outer=True)
    cut = [e for e in res['geom_cut'] if isinstance(e, bmesh.types.BMEdge) and e.is_boundary]
    if cut:
        filled = bmesh.ops.holes_fill(bm, edges=cut, sides=0)
        bmesh.ops.triangulate(bm, faces=filled['faces'])
    bm.to_mesh(obj.data)
    bm.free()


def tris(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def round_off(obj, hard=75.0):
    """One level of Catmull-Clark subdivision with every edge sharper than hard degrees creased: Tripo's handles,
    rings and shafts come out as octagons and hexagons with lumps; this rounds and smooths them while corners and
    bevels that are meant to be hard stay hard. UVs follow (boundaries kept). Decimation takes the count back down."""
    import math
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    layer = bm.edges.layers.float.get('crease_edge') or bm.edges.layers.float.new('crease_edge')
    limit = math.radians(hard)
    creased = 0
    for e in bm.edges:
        sharp = (not e.is_manifold) or (len(e.link_faces) == 2 and e.calc_face_angle(0.0) > limit)
        e[layer] = 1.0 if sharp else 0.0
        creased += sharp
    bm.to_mesh(obj.data)
    bm.free()
    mod = obj.modifiers.new('Round', 'SUBSURF')
    mod.levels = mod.render_levels = 1
    mod.uv_smooth = 'PRESERVE_BOUNDARIES'
    mod.boundary_smooth = 'PRESERVE_CORNERS'
    mod.use_creases = True
    with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj]):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    attr = obj.data.attributes.get('crease_edge')
    if attr:
        obj.data.attributes.remove(attr)
    return creased


def decimate(obj, target):
    now = tris(obj)
    if now <= target:
        return
    mod = obj.modifiers.new('Decimate', 'DECIMATE')
    mod.ratio = target / now
    mod.use_collapse_triangulate = True
    with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj]):
        bpy.ops.object.modifier_apply(modifier=mod.name)


def face_normals(obj):
    n = np.empty(len(obj.data.polygons) * 3)
    obj.data.polygons.foreach_get('normal', n)
    n = n.reshape(-1, 3)
    return np.stack([-n[:, 1], n[:, 2], n[:, 0]], axis=1)


def recentre_pole(obj, u0, u1, blend_to):
    """Moves the pole's axis onto the tool's (v = w = 0) between u0 and u1 (measured there), easing the shift out to
    nothing at blend_to: the bounding-box fit leaves a pole a little off its axis when the head stands out on one side."""
    p = uvw(obj)
    band = p[(p[:, 0] > u0) & (p[:, 0] < u1)]
    dv = 0.5 * (band[:, 1].min() + band[:, 1].max())
    dw = 0.5 * (band[:, 2].min() + band[:, 2].max())
    k = np.clip((blend_to - p[:, 0]) / max(blend_to - u1, 1e-3), 0.0, 1.0)
    p[:, 1] -= dv * k
    p[:, 2] -= dw * k
    co = np.stack([p[:, 2], -p[:, 0], p[:, 1]], axis=1) * CM
    obj.data.vertices.foreach_set('co', co.ravel())
    obj.data.update()
    return dv, dw


def open_bore(obj, u0, u1, radius):
    """Pushes every vertex inside radius (cm, round the u axis) between u0 and u1 out onto it: the clamp rings' bore,
    which must clear the forearm's casing."""
    p = uvw(obj)
    r = np.hypot(p[:, 1], p[:, 2])
    inside = (p[:, 0] > u0) & (p[:, 0] < u1) & (r < radius) & (r > radius * 0.6)
    scale = np.where(inside, radius / np.maximum(r, 1e-6), 1.0)
    p[:, 1] *= scale
    p[:, 2] *= scale
    co = np.stack([p[:, 2], -p[:, 0], p[:, 1]], axis=1) * CM
    obj.data.vertices.foreach_set('co', co.ravel())
    obj.data.update()
    return int(inside.sum()), float(r[(p[:, 0] > u0) & (p[:, 0] < u1) & (r > radius * 0.6)].min(initial=99.0))


def turn_true(obj, u0, u1, r_max, axis=(0.0, 0.0), keep=None, step=0.3, side=0.8):
    """Makes a turned part round (a handle, a shaft, a pole, their bands): Tripo builds them as octagons, which read as
    flat facets up close in first person. Faces lying between u0 and u1 within r_max cm of the axis (its (v, w) along u;
    keep(dv, dw) may narrow it: the fist's cylinder only above its saddle) are split once (each edge halved: an octagon
    becomes a 16-gon), then each vertex on the side wall (past side times its slice's widest) is pushed out onto the
    ellipse through its slice's widest reach along v and w, about the slice's middle (the axis's v for a part cut by
    keep). Slices are step cm long, so bands and tapers keep their profile. Returns the vertices moved."""
    av, aw = axis

    def rel(co):
        return -co.y / CM, co.z / CM - av, co.x / CM - aw

    def ok(co):
        u, a, b = rel(co)
        return u0 <= u <= u1 and np.hypot(a, b) <= r_max and (keep is None or keep(a, b))

    bm = bmesh.new()
    bm.from_mesh(obj.data)
    faces = [f for f in bm.faces if all(ok(v.co) for v in f.verts)]
    edges = list({e for f in faces for e in f.edges})
    if not edges:
        bm.free()
        return 0
    bmesh.ops.subdivide_edges(bm, edges=edges, cuts=1, use_grid_fill=True)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 3])
    verts = [v for v in bm.verts if ok(v.co)]
    p = np.array([rel(v.co) for v in verts])
    # Each slice's reach and middle, smoothed along u (a median of three) and read between slices per vertex: slice by
    # slice, the few vertices in each gave a noisy radius that rippled the surface into streaks along the handle.
    bins = np.floor((p[:, 0] - u0) / step).astype(int)
    rows = []
    for k in np.unique(bins):
        m = bins == k
        if m.sum() < 6:
            continue
        cv = 0.0 if keep is not None else 0.5 * (p[m, 1].max() + p[m, 1].min())
        cw = 0.5 * (p[m, 2].max() + p[m, 2].min())
        rows.append((u0 + (k + 0.5) * step, cv, cw, np.abs(p[m, 1] - cv).max(), np.abs(p[m, 2] - cw).max()))
    if len(rows) < 2:
        bm.to_mesh(obj.data)
        bm.free()
        return 0
    t = np.array(rows)
    for col in range(1, 5):
        x = t[:, col]
        t[1:-1, col] = np.median(np.stack([x[:-2], x[1:-1], x[2:]]), axis=0)
    cv, cw, ev, ew = (np.interp(p[:, 0], t[:, 0], t[:, col]) for col in range(1, 5))
    dv, dw = p[:, 1] - cv, p[:, 2] - cw
    e = np.hypot(dv / np.maximum(ev, 1e-3), dw / np.maximum(ew, 1e-3))   # 1 on the slice's ellipse
    moved = 0
    for i in np.nonzero(e > side)[0]:
        s = 1.0 / e[i]
        verts[i].co.z = (av + cv[i] + dv[i] * s) * CM
        verts[i].co.x = (aw + cw[i] + dw[i] * s) * CM
        moved += 1
    bm.normal_update()
    bm.to_mesh(obj.data)
    bm.free()
    return moved


def relathe(mt, obj, u0, u1, base, band='IRON', bands=True, force_bands=(), build=True, axis=(0.0, 0.0),
            top_only=False, caps=(True, True),
            tuck=(0.0, 0.0), step=0.25, segments=40):
    """Rebuilds a turned part as a clean lathe on Tripo's own profile: its radius along u (measured in step cm slices,
    a median of three) with the bands (where it stands 0.08 cm proud of the part around it) squared off flat at their
    widest. Tripo's faces inside that profile between u0 and u1 go; the lathe (segments round, caps at the ends asked
    for, tuck cm more at each end to slip inside the neighbouring part) comes back as pieces in base and band materials
    (MeleeTools.py names: wood mapped with its grain along u, metal box-mapped like the scripted parts). axis: its
    (v, w); top_only: measure only its top (the fist's cylinder rises out of its saddle and has a vent on one side).
    Returns (pieces, profile rows (u, r))."""
    import bmesh as _bm
    av, aw = axis
    p = uvw(obj)
    dv, dw = p[:, 1] - av, p[:, 2] - aw
    rows = []
    for u in np.arange(u0, u1, step):
        m = (p[:, 0] >= u) & (p[:, 0] < u + step)
        if top_only:
            m &= (dv > 0.0) & (np.abs(dw) < 0.35 * np.abs(dv).max(initial=1.0))
        if m.sum() < 3:
            continue
        r = dv[m].max() if top_only else 0.5 * (np.abs(dv[m]).max() + np.abs(dw[m]).max())
        rows.append((u + 0.5 * step, r))
    t = np.array(rows)
    t[1:-1, 1] = np.median(np.stack([t[:-2, 1], t[1:-1, 1], t[2:, 1]]), axis=0)
    # The part's own radius round each slice (a running median over 3 cm) tells bands from body.
    body = np.array([np.median(t[np.abs(t[:, 0] - u) < 1.5, 1]) for u in t[:, 0]])
    proud = (t[:, 1] > body + 0.08) & bands
    for a, b in force_bands:
        proud |= (t[:, 0] > a) & (t[:, 0] < b)
    # Faces inside the profile go.
    c = face_centres(obj)
    rc = np.interp(c[:, 0], t[:, 0], t[:, 1])
    d = np.hypot(c[:, 1] - av, c[:, 2] - aw)
    doomed = (c[:, 0] >= u0) & (c[:, 0] <= u1) & (d < rc + 0.12)
    delete_faces(obj, np.nonzero(doomed)[0].tolist())
    if not build:
        return [], t
    # Runs of body and band, each its own piece; a band flat at its widest, its edges a little bevelled.
    runs, k = [], 0
    while k < len(t):
        j = k
        while j + 1 < len(t) and proud[j + 1] == proud[k]:
            j += 1
        runs.append((bool(proud[k]), k, j))
        k = j + 1
    pieces = []
    for n, (is_band, k, j) in enumerate(runs):
        ua = t[k, 0] - 0.5 * step if n else t[0, 0] - 0.5 * step - tuck[0]
        ub = t[j, 0] + 0.5 * step if n < len(runs) - 1 else t[-1, 0] + 0.5 * step + tuck[1]
        if is_band:
            r = t[k:j + 1, 1].max()
            prev = t[k - 1, 1] if k else r
            nxt = t[j + 1, 1] if j + 1 < len(t) else r
            bev = min(0.08, 0.25 * (ub - ua))
            ring = [(ua, prev), (ua, r - bev), (ua + bev, r), (ub - bev, r), (ub, r - bev), (ub, nxt)]
        else:
            # The body evened out (a running median over 1 cm: Tripo's lumps) and thinned to the rings it needs.
            us = t[k:j + 1, 0]
            rs = np.array([np.median(t[k:j + 1][np.abs(us - u) <= 0.5, 1]) for u in us])
            ring = _simplify([(ua, rs[0])] + list(zip(us, rs)) + [(ub, rs[-1])], 0.025)
            # A tuck slipping into the neighbouring part runs a little slimmer, so it doesn't fight Tripo's own skin there.
            if n == 0 and tuck[0] > 0.0:
                ring = [(ua, ring[0][1] * 0.96), (ua + tuck[0], ring[0][1] * 0.96)] + ring[1:]
            if n == len(runs) - 1 and tuck[1] > 0.0:
                ring = ring[:-1] + [(ub - tuck[1], ring[-1][1] * 0.96), (ub, ring[-1][1] * 0.96)]
        ring = [pt for i, pt in enumerate(ring) if i == 0 or abs(pt[0] - ring[i - 1][0]) > 1e-4 or abs(pt[1] - ring[i - 1][1]) > 1e-4]
        bm = _bm.new()
        loops = []
        for u, r in ring:
            loops.append([bm.verts.new((aw * CM + r * CM * np.sin(2 * np.pi * s / segments), -u * CM,
                                        av * CM + r * CM * np.cos(2 * np.pi * s / segments))) for s in range(segments)])
        uvl = bm.loops.layers.uv.new('UVMap')
        scale = mt['DENSITY'] / 1024.0
        for a, b in zip(loops, loops[1:]):
            for s in range(segments):
                s1 = (s + 1) % segments
                f = bm.faces.new((a[s], b[s], b[s1], a[s1]))
                for loop, ang in zip(f.loops, (s, s, s + 1, s + 1)):
                    loop[uvl].uv = (-loop.vert.co.y * scale, 2 * np.pi * ang / segments * 0.018 * scale)
        if (n == 0 and caps[0]) or (n == len(runs) - 1 and caps[1]):
            for end in ([0] if n == 0 and caps[0] else []) + ([len(loops) - 1] if n == len(runs) - 1 and caps[1] else []):
                f = bm.faces.new(loops[end])
                for loop in f.loops:
                    loop[uvl].uv = (loop.vert.co.x * scale, loop.vert.co.z * scale)
        _bm.ops.recalc_face_normals(bm, faces=bm.faces)
        obj_piece = lp_mesh(bm)
        name = band if is_band else base
        mat = mt[name]
        if name in ('ASH', 'ASH_GREY', 'HICKORY'):
            obj_piece.data.materials.append(mat)
        else:
            mt['mapped'](obj_piece, mat)
        pieces.append(obj_piece)
    return pieces, t


def lp_mesh(bm):
    import looter_props as lp
    return lp.mesh_object(bm)


def _simplify(pts, tol):
    """Douglas-Peucker on [(u, r)]: the fewest points whose line stays within tol cm of the rest (ends kept)."""
    if len(pts) < 3:
        return list(pts)
    (u0, r0), (u1, r1) = pts[0], pts[-1]
    worst, at = 0.0, 0
    for i in range(1, len(pts) - 1):
        u, r = pts[i]
        line = r0 + (r1 - r0) * (u - u0) / max(u1 - u0, 1e-6)
        if abs(r - line) > worst:
            worst, at = abs(r - line), i
    if worst <= tol:
        return [pts[0], pts[-1]]
    return _simplify(pts[:at + 1], tol)[:-1] + _simplify(pts[at:], tol)


# --- Per-tool fixes: prepare(mt, obj) -> scripted pieces to add (mt: MeleeTools.py's namespace). They also measure
# Tripo's own shape into CTX for the painted layers below. ---

CTX = {}


def _profile_start(obj, u0, u1, radius, step=0.25):
    """The first u (scanning up from u0) where the mesh reaches out past radius round the u axis."""
    p = uvw(obj)
    r = np.hypot(p[:, 1], p[:, 2])
    for u in np.arange(u0, u1, step):
        m = (p[:, 0] >= u) & (p[:, 0] < u + step)
        if m.any() and r[m].max() > radius:
            return float(u)
    return u1


def _log_lathe(key, what, prof, pieces):
    lt._log(f'Tripo {key}: {what} rebuilt as a lathe on its profile (u {prof[0, 0]:.1f}..{prof[-1, 0]:.1f}, radius '
            f'{prof[:, 1].min():.2f}..{prof[:, 1].max():.2f} cm, {len(pieces)} pieces)')


def spade_prepare(mt, obj):
    CTX['socket_u'] = _profile_start(obj, 28.0, 40.0, 1.7)
    lt._log(f'Tripo GraveSpade: the socket starts at u {CTX["socket_u"]:.2f}')
    cut_above(obj, 46.9)
    shaft, prof = relathe(mt, obj, -4.2, CTX['socket_u'] - 0.3, 'ASH', bands=False, caps=(False, False),
                          tuck=(0.6, 0.8))
    _log_lathe('GraveSpade', 'the shaft', prof, shaft)
    collar = mt['turned']([(0.0, 2.53), (2.2, 2.6), (3.0, 2.05)], 46.6, segments=16)
    return mt['spade_head']() + [mt['mapped'](collar, mt['IRON'])] + shaft


def flatten(obj, facing=0.82, passes=40):
    """Presses lumps out of flat faces lying across w (cards): each vertex whose faces all face the same way along w
    (more than facing) takes its neighbours' mean w, passes times; outlines and edges (faces turned away) stay. (Pressing
    each card face onto one plane and snapping the edges onto the scripted outlines was tried: Tripo's cards are fused
    where they overlap, and it tore and folded them.)"""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.verts.ensure_lookup_table()
    bm.normal_update()
    movable = [v for v in bm.verts if v.link_faces and all(abs(f.normal.x) > facing for f in v.link_faces)
               and len({f.normal.x > 0.0 for f in v.link_faces}) == 1]
    near = [[e.other_vert(v) for e in v.link_edges] for v in movable]
    for _ in range(passes):
        new = [sum(n.co.x for n in ns) / len(ns) if ns else v.co.x for v, ns in zip(movable, near)]
        for v, x in zip(movable, new):
            v.co.x = x
    bm.to_mesh(obj.data)
    bm.free()
    return len(movable)


def fan_prepare(mt, obj):
    moved = flatten(obj)
    lt._log(f'Tripo RazorFan: {moved} card-face vertices pressed flat')
    return []


def gaff_prepare(mt, obj):
    dv, dw = recentre_pole(obj, 10.0, 70.0, 86.0)
    lt._log(f'Tripo FerryGaff: pole moved ({-dv:+.2f}, {-dw:+.2f}) cm onto the axis')
    pole, prof = relathe(mt, obj, -14.0, 85.5, 'ASH_GREY', force_bands=((-14.5, -10.5),), caps=(True, False),
                         tuck=(0.0, 1.0))
    _log_lathe('FerryGaff', 'the pole, its butt cap and bands', prof, pole)
    r = float(np.interp(46.0, prof[:, 0], prof[:, 1]))
    return pole + mt['fare_coin'](46.0, 0.0, r)


def fist_prepare(mt, obj):
    p = uvw(obj)
    body = p[(p[:, 0] > -7.5) & (p[:, 0] < -4.5) & (p[:, 1] > 4.3) & (np.abs(p[:, 2]) < 3.4)]
    v_c = 0.5 * (body[:, 1].min() + body[:, 1].max())
    r_b = 0.5 * (body[:, 1].max() - body[:, 1].min())
    CTX['cyl'] = (v_c, r_b)
    lt._log(f'Tripo PistonFist: cylinder axis at v {v_c:.2f}, body radius {r_b:.2f} cm')
    c = face_centres(obj)
    d = np.hypot(c[:, 1] - v_c, c[:, 2])
    yoke = (c[:, 0] > 15.25) & (c[:, 0] < 20.0) & (c[:, 1] > 2.9) & (np.abs(c[:, 2]) < 2.3)
    # Tripo made the brass rear ring a washer (bore 2.8 cm, too small for the forearm): it goes, the scripted ring in.
    washer = ((c[:, 0] > -11.2) & (c[:, 0] < -8.0) & (d > r_b + 0.9) &
              ~((c[:, 1] > 4.4) & (np.abs(c[:, 2]) < 2.3)) |
              ((c[:, 0] > -11.2) & (c[:, 0] < -8.0) & (c[:, 1] < 5.7) & (np.abs(c[:, 2]) > 2.3)) |  # its joins
              ((c[:, 0] > -11.2) & (c[:, 0] < -8.0) & (c[:, 1] < 5.3)))          # shards under the saddle's end
    delete_faces(obj, np.nonzero(yoke | washer)[0].tolist())
    lt._log(f'Tripo PistonFist: {yoke.sum()} faces of the hollow yoke and {washer.sum()} of the rear washer cut away')
    moved, bore = open_bore(obj, -2.6, 3.0, mt['ARM_BORE'])
    lt._log(f'Tripo PistonFist: front ring bore {bore:.2f} cm, {moved} vertices pushed out to {mt["ARM_BORE"]}')
    # The brass cylinder: Tripo's came out lumpy and its bands uneven. Its faces go (its profile, measured on its top:
    # it sinks into the saddle below and has the vent on one side, which stays Tripo's) and the scripted cylinder takes
    # its place, its radii scaled to Tripo's (a fatter cylinder, sitting lower, the saddle and rod built round it).
    _, prof = relathe(mt, obj, -10.6, 9.6, 'BRASS', axis=(v_c, 0.0), top_only=True, build=False)
    k = r_b / 2.0
    turned = lambda profile, u0, mat: mt['mapped'](mt['turned']([(du, r * k) for du, r in profile], u0, v=v_c,
                                                                 segments=40), mt[mat])
    cylinder = [turned([(0.0, 2.0), (17.0, 2.0)], -9.0, 'BRASS'),
                turned([(0.0, 2.2), (0.2, 2.45), (1.0, 2.45), (1.2, 2.2)], -9.8, 'IRON'),
                turned([(0.0, 2.2), (0.2, 2.45), (1.0, 2.45), (1.2, 2.2), (1.2, 1.3), (2.0, 1.25)], 7.6, 'IRON')]
    for u in (-3.5, 2.0):
        cylinder.append(mt['mapped'](mt['ring'](u, u + 0.7, 1.95 * k, 2.18 * k, v=v_c, segments=40, chamfer=0.08),
                                     mt['IRON']))
    lt._log(f'Tripo PistonFist: the cylinder (Tripo radius {prof[:, 1].min():.2f}..{prof[:, 1].max():.2f} cm) rebuilt '
            f'as the scripted one scaled {k:.2f}')
    rear = mt['ring'](-10.4, -8.8, mt['ARM_BORE'] + 0.2, mt['ARM_BORE'] + 0.75, segments=40)
    return [mt['piston_yoke'](), mt['mapped'](rear, mt['BRASS'])] + cylinder


# --- Painted layers: (material name in MeleeTools.py, rule(P, N) -> bool), P and N each texel's (or face centre's)
# place in cm and normal in tool space (u, v, w) on the last axis; the first rule a texel meets paints it. Where Tripo's
# shape strays from the scripted one (the spade's longer socket and wider D-grip, the fist's lower, fatter cylinder),
# the cast would pick up the neighbouring part; these draw the borders on Tripo's own shape, texel by texel, so they
# come out straight and crisp.

def _r(P):
    return np.hypot(P[..., 1], P[..., 2])


def _u(P):
    return P[..., 0]


def _between(P, spans):
    u = _u(P)
    hit = np.zeros(u.shape, bool)
    for a, b in spans:
        hit |= (u > a) & (u < b)
    return hit


SPADE_PAINT = [
    ('IRON', lambda P, N: (_u(P) > CTX['socket_u']) & (_r(P) > 1.66)),                           # the socket
    ('IRON', lambda P, N: (_u(P) > -8.5) & (_u(P) < -4.4) & (_r(P) < 2.25)),                      # the ferrule
    ('IRON', lambda P, N: (_u(P) < -16.2) & (np.abs(P[..., 2]) > 6.3)),                           # the bar's pinned ends
    ('ASH', lambda P, N: _u(P) <= CTX['socket_u']),                                                # the D-grip
]
FIST_PAINT = [
    ('STEEL_BRIGHT', lambda P, N: (_u(P) > 9.4) & (_u(P) < 15.3) &
     (np.hypot(P[..., 1] - CTX['cyl'][0], P[..., 2]) < 1.3)),                                     # the rod
]
# The handle's wood with its grain run straight along it (the cast takes the scripted handle's, one box per facet,
# which streaks); the brass binding and the teal band (their scripted spans, a little wider) stay the cast's.
PICK_PAINT = [
    ('HICKORY', lambda P, N: (_u(P) > -10.2) & (_u(P) < 21.9) & (_r(P) < 2.0) &
     ~_between(P, ((-9.15, -7.85), (-7.75, -6.45), (4.55, 5.85), (17.15, 19.65)))),
]


# Wood grain (ToolWood runs its grain along the texture's U): 'u' wraps it round the u axis (a handle or pole), a
# vector (u, v, w) lays it along that way, per face.
def spade_grain(c):
    if c[0] > -8.0:
        return 'u'
    if c[0] < -16.4:
        return (0.0, 0.0, 1.0)                          # the crossbar runs across
    return (-1.0, 0.0, 0.62 if c[2] > 0.0 else -0.62)   # the cheeks splay out to the bar


# Turned parts made round (turn_true): (u0, u1, r_max[, axis, keep]) each. The spade's shaft, the gaff's pole and the
# fist's cylinder are rebuilt as lathes instead (relathe, in their prepare): rounding Tripo's octagons left dents.
PICK_TURN = lambda: [(-10.0, 21.9, 2.3)]                        # the handle and its binding

PARTS = {
    'GraveSpade': dict(glb='GraveSpade.glb', axes=V_FRONT, prepare=spade_prepare, tris=10000, paint=SPADE_PAINT,
                       grain=spade_grain),
    'RazorFan': dict(glb='RazorFan.glb', axes=W_FRONT, prepare=fan_prepare, tris=12000),
    'FerryGaff': dict(glb='FerryGaff.glb', axes=W_FRONT, prepare=gaff_prepare, tris=9000),
    'PistonFist': dict(glb='PistonFist.glb', axes=V_FRONT, prepare=fist_prepare, tris=11000, paint=FIST_PAINT),
    'RockPick': dict(glb='RockPick.glb', axes=W_FRONT, tris=12500, turn=PICK_TURN, paint=PICK_PAINT),
}


def map_layer(mt, obj, grain=None):
    """UVs for a painted layer's copy at the scripted tools' 1024 px/m: wood with its grain along the handle (wrapped
    round it) or along grain(c)'s way; metal box-projected as the scripted parts are."""
    import bmesh
    from mathutils import Vector
    lt.box_uv(obj, 'Polymer', texel_density=mt['DENSITY'])
    if obj.data.materials[0].name not in {mt[k].name for k in ('ASH', 'ASH_GREY', 'HICKORY')}:
        return
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    uv = bm.loops.layers.uv.active
    scale = mt['DENSITY'] / 1024.0            # UV units per meter (the sets are 1024 px)
    for f in bm.faces:
        cb = f.calc_center_median()
        way = grain((-cb.y / CM, cb.z / CM, cb.x / CM)) if grain else 'u'
        if way == 'u':
            angles = [np.arctan2(loop.vert.co.x, loop.vert.co.z) for loop in f.loops]   # round the u axis: w over v
            a0 = angles[0]
            for loop, a in zip(f.loops, angles):
                while a - a0 > np.pi:
                    a -= 2 * np.pi
                while a - a0 < -np.pi:
                    a += 2 * np.pi
                loop[uv].uv = (-loop.vert.co.y * scale, a * 0.018 * scale)   # 1.8 cm: a handle's radius
        else:
            d = Vector((way[2], -way[0], way[1])).normalized()                  # tool (u, v, w) to Blender
            side = f.normal.cross(d)
            if side.length < 0.2:
                continue                                                         # end grain: the box projection stays
            side.normalize()
            for loop in f.loops:
                loop[uv].uv = (loop.vert.co.dot(d) * scale, loop.vert.co.dot(side) * scale)
    bm.to_mesh(obj.data)
    bm.free()


def swap(mt, model, key):
    """The Tripo tool in place of the scripted model (removed); same name, sockets and size."""
    import looter_rebake as rb
    import looter_tripo as tripo
    spec = PARTS[key]
    CTX.clear()
    path = TRIPO_DIR + spec['glb']
    obj, maps = tripo.load(path)
    p = uvw(model)
    lo, hi = p.min(0), p.max(0)
    tripo.fit(obj, (lo[0], hi[0]), (lo[1], hi[1]), (lo[2], hi[2]), **spec['axes'])
    extra = spec['prepare'](mt, obj) if spec.get('prepare') else []
    if spec.get('round', True):
        lt._log(f'Tripo {key}: rounded off ({round_off(obj, spec.get("hard", 75.0))} hard edges creased)')
    decimate(obj, spec['tris'])
    turns = spec['turn']() if spec.get('turn') else []
    if turns:
        # The turned parts' faces split in four after the decimation (a later one would flatten them again), so
        # it first leaves room for them.
        c = face_centres(obj)
        share = 0.0
        for t in turns:
            u0, u1, r_max = t[:3]
            av, aw = t[3] if len(t) > 3 else (0.0, 0.0)
            dv, dw = c[:, 1] - av, c[:, 2] - aw
            m = (c[:, 0] >= u0) & (c[:, 0] <= u1) & (np.hypot(dv, dw) <= r_max)
            if len(t) > 4:
                m &= np.array([bool(t[4](a, b)) for a, b in zip(dv, dw)])
            share += m.mean()
        decimate(obj, int(spec['tris'] / (1.0 + 3.0 * share)))
        for t in turns:
            moved = turn_true(obj, *t[:3], *(t[3:5]))
            lt._log(f'Tripo {key}: turned u {t[0]:.1f}..{t[1]:.1f} made round ({moved} vertices)')
    uv = obj.data.uv_layers
    while len(uv) > 1:
        uv.remove(uv[-1])
    uv[0].name = 'UVMap'
    set_name, mat_name = 'TripoMelee' + key, 'MeleeTripo' + key
    mat = lt.material(set_name, name=mat_name, master='World')
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    here = os.path.join(lt.REPO, 'Art', 'Models', 'Weapons', 'Melee')
    sources = [obj['TripoFile'], os.path.join(here, 'parts_tripo.py'), os.path.join(here, 'MeleeTools.py'),
               os.path.join(lt.REPO, 'Tools', 'Blender', 'looter_rebake.py')]
    if rb.cached(set_name, sources):
        lt._log(f'{set_name}: maps up to date, not re-baked')
    else:
        paint = []
        if spec.get('paint'):
            P, N = rb.bake_geometry(obj)
            taken = np.zeros(P.shape[:2], bool)
            layers = {}
            for name, rule in spec['paint']:
                where = rule(P, N) & ~taken
                taken |= where
                if where.any():
                    if name not in layers:
                        layers[name] = rb.material_look(obj, mt[name], lambda o: map_layer(mt, o, spec.get('grain')))
                    paint.append((where, layers[name]))
            del P, N
        look = rb.wear(obj, rb.bake_look(obj, model, paint=paint))
        mat = rb.write(look, rb.normals(look, maps.get('N'), strength=spec.get('normal', 0.0)), set_name, mat_name)
    for img in maps.values():
        if img is not None:
            bpy.data.images.remove(img)
    painted = np.zeros(len(obj.data.polygons), bool)
    if spec.get('paint'):
        c, n = face_centres(obj), face_normals(obj)
        for _name, rule in spec['paint']:
            painted |= rule(c, n)
    glows = rb.glow_faces(obj, model, [mt['EMBER_CYAN'], mt['EMBER_ORANGE']])
    for glow, faces in glows.items():
        faces = [f for f in faces if not painted[f]]
        if faces:
            obj.data.materials.append(glow)
            index = len(obj.data.materials) - 1
            for f in faces:
                obj.data.polygons[f].material_index = index
            lt._log(f'Tripo {key}: {len(faces)} faces on {glow.name}')
    del obj['TripoFile']
    lm.smooth(obj, spec.get('smooth', 70.0))
    for piece in extra:
        lm.smooth(piece, 30.0)
    name = model.name
    model.name = '_Scripted' + key
    new = mt['finish'](name, [obj] + extra, [], smooth=None)
    for child in list(model.children):
        world = child.matrix_world.copy()
        child.parent = new
        child.matrix_world = world
    mesh = model.data
    bpy.data.objects.remove(model)
    if not mesh.users:
        bpy.data.meshes.remove(mesh)
    new['LODs'] = LODS
    share = max(new.dimensions) / BODY_LENGTH
    new['LODScreens'] = ','.join(f'{s * share:.4f}' for s in LOD_SCREENS)
    lt._log(f'Tripo {key}: {tris(new)} triangles; materials {", ".join(m.name for m in new.data.materials)}')
    return new
