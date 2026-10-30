"""SK_Gravemother's Tripo pieces (parts_gravemother_build.py) as Gravemother.py's parts: mapped onto Spider.py's atlas so
her own painter (GRAVE: the pale bone hide, the earth climbing from below, the stone's print, the roots' veins) paints
them as it painted the scripted spider, in her slots; the mud caked on her lower legs; her skin (each piece rigid on its
bone) and hit hulls. Not a model: Tools\\models.ps1 skips parts_*.py.

- tripo_spider(S): Spider.py's names with her abdomen the Tripo one (an egg on AB_CENTRE, AB_RADII), and looter_barrow
  told so: the headstone keeps its scripted place (the game's: GravemotherCreatureStone.cpp), the roots enter her hide
  where they did, seen from the stone. Every barrow helper (roots, earth collar, stubs, marks) then fits her.
- The atlas: the legs' tubes along and round (coxa + femur on the femur bone, tibia on the tibia bone), the palps and
  claws on 'palp', the chelicerae on 'chel' (their fangs' last half on the fang patch), the cephalothorax as Spider.py's
  carapace and head ellipsoids (latitude-longitude round each, split at X_HEAD), the abdomen on its egg, the eyes (Tripo's
  milky ones, now our glossy black) on the eye patch. The skulls wear BoneDry (the den's bone).
- Mud: a crust over each tibia from a ragged line part way down, thickening toward the claw (Gravemother.py's rule, laid
  on the tube's own rings so it hugs Tripo's leg)."""
import math

import bmesh
import bpy
import numpy as np
from mathutils import Vector, noise

import looter_barrow as lb
import looter_creatures as lc
import parts_gravemother as pg

X_HEAD = 28.0          # creature cm: the cephalothorax's head part (the head bone, her crit) lies in front of this
COXA_SHARE = 0.2       # the femur bone's tube: its first fifth is the coxa and trochanter (the coxa region)
FANG_FROM = 0.55       # a chelicera's tube from here on is its fang (the fang patch)
UV_MUD = lb.SCALE * 0.01 * 0.5


def tripo_spider(S):
    """S with the Tripo abdomen, looter_barrow patched to match (see the docstring). Call once, before anything reads
    the stone's frame or the roots."""
    fixed = lb.stone_frame(S)
    anchors = [lb.root_anchor(S, root)[0] for root in lb.ROOTS]
    ride = S['RIDE']
    spec = dict(S['ABDOMEN'])
    spec.update(center=(pg.AB_CENTRE[0], pg.AB_CENTRE[1], pg.AB_CENTRE[2] - ride), radii=tuple(pg.AB_RADII),
                fwd=(1.0, 0.0, 0.0), deform=S['egg'](0.12, 0.8))
    T = dict(S)
    T['ABDOMEN'] = spec
    lb.stone_frame = lambda _S: fixed.copy()
    ab = lb.Abdomen(T)
    lb.ROOTS[:] = [dict(root, anchor=ab.coords(p)) for root, p in zip(lb.ROOTS, anchors)]
    return T


# --- UVs ---

def ellipsoid_uv(P, centre, radii, egg=False):
    """lc.ellipsoid's latitude-longitude (U round, 0.5 the top; V along x, 0 the rear pole) of points near an ellipsoid
    along x; egg: Spider.py's egg undone first (looter_barrow.Abdomen.from_uv's inverse)."""
    rel = (np.asarray(P, float) - centre) / radii
    a, s, u = rel[:, 0], rel[:, 1], rel[:, 2]
    if egg:
        front = 1.0 - 0.12 * np.maximum(a, 0.0)
        s = s / front
        u = u / (front * np.where(u < 0.0, 0.8, 1.0))
    n = np.sqrt(a * a + s * s + u * u) + 1e-9
    theta = np.arccos(np.clip(-a / n, -1.0, 1.0))
    phi = np.arctan2(s, u)
    return (phi + math.pi) / (2.0 * math.pi), theta / math.pi


def region_uv(atlas, region, u, v):
    u0, u1, v0, v1 = atlas[region]
    return u0 + np.clip(u, 0.0, 1.0) * (u1 - u0), v0 + np.clip(v, 0.0, 1.0) * (v1 - v0)


def unwrap(F, U):
    """Per-face corner copies of a wrapping coordinate (U round a body or v round a tube): a face across the seam gets
    its small values lifted by one."""
    out = []
    for f in F:
        c = U[f].copy()
        if c.max() - c.min() > 0.5:
            c = np.where(c < 0.5, c + 1.0, c)
        out.append(c)
    return out


