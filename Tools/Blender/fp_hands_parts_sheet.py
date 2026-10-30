"""The grip spots' look sheet (Docs/Work/2026-10-29-GripSpots/CONTRACT.md): for one gun and every value of each slot the
hands hold, the gun assembled (its default parts with that one swapped in, as fp_arms_sheet.py / fp_arms_heroes_sheet.py
build them) and Ellis's first-person arms (SK_FP_Arms) posed on it, from the eye and from beside the gun.
- Hands from the part's grip spots (SOCKET_<Hold|Rack|Reload>_<L|R>_<Pose>, looter_guns Gun.hand_mark; within a role and
  side the latest part in assembly order wins): the palm's grip point 2 cm (GripOffPalmSkin) through the palm from the
  spot, the knuckles along the spot's +X, the back of the hand along its +Z; the palm then seated as the game seats it
  (gap 0.15 cm, a full move up to 2.5 cm, fading out to none at 5); the fingers and thumb from the spot's pose in
  Saved/FPHands/poses.json (FPHandPoses.py writes it; joint curls from straight, as FHandFingers) or a plain closed grip
  without it, then closed or opened till they touch the gun (TouchShare of the way); the trigger hand's index searched
  onto SOCKET_Trigger (else the kind's FGunHands::TriggerAt).
- Without a spot, the hand comes from fp_arms_sheet.py's poser as it is today (the Bullpup's bullpup_hands; the
  heroes sheet's shotgun and revolver posers for the others), so the sheet runs on unmarked guns.
- Sizes as the game draws them: the gun at UWeaponModelComponent::ScaleFor x the first-person GunScale 1.2
  (FirstPersonHandsFraming), the arms at 0.88 (x the kind's ArmsScale), the hold WeaponHipHold.cpp's (the revolver's
  create_revolver.py's) plus the kind's FPHandsFraming hip nudge and turn, turned about the grip. As the heroes sheet does,
  the whole view is scaled about the eye by 1 / the arms' scale instead of shrinking the arms (the eye can't tell).
- A slot held for reloading (the magazine, the cylinder) shows the reload: the gun rolled in as fp_arms_sheet.py's
  reload shot, the part drawn 4.5 cm out, the support hand on its Reload_L spot; unless the part holds a Hold spot.
Each tile is the eye's view (90 degrees, 16:9) and one from beside the gun, side by side, into
Saved/ArtPreviews/Weapons/Hands/<Gun>_<Slot>_<Key>.png (with LOOTER_BULLPUP_TRIPO=0, or only some sets, in a Scripted /
Tripo<sets> folder below it), and a contact sheet <Gun>_sheet.png beside them. Each tile prints (FPHANDSPARTS ...) each
hand's spot or fallback, palm gap (cm, + = air before seating), the fingers touching the gun and the trigger miss.

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Tools\\Blender\\fp_hands_parts_sheet.py -ScriptArgs Bullpup
        [,Body][,Magazine][,Body_Standard ...][,fast][,nosheet]     slots or slot_key tiles only; quick renders
    $env:LOOTER_BULLPUP_TRIPO='0'; ... the scripted Bullpup parts instead of the Tripo ones
fp_arms_sheet.py is run only up to its shots, as fp_arms_heroes_sheet.py runs it; keep-textures is passed to FPArms.py
(the arms' textures are never rewritten here). Nothing is exported or imported.
"""
import csv
import json
import math
import os
import re
import sys
import time

import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_textures as lt

if '--' not in sys.argv:
    sys.argv.append('--')
if 'keep-textures' not in sys.argv[sys.argv.index('--') + 1:]:
    sys.argv.append('keep-textures')
ARGV = [a for arg in sys.argv[sys.argv.index('--') + 1:] for a in arg.split(',') if a]
T0 = time.time()

