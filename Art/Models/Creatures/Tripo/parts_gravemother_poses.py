"""Check renders of SK_Gravemother in the game's poses (Gravemother.py's previews run it), posed as ASpiderCreature poses
its bones: each leg by the game's two-bone IK toward its knee pole (up off the body and 0.4 out), the femur and tibia
carried by their segment's frame (SpiderCreatureRig.cpp: the bone kept as it rests in its segment), the body, fangs and
abdomen about their pivots. Not a model: Tools\\models.ps1 skips parts_*.py.

Poses (creature cm, the spider's 1x; SpiderCreature.cpp, SpiderCreatureDeath.cpp):
  rest     as built: the IK's standing pose, feet on the ground
  gait     mid-stride: the alternating tetrapod's one group (L0 R1 L2 R3) lifted 22 cm and swung 20 ahead, the other planted
           18 behind its rest, the body up 2
  attack   the bite's wind-up at its height: the body pitched 24 deg nose up and lifted 16, drawn back 14, the front feet
           raised to (hip x + 80, hip y * 1.5, 50) in the body's frame, the fangs spread 30 deg
  death    the curled corpse: the thorax at DeadRide 46, each femur swung to DeadKneeYaw and down to its knee at
           DeadKneeHeight, each tip drawn in under the body (DeadTipSide, DeadTipHeight), fangs 20, abdomen dropped 12 deg"""
import math
import os

import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_barrow as lb
import looter_creatures as lc

RIDE = 60.0
DEAD_RIDE = 46.0
DEAD_KNEE_YAW = (8.0, 22.0, 156.0, 142.0)
DEAD_KNEE_HEIGHT = (12.0, 12.0, 12.0, 9.0)
DEAD_TIP_SIDE = (6.0, 18.0, 8.0, 30.0)
DEAD_TIP_HEIGHT = (5.0, 3.0, 7.0, 3.0)
TB = lc.TO_BLENDER


def seg_frame(a, b, pole):
    """FRotationMatrix::MakeFromXZ(b - a, pole) at a, creature cm (4x4)."""
    x = (b - a).normalized()
    y = pole.cross(x).normalized()
    z = x.cross(y)
    M = Matrix((x, y, z)).transposed().to_4x4()
    M.translation = a
    return M


def knee_pole(up, outward):
    return up + outward * 0.4


def rot(yaw=0.0, pitch=0.0):
    """A turn in creature space: yaw about z (toward +y, her left), pitch nose up."""
    return Matrix.Rotation(math.radians(yaw), 4, 'Z') @ Matrix.Rotation(math.radians(-pitch), 4, 'Y')


def solve(hip, target, upper, lower, pole):
    return lc.solve_two_bone(hip, target, upper, lower, pole)