def patch_uv(atlas, region, count):
    """A face on a solid patch: its corners spread a few texels round the patch's middle (no face without UV area)."""
    u0, u1, v0, v1 = atlas[region]
    cu, cv = (u0 + u1) * 0.5, (v0 + v1) * 0.5
    return [(cu + 0.003 * math.cos(2.0 * math.pi * k / count), cv + 0.003 * math.sin(2.0 * math.pi * k / count))
            for k in range(count)]


def mesh_object(name, V, F, corner_uv, slots, face_slot, smooth_angle=60.0):
    """A Blender object (creature cm to Blender metres) with per-corner UVs and a material per face."""
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    verts = [bm.verts.new(lc.to_blender(Vector(p))) for p in V]
    for i, f in enumerate(F):
        try:
            face = bm.faces.new([verts[k] for k in f])
        except ValueError:
            continue
        face.material_index = face_slot[i]
        face.smooth = True
        for loop, uv in zip(face.loops, corner_uv[i]):
            loop[uvl].uv = uv
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    for mat in slots:
        mesh.materials.append(mat)
    mesh.set_sharp_from_angle(angle=math.radians(smooth_angle))
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def tube_uvs(atlas, piece):
    """A tube's corner UVs on its regions: (per-face corner UVs, per-face slot 0)."""
    uv = np.asarray(piece['uv'])
    kind = piece['kind']
    F = piece['F']
    V_around = unwrap(F, uv[:, 1])
    out = []
    for f, vv in zip(F, V_around):
        u = uv[f, 0]
        mid = u.mean()
        if kind == 'femur':
            if mid < COXA_SHARE:
                cu, cv = region_uv(atlas, 'coxa', u / COXA_SHARE, vv)
            else:
                cu, cv = region_uv(atlas, 'femur', (u - COXA_SHARE) / (1.0 - COXA_SHARE), vv)
        elif kind == 'tibia':
            cu, cv = region_uv(atlas, 'tibia', u, vv)
        elif kind in ('palp', 'claw'):
            cu, cv = region_uv(atlas, 'palp', u, vv)
        elif mid >= FANG_FROM:
            out.append(patch_uv(atlas, 'fang', len(f)))
            continue
        else:
            cu, cv = region_uv(atlas, 'chel', u / FANG_FROM, vv)
        out.append(list(zip(cu, cv)))
    return out


def thorax_uvs(atlas, piece, carapace, head):
    """The cephalothorax's corner UVs: all of it on the carapace region, one ellipsoid's latitude-longitude (a head region
    of its own drew a seam across her crown, and its front pole a pinch over her face). The eyes are spheres of their own."""
    V, F = piece['V'], piece['F']
    Uc, Vc = ellipsoid_uv(V, *carapace)
    out = []
    for k, (f, uu) in enumerate(zip(F, unwrap(F, Uc))):
        cu, cv = region_uv(atlas, 'carapace', uu, Vc[f])
        out.append(list(zip(cu, cv)))
    return out


def eyes(atlas, eye_points, head_centre, material):
    """Her eyes as Spider.py's: glossy black spheres on the eye patch, one on each of Tripo's milky eyes (its pale
    vertices clustered), sunk a third into the face. Returns the object, or None."""
    P = np.asarray(eye_points, float)
    group = -np.ones(len(P), int)
    count = 0
    for i in range(len(P)):
        if group[i] >= 0:
            continue
        stack = [i]
        group[i] = count
        while stack:
            k = stack.pop()
            near = np.nonzero((np.linalg.norm(P - P[k], axis=1) < 2.2) & (group < 0))[0]
            group[near] = count
            stack.extend(near.tolist())
        count += 1
    m = lc.Mesh(atlas)
    made = 0
    for g in range(count):
        Q = P[group == g]
        if len(Q) < 3:
            continue
        c = Q.mean(0)
        r = max(float(np.linalg.norm(Q - c, axis=1).max()) * 0.85, 1.2)
        n = (c - head_centre) / np.linalg.norm(c - head_centre)
        big = r > 2.5
        lc.ellipsoid(m, Vector(c - n * r * 0.35), (r, r, r), 'eye', segs=12 if big else 8, rings=8 if big else 5,
                     patch=True)
        made += 1
    pg.log(f'{made} eyes from Tripo\'s {count} pale patches')
    return m.finish('Eyes', material) if made else None


def fit_ellipsoid(P):
    """An ellipsoid along x round points: centre and half extents (a quick bounds fit: the painter only needs the
    latitude-longitude to run evenly over the shape)."""
    lo, hi = P.min(0), P.max(0)
    return (lo + hi) * 0.5, (hi - lo) * 0.5


