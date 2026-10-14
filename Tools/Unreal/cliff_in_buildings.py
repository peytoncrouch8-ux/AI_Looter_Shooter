"""Read-only probe (prefix CLIFFIN): every cliff piece, outcrop or rock standing inside a building of the open level,
and every cliff face that dresses no terrain wall. Changes nothing, never saves. Run with the area's map open, no PIE:
    Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/cliff_in_buildings.py RansomsRest" -Until "CLIFFIN done"
Add "scatter" after the area to also test the scatter's instanced rocks at every building's points (slow).

Inside: each building (every placement the build stood, from layout_computed.json; level.props; the mine's portal; the
mine's pieces tagged Interior_Providence; not level.cliffs.under's platforms nor the rocks level.cliffs.abut runs end
in) is sampled as build_area_keepout.Rooms does: a 50 cm grid through its model's bounds, 75 cm apart up each column,
from 30 cm over the bounds' bottom to the model's topmost surface there (rooms, upper floors, the attic, a canopy, the
eaves; never the open ground round it nor the air over its roof). A rock (a StaticMeshActor of the build labelled
Cliff_*, Outcrop_* or *Talus*, or whose mesh is SM_Cliff*, SM_Outcrop*, SM_Rock*, SM_Boulder*) whose collision comes
within 35 cm of a point stands inside. One line per rock and building:
    CLIFFIN in <rock> (<mesh>) | <building> (<mesh>) [keepOut]: <n> points, x a..b y c..d z e..f
Free: every placed face (Cliff_<group>_<NN>, not a panel group's) whose dressing point in layout_computed.json has no
terrain wall within WALL_REACH (250 cm) behind it, by build_area.wall_behind, the test level.cliffs.walled makes:
    CLIFFIN free <group>: <n> of <m> placed points dress no wall: 01 (none), 02 (512 cm), ...
("none": no terrain met within 15 m behind; a number: how far back it was met.)
"""
import importlib
import json
import os
import re
import sys
import time

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
sys.path.append(os.path.join(PROJECT, 'Tools', 'Unreal'))
import build_area_keepout  # noqa: E402
import build_area  # noqa: E402

ROCK_LABEL = re.compile(r'^(Cliff_|Outcrop_)|Talus')
ROCK_MESH = ('SM_Cliff', 'SM_Outcrop', 'SM_Rock', 'SM_Boulder')
SCATTER_MESH = ('Rock', 'Boulder', 'Stone', 'Cliff', 'Outcrop')
GAMEPLAY_KINDS = ('PlayerStart', 'TargetDummy', 'GunRack', 'Landing')
MINE_TAG = 'Interior_Providence'


def say(text):
    unreal.log(f'CLIFFIN {text}')


def mesh_name(actor):
    for c in actor.get_components_by_class(unreal.StaticMeshComponent):
        if c.static_mesh is not None and not isinstance(c, unreal.InstancedStaticMeshComponent):
            return c.static_mesh.get_name()
    return '?'


def span(points):
    xs, ys, zs = [p.x for p in points], [p.y for p in points], [p.z for p in points]
    return (f'x {min(xs):.0f}..{max(xs):.0f} y {min(ys):.0f}..{max(ys):.0f} z {min(zs):.0f}..{max(zs):.0f} '
            f'(first {points[0].x:.0f}, {points[0].y:.0f}, {points[0].z:.0f})')