# Per gun: kind, ScaleFor, the hip hold (camera cm ahead/right/up; FRotator pitch/yaw/roll), the kind's FPHandsFraming
# nudge and turn (MakeDefaults), its arms' scale on top of 0.88, FGunHands::TriggerAt (u, v), the slots held for the hold
# and for the reload, and the fallback poser.
GUNS = {
    'Bullpup': dict(kind='Rifle', scale=1.1, hip=((43.0, 23.0, -27.5), (0.0, 0.0, 0.0)),
                    frame=((0.0, -3.0, 3.0), (5.0, 0.0, 0.0)), arms=1.0, trigger=(28.0, -6.6),
                    hold=('Body',), reload=('Magazine',), poser='bullpup'),
    'Ranchhand': dict(kind='Shotgun', scale=1.1, hip=((30.0, 22.5, -23.0), (0.0, -3.0, -5.0)),
                      frame=((0.0, 0.0, -1.5), (0.0, 0.0, 0.0)), arms=1.0, trigger=(3.9, -5.2),
                      hold=('Body', 'Pump', 'Stock'), reload=('Magazine',), poser='long', grip_up=(0.0, -0.42, 0.9)),
    'Drover': dict(kind='Revolver', scale=1.2, hip=((50.0, 22.0, -21.0), (0.7, -1.2, -8.0)),
                   frame=((0.0, -5.0, 1.0), (0.0, 0.0, 6.0)), arms=1.0, trigger=(5.1, -4.8),
                   hold=('Body', 'Grip'), reload=('Cylinder',), poser='revolver'),
    'Rustler': dict(kind='SMG', scale=1.1, hip=((44.0, 22.0, -22.5), (0.0, 0.0, 0.0)),
                    frame=((0.0, 0.0, -1.5), (0.0, 0.0, 0.0)), arms=1.0, trigger=(12.5, -5.2),
                    hold=('Body', 'Magazine'), reload=(), poser='long', grip_up=(0.0, -0.3, 0.95)),
    'Longhorn': dict(kind='Sniper', scale=1.1, hip=((34.0, 22.0, -22.0), (0.0, -1.5, 0.0)),
                     frame=((0.0, 0.0, -1.5), (2.0, 0.0, 0.0)), arms=0.9, trigger=(29.1, -5.6),
                     hold=('Body', 'Bolt'), reload=('Magazine',), poser='long', grip_up=(0.0, -0.27, 0.96)),
    'Thresher': dict(kind='LMG', scale=1.1, hip=((40.0, 25.0, -25.0), (0.0, -1.0, -2.0)),
                     frame=((-2.0, 0.0, -3.0), (4.0, 0.0, 0.0)), arms=0.9, trigger=(12.8, -5.6),
                     hold=('Body', 'Magazine'), reload=(), poser='long', grip_up=(0.0, -0.3, 0.95)),
    'Powderkeg': dict(kind='Launcher', scale=1.1, hip=((28.0, 24.0, -20.0), (0.0, -2.0, 0.0)),
                      frame=((9.0, 2.0, -2.0), (0.0, 0.0, 0.0)), arms=1.0, trigger=(55.4, -6.2),
                      hold=('Body',), reload=('Magazine',), poser='long', grip_up=(0.0, -0.19, 0.98)),
}
GUN = next((a for a in ARGV if a in GUNS), 'Bullpup')
CFG = GUNS[GUN]


def game_hip_hold(kind):
    """WeaponHipHold.cpp's hold for kind, read from the source (it is retuned often), or None."""
    path = os.path.join(lt.REPO, 'Source', 'AI_Looter_Shooter', 'Weapons', 'WeaponHipHold.cpp')
    try:
        with open(path, encoding='utf-8') as f:
            text = f.read()
    except OSError:
        return None
    num = r'(-?[\d.]+)f?'
    m = re.search(r'\{\s*EWeaponKind::' + kind + r',\s*FVector\(' + r',\s*'.join([num] * 3) +
                  r'\),\s*(FRotator::ZeroRotator|FRotator\(' + r',\s*'.join([num] * 3) + r'\))', text)
    if not m:
        return None
    turn = (0.0, 0.0, 0.0) if m.group(4).endswith('ZeroRotator') else tuple(float(m.group(k)) for k in (5, 6, 7))
    return tuple(float(m.group(k)) for k in (1, 2, 3)), turn


_HIP = game_hip_hold(CFG['kind'])
if _HIP is not None:
    CFG['hip'] = _HIP
FP_GUN_SCALE = 1.2                      # Looter.FPHands.GunScale
ARMS = 0.88 * CFG['arms']               # UFirstPersonHandsComponent's ArmsScale x the kind's
GUN_DRAWN = CFG['scale'] * FP_GUN_SCALE / ARMS   # the gun in the arms' space
GRIP_OFF_PALM_SKIN = 0.02               # FirstPersonHands::GripOffPalmSkin (hand cm, here m)
SEAT_GAP, SEAT_FULL, SEAT_FADE = 0.0015, 0.025, 0.05
THUMB_FLEX = 80.0                       # Looter.Hands.ThumbFlexToFingers
CUP_WRIST_BEND = 50.0                   # Looter.Hands.CupWristBend (degrees)
END_SHARE = 0.6                         # EndJointRestShare
REACHED = 1.2                           # cm: a trigger pad this near counts as on it (a finger pad is ~1.8 across)
POSES_JSON = os.path.join(lt.REPO, 'Saved', 'FPHands', 'poses.json')
TRIPO_ENV = os.environ.get('LOOTER_BULLPUP_TRIPO', '')
SUB = '' if GUN != 'Bullpup' or TRIPO_ENV == '' else ('Scripted' if TRIPO_ENV == '0' else
                                                        'Tripo' + TRIPO_ENV.replace(',', '_'))
