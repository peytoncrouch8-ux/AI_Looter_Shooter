"""Hollis Crane's gear for his game model (Art/Models/Characters/Crane.py): the toll chain (Docs/Story.md: the
Chainman's, the chain the Unpaid are herded on; "the chain on your shoulder") slung like a bandolier from over the
right shoulder across the chest to the left hip and back up behind, its iron hook hanging from it at the chest on a
few links (the portrait's "hooked"); and the marshal's tin star on the left breast (the portrait's tin_star).

Each link is a stadium of round wire; the detailed links are the bake's source, the game's links are the same at 8
points round and 4 round the wire (64 triangles each). Links lie alternately flat on what's under them and edge on.
"""
import math

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector

import crane_common as cc
import looter_garments as gm
from crane_common import CP, log, slot

LINK = dict(straight=0.020, rr=0.0105, wire=0.0042)    # a heavy chain: the stadium's straight, its half width, the wire
PITCH = 0.030            # a link's inner length: the spacing of links along the chain
LIFT = 0.0055            # the chain's middle off what it lies on (its flat links' wire radius and a little)
HOOK_DROP = 4            # links from the chain down to the hook's eye
LOW_LINK = (6, 4)        # the game's links: points round the stadium, round the wire (48 triangles a link)


def link_mesh(name, around=32, wire=10):
    """One link (unit placement): a stadium in its xy plane, its long axis y, as a closed tube of round wire."""
    s, rr, w = LINK['straight'], LINK['rr'], LINK['wire']
    u = np.linspace(0, 2 * math.pi, around, endpoint=False)
    path = np.stack([rr * np.cos(u), rr * np.sin(u) + s * 0.5 * np.clip(np.sin(u) * 3, -1, 1), np.zeros(around)], 1)
    tang = np.roll(path, -1, 0) - np.roll(path, 1, 0)
    tang /= np.linalg.norm(tang, axis=1, keepdims=True)
    zax = np.array([0.0, 0.0, 1.0])
    out = np.cross(tang, zax)
    v = np.linspace(0, 2 * math.pi, wire, endpoint=False)
    verts = (path[:, None, :] + (np.cos(v)[None, :, None] * out[:, None, :] + np.sin(v)[None, :, None] * zax) * w).reshape(-1, 3)
    faces = []
    for i in range(around):
        for j in range(wire):
            a, b = i * wire + j, i * wire + (j + 1) % wire
            c, d = ((i + 1) % around) * wire + (j + 1) % wire, ((i + 1) % around) * wire + j
            faces.append([a, b, c, d])
    me = bpy.data.meshes.new(name)
    me.from_pydata([tuple(p) for p in verts], [], faces)
    me.update()
    # outward: the tube's quads turned so they face away from the wire's core
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    return me


def place_links(name, path, normals, mesh, start_flat=True, lying=True):
    """Links every PITCH along a path (k, 3), each a quarter turn from the last: the flat ones lie on the surface
    (normals), the others stand edge on (lying: the path is LIFT off a surface, and each link rests on it, an edge-on
    link's centre higher by its half width). Returns one joined object and each link's centre."""
    path, normals = np.asarray(path, float), np.asarray(normals, float)
    s = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(path, axis=0), axis=1))])
    q = np.arange(0.0, s[-1], PITCH)
    P = np.stack([np.interp(q, s, path[:, i]) for i in range(3)], 1)
    Nn = np.stack([np.interp(q, s, normals[:, i]) for i in range(3)], 1)
    bm = bmesh.new()
    centres = []
    src = bmesh.new()
    src.from_mesh(mesh)
    for k in range(len(P) - 1):
        t = Vector(P[k + 1] - P[k]).normalized()
        z = Vector(Nn[k])
        z = (z - t * z.dot(t)).normalized()
        x = t.cross(z)
        nk = Vector(z)
        edge = (k % 2 == 1) == start_flat
        if edge:
            x, z = z, -x
        c = Vector((P[k] + P[k + 1]) * 0.5)
        if lying:
            rest = LINK['wire'] + 0.0012 + (LINK['rr'] if edge else 0.0)
            c = c + nk * (rest - LIFT)
        M = Matrix.Translation(c) @ Matrix((x, t, z)).transposed().to_4x4()
        tmp = src.copy()
        tmp.transform(M)
        me = bpy.data.meshes.new('_link')
        tmp.to_mesh(me)
        tmp.free()
        bm.from_mesh(me)
        bpy.data.meshes.remove(me)
        centres.append(np.array(c))
    src.free()
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    ob.data.materials.append(slot('Iron'))
    me.shade_smooth()
    return ob, np.array(centres)


