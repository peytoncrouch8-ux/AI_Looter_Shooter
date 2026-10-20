"""The heroes' first-person hands (Tools/Blender/fp_arms_heroes.py): each one a distance field on Ellis's bones, meshed
the way Art/Models/Characters/FPArms.py meshes his glove (surface nets, relaxed onto the field, decimated with the
joints spared), but shaped by a HandSpec: a slimmer or heavier hand on the same chains (their joints are the bones;
only the radii change), a thinner or deeper palm, bony knuckles and joints, tendons on the back of a bare hand, and a
bare forearm that runs on up inside the sleeve (a gloved hand stops at its cuff, as Ellis's). Odessa's glove ends in a
short flared cuff (Flare), built like his gauntlet without its concho.

Hand frame (cm, as FPArms.py's): x along the fingers (the wrist joint at 0, up the forearm negative), y toward the
thumb, z out of the back of the hand.

Also here: the bones' weights (FPArms.py's hand_weights on these chains) and the per-texel detail fields the texture
overlays paint from (fp_arms_paint): a glove's seams, points and creases, a bare hand's nails, knuckle wrinkles and palm
lines.
"""
import math

import bmesh
import bpy
import numpy as np

from fp_arms_base import A, C, capsule, chains, ellipsoid, log, smax, smin, smoothstep, unit, v3

PALM_OUTLINE = A['PALM_OUTLINE']
outline_sdf, chain_sdf, chain_field = A['outline_sdf'], A['chain_sdf'], A['chain_field']
surface_nets, relax, stitches = A['surface_nets'], A['relax'], A['stitches']


class HandSpec:
    """One hero's hand against Ellis's glove (cm; scales are shares of his):
    finger, thumb, tip    the fingers' and thumb's radii, the end joints' taper (FPArms' chains, joints unchanged)
    palm_top, palm_bottom the back's dome over the bones' plane and the palm's depth under it (Ellis 1.05, 1.45: keep the
                          palm's underside within 2 mm of his, so FPArms_rig.json's GripPoint holds for every hero)
    heel, cup             the heel's extra depth and the palm's hollow
    outline               the palm's width (its outline's y scaled about the middle finger's line)
    knuckle, lift         the knuckles' bumps (share of the finger's radius) and how far they stand up
    bulge                 bony middle and end joints (share of the radius they swell by)
    tendons               the extensor tendons' ridges on the back of a bare hand (cm proud)
    wrist                 the wrist's (radius at the forearm end, at the hand, flatness)
    forearm               None (a glove: the wrist ends inside its cuff) or (x of its end, radius there, flatness): a
                          bare forearm up inside the sleeve
    pads                  the thenar and hypothenar pads' size
    tris                  one hand's triangles after decimation"""

    def __init__(self, finger=1.0, thumb=None, tip=1.0, palm_top=1.05, palm_bottom=1.45, heel=0.45, cup=0.3,
                 outline=1.0, knuckle=0.92, lift=0.05, bulge=0.0, tendons=0.0, wrist=(2.95, 2.8, 0.72), forearm=None,
                 pads=1.0, tris=1950):
        self.finger, self.thumb, self.tip = finger, thumb, tip
        self.palm_top, self.palm_bottom, self.heel, self.cup = palm_top, palm_bottom, heel, cup
        self.outline, self.knuckle, self.lift, self.bulge, self.tendons = outline, knuckle, lift, bulge, tendons
        self.wrist, self.forearm, self.pads, self.tris = wrist, forearm, pads, tris
        self.chains = chains(finger, thumb, tip)

    @property
    def x_min(self):
        """The hand mesh's far end up the forearm (cm)."""
        if self.forearm is None:
            return -7.0 - self.wrist[0]
        return self.forearm[0] - self.forearm[1]


# ------------------------------------------------------------------------------------------------ the field
def outline_points(spec):
    return [(x, y * spec.outline + 1.15 * (1.0 - spec.outline)) for x, y in PALM_OUTLINE]


def palm_sdf(P, spec):
    x, y, z = P[:, 0] / C, P[:, 1] / C, P[:, 2] / C
    d2 = outline_sdf(x, y, outline_points(spec))
    top = spec.palm_top - 0.22 * (y / 4.2) ** 2 + 0.12 * smoothstep(6.0, 9.0, x)
    heel = spec.heel * (1.0 - smoothstep(1.0, 5.0, x))
    cup = spec.cup * np.exp(-((x - 5.5) / 2.2) ** 2 - (y / 2.0) ** 2)
    slab = smax(z - top, -spec.palm_bottom - heel + cup - z, 0.6)
    return smax(d2, slab, 2.6) * C