OUT = os.path.join(lt.PREVIEW_DIR, 'Weapons', 'Hands', SUB)
WORK = os.path.join(lt.REPO, 'Intermediate', 'FPHandsParts', GUN + ('_' + SUB if SUB else ''))
MARK = re.compile(r'^(Hold|Rack|Reload)_([LR])_([A-Za-z0-9]+)$')


def log(message):
    print(f'FPHANDSPARTS [{time.time() - T0:6.1f}s] {message}', flush=True)


# fp_arms_sheet.py's poser, staging and rendering (its source up to its shots, as fp_arms_heroes_sheet.py runs it)
_sheet_py = os.path.join(lt.REPO, 'Tools', 'Blender', 'fp_arms_sheet.py')
with open(_sheet_py, encoding='utf-8') as _f:
    _src = _f.read()
if '\nstage()\nwant = ' not in _src:
    raise RuntimeError('fp_arms_sheet.py changed: update fp_hands_parts_sheet.py where it cuts it')
S = {'__name__': 'FPSheetDefs', '__file__': _sheet_py}
exec(compile(_src[:_src.find('\nstage()\nwant = ')], _sheet_py, 'exec'), S)
S['WORK'] = WORK
ue, ue_rotator, affine, unit, about, rot = S['ue'], S['ue_rotator'], S['affine'], S['unit'], S['about'], S['rot']
Hand, hand_frame, finger_fk, hits, CHAINS = S['Hand'], S['hand_frame'], S['finger_fk'], S['hits'], S['CHAINS']
gun_point, gun_dir, gun_tree, part_axis = S['gun_point'], S['gun_dir'], S['gun_tree'], S['part_axis']
GRIP_C, CM, PALM_POINTS = S['GRIP_C'], S['CM'], S['PALM_POINTS']
CHAIN = {ch.name: ch for ch in CHAINS}
PALM, FINGERS = np.array([0.0, 0.0, -1.0]), np.array([1.0, 0.0, 0.0])   # hand frame: the palm's facing, the knuckles'


def angle_about(a, b, axis):
    a, b = a - axis * (a @ axis), b - axis * (b @ axis)
    return math.degrees(math.atan2(float(np.cross(a, b) @ axis), float(a @ b)))


# The game's curls are from straight (FFirstPersonHandsRig: the rest bend measured off the bones); the sheet's finger_fk
# turns from the rest pose about the same axes (a finger's lateral is the game's Along x Palm), so a curl is less its rest.
def finger_rest(ch):
    J = ch.joints
    r1 = angle_about(J[2] - J[1], J[3] - J[2], ch.lateral)
    return (angle_about(J[1] - J[0], J[2] - J[1], ch.lateral), r1, r1 * END_SHARE)


def thumb_rig():
    """The game's thumb (FirstPersonHandsFingers.cpp): the base swings about Along x Palm from where it rests, the outer
    joints bend toward a way 80 degrees from the palm toward the fingers, the spread about Along x Fingers."""
    J = CHAIN['thumb'].joints
    a0, a1 = unit(J[1] - J[0]), unit(J[2] - J[1])
    bend = PALM * math.cos(math.radians(THUMB_FLEX)) + FINGERS * math.sin(math.radians(THUMB_FLEX))
    b1 = unit(bend - a1 * (bend @ a1))
    ax1 = unit(np.cross(a1, b1))
    rest1 = angle_about(a0, a1, ax1)
    return dict(ax=(unit(np.cross(a0, PALM)), ax1, ax1), rest=(0.0, rest1, rest1 * END_SHARE),
                spread=unit(np.cross(a0, FINGERS)))


THUMB = thumb_rig()
FINGER_REST = {ch.name: finger_rest(ch) for ch in CHAINS[:4]}


def game_thumb_F(ch, swing, spread, c1, c2):
    J, ax, rest = ch.joints, THUMB['ax'], THUMB['rest']
    F1 = about(J[0], THUMB['spread'], spread) @ about(J[0], ax[0], swing - rest[0])
    F2 = F1 @ about(J[1], ax[1], c1 - rest[1])
    return [F1, F2, F2 @ about(J[2], ax[2], c2 - rest[2])]


class MarkHand(Hand):
    """A hand placed from a grip spot: its thumb on the game's thumb joints (game_thumb: swing, spread, middle, end)."""
    game_thumb = None

    def chain_F(self, ch, curl=None, thumb=None):
        if ch.name == 'thumb' and self.game_thumb is not None and thumb is None:
            return game_thumb_F(ch, *self.game_thumb)
        return Hand.chain_F(self, ch, curl, thumb)


