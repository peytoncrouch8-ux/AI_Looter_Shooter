"""Ransom's Rest's mountain (Docs/Areas/RansomsRest_Expansion.md, section 6 "The mountain"): what stands up the ore lift
besides the buildings layout.json places, for build_area.py's whole build (after models(): the buildings stand, their
sockets with them) or on its own in the open editor:
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_mountain.py RansomsRest"
which clears the area's Mountain folder, places it again and saves the level.

The buildings are layout.json placements (build_area.py's models() stands each as SM_<kind>; patches/mountain.json adds
any the terrain's layout doesn't have): the lift's RR_LiftHouse, RR_LiftTower and RR_LiftHead (build_area_lift.py rigs
the lift on them), Calder's Post's RR_Watchtower, RR_RangerCabin, RR_LeanTo and RR_Outhouse, the Diggings' RR_OreBin,
RR_PowderHouse and RR_Smithy, and the mine's portal RR_MinePortal at the mine's origin. This adds:
- The Providence (build_area_mine.py): its kit in the mine's frame, its lanterns and lights, the Foreman's timber props
  and rubble, the interior's visibility and the minimap's underground.
- The portal: SM_RR_MinePortal at the mine's origin when no placement stands it (one that stands elsewhere is moved there:
  its pivot is the origin), and the tag board's eleven tags as story props on the portal's transform: on IN
  (SM_RR_TagBoard_TagsIn) until Back Pay pays them, on OUT (SM_RR_TagBoard_TagsOut) after (the story flag BackPay.Paid).
- The track out of the mouth onto the apron (three lengths and a bumper, an ore car left on it), and the ore tramway
  across the Bench: the layout's road oreTramway from the ore bin's SOCKET_TrackBottom to the lift head, laid on the
  terrain in the kit's 4 m and 2 m lengths with a bumper at its end (AInstancedProps, one per piece), two ore cars off its
  rails ("two ore cars off the rails"). The ore bin's trestle (SOCKET_TrackTop) wants ground 3.6 m over the yard at its
  north end: the terrain's to give (a warning says how far off it is).
- Calder's Post's board: the RIM RANGERS board (SM_NoticeBoard) at the cabin's door (SOCKET_NoticeBoard); its use
  (NoticeBoard_CalderPost) is build_area_missionprops_mountain.py's.
- HeavyOutside_Providence on the Bench's buildings and the tramway: hidden while the player is in the mine (none of them
  is seen through the mouth from past the portal's box).
Everything goes in the area's Mountain folder (Mine, Diggings, Tramway, CaldersPost); a missing model, socket, placement or
class is warned of and left out. Only a level whose layout has the mountain (its lift's or mine's placements, or
gameplay.mountain) gets any of it.

Hook in build_area.py's run(), the whole build, right after self.models(meshes):
    importlib.reload(importlib.import_module('build_area_mountain')).place(self, meshes)
"""
import importlib
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Mountain'
DIGGINGS = 'Mountain/Diggings'
TRAMWAY = 'Mountain/Tramway'
POST = 'Mountain/CaldersPost'

PAID_FLAG = 'BackPay.Paid'

# The mountain's placement kinds (layout.json), and the ones hidden from inside the mine (section 6 "Performance").
MOUNTAIN_KINDS = ('RR_LiftHouse', 'RR_LiftTower', 'RR_LiftHead', 'RR_Watchtower', 'RR_RangerCabin', 'RR_LeanTo',
                  'RR_OreBin', 'RR_PowderHouse', 'RR_Smithy', 'RR_MinePortal')
HEAVY_KINDS = ('RR_LiftHouse', 'RR_LiftTower', 'RR_LiftHead', 'RR_Watchtower', 'RR_RangerCabin', 'RR_LeanTo', 'RR_Outhouse',
               'RR_Home_G', 'RR_OreBin', 'RR_PowderHouse', 'RR_Smithy')
# Only what stands north of the valley's floor (the farm has an outhouse too): layout X past this (cm).
MOUNTAIN_FROM_X = 9000.0

