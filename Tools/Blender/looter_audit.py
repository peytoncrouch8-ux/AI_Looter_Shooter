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
# ('turn', bone, world axis, degrees) about the bone's head; ('point', bone, world direction); ('move', bone, offset m);
# ('tip', bone, degrees) about the bone's own X; ('twist', side, degrees, bend) the hand on the forearm.
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
    # the hands turned on the forearms as the game's guns turn them (measured in play, 2026-10-15: on a rifle's
    # foregrip or the revolver the left hand twists ~100 degrees about the forearm, the right ~32, each bent a little
    # more), the twist bones taking their shares; the arms up as in aim; a and b the two ways round
    'grip_a': [('turn', 'spine_03', Z, -10.0), ('point', 'upperarm_r', (-0.62, -0.45, -0.25)),
               ('point', 'lowerarm_r', (0.55, -0.75, 0.25)), ('point', 'upperarm_l', (0.25, -0.8, -0.45)),
               ('point', 'lowerarm_l', (-0.3, -0.95, 0.1)), ('twist', 'l', 100.0, 30.0), ('twist', 'r', 32.0, 20.0)],
    'grip_b': [('turn', 'spine_03', Z, -10.0), ('point', 'upperarm_r', (-0.62, -0.45, -0.25)),
               ('point', 'lowerarm_r', (0.55, -0.75, 0.25)), ('point', 'upperarm_l', (0.25, -0.8, -0.45)),
               ('point', 'lowerarm_l', (-0.3, -0.95, 0.1)), ('twist', 'l', -100.0, -30.0), ('twist', 'r', -32.0, -20.0)],
    # (the cape's audit, ellis_cape_audit.py, round 6) a gun carried low across the body: the upper arms a little
    # forward, the forearms level
    'carry': [('point', 'upperarm_r', (-0.25, -0.30, -0.92)), ('point', 'lowerarm_r', (0.45, -0.85, 0.10)),
              ('point', 'upperarm_l', (0.20, -0.40, -0.90)), ('point', 'lowerarm_l', (-0.45, -0.85, 0.25))],
    # aimed 35 degrees up: the chest pitched back, the arms higher on it (a world-level arm is raised on a leaning chest)
    'aim_high': [('turn', 'spine_03', Z, -10.0), ('turn', 'spine_03', X, -15.0),
                 ('point', 'upperarm_r', (-0.60, -0.50, 0.05)), ('point', 'lowerarm_r', (0.50, -0.70, 0.50)),
                 ('point', 'upperarm_l', (0.25, -0.85, -0.10)), ('point', 'lowerarm_l', (-0.30, -0.80, 0.50)),
                 ('turn', 'head', Y, -20.0), ('turn', 'head', X, -10.0), ('tip', 'hat', 18.0)],
    # aimed 35 degrees down: the chest pitched forward, the arms lower
    'aim_low': [('turn', 'spine_03', Z, -10.0), ('turn', 'spine_03', X, 15.0),
                ('point', 'upperarm_r', (-0.55, -0.35, -0.60)), ('point', 'lowerarm_r', (0.50, -0.60, -0.35)),
                ('point', 'upperarm_l', (0.20, -0.60, -0.75)), ('point', 'lowerarm_l', (-0.30, -0.85, -0.40)),
                ('turn', 'head', Y, -20.0), ('turn', 'head', X, 25.0), ('tip', 'hat', 18.0)],
    # crouched and aiming (the chest straighter than the plain crouch's)
    'crouch_aim': [('move', 'pelvis', (0.0, 0.06, -0.32)), ('turn', 'spine_01', X, 12.0), ('turn', 'spine_03', Z, -10.0),
                   ('turn', 'thigh_l', X, -80.0), ('turn', 'calf_l', X, 90.0), ('turn', 'foot_l', X, -10.0),
                   ('turn', 'thigh_r', X, -75.0), ('turn', 'calf_r', X, 90.0), ('turn', 'foot_r', X, -15.0),
                   ('point', 'upperarm_r', (-0.62, -0.45, -0.25)), ('point', 'lowerarm_r', (0.55, -0.75, 0.25)),
                   ('point', 'upperarm_l', (0.25, -0.8, -0.45)), ('point', 'lowerarm_l', (-0.3, -0.95, 0.1)),
                   ('turn', 'head', Y, -20.0), ('turn', 'head', X, 10.0), ('tip', 'hat', 18.0)],
    # a run's arm swing at its widest, the left arm forward and the right back (the game's cape takes about a third
    # of a run's swing, FollowEaseSeconds: this is the whole of it)
    'swing': [('point', 'upperarm_l', (0.12, -0.55, -0.83)), ('point', 'lowerarm_l', (0.10, -0.95, -0.20)),
              ('point', 'upperarm_r', (-0.12, 0.45, -0.88)), ('point', 'lowerarm_r', (-0.10, 0.10, -0.99))],
    # the head alone turned 35 degrees, pitched up 25 and down 25 (rig_check's HEAD_TESTS: the collar must clear)
    'look_l': _arms_down() + [('turn', 'head', Z, 35.0)],
    'look_r': _arms_down() + [('turn', 'head', Z, -35.0)],
    'look_up': _arms_down() + [('turn', 'head', X, -25.0)],
    'look_down': _arms_down() + [('turn', 'head', X, 25.0)],
    # the head pitched up 20 and turned 35 with the hat tipped 20 (HAT_TIP), the brim's back lowest over the collar
    'look_up_tip': _arms_down() + [('turn', 'head', X, -20.0), ('turn', 'head', Z, 35.0), ('tip', 'hat', 20.0)],
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
        pb = arm.pose.bones.get(bone)                  # (a twist step names a side, not a bone)
        h = pb.head.copy() if pb is not None else None
        if kind == 'turn':
            r = Matrix.Rotation(math.radians(args[1] * amount), 4, Vector(args[0]).normalized())
            _apply(pb, Matrix.Translation(h) @ r @ Matrix.Translation(-h))
        elif kind == 'point':
            q = (pb.tail - pb.head).normalized().rotation_difference(Vector(args[0]).normalized())
            q = Quaternion().slerp(q, amount)
            _apply(pb, Matrix.Translation(h) @ q.to_matrix().to_4x4() @ Matrix.Translation(-h))
        elif kind == 'move':
            _apply(pb, Matrix.Translation(Vector(args[0]) * amount))
        elif kind == 'tip':
            # ('tip', bone, degrees): about the bone's own X at its head (the hat's: +X tips the brim's front up, as
            # FBodyHatTip does while aiming); nothing without the bone
            if pb is not None:
                axis = (pb.matrix.to_3x3() @ Vector((1.0, 0.0, 0.0))).normalized()
                _apply(pb, Matrix.Translation(h) @ Matrix.Rotation(math.radians(args[0] * amount), 4, axis)
                       @ Matrix.Translation(-h))
        elif kind == 'twist':
            # ('twist', side, degrees, bend): the hand turned about the forearm's axis, each lowerarm_twist bone by its
            # share of the way down (TWIST_SHARES, as the game turns them), then the hand bent about its own X
            axis = (arm.pose.bones[f'lowerarm_{bone}'].tail - arm.pose.bones[f'lowerarm_{bone}'].head).normalized()
            for name, share in TWIST_SHARES:
                tb = arm.pose.bones.get(f'{name}_{bone}')
                if tb is not None:
                    h = tb.head.copy()
                    _apply(tb, Matrix.Translation(h) @ Matrix.Rotation(math.radians(args[0] * share * amount), 4, axis)
                           @ Matrix.Translation(-h))
            hand = arm.pose.bones[f'hand_{bone}']
            side = (hand.matrix.to_3x3() @ Vector((1.0, 0.0, 0.0))).normalized()
            h = hand.head.copy()
            _apply(hand, Matrix.Translation(h) @ Matrix.Rotation(math.radians(args[1] * amount), 4, side)
                   @ Matrix.Translation(-h))