def body_sdf(P, spec):
    """The palm, the wrist (and a bare forearm), the pads, the knuckles and a bare hand's tendons."""
    chs = spec.chains
    d = palm_sdf(P, spec)
    r0, r1, flat = spec.wrist
    d = smin(d, capsule(P, v3(-7.0, 0.0, -0.2) * C, v3(0.5, 0.0, -0.2) * C, r0 * C, r1 * C, up=v3(0, 0, 1.0),
                        flat=flat), 1.5 * C)
    if spec.forearm is not None:
        xe, re, fe = spec.forearm
        d = smin(d, capsule(P, v3(xe, 0.0, -0.45) * C, v3(-4.0, 0.0, -0.25) * C, re * C, r0 * C, up=v3(0, 0, 1.0),
                            flat=fe), 2.5 * C)
        # the forearm's two bones end in the wrist's knobs: the ulna's on the little finger's side, out of the back
        d = smin(d, ellipsoid(P, v3(-1.6, -2.15, 0.15) * C, [v3(1, 0, 0), v3(0, 1, 0), v3(0, 0, 1)],
                              v3(1.2, 0.75, 0.8) * C), 0.8 * C)
    k = spec.pads
    ax = unit((0.75, 0.5, -0.25))
    side = unit(np.cross(ax, (0.0, 0.0, 1.0)))
    d = smin(d, ellipsoid(P, v3(3.4, 2.3, -1.15) * C, [ax, side, np.cross(side, ax)], v3(3.0, 1.75, 1.3) * C * k),
             1.2 * C)
    d = smin(d, ellipsoid(P, v3(4.0, -2.75, -0.95) * C, [v3(1, 0, 0), v3(0, 1, 0), v3(0, 0, 1)],
                          v3(3.6, 1.25, 1.1) * C * k), 1.0 * C)
    for ch in chs[:4]:
        knuckle = np.linalg.norm(P - (ch.joints[1] + v3(0, 0, spec.lift) * C), axis=1) - ch.radii[1] * spec.knuckle
        d = smin(d, knuckle, 0.9 * C)
    if spec.tendons > 0.0:
        # the extensor tendons fanning from the middle of the wrist out to each knuckle, standing just proud
        for ch in chs[:4]:
            kn = ch.joints[1] / C
            a = v3(0.6, kn[1] * 0.3, 0.55 + spec.tendons)
            b = v3(kn[0] - 1.1, kn[1], 0.62 + spec.tendons)
            d = smin(d, capsule(P, a * C, b * C, 0.3 * C, 0.36 * C), 0.55 * C)
    return d


def finger_sdf(P, ch, spec):
    f = chain_sdf(P, ch)
    if spec.bulge > 0.0:
        # bony joints: the middle and end joints (the thumb's one) swell a little past the bones either side
        first = 1 if ch.name == 'thumb' else 2
        for j in range(first, len(ch.joints) - 1):
            r = ch.radii[j] * (1.0 + spec.bulge)
            ball = np.linalg.norm(P - ch.joints[j], axis=1) - r
            f = smin(f, ball, 0.35 * C)
    return f


def hand_sdf(P, spec):
    body = body_sdf(P, spec)
    out = body
    for ch in spec.chains:
        f = finger_sdf(P, ch, spec)
        if ch.name == 'thumb':
            along = (P - ch.joints[0]) @ unit(ch.joints[1] - ch.joints[0])
            k = 0.3 * C + 1.3 * C * (1.0 - smoothstep(0.03, 0.055, along))
        else:
            along = (P - ch.joints[1]) @ unit(ch.joints[2] - ch.joints[1])
            k = 0.15 * C + 0.85 * C * (1.0 - smoothstep(-0.005, 0.025, along))
        # each finger blends into the palm, never into its neighbours (a hard min between them)
        out = np.minimum(out, smin(body, f, k))
    return out