# The tramway kit (Art/Models/Props/Tramway.py): a straight runs from its origin along its +X; the bumper stands on the
# last length's end, the same way round.
LENGTHS = (('RR_TramTrack_4m', 400.0), ('RR_TramTrack_2m', 200.0))
BUMPER = 'RR_TramTrack_End'
ORE_CAR = 'RR_OreCar'
TRAMWAY_ROAD = 'oreTramway'
# The cars off the rails: (share of the way along, sideways off the track (cm), turn off it, roll): one slewed off, one
# on its side (its pivot then half its width up).
CARS = ((0.42, 150.0, 22.0, 0.0), (0.74, -170.0, -35.0, -90.0))
ON_ITS_SIDE_UP = 50.0
# Out of the mouth onto the apron: this many 4 m lengths south from the mouth's floor, the car on the last.
SPUR_LENGTHS = 3
# Instanced track pieces are drawn this far (cm; Medium draws 60%): the line toward the mine is seen from the lift head.
TRACK_CULL = 15000.0
# Where the ground is looked for round the mine's floor out of the mouth (cm): never the notch's top over the portal.
NEAR_FLOOR = 300.0
# The trestle's top wants ground about this close under it (cm).
TRESTLE_SLACK = 60.0

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def is_mountain(build):
    return bool(build.settings.get('mountain')) or any(
        spot['kind'] in MOUNTAIN_KINDS for spot in build.layout.get('placements', {}).values())


def placed_all(build, kinds, from_x=MOUNTAIN_FROM_X):
    """The actors build_area.py placed for the mountain's placements of these kinds (labelled with their keys)."""
    keys = {key for key, spot in build.layout.get('placements', {}).items()
            if spot['kind'] in kinds and spot['location'][0] >= from_x}
    return [a for a in actors.get_all_level_actors()
            if str(a.get_actor_label()) in keys and unreal.Name(build.tag) in a.tags]


_TILES = {}


def tiles(build):
    import build_area
    if build.tag not in _TILES:
        _TILES[build.tag] = build_area.terrain_tiles(build.tag)
    return _TILES[build.tag]


def terrain_z(build, x, y, near=None, reach=NEAR_FLOOR):
    """The terrain's height under (x, y) (its tiles alone: never a building, a rock or the scatter), within reach of near
    when given, else the topmost; None when there's none."""
    import build_area
    top, bottom = (30000.0, -30000.0) if near is None else (near + reach, near - reach)
    hit = build_area.terrain_hit(tiles(build), unreal.Vector(x, y, top), unreal.Vector(x, y, bottom))
    return None if hit is None else hit.z


def along(path, s):
    """The point s cm along a polyline (x, y), and the way it runs there (unit x, y)."""
    for (x0, y0), (x1, y1) in zip(path, path[1:]):
        length = math.hypot(x1 - x0, y1 - y0)
        if length <= 1e-6:
            continue
        if s <= length:
            t = s / length
            return (x0 + (x1 - x0) * t, y0 + (y1 - y0) * t), ((x1 - x0) / length, (y1 - y0) / length)
        s -= length
    (x0, y0), (x1, y1) = path[-2], path[-1]
    length = max(math.hypot(x1 - x0, y1 - y0), 1e-6)
    return (x1, y1), ((x1 - x0) / length, (y1 - y0) / length)


def path_length(path):
    return sum(math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(path, path[1:]))


def lay_track(path, height, last_piece=True):
    """The track's lengths along a polyline (x, y cm), each on the ground at its start and pitched to the ground at its
    end (height(x, y) -> z, or None): [(piece, (x, y, z), pitch, yaw)], the bumper last."""
    total, s, laid = path_length(path), 0.0, []
    at, _ = along(path, 0.0)
    z = height(*at)
    if z is None:
        return laid
    while True:
        left = total - s
        piece = next(((name, length) for name, length in LENGTHS if left >= length - 25.0), None)
        if piece is None:
            break
        name, length = piece
        target, _ = along(path, s + length)
        dx, dy = target[0] - at[0], target[1] - at[1]
        flat = max(math.hypot(dx, dy), 1e-6)
        ux, uy = dx / flat, dy / flat
        end = (at[0] + ux * length, at[1] + uy * length)
        z_end = height(*end)
        z_end = z if z_end is None else z_end
        pitch = math.degrees(math.atan2(z_end - z, length))
        yaw = math.degrees(math.atan2(uy, ux))
        laid.append((name, (at[0], at[1], z), pitch, yaw))
        horizontal = length * math.cos(math.radians(pitch))
        at = (at[0] + ux * horizontal, at[1] + uy * horizontal)
        z = z + length * math.sin(math.radians(pitch))
        s += length
    if laid and last_piece:
        laid.append((BUMPER, (at[0], at[1], z), laid[-1][2], laid[-1][3]))
    return laid