# ------------------------------------------------------------------------------------------------- the toll chain

def bandolier_path(fig, surfaces, n=180):
    """The chain's loop: where the outermost of surfaces meets a plane through the left hip and the top of the right
    shoulder (spanned by that diagonal and front-to-back), LIFT off it, smoothed. Returns (points, normals, the plane's
    frame), the points going from the shoulder's top down the front, round the hip and up the back."""
    tree = gm.bvh_objects(surfaces)
    hip = np.array([0.155, fig.centre[1], fig.waist - 0.04])
    # (its line over the shoulder near the neck: further out the cape's share of the raised arm came down 2.7 cm
    # through it aiming)
    top = np.array([-0.095, fig.neck[1] + 0.01, fig.shoulder_z + 0.06])
    C = (hip + top) * 0.5
    e1 = gm.unit(top - hip)
    e2 = np.array([0.0, -1.0, 0.0])
    e2 = gm.unit(e2 - e1 * (e2 @ e1))
    pts, nrm = [], []
    for phi in np.linspace(0.0, 2 * math.pi, n, endpoint=False):
        u = math.cos(phi) * e1 + math.sin(phi) * e2
        hit = tree.ray_cast(Vector(tuple(C + u * 0.7)), Vector(tuple(-u)), 0.7)
        if hit[0] is None:
            continue
        pts.append(np.array(hit[0]))
        nrm.append(gm.unit(np.array(hit[1])))
    pts, nrm = np.array(pts), np.array(nrm)
    ring = [[i, (i + 1) % len(pts)] for i in range(len(pts))]
    # a chain under tension bridges hollows (the coat's open front, the spine's groove): its line the hull's, smoothed
    pts = gm.laplacian(pts, ring, 6)
    for k, p in enumerate(pts):
        loc, nn, _i, _d = tree.find_nearest(Vector(tuple(p)), 0.2)
        if loc is not None and (Vector(tuple(p)) - loc).dot(nn) < LIFT:
            pts[k] = np.array(loc + nn * LIFT)
        if loc is not None:
            nrm[k] = gm.unit(np.array(nn))
    nrm = gm.laplacian(nrm, ring, 3)
    nrm = gm.unit(nrm)
    return np.vstack([pts, pts[:1]]), np.vstack([nrm, nrm[:1]]), (C, e1, e2)


def build_chain(fig, surfaces):
    """The toll chain over the right shoulder (under the cape there: the cape is draped over it), across the chest to
    the left hip and up behind: detailed links for the bake; the hook's place on it (fig.hook_at)."""
    path, nrm, frame = bandolier_path(fig, surfaces)
    fig.chain_path = (path, nrm)
    detailed = link_mesh('_LinkHi', 32, 10)
    chain, centres = place_links('Chain', path, nrm, detailed)
    # the hook hangs from the link on the chest's front nearest this height, right of the middle
    z_hook = fig.shoulder_z - 0.07
    front = centres[(centres[:, 1] < fig.centre[1]) & (centres[:, 0] < 0.02)]
    k = int(np.argmin(np.abs(front[:, 2] - z_hook)))
    fig.hook_at = front[k] + np.array([0.0, -0.004, -0.012])
    fig.chain_centres = centres
    log(f'chain: {len(centres)} links over {np.linalg.norm(np.diff(path, axis=0), axis=1).sum():.2f} m; the hook hangs '
        f'from ({fig.hook_at[0] * 100:.1f}, {fig.hook_at[1] * 100:.1f}, {fig.hook_at[2] * 100:.1f}) cm')
    return chain


def chain_low(fig):
    """The chain's game links: the same places, LOW_LINK points round each link and round its wire."""
    path, nrm = fig.chain_path
    low = link_mesh('_LinkLow', *LOW_LINK)
    ob, _ = place_links('Chain_low', path, nrm, low)
    return ob


