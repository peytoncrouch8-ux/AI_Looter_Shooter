"""Pike's gear (Art/Models/Characters/Pike.py calls these with M, the model's module): the bowler (the portrait's,
seated on the head by Tools/Blender/ellis_hat.py's fit) with its brass goggles, the round spectacles (the portrait's
wire), the waist belt with its pouches and the spyglass's frog, the pack's shoulder straps, the cartridge bandolier, the
pack (rucksack, lid, bedroll, the folded tripod, the map tube) and the spyglass in its sling. Units: metres, Z up, the
front -Y; the figure's left +X.
"""
import math

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector

import looter_garments as gm
import looter_model as lm
from pike_clothes import face_out, round_angle


# ------------------------------------------------------------------------------------------------------------ helpers

def realize(ob, name=None):
    """A portrait part as a plain mesh in world space: a curve (the portrait's wires) made a mesh of its bevel, a
    mesh's modifiers applied (subdivision dropped: the game mesh is built by the bake's reduction instead)."""
    name = name or ob.name.split('.')[0]
    if ob.type == 'CURVE':
        ob.data.bevel_resolution = 1
        dg = bpy.context.evaluated_depsgraph_get()
        me = bpy.data.meshes.new_from_object(ob.evaluated_get(dg))
        new = bpy.data.objects.new(name, me)
        bpy.context.scene.collection.objects.link(new)
        new.matrix_world = ob.matrix_world.copy()
        data = ob.data
        bpy.data.objects.remove(ob)
        bpy.data.curves.remove(data)
        ob = new
    else:
        for m in list(ob.modifiers):
            if m.type == 'SUBSURF':
                ob.modifiers.remove(m)
        if ob.modifiers:
            gm.apply_modifiers(ob, keep=())
        ob.name = name
    ob.data.transform(ob.matrix_world)
    ob.matrix_world = Matrix.Identity(4)
    ob.data.materials.clear()
    for p in ob.data.polygons:
        p.use_smooth = True
    return ob


def lathe(name, profile, centre, mat, n=20, axis=(0.0, 0.0, 1.0), cap_top=True, cap_bottom=True):
    """A body of revolution: profile [(radius, height)] from the bottom up about an axis through centre."""
    profile = np.asarray(profile, dtype=np.float64)
    ax = gm.unit(np.asarray(axis, dtype=np.float64))
    e1 = gm.unit(np.cross(ax, [0.0, 1.0, 0.0] if abs(ax[1]) < 0.9 else [1.0, 0.0, 0.0]))
    e2 = np.cross(ax, e1)
    ang = np.linspace(0, 2 * math.pi, n, endpoint=False)
    P = np.asarray(centre)[None, None, :] + profile[:, 0, None, None] * (np.cos(ang)[None, :, None] * e1 +
                                                                        np.sin(ang)[None, :, None] * e2) \
        + profile[:, 1, None, None] * ax[None, None, :]
    P = P[::-1]
    pc = gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True)
    verts = list(pc.verts)
    faces, uv = pc.faces, pc.uv
    rows = len(profile)
    for row, at, on in ((0, profile[-1], cap_top), (rows - 1, profile[0], cap_bottom)):
        if not on or at[0] < 1e-5:
            continue
        c = len(verts)
        verts.append(np.asarray(centre) + ax * at[1])
        for i in range(n):
            a, b = row * n + i, row * n + (i + 1) % n
            faces.append([b, a, c] if row == 0 else [a, b, c])
            uv += [(0.0, 0.0), (0.01, 0.0), (0.005, 0.01)]
    ob = gm.mesh(name, gm.Piece(np.array(verts), faces, uv), mat)
    return ob


def ring_tube(path, radius, n=6):
    """A closed tube round a closed path (k, 3): a torus-like ring (the bowler's rolled brim edge, a strap's loop)."""
    path = np.asarray(path, dtype=np.float64)
    k = len(path)
    t = gm.unit(np.roll(path, -1, 0) - np.roll(path, 1, 0))
    c = path.mean(0)
    out = gm.unit(path - c)
    out = gm.unit(out - t * (out * t).sum(1, keepdims=True))
    up = np.cross(t, out)
    ang = np.linspace(0, 2 * math.pi, n, endpoint=False)
    R = path[:, None, :] + radius * (np.cos(ang)[None, :, None] * out[:, None, :] + np.sin(ang)[None, :, None] * up[:, None, :])
    verts = R.reshape(-1, 3)
    faces = []
    for i in range(k):
        for j in range(n):
            a, b = i * n + j, i * n + (j + 1) % n
            c_, d = ((i + 1) % k) * n + (j + 1) % n, ((i + 1) % k) * n + j
            faces.append([a, b, c_, d])
    return gm.Piece(verts, faces)


def soft_box(name, centre, axes, half, mat, power=0.35, u=16, v=10):
    """A rounded box (a superellipsoid: a sphere's points raised to power), axes (ex, ey, ez) and half sizes."""
    bm = bmesh.new()
    lm.uv_sphere(bm, u, v, 1.0)      # fixed face order: the bake must fit the next run
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    co = np.array([vv.co[:] for vv in me.vertices])
    co = np.sign(co) * np.abs(co) ** power
    ex, ey, ez = (gm.unit(a) for a in axes)
    W = np.asarray(centre) + co[:, 0:1] * ex * half[0] + co[:, 1:2] * ey * half[1] + co[:, 2:3] * ez * half[2]
    me.vertices.foreach_set('co', W.astype(np.float32).ravel())
    me.update()
    me.shade_smooth()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    ob.data.materials.append(mat)
    face_out_solid(ob)
    return ob