# ------------------------------------------------------------------------------------------------- the swing springs
# Player/Animation/BodySwing.cpp's numbers (keep in step): the limbs a chain point keeps clear of (cm: the most it
# keeps), the share of its rest distance it keeps, each chain kind's stiffness and gravity share, and how much of the
# upper arm's movement the cape's corners take.
LIMBS = (('thigh_l', 'calf_l', 14.0), ('calf_l', 'foot_l', 10.0), ('thigh_r', 'calf_r', 14.0), ('calf_r', 'foot_r', 10.0),
         ('pelvis', 'spine_01', 18.0), ('spine_01', 'spine_03', 18.0), ('spine_03', 'spine_05', 18.0),
         ('spine_05', 'neck_01', 15.0), ('neck_01', 'head', 9.0), ('upperarm_l', 'lowerarm_l', 9.0),
         ('upperarm_r', 'lowerarm_r', 9.0), ('lowerarm_l', 'hand_l', 7.0), ('lowerarm_r', 'hand_r', 7.0),
         ('hand_l', 'middle_01_l', 5.0), ('hand_r', 'middle_01_r', 5.0), ('foot_l', 'ball_l', 8.0),
         ('foot_r', 'ball_r', 8.0))
CLEARANCE_SHARE = 0.95
# BodySwingRing: the skirt's chains in order round the body (the open front not joined); the cloth between neighbours
# kept off the limbs too, on what's shown only, moving a level's two points at most RING_PUSH (m) a frame
SKIRT_RING = ('skirt_fl', 'skirt_sl', 'skirt_bl', 'skirt_bli', 'skirt_bm', 'skirt_bri', 'skirt_br', 'skirt_sr', 'skirt_fr')
RING_PUSH = 0.12
TWIST_SHARES = (('lowerarm_twist_01', 0.5), ('lowerarm_twist_02', 0.9), ('hand', 1.0))   # the twist each takes (the game's)
LONE_BONE = 0.18                     # a one-bone chain's length (the lantern's)
MIN_BONE_ALONG = 0.35                # a bone's top 35% isn't pushed off a limb, left to the point above (at 0.2
                                     # crouched thighs swung a coat flap out level with the hip)