def abdomen_uvs(atlas, piece):
    V, F = piece['V'], piece['F']
    if piece.get('uv') is not None:
        U, Vv = piece['uv'][:, 0], piece['uv'][:, 1]          # its own grid's (parts_gravemother_tube.egg_shell)
    else:
        U, Vv = ellipsoid_uv(V, pg.AB_CENTRE, pg.AB_RADII, egg=True)
    out = []
    for f, uu in zip(F, unwrap(F, U)):
        # The abdomen's region spans the atlas's width: past its seam the texture wraps on round (no clamp).
        u0, u1, v0, v1 = atlas['abdomen']
        out.append(list(zip(u0 + uu * (u1 - u0), v0 + np.clip(Vv[f], 0.0, 1.0) * (v1 - v0))))
    return out


def box_uvs(V, F, per):
    """World-scale cube projection (looter_barrow.box_uv_faces's) per face."""
    out = []
    for f in F:
        P = V[f]
        n = np.cross(P[1] - P[0], P[2] - P[0])
        axis = int(np.argmax(np.abs(n)))
        a, b = [(1, 2), (0, 2), (0, 1)][axis]
        out.append([(p[a] * per, p[b] * per) for p in P])
    return out


# --- Mud on the lower legs ---

def mud_sleeve(piece, seed):
    """Grave mud caked on a tibia (Gravemother.py's mud_sleeve, on the tube's own rings): a crust from a ragged line part
    way down, thickening toward the claw and thinning off again just above it, lumpy, patchy. (V, F) creature cm."""
    info = piece['profile']
    rings, sides = info['rings'], info['sides']
    V = np.asarray(piece['V'])[:rings * sides].reshape(rings, sides, 3)
    u = np.asarray(piece['uv'])[:rings * sides:sides, 0]
    centre = V.mean(1)
    t0, t1 = 0.3, 0.96
    keep = [k for k in range(rings) if t0 <= u[k] <= t1]
    out_V = []
    for k in keep:
        t = u[k]
        for j in range(sides):
            a = 2.0 * math.pi * j / sides
            out = V[k, j] - centre[k]
            r = np.linalg.norm(out)
            d = out / max(r, 1e-9)
            start = 0.5 + 0.09 * noise.noise(Vector((math.cos(a) * 1.3, math.sin(a) * 1.3, seed * 0.7)))
            on = lc.smoothstep(start - 0.02, start + 0.1, t)
            thick = on * (0.35 + 0.65 * lc.smoothstep(0.55, 0.88, t)) * lc.smoothstep(0.97, 0.88, t)
            patch = noise.noise(Vector((math.cos(a), math.sin(a), t * 5.0 + seed)))
            thick *= 0.3 + 0.7 * lc.smoothstep(-0.35, 0.45, patch)
            thick *= 1.0 + 0.45 * lb.fbm(Vector(V[k, j] * 0.22) + Vector(d) * 1.1 + Vector((seed, 0.0, 0.0)), 3)
            out_V.append(centre[k] + d * r * (0.96 + 0.6 * max(thick, 0.0)))
    out_V = np.array(out_V)
    F = []
    n = len(keep)
    for k in range(n - 1):
        for j in range(sides):
            j1 = (j + 1) % sides
            F.append(np.array([k * sides + j, k * sides + j1, (k + 1) * sides + j1, (k + 1) * sides + j]))
    # Its two open ends sit inside the leg (thickness zero there, 4% in): closed with a hub each all the same.
    for k, end in ((0, 0), (n - 1, 1)):
        hub = len(out_V)
        out_V = np.vstack([out_V, centre[keep[k]]])
        for j in range(sides):
            j1 = (j + 1) % sides
            F.append(np.array([hub, k * sides + j1, k * sides + j] if end == 0 else [hub, k * sides + j, k * sides + j1]))
    volume = sum(np.dot(out_V[f[0]], np.cross(out_V[f[i]], out_V[f[i + 1]])) for f in F for i in range(1, len(f) - 1))
    if volume < 0:
        F = [f[::-1] for f in F]
    return out_V, F


# --- Skin ---

def skin(rig, name, entries):
    """entries: [(object, per-vertex bone names)]: each vertex wholly on its bone (her pieces are rigid chitin, earth
    and bone). Joined into one mesh called name, on rig."""
    objects = []
    for obj, names in entries:
        for bone in sorted(set(names)):
            group = obj.vertex_groups.get(bone) or obj.vertex_groups.new(name=bone)
            group.add([i for i, b in enumerate(names) if b == bone], 1.0, 'REPLACE')
        objects.append(obj)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    body = bpy.context.view_layer.objects.active
    body.name = body.data.name = name
    body.parent = rig
    body.modifiers.new('Armature', 'ARMATURE').object = rig
    return body


def points_object(name, P):
    """Vertices only (a hit hull's input)."""
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([tuple(lc.to_blender(Vector(p))) for p in P], [], [])
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


# --- All of her ---

TUBE_KINDS = ('femur', 'tibia', 'palp', 'claw', 'chel')


