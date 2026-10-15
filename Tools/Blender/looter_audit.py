"""Auditing a rigged character made by a scripted model (Ellis.py --audit): poses for the UE5 mannequin's bones, the
game's swing springs at rest in a pose (as Player/Animation/BodySwing.cpp settles them), the armature's blend skinning
done here on any part, checks of one layer poking through another, and render setups (Cycles daylight and dusk with
the baked look, Workbench clay).

    looter_audit.pose(arm, POSES['aim'])                   # the mannequin's bones turned (world space, in order)
    looter_audit.settle_swing(arm, chains)                   # the swing chains where the springs would rest them
    mats = looter_audit.skinning(arm, names)                 # each bone's rest-to-posed matrix
    v = looter_audit.deform(co, W, mats)                     # a part's points posed
    looter_audit.poke_through(inner, outer)                  # inner's points out through outer
"""
import math

import bpy
import numpy as np
from mathutils import Matrix, Quaternion, Vector
from mathutils.bvhtree import BVHTree

# ------------------------------------------------------------------------------------------------------------ poses
# Steps applied in order on the posed skeleton (Blender's axes, the figure built facing -Y, its left +X):
# ('turn', bone, world axis, degrees) about the bone's head; ('point', bone, world direction); ('move', bone, offset m).
FWD, BACK, LEFT, UP = (0.0, -1.0, 0.0), (0.0, 1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0)
X, Y, Z = (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)


def _arms_down(off=10.0):
    """The game's idle arms: hanging about off degrees from the body, the elbows a little bent."""
    s, c = math.sin(math.radians(off)), math.cos(math.radians(off))
    return [('point', 'upperarm_l', (s, -0.04, -c)), ('point', 'lowerarm_l', (s * 0.6, -0.22, -0.97)),
            ('point', 'upperarm_r', (-s, -0.04, -c)), ('point', 'lowerarm_r', (-s * 0.6, -0.22, -0.97))]


POSES = {
    'rest': [],
    'idle': _arms_down(),
    # a rifle to the right shoulder: the right elbow out, its hand back at the trigger; the left arm out under the
    # fore-end; the head down onto the stock (rolled right, pitched a little)
    'aim': [('turn', 'spine_03', Z, -10.0),
            ('point', 'upperarm_r', (-0.62, -0.45, -0.25)), ('point', 'lowerarm_r', (0.55, -0.75, 0.25)),
            ('point', 'upperarm_l', (0.25, -0.8, -0.45)), ('point', 'lowerarm_l', (-0.3, -0.95, 0.1)),
            ('turn', 'head', Y, -20.0), ('turn', 'head', X, 10.0)],
    # mid-stride: the left thigh 35 degrees forward, its knee a little bent, the right 20 back
    'stride': _arms_down(14.0) + [('turn', 'thigh_l', X, -35.0), ('turn', 'calf_l', X, 25.0),
                                  ('turn', 'foot_l', X, -10.0), ('turn', 'thigh_r', X, 20.0),
                                  ('turn', 'calf_r', X, 15.0), ('turn', 'foot_r', X, -20.0)],
    # crouched: the hips down 32 cm, the thighs near level, the knees at 90 degrees, the back leaning in
    'crouch': [('move', 'pelvis', (0.0, 0.06, -0.32)), ('turn', 'spine_01', X, 22.0)] + _arms_down(18.0) +
              [('turn', 'thigh_l', X, -80.0), ('turn', 'calf_l', X, 90.0), ('turn', 'foot_l', X, -10.0),
               ('turn', 'thigh_r', X, -75.0), ('turn', 'calf_r', X, 90.0), ('turn', 'foot_r', X, -15.0)],
}


def reset(arm):
    for pb in arm.pose.bones:
        pb.rotation_mode = 'QUATERNION'
        pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()


def _apply(pb, matrix):
    pb.matrix = matrix @ pb.matrix
    bpy.context.view_layer.update()