HOOK = dict(shank=0.070, bend=0.030, wire=0.0075)


def build_hook(fig, low=False):
    """The iron hook on HOOK_DROP links below the chain: a J (the portrait's hook(): a shank, a half circle and on,
    tapering to a point) with an eye at its top; the drop's links hang edge on and flat in turn. Returns the parts
    (drop links, hook) and sets fig.hook_line (the eye's top, the hook's foot) for its swing bone."""
    a = np.array(fig.hook_at)
    down = np.array([0.0, 0.0, -1.0])
    drop = [a + down * PITCH * k for k in range(HOOK_DROP + 1)]
    nrm = [np.array([0.0, -1.0, 0.0])] * len(drop)
    mesh = link_mesh('_LinkLow2' if low else '_LinkHi2', *(LOW_LINK if low else (32, 10)))
    links, _ = place_links('HookLinks' + ('_low' if low else ''), drop, nrm, mesh, start_flat=False, lying=False)
    eye = drop[-1] + down * 0.004
    e1 = np.array([1.0, 0.0, 0.0])                        # the bend opens toward his left
    L, Rb, r0 = HOOK['shank'], HOOK['bend'], HOOK['wire']
    c = eye + down * L + e1 * Rb
    pts = [eye + down * (L * t) for t in np.linspace(0, 1, 8)]
    pts += [c + e1 * (Rb * math.cos(t)) + down * (-Rb * math.sin(t)) for t in np.linspace(math.pi, 2 * math.pi + 0.7, 22)[1:]]
    pts = np.array(pts)
    n = len(pts)
    radii = np.array([r0 * min(1.0, max(0.15, 1.0 - (i - n * 0.6) / (n * 0.42))) for i in range(n)])
    hook = gm.mesh('Hook' + ('_low' if low else ''), gm.tube(pts, radii, n=6 if low else 14, cap0=True, cap1=True),
                   slot('Iron'))
    ring = gm.tube(np.array([eye + np.array([0.0, 0.0, 0.006]) + 0.007 * np.array([math.cos(t), 0.0, math.sin(t)])
                             for t in np.linspace(0, 2 * math.pi, 13 if low else 25)]),
                   np.full(13 if low else 25, 0.0032), n=4 if low else 8)
    eye_ob = gm.mesh('HookEye' + ('_low' if low else ''), ring, slot('Iron'))
    fig.hook_line = (np.array(fig.hook_at), pts[int(np.argmin(pts[:, 2]))].copy())
    return [links, hook, eye_ob]


# ------------------------------------------------------------------------------------------------- the tin star

STAR_AT = (0.120, 0.260)    # on the coat's left breast below the hair's lock (higher, it hid the star), between the V and
                            # the cape's front corner: m to his left of the neck's middle, m below its base


def build_star(fig, surfaces):
    """The marshal's six-point tin star with a ball on every point and a raised boss (the portrait's tin_star), pinned
    on the coat's left breast (surfaces: the coat) 3 mm off it. (Pinned to the cape's front corner, it rode that cloth's
    share of the upper arm and went under the coat as the arms came down.) Its weights: the coat's there, whole (rig)."""
    tree = gm.bvh_objects(surfaces)
    x, z = fig.neck[0] + STAR_AT[0], fig.neck[2] - STAR_AT[1]
    hit = tree.ray_cast(Vector((x, -0.6, z)), Vector((0, 1, 0)), 1.5)
    at, n = (hit[0], hit[1]) if hit[0] is not None else (Vector((x, -0.15, z)), Vector((0, -1, 0)))
    star = CP.tin_star(at + n * 0.003, n, False)
    for m in list(star.modifiers):
        star.modifiers.remove(m)
    star.data.materials.clear()
    star.data.materials.append(slot('Tin'))
    star.name = star.data.name = 'Star'
    bpy.context.view_layer.update()        # (tin_star sets location and rotation: its matrix is stale until updated,
    cc.bake_transform(star)                # and baked so, the star sat at the origin between his feet)
    fig.star_at = np.array(at)
    log(f'star: at ({at.x * 100:.1f}, {at.y * 100:.1f}, {at.z * 100:.1f}) cm on {"the outermost of " + ", ".join(o.name for o in surfaces)}')
    return star
