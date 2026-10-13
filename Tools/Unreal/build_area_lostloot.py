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
    # The words go in a plain variable: the editor's Python (3.11) can't nest the apostrophe's quotes in an f-string.
    where = "in the depot's baggage window" if on_socket else 'beside the station board'
    build.log(f'{LABEL} at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}) facing {yaw:.0f}, {where}')


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
