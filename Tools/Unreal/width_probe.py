"""Across a road or ramp at the spots a path probe flagged: for each spot (x, y) and the route's direction there, which
lateral offsets (cm, left -, right +) a player-sized capsule can stand at, with the step it needs, and what blocks the
others. Read-only. Usage: py width_probe.py <Area> <route> <x,y> [<x,y> ...], the route as ramp_<feature id> or
road_<road id> (e.g. ramp_plateau); keep the console command under about 200 characters."""
import json
import math
import sys
import unreal

AREA, ROUTE = sys.argv[1], sys.argv[2].replace('_', ' ')
SPOTS = [tuple(float(v) for v in s.split(',')) for s in sys.argv[3:]]
LAYOUT = json.load(open(r'C:\Dev\AI_Looter_Shooter\Art\Levels\%s\layout.json' % AREA))
kind, rid = ROUTE.split(' ', 1)
if kind == 'road':
    route = next(r for r in LAYOUT['roads'] if r['id'] == rid)
    path, width = route['path'], route.get('width', 300)
else:
    ramp = next(f for f in LAYOUT['features'] if f['id'] == rid)['ramp']
    path, width = ramp['path'], ramp.get('width', 400)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
RADIUS, HALF, STEP = 34.0, 88.0, 45.0
STATIC = [unreal.ObjectTypeQuery.ECC_WORLD_STATIC, unreal.ObjectTypeQuery.ECC_WORLD_DYNAMIC]


def ground(x, y):
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -20000.0),
                                                 unreal.TraceTypeQuery.ECC_VISIBILITY, False, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    for h in hits or []:
        t = h.to_tuple()
        if t[9] is not None and unreal.Name('Ground') in t[9].tags:
            return t[4].z
    return None


def direction(x, y):
    best = None
    for (ax, ay), (bx, by) in zip(path, path[1:]):
        dx, dy = bx - ax, by - ay
        t = min(max(((x - ax) * dx + (y - ay) * dy) / max(dx * dx + dy * dy, 1e-6), 0.0), 1.0)
        d = math.dist((x, y), (ax + dx * t, ay + dy * t))
        if best is None or d < best[0]:
            length = math.hypot(dx, dy)
            best = (d, dx / length, dy / length)
    return best[1], best[2]


for sx, sy in SPOTS:
    fx, fy = direction(sx, sy)
    rx, ry = -fy, fx
    row = []
    half_w = width * 0.5
    offsets = [o for o in range(-int(half_w), int(half_w) + 1, 50)]
    for off in offsets:
        x, y = sx + rx * off, sy + ry * off
        z = ground(x, y)
        if z is None:
            row.append(f'{off:+d}:none')
            continue
        centre = unreal.Vector(x, y, z + HALF + STEP + 2.0)
        blockers = []
        for c in unreal.SystemLibrary.capsule_overlap_components(world, centre, RADIUS, HALF, STATIC, None, []) or []:
            if c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN) != unreal.CollisionResponseType.ECR_BLOCK:
                continue
            owner = c.get_owner()
            if owner and unreal.Name('Ground') in owner.tags:
                continue
            blockers.append(owner.get_actor_label() if owner else c.get_name())
        row.append(f'{off:+d}:' + ('ok' if not blockers else '|'.join(sorted(set(blockers)))))
    unreal.log(f'WIDTH {ROUTE} at ({sx:.0f}, {sy:.0f}) heading ({fx:.2f}, {fy:.2f}), width {width}: ' + '  '.join(row))
unreal.log('WIDTH done')
