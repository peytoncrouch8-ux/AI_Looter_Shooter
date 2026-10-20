"""The heroes' first-person arms' look sheet: one hero's arms (Tools/Blender/fp_arms_heroes.py) on the game's three guns,
posed by SK_FP_Arms's look sheet's own poser (Tools/Blender/fp_arms_sheet.py: each hand's frame from the handle it
holds, seated till the palm touches, fingers closed joint by joint till they touch, the index on the trigger), seen
from the eye as the game draws them: the view model's 90 degrees at 16:9, each gun at its hip hold
(Source/AI_Looter_Shooter/Weapons/WeaponHipHold.cpp; the revolver's create_revolver.py) and drawn size (rifle and
shotgun 1.1, revolver 1.2: UWeaponModelComponent::ScaleFor), the arms at the hero's ArmsScale (fp_arms_heroes.
ARMS_SCALE; the whole view is scaled about the eye instead, which the eye can't tell). Then close-ups of the trigger hand
on the rifle and the support hand on the revolver. Into Saved/ArtPreviews/Characters/FPArms_<Hero>/ with a composed
sheet (sheet.jpg).

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Tools\\Blender\\fp_arms_heroes_sheet.py -Threads 6
        -ScriptArgs <Hero>[,keep-textures][,fast][,rifle][,shotgun][,revolver][,close][,sheet]
fp_arms_sheet.py is run here only up to its shots (its source cut where they begin), with this hero's arms in place of
Ellis's; it is never edited.
"""
import math
import os
import sys
import time

import bpy
import numpy as np
from mathutils import Matrix, Vector

import looter_textures as lt

ARGV = [a for arg in (sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []) for a in arg.split(',') if a]
HERO = next((a for a in ARGV if a in ('Odessa', 'Crane', 'Gauge', 'Pike')), 'Odessa')
T0 = time.time()

import fp_arms_heroes as fh  # noqa: E402
import fp_arms_hands as hd  # noqa: E402
from fp_arms_base import A as FA  # noqa: E402

ARM, BODY, REST = fh.build(HERO)
BUILT = fh.LAST
SPEC = BUILT.hand[3] if BUILT.hand is not None else None
OUT = os.path.join(lt.PREVIEW_DIR, 'Characters', f'FPArms_{HERO}')
SCALE = fh.ARMS_SCALE[HERO]


def log(message):
    print(f'FPHEROSHEET [{time.time() - T0:6.1f}s] {message}', flush=True)


# fp_arms_sheet.py's poser, staging and rendering, bound to this hero's arms
_ns = dict(FA)
_ns.update(ARM=ARM, BODY=BODY, REST=REST)
if SPEC is not None:
    _ns['CHAINS'] = SPEC.chains
    _ns['body_sdf'] = lambda P: hd.body_sdf(P, SPEC)
else:
    # Gauge: block fingers about 1.3-1.4 cm from their bones' lines, the palm's underside on Ellis's
    import fp_arms_base as fb
    _ns['CHAINS'] = fb.chains(1.22, 1.22, 1.0)
_sheet_py = os.path.join(lt.REPO, 'Tools', 'Blender', 'fp_arms_sheet.py')
with open(_sheet_py, encoding='utf-8') as fh_:
    _src = fh_.read()
_line = "A = runpy.run_path(os.path.join(lt.REPO, 'Art', 'Models', 'Characters', 'FPArms.py'))"
if _line not in _src or '\nstage()\nwant = ' not in _src:
    raise RuntimeError('fp_arms_sheet.py changed: update fp_arms_heroes_sheet.py where it cuts and binds it')
_src = _src.replace(_line, 'A = __HERO_ARMS__')
_src = _src[:_src.find('\nstage()\nwant = ')]
S = {'__name__': 'FPSheetDefs', '__file__': _sheet_py, '__HERO_ARMS__': _ns}
exec(compile(_src, _sheet_py, 'exec'), S)
S['WORK'] = OUT
ue, ue_rotator, affine, unit = S['ue'], S['ue_rotator'], S['affine'], S['unit']
Hand, hand_frame, seat, close_fingers, close_thumb = S['Hand'], S['hand_frame'], S['seat'], S['close_fingers'], \
    S['close_thumb']
