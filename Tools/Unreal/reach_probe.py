"""Can a player walk from one spot to another? A flood fill over the floors in a box: cells on a grid, every walkable
floor under each (stacked floors too: a stair's flights, a cellar under a shop), joined where a step is low enough and
nothing stands between them at knee and chest height. Says whether the goal's floor was reached and, if not, how near the
fill came. Works in a play session or the editor's world. Lines start with REACHPROBE.
Usage: Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/reach_probe.py <from x,y,z> <to x,y,z> [half size cm]
       [grid cm]" -Until REACHPROBE
  from/to: points near a floor (the floor under them, within 2 m down, is used); the box spans both plus the half size
  (default 600 cm) round them."""
import math
import sys
from collections import deque

import unreal

TRACE = getattr(unreal.TraceTypeQuery, 'ECC_VISIBILITY', None) or unreal.TraceTypeQuery.TRACE_TYPE_QUERY1
STEP = 50.0          # the highest step between neighbouring cells (the character's step is 45 cm; a grid cell rounds)
HEAD = 150.0         # room over a floor for the player (0.85 of the mannequin: a capsule about 150 cm tall)
WALKABLE = 0.70      # the floor's normal (about 45 degrees)


def say(text):
    unreal.log(f'REACHPROBE {text}')


def world():
    subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    return subsystem.get_game_world() or subsystem.get_editor_world()


def trace(w, a, b, ignore):
    hit = unreal.SystemLibrary.line_trace_single(w, a, b, TRACE, False, ignore, unreal.DrawDebugTrace.NONE, True)
    return None if hit is None else hit.to_tuple()


def floors_at(w, x, y, top, bottom, ignore):
    """Every walkable floor under (x, y) from top down to bottom, with room for the player over it (looked for from 40 cm
    up, so a ramp rising beside the spot isn't taken for a ceiling)."""
    found = []
    z = top
    while z > bottom:
        t = trace(w, unreal.Vector(x, y, z), unreal.Vector(x, y, bottom), ignore)
        if t is None:
            break
        fz = t[5].z
        if t[7].z >= WALKABLE:
            clear = all(trace(w, unreal.Vector(x + dx, y + dy, fz + 40.0), unreal.Vector(x + dx, y + dy, fz + HEAD), ignore) is None
                        for dx, dy in ((0, 0), (18, 0), (-18, 0), (0, 18), (0, -18)))
            if clear:
                found.append(fz)
        z = fz - 8.0
    return found


def open_between(w, a, b, ignore):
    """Nothing stands between two floor spots at knee and chest height."""
    for h in (55.0, 120.0):
        if trace(w, unreal.Vector(a[0], a[1], a[2] + h), unreal.Vector(b[0], b[1], b[2] + h), ignore) is not None:
            return False
    return True


def main(args):
    if len(args) < 2:
        say(__doc__)
        return
    start = [float(v) for v in args[0].split(',')]
    goal = [float(v) for v in args[1].split(',')]
    half = float(args[2]) if len(args) > 2 else 600.0
    grid = float(args[3]) if len(args) > 3 else 25.0
    w = world()
    pawn = unreal.GameplayStatics.get_player_pawn(w, 0)
    ignore = [pawn] if pawn else []
    x0, x1 = min(start[0], goal[0]) - half, max(start[0], goal[0]) + half
    y0, y1 = min(start[1], goal[1]) - half, max(start[1], goal[1]) + half
    top = max(start[2], goal[2]) + 400.0
    bottom = min(start[2], goal[2]) - 400.0
    nx, ny = int((x1 - x0) / grid) + 1, int((y1 - y0) / grid) + 1
    cells = {}
    for i in range(nx):
        for j in range(ny):
            x, y = x0 + i * grid, y0 + j * grid
            for k, fz in enumerate(floors_at(w, x, y, top, bottom, ignore)):
                cells[(i, j, k)] = (x, y, fz)

    def nearest(p):
        best, best_d = None, 1e18
        for key, c in cells.items():
            if c[2] > p[2] + 50.0 or c[2] < p[2] - 200.0:
                continue
            d = math.hypot(c[0] - p[0], c[1] - p[1]) + abs(c[2] - p[2]) * 0.2
            if d < best_d:
                best, best_d = key, d
        return best

    s, g = nearest(start), nearest(goal)
    if s is None or g is None:
        say(f'no floor near the {"start" if s is None else "goal"} ({len(cells)} floor cells in the box)')
        return
    by_column = {}
    for key in cells:
        by_column.setdefault((key[0], key[1]), []).append(key)
    seen = {s}
    queue = deque([s])
    while queue:
        key = queue.popleft()
        if key == g:
            break
        here = cells[key]
        for di in (-1, 0, 1):
            for dj in (-1, 0, 1):
                if di == 0 and dj == 0:
                    continue
                for other in by_column.get((key[0] + di, key[1] + dj), ()):
                    if other in seen:
                        continue
                    there = cells[other]
                    if abs(there[2] - here[2]) > STEP or not open_between(w, here, there, ignore):
                        continue
                    seen.add(other)
                    queue.append(other)
    goal_cell = cells[g]
    if g in seen:
        say(f'reached: from {fmt(cells[s])} to {fmt(goal_cell)} ({len(seen)} of {len(cells)} floor cells joined)')
        return
    near = min(seen, key=lambda k: math.dist(cells[k], goal_cell))
    high = max(seen, key=lambda k: cells[k][2])
    say(f'NOT reached: the goal floor {fmt(goal_cell)}; nearest reached {fmt(cells[near])} '
        f'({math.dist(cells[near], goal_cell):.0f} cm off); highest reached {fmt(cells[high])}; '
        f'{len(seen)} of {len(cells)} floor cells joined')


def fmt(c):
    return f'({c[0]:.0f}, {c[1]:.0f}, {c[2]:.0f})'


main(sys.argv[1:])
