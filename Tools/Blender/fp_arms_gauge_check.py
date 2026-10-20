"""Do Gauge's first-person finger blocks rest on a grip or sink into it? (Tools/Blender/fp_arms_gauge.py)

The game closes every hero's hand by Ellis's numbers (FirstPersonHandsFingers.cpp, FHandFingers::Of: each grip's curls
from straight; the rig takes off the rest curl, 14 / 20 / 12 degrees). This poses the index and middle fingers and the
thumb by those curls (the trigger hand's index by the gun's TriggerCurl) and measures, for each grip, how near each
finger's palm side comes to the handle the rig is made for: FPArms_rig.json's GripPoint and HandleAxis, a 4 cm handle
(radius 2 cm). Ellis's gloved fingers (FPArms.py's capsules, flattened toward the palm) against Gauge's blocks, now
(fp_arms_gauge.finger_sides) and as they were (a block centred on the bone, 2.5 cm thick: 1.25 cm each way).
Prints, per grip and finger, the nearest approach to the handle's axis (cm) and how much deeper than Ellis's it sinks
(mm; 0 or less rests on, as his do). The thumb turns at its base about its own across-axis (as the rig's CurledLocal).

    powershell -NoProfile -File Tools\\artrun.ps1 -Script Tools\\Blender\\fp_arms_gauge_check.py -Threads 6
"""
import json
import math
import os

import numpy as np

import looter_textures as lt
from fp_arms_base import A, C, unit, v3
import fp_arms_gauge as ga

REST = (14.0, 20.0, 12.0)
# FHandFingers::Of: Fist(knuckle, middle, tip, spread, thumb swing, thumb middle, thumb tip); the index's own where set
GRIPS = {
    'Trigger (rifle 30/50/25)': ((60, 80, 50, 3.0), (28, 15, 10), (30, 50, 25)),
    'Trigger (shotgun 5/20/10)': ((60, 80, 50, 3.0), (28, 15, 10), (5, 20, 10)),
    'Trigger (revolver 25/50/25)': ((60, 80, 50, 3.0), (28, 15, 10), (25, 50, 25)),
    'Forend': ((40, 55, 35, 3.5), (20, 10, 5), None),
    'Wrap': ((35, 45, 30, 2.0), (25, 15, 10), None),
    'Support': ((45, 60, 40, 2.0), (18, 5, 5), None),
    'Pump': ((55, 70, 45, 3.5), (20, 10, 0), None),
}


def rot(axis, deg, about):
    k = unit(axis)
    a = math.radians(deg)
    K = np.array([[0, -k[2], k[1]], [k[2], 0, -k[0]], [-k[1], k[0], 0]])
    R = np.eye(3) + math.sin(a) * K + (1 - math.cos(a)) * K @ K
    return lambda p: (np.asarray(p) - about) @ R.T + about


def posed_segments(ch, curls):
    """The chain's segments posed (hand frame, m): [(head, tail, dorsal, lateral)], each joint turned by its curl less
    its rest one about the finger's across-axis (the thumb: about the axis across its own bone and the palm's facing)."""
    J = [np.array(j) for j in ch.joints]
    first = 0 if ch.name == 'thumb' else 1
    dors = [ch.dorsal(k) for k in range(len(J) - 1)]
    lat = [ch.lateral.copy() for _ in range(len(J) - 1)]
    palm = v3(0, 0, -1.0)
    # the thumb's rest bends as the rig measures them (FirstPersonHandsFingers.cpp): none at its base, its middle
    # joint's between its first two bones about their across-axis, the end joint 0.6 of that
    thumb_rest = [0.0, 0.0, 0.0]
    if ch.name == 'thumb':
        a, b = unit(J[1] - J[0]), unit(J[2] - J[1])
        ax = unit(np.cross(b, palm))
        thumb_rest[1] = math.degrees(math.atan2(np.cross(a, b) @ ax, a @ b))
        thumb_rest[2] = thumb_rest[1] * 0.6
    for j in range(3):
        k = first + j                            # the joint at the head of segment k turns segment k and beyond
        if ch.name == 'thumb':
            rest_curl = thumb_rest[j]
            along = unit(J[k + 1] - J[k])
            axis = unit(np.cross(along, palm))
        else:
            rest_curl = REST[j]
            axis = lat[k]
        f = rot(axis, curls[j] - rest_curl, J[k])
        for m in range(k + 1, len(J)):
            J[m] = f(J[m])
        R = rot(axis, curls[j] - rest_curl, np.zeros(3))
        for m in range(k, len(J) - 1):
            dors[m] = R(dors[m])
            lat[m] = R(lat[m])
    return [(J[k], J[k + 1], unit(dors[k]), unit(lat[k])) for k in range(first, len(J) - 1)]


def nearest(points, grip, axis):
    d = points - grip
    return float(np.min(np.linalg.norm(d - np.outer(d @ axis, axis), axis=1)))


