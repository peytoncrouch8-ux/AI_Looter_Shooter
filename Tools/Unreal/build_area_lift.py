"""The ore lift, the watch fire and the far answering light on Ransom's Rest (Docs/Areas/RansomsRest_Expansion.md: section 4,
"Dead Lift" and "The Last Watch"; section 6, "The ore lift" and "Calder's Post"), for build_area.py's gameplay pass: placed
from layout.json's placements and the mountain art's sockets (Art/Models/Buildings/OreLift.py and CalderPost.py), so they
follow the level whenever it's rebuilt. Only the level whose layout has Ellis's grave (Ransom's Rest) gets any of it.
Everything goes in the area's Gameplay/Lift folder, which a gameplay build clears with the rest of Gameplay.

- AOreLift (label OreLift, tagged OreLift) where the cage rests at the bottom: the winch house's SOCKET_Cage (SM_RR_LiftHouse,
  placement kind RR_LiftHouse). Its TopRest is the lift head's SOCKET_Cage (SM_RR_LiftHead), its TowerSheave the tower's
  SOCKET_Sheave (SM_RR_LiftTower), its Winch the house's SOCKET_Winch; the cage and cable models (SM_RR_LiftCage,
  SM_RR_LiftCable) once imported. It opens with Dead Lift's ride (the C++ defaults: DeadLift, from its step 3).
- Two AOreLiftBellPull (LiftBell_Bottom, LiftBell_Top) on the house's and the head's SOCKET_BellPull, a hand in front of the
  rope.
- The speaking tube at the lift head (an ASpeakerPoint tagged Speaker_LiftTube, Hiram Coyle's voice) on its
  SOCKET_SpeakingTube: Dead Lift's turn-in. Its words come with the missions' data (build step 4).
- AWatchFire (WatchFire, tagged WatchFire for The Last Watch's held Interact) on the watchtower's SOCKET_WatchFire
  (SM_RR_Watchtower).
- AFarSignalLight (FarSignalLight) on the nearest backdrop mesa down the Rim (layout region.backdrop's nearest layer, its
  plain mesa nearest bearing 215), a little in front of its face and under its top; gameplay.lift.farLight [X, Y, Z] in
  the layout overrides it.

Missing art is tolerated: a piece whose model (or placement) isn't there stands where the design puts it, with a greybox
stand-in for the missing building (the engine's cube and cylinder, labelled StandIn_*, in Gameplay/Lift/StandIns) and a
warning, so the lift can be ridden before the mountain is modelled.

The sockets the art gives (each an empty SOCKET_<Name>; its front is the empty's -Y, its top +Z):
  SM_RR_LiftHouse: Cage (the cage's pivot resting on the landing: its floor's middle, facing out of its gate over the
    valley), BellPull (the cord's handle), Winch (the drum, where the winch is heard), SpeakingTube.
  SM_RR_LiftTower: Sheave (the top of the sheave train's middle wheel, where the cable rides over).
  SM_RR_LiftHead: Cage, BellPull, SpeakingTube, ToolBox.
  SM_RR_LiftCage (its pivot the floor's middle, its front out of its gate): Grip (the top of its bail, on the cable),
    Rider (where the rider stands).
  SM_RR_LiftCable: a short straight length of 32 mm haul rope along its longest side (stretched along each span).
  SM_RR_Watchtower: WatchFire (the brazier's fire bed's middle).

From build_area_story.place() after Main 7's pieces (reloaded each run, as the editor keeps modules between runs):
    importlib.reload(importlib.import_module('build_area_lift')).place(build)
On its own, in the open editor (clears the Gameplay/Lift folder, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_lift.py RansomsRest"
Without the game module's classes yet it places nothing and says so.
"""
import json
import math
import os
import sys

import unreal

# Its neighbours (build_area_story, build_area) by their folder, also when it runs on its own in a fresh editor.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Lift'
STANDINS = 'Gameplay/Lift/StandIns'

