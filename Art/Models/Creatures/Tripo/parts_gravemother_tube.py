"""Clean tube topology for the Gravemother's limbs (her legs' femurs and tibias, the palps, claws and chelicerae):
rings of quads round each piece's centre line, every ring's points shot out from the line onto Tripo's surface. Limbs
want loops round them (they bend at their joints, and the painter reads along and around them), which QuadriFlow's
even field doesn't give a thin tapering claw: its counts came out anywhere from a third to three quarters of what was
asked. Not a model: Tools\\models.ps1 skips parts_*.py.

tube(V, F, line, pole, sides, spacing): V, F the piece (closed, creature cm), line its centre line from root to tip
(n x 3), pole the way its top faces (the rig's knee pole: up and a little out). Rings are spaced spacing cm apart, closer
where the radius changes fast (a joint's neck, a plate's rim); the root end is closed by a ring drawn in to a point, the
tip end to the line's tip. Returns (V, F, uv): uv per vertex, u along (0 root .. 1 tip, by length), v around (0.5 the
top)."""
import math

import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree


def _resample(P, step):
    seg = np.linalg.norm(np.diff(P, axis=0), axis=1)
    s = np.concatenate([[0.0], np.cumsum(seg)])
    n = max(int(round(s[-1] / step)), 2)
    at = np.linspace(0.0, s[-1], n + 1)
    return np.column_stack([np.interp(at, s, P[:, k]) for k in range(3)])


def _smooth(P, passes):
    P = P.copy()
    for _ in range(passes):
        Q = P.copy()
        Q[1:-1] = (P[:-2] + 2.0 * P[1:-1] + P[2:]) / 4.0
        P = Q
    return P


def centre_line(V, line, step=1.0):
    """The line smoothed, at even steps, run on straight past both ends to cover the piece's every vertex."""
    P = _smooth(_resample(np.asarray(line, float), 2.0), 3)
    P = _resample(P, step)
    a0 = P[1] - P[0]
    a0 /= np.linalg.norm(a0)
    a1 = P[-1] - P[-2]
    a1 /= np.linalg.norm(a1)
    before = max(0.0, float(((P[0] - V) @ a0).max()))
    after = max(0.0, float(((V - P[-1]) @ a1).max()))
    head = [P[0] - a0 * d for d in np.arange(before, 0.0, -step)]
    tail = [P[-1] + a1 * d for d in np.arange(step, after + step * 0.5, step)]
    return np.vstack(([np.array(head)] if head else []) + [P] + ([np.array(tail)] if tail else []))


def frames(P, pole):
    """Rotation-minimizing frames along P (tangent, top, side), the first top toward pole."""
    T = np.gradient(P, axis=0)
    T /= np.maximum(np.linalg.norm(T, axis=1, keepdims=True), 1e-9)
    top = np.asarray(pole, float) - T[0] * (np.asarray(pole, float) @ T[0])
    if np.linalg.norm(top) < 1e-6:
        top = np.array([0.0, 0.0, 1.0]) - T[0] * T[0][2]
    top /= np.linalg.norm(top)
    out = []
    for k in range(len(P)):
        if k:
            top = top - T[k] * (top @ T[k])
            top /= max(np.linalg.norm(top), 1e-9)
        out.append((T[k], top, np.cross(T[k], top)))
    return out


def watertight(V, F, voxel):
    """The piece's outer skin only (a voxel remesh, its biggest shell): Tripo's limbs keep plates nested inside each
    other at the joints, and a ray from the line stopped on those inner walls (dents and gashes)."""
    import bpy
    import looter_tripo_rig as tr
    obj = tr.new_object('_gm_skin', V, F)
    obj.data.remesh_voxel_size = voxel
    obj.data.remesh_voxel_adaptivity = 0.0
    with bpy.context.temp_override(active_object=obj, object=obj, selected_objects=[obj], selected_editable_objects=[obj]):
        bpy.ops.object.voxel_remesh()
    Vw, Fw = tr.mesh_arrays(obj.data)
    mesh = obj.data
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    return Vw, Fw


