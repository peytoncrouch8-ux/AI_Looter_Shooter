"""Lost Property's window (World/LostPropertyWindow: ALostPropertyWindow; the user's call, 2026-10-13: guns left lying go to
Lost Property, the depot's LOST PROPERTY window in the baggage room, Mr. Sloane's), for build_area.py's gameplay pass. The
window has no model of its own: it stands in the depot's window, and Interact there opens Lost Property's screen.

Where it stands, on the area's station (ATrainStation, the depot the full build placed at its Depot placement):
- the depot's SOCKET_LostProperty (SM_RR_Depot, coming from the art: the socket on the baggage room window's sill, its +X out
  of the building toward whoever stands at it): the window at the socket, facing the way it does, no sign of its own (the
  model has its painted LOST PROPERTY board);
- else (the old SM_Depot, with no baggage window): on the depot's wall beside the station board, SIDE cm to the right of
  the board as the player reads it, at the board's reading height, facing out the way the board does, with the window's
  placeholder sign (LOST PROPERTY painted over it) so it can be found until the new depot is in.
Placed in the <area>/Gameplay/LostProperty folder, which a gameplay build clears with the rest of Gameplay. An area with no
station gets no window (Skyreach: its trips go by the skiff), and says so.

Behind a quest since 2026-10-27 (the user's note 37, "There should be a quest to unlock Lost Loot"): the window opens once
Mr. Sloane's side mission Lost and Found is turned in (open_when; DA_Mission_LostAndFound, create_side_mission_assets.py:
offered at his ticket window after the Old Fare). Until then it's shut and the ground's clock holds (ULostLootSubsystem: a
gun left lying stays where it lies). The quest's two pieces stand here too:
- his keys (AMissionItem LostPropertyKeys, step 0): on the waiting room's floor by its benches, where they skittered as he ran
  for the stair (SOCKET_Wait_2 on SM_RR_Depot, KEYS_OFFSET along it; else on the floor in front of the window);
- the window's shutter (AMissionFitting tagged LostPropertyShutter, step 1, held): at the hatch's counter
  (SOCKET_LostPropertyCounter; else the window's own spot), no model of its own: the window is the model.

From build_area.py's gameplay(), after the vendors (the station must stand; it's placed by the full build's models()):
    importlib.reload(importlib.import_module('build_area_lostloot')).place(self)
On its own, in the open editor (clears the Gameplay/LostProperty folder, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_lostloot.py RansomsRest"
Without the game module's class yet it places nothing and says so.
"""
import json
import math
import os
import sys

import unreal

# Run on its own (py .../build_area_lostloot.py) this folder isn't on the path yet; through build_area.py it is.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import story_texts as texts  # noqa: E402

CLASSES = '/Script/AI_Looter_Shooter.'
FOLDER = 'Gameplay/LostProperty'
LABEL = 'LostProperty_Depot'
STATION_CLASS = 'TrainStation'
WINDOW_CLASS = 'LostPropertyWindow'

# The new depot's baggage window (SOCKET_LostProperty on SM_RR_Depot), and the old depot's board and its reading point.
SOCKET = 'LostProperty'
BOARD_READ_SOCKET = 'Interact'
# Beside the board on the old depot: this far to the reader's right of its reading point (cm), clear of the board's frame.
SIDE = 170.0
# The point looked at to use it, in the window's own frame (cm; +X toward the player): in front of the sill in the new
# depot's window, just off the wall on the old one.
OFFSET_SOCKET = (30.0, 0.0, 0.0)
OFFSET_WALL = (12.0, 0.0, 0.0)

# Lost and Found (the user's note 37, 2026-10-27): the mission that opens the window, its steps counted from 0 as the
# mission script writes them (create_side_mission_assets.py lost_and_found_steps), and its pieces here.
QUEST = 'LostAndFound'
KEYS_STEP = 0
SHUTTER_STEP = 1
KEYS_ID = 'LostPropertyKeys'
# The keys' models to try (Art/Models/Props; the cellar key stands in until a ring of keys is modelled).
KEYS_MODELS = ('LostPropertyKeys', 'CellarKey')
PROPS = '/Game/Art/Props/'
# Where the keys lie: by the waiting room's second waiting spot (the benches), this far along its own frame (cm), on the
# floor; off the waiting spot so the dead who stand there in the Old Fare never stand on them.
KEYS_SOCKET = 'Wait_2'
KEYS_OFFSET = (60.0, 45.0, 1.0)
# Without the waiting room: on the floor this far back from the window's own spot, into the room (cm).
KEYS_BACK = 150.0
SHUTTER_SOCKET = 'LostPropertyCounter'
SHUTTER_TAG = 'LostPropertyShutter'
SHUTTER_LABEL = 'LostProperty_Shutter'
# The hold to unlock it: a key turned and the shutter run up, as long as Dead Lift's fittings take (a key action, the mission
# gates of 2026-10-27).
SHUTTER_HOLD = 1.0
# What's looked at to use it: the hatch over the counter (cm, half sizes: deep, wide, high).
SHUTTER_LOOK = (20.0, 55.0, 40.0)
SHUTTER_REACH = 300.0
SHUTTER_CUE = 'World.LostProperty.Open'