def face_out_solid(ob):
    """A closed solid's faces turned outward if they face in (by the signed volume)."""
    me = ob.data
    vol = 0.0
    co = np.array([v.co[:] for v in me.vertices])
    for p in me.polygons:
        vs = list(p.vertices)
        for k in range(1, len(vs) - 1):
            vol += np.dot(co[vs[0]], np.cross(co[vs[k]], co[vs[k + 1]]))
    if vol < 0.0:
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
        me.update()


def frame_rect(p, side, up, normal, w, h, bar):
    """A buckle's frame: four bars round a rectangle at p (side, up its axes), standing off along normal."""
    frame = None
    corners = ((-1, -1), (1, -1), (1, 1), (-1, 1))
    for i in range(4):
        a, b = corners[i], corners[(i + 1) % 4]
        p0 = p + side * a[0] * w + up * a[1] * h
        p1 = p + side * b[0] * w + up * b[1] * h
        t = gm.tube(np.array([p0, p1]), np.array([bar, bar]), n=6, up=normal, cap0=True, cap1=True)
        frame = t if frame is None else frame + t
    t = gm.tube(np.array([p - up * h, p + up * h]), np.array([bar * 0.7, bar * 0.7]), n=6, up=normal)
    return frame + t


def smooth_path(p, it=6):
    p = np.array(p, dtype=np.float64)
    for _ in range(it):
        p[1:-1] = 0.25 * p[:-2] + 0.5 * p[1:-1] + 0.25 * p[2:]
    return p


def strap_path(tree, front, top_x, zc, back, yc=0.0, n=20, lift=0.006):
    """A path lying on what tree holds (the portrait's over_shoulder): from the chest at front (x, z) straight up to
    the shoulder at top_x, round the shoulder in the y-z plane about (top_x, yc, zc), down the back to back (x, z).
    Returns points (lifted off the surface) and normals, smoothed; the front end first."""
    pts, nrm = [], []

    def cast(o, d):
        d = gm.unit(np.asarray(d, dtype=np.float64))
        hit = tree.ray_cast(Vector(tuple(o)), Vector(tuple(d)), 2.0)
        if hit[0] is not None:
            pts.append(np.array(hit[0]))
            nrm.append(np.array(hit[1]))
    for t in np.linspace(0, 1, n, endpoint=False):
        cast((front[0] + (top_x - front[0]) * t, -0.8, front[1] + (zc - front[1]) * t), (0, 1, 0))
    for p in np.linspace(0, math.pi, n):
        d = np.array([0.0, -math.cos(p), math.sin(p)])
        cast(np.array([top_x, yc, zc]) + d * 0.8, -d)
    for t in np.linspace(0, 1, n)[1:]:
        cast((top_x + (back[0] - top_x) * t, 0.8, zc + (back[1] - zc) * t), (0, -1, 0))
    # no step back on itself (a ray past the shoulder's turn could land behind the one before: the strap folded)
    keep_p, keep_n = [pts[0]], [nrm[0]]
    for p, nn in zip(pts[1:], nrm[1:]):
        step = p - keep_p[-1]
        if np.linalg.norm(step) < 0.004:
            continue
        if len(keep_p) > 1 and step @ (keep_p[-1] - keep_p[-2]) < 0.0:
            continue
        keep_p.append(p)
        keep_n.append(nn)
    pts, nrm = np.array(keep_p), np.array(keep_n)
    nrm = gm.unit(smooth_path(nrm, 3))
    pts = smooth_path(pts, 4)
    # resampled evenly along its length (the rays crowd over the shoulder)
    arc = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))])
    s = np.linspace(0.0, arc[-1], max(12, int(arc[-1] / 0.015)))
    P = np.stack([np.interp(s, arc, pts[:, k]) for k in range(3)], 1)
    N = gm.unit(np.stack([np.interp(s, arc, nrm[:, k]) for k in range(3)], 1))
    # lifted clear of the surface everywhere (the smoothing cut the shoulder's curve a little)
    for i, (p, nn) in enumerate(zip(P, N)):
        loc, n_, _i, _d = tree.find_nearest(Vector(tuple(p)), 0.05)
        if loc is not None:
            d = (Vector(tuple(p)) - loc).dot(n_)
            if d < lift:
                P[i] = np.array(loc) + np.array(n_) * lift
    return P, N


def ribbon(name, pts, nrm, width, mat, cols=2):
    """A strap along a path lying on a surface (points already lifted, their normals): cols quads across."""
    tan = gm.unit(np.gradient(pts, axis=0))
    side = gm.unit(np.cross(tan, nrm))
    # carried along without flipping (where the path turned hard over the shoulder it twisted the strap into a knot)
    for i in range(1, len(side)):
        if side[i] @ side[i - 1] < 0.0:
            side[i] = -side[i]
    for _ in range(2):
        side[1:-1] = gm.unit(0.25 * side[:-2] + 0.5 * side[1:-1] + 0.25 * side[2:])
    across = np.linspace(-0.5, 0.5, cols + 1)
    P = pts[None, :, :] + across[:, None, None] * width * side[None, :, :]
    return gm.mesh(name, gm.grid(P[..., 0], P[..., 1], P[..., 2]), mat)