# The mountain art's placement kinds (layout.json placements: build_area.py's models() places each as SM_<kind>) and models.
HOUSE = 'RR_LiftHouse'
TOWER = 'RR_LiftTower'
HEAD = 'RR_LiftHead'
WATCHTOWER = 'RR_Watchtower'
CAGE_MESH = 'SM_RR_LiftCage'
CABLE_MESH = 'SM_RR_LiftCable'
ART_FOLDERS = ('/Game/Art/Buildings', '/Game/Art/Props', '/Game/Art')


def design(x_east, y_north, height=None):
    """A spot in the design's metres (x east, y north, heights from the valley's base) as the layout's cm (X north, Y east)."""
    return (y_north * 100.0, x_east * 100.0, None if height is None else height * 100.0)


# The design's spots (section 6), where a piece stands while its model isn't there.
LANDING = design(20, 114)            # the cage's landing on the quarry floor, under the head sheave
WINCH_HOUSE = design(28, 110)        # the winch house
TOWER_SPOT = design(23, 126)         # the timber tower on the ridge face
SHEAVE_HEIGHT = 3000.0               # its sheaves, near +30 m
HEAD_SPOT = design(27, 146, 47)      # the lift head's landing, +47 m, behind the crest
WATCHTOWER_SPOT = design(-72, 160)   # Calder's watchtower on the Rim's edge
DECK_HEIGHT = 1200.0                 # its deck, 12 m over the knoll
BRAZIER_UP = 90.0                    # the fire bed over the deck
WINCH_UP = 250.0                     # the drum over the house's floor

# The cage faces out of its gate over the valley (south: the layout's -X) at both landings, without the art's sockets.
CAGE_YAW = 180.0
# A bell pull without its socket: beside the landing, to the cage's right and at a hand's height; its handle a hand in
# front of the rope (cm).
BELL_SIDE = 180.0
BELL_UP = 150.0
BELL_OUT = 10.0
# The speaking tube without its socket: beside the head's bell pull (cm); spoken into from close.
TUBE_SIDE = 120.0
TUBE_REACH = 250.0
TUBE_TAG = 'Speaker_LiftTube'

# The far light: "far down the Rim" from Calder's Post, south-south-west; on the nearest backdrop layer's plain mesa nearest
# this bearing (degrees, 0 north, 90 east), pulled toward the valley so it stands in front of the mesa's face, under its top.
DOWN_THE_RIM = 215.0
FAR_LIGHT_IN = 3000.0
FAR_LIGHT_DOWN = 250.0
DIRECTIONS = ('north', 'east', 'south', 'west')

ENGINE_CUBE = '/Engine/BasicShapes/Cube'
ENGINE_CYLINDER = '/Engine/BasicShapes/Cylinder'


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def built(build, kind):
    """The model build_area.py's models() placed for the first placement of this kind (labelled with its key), or None."""
    keys = [key for key, spot in build.layout['placements'].items() if spot['kind'] == kind]
    if not keys:
        return None
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if str(actor.get_actor_label()) == keys[0] and unreal.Name(build.tag) in actor.tags:
            return actor
    build.warn(f'no {kind} placed for {keys[0]} (build the whole level first)')
    return None


def find_mesh(name):
    """A static mesh by its asset name in the art's folders, or None."""
    for folder in ART_FOLDERS[:-1]:
        path = f'{folder}/{name}'
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.load_asset(path)
    for path in unreal.EditorAssetLibrary.list_assets(ART_FOLDERS[-1], recursive=True, include_folder=False):
        if path.rsplit('/', 1)[-1].split('.')[0] == name:
            return unreal.load_asset(path.split('.')[0])
    return None


def ground(x, y, default=0.0):
    return story.ground_at(x, y, default, warn=False)


def spot_transform(x, y, z, yaw):
    return unreal.Transform(location=unreal.Vector(x, y, z), rotation=unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw),
                            scale=unreal.Vector(1.0, 1.0, 1.0))


def standin(build, shape, location, size, yaw, label):
    """A greybox stand-in for a building whose model isn't there yet: the engine's cube or cylinder (100 cm a side) sized
    to size (cm), its middle at location."""
    mesh = unreal.load_asset(ENGINE_CUBE if shape == 'cube' else ENGINE_CYLINDER)
    actor = build.place(mesh, location, yaw, label=f'StandIn_{label}', folder=STANDINS,
                        scale=tuple(s / 100.0 for s in size), tags=('Obstacle',))
    return actor


