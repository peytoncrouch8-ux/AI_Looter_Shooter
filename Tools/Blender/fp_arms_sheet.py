"""The first-person arms' look sheet (Saved/Concepts/Hands/fp_arms_sheet.png, for the user's phone): SK_FP_Arms
(Art/Models/Characters/FPArms.py) seen from the eye as the game draws it (90 degree view, 16:9), holding stand-ins
built by the gun scripts (Art/Models/Weapons/Bullpup.py, Drover.py, their default parts): the Bullpup at the hip, aimed
down its sight, and mid-reload with the support hand on the magazine; the Drover at its own hip hold, two-handed; and a
close-up of the glove; and views from beside the gun where the eye's view hides the hands. The guns sit where the game
puts them (PlayerLocomotionViewModel.cpp: the long guns' grip at 50, 20, -26 cm from the eye; SOCKET_Aim 22 cm ahead when
aiming; the magazine reload's pose; create_revolver.py's hip hold).
The hands are posed here by hand, the way the code could: each hand's frame from the handle it holds (HANDLE_AXIS through
GRIP_POINT, turned about the handle to suit the forearm and the palm), slid along its palm's normal till the palm just
touches; fingers closed all together, then joint by joint, till they touch the gun (the index searched onto the
trigger); the forearm worked back from the hand (wrist nearly straight), the upper arm aimed at a point below Ellis's
shoulder (the body isn't drawn, so the shoulder end floats out of sight). Golden late-afternoon light, as the level's.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Tools\\Blender\\fp_arms_sheet.py -ScriptArgs keep-textures
    ... -ScriptArgs keep-textures,bullpup     only the Bullpup's shots (also: drover, sheet; fast halves the renders;
                                              debug renders quick looks round each pose and prints the joints)
The renders go to Intermediate/FPArms/Sheet/; the sheet is composed from them.
"""
import math
import os
import runpy
import sys
import time

import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_textures as lt

ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
FAST = 'fast' in ARGV
WORK = os.path.join(lt.REPO, 'Intermediate', 'FPArms', 'Sheet')
SHEET = os.path.join(lt.REPO, 'Saved', 'Concepts', 'Hands', 'fp_arms_sheet.png')
T0 = time.time()
EYE_H = 1.45                                 # the ground under the eye (the player is 0.85 of Ellis's 1.80 m)


def log(message):
    print(f'FPSHEET [{time.time() - T0:6.1f}s] {message}', flush=True)


def unit(v):
    v = np.asarray(v, dtype=np.float64)
    return v / max(float(np.linalg.norm(v)), 1e-12)


def rot(axis, degrees):
    k = unit(axis)
    a = math.radians(degrees)
    K = np.array([[0, -k[2], k[1]], [k[2], 0, -k[0]], [-k[1], k[0], 0]])
    return np.eye(3) + math.sin(a) * K + (1 - math.cos(a)) * K @ K


def about(point, axis, degrees):
    """4x4: a turn about axis through point."""
    m = np.eye(4)
    R = rot(axis, degrees)
    m[:3, :3] = R
    m[:3, 3] = point - R @ point
    return m


def affine(R, t):
    m = np.eye(4)
    m[:3, :3] = R
    m[:3, 3] = t
    return m


# ---------------------------------------------------------------------------------------- Unreal's numbers
def ue(x, y, z):
    """A point in the camera's Unreal space (cm: X ahead, Y right, Z up) in Blender's (m: -Y ahead, +X left)."""
    return np.array([-y, -x, z]) * 0.01


UE_TO_B = np.array([[0.0, -1.0, 0.0], [-1.0, 0.0, 0.0], [0.0, 0.0, 1.0]])


def ue_rotator(pitch=0.0, yaw=0.0, roll=0.0):
    """FRotator (degrees) as a Blender rotation matrix."""
    p, y, r = (math.radians(a) for a in (pitch, yaw, roll))
    cp, sp, cy, sy, cr, sr = math.cos(p), math.sin(p), math.cos(y), math.sin(y), math.cos(r), math.sin(r)
    X = [cp * cy, cp * sy, sp]
    Y = [sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp]
    Z = [-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp]
    R = np.array([X, Y, Z]).T
    return UE_TO_B @ R @ UE_TO_B


# ------------------------------------------------------------------------------------------------- the arms
A = runpy.run_path(os.path.join(lt.REPO, 'Art', 'Models', 'Characters', 'FPArms.py'))
ARM, BODY, REST, CHAINS, CM = A['ARM'], A['BODY'], A['REST'], A['CHAINS'], A['C']
GRIP_C = A['GRIP_POINT'] * CM
MX = np.diag([-1.0, 1.0, 1.0])
H_C = unit(A['HANDLE_AXIS'])                 # a held handle's axis across the palm (hand frame), toward the thumb
Z_C = np.array([0.0, 0.0, 1.0])
B_C = np.column_stack([H_C, Z_C, np.cross(H_C, Z_C)])
REST_BONE = {b.name: np.array(b.matrix_local) for b in ARM.data.bones}


def side_rest(side):
    """The hand frame at rest (rotation, wrist) for a side; the left's is the mirror (a reflected frame)."""
    if side == 'r':
        return REST.R.copy(), REST.wrist.copy()
    return MX @ REST.R, MX @ REST.wrist


def hand_frame(side, handle, forearm, twist=0.0, palm=None, palm_weight=1.0):
    """The hand's frame holding a handle along `handle` (the thumb's way), its forearm leaving toward `forearm` (and its
    palm facing `palm`, if given): of the turns about the handle, the one that matches best; twist turns it further."""
    handle = unit(handle)
    ref = unit(np.cross(handle, [0.31, 0.17, 0.93]))
    best, best_R = -9.0, None
    for psi in np.arange(0.0, 360.0, 1.0):
        back = rot(handle, psi) @ ref
        third = np.cross(handle, back) * (1.0 if side == 'r' else -1.0)
        R = np.column_stack([handle, back, third]) @ B_C.T
        score = float(-R[:, 0] @ unit(forearm))
        if palm is not None:
            score += palm_weight * float(-R[:, 2] @ unit(palm))
        if score > best:
            best, best_R = score, R
    if twist:
        best_R = rot(handle, twist) @ best_R
    return best_R


