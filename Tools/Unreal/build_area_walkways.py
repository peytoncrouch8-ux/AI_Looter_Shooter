"""The ramps' walkways kept open for a player, for build_area.py (layout.json features[].ramp: its path, its width and
the cliff group that lines it, by default the ramp's own id).

A cliff piece is placed from its dressing point and its kit's shape, so near a ramp's foot or a run's end one can
stand in the walkway: the tutorial island's plateau edge ended in a piece across the foot of the ramp to the lookout,
and with that one held back, the ramp's first wall piece still left a gap a player barely fit through. So once the
cliffs stand, a player's capsule is tested down the middle of every walkway (CLEAR_SHARE of its width), a step up off
the terrain, and each cliff piece it meets is moved off a little at a time until none does: one of the ramp's own walls
back into its cut wall, any other piece along its run, away from the walkway. A wall that doesn't face its walkway (a
ledge's outer face, under the path: the bluff path's and the Sink ramp's) sinks instead, so its top ends under the
path's edge rather than standing in the walkway as a kerb (they narrowed both to about 2 m, the user's "narrow areas",
2026-10-08); one that would have to sink past LEDGE_SINK_MOST moves out from under the path the rest of the way.
"""
import math
import re

import unreal

# The middle share of a walkway's width kept clear, tested every WALK_STEP along it and every WALK_ACROSS across it
# (cm) with a player's capsule (radius, half height) standing STEP_UP off the terrain, as a player steps up a foot.
CLEAR_SHARE = 0.6
WALK_STEP = 100.0
WALK_ACROSS = 50.0
PLAYER = (34.0, 88.0)
STEP_UP = 47.0
# A piece in the way moves off MOVE_STEP at a time, MOVE_MOST at most (cm).
MOVE_STEP = 25.0
MOVE_MOST = 400.0
# A ledge's outer face sinks at most this far (cm): further, and its top would drop below the ledge's own lip and show a
# gap under the path's edge.
LEDGE_SINK_MOST = 150.0
# A cliff piece's label: Cliff_<group>_<point>[_<course>].
CLIFF_LABEL = re.compile(r'^Cliff_(.+?)_(\d{2})(?:_\d+)?$')
OBJECTS = [unreal.ObjectTypeQuery.ECC_WORLD_STATIC, unreal.ObjectTypeQuery.ECC_WORLD_DYNAMIC]


def ramp_corridors(source):
    """The walkways up the features' ramps, with the cliff group that lines each (its cliffGroup, else the ramp's id),
    which alone may stand at its edge: [(ramp id, path, half width, walls group)]."""
    return [(ramp.get('id', feature.get('id')), ramp['path'], ramp['width'] * 0.5, ramp.get('cliffGroup') or ramp.get('id'))
            for feature in source.get('features', []) for ramp in [feature.get('ramp')] if ramp and ramp.get('path')]


def nearest(path, x, y):
    """The distance from (x, y) to a path and the nearest point on it."""
    best = None
    for (ax, ay), (bx, by) in zip(path, path[1:]):
        dx, dy = bx - ax, by - ay
        t = min(max(((x - ax) * dx + (y - ay) * dy) / max(dx * dx + dy * dy, 1e-6), 0.0), 1.0)
        point = (ax + dx * t, ay + dy * t)
        d = math.dist((x, y), point)
        if best is None or d < best[0]:
            best = (d, point)
    return best


def in_corridor(point, corridors, group):
    """Whether a point (x, y) lies on a ramp's walkway that another group's walls line."""
    return any(group != walls and nearest(path, *point)[0] < half for _, path, half, walls in corridors)