def relative_location(frame, location):
    return unreal.MathLibrary.inverse_transform_location(frame, location)


def classes():
    """The game module's classes, or None (with a warning) when it isn't built yet."""
    found = {name: story.actor_class(name) for name in ('OreLift', 'OreLiftBellPull', 'SpeakerPoint', 'WatchFire', 'FarSignalLight')}
    missing = [name for name, cls in found.items() if cls is None]
    return found, missing


# ---------------------------------------------------------------------------
# The lift
# ---------------------------------------------------------------------------

def landing_rest(build, model, fallback, label):
    """The cage's rest at a landing (world): the model's SOCKET_Cage, else the design's spot facing the valley."""
    if model is not None:
        rest = story.socket(model, 'Cage')
        if rest is not None:
            return rest
        build.warn(f'{model.get_actor_label()} has no Cage socket: the cage rests at the design\'s {label} landing')
    x, y, z = fallback
    return spot_transform(x, y, ground(x, y) if z is None else max(z, ground(x, y, z)), CAGE_YAW)


def place_line_standins(build, house, tower, head, bottom, sheave, top):
    """Greyboxes for whatever of the line's buildings isn't there: the winch house's box, the tower's post and arm, the
    head's platform; each warned about."""
    if house is None:
        x, y, _ = WINCH_HOUSE
        z = ground(x, y, bottom.translation.z)
        standin(build, 'cube', (x, y, z + 200.0), (800.0, 1000.0, 400.0), CAGE_YAW, 'LiftHouse')
        build.warn(f'no SM_{HOUSE} placed: a greybox winch house stands at the design\'s spot')
    if tower is None:
        foot = ground(sheave.x, sheave.y, 0.0)
        height = max(sheave.z - foot, 200.0)
        standin(build, 'cylinder', (sheave.x, sheave.y, foot + height * 0.5 - 60.0), (60.0, 60.0, height - 120.0), 0.0, 'LiftTowerPost')
        standin(build, 'cube', (sheave.x, sheave.y, sheave.z - 80.0), (80.0, 260.0, 30.0), 0.0, 'LiftTowerArm')
        build.warn(f'no SM_{TOWER} placed: a greybox tower stands at the design\'s spot')
    if head is None:
        at = top.translation
        standin(build, 'cube', (at.x, at.y, at.z - 20.0), (500.0, 500.0, 40.0), CAGE_YAW, 'LiftHeadLanding')
        build.warn(f'no SM_{HEAD} placed: a greybox landing stands at the design\'s lift head')


def place_lift(build, cls):
    """The lift at the bottom landing, its line set from the art's sockets (stand-ins where a model is missing). Returns
    (lift, the winch house or None, the lift head or None, the bottom landing's rest, the top's)."""
    house, tower, head = built(build, HOUSE), built(build, TOWER), built(build, HEAD)
    bottom = landing_rest(build, house, LANDING, 'bottom')
    top = landing_rest(build, head, HEAD_SPOT, 'top')
    sheave_socket = story.socket(tower, 'Sheave') if tower is not None else None
    if sheave_socket is not None:
        sheave = sheave_socket.translation
    else:
        if tower is not None:
            build.warn(f'{tower.get_actor_label()} has no Sheave socket: the cable rides over the design\'s tower spot')
        x, y, _ = TOWER_SPOT
        sheave = unreal.Vector(x, y, SHEAVE_HEIGHT)
    place_line_standins(build, house, tower, head, bottom, sheave, top)

    at = bottom.translation
    lift = build.place(cls, (at.x, at.y, at.z), bottom.rotation.rotator().yaw, label='OreLift', folder=FOLDER, tags=('OreLift',))
    frame = lift.get_actor_transform()
    lift.set_editor_property('bottom_rest', unreal.Transform())
    lift.set_editor_property('top_rest', unreal.MathLibrary.make_relative_transform(top, frame))
    lift.set_editor_property('tower_sheave', relative_location(frame, sheave))
    winch = story.socket(house, 'Winch') if house is not None else None
    if winch is not None:
        drum = winch.translation
    else:
        x, y, _ = WINCH_HOUSE
        drum = unreal.Vector(x, y, ground(x, y, at.z) + WINCH_UP)
    lift.get_editor_property('winch').set_relative_location(relative_location(frame, drum), False, True)
    for prop, name in (('cage_mesh', CAGE_MESH), ('cable_mesh', CABLE_MESH)):
        mesh = find_mesh(name)
        if mesh is not None:
            lift.set_editor_property(prop, mesh)
            if prop == 'cable_mesh':
                # The art's rope has its own material: the stand-in cylinder's iron goes.
                lift.set_editor_property('cable_material', None)
        else:
            build.warn(f'no {name} yet: the lift runs with its stand-in {"cage" if prop == "cage_mesh" else "cable"}')
    top_at = top.translation
    build.log(f'the ore lift from ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}) over the tower at ({sheave.x:.0f}, {sheave.y:.0f}, '
              f'{sheave.z:.0f}) to ({top_at.x:.0f}, {top_at.y:.0f}, {top_at.z:.0f})')
    return lift, house, head, bottom, top