def main(area, scatter=False):
    keepout = importlib.reload(build_area_keepout)
    ba = importlib.reload(build_area)
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if editor.get_game_world():
        say('stopped: a play session is running (stop it first)')
        return
    world = editor.get_editor_world()
    folder = os.path.join(PROJECT, 'Art', 'Levels', area)
    with open(os.path.join(folder, 'layout.json')) as f:
        source = json.load(f)
    with open(os.path.join(folder, 'layout_computed.json')) as f:
        computed = json.load(f)
    level = source.get('level', {})
    map_path, tag = level.get('map', f'/Game/Maps/Lvl_{area}'), level.get('tag', f'{area}Build')
    open_map = world.get_path_name().split('.')[0]
    if open_map != map_path:
        say(f'stopped: the open map is {open_map}, not {map_path}')
        return
    look = level.get('cliffs', {})
    keep = set(look.get('keepOut', []))
    skip = set(look.get('under', [])) | {k for keys in look.get('abut', {}).values() for k in keys}
    names = ({k for k, s in computed.get('placements', {}).items() if s['kind'] not in GAMEPLAY_KINDS}
             | {p['id'] for p in level.get('props', [])} | {'MinePortal'})
    tagged = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
              if unreal.Name(tag) in a.tags]

    buildings, skipped = [], []
    for a in tagged:
        label = str(a.get_actor_label())
        if label in skip:
            skipped.append(label)
        elif label in names or unreal.Name(MINE_TAG) in a.tags:
            buildings.append(a)
    rocks = []
    for a in tagged:
        if not isinstance(a, unreal.StaticMeshActor) or unreal.Name(MINE_TAG) in a.tags:
            continue
        label, mesh = str(a.get_actor_label()), mesh_name(a)
        if label in names:
            continue
        if ROCK_LABEL.search(label) or mesh.startswith(ROCK_MESH):
            rocks.append(a)
    rooms = [r for r in (keepout.Rooms(a) for a in buildings) if r.box is not None]
    missing = sorted(keep - {r.label for r in rooms})
    say(f'{area}: {len(rooms)} buildings ({sum(1 for r in rooms if r.label in keep)} in level.cliffs.keepOut), '
        f'{len(rocks)} rocks; not sampled: {", ".join(sorted(skipped)) or "none"}'
        + (f'; keepOut names missing from the level: {", ".join(missing)}' if missing else ''))

    started, count, in_keep = time.time(), 0, 0
    pieces, places = set(), set()
    for rock in rocks:
        box = keepout.box_of(rock, keepout.KEEP_CLEAR)
        path = rock.get_path_name()
        for room in rooms:
            if room.building.get_path_name() == path or not keepout.boxes_meet(box, room.box):
                continue
            found = room.hits(world, rock, path, box)
            if not found:
                continue
            count += 1
            in_keep += room.label in keep
            pieces.add(str(rock.get_actor_label()))
            places.add(room.label)
            say(f'in {rock.get_actor_label()} ({mesh_name(rock)}) | {room.label} ({mesh_name(room.building)})'
                f'{" [keepOut]" if room.label in keep else ""}: {len(found)} points, {span(found)}')
    say(f'inside: {count} rock-in-building pairs ({in_keep} in keepOut buildings), {len(pieces)} rocks, '
        f'{len(places)} buildings ({time.time() - started:.1f} s)')

    if scatter:
        started, seen = time.time(), {}
        for room in rooms:
            for p in room.points():
                for c in unreal.SystemLibrary.sphere_overlap_components(world, p, keepout.KEEP_CLEAR,
                                                                        keepout.OBJECTS, None, []) or []:
                    if not isinstance(c, unreal.InstancedStaticMeshComponent) or c.static_mesh is None:
                        continue
                    name = c.static_mesh.get_name()
                    if any(word in name for word in SCATTER_MESH):
                        key = (room.label, name, str(c.get_owner().get_actor_label()) if c.get_owner() else '?')
                        seen.setdefault(key, []).append(p)
        for (label, name, owner), found in sorted(seen.items()):
            say(f'scatter {name} of {owner} | {label}: {len(found)} points, {span(found)}')
        say(f'scatter: {len(seen)} instanced rock meshes inside buildings ({time.time() - started:.1f} s)')

    tiles = ba.terrain_tiles(tag)
    if not tiles:
        say('free: no Ground tiles of the build to trace against')
        return
    panels = set(look.get('panels', {}))
    groups = computed.get('cliffs', {})
    placed = {}
    for a in tagged:
        match = keepout.CLIFF_LABEL.match(str(a.get_actor_label()))
        if match and isinstance(a, unreal.StaticMeshActor):
            placed.setdefault(match.group(1), set()).add(int(match.group(2)))
    total = 0
    walled = look.get('walled', [])
    for group, numbers in sorted(placed.items()):
        if group in panels or group not in groups:
            continue
        free = []
        for n in sorted(numbers):
            point = groups[group][n - 1] if 0 < n <= len(groups[group]) else None
            if point is None or point.get('kind'):
                continue
            x, y, bottom, height, yaw = ba.wall_span(point)
            behind = ba.wall_behind(tiles, x, y, bottom, height, yaw)
            if behind is None or behind > ba.WALL_REACH:
                free.append(f'{n:02d} ({"none" if behind is None else f"{behind:.0f} cm"})')
        if free:
            total += len(free)
            say(f'free {group}{" [walled]" if walled is True or group in walled else ""}: {len(free)} of '
                f'{len(numbers)} placed points dress no wall: {", ".join(free)}')
    say(f'free: {total} placed faces dress no terrain wall')


args = [a for a in sys.argv[1:] if a]
try:
    main(next((a for a in args if a != 'scatter'), 'RansomsRest'), scatter='scatter' in args)
finally:
    say('done')