def load_poses():
    if not os.path.exists(POSES_JSON):
        log(f'no {POSES_JSON}: marked hands get a plain closed grip')
        return {}
    with open(POSES_JSON, encoding='utf-8') as f:
        data = json.load(f)
    log(f'poses: {", ".join(data.get("poses", {}))}')
    return data.get('poses', {})


POSES = load_poses()


# ----------------------------------------------------------------------------------------------------- hands
def palm_gap(hand, trees, reach=0.05):
    """How far the palm is off the gun along its facing (m; + = air, - = in), from the palm's points; None: nothing."""
    n = hand.R @ PALM
    best = None
    for p in PALM_POINTS:
        start = hand.world(p) - n * reach
        for tree in trees:
            hit = tree.ray_cast(Vector(start), Vector(n), reach * 2.0)
            if hit[0] is not None:
                best = hit[3] - reach if best is None else min(best, hit[3] - reach)
    return best


def seat_as_game(hand, gap):
    if gap is None:
        return
    move = gap - SEAT_GAP
    k = 1.0 if abs(move) <= SEAT_FULL else max(0.0, (SEAT_FADE - abs(move)) / (SEAT_FADE - SEAT_FULL))
    hand.wrist = hand.wrist + hand.R @ PALM * (move * k)


def touch_close(hand, trees, names, share, step=2.0, limits=(85.0, 100.0, 75.0)):
    """Each finger from its pose closed till it touches (all joints together, then each), or opened till it is free when
    the pose puts it into the gun; share of the way (the pose's TouchShare)."""
    for name in names:
        ch = CHAIN[name]
        start = list(hand.curls[name])
        test = lambda a, first=1: hits(trees, *hand.samples(ch, finger_fk(ch, a, hand.spread[name]), first))
        ang = list(start)
        if test(ang):
            for k in range(1, 26):
                trial = [s - step * k * f for s, f in zip(start, (1.0, 1.15, 0.8))]
                if not test(trial):
                    ang = trial
                    break
        else:
            while all(a < m for a, m in zip(ang, limits)):
                trial = [ang[0] + step, ang[1] + step * 1.15, ang[2] + step * 0.8]
                if test(trial):
                    break
                ang = trial
            for j in range(3):
                while ang[j] < limits[j]:
                    trial = list(ang)
                    trial[j] += step
                    if test(trial, j + 1):
                        break
                    ang = trial
        hand.curls[name] = tuple(s + share * (a - s) for s, a in zip(start, ang))


def touch_thumb(hand, trees, share, step=2.0, limit=85.0):
    """The game's thumb: base swing and spread as the pose has them (backed off when that is in the gun), the outer
    joints closed till they touch; its metacarpal doesn't count."""
    ch = CHAIN['thumb']
    t = list(hand.game_thumb)
    test = lambda v, first=1: hits(trees, *hand.samples(ch, game_thumb_F(ch, *v), first))
    if test(t):
        for k in range(1, 16):
            trial = [t[0] - 3.0 * k, t[1], t[2] * 0.5, t[3] * 0.5]
            if not test(trial):
                t = trial
                break
    start = list(t)
    for j in (2, 3):
        while t[j] < limit:
            trial = list(t)
            trial[j] += step
            if test(trial, j - 1):
                break
            t = trial
    hand.game_thumb = tuple(s + share * (a - s) for s, a in zip(start, t))


def index_pad(hand):
    ch = CHAIN['index']
    F3 = finger_fk(ch, hand.curls['index'], hand.spread['index'])[3]
    a, b = ch.joints[3], ch.joints[4]
    pad = F3[:3, :3] @ (a + (b - a) * 0.55 - np.cross(unit(b - a), ch.lateral) * ch.radii[4]) + F3[:3, 3]
    return hand.world(pad)


def touching(hand, trees, slack=0.003):
    out = []
    for ch in CHAINS:
        pts, rads = hand.samples(ch, hand.chain_F(ch), 1)
        if any(tree.find_nearest(Vector(p), r + slack)[0] is not None for p, r in zip(pts, rads) for tree in trees):
            out.append(ch.name)
    return out