def capsule_points(seg, r0, r1, flat, n=13, m=24):
    """Ellis's gloved finger segment's surface (FPArms.py's tapered capsule, its section squashed to flat toward the
    back and the palm), sampled round and along it, its ends' half-spheres too."""
    h, t, dors, lat = seg
    ax = unit(t - h)
    pts = []
    ang = np.linspace(0.0, 2 * math.pi, m, endpoint=False)
    for a in np.linspace(-0.4, 1.4, n):
        u = min(max(a, 0.0), 1.0)
        r = r0 + (r1 - r0) * u
        beyond = (a - u) * float(np.linalg.norm(t - h))                  # into an end's half-sphere
        k = math.sqrt(max(r * r - beyond * beyond, 0.0)) / r if r > 0 else 0.0
        c = h + (t - h) * u + ax * beyond
        for g in ang:
            pts.append(c + (lat * math.cos(g) + dors * math.sin(g) * flat) * r * k)
    return np.array(pts)


def block_points(seg, palm_side, back_side, width, bevel, ext, n=13, m=40):
    """Gauge's finger block's surface: a box along the segment (past each joint by ext), its section width across and
    palm_side + back_side through, the long edges rounded by bevel."""
    h, t, dors, lat = seg
    ax = unit(t - h)
    L = float(np.linalg.norm(t - h))
    zc = (back_side - palm_side) / 2
    hw, hz = width / 2 - bevel, (palm_side + back_side) / 2 - bevel
    pts = []
    for a in np.linspace(-ext, L + ext, n):
        c = h + ax * a
        for g in np.linspace(0.0, 2 * math.pi, m, endpoint=False):
            # a rounded rectangle's outline: the inner rectangle's nearest corner plus the bevel's circle
            x, z = math.cos(g), math.sin(g)
            px = hw * np.sign(x) + bevel * x
            pz = hz * np.sign(z) + bevel * z
            pts.append(c + lat * px + dors * (zc + pz))
    return np.array(pts)


def main():
    rig = json.load(open(os.path.join(lt.REPO, 'Intermediate', 'FPArms', 'FPArms_Gauge_rig.json')))
    ellis = json.load(open(os.path.join(lt.REPO, 'Intermediate', 'FPArms', 'FPArms_rig.json')))
    assert rig['hands'] == ellis['hands'] and rig['bones'] == ellis['bones'], 'Gauge\'s rig no longer matches Ellis\'s'
    grip = A['GRIP_POINT'] * C                    # the json's GripPoint, in the hand frame
    axis = unit(A['HANDLE_AXIS'])
    R_HANDLE = 0.02
    chains = {ch.name: ch for ch in A['CHAINS'] if ch.name in ('index', 'middle', 'thumb')}
    print('FPCHECK rig json: Gauge\'s grip points and bones are Ellis\'s')
    print('FPCHECK nearest approach to a 4 cm handle\'s axis (cm; under 2.0 is inside it) and the sink past Ellis\'s (mm)')
    worst = {'before': 0.0, 'now': 0.0}
    for grip_name, (fist, thumb, index) in GRIPS.items():
        row = []
        for name, ch in chains.items():
            fi = {'index': 0, 'middle': 1}.get(name)
            if name == 'thumb':
                curls = thumb
            elif name == 'index' and index is not None:
                curls = index
            else:
                k, m, t, spread = fist
                curls = (k + spread * fi, m + spread * fi, t)
            segs = posed_segments(ch, curls)
            first = 0 if name == 'thumb' else 1
            flat = ga.PALM_FLAT.get(name, 0.9)
            width, thick = ga.FINGER[name]
            last = len(segs) - 1
            e_pts = np.vstack([capsule_points(s, ch.radii[first + i], ch.radii[first + i + 1], flat)
                               for i, s in enumerate(segs)])
            now_pts = np.vstack([block_points(s, *(v * C for v in ga.finger_sides(ch, first + i, thick)),
                                              ga.finger_width(ch, first + i) * C,
                                              ga.block_bevel(ch, first + i, i == last) * C,
                                              (0.45 if i == last else 0.3) * C)
                                 for i, s in enumerate(segs)])
            old_pts = np.vstack([block_points(s, thick / 2 * C, thick / 2 * C, width * C,
                                              (0.75 if i == last else 0.5) * C, (0.45 if i == last else 0.3) * C)
                                 for i, s in enumerate(segs)])
            e, n, o = (nearest(p, grip, axis) for p in (e_pts, now_pts, old_pts))
            if name == 'index' and index is not None:
                row.append(f'{name} on the trigger')
                continue
            worst['before'] = max(worst['before'], (e - o) * 1000)
            worst['now'] = max(worst['now'], (e - n) * 1000)
            row.append(f'{name} Ellis {e / C:.2f}, Gauge {n / C:.2f} ({(e - n) * 1000:+.1f} mm; was {o / C:.2f}, '
                       f'{(e - o) * 1000:+.1f})')
        print(f'FPCHECK {grip_name}: ' + '; '.join(row))
    print(f'FPCHECK deepest past Ellis\'s: before {worst["before"]:.1f} mm, now {worst["now"]:.1f} mm')


main()