# ------------------------------------------------------------------------------------------------------------ the hat

def build_hat(M, fig):
    """The portrait's bowler (Pike.build_bowler: a domed crown belled a little above the band, a short brim rolled up
    at the sides, a grosgrain band) moved into the figure's space and seated on the head (ellis_hat.fit, as Ellis's:
    the band's foot on a plane from over the brows to over the occipital bump, the crown fitted to the head's section);
    the brim's rolled edge; the brass goggles pushed up on the crown's front with their strap round it."""
    import ellis_hat
    M.pp.build_bowler(fig.b.h, fig.hf, M.OPT)
    rename = {'Crown': 'HatCrown', 'Brim': 'HatBrim', 'HatBand': 'HatBand'}
    parts = []
    for ob in list(bpy.context.scene.objects):
        key = ob.name.split('.')[0]
        if key == 'BrimRoll':
            data = ob.data
            bpy.data.objects.remove(ob)
            bpy.data.curves.remove(data)
        elif key in rename and ob.type == 'MESH':
            for m in list(ob.modifiers):
                ob.modifiers.remove(m)
            fig.to_portrait(ob)
            ob.data.transform(ob.matrix_world)
            ob.matrix_world = Matrix.Identity(4)
            ob.name = ob.data.name = rename[key]
            parts.append(ob)
    parts.sort(key=lambda o: ('HatCrown', 'HatBrim', 'HatBand').index(o.name))
    # the brim a little shorter than the portrait's (fit keeps its outline while the crown slims: it reached further)
    brim = parts[1]
    co = gm._object_coords(brim).reshape(-1, 96, 3)
    co = co[:1] + (co - co[:1]) * BRIM_REACH
    brim.data.vertices.foreach_set('co', co.reshape(-1).astype(np.float32))
    brim.data.update()
    parts = ellis_hat.fit(fig, parts)
    M.HAT['pivot'] = ellis_hat.pivot(fig, parts)
    band = next(o for o in parts if o.name.startswith('HatBand'))
    M.HAT['band_z'] = float(gm._object_coords(band)[:, 2].min())
    for ob in parts:
        ob.data.materials.clear()
        ob.data.materials.append(M.slot('HatBand' if ob.name.startswith('HatBand') else 'Felt'))
    brim = next(o for o in parts if o.name == 'HatBrim')
    bc = gm._object_coords(brim).reshape(-1, 96, 3)
    roll = gm.mesh('BrimRoll', ring_tube(bc[-1], 0.0042, n=6), M.slot('Felt'))
    face_out_solid(roll)
    parts.append(roll)
    parts += goggles(M, fig, next(o for o in parts if o.name == 'HatCrown'))
    fit = fig.hat_fit
    M.log(f'hat: seated, crown raised {fit["raise_by"] * 100:.0f}%, its air over the skull {fit["air"] * 1000:.1f} mm; '
          f'pivot {np.round(M.HAT["pivot"][0], 3)}')
    return parts


BRIM_REACH = 0.80       # the brim's reach past the crown, of the portrait's
GOGGLE_ROW = 6           # the crown's row (of 30, from the band up) the goggles ride on, as the portrait's 22 %


def goggles(M, fig, crown):
    """The brass goggles (the portrait's goggles(): two cups of smoked glass, rims, a bridge) up on the crown's front,
    their leather strap round the crown."""
    cc = gm._object_coords(crown)
    rows = cc[:len(cc) - (len(cc) % 96)].reshape(-1, 96, 3)
    p = rows[GOGGLE_ROW, 0]
    t = gm.unit(rows[GOGGLE_ROW + 1, 0] - rows[GOGGLE_ROW, 0])
    n = gm.unit(np.array([0.0, -t[2], t[1]]))
    origin = p + n * 0.012
    ez = n
    ex = gm.unit(np.array([1.0, 0.0, 0.0]) - ez * ez[0])
    F = np.eye(4)
    F[:3, 0], F[:3, 1], F[:3, 2], F[:3, 3] = ex, np.cross(ez, ex), ez, origin
    made = M.pp.goggles(Matrix(F.tolist()))
    out = []
    for ob in made:
        key = ob.name.split('.')[0]
        ob = realize(ob, key)
        ob.data.materials.append(M.slot('Glass' if key == 'GoggleLens' else 'Brass'))
        out.append(ob)
    # the strap round the crown, just under the goggles' row, standing 3 mm off the felt
    c = rows[GOGGLE_ROW].mean(0)
    band = []
    for r in (GOGGLE_ROW - 0.9, GOGGLE_ROW + 0.6):
        i0 = int(math.floor(r))
        f = r - i0
        ring = rows[i0] * (1 - f) + rows[i0 + 1] * f
        out_dir = gm.unit((ring - c) * np.array([1.0, 1.0, 0.25]))
        band.append(ring + out_dir * 0.003)
    P = np.array(band[::-1])
    strap = gm.mesh('GoggleStrap', gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True), M.slot('Leather'))
    face_out(strap, c)
    out.append(strap)
    return out


# ------------------------------------------------------------------------------------------------------ the spectacles