reach_with, pose, gun_point, gun_dir, gun_tree = S['reach_with'], S['pose'], S['gun_point'], S['gun_dir'], S['gun_tree']
load_gun, show_gun, part_axis, arms_tree = S['load_gun'], S['show_gun'], S['part_axis'], S['arms_tree']
camera, eye_camera, render = S['camera'], S['eye_camera'], S['render']
GRIP_C = S['GRIP_C']

# The game's hip holds (camera space, cm; WeaponHipHold.cpp, create_revolver.py), drawn sizes, the arms' scale
HOLDS = {'rifle': ((50.0, 23.0, -23.5), (0.0, 0.0, 0.0), 1.1),
         'shotgun': ((30.0, 22.5, -23.0), (0.0, -3.0, -5.0), 1.1),
         'revolver': ((50.0, 22.0, -21.0), (0.7, -1.2, -8.0), 1.2)}


def place(gun, kind):
    """The gun at its hip hold and drawn size, in the arms' space (the view scaled about the eye by 1 / ArmsScale)."""
    (x, y, z), (p, yw, r), k = HOLDS[kind]
    target = ue(x, y, z) / SCALE
    R = ue_rotator(p, yw, r)
    pivot = gun['sockets']['Grip']
    s = k / SCALE
    M = affine(np.eye(3), target) @ affine(R, np.zeros(3)) @ affine(np.eye(3) * s, np.zeros(3)) @ \
        affine(np.eye(3), -pivot)
    gun['M'] = M
    gun['rig'].matrix_world = Matrix([list(row) for row in M])
    bpy.context.view_layer.update()


def note_hands(*hands):
    for h in hands:
        log(f'  {h.side}: ' + ', '.join(f'{k} {tuple(int(a) for a in v)}' for k, v in h.curls.items()) +
            f', thumb {tuple(int(a) for a in h.thumb)}')


# ----------------------------------------------------------------------------------------------------- the guns
def rifle():
    gun = load_gun('Bullpup')
    show_gun(gun, True)
    place(gun, 'rifle')
    r, l = S['bullpup_hands'](gun)
    pose([r, l])
    eye_camera(focus=0.62 / SCALE, fstop=2.8)
    render('rifle_hip')
    # the trigger hand from its right, a little behind and above (the eye's view hides most of it)
    target = r.world(GRIP_C)
    look('close_trigger', target, (-0.85, 0.45, 0.55), 0.34)
    show_gun(gun, False)


def shotgun():
    gun = load_gun('Ranchhand')
    show_gun(gun, True)
    place(gun, 'shotgun')
    trees = [gun_tree(gun)]
    # the trigger hand round the stock's wrist (its index-way up and ahead), as the rifle's
    up = gun_dir(gun, (0.0, -0.42, 0.9))
    right = Hand('r', hand_frame('r', up, gun_dir(gun, (-0.35, 0.8, -0.45)), palm=gun_dir(gun, (0.7, -0.7, 0.0)),
                                 palm_weight=1.0), gun_point(gun, gun['sockets']['Grip']), along=1.0, tilt=18.0)
    seat(right, trees)
    right.spread = {'index': 4.0, 'middle': 0.0, 'ring': -2.0, 'pinky': -5.0}
    trig = gun['sockets'].get('Trigger')
    if trig is None:
        g = gun['sockets']['Grip']
        trig = g + np.array([0.0, -0.075, 0.035])            # about 8 cm ahead of the grip, a little up
    reach_with(right, trees, 'index', gun_point(gun, trig))
    close_fingers(right, trees, names=('middle', 'ring', 'pinky'))
    close_thumb(right, trees, opp=26.0, flex=6.0)
    # the support hand round the pump from under it, thumb along its near side
    fore = gun_point(gun, gun['sockets']['Foregrip'])
    ahead = gun_dir(gun, (0.0, -1.0, 0.0))
    left = Hand('l', hand_frame('l', ahead, gun_dir(gun, (0.5, 0.5, -0.7)), palm=gun_dir(gun, (-0.45, 0.0, 1.0)),
                                palm_weight=1.5), fore)
    seat(left, trees)
    close_fingers(left, trees)
    close_thumb(left, trees, opp=14.0, flex=0.0)
    note_hands(right, left)
    pose([right, left])
    eye_camera(focus=0.45 / SCALE, fstop=2.8)
    render('shotgun_hip')
    show_gun(gun, False)