def samples(path, half, tiles, terrain_hit):
    """The capsule's centres down the middle of a walkway, on the terrain."""
    band = half * CLEAR_SHARE
    across = [-band + WALK_ACROSS * k for k in range(int(2 * band // WALK_ACROSS) + 1)]
    out = []
    for (ax, ay), (bx, by) in zip(path, path[1:]):
        length = math.dist((ax, ay), (bx, by))
        rx, ry = -(by - ay) / length, (bx - ax) / length
        for i in range(max(int(length // WALK_STEP), 1) + 1):
            t = i / max(int(length // WALK_STEP), 1)
            for off in across:
                x, y = ax + (bx - ax) * t + rx * off, ay + (by - ay) * t + ry * off
                hit = terrain_hit(tiles, unreal.Vector(x, y, 30000.0), unreal.Vector(x, y, -30000.0))
                if hit is not None:
                    out.append(unreal.Vector(x, y, hit.z + STEP_UP + PLAYER[1]))
    return out


def blocking(world, centre, tag):
    """The build's actors (tagged tag) whose blocking collision a player's capsule at centre meets, by label."""
    found = unreal.SystemLibrary.capsule_overlap_components(world, centre, PLAYER[0], PLAYER[1], OBJECTS, None, [])
    out = {}
    for c in found or []:
        owner = c.get_owner()
        if (owner and tag in owner.tags and c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN)
                == unreal.CollisionResponseType.ECR_BLOCK):
            out[str(owner.get_actor_label())] = owner
    return out


def sink_ledge_face(world, piece, label, where, tag):
    """Sinks a ledge's outer face under its path a step at a time until no capsule on the walkway meets it: how far it
    sank (cm), or None when even LEDGE_SINK_MOST wasn't enough (it's left sunk that far for the caller to move out)."""
    location = piece.get_actor_location()
    distance = 0.0
    while distance < LEDGE_SINK_MOST and any(label in blocking(world, c, tag) for c in where):
        distance += MOVE_STEP
        piece.set_actor_location(location - unreal.Vector(0.0, 0.0, distance), False, True)
    return None if any(label in blocking(world, c, tag) for c in where) else distance


def clear(build, tiles, terrain_hit):
    """Moves the cliff pieces out of the walkways' middles (see the module's docstring); returns how many moved."""
    corridors = ramp_corridors(build.source)
    if not corridors or not tiles:
        return 0
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    tag = unreal.Name(build.tag)
    moved = 0
    for ramp_id, path, half, walls in corridors:
        centres = samples(path, half, tiles, terrain_hit)
        hits = {}
        for centre in centres:
            for label, actor in blocking(world, centre, tag).items():
                if CLIFF_LABEL.match(label):
                    hits.setdefault(label, (actor, []))[1].append(centre)
        for label, (piece, where) in sorted(hits.items()):
            group = CLIFF_LABEL.match(label).group(1)
            rotation = piece.get_actor_rotation()
            forward, right = rotation.get_forward_vector(), rotation.get_right_vector()
            location = piece.get_actor_location()
            _, (cx, cy) = nearest(path, location.x, location.y)
            toward = (cx - location.x, cy - location.y)
            if group == walls:
                if forward.x * toward[0] + forward.y * toward[1] <= 0.0:
                    sunk = sink_ledge_face(world, piece, label, where, tag)
                    if sunk is not None:
                        build.log(f'{label} sunk {sunk:.0f} cm under the {ramp_id} walkway (a ledge\'s face)')
                        moved += 1
                        continue
                    # Too deep to sink: out from under the path, along its own facing, from where it sank to.
                    location = piece.get_actor_location()
                    way = (forward.x, forward.y)
                else:
                    way = (-forward.x, -forward.y)
            else:
                side = 1.0 if right.x * toward[0] + right.y * toward[1] < 0.0 else -1.0
                way = (right.x * side, right.y * side)
            norm = math.hypot(*way) or 1.0
            way = unreal.Vector(way[0] / norm, way[1] / norm, 0.0)
            distance = 0.0
            while distance < MOVE_MOST and any(label in blocking(world, c, tag) for c in where):
                distance += MOVE_STEP
                piece.set_actor_location(location + way * distance, False, True)
            if any(label in blocking(world, c, tag) for c in where):
                build.warn(f'{label} still stands in the {ramp_id} walkway {MOVE_MOST:.0f} cm off its place')
            build.log(f'{label} moved {distance:.0f} cm off the {ramp_id} walkway '
                      f'({"back into its wall" if group == walls else "along its run"})')
            moved += 1
    return moved