def transform(location, pitch, yaw, roll=0.0):
    return unreal.Transform(location=unreal.Vector(*location), rotation=unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw),
                            scale=unreal.Vector(1.0, 1.0, 1.0))


def stand_track(build, laid, meshes, folder, label, tags):
    """The laid lengths as one AInstancedProps per piece (solid, shadowed), else as plain meshes. How many stood."""
    import build_area_mine as mine_kit
    cls = story.actor_class('InstancedProps')
    by_piece = {}
    for name, location, pitch, yaw in laid:
        by_piece.setdefault(name, []).append(transform(location, pitch, yaw))
    stood = 0
    for name, transforms in sorted(by_piece.items()):
        mesh = mine_kit.mesh_named(meshes, name)
        short = name.replace('RR_TramTrack_', '')
        if mesh is None:
            build.warn(f'no SM_{name} (import Art/Models/Props/Tramway.py): {len(transforms)} of {label} left out')
            continue
        if cls is None:
            for i, t in enumerate(transforms):
                r = t.rotation.rotator()
                actor = build.place(mesh, t.translation.to_tuple(), r.yaw, label=f'{label}_{short}_{i + 1:02d}',
                                    folder=folder, tags=tags)
                actor.set_actor_rotation(r, False)
            stood += len(transforms)
            continue
        actor = build.place(cls, (0.0, 0.0, 0.0), label=f'{label}_{short}', folder=folder, tags=tags)
        actor.set_editor_property('cull_distance', TRACK_CULL)
        actor.set_editor_property('solid', True)
        actor.set_editor_property('cast_shadows', True)
        actor.set_instances(mesh, transforms)
        stood += len(transforms)
    return stood


def place_car(build, meshes, location, pitch, yaw, roll, label, folder, tags=('Obstacle',)):
    import build_area_mine as mine_kit
    mesh = mine_kit.mesh_named(meshes, 'SM_' + ORE_CAR)
    if mesh is None:
        build.warn(f'no SM_{ORE_CAR} (import Art/Models/Props/Tramway.py): no {label}')
        return None
    actor = build.place(mesh, location, yaw, label=label, folder=folder, tags=tags)
    actor.set_actor_rotation(unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw), False)
    return actor


# ---------------------------------------------------------------------------
# The Diggings: the portal, its tags, the track
# ---------------------------------------------------------------------------

def place_portal(build, mine, meshes):
    """The portal at the mine's origin (moved there if a placement stood it elsewhere), and the tag board's tags."""
    import build_area_mine as mine_kit
    import build_area_missionprops_mountain as props
    turn = mine.turn(180.0)
    standing = placed_all(build, ('RR_MinePortal',), from_x=0.0)
    if standing:
        portal = standing[0]
        at = portal.get_actor_location()
        if (at - unreal.Vector(*mine.origin)).length() > 5.0:
            portal.set_actor_location(unreal.Vector(*mine.origin), False, True)
            portal.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=turn), False)
            build.log(f'the portal ({portal.get_actor_label()}) moved onto the mine\'s origin')
    else:
        mesh = mine_kit.mesh_named(meshes, 'SM_RR_MinePortal')
        if mesh is None:
            build.warn('no SM_RR_MinePortal (import Art/Models/Buildings/Diggings.py): the mouth stands open')
        else:
            build.place(mesh, mine.origin, turn, label='MinePortal', folder=DIGGINGS, tags=('Obstacle',))
            build.log('the portal at the mine\'s origin (no RR_MinePortal placement)')
    cls = story.actor_class('StoryProp')
    for asset, shown, label in (('SM_RR_TagBoard_TagsIn', props.when(without_flags=[PAID_FLAG]), 'TagBoard_TagsIn'),
                                ('SM_RR_TagBoard_TagsOut', props.when(with_flags=[PAID_FLAG]), 'TagBoard_TagsOut')):
        mesh = mine_kit.mesh_named(meshes, asset)
        if mesh is None:
            build.warn(f'no {asset} (import Art/Models/Buildings/Diggings.py): no {label}')
            continue
        if cls is None:
            if label.endswith('In'):
                build.place(mesh, mine.origin, turn, label=label, folder=DIGGINGS)
            build.warn(f'no StoryProp class (build the game module first): {label} '
                       f'{"hangs for good" if label.endswith("In") else "left out"}')
            continue
        tags = build.place(cls, mine.origin, turn, label=label, folder=DIGGINGS)
        tags.get_editor_property('mesh').set_static_mesh(mesh)
        tags.set_editor_property('shown_when', [shown])
    build.log(f'the tag board\'s tags: on IN until {PAID_FLAG}, on OUT after')