# Finger and thumb kinematics in the hand frame (the right hand's sense; the left is the same in its mirrored frame).
def finger_fk(ch, curl, spread=0.0):
    J = ch.joints
    F1 = about(J[1], Z_C, spread) @ about(J[1], ch.lateral, curl[0])
    F2 = F1 @ about(J[2], ch.lateral, curl[1])
    F3 = F2 @ about(J[3], ch.lateral, curl[2])
    return [np.eye(4), F1, F2, F3]


def thumb_axes(ch):
    out = []
    for k in range(3):
        e = unit(ch.joints[k + 1] - ch.joints[k])
        out.append(unit(np.cross(e, -ch.dorsal(k))))
    return out


def thumb_fk(ch, opp, flex, c2, c3):
    J = ch.joints
    k = thumb_axes(ch)
    F1 = about(J[0], [-1.0, 0.0, 0.0], opp) @ about(J[0], k[0], flex)
    F2 = F1 @ about(J[1], k[1], c2)
    F3 = F2 @ about(J[2], k[2], c3)
    return [F1, F2, F3]


class Hand:
    """One posed hand: its frame and wrist (Blender), and its fingers' turns."""

    def __init__(self, side, R, grip_world, along=0.0, tilt=0.0, forward=0.0):
        """tilt turns the hand about its palm's normal (the handle then crosses the palm a little steeper or flatter);
        along slides it along the handle, forward along its fingers (cm; a thick grip sits nearer the wrist)."""
        self.side = side
        if tilt:
            R = rot(R @ np.array([0.0, 0.0, -1.0]), tilt) @ R
        self.R = R
        self.wrist = np.asarray(grip_world) - R @ (GRIP_C + np.array([-forward, along, 0.0]) * CM)
        self.curls = {ch.name: (0.0, 0.0, 0.0) for ch in CHAINS[:4]}
        self.spread = {ch.name: 0.0 for ch in CHAINS[:4]}
        self.thumb = (0.0, 0.0, 0.0, 0.0)

    def M(self):
        return affine(self.R, self.wrist)

    def rest_M(self):
        R, W = side_rest(self.side)
        return affine(R, W)

    def world(self, p):
        return self.wrist + self.R @ p

    def chain_F(self, ch, curl=None, thumb=None):
        if ch.name == 'thumb':
            return thumb_fk(ch, *(thumb or self.thumb))
        return finger_fk(ch, curl or self.curls[ch.name], self.spread[ch.name])

    def samples(self, ch, F, first):
        """Points along the chain's posed segments from `first` on, with their radii (Blender space)."""
        pts, rads = [], []
        segs = range(first, 4) if ch.name != 'thumb' else range(first, 3)
        for s in segs:
            Fs = F[s]
            a, b = ch.joints[s], ch.joints[s + 1]
            # The joint being turned doesn't move, so it can't be what stops the turn: start past it.
            for t in np.linspace(0.3 if s == first else 0.0, 1.0, 6):
                p = a + (b - a) * t
                pc = Fs[:3, :3] @ p + Fs[:3, 3]
                pts.append(self.world(pc))
                rads.append(ch.radii[s] + (ch.radii[s + 1] - ch.radii[s]) * t)
        return pts, rads


def palm_points():
    """Points on the glove's palm (hand frame, m), found on the hand's own distance field."""
    pts = []
    for x in (2.6, 4.2, 5.8, 7.4, 8.6):
        for y in (-2.9, -1.4, 0.0, 1.4, 2.9):
            lo, hi = -6.0 * CM, 0.0
            p = lambda z: np.array([[x * CM, y * CM, z]])
            for _ in range(24):
                mid = 0.5 * (lo + hi)
                if A['body_sdf'](p(mid))[0] > 0.0:
                    lo = mid
                else:
                    hi = mid
            pts.append(np.array([x * CM, y * CM, hi]))
    return pts


PALM_POINTS = palm_points()


def seat(hand, trees, gap=0.0015, reach=0.08):
    """Slides the hand along its palm's normal until the palm just touches what it holds."""
    n = hand.R @ np.array([0.0, 0.0, -1.0])
    best = None
    for p in PALM_POINTS:
        start = hand.world(p) - n * reach
        for tree in trees:
            hit = tree.ray_cast(Vector(start), Vector(n), reach * 2.0)
            if hit[0] is not None:
                d = hit[3] - reach
                best = d if best is None else min(best, d)
    if best is None:
        log(f'  {hand.side} palm touches nothing')
        return
    hand.wrist = hand.wrist + n * (best - gap)
    log(f'  {hand.side} palm seated {best * 100:+.1f} cm')


def hits(trees, pts, rads, squash=0.9):
    for p, r in zip(pts, rads):
        v = Vector(p)
        for tree in trees:
            if tree.find_nearest(v, r * squash)[0] is not None:
                return True
    return False


def close_fingers(hand, trees, names=('index', 'middle', 'ring', 'pinky'), limits=(80.0, 95.0, 70.0), step=2.0,
                  start=None):
    """Each finger closes knuckle, middle joint, end joint in turn, each until the finger touches the gun."""
    for ch in CHAINS[:4]:
        if ch.name not in names:
            continue
        ang = list(start.get(ch.name, (0.0, 0.0, 0.0)) if start else (0.0, 0.0, 0.0))
        test = lambda a, first=1: hits(trees, *hand.samples(ch, finger_fk(ch, a, hand.spread[ch.name]), first))
        if test(ang):
            # Blocked where it rests (a trigger, a loop): straighten it, lift it at the knuckle or spread it, till free.
            base = hand.spread[ch.name]
            tries = [(base, (-14.0 * k / 4, -20.0 * k / 4, -12.0 * k / 4)) for k in range(1, 5)]
            tries += [(base + s, (0.0, 0.0, 0.0)) for s in (-5, 5, -10, 10, -15, 15)]
            tries += [(base, (-14.0 - 4.0 * k, -20.0, -12.0)) for k in range(1, 8)]
            for spread, lift in tries:
                hand.spread[ch.name] = spread
                if not test(list(lift)):
                    ang = list(lift)
                    break
            else:
                hand.spread[ch.name] = base
                log(f'  {hand.side} {ch.name} is blocked')
        # All three joints together first (a long finger doesn't sweep its tip into what's ahead), then each on its own.
        while all(a < m for a, m in zip(ang, limits)):
            trial = [ang[0] + step, ang[1] + step * 1.15, ang[2] + step * 0.8]
            if test(trial):
                break
            ang = trial
        for j in range(3):
            while ang[j] < limits[j]:
                trial = list(ang)
                trial[j] += step
                if hits(trees, *hand.samples(ch, finger_fk(ch, trial, hand.spread[ch.name]), j + 1)):
                    break
                ang = trial
        hand.curls[ch.name] = tuple(ang)