def build_specs(M, fig):
    """The portrait's round spectacles (Pike.build_specs: thin brass rims over the eyes, a saddle bridge, temples back
    over the ears), in the figure's space. No lenses: the game has no glass, and his eyes burn through."""
    made = M.pp.build_specs(fig.b.h, fig.hf, M.OPT)
    out = []
    for ob in made:
        key = ob.name.split('.')[0]
        if key in ('Lens', 'LensShard', 'Crack'):
            data = ob.data
            bpy.data.objects.remove(ob)
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
            continue
        fig.to_portrait(ob)
        ob = realize(ob, key)
        ob.data.materials.append(M.slot('Brass'))
        out.append(ob)
    return out


# --------------------------------------------------------------------------------------------------------- the belt

BELT_H = 0.024           # the belt's half height (m)


def belt_height(M, th):
    """The belt's middle at angle th round the waist: level, dipping 1.2 cm at the front under his belly."""
    return M.BELT - 0.012 * np.maximum(0.0, np.cos(th)) ** 2


def build_belt(M, fig, under_obs):
    """A brown leather belt over the coat with a brass frame buckle in front and its keeper; two pouches at the front
    sides; the frog at the left hip the spyglass hangs from (build_spyglass)."""
    tree = gm.bvh_objects(under_obs)
    n = 120
    th = np.linspace(-math.pi, math.pi, n, endpoint=False)
    dx, dy = gm.radial(th, M.FWD)
    path, nrm = [], []
    for i in range(n):
        z = float(belt_height(M, th[i]))
        d = Vector((dx[i], dy[i], 0.0))
        o = Vector((fig.centre[0], fig.centre[1], z)) + d * 0.6
        hit = tree.ray_cast(o, -d, 1.0)
        p = np.array(hit[0]) if hit[0] is not None else np.array(o - d * 0.42)
        nn = np.array(hit[1]) if hit[0] is not None else np.array(d)
        nn[2] *= 0.4
        path.append(p + gm.unit(nn) * 0.004)
        nrm.append(gm.unit(nn))
    path, nrm = np.array(path), np.array(nrm)
    path = gm.laplacian(path, [[i, (i + 1) % n] for i in range(n)], 3)
    # a belt is stiff: its band stands at its fullest round the waist, straight up and down
    up = gm.unit(np.array([0.0, 0.0, 1.0]) - nrm * nrm[:, 2:3])
    P = np.array([path + up * BELT_H, path + nrm * 0.002, path - up * BELT_H])
    belt = gm.mesh('Belt', gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True), M.slot('Leather'))
    face_out(belt, fig.centre)
    parts = [belt]
    fig.belt = (th, path, nrm, up)
    # the buckle: a brass frame in front, the prong, the belt's tongue through a leather keeper beside it
    p, nn, uu, tt = belt_frame(fig, 0.0)
    side = gm.unit(np.cross(uu, nn))
    f = p + nn * 0.006
    parts.append(gm.mesh('Buckle', frame_rect(f, side, uu, nn, 0.026, 0.027, 0.0035), M.slot('Brass')))
    kp = p + side * 0.05 + nn * 0.004
    keeper = np.array([[kp + side * w + uu * h + nn * (0.003 if abs(h) < 0.03 else 0.0) for w in (-0.007, 0.007)]
                       for h in (0.029, 0.0, -0.029)])
    parts.append(gm.mesh('Belt_keeper', gm.grid(keeper[..., 0], keeper[..., 1], keeper[..., 2]), M.slot('Leather')))
    # the pouches: leather boxes hung from the belt, a flap over each with a brass stud
    for k, a in enumerate((0.78, -0.78)):
        p, nn, uu, tt = belt_frame(fig, a)
        c = p + nn * 0.024 - uu * 0.010
        side = gm.unit(np.cross(uu, nn))
        parts.append(soft_box(f'Pouch_{k}', c, (side, nn, uu), (0.034, 0.018, 0.040), M.slot('Leather'), power=0.3))
        parts.append(soft_box(f'PouchFlap_{k}', c + nn * 0.004 + uu * 0.024, (side, nn, uu), (0.037, 0.021, 0.019),
                              M.slot('Leather'), power=0.25, u=14, v=8))
        stud = c + nn * 0.026 + uu * 0.012
        parts.append(soft_box(f'Buckle_stud{k}', stud, (side, nn, uu), (0.004, 0.0025, 0.004), M.slot('Brass'), u=8, v=6))
    return parts


def belt_frame(fig, angle):
    """Point, outward normal, up and the direction round the belt at angle (0 the front, + his left)."""
    th, path, nrm, up = fig.belt
    i = int(np.argmin(np.abs((th - angle + math.pi) % (2 * math.pi) - math.pi)))
    tangent = gm.unit(path[(i + 1) % len(th)] - path[i - 1])
    return path[i], nrm[i], up[i], tangent


# ----------------------------------------------------------------------------------------- straps and bandolier