def place_spur(build, mine, meshes):
    """Three lengths of track south out of the mouth onto the apron, a bumper and an ore car left on them."""
    floor = mine.origin[2]
    start = mine.at(0.0, 0.0)[:2]
    end = mine.at(0.0, -4.0 * SPUR_LENGTHS - 0.1)[:2]

    def height(x, y):
        found = terrain_z(build, x, y, near=floor)
        # Inside the portal's entry the floor is the mine's (the terrain there is the notch's top).
        return floor if found is None or math.hypot(x - start[0], y - start[1]) < 360.0 else found
    laid = lay_track([start, end], height)
    stood = stand_track(build, laid, meshes, DIGGINGS, 'MouthTrack', ('Obstacle',))
    if len(laid) >= 2:
        name, (x, y, z), pitch, yaw = laid[-2]
        ux, uy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        place_car(build, meshes, (x + ux * 230.0, y + uy * 230.0, z), pitch, yaw, 0.0, 'OreCar_Mouth', DIGGINGS)
    build.log(f'the track out of the mouth: {stood} pieces')


def tramway_path(build):
    """The tramway's line: from the ore bin's SOCKET_TrackBottom (else the road's start) along the layout's road."""
    road = next((r for r in build.source.get('roads', []) if r.get('id') == TRAMWAY_ROAD), None)
    if road is None or len(road.get('path', [])) < 2:
        build.warn(f'the layout has no road {TRAMWAY_ROAD}: no ore tramway across the Bench')
        return None, None
    path = [tuple(p[:2]) for p in road['path']]
    bins = placed_all(build, ('RR_OreBin',))
    if not bins:
        build.warn('no RR_OreBin placed: the tramway starts at its road\'s first point')
        return path, None
    ore_bin = bins[0]
    bottom = story.socket(ore_bin, 'TrackBottom')
    if bottom is None:
        build.warn(f'{ore_bin.get_actor_label()} has no SOCKET_TrackBottom: the tramway starts at its road\'s first point')
        return path, ore_bin
    at = bottom.translation
    # The road's first point stands in for the socket: the line runs from under the chute.
    return [(at.x, at.y)] + path[1:], ore_bin


def check_trestle(build, ore_bin):
    """The bin's trestle top (SOCKET_TrackTop) wants ground under it: how far off it is, as a warning to the terrain."""
    top = story.socket(ore_bin, 'TrackTop') if ore_bin is not None else None
    if top is None:
        return
    at = top.translation
    ground = terrain_z(build, at.x, at.y)
    if ground is not None and at.z - ground > TRESTLE_SLACK:
        build.warn(f'the ore bin\'s trestle top at ({at.x:.0f}, {at.y:.0f}) stands {(at.z - ground) / 100.0:.1f} m over the '
                   f'ground: the terrain wants a step up to it there (Diggings.py: "about 3.6 m higher")')