def report(hand, gun):
    """Where a hand's joints are in the gun's own (u, v, w) cm, for working on a pose (debug only)."""
    if 'debug' not in ARGV:
        return
    inv = np.linalg.inv(gun['M'])
    for ch in CHAINS:
        F = hand.chain_F(ch)
        pts = []
        for k in range(len(ch.joints)):
            Fk = F[min(k, len(F) - 1)] if ch.name != 'thumb' else F[min(max(k - 1, 0), 2)]
            p = hand.world(Fk[:3, :3] @ ch.joints[k] + Fk[:3, 3])
            g = inv[:3, :3] @ p + inv[:3, 3]
            pts.append(f'({-g[1] * 100:.1f},{g[2] * 100:.1f},{g[0] * 100:.1f})')
        log(f'    {hand.side} {ch.name}: ' + ' '.join(pts))
    g = inv[:3, :3] @ hand.wrist + inv[:3, 3]
    log(f'    {hand.side} wrist ({-g[1] * 100:.1f},{g[2] * 100:.1f},{g[0] * 100:.1f})')


def reach_with(hand, trees, name, target, spreads=(-6.0, -3.0, 0.0, 3.0, 6.0)):
    """A finger's pad put on a point (the trigger): of the poses that don't go into the gun, the nearest."""
    ch = next(c for c in CHAINS if c.name == name)
    best = None
    for spread in spreads:
        hand.spread[name] = spread
        for m in np.arange(-10.0, 75.0, 5.0):
            for p in np.arange(0.0, 100.0, 5.0):
                curl = (m, p, 0.6 * p)
                F = finger_fk(ch, curl, spread)
                if hits(trees, *hand.samples(ch, F, 1), squash=0.8):
                    continue
                a, b = ch.joints[3], ch.joints[4]
                F3 = F[3]
                pad = F3[:3, :3] @ (a + (b - a) * 0.55 - np.cross(unit(b - a), ch.lateral) * ch.radii[4]) + F3[:3, 3]
                d = float(np.linalg.norm(hand.world(pad) - target))
                if best is None or d < best[0]:
                    best = (d, spread, curl)
    if best is None:
        log(f'  {hand.side} {name} finds no way to the trigger')
        return
    hand.spread[name] = best[1]
    hand.curls[name] = best[2]
    log(f'  {hand.side} {name} on the trigger, {best[0] * 100:.1f} cm off')


def close_thumb(hand, trees, opp, flex, limits=(60.0, 60.0), step=2.0):
    """The thumb swung across (opp) and bent (flex) at its base, backed off till it's clear, then closed till it
    touches."""
    ch = CHAINS[4]
    # Only the thumb's free part counts (its metacarpal lies in the palm's pad, against the grip anyway), and it
    # backs off only so far: swung far back it would tear the web between thumb and finger.
    free = lambda o, f: not hits(trees, *hand.samples(ch, thumb_fk(ch, o, f, 0.0, 0.0), 1))
    tries = [(opp - 3.0 * k, flex) for k in range(int((opp + 8.0) / 3.0) + 1)]
    tries += [(o, flex + df) for df in (10.0, -10.0, 20.0) for o in (opp, opp * 0.5, 0.0, -8.0)]
    for o, f in tries:
        if free(o, f):
            opp, flex = o, f
            break
    else:
        # Nowhere clear (a thick grip): the swing that goes least far into it.
        def depth(o, f):
            pts, rads = hand.samples(ch, thumb_fk(ch, o, f, 0.0, 0.0), 1)
            total = 0.0
            for p, r in zip(pts, rads):
                for tree in trees:
                    hit = tree.find_nearest(Vector(p), r)
                    if hit[0] is not None:
                        total += r - hit[3]
            return total
        opp, flex = min(((o, f) for o in np.arange(-8.0, 64.0, 4.0) for f in np.arange(-20.0, 40.0, 10.0)),
                        key=lambda of: depth(*of))
        log(f'  {hand.side} thumb has no clear way; least in at swing {opp:.0f}, bend {flex:.0f}')
    c = [0.0, 0.0]
    for j in range(2):
        while c[j] < limits[j]:
            trial = list(c)
            trial[j] += step
            if hits(trees, *hand.samples(ch, thumb_fk(ch, opp, flex, *trial), j + 1)):
                break
            c = trial
    hand.thumb = (opp, flex, c[0], c[1])