MAX_BONE_PUSH = 0.5                  # a bone push moves its tip at most this share of the bone's length a pass
FLOOR_CLEARANCE = 0.02               # no point under the ground at his feet (the armature's origin) + this
GATHER_REACH = 1.0 / 3.0             # a chain's last point lies on the floor at most this share of its length out
SPRINGS = {'skirt': (7.0, 0.08), 'cape': (12.0, 0.05), 'bandana': (9.0, 0.05), 'hair': (9.0, 0.05), 'lantern': (2.0, 1.0)}
BULK = {'lantern': 0.025, 'cape': 0.06}   # a thick thing keeps this much more than a limb's cap (m; its body's
# thickness); the cape (round 6, PROPOSED: BodySwing's cape Bulk 6 cm): its chains keep their room from the arms, the
# forearms and the hands too, so a gun's hands and forearms in front of the chest don't come through its front panels
# Round 6 (the CAPE agent; PROPOSED for BodySwing, which still has round 5's: f 0.6, any other 0.4, from the bind pose's
# upper arm, the side its name's 7th letter, KeepUnder level with the head): how much of its upper arm's turn each cape
# chain takes (cape_<key><side>: Ellis.py's CAPE_CHAINS; the side is the name's last letter, l or r; cape_bm, down the
# middle of the back, rides no arm), the arm taken from where it hangs at the game's idle (`cape_arm_<side>`: the cape
# is draped over arms hanging so, the bind pose's raised arms go through it), the upper arm's rest for every clearance
# from there too, and a chain point may rise as high as its target (a cape riding an arm raised to aim; KeepUnder).
CAPE_FOLLOW = {'f': 0.2, 'a': 1.0, 's': 1.0, 'b': 0.5}
IDLE_ARM_BONE = 'cape_arm_{}'


def cape_follow(chain):
    """(upper arm bone, share of its turn) a cape chain takes, or None (cape_bm, another chain)."""
    if not chain.startswith('cape_') or chain[-1] not in 'lr':
        return None
    share = CAPE_FOLLOW.get(chain[5:-1], 0.0)
    return (f'upperarm_{chain[-1]}', share) if share > 0.0 else None


def _idle_arm(bones, side):
    """The upper arm's rest as it hangs at the idle (cape_arm_<side>), or the bind pose's without one."""
    return bones.get(IDLE_ARM_BONE.format(side)) or bones[f'upperarm_{side}']


def _rest_limb(bones, a, b):
    """A limb's rest segment (from a's head to b's): the upper arm's as it hangs at the idle (_idle_arm)."""
    if a.startswith('upperarm_'):
        idle = _idle_arm(bones, a[-1])
        return idle.head_local.copy(), idle.matrix_local @ (bones[a].matrix_local.inverted() @ bones[b].head_local)
    return bones[a].head_local, bones[b].head_local


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