def revolver():
    gun = load_gun('Drover')
    show_gun(gun, True)
    place(gun, 'revolver')
    grip = next(o for o in gun['parts'] if 'Grip' in o.name)
    axis, _ = part_axis(grip, gun)
    if axis @ gun_dir(gun, (0, 0, 1.0)) < 0:
        axis = -axis
    trees = [gun_tree(gun)]
    right = Hand('r', hand_frame('r', axis, gun_dir(gun, (-0.35, 0.8, -0.45)), palm=gun_dir(gun, (0.7, -0.7, 0.0)),
                                 palm_weight=1.0), gun_point(gun, gun['sockets']['Grip']), tilt=10.0)
    seat(right, trees)
    right.spread = {'index': 3.0, 'middle': 0.0, 'ring': -2.0, 'pinky': -4.0}
    reach_with(right, trees, 'index', gun_point(gun, (0.0, -0.044, -0.05)))
    close_fingers(right, trees, names=('middle', 'ring', 'pinky'))
    close_thumb(right, trees, opp=22.0, flex=4.0)
    pose([right])
    trees_l = trees + [arms_tree('r')]
    left = Hand('l', hand_frame('l', axis, gun_dir(gun, (0.55, 0.65, -0.5)), palm=gun_dir(gun, (-0.8, -0.3, 0.0)),
                                palm_weight=1.0), gun_point(gun, gun['sockets']['Foregrip']))
    seat(left, trees_l)
    close_fingers(left, trees_l)
    close_thumb(left, trees_l, opp=18.0, flex=0.0)
    note_hands(right, left)
    pose([right, left])
    eye_camera(focus=0.55 / SCALE, fstop=2.8)
    render('revolver_hip')
    # the support hand up close, from outside, a little above and ahead (its back is the one the low sun lights)
    hand_c = left.world(np.array([4.0, 0.0, 0.5]) * FA['C'])
    back = left.R[:, 2]
    cam_at = hand_c + unit(back * 0.8 + np.array([0.0, 0.0, 0.55]) + np.array([0.0, -0.3, 0.0])) * 0.5
    camera(cam_at, hand_c + np.array([0.0, 0.035, 0.035]), lens=50.0, focus=float(np.linalg.norm(cam_at - hand_c)),
           fstop=5.6)
    render('close_support', resolution=(1600, 1100))
    eye_camera()
    show_gun(gun, False)


def look(name, target, direction, dist, lens=40.0):
    target = np.asarray(target)
    at = target + unit(np.asarray(direction)) * dist
    camera(at, target, lens=lens, focus=dist, fstop=5.6)
    render(name, resolution=(1600, 1100))
    eye_camera()


def compose():
    sys.path.insert(0, os.path.join(lt.REPO, 'Art', 'Backlog', 'Creatures'))
    import MinerConcepts as mc
    p = lambda n: os.path.join(OUT, n + '.png')
    tris = sum(len(poly.vertices) - 2 for poly in BODY.data.polygons)
    s = mc.Sheet(width=2000)
    s.title(f'First-person arms: {HERO}')
    s.lines(f'SK_FP_Arms_{HERO}: {tris / 1000:.1f}k triangles, the 46 bones of SK_FP_Arms (Ellis\'s skeleton, bone for '
            f'bone); {len(BODY.data.materials)} material{"s" if len(BODY.data.materials) > 1 else ""}.\nDrawn as the '
            f'game draws them: 90 degrees, each gun at its hip hold and drawn size, the arms at {SCALE:.2f}. '
            'Posed by the look sheet\'s poser; the game poses them by code.', size=32)
    s.heading('From the eye')
    s.row([(p('rifle_hip'), '', 'The Bullpup at the hip.'), (p('shotgun_hip'), '', 'The Ranchhand at the hip.')])
    s.row([(p('revolver_hip'), '', 'The Drover, two-handed.'), (p('rest'), '', 'The bind pose.')]
          if os.path.exists(p('rest')) else [(p('revolver_hip'), '', 'The Drover, two-handed.')])
    s.heading('Up close')
    s.row([(p('close_trigger'), '', 'The trigger hand on the Bullpup.'),
           (p('close_support'), '', 'The support hand on the Drover.')])
    jpg = os.path.join(OUT, 'sheet.jpg')
    s.render(jpg)


S['stage']()
want = [a for a in ARGV if a in ('rifle', 'shotgun', 'revolver', 'sheet')] or ['rifle', 'shotgun', 'revolver', 'sheet']
for name in ('rifle', 'shotgun', 'revolver'):
    if name in want:
        globals()[name]()
if 'sheet' in want:
    compose()
log('done')