# ---------------------------------------------------------------------------------------------------- posing
def arm_mats(side, hand, shoulder_shift=(0.0, 0.0, 0.0), bend=0.45):
    """The arm's bones (clavicle, upper arm, forearm) for a placed hand, worked back from the hand: the forearm
    leaves the wrist along the hand (bent `bend` of the way toward the shoulder, so the wrist stays nearly straight),
    the upper arm aims from the elbow at Ellis's shoulder (moved by shoulder_shift). The body isn't drawn, so the
    shoulder end floats wherever that puts it, out of sight. The forearm takes most of the hand's turn about it."""
    m = (lambda p: p) if side == 'r' else (lambda p: MX @ p)
    # The arms come up from below the frame: they aim at a point a little out from and well below the shoulder.
    low = np.array([-0.04 if side == 'r' else 0.04, 0.0, -0.16])
    clav0, S_anat = m(REST.clavicle), m(REST.shoulder) + low + np.asarray(shoulder_shift)
    W = hand.wrist
    d = unit(-hand.R[:, 0] * (1.0 - bend) + unit(S_anat - W) * bend)
    E = W + d * REST.fore
    S = E + unit(S_anat - E) * REST.upper
    sign = 1.0 if side == 'r' else -1.0
    hinge = np.cross(E - S, W - E) * sign
    if np.linalg.norm(hinge) < 1e-4:
        hinge = REST_BONE[f'upperarm_{side}'][:3, 2]
    hinge = unit(hinge)
    out = {}

    def frame(name, head, tail, z):
        y = unit(tail - head)
        z = unit(z - y * (z @ y))
        x = np.cross(y, z)
        mat = np.eye(4)
        mat[:3, 0], mat[:3, 1], mat[:3, 2], mat[:3, 3] = x, y, z, head
        out[f'{name}_{side}'] = mat

    rest_clav = REST_BONE[f'clavicle_{side}']
    frame('clavicle', clav0, S, rest_clav[:3, 2])
    frame('upperarm', S, E, hinge)
    # the forearm's own turn: part of the hand's twist about it
    y = unit(W - E)
    R_rest, _ = side_rest(side)
    L_rest = REST_BONE[f'lowerarm_{side}'][:3, :3]
    z0 = unit(hinge - y * (hinge @ y))
    L_pose = np.column_stack([np.cross(y, z0), y, z0])
    keep = L_pose @ L_rest.T @ R_rest[:, 1]
    want = hand.R[:, 1]
    keep, want = unit(keep - y * (keep @ y)), unit(want - y * (want @ y))
    twist = math.degrees(math.atan2(np.cross(keep, want) @ y, keep @ want))
    frame('lowerarm', E, W, rot(y, 0.8 * twist) @ z0)
    return out


def hand_mats(hand):
    out = {}
    side = hand.side
    G0 = hand.M() @ np.linalg.inv(hand.rest_M())
    out[f'hand_{side}'] = G0 @ REST_BONE[f'hand_{side}']
    for ch in CHAINS:
        F = hand.chain_F(ch)
        for k, bone in enumerate(ch.bones):
            Fk = F[k]
            G = hand.M() @ Fk @ np.linalg.inv(hand.rest_M())
            out[f'{bone}_{side}'] = G @ REST_BONE[f'{bone}_{side}']
    return out


def depth(bone):
    d = 0
    while bone.parent:
        d += 1
        bone = bone.parent
    return d


def apply_pose(mats):
    pbs = ARM.pose.bones
    for pb in pbs:
        pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()
    levels = {}
    for b in ARM.data.bones:
        levels.setdefault(depth(b), []).append(b.name)
    for lvl in sorted(levels):
        for name in levels[lvl]:
            if name in mats:
                pbs[name].matrix = Matrix([list(r) for r in mats[name]])
        bpy.context.view_layer.update()


def pose(hands, shifts=None):
    """hands: [Hand]; shifts: {side: how far that shoulder is moved}."""
    mats = {}
    for hand in hands:
        shift = (shifts or {}).get(hand.side, (0.0, 0.0, 0.0))
        mats.update(arm_mats(hand.side, hand, shift))
        mats.update(hand_mats(hand))
    apply_pose(mats)