def marked_hand(side, mark, gun, trees, trigger_at):
    """A hand on a grip spot (its matrix in the gun's space), posed from its pose and closed onto the gun."""
    M = gun['M'] @ mark['M']
    f = unit(-M[:3, 1])                       # the socket's +X: the knuckles' way
    z = M[:3, 2] - f * (M[:3, 2] @ f)         # its +Z: out of the part toward the palm (the back of the hand's way)
    z = unit(z)
    y = np.cross(z, f) if side == 'r' else np.cross(f, z)   # the left hand's frame is the mirror (side_rest)
    R = np.column_stack([f, y, z])
    hold = M[:3, 3] - z * GRIP_OFF_PALM_SKIN  # the grip point 2 cm through the palm, into the part
    hand = MarkHand(side, R, hold)
    # Seated on the spot's own part only: a neighbour (a wide drum beside the grip) would stop the palm's rays first.
    hand.own = [gun_tree(gun, only=[mark['part']])]
    gap = palm_gap(hand, hand.own)
    seat_as_game(hand, gap)
    pose = POSES.get(mark['pose'])
    if pose is None:
        log(f'  {side}: pose {mark["pose"]} not in poses.json: a plain closed grip')
    curl = pose['Curl'] if pose else [[20.0, 30.0, 20.0]] + [[30.0, 45.0, 25.0]] * 4
    spread = pose['Spread'] if pose else [0.0] * 5
    share = float(pose.get('TouchShare', 1.0)) if pose else 1.0
    for k, name in enumerate(('index', 'middle', 'ring', 'pinky')):
        rest = FINGER_REST[name]
        hand.curls[name] = tuple(c - r for c, r in zip(curl[k + 1], rest))
        hand.spread[name] = float(spread[k + 1])
    hand.game_thumb = (float(curl[0][0]), float(spread[0]), float(curl[0][1]), float(curl[0][2]))
    hand.behind = bool(pose.get('bForearmBehindKnuckles')) if pose else False
    on_trigger = side == 'r' and mark['role'] == 'Hold' and trigger_at is not None
    if on_trigger:
        S['reach_with'](hand, trees, 'index', trigger_at)
    touch_close(hand, trees, ('middle', 'ring', 'pinky') if on_trigger else ('index', 'middle', 'ring', 'pinky'), share)
    touch_thumb(hand, trees, share)
    hand.note = f'{mark["name"]} on {mark["part"]}; palm gap {"none" if gap is None else f"{gap * 100:+.1f}"}'
    return hand


# ------------------------------------------------------------------------------------------------- fallbacks
def long_fallback(gun, trees, trigger_at, reload_mag):
    """fp_arms_heroes_sheet.py's shotgun poser: the trigger hand round the grip, the support under SOCKET_Foregrip."""
    right = Hand('r', hand_frame('r', gun_dir(gun, CFG.get('grip_up', (0.0, 0.14, 0.99))),
                                 gun_dir(gun, (-0.35, 0.8, -0.45)), palm=gun_dir(gun, (0.7, -0.7, 0.0)), palm_weight=1.0),
                 gun_point(gun, gun['sockets']['Grip']), along=1.0, tilt=18.0)
    S['seat'](right, trees)
    right.spread = {'index': 4.0, 'middle': 0.0, 'ring': -2.0, 'pinky': -5.0}
    S['reach_with'](right, trees, 'index', trigger_at)
    S['close_fingers'](right, trees, names=('middle', 'ring', 'pinky'))
    S['close_thumb'](right, trees, opp=26.0, flex=6.0)
    if reload_mag is not None:
        axis, mid = reload_mag
        left = Hand('l', hand_frame('l', axis, (0.6, 0.45, -0.66), palm=gun_dir(gun, (-1.0, 0.0, 0.0))), mid)
    else:
        left = Hand('l', hand_frame('l', gun_dir(gun, (0.0, -1.0, 0.0)), gun_dir(gun, (0.5, 0.5, -0.7)),
                                    palm=gun_dir(gun, (-0.45, 0.0, 1.0)), palm_weight=1.5),
                    gun_point(gun, gun['sockets'].get('Foregrip', gun['sockets']['Grip'])))
    S['seat'](left, trees)
    S['close_fingers'](left, trees)
    S['close_thumb'](left, trees, opp=14.0, flex=0.0)
    return right, left