def hand_mesh(spec, name='Hand_r', h=0.0015):
    """The hand's surface (hand frame, m): sampled, meshed, relaxed onto the field, decimated (more triangles round the
    joints, where the code bends it). Returns the object (smart-projected UVs) and its vertices (hand frame, m)."""
    sdf = lambda Q: hand_sdf(Q, spec)
    lo, hi = v3(spec.x_min - 0.6, -6.5, -8.5) * C, v3(22.0, 10.5, 4.0) * C
    n = np.ceil((hi - lo) / h).astype(int) + 1
    X, Y, Z = np.meshgrid(*[lo[i] + np.arange(n[i]) * h for i in range(3)], indexing='ij')
    Q = np.stack([X.ravel(), Y.ravel(), Z.ravel()], axis=1)
    del X, Y, Z
    # in slabs: the field's temporaries over a whole bare forearm's grid ran to gigabytes
    F = np.concatenate([sdf(Q[i:i + 400000]) for i in range(0, len(Q), 400000)]).reshape(n)
    del Q
    verts, quads = surface_nets(F, lo, h)
    del F
    verts = relax(verts, quads, sdf)
    log(f'{name}: field {tuple(n)}, {len(quads)} quads')
    mesh = bpy.data.meshes.new('_HandDense')
    mesh.from_pydata([tuple(p) for p in verts], [], [tuple(int(i) for i in q) for q in quads])
    mesh.validate()
    obj = bpy.data.objects.new('_HandDense', mesh)
    bpy.context.scene.collection.objects.link(obj)
    protect = obj.vertex_groups.new(name='Protect')
    w = np.full(len(verts), 0.08)
    for ch in spec.chains:
        d, s, _, _, _ = chain_field(verts, ch)
        near = d < 0.006
        bend = np.zeros(len(verts))
        for sj in ch.cum[1:-1]:
            bend = np.maximum(bend, np.exp(-((s - sj) / 0.011) ** 2))
        w = np.where(near & (s > -0.006), np.maximum(w, 0.65 + 0.35 * bend), w)
    # the forearm inside the sleeve needs few triangles; the wrist where it shows needs its share
    x = verts[:, 0] / C
    w = np.where(x < -4.0, np.minimum(w, 0.02 + 0.06 * smoothstep(-9.0, -4.0, x)), w)
    for i, wi in enumerate(w):
        protect.add([i], float(wi), 'REPLACE')
    mod = obj.modifiers.new('Decimate', 'DECIMATE')
    mod.ratio = spec.tris / (2.0 * len(quads))
    mod.use_collapse_triangulate = True
    mod.vertex_group = 'Protect'
    mod.vertex_group_factor = 6.0
    depsgraph = bpy.context.evaluated_depsgraph_get()
    low = bpy.data.meshes.new_from_object(obj.evaluated_get(depsgraph))
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    bm = bmesh.new()
    bm.from_mesh(low)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    bmesh.ops.dissolve_degenerate(bm, dist=2e-5, edges=bm.edges[:])
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.verts.index_update()
    v = np.array([vv.co[:] for vv in bm.verts])
    f = [[vv.index for vv in face.verts] for face in bm.faces]
    bm.free()
    bpy.data.meshes.remove(low)
    me = bpy.data.meshes.new(name)
    me.from_pydata([tuple(p) for p in v], [], f)
    me.validate()
    me.shade_smooth()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    smart_uv(obj)
    log(f'{name}: {len(f)} triangles')
    return obj, v


def smart_uv(obj, angle=40.0):
    """Smart projection (40 degrees: at FPArms.py's 60 a few faces seen nearly edge-on were squeezed into slivers of
    the atlas, and the painted seams aliased across them)."""
    me = obj.data
    if 'UVMap' not in me.uv_layers:
        me.uv_layers.new(name='UVMap')
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(angle), island_margin=0.0, area_weight=0.0, correct_aspect=True,
                             scale_to_bounds=False)
    bpy.ops.object.mode_set(mode='OBJECT')