def pose(arm, steps, amount=1.0):
    """The skeleton from its rest in steps (POSES), each on the bones as already posed; amount < 1 goes that share of
    the way (turns and moves scaled, a pointed bone turned that share toward its direction)."""
    reset(arm)
    for kind, bone, *args in steps:
        pb = arm.pose.bones[bone]
        h = pb.head.copy()
        if kind == 'turn':
            r = Matrix.Rotation(math.radians(args[1] * amount), 4, Vector(args[0]).normalized())
            _apply(pb, Matrix.Translation(h) @ r @ Matrix.Translation(-h))
        elif kind == 'point':
            q = (pb.tail - pb.head).normalized().rotation_difference(Vector(args[0]).normalized())
            q = Quaternion().slerp(q, amount)
            _apply(pb, Matrix.Translation(h) @ q.to_matrix().to_4x4() @ Matrix.Translation(-h))
        elif kind == 'move':
            _apply(pb, Matrix.Translation(Vector(args[0]) * amount))


# ------------------------------------------------------------------------------------------------- the swing springs
# Player/Animation/BodySwing.cpp's numbers (keep in step): the limbs a chain point keeps clear of (cm: the most it
# keeps), the share of its rest distance it keeps, each chain kind's stiffness and gravity share, and how much of the
# upper arm's movement the cape's corners take.
LIMBS = (('thigh_l', 'calf_l', 14.0), ('calf_l', 'foot_l', 10.0), ('thigh_r', 'calf_r', 14.0), ('calf_r', 'foot_r', 10.0),
         ('pelvis', 'spine_01', 18.0), ('spine_01', 'spine_03', 18.0), ('spine_03', 'spine_05', 18.0),
         ('spine_05', 'neck_01', 15.0), ('neck_01', 'head', 9.0), ('upperarm_l', 'lowerarm_l', 9.0),
         ('upperarm_r', 'lowerarm_r', 9.0), ('lowerarm_l', 'hand_l', 7.0), ('lowerarm_r', 'hand_r', 7.0),
         ('hand_l', 'middle_01_l', 5.0), ('hand_r', 'middle_01_r', 5.0))
CLEARANCE_SHARE = 0.95
LONE_BONE = 0.18                     # a one-bone chain's length (the lantern's)
MIN_BONE_ALONG = 0.2                 # a bone's first fifth isn't pushed off a limb (its head is held by the one before)
FLOOR_CLEARANCE = 0.02               # no point under the ground at his feet (the armature's origin) + this
SPRINGS = {'skirt': (7.0, 0.08), 'cape': (12.0, 0.05), 'bandana': (9.0, 0.05), 'hair': (9.0, 0.05), 'lantern': (2.0, 1.0)}
CAPE_FOLLOW = {'f': 0.6, 'b': 0.4}


def _closest(p, a, b):
    ab = b - a
    t = min(1.0, max(0.0, (p - a).dot(ab) / max(ab.length_squared, 1e-12)))
    return a + ab * t


def _segments(p1, q1, p2, q2):
    """The closest points of two segments (FMath::SegmentDistToSegmentSafe), on the first and on the second."""
    d1, d2, r = q1 - p1, q2 - p2, p1 - p2
    a, e, f = d1.dot(d1), d2.dot(d2), d2.dot(r)
    if a <= 1e-12 and e <= 1e-12:
        return p1.copy(), p2.copy()
    if a <= 1e-12:
        s, t = 0.0, min(1.0, max(0.0, f / e))
    else:
        c = d1.dot(r)
        if e <= 1e-12:
            s, t = min(1.0, max(0.0, -c / a)), 0.0
        else:
            b = d1.dot(d2)
            den = a * e - b * b
            s = min(1.0, max(0.0, (b * f - c * e) / den)) if den > 1e-12 else 0.0
            t = (b * s + f) / e
            if t < 0.0:
                s, t = min(1.0, max(0.0, -c / a)), 0.0
            elif t > 1.0:
                s, t = min(1.0, max(0.0, (b - c) / a)), 1.0
    return p1 + d1 * s, p2 + d2 * t


def _push_bone_out(head, tip, a, c, r):
    """BodySwing's PushBoneOut: the tip turned about head so the bone keeps r from the limb a..c (not by its first
    MIN_BONE_ALONG), the tip moving the overlap over how far down the bone the nearest place is."""
    if r <= 0.0:
        return tip
    on_bone, on_limb = _segments(head, tip, a, c)
    away = on_bone - on_limb
    dist, length = away.length, (tip - head).length
    if dist >= r or length <= 1e-9:
        return tip
    along = (on_bone - head).length / length
    if along < MIN_BONE_ALONG:
        return tip
    if dist > 1e-9:
        out = away / dist
    else:
        axis = (c - a).normalized()
        v = tip - on_limb
        v = v - axis * v.dot(axis)
        out = v.normalized() if v.length > 1e-9 else Vector(FWD)
    return tip + out * ((r - dist) / along)