def build_straps(M, fig, under_obs):
    """The pack's shoulder straps: from the belt in front (a brass hook over its top) up the chest, over each shoulder
    and down to the pack's top behind; a brass buckle on each at the chest."""
    tree = gm.bvh_objects(under_obs)
    sz = fig.shoulder_z
    yc = fig.sk['clavicle_l'][1][1]
    parts, buckles = [], []
    for side in (1.0, -1.0):
        s = 'l' if side > 0 else 'r'
        # down the back under the pack's top (it hides where they go into the pack)
        pts, nrm = strap_path(tree, (side * 0.122, M.BELT + 0.012), side * 0.148, sz - 0.04,
                              (side * 0.118, sz - 0.11), yc=yc, lift=0.0055)
        # only from the belt's top up and over to the back just past the shoulder blades (the pack's top hides the rest)
        keep = pts[:, 2] > M.BELT + 0.012
        pts, nrm = pts[keep], nrm[keep]
        parts.append(ribbon(f'Strap_{s}', pts, nrm, 0.040, M.slot('Leather')))
        k = int(np.argmin(np.abs(pts[:, 2] - (sz - 0.12)) + (pts[:, 1] > fig.centre[1]) * 9))
        t = gm.unit(pts[min(k + 1, len(pts) - 1)] - pts[max(k - 1, 0)])
        sd = gm.unit(np.cross(t, nrm[k]))
        buckles.append(gm.mesh(f'StrapBuckle_{s}', frame_rect(pts[k] + nrm[k] * 0.004, sd, t, nrm[k], 0.024, 0.016, 0.0028),
                               M.slot('Brass')))
        # the hook over the belt's top
        h0 = pts[0] + nrm[0] * 0.003
        hook = np.array([h0 + np.array([0, 0, 0.006]), h0 - np.array([0, 0, 0.012]), h0 - np.array([0, 0, 0.026]) + nrm[0] * 0.004])
        buckles.append(gm.mesh(f'StrapBuckle_hook{s}', gm.tube(hook, np.array([0.0028] * 3), n=6, cap0=True, cap1=True),
                               M.slot('Brass')))
        fig.strap_top = getattr(fig, 'strap_top', {})
        fig.strap_top[s] = pts[-1]
    return parts + buckles


BANDOLIER_W = 0.050


def build_bandolier(M, fig, under_obs):
    """The cartridge bandolier from the left hip in front, up across the chest, over the right shoulder and down the
    back to the left hip; a row of brass cartridges in loops down its front from the chest to the belt."""
    tree = gm.bvh_objects(under_obs)
    sz = fig.shoulder_z
    yc = fig.sk['clavicle_r'][1][1]
    pts, nrm = strap_path(tree, (0.170, M.BELT + 0.030), -0.112, sz - 0.035, (0.150, M.BELT + 0.035), yc=yc, lift=0.007)
    parts = [ribbon('Bandolier', pts, nrm, BANDOLIER_W, M.slot('Leather'), cols=3)]
    # the cartridges: down the front between the chest and the belt
    front = (pts[:, 1] < fig.centre[1]) & (pts[:, 2] < sz - 0.05) & (pts[:, 2] > M.BELT + 0.07)
    idx = np.nonzero(front)[0]
    arc = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(pts[idx], axis=0), axis=1))])
    case, bullet, loops = None, None, []
    for a in np.arange(0.012, arc[-1] - 0.008, 0.0235):
        i = idx[int(np.searchsorted(arc, a))]
        t = gm.unit(pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)])
        upv = gm.unit(np.cross(t, nrm[i]))
        if upv[2] < 0:
            upv = -upv
        c0 = pts[i] + nrm[i] * 0.0095
        c = gm.tube(np.array([c0 - upv * 0.019, c0 + upv * 0.017]), np.array([0.0055, 0.0055]), n=8, cap0=True)
        b = gm.tube(np.array([c0 + upv * 0.017, c0 + upv * 0.025, c0 + upv * 0.030]), np.array([0.0050, 0.0040, 0.0012]),
                    n=8, cap1=True)
        case = c if case is None else case + c
        bullet = b if bullet is None else bullet + b
    parts.append(gm.mesh('Cartridges', case, M.slot('Brass')))
    parts.append(gm.mesh('Bullets', bullet, M.slot('Lead')))
    # the loops' band over the cartridges' middles
    lp = pts[idx]
    ln = nrm[idx]
    t = gm.unit(np.gradient(lp, axis=0))
    upv = gm.unit(np.cross(t, ln))
    upv = np.where(upv[:, 2:3] < 0, -upv, upv)
    band = lp + ln * 0.0158 - upv * 0.004
    parts.append(ribbon('Bandolier_loops', band, ln, 0.018, M.slot('Leather')))
    return parts


# ----------------------------------------------------------------------------------------------------------- the pack

PACK = dict(half_w=0.160, depth=0.150, top=None, bottom=None)