def revolver_fallback(gun, trees, trigger_at, reload_mag):
    """fp_arms_heroes_sheet.py's revolver poser: the support hand wraps the shooting hand."""
    grip = next(o for o in gun['parts'] if 'Grip' in o.name)
    axis, _ = part_axis(grip, gun)
    axis = axis if axis @ gun_dir(gun, (0, 0, 1.0)) >= 0 else -axis
    right = Hand('r', hand_frame('r', axis, gun_dir(gun, (-0.35, 0.8, -0.45)), palm=gun_dir(gun, (0.7, -0.7, 0.0))),
                 gun_point(gun, gun['sockets']['Grip']), tilt=10.0)
    S['seat'](right, trees)
    right.spread = {'index': 3.0, 'middle': 0.0, 'ring': -2.0, 'pinky': -4.0}
    S['reach_with'](right, trees, 'index', trigger_at)
    S['close_fingers'](right, trees, names=('middle', 'ring', 'pinky'))
    S['close_thumb'](right, trees, opp=22.0, flex=4.0)
    if reload_mag is not None:
        return right, long_fallback(gun, trees, trigger_at, reload_mag)[1]
    pose_arms([right])
    trees_l = trees + [S['arms_tree']('r')]
    left = Hand('l', hand_frame('l', axis, gun_dir(gun, (0.55, 0.65, -0.5)), palm=gun_dir(gun, (-0.8, -0.3, 0.0))),
                gun_point(gun, gun['sockets']['Foregrip']))
    S['seat'](left, trees_l)
    S['close_fingers'](left, trees_l)
    S['close_thumb'](left, trees_l, opp=18.0, flex=0.0)
    return right, left


def fallback(gun, trees, trigger_at, reload_mag):
    if CFG['poser'] == 'bullpup':
        return S['bullpup_hands'](gun, reload_mag=reload_mag)
    if CFG['poser'] == 'revolver':
        return revolver_fallback(gun, trees, trigger_at, reload_mag)
    return long_fallback(gun, trees, trigger_at, reload_mag)


# ----------------------------------------------------------------------------------------------------- the gun
def pose_arms(hands):
    """fp_arms_sheet.py's pose(); a pose whose forearm runs behind the knuckles (the cup) gets the game's rule instead
    (FirstPersonHandsMarks.cpp: back from the knuckles, the wrist bent CUP_WRIST_BEND toward the back of the hand)."""
    mats = {}
    for h in hands:
        arm = h
        if getattr(h, 'behind', False):
            bend = math.radians(CUP_WRIST_BEND)
            way = -h.R[:, 0] * math.cos(bend) + h.R[:, 2] * math.sin(bend)
            arm = type('Arm', (), {})()     # arm_mats reads the forearm's way off R's first column (and R's second)
            arm.wrist, arm.side = h.wrist, h.side
            arm.R = np.column_stack([-way, h.R[:, 1], h.R[:, 2]])
        mats.update(S['arm_mats'](h.side, arm, bend=0.0 if arm is not h else 0.45))
        mats.update(S['hand_mats'](h))
    S['apply_pose'](mats)


def place(gun, reload=False):
    """The gun at its hip hold (plus the kind's framing), turned about its grip and drawn at its first-person size, in
    the arms' space (the view scaled about the eye by 1 / ARMS); mid-reload rolled in as fp_arms_sheet.py's reload."""
    (x, y, z), (p, yw, r) = CFG['hip']
    (nx, ny, nz), (fp, fy, fr) = CFG['frame']
    target = ue(x + nx, y + ny, z + nz) / ARMS
    R = ue_rotator(p + fp, yw + fy, r + fr)
    if reload:
        R = ue_rotator(10.0, -14.0, 34.0) @ R
        target = target + ue(-1.0, -6.0, 3.5) / ARMS
    M = affine(np.eye(3), target) @ affine(R, np.zeros(3)) @ affine(np.eye(3) * GUN_DRAWN, np.zeros(3)) @ \
        affine(np.eye(3), -gun['sockets']['Grip'])
    gun['M'] = M
    gun['rig'].matrix_world = Matrix([list(row) for row in M])
    bpy.context.view_layer.update()


def assemble(gun, choice):
    """The gun with choice's parts on (the others hidden); its sockets read again."""
    shown = gun['ns']['assemble'](choice)
    on = set()
    for o in shown:
        on |= {o} | set(o.children_recursive)
    for o in gun['every']:
        for x in [o] + list(o.children_recursive):
            x.hide_render = x not in on or x.name.startswith(('UCX_', 'SOCKET_', '_'))
    gun['parts'] = shown
    read_sockets(gun)
    return shown


def read_sockets(gun):
    bpy.context.view_layer.update()
    gun['mats'], gun['owners'] = S['socket_mats'](gun['parts'], gun['rig'])
    gun['sockets'] = {k: M[:3, 3].copy() for k, M in gun['mats'].items()}


def hand_marks(gun):
    """{(role, side): spot}: of the spots for one role and side, the latest part's (assembly order)."""
    order = {o.name: i for i, o in enumerate(gun['parts'])}
    marks = {}
    for name, M in gun['mats'].items():
        m = MARK.match(name)
        if not m:
            continue
        role, side, pose = m.groups()
        owner = gun['owners'][name]
        spot = dict(M=M, role=role, pose=pose, name=name, part=owner, order=order.get(owner, -1))
        if (role, side) not in marks or spot['order'] >= marks[(role, side)]['order']:
            marks[(role, side)] = spot
    return marks