def bell_spot(build, model, rest, label):
    """Where a landing's bell cord is handled (world location) and the way it faces: the model's SOCKET_BellPull with the
    handle a hand in front of the rope, else beside the landing at a hand's height."""
    socket = story.socket(model, 'BellPull') if model is not None else None
    if socket is not None:
        front = socket.rotation.rotator().get_forward_vector()
        at = socket.translation
        return unreal.Vector(at.x + front.x * BELL_OUT, at.y + front.y * BELL_OUT, at.z + front.z * BELL_OUT), socket.rotation.rotator().yaw
    if model is not None:
        build.warn(f'{model.get_actor_label()} has no BellPull socket: the {label} bell pull stands beside the landing')
    return unreal.MathLibrary.transform_location(rest, unreal.Vector(0.0, BELL_SIDE, BELL_UP)), rest.rotation.rotator().yaw


def place_bell_pulls(build, cls, lift, house, head, bottom, top):
    for model, rest, end, label in ((house, bottom, unreal.OreLiftEnd.BOTTOM, 'Bottom'), (head, top, unreal.OreLiftEnd.TOP, 'Top')):
        at, yaw = bell_spot(build, model, rest, label.lower())
        pull = build.place(cls, (at.x, at.y, at.z), yaw, label=f'LiftBell_{label}', folder=FOLDER, tags=(f'LiftBell_{label}',))
        pull.set_editor_property('lift', lift)
        pull.set_editor_property('end', end)
    build.log('the bell pulls at both landings')


def place_speaking_tube(build, cls, head, top):
    """The lift head's speaking tube, Dead Lift's turn-in: Hiram's voice out of it."""
    socket = story.socket(head, 'SpeakingTube') if head is not None else None
    if socket is not None:
        at, yaw = socket.translation, socket.rotation.rotator().yaw
    else:
        if head is not None:
            build.warn(f'{head.get_actor_label()} has no SpeakingTube socket: the tube stands beside the bell pull')
        at = unreal.MathLibrary.transform_location(top, unreal.Vector(0.0, BELL_SIDE + TUBE_SIDE, BELL_UP))
        yaw = top.rotation.rotator().yaw
    story.speaker(build, cls, (at.x, at.y, at.z), yaw, 'SpeakingTube_LiftHead', TUBE_TAG, name='Hiram Coyle',
                  prompt='Speak into the tube', reach=TUBE_REACH)
    build.log(f'the speaking tube at the lift head ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}), tagged {TUBE_TAG}')


# ---------------------------------------------------------------------------
# Calder's Post's watch fire, and the far answer
# ---------------------------------------------------------------------------