def build_pack(M, fig, under_obs):
    """The pack on his back: a canvas rucksack (its front following his back over the straps and the bandolier, the
    back face full) with a buckled lid; the bedroll strapped under it; the folded tripod (three ash legs, brass-shod,
    strapped together) and the leather map tube riding up its right side."""
    sz = fig.shoulder_z
    pts = np.concatenate([gm.object_points(o) for o in under_obs])
    z_top, z_bot = sz - 0.012, M.BELT + 0.075
    hw, depth = PACK['half_w'], PACK['depth']
    rows = 16
    zs = np.linspace(z_top, z_bot, rows)
    back_y = []
    for z in zs:
        band = pts[(np.abs(pts[:, 2] - z) < 0.03) & (np.abs(pts[:, 0]) < hw + 0.01)]
        back_y.append(band[:, 1].max() + 0.007 if len(band) else fig.centre[1] + 0.13)
    back_y = smooth_path(np.array(back_y), 3)
    # the section: a rounded rectangle (superellipse) across x and y, fuller at the back, rounded over the top and bottom
    ang = np.linspace(0, 2 * math.pi, 40, endpoint=False)
    ca, sa = np.cos(ang), np.sin(ang)
    sx = np.sign(ca) * np.abs(ca) ** 0.35
    sy = np.sign(sa) * np.abs(sa) ** 0.35
    ring = []
    for j, z in enumerate(zs):
        v = j / (rows - 1)
        round_ = np.sqrt(np.clip(1.0 - (np.clip((0.12 - v) / 0.12, 0, 1)) ** 2, 0.0, 1.0)) * \
            np.sqrt(np.clip(1.0 - (np.clip((v - 0.88) / 0.12, 0, 1)) ** 2, 0.0, 1.0))
        w = hw * (0.82 + 0.18 * round_)
        d = depth * (0.55 + 0.45 * round_) * (1.0 + 0.10 * math.sin(math.pi * v))
        y0 = back_y[j]
        # the front half flat against him, the back half full
        y = np.where(sy < 0, y0 + d * 0.5 + sy * d * 0.5, y0 + d * 0.5 + sy * d * 0.5 * (1.0 + 0.06 * (1 - np.abs(sx))))
        ring.append(np.stack([sx * w, y, np.full(len(ang), z)], 1))
    R = np.array(ring)
    piece = gm.grid(R[..., 0], R[..., 1], R[..., 2], wrap=True)
    verts, faces = list(piece.verts), piece.faces
    n = len(ang)
    for row, cz in ((0, z_top + 0.012), (rows - 1, z_bot - 0.01)):
        c = len(verts)
        verts.append(np.array([0.0, R[row][:, 1].mean(), cz]))
        for k in range(n):
            a, b = row * n + k, row * n + (k + 1) % n
            faces.append([c, a, b] if row == 0 else [c, b, a])
    sack = gm.mesh('Rucksack', gm.Piece(np.array(verts), faces), M.slot('Canvas'))
    face_out_solid(sack)
    parts = [sack]
    yc = float(R[:, :, 1].mean())
    # the lid: over the top and a third of the way down, standing 7 mm off the sack
    lid_rows = [R[j] for j in range(0, 7)]
    L = []
    for j, rr in enumerate(lid_rows):
        c = np.array([0.0, rr[:, 1].mean(), rr[0, 2]])
        o = gm.unit((rr - c) * np.array([1.0, 1.0, 0.0]))
        L.append(rr + o * (0.007 + 0.002 * (j == 6)))
    L = np.array(L)
    # a lip down the lid's front edge, rows top down
    lid = gm.grid(L[..., 0], L[..., 1], L[..., 2], wrap=True)
    lv, lf = list(lid.verts), lid.faces
    c = len(lv)
    lv.append(np.array([0.0, L[0][:, 1].mean(), z_top + 0.020]))
    for k in range(n):
        lf.append([c, k, (k + 1) % n])
    lid_ob = gm.mesh('RuckLid', gm.Piece(np.array(lv), lf), M.slot('Canvas'))
    face_out_solid(lid_ob)
    parts.append(lid_ob)
    # the lid's two straps down the back face, brass buckles at their ends
    hit_tree = gm.bvh_objects([lid_ob, sack])
    for k, x in enumerate((0.085, -0.118)):
        line = []
        for j in range(0, 10):
            # over the lid's top (rays from above) and down its back (rays from behind)
            if j < 2:
                o, d = Vector((x, yc + 0.02 + 0.035 * j, z_top + 0.3)), Vector((0, 0, -1))
            else:
                o, d = Vector((x, yc + 0.5, z_top - (j - 2) * 0.018)), Vector((0, -1, 0))
            hit = hit_tree.ray_cast(o, d, 1.0)
            if hit[0] is not None:
                line.append((np.array(hit[0] + hit[1] * 0.003), np.array(hit[1])))
        # over the top too: from the lid's top front edge
        P = np.array([p for p, _ in line])
        N = np.array([q for _, q in line])
        if len(P) > 2:
            parts.append(ribbon(f'RuckBelt_{k}', P, N, 0.022, M.slot('Leather')))
            pk, nk = P[-2], N[-2]
            t = gm.unit(P[-1] - P[-3])
            parts.append(gm.mesh(f'RuckBuckle_{k}', frame_rect(pk + nk * 0.003, gm.unit(np.cross(t, nk)), t, nk, 0.014, 0.011,
                                                               0.0022), M.slot('Brass')))
    # the bedroll under the sack, strapped
    br = 0.062
    bc = np.array([0.0, back_y[-1] + br + 0.004, z_bot - br + 0.012])
    roll = lathe('Bedroll', [(0.0, -0.20), (br * 0.85, -0.20), (br, -0.19), (br * 1.02, 0.0), (br, 0.19), (br * 0.85, 0.20),
                             (0.0, 0.20)], bc, M.slot('Bedroll'), n=28, axis=(1.0, 0.0, 0.0))
    parts.append(roll)
    for k, x in enumerate((-0.12, 0.12)):
        ring = [bc + np.array([x, math.cos(a) * (br + 0.003), math.sin(a) * (br + 0.003)]) for a in
                np.linspace(0, 2 * math.pi, 24, endpoint=False)]
        R2 = np.array(ring)
        P = np.stack([R2 - np.array([0.011, 0, 0]), R2 + np.array([0.011, 0, 0])])
        parts.append(gm.mesh(f'BedrollStrap_{k}', gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True), M.slot('Leather')))
    fig.pack = dict(z_top=z_top, z_bot=z_bot, yc=yc, back_y=back_y, zs=zs, bedroll=(bc, br))
    parts += tripod(M, fig, sack, yc)
    parts += map_tube(M, fig, [sack, lid_ob])
    M.log(f'pack: {z_bot:.3f} to {z_top:.3f} m, its front {min(back_y):.3f}..{max(back_y):.3f} y, bedroll at {bc[2]:.3f}')
    return parts