def draw_out(gun, part):
    """The reload part drawn out of its well (the Drover's cylinder swung out); returns its axis and the hand's spot."""
    ns = gun['ns']
    if 'swing_out' in ns and 'Cylinder' in part.name:
        ns['swing_out'](part, gun['parts'][0])
    else:
        axis, _ = part_axis(part, gun)
        axis = axis if axis @ gun_dir(gun, (0, 0, 1.0)) >= 0 else -axis
        part.matrix_world = Matrix.Translation(Vector(-axis * 0.045 * GUN_DRAWN)) @ part.matrix_world
    bpy.context.view_layer.update()
    read_sockets(gun)
    axis, mid = part_axis(part, gun)
    axis = axis if axis @ gun_dir(gun, (0, 0, 1.0)) >= 0 else -axis
    return axis, mid - axis * 0.01 * GUN_DRAWN


# ----------------------------------------------------------------------------------------------------- tiles
def tile(gun, slot, key, model, role):
    ns = gun['ns']
    part_name = f'{slot}_{model}'
    assemble(gun, dict(ns['DEFAULT'], **{slot: model}))
    part = ns['built'][part_name]
    marks = hand_marks(gun)
    reload = role == 'Reload' and not any(m['role'] == 'Hold' and m['part'] == part.name for m in marks.values())
    place(gun, reload)
    reload_mag = draw_out(gun, part) if reload else None
    marks = hand_marks(gun)
    trees = [gun_tree(gun)]
    trig = gun['mats'].get('Trigger')
    u, v = CFG['trigger']
    trigger_at = gun_point(gun, trig[:3, 3] if trig is not None else np.array([0.0, -u, v]) * CM)
    rmark = marks.get(('Hold', 'R'))
    lmark = marks.get(('Reload', 'L')) if reload else (marks.get(('Hold', 'L')) or marks.get(('Rack', 'L')))
    wears = f' (wears {model})' if model != key else ''
    log(f'{slot} {key}{wears}: {"reload" if reload else "hold"}; spots '
        f'{", ".join(m["name"] for m in marks.values()) or "none"}')
    fb = fallback(gun, trees, trigger_at, reload_mag) if rmark is None or lmark is None else (None, None)
    right = marked_hand('r', rmark, gun, trees, trigger_at) if rmark else fb[0]
    left = marked_hand('l', lmark, gun, trees, None) if lmark else fb[1]
    for h, m in ((right, rmark), (left, lmark)):
        if m is None:
            h.note = 'no spot: today\'s sheet poser'
    pose_arms([right, left])
    stats = []
    for h in (right, left):
        gap = palm_gap(h, getattr(h, 'own', trees))   # a spot's hand: against its own part
        touch = touching(h, trees)
        seated = 'none' if gap is None else f'{gap * 100:+.1f}'
        stats.append(f'{h.side.upper()} {h.note}; seated {seated}; touching {", ".join(touch) or "nothing"}')
    miss = float(np.linalg.norm(index_pad(right) - trigger_at)) * 100.0
    stats.append(f'trigger {"reached" if miss <= REACHED else "missed"} ({miss:.1f} cm off, '
                 f'{"SOCKET_Trigger" if trig is not None else "TriggerAt"})')
    for line in stats:
        log('  ' + line)
    name = f'{GUN}_{slot}_{key}'
    S['eye_camera'](focus=float(np.linalg.norm(right.world(GRIP_C))), fstop=2.8)
    paths = [S['render'](name + '_eye', resolution=(1280, 720), samples=48)]
    target = 0.5 * (right.world(GRIP_C) + left.world(GRIP_C))
    way = unit(np.array((0.85, 0.15, 0.5) if reload else (-1.0, -0.3, -0.35)))
    S['camera'](target + way * 0.5, target, lens=35.0, focus=0.5, fstop=5.6)
    paths.append(S['render'](name + '_side', resolution=(1280, 720), samples=48))
    S['eye_camera']()
    out = os.path.join(OUT, name + '.png')
    side_by_side(paths, out)
    tripo = ' (Tripo)' if part_name in ns.get('TRIPO', {}) else ''
    words = f'{slot} {key}{tripo}{wears}; {"reload" if reload else "hold"}'
    return out, words, stats


def pixels(path):
    image = bpy.data.images.load(path, check_existing=False)
    image.colorspace_settings.name = 'Non-Color'
    w, h = image.size
    px = np.empty(w * h * 4, np.float32)
    image.pixels.foreach_get(px)
    bpy.data.images.remove(image)
    return px.reshape(h, w, 4)[::-1, :, :3]


