"""The Stone Sentinel's poses as the game makes them, for the check renders (parts_sentinel.py): a line-for-line port of
AStoneSentinelCreature::AnimateBody (Source/AI_Looter_Shooter/Creatures/StoneSentinelCreaturePose.cpp) and its poses
(StoneSentinelRules.cpp: Idle, Raised, Down). Not a model: Tools\\models.ps1 skips parts_*.py.

The C++ works in component space (Unreal: cm, x forward, y right, z up) on each bone's rest transform; a bone's own rest
rotation drops out of everything it does (each turn is laid on in component space, each aim measured from the rest
way), so the port keeps only rotations D (component space) and places, then maps them to Blender (metres, x left,
-y forward): B = M U with M = [[0,-1,0],[-1,0,0],[0,0,1]] (a mirror, so a rotation maps as M D M).
"""
import math

import numpy as np
from mathutils import Matrix, Quaternion, Vector

UPPER_SPREAD, FORE_SPREAD = 0.2, -0.12
LOWER_SPINE, UPPER_SPINE = 0.6, 0.4
THIGH_SWING, KNEE_BEND, STEP_SWAY = 15.0, 16.0, 2.5
HEAD_LIFT = 12.0
WAKE_SHUDDER = 2.5
RIGHT, FORWARD, UP = Vector((0.0, 1.0, 0.0)), Vector((1.0, 0.0, 0.0)), Vector((0.0, 0.0, 1.0))
M = Matrix(((0.0, -1.0, 0.0), (-1.0, 0.0, 0.0), (0.0, 0.0, 1.0)))

# StoneSentinelRules.cpp: (ArmSwing, ElbowBend, Bow) in degrees. Raised and Down are the clean extremes found for the Tripo
# model (2026-10-30; were 172/35/-10 and 62/6/34, which split the shoulders and the trunk into shards and fins): the
# fists still come up over the hood with the elbows bent, and come down to the ground with less of a bow.
IDLE, RAISED, DOWN = (8.0, 12.0, 4.0), (128.0, 70.0, -8.0), (45.0, 20.0, 22.0)

# The check poses: name -> (awake weight, walking, gait phase, pose, shudder clock or None).
POSES = {
    'dormant': (0.0, 0.0, 0.0, IDLE, None),
    'waking': (0.4, 0.0, 0.0, IDLE, 0.37),
    'walk': (1.0, 1.0, 0.25, IDLE, None),
    'slam_windup': (1.0, 0.0, 0.0, RAISED, None),
    'slam': (1.0, 0.0, 0.0, DOWN, None),
}


def _q(axis, degrees):
    return Quaternion(axis, math.radians(degrees))


def _limb_way(swing, out):
    r = math.radians(swing)
    return Vector((math.sin(r), out, -math.cos(r))).normalized()


def _unwind(a):
    while a > 180.0:
        a -= 360.0
    while a < -180.0:
        a += 360.0
    return a


def _aim(d, local_way, wanted_swing, wanted, weight):
    """AimLimb: d (component-space rotation) swung round the side axis to wanted_swing, then onto wanted."""
    if weight <= 0.0:
        return d
    now = (d @ local_way).normalized()
    now_swing = math.degrees(math.atan2(now.x, -now.z))
    swing = Quaternion(RIGHT, -math.radians(_unwind(wanted_swing - now_swing)))
    full = swing
    if wanted.length > 1e-6:
        full = (swing @ now).rotation_difference(wanted.normalized()) @ swing
    return Quaternion().slerp(full, min(max(weight, 0.0), 1.0)) @ d


def pose(bones, weight, walking, phase, arm_pose, clock=None):
    """bones: [(name, parent or None, rest head in Blender m)], parents first. Returns {name: 4x4 (Blender)} moving each
    bone's rest to its pose."""
    heads = {n: (M @ Vector(h)) * 100.0 for n, _, h in bones}
    parent = {n: p for n, p, _ in bones}
    children = {}
    for n, p, _ in bones:
        children.setdefault(p, []).append(n)

    def first_child(n, prefix):
        return next((c for c in children.get(n, []) if c.startswith(prefix)), None)

    def way(n, nxt):
        return (heads[nxt] - heads[n]).normalized() if nxt else Vector((0.0, 0.0, -1.0))

    arm_swing, elbow, bow = arm_pose
    shudder = WAKE_SHUDDER * (1.0 - weight) if clock is not None else 0.0
    quake = (math.sin(clock * 37.0) * shudder, math.sin(clock * 29.0 + 1.3) * shudder) if clock is not None else (0.0, 0.0)
    cycle = phase * 2.0 * math.pi
    sway = STEP_SWAY * walking * math.sin(cycle)
    bow1 = _q(RIGHT, bow * weight * LOWER_SPINE)
    bow2 = _q(RIGHT, bow * weight * UPPER_SPINE + quake[0])
    roll1 = _q(FORWARD, sway * weight)
    roll2 = _q(FORWARD, quake[1])
    head = _q(UP, 0.0) @ _q(RIGHT, -HEAD_LIFT * weight)
    rot, place = {}, {}
    for n, p, _ in bones:
        if p is None:
            d, at = Quaternion(), heads[n].copy()
        else:
            d = rot[p].copy()
            at = place[p] + rot[p] @ (heads[n] - heads[p])
        if n == 'spine_01':
            d = roll1 @ bow1 @ d
        elif n == 'spine_02':
            d = roll2 @ bow2 @ d
        elif n == 'head':
            d = head @ d
        side = n[-2:]
        if side in ('_l', '_r'):
            out = -1.0 if side == '_l' else 1.0
            phase_side = 0.0 if side == '_l' else math.pi
            if n.startswith('upperarm'):
                d = _aim(d, way(n, first_child(n, 'lowerarm')), arm_swing, _limb_way(arm_swing, out * UPPER_SPREAD), weight)
            elif n.startswith('lowerarm'):
                s = arm_swing + elbow
                d = _aim(d, way(n, first_child(n, 'hand')), s, _limb_way(s, out * FORE_SPREAD), weight)
            elif n.startswith('thigh'):
                leg = -THIGH_SWING * walking * math.sin(cycle + phase_side)
                d = _aim(d, way(n, first_child(n, 'calf')), leg, Vector(), weight)
            elif n.startswith('calf'):
                leg = -THIGH_SWING * walking * math.sin(cycle + phase_side)
                bend = KNEE_BEND * walking * max(0.0, math.cos(cycle + phase_side))
                d = _aim(d, way(n, first_child(n, 'foot')), leg - bend, Vector(), weight)
        rot[n], place[n] = d, at
    out = {}
    for n, _, h in bones:
        r = M @ rot[n].to_matrix() @ M
        t = (M @ place[n]) * 0.01
        out[n] = Matrix.Translation(t) @ r.to_4x4() @ Matrix.Translation(-Vector(h))
    return out


def posed_copy(body, bones, matrices, name):
    """A copy of body (each vertex wholly one bone's) moved by matrices."""
    import bpy
    obj = body.copy()
    obj.data = body.data.copy()
    obj.name = obj.data.name = name
    obj.modifiers.clear()
    obj.parent = None
    bpy.context.scene.collection.objects.link(obj)
    me = obj.data
    groups = {g.index: g.name for g in obj.vertex_groups}
    co = np.empty(len(me.vertices) * 3, np.float64)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    for v in me.vertices:
        if not v.groups:
            continue
        g = max(v.groups, key=lambda e: e.weight)
        m = matrices.get(groups[g.group])
        if m is not None:
            co[v.index] = m @ Vector(co[v.index])
    me.vertices.foreach_set('co', co.ravel())
    me.update()
    return obj