def arms_tree(side):
    """The posed arms' faces on one side, as a tree (for the other hand closing over this one)."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    ev = BODY.evaluated_get(depsgraph)
    mesh = ev.to_mesh()
    rest_x = np.array([v.co.x for v in BODY.data.vertices])
    verts = [ARM.matrix_world @ v.co for v in mesh.vertices]
    polys = [list(p.vertices) for p in mesh.polygons if (rest_x[p.vertices[0]] < 0) == (side == 'r')]
    ev.to_mesh_clear()
    return BVHTree.FromPolygons(verts, polys)


# ----------------------------------------------------------------------------------------------------- guns
def socket_mats(parts, rig=None):
    """Every socket on parts by name (Blender's .001 numbering dropped) as its full matrix (4x4, m) in the gun's own space
    (the rig's, when given: the parts parented under it), with the part it is on. A name on several parts: the latest
    part in assembly order wins, as UWeaponModelComponent::FindSocket (the grip spots' rule, Docs/Work/2026-10-29-
    GripSpots/CONTRACT.md). An empty's -Y is the socket's front in Unreal and its +Z its top (looter_export.socket_entry)."""
    inv = np.linalg.inv(np.array(rig.matrix_world)) if rig is not None else np.eye(4)
    mats, owners = {}, {}
    for part in parts:
        for o in part.children_recursive:
            if o.name.startswith('SOCKET_'):
                key = o.name[7:].split('.')[0]
                mats[key] = inv @ np.array(o.matrix_world)
                owners[key] = part.name
    return mats, owners


def load_gun(name, keep_all=False):
    """A gun script's default gun as a stand-in, under one empty at its origin; its sockets in its own space (sockets:
    points; mats: full matrices, socket_mats). keep_all keeps every part the script built (its `built`), parented under
    the same empty and hidden but for the default's, so the caller can put other parts on with ns['assemble'](choice)
    (locations are the rig's own space) and read them again with socket_mats(parts, rig)."""
    before = set(bpy.data.objects)
    ns = runpy.run_path(os.path.join(lt.REPO, 'Art', 'Models', 'Weapons', name + '.py'))
    shown = ns['assemble'](ns['DEFAULT'])
    every = list(ns.get('built', {}).values()) if keep_all else []
    every += [o for o in shown if o not in every]
    keep = set()
    for o in every:
        keep |= {o} | set(o.children_recursive)
    for o in list(set(bpy.data.objects) - before - keep):
        bpy.data.objects.remove(o)
    bpy.context.view_layer.update()
    sockets = {}
    on_show = set()
    for o in shown:
        on_show |= {o} | set(o.children_recursive)
    for o in keep:
        if o.name.startswith('SOCKET_'):
            key = o.name[7:].split('.')[0]
            if o in on_show:
                sockets.setdefault(key, np.array(o.matrix_world.translation))
        if o.name.startswith(('UCX_', '_')) and o.type == 'MESH':
            o.hide_render = True
    mats, owners = socket_mats(shown)
    rig = bpy.data.objects.new('_Gun_' + name, None)
    bpy.context.scene.collection.objects.link(rig)
    for o in every:
        mw = o.matrix_world.copy()
        o.parent = rig
        o.matrix_world = mw
        if o not in shown:
            o.hide_render = True
    glow = bpy.data.materials.get('GunReticle')
    if glow:
        bsdf = next(n for n in glow.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
        bsdf.inputs['Emission Color'].default_value = (1.0, 0.08, 0.04, 1.0)
        bsdf.inputs['Emission Strength'].default_value = 12.0
    accent = bpy.data.materials.get('GunAccentGlow')
    if accent:
        bsdf = next(n for n in accent.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
        bsdf.inputs['Emission Color'].default_value = (0.75, 0.85, 1.0, 1.0)
        bsdf.inputs['Emission Strength'].default_value = 1.2
    log(f'{name}: {len(shown)} parts; sockets {", ".join(sorted(sockets))}')
    return dict(rig=rig, parts=shown, sockets=sockets, mats=mats, owners=owners, ns=ns, every=every)


def place_gun(gun, grip_target, R=np.eye(3), offset=(0.0, 0.0, 0.0), pivot=None):
    """Puts the gun's pivot (its Grip socket) at grip_target + offset, turned by R about it."""
    pivot = gun['sockets']['Grip'] if pivot is None else pivot
    M = affine(np.eye(3), np.asarray(grip_target) + np.asarray(offset)) @ affine(R, np.zeros(3)) @ \
        affine(np.eye(3), -pivot)
    gun['M'] = M
    gun['rig'].matrix_world = Matrix([list(r) for r in M])
    bpy.context.view_layer.update()


def gun_point(gun, local):
    M = gun['M']
    return M[:3, :3] @ np.asarray(local) + M[:3, 3]


def gun_dir(gun, local):
    return unit(gun['M'][:3, :3] @ np.asarray(local))


def gun_tree(gun, only=None):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    verts, polys = [], []
    for o in gun['parts']:
        if only and not any(k in o.name for k in only):
            continue
        ev = o.evaluated_get(depsgraph)
        mesh = ev.to_mesh()
        base = len(verts)
        verts.extend(o.matrix_world @ v.co for v in mesh.vertices)
        polys.extend([base + i for i in p.vertices] for p in mesh.polygons)
        ev.to_mesh_clear()
    return BVHTree.FromPolygons(verts, polys)


def part_axis(obj, gun):
    """A gun part's long axis (its vertices' main direction), in Blender space, and its middle."""
    co = np.array([obj.matrix_world @ v.co for v in obj.data.vertices])
    c = co.mean(axis=0)
    _, _, vt = np.linalg.svd(co - c, full_matrices=False)
    return unit(vt[0]), c


# ----------------------------------------------------------------------------------------------------- stage
def stage():
    scene = bpy.context.scene
    world = bpy.data.worlds.new('_FPSky')
    world.use_nodes = True
    nodes, links = world.node_tree.nodes, world.node_tree.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputWorld')
    bg = nodes.new('ShaderNodeBackground')
    bg.inputs['Strength'].default_value = 0.95
    coords = nodes.new('ShaderNodeTexCoord')
    norm = nodes.new('ShaderNodeVectorMath')
    norm.operation = 'NORMALIZE'
    links.new(coords.outputs['Generated'], norm.inputs[0])
    split = nodes.new('ShaderNodeSeparateXYZ')
    links.new(norm.outputs['Vector'], split.inputs['Vector'])
    remap = nodes.new('ShaderNodeMapRange')
    remap.inputs['From Min'].default_value = -1.0
    links.new(split.outputs['Z'], remap.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    links.new(remap.outputs['Result'], ramp.inputs['Fac'])
    els = ramp.color_ramp.elements
    sky = [(0.0, 0x6b5a46), (0.47, 0x8a7a62), (0.5, 0xf6c98c), (0.535, 0xecd3a8), (0.6, 0xc3c7c2), (0.7, 0x93acc8),
           (1.0, 0x5679ad)]
    els[0].position, els[0].color = sky[0][0], lt.hex_color(sky[0][1])
    els[1].position, els[1].color = sky[-1][0], lt.hex_color(sky[-1][1])
    for p, c in sky[1:-1]:
        els.new(p).color = lt.hex_color(c)
    links.new(ramp.outputs['Color'], bg.inputs['Color'])
    links.new(bg.outputs['Background'], out.inputs['Surface'])
    scene.world = world
    toward = unit((0.66, 0.42, 0.26))
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy = 4.8
    sun.data.color = (1.0, 0.72, 0.46)
    sun.data.angle = math.radians(2.0)
    sun.rotation_euler = (-Vector(toward)).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    # warm bounce from the sunlit ground ahead
    fill = bpy.data.objects.new('_Bounce', bpy.data.lights.new('_Bounce', 'SUN'))
    fill.data.energy = 0.45
    fill.data.color = (1.0, 0.82, 0.6)
    fill.data.use_shadow = False
    fill.rotation_euler = (-Vector((0.1, -0.5, -1.0))).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(fill)
    # the ground: dry grass and dust, hazing into the distance
    mesh = bpy.data.meshes.new('_Ground')
    s = 3000.0
    mesh.from_pydata([(-s, -s, -EYE_H), (s, -s, -EYE_H), (s, s, -EYE_H), (-s, s, -EYE_H)], [], [(0, 1, 2, 3)])
    ground = bpy.data.objects.new('_Ground', mesh)
    scene.collection.objects.link(ground)
    mat = bpy.data.materials.new('_GroundMat')
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes['Principled BSDF']
    bsdf.inputs['Roughness'].default_value = 1.0
    # the island's dry grass texture, tiled every 2 m, broken up by a large noise
    coords_g = nt.nodes.new('ShaderNodeTexCoord')
    scale_g = nt.nodes.new('ShaderNodeVectorMath')
    scale_g.operation = 'SCALE'
    scale_g.inputs['Scale'].default_value = 0.5
    nt.links.new(coords_g.outputs['Object'], scale_g.inputs[0])
    img = nt.nodes.new('ShaderNodeTexImage')
    grass = lt.texture_path('GroundGrass', 'BC')
    if os.path.exists(grass):
        img.image = bpy.data.images.load(grass, check_existing=True)
    nt.links.new(scale_g.outputs['Vector'], img.inputs['Vector'])
    tex = nt.nodes.new('ShaderNodeTexNoise')
    tex.inputs['Scale'].default_value = 0.08
    tex.inputs['Detail'].default_value = 4.0
    gramp = nt.nodes.new('ShaderNodeMix')
    gramp.data_type = 'RGBA'
    gramp.blend_type = 'MULTIPLY'
    nt.links.new(tex.outputs['Fac'], gramp.inputs['Factor'])
    # late summer: the green grass texture turned to gold
    hsv = nt.nodes.new('ShaderNodeHueSaturation')
    hsv.inputs['Hue'].default_value = 0.455
    hsv.inputs['Saturation'].default_value = 0.8
    hsv.inputs['Value'].default_value = 1.1
    nt.links.new(img.outputs['Color'], hsv.inputs['Color'])
    nt.links.new(hsv.outputs['Color'], gramp.inputs[6])
    gramp.inputs[7].default_value = lt.hex_color(0xdcb478)
    camd = nt.nodes.new('ShaderNodeCameraData')
    haze = nt.nodes.new('ShaderNodeMapRange')
    haze.inputs['From Min'].default_value = 20.0
    haze.inputs['From Max'].default_value = 2200.0
    nt.links.new(camd.outputs['View Distance'], haze.inputs['Value'])
    mixn = nt.nodes.new('ShaderNodeMix')
    mixn.data_type = 'RGBA'
    nt.links.new(haze.outputs['Result'], mixn.inputs['Factor'])
    nt.links.new(gramp.outputs[2], mixn.inputs[6])
    mixn.inputs[7].default_value = lt.hex_color(0xe2cda8)
    nt.links.new(mixn.outputs[2], bsdf.inputs['Base Color'])
    mesh.materials.append(mat)
    # rolling prairie: flat round the player, swelling into low hills toward the horizon (the same grass, hazed)
    rings = np.geomspace(40.0, 2800.0, 70)
    segs = 160
    th = np.linspace(-math.pi, math.pi, segs, endpoint=False)
    rng = np.random.default_rng(4)
    waves = [(rng.uniform(2, 9), rng.uniform(0, 6.28), rng.uniform(0.3, 1.0)) for _ in range(7)]
    verts = []
    for r in rings:
        rise = np.clip((r - 60.0) / 900.0, 0.0, 1.0) ** 1.4
        h = sum(a * np.sin(k * th + ph + 0.002 * r * k) for k, ph, a in waves) / 2.5
        h = (0.55 + 0.45 * h) * 95.0 * rise + 6.0 * rise * np.sin(th * 23.0 + r * 0.01)
        verts += [(math.sin(t) * r, -math.cos(t) * r, -EYE_H + float(z) - 0.05) for t, z in zip(th, h)]
    faces = [(i * segs + j, i * segs + (j + 1) % segs, (i + 1) * segs + (j + 1) % segs, (i + 1) * segs + j)
             for i in range(len(rings) - 1) for j in range(segs)]
    tm = bpy.data.meshes.new('_Prairie')
    tm.from_pydata(verts, [], faces)
    tm.shade_smooth()
    tm.materials.append(mat)
    scene.collection.objects.link(bpy.data.objects.new('_Prairie', tm))


def camera(location, target, lens=None, fov=90.0, focus=None, fstop=None):
    scene = bpy.context.scene
    cam = scene.camera
    if cam is None:
        cam = bpy.data.objects.new('_Cam', bpy.data.cameras.new('_Cam'))
        scene.collection.objects.link(cam)
        scene.camera = cam
    cam.location = Vector(location)
    cam.rotation_euler = (Vector(target) - Vector(location)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.sensor_fit = 'HORIZONTAL'
    if lens:
        cam.data.lens = lens
    else:
        cam.data.angle = math.radians(fov)
    cam.data.clip_start = 0.01
    cam.data.clip_end = 5000.0
    cam.data.dof.use_dof = focus is not None
    if focus is not None:
        cam.data.dof.focus_distance = focus
        cam.data.dof.aperture_fstop = fstop or 4.0
    return cam


def eye_camera(focus=0.55, fstop=1.6):
    return camera((0.0, 0.0, 0.0), (0.0, -1.0, 0.0), fov=90.0, focus=focus, fstop=fstop)


def render(name, resolution=(1600, 900), samples=64):
    scene = bpy.context.scene
    if FAST:
        resolution, samples = (resolution[0] // 2, resolution[1] // 2), 16
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = samples
    scene.eevee.use_shadows = True
    scene.eevee.shadow_ray_count = 2
    scene.eevee.shadow_step_count = 8
    if hasattr(scene.eevee, 'use_gtao'):
        scene.eevee.use_gtao = True
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.view_settings.exposure = 0.15
    scene.use_nodes = True
    tree = scene.node_tree
    tree.nodes.clear()
    layers = tree.nodes.new('CompositorNodeRLayers')
    glare = tree.nodes.new('CompositorNodeGlare')
    glare.glare_type = 'BLOOM'
    for key, value in (('Threshold', 1.6), ('Strength', 0.4), ('Size', 0.5)):
        if key in glare.inputs:
            glare.inputs[key].default_value = value
    comp = tree.nodes.new('CompositorNodeComposite')
    tree.links.new(layers.outputs['Image'], glare.inputs['Image'])
    tree.links.new(glare.outputs['Image'], comp.inputs['Image'])
    path = os.path.join(WORK, name + '.png')
    os.makedirs(WORK, exist_ok=True)
    scene.render.filepath = path
    t = time.time()
    bpy.ops.render.render(write_still=True)
    log(f'rendered {name} in {time.time() - t:.0f} s')
    return path


def debug_views(name, center, dist=0.45):
    """Quick looks round a pose (left, right, above, below the gun), for working on it: Intermediate/FPArms/Sheet."""
    if 'debug' not in ARGV:
        return
    center = np.asarray(center)
    paths = []
    for tag, d in (('L', (1.0, 0.25, 0.35)), ('R', (-1.0, 0.25, 0.3)), ('T', (0.15, 0.35, 1.0)), ('B', (0.3, 0.2, -1.0))):
        camera(center + unit(d) * dist, center, lens=35.0)
        paths.append(render(f'debug_{name}_{tag}', resolution=(960, 640), samples=8))
    eye_camera()


def look_from(name, target, direction, dist, lens=32.0, fstop=4.0):
    """A view from beside the gun (where the eye's view hides the hands), focused on target."""
    target = np.asarray(target)
    at = target + unit(direction) * dist
    camera(at, target, lens=lens, focus=dist, fstop=fstop)
    path = render(name)
    eye_camera()
    return path


def show_gun(gun, visible):
    for o in [gun['rig']] + list(gun['rig'].children_recursive):
        o.hide_render = not visible or o.name.startswith(('UCX_', 'SOCKET_'))


# ----------------------------------------------------------------------------------------------------- shots
LONG_GRIP = ue(50.0, 20.0, -26.0)            # the long guns' hip hold: the grip from the eye
ADS_AIM = ue(22.0, 0.0, 0.0)                  # SOCKET_Aim when aiming down the sights


def bullpup_hands(gun, reload_mag=None):
    """Both hands on the Bullpup as it stands: the right on the grip (index on the trigger), the left on the handguard
    or, mid-reload, on the magazine."""
    up = gun_dir(gun, (0.0, 0.14, 0.99))
    trees = [gun_tree(gun)]
    right = Hand('r', hand_frame('r', up, gun_dir(gun, (-0.35, 0.8, -0.45)), palm=gun_dir(gun, (0.7, -0.7, 0.0)),
                                 palm_weight=1.0), gun_point(gun, gun['sockets']['Grip']), along=1.0, tilt=22.0)
    seat(right, trees)
    right.spread = {'index': 4.0, 'middle': 0.0, 'ring': -2.0, 'pinky': -5.0}
    report(right, gun)
    reach_with(right, trees, 'index', gun_point(gun, (0.0, -0.268, -0.07)))
    close_fingers(right, trees, names=('middle', 'ring', 'pinky'))
    close_thumb(right, trees, opp=28.0, flex=8.0)
    if reload_mag is None:
        fore = gun_point(gun, gun['sockets']['Foregrip'])
        ahead = gun_dir(gun, (0.0, -1.0, 0.0))
        # Under the handguard, thumb along its left side, fingers up its right side and over the top.
        left = Hand('l', hand_frame('l', ahead, gun_dir(gun, (0.55, 0.55, -0.62)), palm=gun_dir(gun, (-0.3, 0.0, 1.0)),
                                    palm_weight=1.5), fore)
    else:
        axis, mid = reload_mag
        left = Hand('l', hand_frame('l', axis, (0.6, 0.45, -0.66), palm=gun_dir(gun, (-1.0, 0.0, 0.0)),
                                    palm_weight=1.0), mid)
    seat(left, trees)
    close_fingers(left, trees)
    close_thumb(left, trees, opp=20.0, flex=0.0)
    for h in (right, left):
        log(f'  {h.side}: ' + ', '.join(f'{k} {tuple(int(a) for a in v)}' for k, v in h.curls.items()) +
            f', thumb {tuple(int(a) for a in h.thumb)}')
    return right, left


def shots_bullpup():
    gun = load_gun('Bullpup')
    show_gun(gun, True)
    eye_camera()
    # 1 the hip
    place_gun(gun, LONG_GRIP)
    r, l = bullpup_hands(gun)
    pose([r, l])
    debug_views('hip', gun_point(gun, gun['sockets']['Grip'] * 0.5 + gun['sockets']['Foregrip'] * 0.5))
    debug_views('hipL', l.world(GRIP_C), dist=0.3)
    if 'debug' in ARGV:
        return
    eye_camera(focus=0.6)
    render('bullpup_hip')
    mid = 0.5 * (r.world(GRIP_C) + l.world(GRIP_C))
    look_from('bullpup_hip_beside', mid, (-1.0, 0.1, 0.42), 0.55)
    # 2 aiming down the sight
    aim = gun['sockets']['Aim']
    place_gun(gun, ADS_AIM, pivot=aim)
    r, l = bullpup_hands(gun)
    pose([r, l])
    debug_views('ads', gun_point(gun, gun['sockets']['Grip'] * 0.5 + gun['sockets']['Foregrip'] * 0.5))
    eye_camera(focus=0.42)
    render('bullpup_ads')
    # 3 mid-reload: the gun brought in and rolled (ReloadMotion's magazine pose), the magazine out of its well
    place_gun(gun, LONG_GRIP, R=ue_rotator(10.0, -14.0, 34.0), offset=ue(-1.0, -6.0, 3.5))
    mag = next(o for o in gun['parts'] if 'Magazine' in o.name)
    mag_axis, mag_mid = part_axis(mag, gun)
    if mag_axis @ gun_dir(gun, (0, 0, 1.0)) < 0:
        mag_axis = -mag_axis
    drop = 0.045
    mag.matrix_world = Matrix.Translation(Vector(-mag_axis * drop)) @ mag.matrix_world
    bpy.context.view_layer.update()
    mag_axis, mag_mid = part_axis(mag, gun)
    if mag_axis @ gun_dir(gun, (0, 0, 1.0)) < 0:
        mag_axis = -mag_axis
    r, l = bullpup_hands(gun, reload_mag=(mag_axis, mag_mid - mag_axis * 0.01))
    pose([r, l])
    debug_views('reload', l.world(GRIP_C), dist=0.35)
    eye_camera(focus=0.5)
    render('bullpup_reload')
    look_from('bullpup_reload_beside', l.world(GRIP_C), (0.85, 0.15, 0.5), 0.45)
    mag.matrix_world = Matrix.Translation(Vector(mag_axis * drop)) @ mag.matrix_world
    show_gun(gun, False)


def shots_drover():
    gun = load_gun('Drover')
    show_gun(gun, True)
    place_gun(gun, ue(46.7, 19.6, -18.3), R=ue_rotator(0.7, -1.2, -8.0))
    grip = next(o for o in gun['parts'] if 'Grip' in o.name)
    axis, _ = part_axis(grip, gun)
    if axis @ gun_dir(gun, (0, 0, 1.0)) < 0:
        axis = -axis
    trees = [gun_tree(gun)]
    right = Hand('r', hand_frame('r', axis, gun_dir(gun, (-0.35, 0.8, -0.45)), palm=gun_dir(gun, (0.7, -0.7, 0.0)),
                                 palm_weight=1.0), gun_point(gun, gun['sockets']['Grip']), tilt=10.0)
    seat(right, trees)
    right.spread = {'index': 3.0, 'middle': 0.0, 'ring': -2.0, 'pinky': -4.0}
    report(right, gun)
    reach_with(right, trees, 'index', gun_point(gun, (0.0, -0.044, -0.05)))
    close_fingers(right, trees, names=('middle', 'ring', 'pinky'))
    close_thumb(right, trees, opp=22.0, flex=4.0)
    pose([right])
    # The support hand wraps the shooting hand from the left: it closes over the right hand's fingers.
    trees_l = trees + [arms_tree('r')]
    left = Hand('l', hand_frame('l', axis, gun_dir(gun, (0.55, 0.65, -0.5)), palm=gun_dir(gun, (-0.8, -0.3, 0.0)),
                                palm_weight=1.0), gun_point(gun, gun['sockets']['Foregrip']))
    seat(left, trees_l)
    close_fingers(left, trees_l)
    close_thumb(left, trees_l, opp=18.0, flex=0.0)
    for h in (right, left):
        log(f'  {h.side}: ' + ', '.join(f'{k} {tuple(int(a) for a in v)}' for k, v in h.curls.items()) +
            f', thumb {tuple(int(a) for a in h.thumb)}')
    pose([right, left])
    debug_views('drover', 0.5 * (right.world(GRIP_C) + left.world(GRIP_C)), dist=0.35)
    eye_camera(focus=0.5)
    render('drover_hip')
    look_from('drover_beside', 0.5 * (right.world(GRIP_C) + left.world(GRIP_C)), (-0.3, -1.0, 0.35), 0.5)
    # the glove up close: the right hand on the Drover, from outside and a little above and ahead
    # (the support hand: its back is the one the low sun lights)
    hand_c = left.world(np.array([4.0, 0.0, 0.5]) * CM)
    back = left.R[:, 2]
    cam_at = hand_c + unit(back * 0.8 + np.array([0.0, 0.0, 0.55]) + np.array([0.0, -0.3, 0.0])) * 0.5
    camera(cam_at, hand_c + np.array([0.0, 0.035, 0.035]), lens=50.0, focus=float(np.linalg.norm(cam_at - hand_c)),
           fstop=5.6)
    render('glove_closeup', resolution=(1600, 1100))
    show_gun(gun, False)


def compose():
    sys.path.insert(0, os.path.join(lt.REPO, 'Art', 'Backlog', 'Creatures'))
    import MinerConcepts as mc
    p = lambda n: os.path.join(WORK, n + '.png')
    tris = sum(len(poly.vertices) - 2 for poly in BODY.data.polygons)
    s = mc.Sheet(width=2000)
    s.title('First-person arms: Ellis\'s gloves and duster')
    s.lines(f'SK_FP_Arms: {tris / 1000:.1f}k triangles, 46 bones on the mannequin\'s names, its root at the eye; two '
            'materials, each with its own\npainted textures (leather and oilskin, 2048). The guns are the gun scripts\' '
            'default parts, placed where the game holds them today.\nPosed by hand for this sheet; the game will pose '
            'the hands by code. Nothing imported yet.', size=34)
    s.heading('From the eye: the game\'s 90 degree view, the game\'s holds')
    s.row([(p('bullpup_hip'), '', 'The Bullpup at the hip. The left arm reaches for the handguard; the\nright hand on '
            'the grip sits just under the screen\'s bottom edge.'),
           (p('bullpup_ads'), '', 'Aiming down the sight: the left gauntlet and hand under the\nhandguard; the right '
            'hand is below the screen, as in the game.')])
    s.row([(p('drover_hip'), '', 'The Drover at its own hip hold, two-handed: the left hand\nwraps the right.'),
           (p('bullpup_reload'), '', 'Mid-reload, the gun rolled in (the game\'s reload pose): the\nBullpup\'s magazine '
            'is behind its grip, so the hand on it is off screen.')])
    s.heading('Beside the gun: the same poses, seen from outside')
    s.row([(p('bullpup_hip_beside'), '', 'The hip hold: the right hand round the grip, its finger\non the trigger; '
            'the left over the handguard.'),
           (p('bullpup_reload_beside'), '', 'Mid-reload: the support hand draws the magazine\nout of its well.')])
    s.heading('The glove up close')
    s.image(p('glove_closeup'), 'Tan roper leather: seams up the sides of every finger and round the tips, three '
            'stitched points on the back, a keystone thumb,\nscuffed knuckles and fingertips, a grip-polished palm; the '
            'flared gauntlet\'s pale stitched hems, tooled border and brass concho.\nThe duster\'s oilskin in Ellis\'s '
            'coat colour: waxed, worn chalky on the folds, crackled, with felled seams.')
    jpg = os.path.join(WORK, 'fp_arms_sheet.jpg')
    s.render(jpg)
    img = bpy.data.images.load(jpg)
    _ = img.pixels[0]                  # read the pixels now, before the image is pointed at its new file
    img.filepath_raw = SHEET
    img.file_format = 'PNG'
    os.makedirs(os.path.dirname(SHEET), exist_ok=True)
    img.save()
    log(f'sheet {SHEET} ({img.size[0]} x {img.size[1]})')


stage()
want = [a for a in ARGV if a in ('bullpup', 'drover', 'sheet')] or ['bullpup', 'drover', 'sheet']
if 'bullpup' in want:
    shots_bullpup()
if 'drover' in want:
    shots_drover()
if 'sheet' in want:
    compose()