# --------------------------------------------------------------------------------------------- a flared cuff
class Flare:
    """A glove's flared cuff round the wrist (hand frame, cm), FPArms.py's gauntlet made small and plain: its narrow end
    at x0 over the wrist, opening h cm up the forearm to its top (higher over the back of the hand by top_back), its
    half sizes across the thumb side (ry) and the back (rz) growing from the narrow end's to the top's along a curve
    of power; t the leather's thickness."""

    def __init__(self, x0, top, top_back, narrow, wide, zc=(-0.25, -0.15), power=1.4, t=0.22, segs=24):
        self.x0, self.top0, self.top_back = x0, top, top_back
        self.narrow, self.wide, self.zc, self.power, self.t, self.segs = narrow, wide, zc, power, t, segs

    def top(self, theta):
        return self.top0 + self.top_back * np.cos(theta)

    def radii(self, h):
        f = np.clip(np.asarray(h, dtype=np.float64) / self.top0, 0.0, 1.15) ** self.power
        ry = self.narrow[0] + (self.wide[0] - self.narrow[0]) * f
        rz = self.narrow[1] + (self.wide[1] - self.narrow[1]) * f
        zc = self.zc[0] + (self.zc[1] - self.zc[0]) * np.clip(np.asarray(h) / self.top0, 0.0, 1.0)
        return ry, rz, zc

    def surface(self, theta, h, inset=0.0):
        ry, rz, zc = self.radii(h)
        wob = 0.05 * np.sin(3.0 * theta + 1.3) * np.clip(h / self.top0, 0, 1)
        ry, rz = ry + wob - inset, rz + wob - inset
        y, z = ry * np.sin(theta), zc + rz * np.cos(theta)
        x = self.x0 - h
        dr = (self.wide[0] - self.narrow[0]) / self.top0
        n = np.stack([np.full_like(theta, dr), np.sin(theta) / ry * np.hypot(ry, rz) / math.sqrt(2),
                      np.cos(theta) / rz * np.hypot(ry, rz) / math.sqrt(2)], axis=-1)
        return np.stack([x, y, z], axis=-1), unit(n)

    def mesh(self, name='GloveCuff_r'):
        """Rows round its profile: its inside under the narrow end, the narrow end's rolled edge, the outside up, the
        top's rolled edge and a little of its inside. Returns the object (UVs unrolled) and per vertex (part: 1 outside,
        2 inside; theta; h)."""
        segs, t = self.segs, self.t
        theta = np.linspace(-math.pi, math.pi, segs + 1)
        top = self.top(theta)
        rows = [(np.full_like(theta, 0.9), t, 2), (np.full_like(theta, 0.05), t * 0.8, 1),
                (np.full_like(theta, 0.0), t * 0.3, 1)]
        for u in (0.0, 0.12, 0.27, 0.43, 0.6, 0.76, 0.9, 0.98):
            rows.append((top * u, 0.0, 1))
        rows += [(top + 0.05, t * 0.3, 1), (top + 0.05, t * 0.8, 1), (top - 0.9, t, 2), (top - 2.4, t, 2)]
        verts, uvs, part, th, hh = [], [], [], [], []
        arc, prev = 0.0, None
        for h, inset, kind in rows:
            p, _ = self.surface(theta, h, inset)
            if prev is not None:
                arc += float(np.mean(np.linalg.norm(p - prev, axis=1)))
            prev = p
            ry, rz, _ = self.radii(np.mean(h))
            r = 0.5 * (ry + rz) - inset
            verts.append(p)
            uvs.append(np.stack([theta * r, np.full_like(theta, arc)], axis=1))
            part.append(np.full(len(theta), float(kind)))
            th.append(theta)
            hh.append(np.asarray(h, dtype=np.float64) * np.ones_like(theta))
        nr = len(rows)
        faces = []
        for i in range(nr - 1):
            for j in range(segs):
                a, b = i * (segs + 1) + j, i * (segs + 1) + j + 1
                faces.append([a, b, b + segs + 1, a + segs + 1])
        V, UV = np.vstack(verts) * C, np.vstack(uvs) * C
        part = np.concatenate(part)
        axis = np.stack([np.linspace(self.x0 - self.top0 - 3.0, self.x0 + 3.0, 24), np.zeros(24),
                         np.full(24, 0.5 * (self.zc[0] + self.zc[1]))], 1) * C
        obj = mesh_object(name, V, faces, UV, axis=axis, outer=part == 1)
        return obj, dict(part=part, theta=np.concatenate(th), h=np.concatenate(hh), co=V)


