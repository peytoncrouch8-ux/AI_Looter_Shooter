"""The stern push's way up the side walks, walked as the game's test walks it (Tests/BargeBoardingFightTests.cpp, RouteOpen
in Tests/BargeTestWorld.h): a grid of cells STEP_X along x by STEP_Y across over one side's half of the hull, from that
side's stern fight spot (BargeFightRules.h SternFightSpots) forward to the well deck's lane (LaneAftX); a cell stands
where a drop from over it finds a collision floor within RISE of the cross walk's level and a Dredger's body (RADIUS) stands
clear on it from OVER_FEET to TALL over its feet. looter_tripo_barge_check runs it on the UCX hulls (its rule: open on both
sides); the numbers are the test's, kept in step with it by hand.
"""
import numpy as np

from looter_tripo_rework import B, D

START = {-1: (-600.0, -820.0), 1: (-600.0, 820.0)}    # SternFightSpots (port, starboard)
GOAL_X = 2320.0                                       # LaneAftX: the well deck's lane
Z = 2256.0                                            # CrossWalkZ
HALF_BEAM = 1237.5                                    # HullHalfBeam: each side's search runs from the centreline to it
STEP_X, STEP_Y, RISE, DROP = 30.0, 15.0, 30.0, 120.0  # cm: the cells; a floor within RISE of Z, found from DROP over it
RADIUS, OVER_FEET, TALL = 36.0, 30.0, 180.0           # cm: a Dredger's body (DredgerCreature.cpp), clear from its feet's step
CORE_STEP = 8.0                                       # cm between the points along its core


def _floor(hulls, x, y):
    """The highest collision surface under (x, y) from DROP over Z, or None."""
    top = None
    p, down = B((x, y, Z + DROP)), D((0.0, 0.0, -1.0))
    for _, lo, hi, _, tree in hulls:
        if not (lo[0] <= p[0] <= hi[0] and lo[1] <= p[1] <= hi[1]) or lo[2] > p[2]:
            continue
        loc = tree.ray_cast(p, down, (DROP + RISE * 2.0) / 100.0)[0]
        if loc is not None and (top is None or loc.z > top):
            top = loc.z
    return None if top is None else top * 100.0


def _blocker(hulls, x, y, feet):
    """The first hull a Dredger standing at (x, y) on feet meets, or None."""
    half = max((TALL - OVER_FEET) / 2.0, RADIUS)
    mid = feet + OVER_FEET + half
    core = [B((x, y, z)) for z in np.arange(mid - (half - RADIUS), mid + (half - RADIUS) + 0.1, CORE_STEP)]
    reach = RADIUS / 100.0
    for name, lo, hi, planes, tree in hulls:
        for b in core:
            if not all(lo[i] - reach <= b[i] <= hi[i] + reach for i in range(3)):
                continue
            if all(n.dot(b - q) < 0.0 for n, q in planes) or tree.find_nearest(b)[3] < reach:
                return name
    return None


def route(hulls, side):
    """(open, the farthest x reached, what stops it one cell on) for side (-1 port, 1 starboard)."""
    sx, sy = START[side]
    y0, y1 = (-HALF_BEAM, 0.0) if side < 0 else (0.0, HALF_BEAM)
    cols = int(np.ceil(abs(GOAL_X - sx) / STEP_X)) + 1
    rows = max(1, int(np.ceil((y1 - y0) / STEP_Y)) + 1)
    at = lambda c, r: (sx + c * STEP_X, y0 + r * STEP_Y)
    cells = {}

    def stands(c, r):
        if (c, r) not in cells:
            x, y = at(c, r)
            f = _floor(hulls, x, y)
            cells[(c, r)] = f is not None and abs(f - Z) <= RISE and _blocker(hulls, x, y, f) is None
        return cells[(c, r)]
    start = min(max(int(round((sy - y0) / STEP_Y)), 0), rows - 1)
    queue = [(0, r) for r in (start, start + 1, start - 1, start + 2, start - 2) if 0 <= r < rows and stands(0, r)][:1]
    seen = set(queue)
    far = (0, start)
    head = 0
    while head < len(queue):
        c, r = queue[head]
        head += 1
        if c >= cols - 1:
            return True, at(c, r)[0], None
        if c > far[0]:
            far = (c, r)
        for dc, dr in ((1, 0), (0, 1), (0, -1), (-1, 0)):
            n = (c + dc, r + dr)
            if 0 <= n[0] < cols and 0 <= n[1] < rows and n not in seen and stands(*n):
                seen.add(n)
                queue.append(n)
    x, y = at(min(far[0] + 1, cols - 1), far[1])
    f = _floor(hulls, x, y)
    why = (f'no floor within {RISE:.0f} of {Z:.0f} at ({x:.0f}, {y:.0f})' if f is None or abs(f - Z) > RISE
           else f'{_blocker(hulls, x, y, f)} at ({x:.0f}, {y:.0f})')
    return False, at(*far)[0], why if queue else f'no room at the start ({sx:.0f}, {sy:.0f})'