def actor_class(name):
    return unreal.load_class(None, CLASSES + name)


def find_station(build):
    """The area's station (the depot, an ATrainStation the full build placed), or None."""
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if unreal.Name(build.tag) in actor.tags and actor.get_class().get_name() == STATION_CLASS:
            return actor
    return None


def yaw_of(vector):
    return math.degrees(math.atan2(vector.y, vector.x))


def socket_spot(station):
    """The new depot's baggage window: (location, yaw) at SOCKET_LostProperty, or None when the depot has no such socket."""
    building = station.get_editor_property('building')
    if building is None or not building.does_socket_exist(SOCKET):
        return None
    where = building.get_socket_transform(SOCKET, unreal.RelativeTransformSpace.RTS_WORLD)
    return where.translation, where.rotation.rotator().yaw


def beside_board(station):
    """Beside the old depot's station board: (location, yaw) on its wall at the board's reading height, facing out as it does."""
    board = station.get_editor_property('board')
    origin = board.get_world_location()
    if board.does_socket_exist(BOARD_READ_SOCKET):
        read = board.get_socket_location(BOARD_READ_SOCKET)
    else:
        ahead = station.get_actor_forward_vector()
        read = unreal.Vector(origin.x + ahead.x * 20.0, origin.y + ahead.y * 20.0, origin.z + 150.0)
    out = unreal.Vector(read.x - origin.x, read.y - origin.y, 0.0)
    if out.length() < 1.0:
        out = station.get_actor_forward_vector()
    yaw = yaw_of(out)
    # The reader faces the board (along -out); their right is out turned a quarter the other way.
    face = math.radians(yaw)
    right = unreal.Vector(math.sin(face), -math.cos(face), 0.0)
    return unreal.Vector(read.x + right.x * SIDE, read.y + right.y * SIDE, read.z), yaw


def when(**spec):
    """A story condition (story_texts.condition's fields: after, during, from_step, before_step...)."""
    return texts.condition(spec)


def prop_mesh(names):
    """The first of the props' models that's imported (SM_<name> under /Game/Art/Props), or None."""
    for name in names:
        path = f'{PROPS}SM_{name}'
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.load_asset(path)
    return None


def building_socket(station, name):
    """A socket of the station's building (SM_RR_Depot) in the world, or None."""
    building = station.get_editor_property('building')
    if building is None or not building.does_socket_exist(name):
        return None
    return building.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)


def place_keys(build, station, window_at, window_yaw):
    """Lost and Found's first step: Mr. Sloane's keys on the waiting room's floor, taken with a hold from its step."""
    cls = actor_class('MissionItem')
    if cls is None:
        build.warn('no MissionItem class (build the game module first): no keys for Lost and Found')
        return
    spot = building_socket(station, KEYS_SOCKET)
    if spot is not None:
        at = spot.transform_location(unreal.Vector(*KEYS_OFFSET))
        yaw = spot.rotation.rotator().yaw + 35.0
    else:
        face = math.radians(window_yaw)
        ground = station.get_actor_location().z
        at = unreal.Vector(window_at.x - math.cos(face) * KEYS_BACK, window_at.y - math.sin(face) * KEYS_BACK, ground + 1.0)
        yaw = window_yaw
        build.warn(f'{station.get_actor_label()} has no SOCKET_{KEYS_SOCKET}: the keys lie in front of the window instead')
    spec = texts.ITEMS[KEYS_ID]
    item = build.place(cls, (at.x, at.y, at.z), yaw, label=f'Item_{KEYS_ID}', folder=FOLDER,
                       tags=('MissionItem', f'Item_{KEYS_ID}'))
    item.set_editor_property('item_id', unreal.Name(KEYS_ID))
    item.set_editor_property('display_name', unreal.Text(spec['name']))
    item.set_editor_property('prompt', unreal.Text(spec['prompt']))
    item.set_editor_property('shown_when', texts.condition(spec['shown_when']))
    try:
        # The mission gates (2026-10-27): taken with a hold, only from its step.
        item.set_editor_property('take_when', when(during=QUEST, from_step=KEYS_STEP))
    except Exception as error:
        build.warn(f'no MissionItem.take_when ({error}): the keys are taken any time they show (build the game module)')
    model = prop_mesh(KEYS_MODELS)
    if model is not None:
        item.get_editor_property('mesh').set_static_mesh(model)
    else:
        build.log(f'no SM_{KEYS_MODELS[0]} yet: the keys wear their item\'s model or the stand-in')
    build.log(f'{KEYS_ID} at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')