def tripod(M, fig, sack, yc):
    """The surveyor's tripod folded: three ash legs side by side (feet up, brass-shod), from a strap at the sack's
    lower right corner up past his right shoulder, leaning out a little; two leather straps round them; the brass head
    at the bottom."""
    z_bot = fig.pack['z_bot']
    base = np.array([-0.186, yc + 0.010, z_bot + 0.020])
    top = np.array([-0.256, yc + 0.004, 1.585])
    ax = gm.unit(top - base)
    e1 = gm.unit(np.cross(ax, (0.0, 1.0, 0.0)))
    e2 = np.cross(ax, e1)
    parts = []
    legs = None
    shoes = None
    bands = None
    for k in range(3):
        a = 2 * math.pi * k / 3 + 0.5
        o = e1 * math.cos(a) + e2 * math.sin(a)
        p, q = base + o * 0.013, top + o * 0.020
        d = gm.unit(q - p)
        leg = gm.tube(np.array([p, p + (q - p) * 0.5, q - d * 0.05]), np.array([0.0115, 0.0110, 0.0100]), n=8, cap0=True)
        legs = leg if legs is None else legs + leg
        sh = gm.tube(np.array([q - d * 0.052, q - d * 0.046, q, q + d * 0.040]), np.array([0.0105, 0.0108, 0.0102, 0.0025]),
                     n=8, cap1=True)
        shoes = sh if shoes is None else shoes + sh
        for f in (0.30, 0.62):
            c = p + (q - p) * f
            b_ = gm.tube(np.array([c, c + d * 0.022]), np.array([0.0124, 0.0124]), n=8)
            bands = b_ if bands is None else bands + b_
    parts.append(gm.mesh('Tripod', legs, M.slot('Wood')))
    parts.append(gm.mesh('TripodShoe', shoes, M.slot('Brass')))
    parts.append(gm.mesh('TripodBand', bands, M.slot('Brass')))
    head = lathe('TripodHead', [(0.0, -0.030), (0.020, -0.030), (0.034, -0.022), (0.034, -0.012), (0.024, -0.006),
                                (0.022, 0.012)], base, M.slot('Brass'), n=16, axis=tuple(ax))
    parts.append(head)
    for k, f in enumerate((0.42, 0.78)):
        c = base + (top - base) * f
        r = 0.020 + 0.007 * f + 0.004
        ring = [c + (e1 * math.cos(a) + e2 * math.sin(a)) * r for a in np.linspace(0, 2 * math.pi, 16, endpoint=False)]
        R = np.array(ring)
        P = np.stack([R - ax * 0.010, R + ax * 0.010])
        parts.append(gm.mesh(f'TripodStrap_{k}', gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True), M.slot('Leather')))
    fig.tripod = (base, top)
    return parts


def map_tube(M, fig, holders):
    """The leather map tube upright on the sack's back right of the middle (the portrait's: between his head and the
    tripod), a brass cap and two bands, held by two leather loops."""
    z_top, z_bot = fig.pack['z_top'], fig.pack['z_bot']
    tree = gm.bvh_objects(holders)
    x0, x1 = -0.058, -0.074
    r = 0.033

    def on_back(x, z):
        hit = tree.ray_cast(Vector((x, 1.0, z)), Vector((0, -1, 0)), 2.0)
        return np.array(hit[0]) if hit[0] is not None else np.array([x, fig.pack['yc'] + 0.09, z])
    a = on_back(x0, z_bot + 0.03) + np.array([0.0, r + 0.004, 0.0])
    b = on_back(x1, z_top - 0.02) + np.array([0.0, r + 0.004, 0.0])
    b[2] = 1.468
    d = gm.unit(b - a)
    parts = [gm.mesh('MapTube', gm.tube(np.array([a, (a + b) * 0.5, b]), np.array([r, r, r]), n=16, cap0=True),
                     M.slot('TubeLeather'))]
    parts.append(lathe('MapCap', [(r + 0.0008, -0.042), (r + 0.0025, -0.040), (r + 0.0025, 0.004), (r - 0.004, 0.012),
                                  (0.008, 0.014), (0.0, 0.014)], b, M.slot('Brass'), n=16, axis=tuple(d), cap_bottom=False))
    bands = None
    for f in (0.12, 0.55):
        c = a + (b - a) * f
        t = gm.tube(np.array([c, c + d * 0.012]), np.array([r + 0.0022, r + 0.0022]), n=16)
        bands = t if bands is None else bands + t
    parts.append(gm.mesh('MapBand', bands, M.slot('Brass')))
    for k, f in enumerate((0.25, 0.70)):
        c = a + (b - a) * f
        ring = [c + np.array([math.cos(q) * (r + 0.004), math.sin(q) * (r + 0.004), 0.0]) for q in
                np.linspace(0, 2 * math.pi, 16, endpoint=False)]
        Rr = np.array(ring)
        P = np.stack([Rr - d * 0.010, Rr + d * 0.010])
        parts.append(gm.mesh(f'MapLoop_{k}', gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True), M.slot('Leather')))
    fig.map_tube = (a, b, r)
    return parts