def mesh_object(name, V, faces, UV=None, smooth=True, axis=None, outer=None, closed=False):
    """A mesh object from vertices (m), quads or triangles and per-vertex UVs (or none). Its faces are turned outward:
    a closed one by Blender's own reckoning; a sheet round an axis (points along it, m) so that its outer faces (those
    whose vertices are all `outer`) face away from the axis (one sheet folded over itself is wound one way throughout,
    so its inside then faces in)."""
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.new('UVMap')
    made = [bm.verts.new(tuple(p)) for p in V]
    bm.verts.ensure_lookup_table()
    for f in faces:
        try:
            face = bm.faces.new([made[i] for i in f])
        except ValueError:
            continue
        if UV is not None:
            for loop, i in zip(face.loops, f):
                loop[uv_layer].uv = (float(UV[i][0]), float(UV[i][1]))
    if closed:
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    if axis is not None:
        axis = np.asarray(axis, dtype=np.float64)
        outer = np.ones(len(V), bool) if outer is None else np.asarray(outer, bool)
        score = 0.0
        for p in me.polygons:
            if not all(outer[i] for i in p.vertices):
                continue
            c = np.array(p.center[:])
            q = axis[int(np.argmin(np.linalg.norm(axis - c, axis=1)))]
            score += float(np.array(p.normal[:]) @ unit(c - q))
        if score < 0.0:
            me.flip_normals()
    if smooth:
        me.shade_smooth()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    return obj


# ------------------------------------------------------------------------------------------------------ weights
def hand_weights(P, spec, cuff=None):
    """Bone weights (no side) for the hand's vertices (hand frame, m): each chain by its joints, blended across the
    webs; the wrist and a bare forearm on the forearm. FPArms.py's hand_weights on this hand's chains."""
    n = len(P)
    W = {}

    def add(bone, w):
        W[bone] = W.get(bone, np.zeros(n)) + w

    fields, chainw = [], []
    for ch in spec.chains:
        d, s, _, rad, _ = chain_field(P, ch)
        cw = {}
        marks = ch.cum[1:-1]
        ws = [smoothstep(sj - 0.85 * ch.radii[k + 1], sj + 0.85 * ch.radii[k + 1], s) for k, sj in enumerate(marks)]
        if ch.name == 'thumb':
            cw[ch.bones[0]] = 1.0 - ws[0]
            cw[ch.bones[1]] = ws[0] * (1.0 - ws[1])
            cw[ch.bones[2]] = ws[1]
            into = smoothstep(0.004, 0.022, s)
            cw = {k: v * into for k, v in cw.items()}
            cw['hand'] = 1.0 - into
        else:
            cw[ch.bones[0]] = 1.0 - ws[0]
            cw[ch.bones[1]] = ws[0] * (1.0 - ws[1])
            cw[ch.bones[2]] = ws[1] * (1.0 - ws[2])
            cw[ch.bones[3]] = ws[2]
            back = smoothstep(-0.05, -0.012, s)
            cw = {k: v * (back if k == ch.bones[0] else 1.0) for k, v in cw.items()}
            cw['hand'] = cw.get('hand', 0) + (1.0 - back) * (1.0 - ws[0])
        fields.append(d)
        chainw.append(cw)
    D = np.stack(fields, axis=1)
    Aw = 1.0 / (np.maximum(D - D.min(axis=1, keepdims=True), 0.0) + 0.0025) ** 4
    Aw /= Aw.sum(axis=1, keepdims=True)
    for c, cw in enumerate(chainw):
        for bone, w in cw.items():
            add(bone, Aw[:, c] * w)
    x = P[:, 0] / C
    fore = 1.0 - smoothstep(-3.5, -0.8, x)
    for bone in list(W):
        W[bone] = W[bone] * (1.0 - fore)
    add('lowerarm', fore)
    return W


def cuff_weights(h, part):
    """A glove's cuff rides the forearm, its narrow end a third on the hand (FPArms.py's gauntlet)."""
    on_hand = (1.0 - smoothstep(0.5, 3.0, h)) * 0.33
    return {'hand': on_hand, 'lowerarm': 1.0 - on_hand}


def set_weights(obj, weights, side='r'):
    A['set_weights'](obj, weights, side)