def _push_bone_out(head, tip, a, c, r, outward=None):
    """BodySwing's PushBoneOut: the tip turned about head so the bone keeps r from the limb a..c (not by its first
    MIN_BONE_ALONG), the tip moving the overlap over how far down the bone the nearest place is; out the body's side
    of the limb (outward, the chain's way out of the body, square to the limb and the bone), back across it (r plus
    the distance) when the limb has come through; with no side, straight away from the limb. (The shortest way apart
    pointed sideways when bone and limb lay in one front-to-back plane, and a stride's back shin swung the coat's back
    panel out round its side; away from the limb's line pointed down when the tip was below the shin, 2026-10-15.)"""
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
    axis = (c - a).normalized()
    side = Vector((0.0, 0.0, 0.0)) if outward is None else outward - axis * outward.dot(axis)
    bone = (tip - head) / length
    side = side - bone * side.dot(bone)
    overlap = r - dist
    if side.length > 1e-8:
        out = side.normalized()
        if dist > 1e-9 and (away / dist).dot(out) < -0.5:
            overlap = r + dist
    elif dist > 1e-9:
        out = away / dist
    else:
        v = tip - on_limb
        v = v - axis * v.dot(axis)
        out = v.normalized() if v.length > 1e-8 else Vector(FWD)
    return tip + out * min(overlap / along, length * MAX_BONE_PUSH)


def _keep_under(head, tip, length, target_z=None):
    """BodySwing's KeepUnder: nothing hanging rises above what it hangs from, or, riding an arm (target_z), above its
    target (round 6, PROPOSED: a cape riding an arm raised to aim is meant to rise); a tip over that goes level, out
    the way it went (straight up has no way: it stays as it is). Crouched thighs had swung a coat flap out level with
    the hip, once up to the shoulder."""
    rise = 0.0 if target_z is None else min(max(target_z - head.z, 0.0), length)
    if tip.z <= head.z + rise:
        return tip
    flat = Vector((tip.x - head.x, tip.y - head.y, 0.0))
    if flat.length <= 1e-8:
        return tip
    return head + flat.normalized() * math.sqrt(max(length * length - rise * rise, 0.0)) + Vector((0.0, 0.0, rise))


def _keep_above(head, tip, length, floor, lean, reach=None):
    """BodySwing's KeepAbove: a tip under the floor swung up about head onto it, out the way it leans (lean's, the rest
    direction, when it hangs straight down), no further out than reach (a chain's last: GATHER_REACH of its length, so
    its bone shortens and the hem gathers on the ground). The game also stills a point it finds under the floor (its
    speed into the ground flipped a gathered hem up over its head); this steady state has no speed to still."""
    if tip.z >= floor:
        return tip
    flat = Vector((tip.x - head.x, tip.y - head.y, 0.0))
    if flat.length <= 1e-9:
        flat = Vector((lean.x, lean.y, 0.0))
    if flat.length <= 1e-9:
        flat = Vector(FWD)
    drop = min(max(head.z - floor, 0.0), length)
    out = math.sqrt(max(length * length - drop * drop, 0.0))
    return head + flat.normalized() * min(out, length if reach is None else reach) - Vector((0.0, 0.0, drop))


def _push_place_out(s, a, c, r, outward):
    """BodySwing's PushPlaceOut: how far a place on the cloth moves to keep r from the limb a..c: straight away from
    it, or, on the limb's line or with the limb come through to the cloth's outer side, out the body's side (outward
    square to the limb) across it."""
    q = _closest(s, a, c)
    away = s - q
    d = away.length
    if d >= r:
        return Vector((0.0, 0.0, 0.0))
    axis = (c - a).normalized()
    side = outward - axis * outward.dot(axis)
    side = side.normalized() if side.length > 1e-8 else None
    if d > 1e-9 and (side is None or (away / d).dot(side) >= -0.5):
        return away / d * (r - d)
    return Vector((0.0, 0.0, 0.0)) if side is None else side * (r + d)


def _move_level(points, anchor, level, delta):
    """BodySwingRing's MoveLevel: points[level] moved by delta, re-hung from the one above at its length, the points
    below re-hung along their old ways from it at theirs."""
    lengths = [(points[i] - (points[i - 1] if i > 0 else anchor)).length for i in range(level, len(points))]
    head = points[level - 1] if level > 0 else anchor
    way = lambda v: v.normalized() if v.length > 1e-9 else Vector((0.0, 0.0, -1.0))
    points[level] = head + way(points[level] + delta - head) * lengths[0]
    for i in range(level + 1, len(points)):
        points[i] = points[i - 1] + way(points[i] - points[i - 1]) * lengths[i - level]


