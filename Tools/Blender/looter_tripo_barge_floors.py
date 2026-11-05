"""The reworked dredge barge's floors, measured on the mesh (looter_tripo_barge's squeeze, its collision and its
checks use them): per 25 cm station along x and per side, the walk floor's run across y (Tripo's trench from the foot
of the house or casing to the bulwark's inner face, or the forecastle's floor between the scoop's walls) and the
bulwark's top beside it. Frame: the barge's, cm (looter_tripo_rework.U)."""
import math

from mathutils import Vector
from mathutils.bvhtree import BVHTree

from looter_tripo_rework import U, ue_centre

DECK_UP = 0.85                         # a face's normal z over this: it faces up (a floor)
RIM_OUT, RIM_SPAN, RIM_CLEAR = 5.0, 250.0, 15.0   # the bulwark's top: its points from its inner face + RIM_OUT out
                                       # RIM_SPAN; what stands RIM_CLEAR over it and inboard (ledges) squeezes whole
WALK_FLOOR, WALK_Y, WALK_STEP = (2240.0, 2262.0), (450.0, 1100.0), 25.0   # where trenches() looks for the walk floors
TRENCH_JOIN = 2.0                      # cm: floor faces closer than this across y are one run
OUTBOARD_TEST = (30.0, 400.0)          # cm: a floor has its bulwark (or wall) within this, ray out at this height
RIM_Z_MAX = 2450.0                     # the bulwark's points looked at stop under this (the walk canopies start here)
HEAD_RAYS = (-10.0, -5.0, 0.0, 5.0, 10.0)   # cm along x round a station: its headroom rays (a strap 5 cm thick across a
                                       # walk between two stations' rays stood at head height at x 1607, unseen)
STEP_OVER = 10.0                       # cm: a walk's headroom is measured from this over its floor (a plate or a cut
                                       # stub lower than this is a step on the walk, not a roof over it)
PATH_IN = 30.0                         # cm: a side walk's middle line keeps within its width rule's half + this of the
                                       # bulwark (where the floor runs inboard round a wall's corner, its middle lay under
                                       # the house's overhang, where no pawn walks)


def walk_middle(lo, hi, width):
    """|y| of a side walk's middle line over a floor lo..hi: its middle, no farther inboard of the bulwark's face than
    width / 2 + PATH_IN (width: the walk's rule)."""
    return max((lo + hi) / 2.0, hi - width / 2.0 - PATH_IN)


def rims(bm, trench, step=WALK_STEP):
    """The bulwark's top per trench station: the highest of its points from its inner face + RIM_OUT out RIM_SPAN."""
    top = {}
    for v in bm.verts:
        x, y, z = U(v.co)
        t = trench_at(trench, x, y)
        if t is None or not t[1] + RIM_OUT <= abs(y) <= t[1] + RIM_SPAN or not WALK_FLOOR[0] < z < RIM_Z_MAX:
            continue
        key = (1 if y > 0 else -1, int(round(x / step)))
        top[key] = max(top.get(key, -1e9), z)
    return top


def rim_at(table, x, y, step=WALK_STEP):
    """The bulwark's top near x on y's side (the highest within a station either way: a post counts at its own)."""
    s, k = (1 if y > 0 else -1), int(round(x / step))
    near = [table[(s, k + d)] for d in (-1, 0, 1) if (s, k + d) in table]
    return max(near) if near else None


def trenches(bm, z_range=WALK_FLOOR, y_range=WALK_Y, step=WALK_STEP, outermost=True):
    """The walk floors per station x = k * step and side: {(side, k): (|y| lo, |y| hi, top z)}: the up-facing faces on
    the floor level under the open sky or an overhang (not inside a shell: Tripo's deck runs on under the casing), their
    outermost run across y (Tripo's trench from the foot of the house or casing to the bulwark's inner face; nearly the
    whole width at the cross walk and the well deck); with outermost False the run from the centreline out (the
    forecastle's floor between the scoop's walls). A face counts at every station its x span reaches."""
    bm.normal_update()
    tree = BVHTree.FromBMesh(bm)
    up = Vector((0.0, 0.0, 1.0))
    spans = {}
    for f in bm.faces:
        x, y, z = ue_centre(f)
        if f.normal.z < DECK_UP or not (z_range[0] < z < z_range[1] and y_range[0] < abs(y) < y_range[1]):
            continue
        hit = tree.ray_cast(f.calc_center_median() + up * 0.01, up)
        if hit[0] is not None and hit[1].dot(up) > 0.0:     # the first thing above faces up: inside a shell
            continue
        s = 1 if y > 0 else -1
        # A ledge outside the bulwark (the hull's upper strake is level with the floors) has nothing outboard of it.
        side = Vector((-float(s), 0.0, 0.0))
        start = f.calc_center_median() + up * (OUTBOARD_TEST[0] / 100.0)
        if tree.ray_cast(start, side, OUTBOARD_TEST[1] / 100.0)[0] is None:
            continue
        ps = [U(v.co) for v in f.verts]
        xs = [p[0] for p in ps]
        for k in range(math.ceil(min(xs) / step - 0.5), math.floor(max(xs) / step + 0.5) + 1):
            spans.setdefault((s, k), []).append((min(abs(p[1]) for p in ps), max(abs(p[1]) for p in ps),
                                                 max(p[2] for p in ps)))
    table = {}
    for key, runs in spans.items():
        if outermost:                           # from the bulwark inboard while the faces touch
            runs.sort(key=lambda r: -r[1])
            lo, hi, top = runs[0]
            for a, b, t in runs[1:]:
                if b < lo - TRENCH_JOIN:
                    break
                lo, top = min(lo, a), max(top, t)
        else:                                   # from the centreline out
            runs.sort(key=lambda r: r[0])
            lo, hi, top = runs[0]
            for a, b, t in runs[1:]:
                if a > hi + TRENCH_JOIN:
                    break
                hi, top = max(hi, b), max(top, t)
        table[key] = (lo, hi, top)
    return table


def trench_at(table, x, y, step=WALK_STEP):
    """The floor (lo, hi, top z) at the station nearest x on y's side, or None."""
    s, k = (1 if y > 0 else -1), int(round(x / step))
    return next((table[(s, k + d)] for d in (0, -1, 1, -2, 2, -3, 3) if (s, k + d) in table), None)