def _keep_above(head, tip, length, floor, lean):
    """BodySwing's KeepAbove: a tip under the floor swung up about head onto it, out the way it leans (lean's, the rest
    direction, when it hangs straight down)."""
    if tip.z >= floor:
        return tip
    flat = Vector((tip.x - head.x, tip.y - head.y, 0.0))
    if flat.length <= 1e-9:
        flat = Vector((lean.x, lean.y, 0.0))
    if flat.length <= 1e-9:
        flat = Vector(FWD)
    drop = min(max(head.z - floor, 0.0), length)
    return head + flat.normalized() * math.sqrt(max(length * length - drop * drop, 0.0)) - Vector((0.0, 0.0, drop))


def _rest_tips(arm, names):
    """A chain's bone tips as the game takes them (BodySwing's Bind: an FBX skeleton has no tails): the next bone's
    head, the last bone's head plus the bone before it, a lone bone's LONE_BONE straight down."""
    heads = [arm.data.bones[n].head_local.copy() for n in names]
    tips = []
    for i, h in enumerate(heads):
        if i + 1 < len(heads):
            tips.append(heads[i + 1].copy())
        elif i > 0:
            tips.append(h + (h - heads[i - 1]))
        else:
            tips.append(h + Vector((0.0, 0.0, -LONE_BONE)))
    return tips


def settle_swing(arm, chain_bones, steps=None, stages=12, reach=0.01):
    """Turns the swing bones where the game's springs would rest them in a pose (BodySwing's steady state: each point
    at its animated place, the cape's corners partly riding the upper arms, sagging by its gravity share over its
    stiffness squared, then each its length from the one before and pushed out of the limbs it kept clear of at
    rest, its bone too (_push_bone_out), and kept over the floor (_keep_above); the points the game's, _rest_tips). chain_bones: {chain: [bone names in order]}. With steps (the pose's POSES
    entry) the pose is reached in stages from rest, each point moving at most reach (m) a pass toward its place and
    kept out of the limbs on the way, as the springs follow a move frame by frame (set straight into a pose, a limb
    already past a point would push it out the far side). Without steps, the pose as it is, in one go."""
    bones = arm.data.bones
    rest_limbs = [(bones[a].head_local, bones[b].head_local, m / 100.0) for a, b, m in LIMBS]
    tips = {chain: _rest_tips(arm, names) for chain, names in chain_bones.items()}
    clear = {chain: [[min((t - _closest(t, a, c)).length * CLEARANCE_SHARE, m) for a, c, m in rest_limbs]
                     for t in tips[chain]] for chain in chain_bones}
    # and each bone, head to tip, its rest distance from each limb (segment to segment), as BodySwing's Bind
    gap = lambda q: (q[0] - q[1]).length
    bone_clear = {chain: [[min(gap(_segments(bones[n].head_local, t, a, c)) * CLEARANCE_SHARE, m) for a, c, m in rest_limbs]
                          for n, t in zip(names, tips[chain])] for chain, names in chain_bones.items()}
    floor = arm.matrix_world.translation.z + FLOOR_CLEARANCE
    to_pose = lambda pbs, n: pbs[n].matrix @ bones[n].matrix_local.inverted()
    held = {}
    for stage in range(1, stages + 1) if steps is not None else (None,):
        if stage is not None:
            pose(arm, steps, stage / stages)
        bpy.context.view_layer.update()
        pbs = arm.pose.bones
        now_limbs = [(pbs[a].head.copy(), pbs[b].head.copy()) for a, b, _m in LIMBS]
        for chain, names in chain_bones.items():
            k, g = SPRINGS[chain.split('_')[0]]
            follow = None
            if chain.startswith('cape_'):
                arm_bone = 'upperarm_l' if chain[6] == 'l' else 'upperarm_r'
                follow = (to_pose(pbs, arm_bone), CAPE_FOLLOW[chain[5]])
            targets, lengths = [], []
            for name, tip_rest in zip(names, tips[chain]):
                tip = to_pose(pbs, name) @ tip_rest
                if follow is not None:
                    tip = tip.lerp(follow[0] @ tip_rest, follow[1])
                targets.append(tip + Vector((0.0, 0.0, -9.8 * g / (k * k))))
                lengths.append((tip_rest - bones[name].head_local).length)
            anchor = pbs[names[0]].head.copy()
            stepping = chain in held
            points = held[chain] if stepping else [t.copy() for t in targets]
            for _ in range(4 if stage is None or stage < stages else 40):
                head = anchor
                for i, p in enumerate(points):
                    if stepping:
                        d = targets[i] - p
                        p = p + d * min(1.0, reach / max(d.length, 1e-9))
                    rest_dir = (targets[i] - head).normalized()
                    p = head + ((p - head).normalized() if (p - head).length > 1e-9 else rest_dir) * lengths[i]
                    for (a, c), r, rb in zip(now_limbs, clear[chain][i], bone_clear[chain][i]):
                        q = _closest(p, a, c)
                        if (p - q).length < r:
                            p = q + ((p - q).normalized() if (p - q).length > 1e-9 else Vector((0, -1, 0))) * r
                        p = _push_bone_out(head, p, a, c, rb)
                    p = head + ((p - head).normalized() if (p - head).length > 1e-9 else rest_dir) * lengths[i]
                    p = _keep_above(head, p, lengths[i], floor, rest_dir)
                    points[i] = p
                    head = p
            held[chain] = points
    pbs = arm.pose.bones
    for chain, names in chain_bones.items():
        for name, tip_rest, p in zip(names, tips[chain], held[chain]):
            pb = pbs[name]
            h = pb.head.copy()
            q = (to_pose(pbs, name) @ tip_rest - h).normalized().rotation_difference((p - h).normalized())
            _apply(pb, Matrix.Translation(h) @ q.to_matrix().to_4x4() @ Matrix.Translation(-h))