def _ring(arm, chain_bones, tips, shown, anchors, ways, limbs_now):
    """BodySwingRing's ApplyRing on the shown points: per neighbouring pair of skirt chains and level, the middle of
    the two points and the middle of the patch above them kept off every limb (each its rest distance x 0.95, capped),
    the push levelled and shared by nearness (the nearer point more), each point moved by its share, at most RING_PUSH."""
    bones = arm.data.bones
    rest_limbs = [(*_rest_limb(bones, a, b), m / 100.0) for a, b, m in LIMBS]
    order = [c for c in SKIRT_RING if c in chain_bones]
    for ca, cb in zip(order, order[1:]):
        rest_a = [bones[chain_bones[ca][0]].head_local] + tips[ca]
        rest_b = [bones[chain_bones[cb][0]].head_local] + tips[cb]
        out = (ways[ca] + ways[cb]).normalized()
        for level in range(min(len(shown[ca]), len(shown[cb]))):
            mid_rest = (rest_a[level + 1] + rest_b[level + 1]) * 0.5
            cen_rest = (rest_a[level] + rest_b[level] + rest_a[level + 1] + rest_b[level + 1]) * 0.25
            above_a = shown[ca][level - 1] if level > 0 else anchors[ca]
            above_b = shown[cb][level - 1] if level > 0 else anchors[cb]
            da, db = Vector((0.0, 0.0, 0.0)), Vector((0.0, 0.0, 0.0))
            for (a, c), (ra, rc, m) in zip(limbs_now, rest_limbs):
                pa, pb_ = shown[ca][level] + da, shown[cb][level] + db
                # the nearer point takes more of the push, levelled (both taking all of it folded a corner another limb
                # had pushed out right over it, and lifted the hem)
                na, nb = (pa - _closest(pa, a, c)).length, (pb_ - _closest(pb_, a, c)).length
                share = nb / (na + nb) if na + nb > 1e-9 else 0.5

                def spread(push, corners):
                    nonlocal da, db
                    push = Vector((push.x, push.y, 0.0))
                    da = da + push * (corners * share)
                    db = db + push * (corners * (1.0 - share))
                r_mid = min((mid_rest - _closest(mid_rest, ra, rc)).length * CLEARANCE_SHARE, m)
                r_cen = min((cen_rest - _closest(cen_rest, ra, rc)).length * CLEARANCE_SHARE, m)
                spread(_push_place_out((pa + pb_) * 0.5, a, c, r_mid, out), 2.0)
                centre = (above_a + above_b + shown[ca][level] + da + shown[cb][level] + db) * 0.25
                spread(_push_place_out(centre, a, c, r_cen, out), 4.0)
            for chain, d in ((ca, da), (cb, db)):
                if d.length > RING_PUSH:
                    d = d.normalized() * RING_PUSH
                if d.length > 1e-4:
                    _move_level(shown[chain], anchors[chain], level, d)


def _rest_tips(arm, names):
    """A chain's bone tips as the game takes them (BodySwing's Bind: an FBX skeleton has no tails): the next bone's
    head, the last bone's its `<chain>_end` leaf's head (where the cloth ends), or without one the last head plus the
    bone before it, a lone bone's LONE_BONE straight down."""
    heads = [arm.data.bones[n].head_local.copy() for n in names]
    end = arm.data.bones.get(names[0].rsplit('_', 1)[0] + '_end')
    tips = []
    for i, h in enumerate(heads):
        if i + 1 < len(heads):
            tips.append(heads[i + 1].copy())
        elif end is not None:
            tips.append(end.head_local.copy())
        elif i > 0:
            tips.append(h + (h - heads[i - 1]))
        else:
            tips.append(h + Vector((0.0, 0.0, -LONE_BONE)))
    return tips