def _rays(tree, P, fr, angles, reach=60.0):
    R = np.full((len(P), len(angles)), np.nan)
    for i, (p, (t, top, side)) in enumerate(zip(P, fr)):
        for j, a in enumerate(angles):
            d = top * math.cos(a) + side * math.sin(a)
            hit = tree.ray_cast(Vector(p), Vector(d), reach)
            if hit[0] is not None:
                R[i, j] = hit[3]
    return R


def _fill(R):
    for j in range(R.shape[1]):
        col = R[:, j]
        bad = ~np.isfinite(col)
        if bad.any() and (~bad).any():
            col[bad] = np.interp(np.nonzero(bad)[0], np.nonzero(~bad)[0], col[~bad])
    return R


def tube(V, F, line, pole, sides=12, spacing=4.0, tip_point=True, voxel=0.3, joints=None, plates=0.7):
    Vw, Fw = watertight(np.asarray(V, float), F, voxel)
    tris = []
    for f in Fw:
        for k in range(1, len(f) - 1):
            tris.append((int(f[0]), int(f[k]), int(f[k + 1])))
    tree = BVHTree.FromPolygons(Vw.tolist(), tris, all_triangles=True)
    P = centre_line(Vw, line)
    angles = -math.pi + 2.0 * math.pi * np.arange(sides) / sides
    # Radii round every station (1 cm apart): rays out from the line to Tripo's surface; the line is moved into the
    # middle of each ring of hits twice (Tripo's slices along the surface put it off centre on a bend).
    for _ in range(3):
        fr = frames(P, pole)
        R = _rays(tree, P, fr, angles)
        ok = np.isfinite(R)
        for i in range(len(P)):
            if ok[i].sum() >= sides * 0.75:
                t, top, side = fr[i]
                dirs = np.outer(np.cos(angles[ok[i]]), top) + np.outer(np.sin(angles[ok[i]]), side)
                P[i] = P[i] + (R[i, ok[i], None] * dirs).mean(0)
        P = _resample(_smooth(P, 2), 1.0)
    fr = frames(P, pole)
    R = _rays(tree, P, fr, angles)
    # Stations whose rays mostly missed lie past the piece's ends.
    good = np.isfinite(R).mean(1) > 0.6
    first, last = int(np.argmax(good)), len(good) - 1 - int(np.argmax(good[::-1]))
    P, R, fr = P[first:last + 1], _fill(R[first:last + 1]), fr[first:last + 1]
    # Each ray kept within 0.6..1.45 of its ring's median, then eased with its neighbours round and along.
    med = np.median(R, axis=1, keepdims=True)
    R = np.clip(R, med * 0.6, med * 1.45)
    R = (np.roll(R, 1, axis=1) + 2.0 * R + np.roll(R, -1, axis=1)) / 4.0
    R[1:-1] = (R[:-2] + 2.0 * R[1:-1] + R[2:]) / 4.0
    if joints:
        R = R * plate_profile(P, joints, plates)[:, None]
    # Rings: spacing apart, closer where the mean radius changes fast.
    mean = R.mean(1)
    change = np.abs(np.gradient(mean))
    density = 1.0 / spacing + np.clip(change, 0.0, 0.5) * 0.35
    cum = np.concatenate([[0.0], np.cumsum(0.5 * (density[1:] + density[:-1]))])
    count = max(int(math.ceil(cum[-1])), 3)
    picks = np.interp(np.linspace(0.0, cum[-1], count + 1), cum, np.arange(len(P)))
    out_V, out_F, uv = [], [], []
    length = float(np.linalg.norm(np.diff(P, axis=0), axis=1).sum())
    s_at = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(P, axis=0), axis=1))])
    rings = []
    for x in picks:
        i = int(min(math.floor(x), len(P) - 2))
        w = x - i
        c = P[i] * (1.0 - w) + P[i + 1] * w
        t, top, side = (fr[i][k] * (1.0 - w) + fr[i + 1][k] * w for k in range(3))
        top = top - t * (top @ t)
        top /= np.linalg.norm(top)
        side = np.cross(t, top)
        r = R[i] * (1.0 - w) + R[i + 1] * w
        u = (s_at[i] * (1.0 - w) + s_at[i + 1] * w) / length
        ring = []
        for j, a in enumerate(angles):
            ring.append(len(out_V))
            out_V.append(c + (top * math.cos(a) + side * math.sin(a)) * r[j])
            uv.append((u, j / sides))                            # v: 0 underneath, 0.5 the top (a = 0)
        rings.append(ring)
    for k in range(len(rings) - 1):
        for j in range(sides):
            j1 = (j + 1) % sides
            out_F.append([rings[k][j], rings[k + 1][j], rings[k + 1][j1], rings[k][j1]])
    # The ends, rounded as a plate's end is (a shoulder ring, then a point: about half a sphere), the tip of a claw drawn
    # straight to its point. The rings' vertices stay first, ring by ring (the mud sleeves read them so).
    count_core = len(out_V)
    peak = float(mean.max())
    for k, end in ((0, 0), (len(rings) - 1, 1)):
        ring = rings[k]
        c = np.mean([out_V[i] for i in ring], axis=0)
        way = (fr[0][0] if end == 0 else fr[-1][0]) * (-1.0 if end == 0 else 1.0)
        r = float(np.mean([np.linalg.norm(out_V[i] - c) for i in ring]))
        u_end = 0.0 if end == 0 else 1.0
        if end == 1 and tip_point and r < 0.35 * peak:
            last = ring
            hub_at = c + way * max(r * 1.6, 0.4)
        else:
            shoulder = []
            for i in ring:
                shoulder.append(len(out_V))
                out_V.append(c + (out_V[i] - c) * 0.72 + way * r * 0.5)
                uv.append((u_end, uv[i][1]))
            for j in range(sides):
                j1 = (j + 1) % sides
                q = [ring[j], shoulder[j], shoulder[j1], ring[j1]]
                out_F.append(q[::-1] if end == 0 else q)
            last = shoulder
            hub_at = c + way * r * 0.82
        hub = len(out_V)
        out_V.append(hub_at)
        uv.append((u_end, 0.5))
        for j in range(sides):
            j1 = (j + 1) % sides
            out_F.append([hub, last[j1], last[j]] if end == 0 else [hub, last[j], last[j1]])
    out_V = np.array(out_V)
    out_F = [np.array(f) for f in out_F]
    # Faces out: the tube's volume positive.
    volume = sum(np.dot(out_V[f[0]], np.cross(out_V[f[k]], out_V[f[k + 1]])) for f in out_F for k in range(1, len(f) - 1))
    if volume < 0:
        out_F = [f[::-1] for f in out_F]
    return out_V, out_F, np.array(uv), dict(rings=len(rings), sides=sides, core=count_core, profile=mean)