def side_by_side(paths, out, gap=8):
    images = [pixels(p) for p in paths]
    h = max(i.shape[0] for i in images)
    sheet = np.full((h, sum(i.shape[1] for i in images) + gap * (len(images) - 1), 3), 0.08, np.float32)
    x = 0
    for i in images:
        sheet[:i.shape[0], x:x + i.shape[1]] = i
        x += i.shape[1] + gap
    os.makedirs(os.path.dirname(out), exist_ok=True)
    lt.write_png(out, lt.to8(np.clip(sheet, 0.0, 1.0)))
    log(f'tile {out}')


def contact_sheet(done):
    sys.path.insert(0, os.path.join(lt.REPO, 'Art', 'Backlog', 'Creatures'))
    import MinerConcepts as mc
    parts = 'scripted' if TRIPO_ENV == '0' else 'Tripo where made' + (f' (sets {TRIPO_ENV})' if TRIPO_ENV else '')
    s = mc.Sheet(width=2400)
    s.title(f'Grip spots: the {GUN}\'s hands, part by part')
    s.lines(f'Each tile: the eye\'s 90 degree view at 16:9 and a view from beside the gun. The gun at its hip hold and '
            f'first-person size ({CFG["scale"]} x {FP_GUN_SCALE}), the arms at {ARMS:.2f}; hands from the parts\' grip '
            f'spots, else today\'s sheet poser.\nPalm gap: cm off the gun before seating (+ = air); seated: after; '
            f'touching: what closes on the gun; trigger: the index pad\'s miss. Parts: {parts}.', size=30)
    for k in range(0, len(done), 2):
        pair = [(path, words, '\n'.join(stats)) for path, words, stats in done[k:k + 2]]
        if len(pair) == 1:   # a last tile alone keeps the others' size: a blank beside it
            blank = os.path.join(WORK, 'blank.png')
            lt.write_png(blank, lt.to8(np.full(pixels(pair[0][0]).shape, 0.08, np.float32)))
            pair.append((blank, '', ''))
        s.row(pair)
    jpg = os.path.join(WORK, f'{GUN}_sheet.jpg')
    s.render(jpg)
    img = bpy.data.images.load(jpg)
    _ = img.pixels[0]                  # read the pixels now, before the image is pointed at its new file
    out = os.path.join(OUT, f'{GUN}_sheet.png')
    img.filepath_raw = out
    img.file_format = 'PNG'
    img.save()
    log(f'sheet {out} ({img.size[0]} x {img.size[1]})')


def held_parts(gun):
    """(slot, key, model, role) for every value of each held slot (the gun's .parts.csv: a part may wear another's mesh,
    its Model column). Held: the gun's hold and reload slots, and any slot whose parts carry grip spots."""
    with open(os.path.join(lt.REPO, 'Art', 'Models', 'Weapons', GUN + '.parts.csv'), encoding='utf-8') as f:
        rows = list(csv.DictReader(f))
    roles = {s: 'Hold' for s in CFG['hold']}
    roles.update({s: 'Reload' for s in CFG['reload'] if s not in roles})
    for name, obj in gun['ns']['built'].items():
        slot = name.split('_')[0]
        if slot not in roles and any(MARK.match(c.name[7:].split('.')[0]) for c in obj.children
                                     if c.name.startswith('SOCKET_')):
            roles[slot] = 'Hold'
    wanted_slots = [a for a in ARGV if a in roles]
    wanted_keys = [a for a in ARGV if '_' in a and a.split('_')[0] in roles]
    out = []
    for row in rows:
        slot, key = row['Slot'], row['Key']
        model = (row.get('Model') or '').strip() or key
        if slot not in roles:
            continue
        if (wanted_slots or wanted_keys) and slot not in wanted_slots and f'{slot}_{key}' not in wanted_keys:
            continue
        if f'{slot}_{model}' not in gun['ns']['built']:
            log(f'{slot} {key}: no model {slot}_{model}; skipped')
            continue
        out.append((slot, key, model, roles[slot]))
    return out


S['stage']()
log(f'{GUN}: hip hold {CFG["hip"]} (WeaponHipHold.cpp {"read" if _HIP else "not found: the table"}), framing {CFG["frame"]}')
GUN_OBJ = S['load_gun'](GUN, keep_all=True)
done = []
for slot, key, model, role in held_parts(GUN_OBJ):
    try:
        done.append(tile(GUN_OBJ, slot, key, model, role))
    except Exception as error:   # one broken part doesn't stop the sheet
        import traceback
        traceback.print_exc()
        log(f'{slot} {key}: FAILED {error}')
if done and 'nosheet' not in ARGV:
    contact_sheet(done)
log(f'done: {len(done)} tiles in {OUT}')