def settle_swing(arm, chain_bones, steps=None, stages=12, reach=0.01):
    """Turns the swing bones where the game's springs would rest them in a pose (BodySwing's steady state: each point
    at its animated place, the cape's corners partly riding the upper arms, sagging by its gravity share over its
    stiffness squared, then each its length from the one before and pushed out of the limbs it kept clear of at
    rest, its bone too (_push_bone_out, out the chain's way out of the body), and kept over the floor (_keep_above,
    the last point gathering); the points the game's, _rest_tips). chain_bones: {chain: [bone names in order]}. With
    steps (the pose's POSES
    entry) the pose is reached in stages from rest, each point moving at most reach (m) a pass toward its place and
    kept out of the limbs on the way, as the springs follow a move frame by frame (set straight into a pose, a limb
    already past a point would push it out the far side). Without steps, the pose as it is, in one go."""
    bones = arm.data.bones
    rest_limbs = [(*_rest_limb(bones, a, b), m / 100.0) for a, b, m in LIMBS]
    tips = {chain: _rest_tips(arm, names) for chain, names in chain_bones.items()}
    bulk = lambda chain: BULK.get(chain.split('_')[0], 0.0)
    clear = {chain: [[min((t - _closest(t, a, c)).length * CLEARANCE_SHARE, m + bulk(chain)) for a, c, m in rest_limbs]
                     for t in tips[chain]] for chain in chain_bones}
    # and each bone, head to tip, its rest distance from each limb (segment to segment), as BodySwing's Bind
    gap = lambda q: (q[0] - q[1]).length
    bone_clear = {chain: [[min(gap(_segments(bones[n].head_local, t, a, c)) * CLEARANCE_SHARE, m + bulk(chain))
                           for a, c, m in rest_limbs]
                          for n, t in zip(names, tips[chain])] for chain, names in chain_bones.items()}
    floor = arm.matrix_world.translation.z + FLOOR_CLEARANCE
    # each chain's way out of the body (Bind's Outward): level, from the pelvis to where the chain ends, turning with
    # its first bone
    mid = bones['pelvis'].head_local
    outward = {chain: Vector((tips[chain][-1].x - mid.x, tips[chain][-1].y - mid.y, 0.0)).normalized()
               for chain in chain_bones}
    to_pose = lambda pbs, n: pbs[n].matrix @ bones[n].matrix_local.inverted()
    held, anchors, ways = {}, {}, {}
    for stage in range(1, stages + 1) if steps is not None else (None,):
        if stage is not None:
            pose(arm, steps, stage / stages)
        bpy.context.view_layer.update()
        pbs = arm.pose.bones
        now_limbs = [(pbs[a].head.copy(), pbs[b].head.copy()) for a, b, _m in LIMBS]
        for chain, names in chain_bones.items():
            k, g = SPRINGS[chain.split('_')[0]]
            follow = cape_follow(chain)
            if follow is not None:
                # from the arm as it hangs at the idle (_idle_arm), not the bind pose's
                follow = (pbs[follow[0]].matrix @ _idle_arm(bones, follow[0][-1]).matrix_local.inverted(), follow[1])
            targets, lengths = [], []
            for name, tip_rest in zip(names, tips[chain]):
                tip = to_pose(pbs, name) @ tip_rest
                if follow is not None:
                    tip = tip.lerp(follow[0] @ tip_rest, follow[1])
                targets.append(tip + Vector((0.0, 0.0, -9.8 * g / (k * k))))
                lengths.append((tip_rest - bones[name].head_local).length)
            anchor = pbs[names[0]].head.copy()
            way = (to_pose(pbs, names[0]).to_3x3().normalized() @ outward[chain]).normalized()
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
                        p = _push_bone_out(head, p, a, c, rb, way)
                    p = head + ((p - head).normalized() if (p - head).length > 1e-9 else rest_dir) * lengths[i]
                    # (a chain riding an arm may rise to its target; the rest no higher than its head)
                    p = _keep_under(head, p, lengths[i], targets[i].z if follow is not None else None)
                    p = _keep_above(head, p, lengths[i], floor, rest_dir,
                                    lengths[i] * (GATHER_REACH if i == len(points) - 1 else 1.0))
                    points[i] = p
                    head = p
            held[chain], anchors[chain], ways[chain] = points, anchor, way
    # the cloth between the skirt's chains off the limbs too, on what's shown (not fed back to the springs)
    shown = {chain: [p.copy() for p in pts] for chain, pts in held.items()}
    _ring(arm, chain_bones, tips, shown, anchors, ways, now_limbs)
    pbs = arm.pose.bones
    for chain, names in chain_bones.items():
        for name, tip_rest, p in zip(names, tips[chain], shown[chain]):
            pb = pbs[name]
            h = pb.head.copy()
            q = (to_pose(pbs, name) @ tip_rest - h).normalized().rotation_difference((p - h).normalized())
            _apply(pb, Matrix.Translation(h) @ q.to_matrix().to_4x4() @ Matrix.Translation(-h))
            # gathered on the floor (its tip nearer than its length): the bone shortened along its length (Blender's
            # Y; the game's Apply scales the bone's own axis the tip lies along)
            share = (p - h).length / max((tip_rest - bones[name].head_local).length, 1e-9)
            if share < 0.995:
                pb.scale = (1.0, max(share, 0.05), 1.0)
                bpy.context.view_layer.update()


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