def pose(rig, bones, kind):
    """Poses rig (bones: (name, head, tail, parent) creature cm, as laid) in a pose; returns the abdomen's motion (4x4,
    Blender space) for whatever rides on it."""
    rest = {n: (Vector(h), Vector(t)) for n, h, t, p in bones}
    thorax = rest['body'][0]
    body = Matrix.Identity(4)
    fang = 0.0
    abdomen_pitch = 0.0
    if kind == 'gait':
        body = Matrix.Translation((0.0, 0.0, 2.0))
    elif kind == 'attack':
        body = Matrix.Translation((-14.0, 0.0, 16.0)) @ Matrix.Translation(thorax) @ rot(pitch=24.0) @ \
            Matrix.Translation(-thorax)
        fang = 30.0
    elif kind == 'death':
        body = Matrix.Translation((0.0, 0.0, DEAD_RIDE - RIDE))
        fang = 20.0
        abdomen_pitch = -12.0
    world = {}                                      # bone: its new armature-space matrix (creature cm)
    rest_m = {}
    for n, (h, t) in rest.items():
        rb = rig.data.bones[n]
        R = (TB.inverted() @ rb.matrix_local)        # its rest matrix in creature cm (with the 0.01 scale undone below)
        R = Matrix.LocRotScale(TB.inverted() @ rb.head_local, R.to_3x3().normalized().to_quaternion(), None)
        rest_m[n] = R
    for n in rest:
        if n.startswith(('femur', 'tibia', 'foot')):
            continue
        M = body @ rest_m[n]
        if n.startswith('fang'):
            side = 1.0 if n.endswith('_l') else -1.0
            h = body @ rest[n][0]
            M = Matrix.Translation(h) @ (body.to_3x3().to_4x4()) @ rot(yaw=fang * side, pitch=fang * 0.4) @ \
                (body.to_3x3().to_4x4()).inverted() @ Matrix.Translation(-h) @ M
        if n == 'abdomen' and abdomen_pitch:
            h = body @ rest[n][0]
            M = Matrix.Translation(h) @ rot(pitch=abdomen_pitch) @ Matrix.Translation(-h) @ M
        world[n] = M
    up = (body.to_3x3() @ Vector((0.0, 0.0, 1.0))).normalized()
    for pair in range(4):
        for suffix in ('l', 'r'):
            leg = f'{pair}_{suffix}'
            hip_r, knee_r = rest[f'femur_{leg}']
            foot_r = rest[f'tibia_{leg}'][1]
            upper, lower = (knee_r - hip_r).length, (foot_r - knee_r).length
            out_r = Vector((hip_r.x, hip_r.y, 0.0)).normalized()
            pole_r = knee_pole(Vector((0.0, 0.0, 1.0)), out_r)
            hip = body @ hip_r
            out = Vector((hip.x - (body @ thorax).x, hip.y - (body @ thorax).y, 0.0)).normalized()
            pole = knee_pole(up, out)
            side = 1.0 if hip_r.y > 0 else -1.0
            group = (pair + (1 if suffix == 'r' else 0)) % 2
            target = foot_r.copy()
            if kind == 'gait':
                target = foot_r + (Vector((20.0, 0.0, 22.0)) if group == 0 else Vector((-18.0, 0.0, 0.0)))
            elif kind == 'attack' and pair == 0:
                hb = hip_r - thorax
                target = body @ (thorax + Vector((hb.x + 80.0, hb.y * 1.5, 50.0)))
            if kind == 'death':
                hb = hip_r - thorax
                yaw = math.radians(DEAD_KNEE_YAW[pair])
                flat = Vector((math.cos(yaw), side * math.sin(yaw), 0.0))
                drop = max(-1.0, min(1.0, (hb.z - (DEAD_KNEE_HEIGHT[pair] - DEAD_RIDE)) / upper))
                kb = hb + (flat * math.sqrt(1.0 - drop * drop) - Vector((0.0, 0.0, 1.0)) * drop) * upper
                across = side * DEAD_TIP_SIDE[pair] - kb.y
                down = (DEAD_TIP_HEIGHT[pair] - DEAD_RIDE) - kb.z
                along = lower * lower - across * across - down * down
                fb = kb + (Vector(((-1.0 if flat.x > 0 else 1.0) * math.sqrt(along), across, down)) if along > 0
                           else Vector((0.0, across, down)).normalized() * lower)
                base = Vector((0.0, 0.0, DEAD_RIDE))
                knee, foot = base + kb, base + fb
            else:
                knee, foot = solve(hip, target, upper, lower, pole)
            Fr = seg_frame(hip_r, knee_r, pole_r)
            Tr = seg_frame(knee_r, foot_r, pole_r)
            world[f'femur_{leg}'] = seg_frame(hip, knee, pole) @ Fr.inverted() @ rest_m[f'femur_{leg}']
            world[f'tibia_{leg}'] = seg_frame(knee, foot, pole) @ Tr.inverted() @ rest_m[f'tibia_{leg}']
            world[f'foot_{leg}'] = world[f'tibia_{leg}'] @ rest_m[f'tibia_{leg}'].inverted() @ rest_m[f'foot_{leg}']
    # Into Blender (parents first): each bone's armature-space matrix, its rest scale kept.
    order = [b.name for b in rig.data.bones]
    order.sort(key=lambda n: len(rig.data.bones[n].parent_recursive))
    for n in order:
        M = world[n]
        loc = TB @ M.translation
        q = (TB.to_3x3().normalized() @ M.to_3x3()).to_quaternion()
        rig.pose.bones[n].matrix = Matrix.LocRotScale(loc, q, None)
        bpy.context.view_layer.update()
    a = rig.data.bones['abdomen']
    return rig.matrix_world @ rig.pose.bones['abdomen'].matrix @ a.matrix_local.inverted() @ rig.matrix_world.inverted()


def clear(rig):
    for pb in rig.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()


def render_poses(rig, mesh, stone, out_dir, name='Gravemother'):
    """The side view on the ground (the level check) and each pose from her front three-quarter and side."""
    shots = []
    rest_stone = stone.matrix_world.copy() if stone else None
    for kind in ('rest', 'gait', 'attack', 'death'):
        clear(rig)
        move = pose(rig, kind_bones(rig), kind) if kind != 'rest' else Matrix.Identity(4)
        if stone:
            stone.matrix_world = move @ rest_stone
        shown = [mesh] + ([stone] if stone else [])
        for view, cam, target in (('three', (-2.3, -3.2, 1.6), (0.0, 0.3, 0.6)), ('side', (4.2, 0.25, 0.55), (0.0, 0.25, 0.5))):
            shots.append(lb.render(shown, os.path.join(out_dir, f'{name}_pose_{kind}_{view}.png'), cam, target, lens=38.0,
                                   size=(1200, 760), sun_dir=(-0.45, -0.75, 0.75)))
    clear(rig)
    if stone:
        stone.matrix_world = rest_stone
    # The level check: square to her side, orthographic, on the ground.
    shots.append(lb.render([mesh] + ([stone] if stone else []), os.path.join(out_dir, f'{name}_level_side.png'),
                           (6.0, 0.3, 0.6), (0.0, 0.3, 0.6), size=(1400, 760), ortho=4.6, sun_dir=(-0.3, -0.8, 0.6)))
    lb.sheet(shots, os.path.join(out_dir, f'{name}_poses_sheet.png'), cols=2)
    return shots


def kind_bones(rig):
    """The rig's bones as laid (name, head, tail, parent) in creature cm, read back off the armature."""
    inv = TB.inverted()
    return [(b.name, inv @ b.head_local, inv @ b.tail_local, b.parent.name if b.parent else None) for b in rig.data.bones]
