"""The four other heroes' first-person arms (Art/Models/Characters/FPArms<Hero>.py, Tools/Blender/fp_arms_heroes.py):
what they share with Ellis's SK_FP_Arms (Art/Models/Characters/FPArms.py), read from that script without running it.

FPArms.py builds Ellis's arms when it runs (its last lines). Here its source is executed only up to that build, into a
namespace of its own (A): its rest pose (Rest, Ellis's arm from the eye), its finger and thumb chains (the bones), the
distance-field and surface-nets kit, the sleeve's folds, the rasterizer and the leather's stitches. Every hero's arms
use the same rest pose and the same chains' joints, so their skeletons are Ellis's bone for bone (the hands code,
FFirstPersonHandsRig, binds any of them unchanged); only what's drawn round the bones differs. FPArms.py itself is never
edited from here (its textures and FBX stay as they are).
"""
import math
import os
import time

import numpy as np

import looter_textures as lt

FPARMS_PY = os.path.join(lt.REPO, 'Art', 'Models', 'Characters', 'FPArms.py')
WORK = os.path.join(lt.REPO, 'Intermediate', 'FPArms')
T0 = time.time()
C = 0.01                                   # hand-frame layouts are in cm, built in m


def log(message):
    print(f'FPHEROES [{time.time() - T0:6.1f}s] {message}', flush=True)


def _load_fparms():
    """FPArms.py's definitions (everything above its 'ARM, BODY, REST = build()'), as a dict."""
    with open(FPARMS_PY, encoding='utf-8') as fh:
        src = fh.read()
    cut = src.find('\nARM, BODY, REST = build()')
    if cut < 0:
        raise RuntimeError('FPArms.py has no "ARM, BODY, REST = build()" line to stop before: update fp_arms_base.py')
    ns = {'__name__': 'FPArmsDefs', '__file__': FPARMS_PY}
    exec(compile(src[:cut], FPARMS_PY, 'exec'), ns)
    return ns


A = _load_fparms()
v3, unit, smoothstep, rotate = A['v3'], A['unit'], A['smoothstep'], A['rotate']
smin, smax, capsule, ellipsoid = A['smin'], A['smax'], A['capsule'], A['ellipsoid']
Chain = A['Chain']
REST_CURL = A['REST_CURL']
GRIP_POINT, HANDLE_AXIS = A['GRIP_POINT'], A['HANDLE_AXIS']
ELLIS_CHAINS = A['CHAINS']


def chains(finger_scale=1.0, thumb_scale=None, tip_scale=1.0):
    """Ellis's chains (the same joints: the same bones) with their radii scaled: a slimmer or a heavier hand on the same
    skeleton. tip_scale narrows the end joints and the tips further (bony fingers taper more)."""
    thumb_scale = finger_scale if thumb_scale is None else thumb_scale
    out = []
    for ch in ELLIS_CHAINS:
        k = thumb_scale if ch.name == 'thumb' else finger_scale
        n = len(ch.radii)
        radii = [r * k * (1.0 + (tip_scale - 1.0) * max(0.0, (i - (n - 3)) / 2.0)) for i, r in enumerate(ch.radii)]
        out.append(Chain(ch.name, [p.copy() for p in ch.joints], radii, ch.lateral.copy(), ch.s0, list(ch.bones)))
    return out


def ellipse_r(theta, ry, rz):
    """The radius of an ellipse with half sizes ry (across, theta = pi/2) and rz (theta = 0) at turn theta."""
    return 1.0 / np.sqrt((np.sin(theta) / ry) ** 2 + (np.cos(theta) / rz) ** 2)