def place_watch_fire(build, cls):
    tower = built(build, WATCHTOWER)
    socket = story.socket(tower, 'WatchFire') if tower is not None else None
    if socket is not None:
        at, yaw = socket.translation, socket.rotation.rotator().yaw
    else:
        x, y, _ = WATCHTOWER_SPOT
        foot = ground(x, y, 5000.0)
        if tower is not None:
            build.warn(f'{tower.get_actor_label()} has no WatchFire socket: the fire burns over the design\'s deck')
        else:
            standin(build, 'cube', (x, y, foot + DECK_HEIGHT * 0.5), (400.0, 400.0, DECK_HEIGHT), 0.0, 'Watchtower')
            build.warn(f'no SM_{WATCHTOWER} placed: a greybox tower stands at the design\'s spot, the fire on its top')
        at, yaw = unreal.Vector(x, y, foot + DECK_HEIGHT + BRAZIER_UP), 0.0
    build.place(cls, (at.x, at.y, at.z), yaw, label='WatchFire', folder=FOLDER, tags=('WatchFire',))
    build.log(f'the watch fire at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')
    return at


def eased(azimuth, values):
    """Per-direction values (north, east, south, west) at an azimuth, eased between them (area_beyond.py's _eased)."""
    t = (azimuth % 360.0) / 90.0
    k = int(math.floor(t)) % 4
    f = t - math.floor(t)
    f = f * f * (3.0 - 2.0 * f)
    return values[k] + (values[(k + 1) % 4] - values[k]) * f


def mesa_spot(build):
    """The nearest backdrop layer's plain mesa nearest DOWN_THE_RIM: (azimuth, distance, its top's height) in cm, or None."""
    layers = build.source.get('region', {}).get('backdrop', {}).get('layers', [])
    if not layers:
        return None
    layer = min(layers, key=lambda each: each.get('distance', 1e12))
    mesas = []
    for entry in layer.get('mesas', []):
        if isinstance(entry, dict):
            azimuth, height, kind = entry['azimuth'], entry['height'], entry.get('kind', 'mesa')
        else:
            azimuth, height, kind = entry[0], entry[2], (entry[3] if len(entry) > 3 else 'mesa')
        if kind == 'mesa':
            mesas.append((azimuth, height))
    if not mesas:
        return None
    off = lambda azimuth: abs(((azimuth - DOWN_THE_RIM + 180.0) % 360.0) - 180.0)
    azimuth, height = min(mesas, key=lambda mesa: off(mesa[0]))
    low = eased(azimuth, [layer[name][0] for name in DIRECTIONS])
    return azimuth, layer['distance'], low + height


def place_far_light(build, cls):
    override = build.settings.get('lift', {}).get('farLight')
    if override:
        at = unreal.Vector(*override)
        build.log('the far light where the layout\'s gameplay.lift.farLight puts it')
    else:
        mesa = mesa_spot(build)
        if mesa is None:
            build.warn('the layout has no backdrop mesa to answer from (region.backdrop): no far light')
            return
        azimuth, distance, top = mesa
        reach = distance - FAR_LIGHT_IN
        bearing = math.radians(azimuth)
        at = unreal.Vector(math.cos(bearing) * reach, math.sin(bearing) * reach, top - FAR_LIGHT_DOWN)
        build.log(f'the far light on the backdrop\'s mesa at bearing {azimuth:.1f}, {distance / 100000.0:.1f} km out, '
                  f'{top / 100.0:.0f} m up (check it from the watchtower on a tour: Looter.FarLight 1 shows it)')
    build.place(cls, (at.x, at.y, at.z), 0.0, label='FarSignalLight', folder=FOLDER, tags=('FarSignalLight',))


# ---------------------------------------------------------------------------
# All of it
# ---------------------------------------------------------------------------

def place(build):
    """The lift, its bell pulls and speaking tube, the watch fire and the far light (build_area_story's place() calls this
    after Main 7's pieces). Only Ransom's Rest has them."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    found, missing = classes()
    if missing:
        build.warn(f'no {", ".join(missing)} class (build the game module first): no ore lift, watch fire or far light')
        return
    lift, house, head, bottom, top = place_lift(build, found['OreLift'])
    place_bell_pulls(build, found['OreLiftBellPull'], lift, house, head, bottom, top)
    place_speaking_tube(build, found['SpeakerPoint'], head, top)
    place_watch_fire(build, found['WatchFire'])
    place_far_light(build, found['FarSignalLight'])


def run(name):
    """Places them again on their own (clears the Gameplay/Lift folder), saves."""
    import build_area  # noqa: E402 (here, not at the top: build_area imports build_area_story, which imports this)
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the ore lift, the watch fire and the far light placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_lift.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
