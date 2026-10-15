"""Ellis's silhouette options the user didn't pick (2026-10-15: they chose the third, C, a sharper outline: squarer
shoulders and a pointed back on the cape, a deeper hat crease and harder brim curl, the coat to the ankle, now
Ellis.py's own build). Kept off unless asked (Ellis.py --option A|B), each its own idea of what makes him read as a hero
from behind and at a distance, within portrait A:
  A  gear and story: a cartridge bandolier over the coat (shoulder to hip), a satchel at the right hip, spurs, a watch
     chain on the waistcoat, Pa's lantern a third bigger
  B  the coat itself: lapels, pocket flaps, buckled straps round the sleeves, a second, shorter cape tier, a frayed hem
     with patches
New pieces use the costume's looks, so they bake into its four sets; they weigh as what they lie on (Ellis.py's
weights(): a 'Duster...' piece as the coat, a 'Cape...' piece as the cape, the rest as the body under it).
Clay renders: Saved/Concepts/EllisOptions/<option>_<view>.png (ellis_audit.option_views)."""
import math

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_garments as gm


# ------------------------------------------------------------------------------------------------------ helpers

def _mesh(E, name, piece, look, thick=0.0, low=None):
    ob = gm.mesh(name, piece, E.slot(look))
    if thick:
        E.THICK[name] = thick
    if low is not None:
        E.LOW[name] = low
    return ob


def _surface(E, tree, centre, ang, z, fallback):
    try:
        return E.on_surface(tree, centre, ang, z)
    except ValueError:
        return fallback


def _sash(E, fig, tree, start, end, n=120, out=0.007):
    """A strap round the body in the plane through start and end that holds the forward axis, on the outermost layer
    (tree): its path, outward and across directions."""
    c = fig.centre
    nrm = gm.unit(np.cross(end - start, np.array([0.0, 1.0, 0.0])))
    pts, last = [], None
    for th in np.linspace(-math.pi, math.pi, n, endpoint=False):
        dx, dy = gm.radial(np.float64(th), E.FWD)
        px, py = c[0] + dx * 0.2, c[1] + dy * 0.2
        z = start[2] - (nrm[0] * (px - start[0]) + nrm[1] * (py - start[1])) / nrm[2]
        p = _surface(E, tree, c, th, z, last)
        if p is None:
            continue
        pts.append(p + np.array([dx, dy, 0.0]) * out)
        last = p
    P = gm.laplacian(np.array(pts), [[i, (i + 1) % len(pts)] for i in range(len(pts))], 6)
    radial = np.c_[P[:, :2] - c, np.zeros(len(P))]
    radial = gm.unit(radial)
    tang = gm.unit(np.roll(P, -1, 0) - np.roll(P, 1, 0))
    across = gm.unit(np.cross(tang, radial))
    across = np.where((across[:, 2:3] < 0.0), -across, across)
    return P, radial, across


def _strap(P, radial, across, width, lift=0.003, closed=True):
    rows = np.stack([P - across * width * 0.5, P + radial * lift, P + across * width * 0.5], 0)
    if closed:
        rows = np.concatenate([rows, rows[:, :1]], 1)
    return gm.grid(rows[..., 0], rows[..., 1], rows[..., 2])