def place_shutter(build, station, window_at, window_yaw):
    """Lost and Found's second step: the window's shutter unlocked at the hatch (held), done for good once used."""
    cls = actor_class('MissionFitting')
    if cls is None:
        build.warn('no MissionFitting class (build the game module first): Lost and Found has no shutter to unlock')
        return
    spot = building_socket(station, SHUTTER_SOCKET)
    if spot is not None:
        at, yaw = spot.translation, spot.rotation.rotator().yaw
    else:
        face = math.radians(window_yaw)
        at = unreal.Vector(window_at.x + math.cos(face) * OFFSET_SOCKET[0], window_at.y + math.sin(face) * OFFSET_SOCKET[0],
                           window_at.z)
        yaw = window_yaw + 180.0
    made = build.place(cls, (at.x, at.y, at.z), yaw, label=SHUTTER_LABEL, folder=FOLDER, tags=(SHUTTER_TAG,))
    made.set_editor_property('prompt', unreal.Text('Unlock the window'))
    made.set_editor_property('hold', True)
    made.set_editor_property('hold_seconds', SHUTTER_HOLD)
    made.set_editor_property('reach', SHUTTER_REACH)
    made.set_editor_property('usable_when', when(during=QUEST, from_step=SHUTTER_STEP, before_step=SHUTTER_STEP + 1))
    made.set_editor_property('done_when', [when(during=QUEST, from_step=SHUTTER_STEP + 1), when(after=[QUEST])])
    # No model of its own: the window is what's seen; its prompt and hand are at the hatch.
    made.set_editor_property('hide_until_done', True)
    made.set_editor_property('use_cue', unreal.Name(SHUTTER_CUE))
    try:
        made.set_editor_property('look_half_size', unreal.Vector(*SHUTTER_LOOK))
    except Exception as error:
        build.warn(f'no MissionFitting.look_half_size ({error}): the shutter is used at its own spot')
    build.log(f'{SHUTTER_LABEL} at the hatch ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}), Lost and Found\'s step {SHUTTER_STEP}')


def place(build):
    """The area's Lost Property window (build_area.py's gameplay() calls this after the vendors). Only an area with a station."""
    cls = actor_class(WINDOW_CLASS)
    if cls is None:
        build.warn(f'no {WINDOW_CLASS} class (build the game module first): no Lost Property window placed')
        return
    station = find_station(build)
    if station is None:
        build.log('no station here: no Lost Property window (the shelves are the session\'s, kept at the depot)')
        return
    spot = socket_spot(station)
    on_socket = spot is not None
    if not on_socket:
        spot = beside_board(station)
        build.warn(f'{station.get_actor_label()} has no SOCKET_{SOCKET} (the new SM_RR_Depot has it): Lost Property stands '
                   f'beside the station board with its placeholder sign')
    at, yaw = spot
    window = build.place(cls, (at.x, at.y, at.z), yaw, label=LABEL, folder=FOLDER)
    window.set_editor_property('show_placeholder_sign', not on_socket)
    offset = OFFSET_SOCKET if on_socket else OFFSET_WALL
    window.set_editor_property('interact_offset', unreal.Vector(*offset))
    # Shut until Mr. Sloane's Lost and Found is turned in (the user's note 37, 2026-10-27).
    window.set_editor_property('open_when', when(after=[QUEST]))
    # The words go in a plain variable: the editor's Python (3.11) can't nest the apostrophe's quotes in an f-string.
    where = "in the depot's baggage window" if on_socket else 'beside the station board'
    build.log(f'{LABEL} at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}) facing {yaw:.0f}, {where}, open after {QUEST}')
    place_keys(build, station, at, yaw)
    place_shutter(build, station, at, yaw)


def run(name):
    """Places the window again on its own (clears the Gameplay/LostProperty folder), saves."""
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    import build_area  # noqa: E402 (here, not at the top: build_area imports this module)
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('Lost Property placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_lostloot.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