def assemble(pieces, extra, atlas, mats):
    """Her pieces as objects in her slots (mats: hide, abdomen, mud, bone). Returns (entries for skin(), {bone: [points
    a hit hull wraps]})."""
    entries, hulls = [], {}

    def add(obj, bone_names, V):
        entries.append((obj, bone_names))
        for bone in sorted(set(bone_names)):
            hulls.setdefault(bone, []).append(np.asarray(V)[[i for i, b in enumerate(bone_names) if b == bone]])

    th = pieces['thorax']
    V, F = th['V'], th['F']
    carapace = fit_ellipsoid(V)
    head = fit_ellipsoid(V[V[:, 0] >= X_HEAD - 6.0])
    obj = mesh_object('Thorax', V, F, thorax_uvs(atlas, th, carapace, head), [mats['hide']], [0] * len(F))
    add(obj, ['head' if p[0] >= X_HEAD else 'body' for p in V], V)
    obj = eyes(atlas, extra['eye_points'], head[0], mats['hide'])
    if obj is not None:
        Ve = np.array([tuple(lc.TO_BLENDER.inverted() @ v.co) for v in obj.data.vertices])
        add(obj, ['head'] * len(Ve), Ve)
    seed = 0
    for name, p in sorted(pieces.items()):
        if p['kind'] not in TUBE_KINDS:
            continue
        V, F = p['V'], p['F']
        obj = mesh_object(name.title().replace('_', ''), V, F, tube_uvs(atlas, p), [mats['hide']], [0] * len(F))
        add(obj, [p['bone']] * len(V), V)
        if p['kind'] == 'tibia':
            Vm, Fm = mud_sleeve(p, seed)
            seed += 1
            obj = mesh_object('Mud' + name.title().replace('_', ''), Vm, Fm, box_uvs(Vm, Fm, UV_MUD), [mats['mud']],
                              [0] * len(Fm), smooth_angle=75.0)
            add(obj, [p['bone']] * len(Vm), Vm)
    ab = pieces['abdomen']
    V, F = ab['V'], ab['F']
    obj = mesh_object('Abdomen', V, F, abdomen_uvs(atlas, ab), [mats['abdomen']], [0] * len(F))
    add(obj, ['abdomen'] * len(V), V)
    # Spinnerets at its rear (Spider.py's, her size): they also hide the latitude-longitude's pole there.
    rear = Vector(V[np.argmin(V[:, 0])])
    m = lc.Mesh(atlas)
    for side in (1.0, -1.0):
        lc.tube(m, [rear + Vector((6.0, side * 4.0, -2.5)), rear + Vector((-7.0, side * 5.5, -6.5))], [4.6, 1.6], 'misc',
                sides=7, patch=True)
    obj = m.finish('Spinnerets', mats['abdomen'])
    Vs = np.array([tuple(lc.TO_BLENDER.inverted() @ v.co) for v in obj.data.vertices])
    add(obj, ['abdomen'] * len(Vs), Vs)
    for k in range(2):
        sk = pieces.get(f'skull{k}')
        if sk is None:
            continue
        V, F = sk['V'], sk['F']
        obj = mesh_object(f'Skull{k}', V, F, box_uvs(V, F, 0.02), [mats['bone']], [0] * len(F), smooth_angle=50.0)
        add(obj, ['abdomen'] * len(V), V)
    return entries, hulls


def body(S, mats, stubs):
    """SK_Gravemother's rig and skin from the Tripo pieces: Spider.py's bones (the fangs' moved onto Tripo's chelicerae:
    the game turns a fang about its bone's head), every piece on its bone, a hit hull round each bone's pieces (stubs: the
    root stubs, on the abdomen). Returns (rig, the skinned mesh, the bones as laid)."""
    import looter_model as lm
    import parts_gravemother_build as pb
    bones = list(S['bones'])
    pieces, extra = pb.build({n: (np.array(h[:]), np.array(t[:])) for n, h, t, p in bones})
    for i, (n, h, t, p) in enumerate(bones):
        if n in extra['fangs']:
            root, tip = extra['fangs'][n]
            bones[i] = (n, Vector(root), Vector(tip), p)
    rig = lm.armature()
    lm.bones(rig, [(n, lc.to_blender(h), lc.to_blender(t), p) for n, h, t, p in bones])
    entries, hulls = assemble(pieces, extra, S['ATLAS'], mats)
    entries.append((stubs, ['abdomen'] * len(stubs.data.vertices)))
    for bone, sets in sorted(hulls.items()):
        points = points_object(f'_hull_{bone}', np.vstack(sets))
        lm.hit_hull(rig, bone, [points] + ([stubs] if bone == 'abdomen' else []))
        mesh = points.data
        bpy.data.objects.remove(points)
        bpy.data.meshes.remove(mesh)
    return rig, skin(rig, 'Gravemother', entries), bones