# ------------------------------------------------------------------------------------------ the detail fields
def hand_fields(P, N, spec, glove=True, nails=False):
    """Per-texel detail of the hand (P, N: hand frame, m) for the overlays: height (mm), cavity, scuff, polish (0..1),
    thread cover and its height, ridges (rubbed crests), and for a bare hand the nails (nail: 0..1 plate, its lunula,
    its free edge, the folds round it), knuckles (the skin over them) and creases' depth. FPArms.py's paint_glove's
    fields on this hand: a glove's seams up every finger and round its tip, the keystone thumb, three stitched points
    on the back; joint creases (deep on the palm side, wrinkles bunched over the back), palm lines."""
    n = len(P)
    chs = spec.chains
    out = {k: np.zeros(n) for k in ('height', 'cavity', 'scuff', 'polish', 'thread', 'thread_h', 'ridges', 'nail',
                                    'lunula', 'free', 'fold', 'knuckle', 'crease')}

    def seam(across, along, valid, rows=(-1.5, 1.5), groove=True, pitch=3.0, length=2.1, width=0.62):
        valid = valid & (np.abs(across) < 6.0)
        if not valid.any():
            return
        for off in rows:
            c, hgt, hole = stitches(across, along, off, pitch=pitch, length=length, width=width)
            out['thread'] = np.where(valid, np.maximum(out['thread'], c), out['thread'])
            out['thread_h'] = np.where(valid, np.maximum(out['thread_h'], hgt), out['thread_h'])
            out['cavity'] = np.where(valid, np.maximum(out['cavity'], hole * 0.6), out['cavity'])
            out['height'] = np.where(valid, out['height'] - hole * 0.1, out['height'])
        if groove:
            g = np.exp(-(across / 0.5) ** 2)
            out['height'] = np.where(valid, out['height'] - 0.3 * g + 0.14 * np.exp(-((np.abs(across) - 1.1) / 0.8) ** 2),
                                     out['height'])
            out['cavity'] = np.where(valid, np.maximum(out['cavity'], 0.6 * g), out['cavity'])

    def crease(d, width, depth, where, dark=0.45):
        valley = np.exp(-(d / width) ** 2)
        ridge = np.exp(-((d - 1.4 * width) / (0.9 * width)) ** 2)
        out['height'] = out['height'] + where * depth * (0.45 * ridge - valley)
        out['cavity'] = np.maximum(out['cavity'], where * valley * dark)
        out['ridges'] = np.maximum(out['ridges'], where * ridge * min(1.0, depth * 3.0))
        out['crease'] = np.maximum(out['crease'], where * valley * min(1.0, depth * 3.0))

    breakup = 0.5 + 0.5 * np.clip(A['fbm3'](P, 0.008, 23, 2) * 2.2 + 0.3, -1, 1)
    nearest = np.full(n, -1)
    best = np.full(n, np.inf)
    fields = [chain_field(P, ch) for ch in chs]
    for c, f in enumerate(fields):
        closer = f[0] < best
        best = np.where(closer, f[0], best)
        nearest = np.where(closer, c, nearest)
    body = body_sdf(P, spec)
    on_body = body < best + 0.0008
    for c, ch in enumerate(chs):
        d, s, phi, rad, segk = fields[c]
        mine = (nearest == c) & ~on_body
        s_mm, r_mm = s * 1000.0, rad * 1000.0
        tip = ch.cum[-1] * 1000.0
        if ch.name != 'thumb':
            if glove:
                on = mine & (s > -0.002)
                for sign in (1.0, -1.0):
                    seam(r_mm * (sign * phi - math.pi / 2), s_mm, on & (sign * phi > 0.3))
            palmar = np.clip((np.abs(phi) - 1.9) / 0.6, 0.0, 1.0)
            dorsal = np.clip((1.3 - np.abs(phi)) / 0.6, 0.0, 1.0)
            for k, sj in enumerate(ch.cum[1:-1]):
                sj_mm = sj * 1000.0 + (2.5 if k == 0 else 0.0)
                for off, depth in ((-1.0, 0.3), (1.3, 0.2)):
                    crease(s_mm - sj_mm - off - 0.0003 * (phi * r_mm) ** 2, 0.6 if glove else 0.45, depth,
                           mine * palmar, dark=0.6)
                if k >= 1:
                    count = 3 if glove else 5
                    for w in range(count):
                        off = (w - (count - 1) / 2.0) * (2.0 if glove else 1.3) + 0.4 * math.sin(k * 3 + w)
                        bendc = 0.0015 * (phi * r_mm) ** 2
                        crease(s_mm - sj_mm - off + bendc, 0.6 if glove else 0.32,
                               (0.1 if glove else 0.13) * (1.0 - 0.6 * abs(w - (count - 1) / 2.0) / count),
                               mine * dorsal * breakup, dark=0.18 if glove else 0.3)
                    out['scuff'] = np.maximum(out['scuff'], mine * dorsal * np.exp(-((s_mm - sj_mm) / 6.0) ** 2) * 0.55)
                    out['knuckle'] = np.maximum(out['knuckle'], mine * dorsal * np.exp(-((s_mm - sj_mm) / 4.5) ** 2))
            out['scuff'] = np.maximum(out['scuff'], mine * smoothstep(tip - 9.0, tip + 3.0, s_mm) * 0.75)
            out['polish'] = np.maximum(out['polish'], mine * palmar * 0.65)
        else:
            if glove:
                on = mine & (s > 0.012)
                for sign in (1.0, -1.0):
                    seam(r_mm * (sign * phi - math.pi / 2), s_mm, on & (sign * phi > 0.3))
                ring = (s_mm - 33.0 - 6.0 * np.cos(phi + 0.6))
                seam(ring, phi * r_mm, (nearest == c) & (d < 0.004), rows=(-1.6, 1.6))
            for sj in ch.cum[1:-1]:
                crease(s_mm - sj * 1000.0, 0.6 if glove else 0.45, 0.2, mine * np.clip((np.abs(phi) - 1.6) / 0.6, 0, 1),
                       dark=0.5)
            if not glove:
                sj = ch.cum[2] * 1000.0
                for w in range(4):
                    crease(s_mm - sj - (w - 1.5) * 1.4, 0.32, 0.11, mine * np.clip((1.2 - np.abs(phi)) / 0.6, 0, 1)
                           * breakup, dark=0.3)
            out['scuff'] = np.maximum(out['scuff'], mine * smoothstep(tip - 9.0, tip + 3.0, s_mm) * 0.6)
            out['polish'] = np.maximum(out['polish'], mine * np.clip((np.abs(phi) - 1.4) / 0.8, 0, 1) * 0.6)
        if nails:
            # the nail: on the last bone's back, from its fold (cuticle) to its free edge just past the tip's start
            seg_len = ch.lengths[-1] * 1000.0
            length = min(seg_len * (0.62 if ch.name != 'thumb' else 0.58), 15.5)
            end = tip + 0.8
            a = (s_mm - (end - length)) / length                    # 0 at the fold, 1 at the free edge
            half = ch.radii[-1] * 1000.0 * (0.78 if ch.name != 'thumb' else 0.82)
            b = (phi * r_mm) / half                                 # -1..1 across
            on_seg = (nearest == c) & (segk == len(ch.lengths) - 1) & (np.abs(phi) < 1.5)
            # a rounded rectangle: its fold end a U (radius cap of its length), the free edge a gentle curve
            cap = 0.3
            w = np.where(a < cap, np.sqrt(np.clip(1.0 - ((cap - a) / cap) ** 2, 0.0, 1.0)), 1.0)
            shape = np.abs(b) / np.maximum(w, 1e-3)
            a_end = 1.0 - 0.05 * b * b
            inside = on_seg & (a >= 0.0) & (a <= a_end) & (shape < 1.0)
            edge = np.clip((1.0 - shape) / 0.1, 0.0, 1.0) * (1.0 - smoothstep(a_end - 0.04, a_end, a))
            plate = np.where(inside, edge, 0.0)
            out['nail'] = np.maximum(out['nail'], plate)
            out['lunula'] = np.maximum(out['lunula'], plate * (1.0 - smoothstep(0.16, 0.26, a + 0.1 * b * b)))
            out['free'] = np.maximum(out['free'], plate * smoothstep(a_end - 0.16, a_end - 0.1, a))
            # the folds round it: a soft groove at its sides and fold
            ring = on_seg & (a > -0.2) & (a < 0.92) & (np.abs(shape - 1.0) < 0.4)
            fold = np.where(ring, np.exp(-((shape - 1.0) / 0.13) ** 2), 0.0)
            back_fold = on_seg & (np.abs(b) < 1.0) & (a > -0.2) & (a < 0.05)
            fold = np.maximum(fold, np.where(back_fold, np.exp(-(a / 0.06) ** 2) * (1.0 - np.abs(b) ** 4), 0.0))
            out['fold'] = np.maximum(out['fold'], fold)
            out['height'] = out['height'] + plate * 0.12 - fold * 0.12
            out['cavity'] = np.maximum(out['cavity'], fold * 0.45)
    x, y, z = P[:, 0] / C, P[:, 1] / C, P[:, 2] / C
    back = on_body & (N[:, 2] > 0.25)
    palm = on_body & (N[:, 2] < -0.35)
    if glove:
        # three raised, stitched points up the back between the metacarpals
        for (y1, y0) in ((2.25, 1.35), (0.0, 0.1), (-2.1, -1.2)):
            yl = y0 + (y1 - y0) * smoothstep(1.2, 6.4, x)
            across = (y - yl) * 10.0
            on = back & (x > 1.3) & (x < 6.6)
            fade = smoothstep(1.3, 2.2, x) * (1.0 - smoothstep(5.6, 6.5, x))
            cvr, hgt, hole = stitches(across, x * 10.0, 0.0, pitch=2.5, length=1.7, width=0.55, slant=0.3)
            out['thread'] = np.where(on, np.maximum(out['thread'], cvr * fade), out['thread'])
            out['thread_h'] = np.where(on, np.maximum(out['thread_h'], hgt * fade), out['thread_h'])
            out['cavity'] = np.where(on, np.maximum(out['cavity'], hole * fade * 0.5), out['cavity'])
            cord = np.exp(-(across / 1.4) ** 2) * fade
            channel = np.exp(-((np.abs(across) - 2.0) / 0.5) ** 2) * fade
            out['height'] = out['height'] + on * (cord * 0.22 - channel * 0.1)
            out['cavity'] = np.maximum(out['cavity'], on * channel * 0.3)
        edge = on_body & (y < -2.2) & (N[:, 1] < -0.45) & (x < 8.4) & (x > 0.6)
        seam(N[:, 2] * 11.0, x * 10.0, edge)
    # palm lines, the grip's polish; the back's wrinkles behind the knuckles and at the wrist
    for (x0, y0, x1, y1, depth) in ((7.4, -3.8, 8.3, 2.2, 0.32), (5.6, -3.6, 7.0, 3.4, 0.28), (2.0, 1.4, 6.6, 2.9, 0.26)):
        t = np.clip(((x - x0) * (x1 - x0) + (y - y0) * (y1 - y0)) / ((x1 - x0) ** 2 + (y1 - y0) ** 2), 0, 1)
        dist = np.hypot(x - (x0 + t * (x1 - x0)), y - (y0 + t * (y1 - y0))) * 10.0
        crease(dist, 0.8 if glove else 0.55, depth, palm * np.sin(math.pi * np.clip(t, 0, 1)) ** 0.5, dark=0.55)
    if not glove:
        # the wrist's bracelets across the inside of the wrist
        for k in range(3):
            xk = -0.4 - k * 0.55 + 0.1 * np.sin(y * 1.9 + k)
            crease((x - xk) * 10.0, 0.4, 0.14, on_body * (N[:, 2] < 0.2) * (np.abs(y) < 3.0), dark=0.35)
    out['polish'] = np.maximum(out['polish'], palm * 0.75)
    for k in range(2):
        xk = 7.1 - k * 0.55 + 0.1 * np.sin(y * 1.7 + k)
        fade = smoothstep(-3.5, -2.3, y) * (1.0 - smoothstep(2.8, 4.0, y))
        crease((x - xk) * 10.0, 0.75 if glove else 0.4, 0.16, back * fade * breakup, dark=0.3)
    for k in range(3):
        xk = 0.9 + k * 0.5 + 0.08 * np.sin(y * 2.3 + k * 2)
        crease((x - xk) * 10.0, 0.85 if glove else 0.45, 0.2 * (1.0 - k / 4), on_body * (N[:, 2] > -0.2) * breakup,
               dark=0.32)
    knuck = np.zeros(n)
    for ch in chs[:4]:
        kp = ch.joints[1] / C + v3(0, 0, 1.0)
        knuck = np.maximum(knuck, np.exp(-np.sum((P / C - kp) ** 2, axis=1) / 1.1 ** 2))
    out['scuff'] = np.maximum(out['scuff'], knuck * 0.85)
    out['knuckle'] = np.maximum(out['knuckle'], knuck)
    out['on_body'] = on_body.astype(np.float64)
    out['back'] = np.clip(N[:, 2], 0.0, 1.0)
    return out