# --------------------------------------------------------------------------------------------------------- the spyglass

SPY_AT = 2.0             # the frog's place round the belt (radians from the front, + his left): behind his left hip, clear
                         # of the hip pocket and of the hand hanging at his side
SPY_LEN = 0.205


def build_spyglass(M, fig, under_obs):
    """The brass spyglass shut, in a leather loop on a frog hooked over the belt at his left hip, lying along the coat's
    skirt (it rides the skirt's springs: hung free on a chain of its own it hung out in the air past the skirt's
    flare): the objective's end down, a leather grip round the barrel, two draw tubes and the eyepiece up."""
    p, nn, uu, tt = belt_frame(fig, SPY_AT)
    side = gm.unit(np.cross(uu, nn))
    parts = []
    skirt = next(o for o in under_obs if o.name.startswith('Skirt'))
    tree = gm.bvh_objects(under_obs)
    stree = gm.bvh_objects([skirt])
    # the frog: a leather tab folded over the belt, down onto the skirt to the loop
    tab = []
    for h in np.linspace(0.030, -0.060, 8):
        row = []
        for w in (-0.013, 0.0, 0.013):
            q = p + uu * h + side * w
            hit = tree.ray_cast(Vector(tuple(q + nn * 0.15)), Vector(tuple(-nn)), 0.3)
            row.append(np.array(hit[0]) + np.array(hit[1]) * 0.0045 if hit[0] is not None else q + nn * 0.008)
        tab.append(row)
    tab = np.array(tab)
    parts.append(gm.mesh('Frog', gm.grid(tab[..., 0], tab[..., 1], tab[..., 2]), M.slot('Leather')))
    face_out(parts[-1], fig.centre)
    # the axis: from under the frog down along the skirt, standing its radius and 3 mm off it all the way
    r = 0.0212
    a = round_angle(p, fig.centre)[0]
    z_top = p[2] - 0.050
    pts = []
    for h in np.linspace(0.0, SPY_LEN, 7):
        z = z_top - h * 0.985
        dx, dy = gm.radial(np.float64(a), M.FWD)
        o = Vector((fig.centre[0] + dx * 0.6, fig.centre[1] + dy * 0.6, z))
        hit = stree.ray_cast(o, Vector((-dx, -dy, 0.0)), 0.6)
        q = np.array(hit[0]) if hit[0] is not None else np.array([fig.centre[0] + dx * 0.25, fig.centre[1] + dy * 0.25, z])
        pts.append(q)
    pts = np.array(pts)
    out = gm.unit(np.array([gm.radial(np.float64(a), M.FWD)[0], gm.radial(np.float64(a), M.FWD)[1], 0.0]))
    top = pts[0] + out * (r + 0.003)
    bottom = pts[-1] + out * (r + 0.003)
    ax = gm.unit(top - bottom)
    # clear of the skirt along its whole length (its folds): moved out by the worst
    need = max(float((q - (bottom + ax * ((q - bottom) @ ax))) @ out) + r + 0.003 for q in pts)
    shift = max(0.0, need)
    top, bottom = top + out * shift, bottom + out * shift
    # the loop holding it under the frog
    lc = top - ax * 0.030
    e1 = gm.unit(np.cross(ax, out))
    ring = np.array([lc + (e1 * math.cos(q) + np.cross(ax, e1) * math.sin(q)) * (r + 0.0025)
                     for q in np.linspace(0, 2 * math.pi, 18, endpoint=False)])
    P = np.stack([ring - ax * 0.009, ring + ax * 0.009])
    parts.append(gm.mesh('FrogStrap', gm.grid(P[..., 0], P[..., 1], P[..., 2], wrap=True), M.slot('Leather')))
    z0 = bottom
    prof_brass = [(0.0, 0.0), (0.0190, 0.0), (0.0212, 0.004), (0.0212, 0.028), (0.0198, 0.032)]
    parts.append(lathe('SpyglassBrass_objective', prof_brass, z0, M.slot('Brass'), n=18, axis=tuple(ax)))
    parts.append(lathe('SpyglassGrip', [(0.0201, 0.030), (0.0204, 0.034), (0.0204, 0.118), (0.0201, 0.122)], z0,
                       M.slot('TubeLeather'), n=18, axis=tuple(ax), cap_top=False, cap_bottom=False))
    draws = [(0.0196, 0.120), (0.0196, 0.128), (0.0172, 0.130), (0.0172, 0.158), (0.0180, 0.160), (0.0180, 0.164),
             (0.0150, 0.166), (0.0150, 0.188), (0.0158, 0.190), (0.0158, 0.194), (0.0120, 0.197), (0.0118, 0.205),
             (0.0050, 0.206), (0.0, 0.206)]
    parts.append(lathe('Spyglass', draws, z0, M.slot('Brass'), n=18, axis=tuple(ax), cap_bottom=False))
    parts.append(lathe('SpyglassBrass_lens', [(0.0, 0.0015), (0.016, 0.0015)], z0, M.slot('Glass'), n=18, axis=tuple(ax),
                       cap_bottom=False))
    fig.spyglass = (lc, top, bottom)
    M.log(f'spyglass: along the skirt from {top[2]:.3f} to {bottom[2]:.3f} m, {shift * 100:.1f} cm out past its folds')
    return parts