def place_tramway(build, meshes):
    """The ore tramway across the Bench, laid on the terrain, and its two cars off the rails."""
    path, ore_bin = tramway_path(build)
    check_trestle(build, ore_bin)
    if not path:
        return
    laid = lay_track(path, lambda x, y: terrain_z(build, x, y))
    if not laid:
        build.warn('no terrain under the tramway\'s start: no ore tramway')
        return
    import build_area_mine as mine_kit
    stood = stand_track(build, laid, meshes, TRAMWAY, 'Tramway', (mine_kit.OUTSIDE_TAG,))
    total = path_length(path)
    for n, (share, side, turn, roll) in enumerate(CARS):
        (x, y), (ux, uy) = along(path, total * share)
        z = terrain_z(build, x + uy * side, y - ux * side)
        if z is None:
            continue
        yaw = math.degrees(math.atan2(uy, ux)) + turn
        place_car(build, meshes, (x + uy * side, y - ux * side, z + (ON_ITS_SIDE_UP if roll else 0.0)), 0.0, yaw, roll,
                  f'OreCar_Tramway_{n + 1}', TRAMWAY, tags=('Obstacle', mine_kit.OUTSIDE_TAG))
    build.log(f'the ore tramway across the Bench: {stood} pieces over {total / 100.0:.0f} m, {len(CARS)} cars off its rails')


# ---------------------------------------------------------------------------
# Calder's Post's board, the heavy outside, the buildings
# ---------------------------------------------------------------------------

def place_post_board(build, meshes):
    """The RIM RANGERS board at Ruth's cabin door (the cabin's SOCKET_NoticeBoard)."""
    import build_area_mine as mine_kit
    cabins = placed_all(build, ('RR_RangerCabin',))
    if not cabins:
        build.warn('no RR_RangerCabin placed: Calder\'s Post has no board')
        return
    spot = story.socket(cabins[0], 'NoticeBoard')
    mesh = mine_kit.mesh_named(meshes, 'SM_NoticeBoard')
    if spot is None or mesh is None:
        build.warn('the cabin has no SOCKET_NoticeBoard or there\'s no SM_NoticeBoard: Calder\'s Post has no board')
        return
    at = spot.translation
    build.place(mesh, (at.x, at.y, at.z), spot.rotation.rotator().yaw, label='CalderPost_BoardModel', folder=POST,
                tags=('Obstacle', mine_kit.OUTSIDE_TAG))
    build.log(f'Calder\'s Post\'s board at the cabin door ({at.x:.0f}, {at.y:.0f})')


def tag_heavy(build):
    """HeavyOutside_Providence on the Bench's and the lift's buildings: hidden while the player is in the mine."""
    import build_area_mine as mine_kit
    tagged = placed_all(build, HEAVY_KINDS)
    for actor in tagged:
        mine_kit.add_tags(actor, mine_kit.OUTSIDE_TAG)
    build.log(f'{len(tagged)} of the mountain\'s buildings tagged {mine_kit.OUTSIDE_TAG}')


def check_buildings(build):
    """Which of the mountain's buildings the layout places, and which it doesn't (patches/mountain.json has them)."""
    kinds = {spot['kind'] for spot in build.layout.get('placements', {}).values()}
    missing = [kind for kind in MOUNTAIN_KINDS if kind not in kinds and kind != 'RR_MinePortal']
    if missing:
        build.warn(f'layout.json places no {", ".join(missing)} (apply Art/Levels/RansomsRest/patches/mountain.json and run '
                   'the terrain model with --computed): they\'re left out (the lift stands its own greyboxes)')


# ---------------------------------------------------------------------------
# All of it
# ---------------------------------------------------------------------------

def place(build, meshes):
    """The mountain's mine, portal, tracks, board and tags (build_area.py's whole build calls this after models())."""
    if not is_mountain(build):
        return
    _TILES.clear()
    for name in ('build_area_mine', 'build_area_missionprops_mountain'):
        importlib.reload(importlib.import_module(name))
    import build_area_mine as mine_kit
    check_buildings(build)
    mine = mine_kit.place(build, meshes)
    if mine is not None:
        place_portal(build, mine, meshes)
        place_spur(build, mine, meshes)
    place_tramway(build, meshes)
    place_post_board(build, meshes)
    tag_heavy(build)
    build.log('the mountain placed')


def run(name):
    """Places it again on its own (clears the Mountain folder), saves."""
    import build_area
    build = importlib.reload(build_area).AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build, build_area.mesh_index())
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the mountain placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_mountain.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
