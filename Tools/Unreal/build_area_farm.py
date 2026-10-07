"""Ransom Farm's pieces for build_area_story.py (Docs/Areas/RansomsRest.md: Grandma Delia's door, Main 1, 6 and 7), placed
on the lived-in farmhouse's sockets (SM_Farmhouse_Ransom, Art/Models/Buildings/Farmhouse.py, which layout.json's
level.models stands where the farmhouse is), so they follow the house whenever the level is rebuilt. build_area_story's
place() calls place() here (reloaded each run, as the editor keeps modules between runs). Everything goes in the area's
Gameplay folder, so a "gameplay" build places it all again; a piece whose socket, model or class isn't there yet is left
out with a warning.

- Grandma Delia's screen door (ASpeakerPoint tagged Speaker_Delia) on SOCKET_Speaker, the middle of the screen door's
  face: her Main 1 lines while it lasts, the porch after; after Main 5, until Main 6 is done, "Take him the lantern..."
  (DA_Lines_DeliaMain6), whose event (Delia.Main6) starts Main 6; in Main 7, as she hands Heirloom out through the door,
  "A keeper's buried with his lantern..." (DA_Lines_DeliaMain7; the hand-off itself is build_area_depot.py's), then a word
  after (DA_Lines_DeliaMain7After). On a farmhouse without the socket (the tutorial island's old model) it stands on that
  house's front door.
- The screen door (SM_ScreenDoor) hung closed on SOCKET_ScreenDoor, the hinge line at its foot.
- The plate Delia sets out (SM_PorchPlate) on the porch stool's SOCKET_Plate.
Her hall lamp and lamplit windows (AHouseLights on SOCKET_Light) are build_area.py's, as every lived-in house's are.
"""
import unreal

import build_area_story as story

DELIA_TAG = 'Speaker_Delia'
# Main 6 starts with Delia's word through the door: her topic sends this event (create_mission_assets.py's StartEvent).
MAIN5 = 'Main5'
MAIN6 = 'Main6'
MAIN6_EVENT = 'Delia.Main6'
MAIN7 = 'Main7'
SCREEN_DOOR = '/Game/Art/Buildings/SM_ScreenDoor'
PORCH_PLATE = '/Game/Art/Buildings/SM_PorchPlate'
# The old farmhouse's front door, in its model's frame (Farmhouse.py: the door in the middle of the front wall, which
# stands 3.1 m out from the pivot, its sill on the 0.6 m foundation): the speaker point stands on its face.
OLD_DOOR = unreal.Vector(312.0, 0.0, 60.0)


def on_socket(build, house, socket_name, what, label, tags=()):
    """A model (a static mesh path) or an actor class on one of the house's sockets, turned as the socket is, or None when
    the socket or the model isn't there."""
    at = story.socket(house, socket_name)
    if at is None:
        build.warn(f'the farmhouse has no {socket_name} socket: no {label}')
        return None
    if isinstance(what, str):
        if not unreal.EditorAssetLibrary.does_asset_exist(what):
            build.warn(f'no {what} yet (Art/Models/Buildings/Farmhouse.py): no {label}')
            return None
        what = unreal.load_asset(what)
    where = at.translation
    actor = build.place(what, (where.x, where.y, where.z), 0.0, label=label, folder='Gameplay', tags=tags)
    # The socket's whole turn, not only its yaw, as the model's notes ask.
    actor.set_actor_rotation(at.rotation.rotator(), False)
    return actor


def place_delia(build, house):
    cls = story.actor_class('SpeakerPoint')
    if cls is None:
        return
    door = story.socket(house, 'Speaker')
    if door is not None:
        # The point on the socket itself (a reach of its own puts it there, rather than over the actor).
        at, yaw, reach = door.translation, door.rotation.rotator().yaw, story.TALK_REACH
    else:
        at, yaw, reach = house.get_actor_transform().transform_location(OLD_DOOR), house.get_actor_rotation().yaw, 0.0
    # The first that holds is said: Main 1's lines while it lasts; after Main 5 until Main 6 is done, "Take him the lantern"
    # (its event starts Main 6); else the porch.
    said = []
    during = story.lines(build, 'DA_Lines_DeliaMain1')
    if during:
        said.append(story.topic(story.condition(before=[story.MAIN1]), during))
    lantern = story.lines(build, 'DA_Lines_DeliaMain6')
    if lantern:
        take_him = story.topic(story.condition(after=[MAIN5], before=[MAIN6]), lantern)
        take_him.set_editor_property('event', unreal.Name(MAIN6_EVENT))
        said.append(take_him)
    # Main 7: Heirloom handed out through the door (its first step, from 0: 0), then a word once she has.
    said += story.topics(build, (story.condition(during=MAIN7, from_step=1), 'DA_Lines_DeliaMain7After'),
                         (story.condition(during=MAIN7), 'DA_Lines_DeliaMain7'))
    story.speaker(build, cls, (at.x, at.y, at.z), yaw, DELIA_TAG, DELIA_TAG, name='Grandma Delia', reach=reach,
                  line_set=story.lines(build, 'DA_Lines_DeliaPorch'), topics=said)
    build.log(f'Grandma Delia\'s screen door at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')


def place_house(build, house):
    """The screen door and the plate: only the lived-in house has their sockets. (Its lights are build_area.py's, with
    every lived-in house's: layout.json level.lights.)"""
    if story.socket(house, 'ScreenDoor') is None:
        return
    # Tagged as the house is, so the minimap draws them with it.
    on_socket(build, house, 'ScreenDoor', SCREEN_DOOR, 'Farmhouse_ScreenDoor', tags=('Obstacle',))
    on_socket(build, house, 'Plate', PORCH_PLATE, 'Farmhouse_Plate', tags=('Obstacle',))
    build.log('the farmhouse\'s screen door and plate on their sockets')


def place(build):
    house = story.placed(build, 'Farmhouse')
    if house is None:
        return
    place_delia(build, house)
    place_house(build, house)