def egg_shell(V, F, centre, radii, segs=56, rings=30, clamp=(0.86, 1.14)):
    """The abdomen as clean latitude-longitude quads (looter_creatures.ellipsoid's own grid and UVs, Spider.py's egg on
    it), each point pushed out or in along its ray from the middle to where Tripo's hide is (within clamp of the egg;
    where a ray finds no hide, a hole Tripo's stone and fibres left, the egg's own), the pushes eased with their
    neighbours. Returns (V, F, uv)."""
    V = np.asarray(V, float)
    tris = [(int(f[0]), int(f[k]), int(f[k + 1])) for f in F for k in range(1, len(f) - 1)]
    tree = BVHTree.FromPolygons(V.tolist(), tris, all_triangles=True)
    centre, radii = np.asarray(centre, float), np.asarray(radii, float)

    def egg_point(theta, phi):
        a, r = -math.cos(theta), math.sin(theta)
        s, u = r * math.sin(phi), r * math.cos(phi)
        front = 1.0 - 0.12 * max(a, 0.0)
        return centre + np.array([a, s * front, u * front * (0.8 if u < 0.0 else 1.0)]) * radii
    grid = []
    for i in range(rings + 1):
        row = []
        for j in range(segs if 0 < i < rings else 1):
            row.append(egg_point(math.pi * i / rings, -math.pi + 2.0 * math.pi * j / segs))
        grid.append(row)
    ratio = [np.ones(len(row)) for row in grid]
    for i, row in enumerate(grid):
        for j, e in enumerate(row):
            d = e - centre
            length = np.linalg.norm(d)
            hit = tree.ray_cast(Vector(centre), Vector(d / length), length * 1.6)
            if hit[0] is not None:
                ratio[i][j] = min(max(hit[3] / length, clamp[0]), clamp[1])
    # Eased round each ring and along (the poles keep their ring's mean).
    for _ in range(2):
        new = []
        for i, row in enumerate(ratio):
            if len(row) == 1:
                new.append(row.copy())
                continue
            r = (np.roll(row, 1) + 2.0 * row + np.roll(row, -1)) / 4.0
            up = ratio[i - 1] if len(ratio[i - 1]) > 1 else np.full(len(row), ratio[i - 1][0])
            dn = ratio[i + 1] if len(ratio[i + 1]) > 1 else np.full(len(row), ratio[i + 1][0])
            new.append((up + 2.0 * r + dn) / 4.0)
        new[0] = np.array([new[1].mean()])
        new[-1] = np.array([new[-2].mean()])
        ratio = new
    out_V, uv, index = [], [], []
    for i, row in enumerate(grid):
        ids = []
        for j, e in enumerate(row):
            ids.append(len(out_V))
            out_V.append(centre + (e - centre) * ratio[i][j])
            uv.append((j / segs if len(row) > 1 else 0.5, i / rings))
        index.append(ids)
    out_F = []
    for i in range(rings):
        for j in range(segs):
            j1 = (j + 1) % segs
            if i == 0:
                out_F.append([index[0][0], index[1][j1], index[1][j]])
            elif i == rings - 1:
                out_F.append([index[i][j], index[i][j1], index[rings][0]])
            else:
                out_F.append([index[i][j], index[i][j1], index[i + 1][j1], index[i + 1][j]])
    out_V = np.array(out_V)
    out_F = [np.array(f) for f in out_F]
    volume = sum(np.dot(out_V[f[0]], np.cross(out_V[f[k]], out_V[f[k + 1]])) for f in out_F for k in range(1, len(f) - 1))
    if volume < 0:
        out_F = [f[::-1] for f in out_F]
    return out_V, out_F, np.array(uv)


def plate(s):
    """Spider.py's chitin plate along a segment (s 0..1): a neck leaving the joint, a swell over its first third, easing
    off, a hard rim just before its far end; about 1 on the average."""
    s = np.clip(s, 0.0, 1.0)
    sm = lambda e0, e1, x: np.clip((x - e0) / (e1 - e0), 0.0, 1.0) ** 2 * (3.0 - 2.0 * np.clip((x - e0) / (e1 - e0), 0.0, 1.0))
    body = 0.8 + 0.2 * sm(0.0, 0.28, s) - 0.14 * sm(0.3, 0.85, s)
    return (body + 0.09 * np.exp(-((s - 0.9) / 0.05) ** 2)) / 0.92


def plate_profile(P, joints, strength):
    """A radius factor per station: the plate profile between the joints (shares of the length, 0 and 1 included), so
    the leg reads in its segments where the painter draws their joints (Tripo's own rims are too soft to carry it)."""
    s_at = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(P, axis=0), axis=1))])
    u = s_at / s_at[-1]
    out = np.ones(len(P))
    for a, b in zip(joints, joints[1:]):
        inside = (u >= a) & (u <= b)
        out[inside] = 1.0 + strength * (plate((u[inside] - a) / (b - a)) - 1.0)
    return out