# --------------------------------------------------------------------------------------------- skinning and checks

def skinning(arm, names):
    """(bones, 4, 4): each named bone's rest-to-posed matrix (the armature at the origin)."""
    bpy.context.view_layer.update()
    return np.array([np.array(arm.pose.bones[n].matrix @ arm.data.bones[n].matrix_local.inverted()) for n in names])


def deform(co, W, mats):
    """A part's points posed by blend skinning (the armature modifier's linear blend)."""
    out = np.zeros_like(co)
    for j in np.nonzero(W.max(0) > 1e-6)[0]:
        out += W[:, j:j + 1] * (co @ mats[j, :3, :3].T + mats[j, :3, 3])
    return out


def normals(co, faces):
    n = np.zeros_like(co)
    for f in faces:
        p = co[f]
        fn = np.cross(p[1] - p[0], p[-1] - p[0])
        n[f] += fn
    return n / np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)


def tree(co, faces):
    return BVHTree.FromPolygons([tuple(map(float, p)) for p in co], faces)


def away_from_bones(points, segments):
    """Per point, the direction from the nearest bone (segments: (heads, tails)) out to it: which way is out."""
    heads, tails = segments
    ab = tails - heads
    t = np.clip(np.einsum('pkd,kd->pk', points[:, None, :] - heads[None], ab) / np.maximum((ab * ab).sum(1), 1e-12),
                0.0, 1.0)
    closest = heads[None] + t[..., None] * ab[None]
    d = points[:, None, :] - closest
    k = np.argmin((d * d).sum(2), 1)
    out = d[np.arange(len(points)), k]
    return out / np.maximum(np.linalg.norm(out, axis=1, keepdims=True), 1e-12)


def poke_through(inner, outer, cover=0.06, depth=0.03, outward=None):
    """Points of an inner layer out through an outer one: each point looks out along its normal (the outer layer
    within cover: under it, fine) and back in (the outer layer within depth behind it: through it). inner and outer are
    (points, faces); outward (per point, away from the body) turns normals that face in (a thickened part's inner shell)
    out. Returns (indices, depths m)."""
    co, faces = inner
    nrm = normals(co, faces)
    if outward is not None:
        nrm = np.where(((nrm * outward).sum(1) < 0.0)[:, None], -nrm, nrm)
    t = tree(*outer)
    hits, deep = [], []
    for i, (p, n) in enumerate(zip(co, nrm)):
        o, d = Vector(tuple(p)), Vector(tuple(n))
        if t.ray_cast(o + d * 1e-4, d, cover)[0] is not None:
            continue
        back = t.ray_cast(o - d * 1e-4, -d, depth)
        if back[0] is not None:
            hits.append(i)
            deep.append(back[3])
    return np.array(hits, dtype=int), np.array(deep)