def _rounded_box(centre, axes, half, n=3, p=4.0):
    """A box with rounded edges (a superellipsoid), its axes (3 unit vectors) and half sizes."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=2.0)
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=n, use_grid_fill=True)
    for v in bm.verts:
        q = np.array(v.co)
        q = q / max(np.sum(np.abs(q) ** p) ** (1.0 / p), 1e-9)
        w = centre + axes[0] * q[0] * half[0] + axes[1] * q[1] * half[1] + axes[2] * q[2] * half[2]
        v.co = Vector(tuple(w))
    verts = np.array([v.co[:] for v in bm.verts])
    faces = [[v.index for v in f.verts] for f in bm.faces]
    bm.free()
    return gm.Piece(verts, faces, None)           # no UVs: prepare_uvs projects them


# ----------------------------------------------------------------------------------------------- A: gear, story

def option_a(E, fig, parts):
    duster, cape, vest = parts['duster'], parts['cape'], parts['vest']
    tree = gm.bvh_objects([duster, cape, vest])
    sk = fig.sk
    built = []
    # the bandolier: over the left shoulder, across the back and the chest, down to the right hip
    L = np.array([0.15, -0.02, 1.47])
    H = np.array([-0.21, -0.02, 0.93])
    P, radial, across = _sash(E, fig, tree, L, H)
    built.append(_mesh(E, 'Bandolier', _strap(P, radial, across, 0.05), 'Belt', thick=0.004, low=0.6))
    ang = E.round_angle(P, fig.centre)
    case = bullet = None
    for i in range(0, len(P), 2):
        a = ang[i]
        if not (abs(a) < math.radians(48) or abs(a) > math.radians(128)):
            continue                                   # over the shoulder and the hip the loops would kink
        p0 = P[i] + radial[i] * 0.008
        c = gm.tube(np.array([p0 - across[i] * 0.016, p0 + across[i] * 0.018]), np.array([0.0058, 0.0058]), n=6, up=radial[i])
        b = gm.tube(np.array([p0 + across[i] * 0.018, p0 + across[i] * 0.026, p0 + across[i] * 0.031]),
                    np.array([0.0052, 0.0042, 0.0012]), n=6, up=radial[i])
        case = c if case is None else case + c
        bullet = b if bullet is None else bullet + b
    built.append(_mesh(E, 'BandolierShells', case, 'Brass', low=1.0))
    built.append(_mesh(E, 'BandolierBullets', bullet, 'Lead', low=1.0))
    # a satchel on the bandolier's end at the back of the right hip, its flap and the two short straps up to it
    th = math.radians(-150)
    dx, dy = gm.radial(np.float64(th), E.FWD)
    out = np.array([dx, dy, 0.0])
    side = gm.unit(np.cross(np.array([0.0, 0.0, 1.0]), out))
    base = E.on_surface(gm.bvh_objects([duster]), fig.centre, th, 0.86)
    ctr = base + out * 0.048
    axes = (side, out, np.array([0.0, 0.0, 1.0]))
    built.append(_mesh(E, 'Satchel', _rounded_box(ctr, axes, (0.12, 0.036, 0.09)), 'Belt', low=0.7))
    built.append(_mesh(E, 'SatchelFlap', _rounded_box(ctr + out * 0.006 + np.array([0, 0, 0.035]), axes,
                                                     (0.124, 0.038, 0.06), n=2), 'Belt', low=0.7))
    hip = P[np.argmin(np.linalg.norm(P - (ctr + np.array([0, 0, 0.1])), axis=1))]
    for sgn in (-1.0, 1.0):
        top = ctr + side * sgn * 0.08 + np.array([0.0, 0.0, 0.085])
        built.append(_mesh(E, 'SatchelStrap', gm.tube(np.array([top, (top + hip) * 0.5 + out * 0.01, hip]),
                                                      np.array([0.006] * 3), n=6), 'Belt', low=1.0))
    # spurs: a strap round each heel, a shank back, a rowel of eight points
    for s in 'lr':
        ankle, ball = sk[f'foot_{s}'][0], sk[f'ball_{s}'][0]
        f = gm.unit(np.array([ball[0] - ankle[0], ball[1] - ankle[1], 0.0]))
        lat = np.array([-f[1], f[0], 0.0])
        heel = np.array([ankle[0], ankle[1], E.LIFT + 0.035]) - f * 0.075
        band = np.array([heel + f * (0.06 * (1 - math.cos(a))) + lat * 0.045 * math.sin(a) for a in np.linspace(-1.6, 1.6, 11)])
        built.append(_mesh(E, f'SpurStrap_{s}', gm.tube(band, np.array([0.004] * 11), n=6), 'Belt', low=1.0))
        shank = np.array([heel, heel - f * 0.035 + np.array([0, 0, -0.004])])
        built.append(_mesh(E, f'SpurShank_{s}', gm.tube(shank, np.array([0.003, 0.0025]), n=6), 'Iron', low=1.0))
        hub = heel - f * 0.045
        star = []
        for k in range(16):
            r = 0.017 if k % 2 == 0 else 0.006
            a = k * math.pi / 8
            star.append(hub + f * r * math.cos(a) + np.array([0, 0, r * math.sin(a)]))
        star = np.array(star)
        verts = np.concatenate([star - lat * 0.0015, star + lat * 0.0015, [hub - lat * 0.0015, hub + lat * 0.0015]])
        faces = [[k, (k + 1) % 16, 32] for k in range(16)] + [[16 + (k + 1) % 16, 16 + k, 33] for k in range(16)]
        faces += [[k, 16 + k, 16 + (k + 1) % 16, (k + 1) % 16] for k in range(16)]
        built.append(_mesh(E, f'SpurRowel_{s}', gm.Piece(verts, faces, None),
                           'Iron', low=1.0))
    # a watch chain from the third button down to the left pocket
    buttons = sorted(parts['buttons'], key=lambda b: -b.location.z)
    a0 = np.array(buttons[2].location)
    a1 = np.array([0.085, a0[1] + 0.012, 1.085])
    vt = gm.bvh_objects([vest])
    chain = []
    for t in np.linspace(0.0, 1.0, 14):
        q = a0 * (1 - t) + a1 * t + np.array([0.0, 0.0, -0.035 * 4 * t * (1 - t)])
        hit = vt.find_nearest(Vector(tuple(q)), 0.05)
        chain.append(np.array(hit[0]) + np.array(hit[1]) * 0.004 if hit[0] is not None else q)
    built.append(_mesh(E, 'WatchChain', gm.tube(np.array(chain), np.array([0.0018] * 14), n=5), 'Brass', low=1.0))
    # Pa's lantern a third bigger, from where it hangs
    pivot = fig.lantern_hook
    for ob in parts['lantern']:
        if ob.name.startswith('Lantern'):
            co = gm._object_coords(ob)
            ob.data.vertices.foreach_set('co', (pivot + (co - pivot) * 1.33).astype(np.float32).ravel())
            ob.data.update()
    fig.lantern = pivot + (fig.lantern - pivot) * 1.33
    return built


# ------------------------------------------------------------------------------------------------ B: the coat

def option_b(E, fig, parts):
    duster, cape, sleeves = parts['duster'], parts['cape'], parts['sleeves']
    co, ed = E.mesh_arrays(duster)
    tree = gm.bvh_objects([duster])
    c = fig.centre
    built = []
    # lapels: from under the collar to the waist, folded back over the coat's fronts, notched below the collar
    for side in (1, -1):
        zs = np.linspace(1.10, 1.47, 16)
        rows = []
        for z in zs:
            e = E.front_edge(co, ed, c, z, side)
            p0 = E.on_surface(tree, c, side * e, z)
            r = math.hypot(p0[0] - c[0], p0[1] - c[1])
            w = 0.02 + 0.06 * float(gm.smoothstep(1.12, 1.42, z)) - 0.022 * math.exp(-((z - 1.405) / 0.012) ** 2)
            p1 = E.on_surface(tree, c, side * (e + w / r), z)
            mid = E.on_surface(tree, c, side * (e + 0.5 * w / r), z)
            out = gm.unit(np.array([mid[0] - c[0], mid[1] - c[1], 0.0]))
            rows.append([p0 + out * 0.004, mid + out * 0.007, p1 + out * 0.005])
        R = np.array(rows).transpose(1, 0, 2)
        if side < 0:
            R = R[::-1]
        built.append(_mesh(E, f'DusterLapel_{"l" if side > 0 else "r"}', gm.grid(R[..., 0], R[..., 1], R[..., 2]),
                           'Duster', thick=0.003, low=0.8))
    # pocket flaps on the hips, just behind each front's own edge (the right one opens wider, for the lantern)
    for side in (1, -1):
        e = E.front_edge(co, ed, c, 0.86, side)
        rows = []
        for z in np.linspace(0.89, 0.82, 4):
            row = []
            for a in np.linspace(e + math.radians(8), e + math.radians(34), 7):
                p = E.on_surface(tree, c, side * a, z)
                out = gm.unit(np.array([p[0] - c[0], p[1] - c[1], 0.0]))
                row.append(p + out * (0.004 + 0.004 * (0.89 - z) / 0.07))
            rows.append(row if side > 0 else row[::-1])
        R = np.array(rows)
        built.append(_mesh(E, f'DusterPocket_{"l" if side > 0 else "r"}', gm.grid(R[..., 0], R[..., 1], R[..., 2]),
                           'Duster', thick=0.003, low=1.0))
    # buckled straps round the sleeves above the gauntlets
    st = gm.bvh_objects([sleeves])
    for s in 'lr':
        e, w = fig.sk[f'lowerarm_{s}'][0], fig.sk[f'lowerarm_{s}'][1]
        axis = gm.unit(w - e)
        u = gm.unit(np.cross(axis, np.array([0.0, 1.0, 0.0])))
        v = np.cross(axis, u)
        ring = []
        for k in range(24):
            a = 2 * math.pi * k / 24
            d = u * math.cos(a) + v * math.sin(a)
            centre = w - axis * 0.135
            hit = st.ray_cast(Vector(tuple(centre + d * 0.15)), Vector(tuple(-d)), 0.2)
            ring.append((np.array(hit[0]) if hit[0] is not None else centre + d * 0.05) + d * 0.004)
        ring = np.array(ring + ring[:1])
        rows = np.stack([ring - axis * 0.013, ring, ring + axis * 0.013], 0)
        built.append(_mesh(E, f'CuffStrap_{s}', gm.grid(rows[..., 0], rows[..., 1], rows[..., 2]), 'Belt', thick=0.003, low=1.0))
        outer = ring[np.argmax(ring @ np.array([1.0 if s == 'l' else -1.0, 0.0, 0.3]))]
        frame = None
        for a0, a1 in (((-1, -1), (1, -1)), ((1, -1), (1, 1)), ((1, 1), (-1, 1)), ((-1, 1), (-1, -1))):
            q0 = outer + axis * a0[0] * 0.012 + v * a0[1] * 0.01
            q1 = outer + axis * a1[0] * 0.012 + v * a1[1] * 0.01
            bar = gm.tube(np.array([q0, q1]), np.array([0.0018, 0.0018]), n=6)
            frame = bar if frame is None else frame + bar
        built.append(_mesh(E, f'CuffBuckle_{s}', frame, 'Brass', low=1.0))
    # a second, shorter tier over the cape
    tier, _r = E.build_cape(fig, parts['body'], duster, sleeves, name='CapeTier', drop=0.065, over=[cape], top_drop=0.02)
    E.THICK['CapeTier'] = 0.004
    built.append(tier)
    # the hem frayed: torn into notches here and there, and two patches sewn on
    zc = gm._object_coords(duster)
    hem = zc[:, 2].min()
    ang = E.round_angle(zc, c)
    rng = np.random.default_rng(13)
    lift = np.zeros(len(zc))
    for th in rng.uniform(-math.pi, math.pi, 10):
        r = 0.25
        d = np.abs((ang - th + math.pi) % (2 * math.pi) - math.pi) * r
        hw = rng.uniform(0.018, 0.035)
        depth = rng.uniform(0.022, 0.042)
        lift = np.maximum(lift, depth * np.clip(1.0 - d / hw, 0.0, 1.0))
    lift *= gm.smoothstep(hem + 0.06, hem + 0.005, zc[:, 2])
    zc[:, 2] += lift + (rng.random(len(zc)) - 0.5) * 0.006 * gm.smoothstep(hem + 0.03, hem, zc[:, 2])
    duster.data.vertices.foreach_set('co', zc.astype(np.float32).ravel())
    duster.data.update()
    for th, z0 in ((math.radians(150), 0.62), (math.radians(-110), 0.55)):
        rows = []
        for z in np.linspace(z0 + 0.06, z0 - 0.06, 7):
            row = []
            for a in np.linspace(th - 0.2, th + 0.2, 9):
                p = E.on_surface(tree, c, a, z)
                out = gm.unit(np.array([p[0] - c[0], p[1] - c[1], 0.0]))
                row.append(p + out * 0.007)
            rows.append(row)
        R = np.array(rows)
        patch = _mesh(E, 'DusterPatch', gm.grid(R[..., 0], R[..., 1], R[..., 2]), 'Duster', thick=0.002, low=1.0)
        E.set_seams(patch)
        built.append(patch)
    return built


def build(option, E, fig, parts):
    """The option's new pieces (and its changes to the built ones); returns the new objects."""
    if option == 'A':
        return option_a(E, fig, parts)
    if option == 'B':
        return option_b(E, fig, parts)
    return []
