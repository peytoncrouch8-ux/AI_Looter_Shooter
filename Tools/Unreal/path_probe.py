"""Walks a player-sized capsule down the middle of every road and ramp of the open area (layout.json roads and
features[].ramp) and reports what blocks it: the actor, its mesh, the road and where. Read-only; the area's level must
be open. Run it after cliff or dressing changes; width_probe.py looks across a flagged spot.
Usage: Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/path_probe.py <Area>" -Until "PATHS done" """
import json
import math
import sys
import unreal

AREA = sys.argv[1] if len(sys.argv) > 1 else 'RansomsRest'
ROOT = r'C:\Dev\AI_Looter_Shooter\Art\Levels\%s' % AREA
LAYOUT = json.load(open(ROOT + r'\layout.json'))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
RADIUS, HALF = 34.0, 88.0
STATIC = [unreal.ObjectTypeQuery.ECC_WORLD_STATIC, unreal.ObjectTypeQuery.ECC_WORLD_DYNAMIC]

routes = []
for road in LAYOUT.get('roads', []):
    if road.get('path'):
        routes.append((f"road {road['id']}", road['path']))
for feature in LAYOUT.get('features', []):
    ramp = feature.get('ramp')
    if ramp and ramp.get('path'):
        routes.append((f"ramp {feature['id']}", ramp['path']))


def ground(x, y):
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -20000.0),
                                                 unreal.TraceTypeQuery.ECC_VISIBILITY, False, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    for h in hits or []:
        t = h.to_tuple()
        actor = t[9]
        if actor is None or isinstance(actor, (unreal.Volume, unreal.PCGVolume)):
            continue
        if unreal.Name('Ground') in actor.tags:
            return t[4].z
    return None


report = {}
probed = 0
for name, path in routes:
    for (ax, ay), (bx, by) in zip(path, path[1:]):
        length = math.dist((ax, ay), (bx, by))
        n = max(int(length // 100), 1)
        for i in range(n + 1):
            x, y = ax + (bx - ax) * i / n, ay + (by - ay) * i / n
            z = ground(x, y)
            if z is None:
                continue
            probed += 1
            centre = unreal.Vector(x, y, z + HALF + 35.0)
            for c in unreal.SystemLibrary.capsule_overlap_components(world, centre, RADIUS, HALF, STATIC, None, []) or []:
                if c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN) != unreal.CollisionResponseType.ECR_BLOCK:
                    continue
                owner = c.get_owner()
                label = owner.get_actor_label() if owner else '?'
                if owner and unreal.Name('Ground') in owner.tags:
                    continue  # the terrain itself (a step up)
                mesh = c.static_mesh.get_name() if isinstance(c, unreal.StaticMeshComponent) and c.static_mesh else c.get_class().get_name()
                if isinstance(c, unreal.InstancedStaticMeshComponent):
                    label += '/' + c.get_name()
                report.setdefault((name, label, mesh), []).append((round(x), round(y), round(z)))

unreal.log(f'PATHS {AREA}: probed {probed} spots on {len(routes)} roads and ramps')
for (name, label, mesh), spots in sorted(report.items()):
    unreal.log(f'PATHS {name}: {label} ({mesh}) blocks {len(spots)} spots, first {spots[0]}, last {spots[-1]}')
unreal.log('PATHS done')