def inside(points, solid, reach=0.05):
    """Points of one part inside another closed-ish part (its normals outward). Returns (indices, depths m)."""
    t = tree(*solid)
    hits, deep = [], []
    for i, p in enumerate(points):
        loc, nrm, _i, dist = t.find_nearest(Vector(tuple(p)), reach)
        if loc is not None and (Vector(tuple(p)) - loc).dot(nrm) < 0.0:
            hits.append(i)
            deep.append(dist)
    return np.array(hits, dtype=int), np.array(deep)


# --------------------------------------------------------------------------------------------------------- renders

def light(kind='day'):
    """A sun ~40 degrees up and a sky (day), or a low warm sun under a darkening sky (dusk), on a grey ground."""
    sc = bpy.context.scene
    world = bpy.data.worlds.get('AuditSky') or bpy.data.worlds.new('AuditSky')
    sc.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes['Background']
    for ob in [o for o in bpy.data.objects if o.name.startswith('_AuditSun')]:
        bpy.data.objects.remove(ob)
    ld = bpy.data.lights.new('_AuditSun', 'SUN')
    sun = bpy.data.objects.new('_AuditSun', ld)
    sc.collection.objects.link(sun)
    if kind == 'dusk':
        bg.inputs['Color'].default_value = (0.20, 0.24, 0.40, 1.0)
        bg.inputs['Strength'].default_value = 0.45
        ld.energy, ld.color, ld.angle = 2.5, (1.0, 0.52, 0.28), math.radians(3)
        sun.rotation_euler = (math.radians(84), 0.0, math.radians(-60))
    else:
        bg.inputs['Color'].default_value = (0.50, 0.60, 0.78, 1.0)
        bg.inputs['Strength'].default_value = 0.9
        ld.energy, ld.color, ld.angle = 4.0, (1.0, 0.96, 0.9), math.radians(1.5)
        sun.rotation_euler = (math.radians(50), 0.0, math.radians(-35))
    if '_AuditGround' not in bpy.data.objects:
        me = bpy.data.meshes.new('_AuditGround')
        me.from_pydata([(-6, -6, 0), (6, -6, 0), (6, 6, 0), (-6, 6, 0)], [], [(0, 1, 2, 3)])
        g = bpy.data.objects.new('_AuditGround', me)
        sc.collection.objects.link(g)
        m = bpy.data.materials.new('_AuditGround')
        m.use_nodes = True
        m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.32, 0.30, 0.28, 1.0)
        m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = 0.9
        me.materials.append(m)


def cycles(samples=48):
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = samples
    sc.cycles.use_denoising = True
    sc.render.film_transparent = False
    try:
        sc.view_settings.view_transform = 'AgX'
    except Exception:
        pass


def clay():
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.display.render_aa = '8'
    sh = sc.display.shading
    sh.light, sh.color_type, sh.show_cavity, sh.cavity_type = 'STUDIO', 'OBJECT', True, 'WORLD'
    sh.show_specular_highlight = False


def camera():
    sc = bpy.context.scene
    cam = bpy.data.objects.get('_AuditCam')
    if cam is None:
        cam = bpy.data.objects.new('_AuditCam', bpy.data.cameras.new('_AuditCam'))
        sc.collection.objects.link(cam)
    sc.camera = cam
    return cam


def shoot(path, target, offset, lens=85.0, size=(1200, 1600)):
    """One render to path: the camera at target + offset, looking at target."""
    sc = bpy.context.scene
    cam = camera()
    cam.data.lens = lens
    cam.data.sensor_fit = 'AUTO'
    sc.render.resolution_x, sc.render.resolution_y = size
    sc.render.resolution_percentage = 100
    t = Vector(target)
    cam.location = t + Vector(offset)
    cam.rotation_euler = (t - cam.location).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = path
    bpy.ops.render.render(write_still=True)
    return path
