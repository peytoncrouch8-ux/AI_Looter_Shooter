"""Main 7, "The Lantern Leans" (Docs/Areas/RansomsRest.md: Main 7; Stations and the train; The exit and the unlock), for
build_area_story.py: the train at the depot's platform, the depot's place, Delia's hand-off and the lantern's leaning
flame, placed from the layout's placements and the placed models' sockets, so they follow the level whenever it's rebuilt.
build_area_story's place() calls place() here (reloaded each run, as the editor keeps modules between runs), after Main
6's pieces (the lantern on the keeper's post must stand already), and gets Hob's Main 7 perches back for his. It uses
build_area_story's helpers. Everything goes in the area's Gameplay folder, so a "gameplay" build places it all again; a
piece whose model, socket or class isn't there yet is left out with a warning.

- The train (ATrain, tagged Train and Obstacle) where layout.json parks the hearse car (hearseCar), facing as it does:
  Locomotive B, the passenger car and Tilly's hearse car coupled on their wheels, cold and shut until Main 7 (after Main
  6), then steam up and the hearse car's door open. It takes the place of the plain bodies build_area.py's models() puts
  at the hearseCar, passengerCar and locomotive placements, and of the steam its effects() puts on the locomotive's stack
  (Smoke_locomotive): those are removed here (the train has its own steam, from Main 7 on). A "gameplay" build finds
  them gone already and places the train from the layout alone.
- The depot's place (a target point tagged Place_Depot) at the hearse car's door on the platform (its SOCKET_Arrival, the
  Landing_Depot placement without the train): Main 7's second step, "Go to the depot".
- Delia's hand-off (ADoorHandoff, Handoff_Delia) on the lived-in farmhouse's SOCKET_Handoff, facing out as the socket
  does, with her screen door (Farmhouse_ScreenDoor, build_area_farm.py) to open a crack: Heirloom comes out as Main 7's
  first step, the talk at her door, ends. The gun is Main 7's by-hand reward (DA_Mission_Main7).
- The lantern's flame (ALanternFlame) in the Keeper's Lantern Main 6 hangs on the keeper's post (AKeeperLanternPost
  tagged LanternPost_Keeper, build_area_deck.py; else the SM_KeepersLantern nearest the deck's SOCKET_LanternPost_3,
  within 6 m): on its SOCKET_Light, attached to it so it goes where the lantern goes, leaning north-east (toward the Lily)
  once Abel lights it (the post's OnKeepersLanternLit) and after Main 6. Without such a lantern there, none.
Hob's spots (returned for build_area_story.place_hob): 'post', on the keeper's post's top by the lantern, facing
north-east as the flame leans; 'hearse', on Tilly's car's roof rail over its door, facing the platform.
"""
import unreal

import build_area_story as story

# The story's ids and tags, as the C++ (ATrain, ADoorHandoff, ALanternFlame) and the mission asset
# (Tools/Unreal/create_mission_assets.py, DA_Mission_Main7) name them.
MAIN6 = 'Main6'
MAIN7 = 'Main7'
PLACE_DEPOT = 'Place_Depot'
TRAIN_TAG = 'Train'
# Delia's hand-off: Main 7's step (from 0) whose end hands Heirloom out.
HANDOFF_STEP = 0

# What build_area.py places that the train replaces: the bodies at these placements, and the steam on the stack.
BODY_KEYS = ('hearseCar', 'passengerCar', 'locomotive')
STEAM_LABEL = 'Smoke_locomotive'
SCREEN_DOOR_LABEL = 'Farmhouse_ScreenDoor'

KEEPERS_LANTERN = 'SM_KeepersLantern'
# The keeper's post Main 6 stands by the deck's entrance (AKeeperLanternPost::KeepersPostTag): its flame lights with it.
KEEPERS_POST_TAG = 'LanternPost_Keeper'
KEEPERS_POST_SOCKET = 'LanternPost_3'
# How far from the keeper's post the lantern Main 6 hangs there may be (cm).
LANTERN_NEAR = 600.0
# The Keeper's Lantern's flame over its foot, without its SOCKET_Light (BurialDeck.py: 0.122 m), cm.
FLAME_UP = 12.2
NORTH_EAST = 45.0

# Without the hearse car's Arrival socket: the door on the platform in the train's frame (ATrain's own fallback), cm.
ARRIVAL = (67.5, -235.0, 40.0)
# Hob on Tilly's car's roof rail over its door, in the train's frame (Train.py: the rail at 4.13 m, 0.68 m to the
# platform's side; the doorway's middle 0.675 m along), cm; facing the platform (the train's -Y).
HEARSE_ROOF = (67.5, -68.0, 415.0)
# Hob on the keeper's post's top without its model: the post is 3.2 m tall (BurialDeck.py), cm.
POST_HEIGHT = 320.0


def level_actors(build):
    """Every actor the area's builds placed."""
    return [actor for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
            if unreal.Name(build.tag) in actor.tags]


def labelled(build, label):
    """The built actor with this label, or None."""
    return next((actor for actor in level_actors(build) if str(actor.get_actor_label()) == label), None)


def mesh_name(component):
    mesh = component.static_mesh
    return mesh.get_name() if mesh is not None else ''


# ---------------------------------------------------------------------------
# The train and the depot's place
# ---------------------------------------------------------------------------

def remove_plain_train(build, cls):
    """build_area.py's plain bodies at the train's placements, and the steam on the locomotive's stack."""
    doomed = [actor for actor in level_actors(build)
              if str(actor.get_actor_label()) in BODY_KEYS + (STEAM_LABEL,) and actor.get_class() != cls]
    if doomed:
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actors(doomed)
        build.log(f'the train replaces {len(doomed)} plain bodies and steam')


def place_train(build):
    """The train where the hearse car is parked. Returns it, or None."""
    cls = story.actor_class('Train')
    spot = build.layout['placements'].get('hearseCar')
    if cls is None:
        build.warn('no Train class (build the game module first): the train stays plain bodies')
        return None
    if spot is None:
        build.warn('layout.json has no hearseCar placement: no train')
        return None
    remove_plain_train(build, cls)
    train = build.place(cls, spot['location'], spot['yaw'], label='Train', folder='Gameplay', tags=(TRAIN_TAG, 'Obstacle'))
    # Cold and shut until Main 7: from Main 6's end on, steam up and Tilly's car's door open.
    train.set_editor_property('warm_when', story.condition(after=[MAIN6]))
    x, y, z = spot['location']
    build.log(f'the train at the platform ({x:.0f}, {y:.0f}, {z:.0f}), warm after {MAIN6}')
    return train


def arrival_spot(build, train):
    """Where the hearse car's door opens onto the platform (world), or None."""
    if train is not None:
        hearse = train.get_editor_property('hearse_car')
        if hearse.does_socket_exist('Arrival'):
            return hearse.get_socket_location('Arrival')
        return train.get_actor_transform().transform_location(unreal.Vector(*ARRIVAL))
    landing = build.layout['placements'].get('Landing_Depot')
    return unreal.Vector(*landing['location']) if landing else None


def place_depot(build, train):
    """The depot's place: a marker at the hearse car's door on the platform."""
    at = arrival_spot(build, train)
    if at is None:
        build.warn('no train and no Landing_Depot placement: no depot place for Main 7')
        return
    z = story.marker(build, at.x, at.y, PLACE_DEPOT)
    build.log(f'the depot\'s place at the hearse car\'s door ({at.x:.0f}, {at.y:.0f}, {z:.0f}), tagged {PLACE_DEPOT}')


# ---------------------------------------------------------------------------
# Delia's hand-off
# ---------------------------------------------------------------------------

def place_handoff(build):
    """Heirloom through the screen door: the hand-off on the farmhouse's SOCKET_Handoff, opening her screen door."""
    cls = story.actor_class('DoorHandoff')
    house = story.placed(build, 'Farmhouse')
    if cls is None or house is None:
        if cls is None:
            build.warn('no DoorHandoff class (build the game module first): Delia hands nothing out')
        return
    at = story.socket(house, 'Handoff')
    if at is None:
        build.warn('the farmhouse has no Handoff socket (Farmhouse_Ransom, Art/Models/Buildings/Farmhouse.py): Delia hands nothing out')
        return
    where = at.translation
    handoff = build.place(cls, (where.x, where.y, where.z), at.rotation.rotator().yaw, label='Handoff_Delia', folder='Gameplay')
    # The socket's whole turn: facing out of the door.
    handoff.set_actor_rotation(at.rotation.rotator(), False)
    handoff.set_editor_property('mission', unreal.Name(MAIN7))
    handoff.set_editor_property('step', HANDOFF_STEP)
    door = labelled(build, SCREEN_DOOR_LABEL)
    if door is not None:
        handoff.set_editor_property('door', door)
    else:
        build.warn(f'no {SCREEN_DOOR_LABEL} (build_area_farm.py): Heirloom comes out with the screen door shut')
    build.log(f'Delia\'s hand-off at ({where.x:.0f}, {where.y:.0f}, {where.z:.0f}), as {MAIN7}\'s first step ends')


# ---------------------------------------------------------------------------
# The lantern's flame
# ---------------------------------------------------------------------------

def keepers_post(build):
    """The keeper's post on the burial deck: its socket's transform (world), or None."""
    deck = story.placed(build, 'BurialDeck')
    return story.socket(deck, KEEPERS_POST_SOCKET) if deck is not None else None


def deck_lantern(build, post):
    """The Keeper's Lantern Main 6 hangs on the keeper's post: the post's own (AKeeperLanternPost tagged
    LanternPost_Keeper, build_area_deck.py: its KeepersLantern), else the SM_KeepersLantern component nearest the post's
    socket, within reach (never the Sink's, in its snare). None when there's none."""
    keepers = next((actor for actor in level_actors(build) if unreal.Name(KEEPERS_POST_TAG) in actor.tags), None)
    if keepers is not None:
        try:
            return keepers.get_editor_property('keepers_lantern')
        except Exception:
            pass
    best = None
    for actor in level_actors(build):
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            if mesh_name(component) != KEEPERS_LANTERN:
                continue
            distance = (component.get_world_location() - post.translation).length()
            if distance <= LANTERN_NEAR and (best is None or distance < best[0]):
                best = (distance, component)
    return best[1] if best else None


def place_flame(build, post):
    """The flame on the deck's lit lantern, leaning north-east after Main 6."""
    cls = story.actor_class('LanternFlame')
    if cls is None or post is None:
        if cls is None:
            build.warn('no LanternFlame class (build the game module first): the lantern\'s flame doesn\'t lean')
        else:
            build.warn(f'the burial deck has no {KEEPERS_POST_SOCKET} socket: the lantern\'s flame doesn\'t lean')
        return
    lantern = deck_lantern(build, post)
    if lantern is None:
        build.warn('no Keeper\'s Lantern on the keeper\'s post (Main 6\'s pieces first): the lantern\'s flame doesn\'t lean')
        return
    socket = 'Light' if lantern.does_socket_exist('Light') else ''
    wick = lantern.get_socket_location('Light') if socket else lantern.get_world_location() + unreal.Vector(0.0, 0.0, FLAME_UP)
    flame = build.place(cls, (wick.x, wick.y, wick.z), 0.0, label='LanternFlame_Keeper', folder='Gameplay')
    flame.set_editor_property('bearing', NORTH_EAST)
    flame.set_editor_property('shown_when', story.condition(after=[MAIN6]))
    keep = unreal.AttachmentRule.KEEP_WORLD
    flame.attach_to_component(lantern, socket, keep, keep, keep, False)
    build.log(f'the lantern\'s flame on {lantern.get_owner().get_actor_label()} ({wick.x:.0f}, {wick.y:.0f}, {wick.z:.0f}), '
              f'leaning north-east after {MAIN6}')


# ---------------------------------------------------------------------------
# Hob
# ---------------------------------------------------------------------------

def hob_on_post(build, post):
    """On the keeper's post's top, by the lantern, facing north-east as its flame leans."""
    if post is None:
        return None
    at = post.translation
    top = at.z + POST_HEIGHT
    # The post's own model, if Main 6's pieces stand it there: the top of what's built over the socket.
    for actor in level_actors(build):
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            if mesh_name(component) == 'SM_KeeperLanternPost' and (component.get_world_location() - at).length() < 100.0:
                box = component.static_mesh.get_bounding_box()
                top = component.get_world_location().z + box.max.z * component.get_world_scale().z
    return at.x, at.y, top, NORTH_EAST


def hob_on_hearse(train):
    """On Tilly's car's roof rail over its door, facing the platform."""
    if train is None:
        return None
    at = train.get_actor_transform().transform_location(unreal.Vector(*HEARSE_ROOF))
    return at.x, at.y, at.z, train.get_actor_rotation().yaw - 90.0


def place(build):
    """Everything above. Returns Hob's Main 7 spots ({'post', 'hearse'}: (x, y, z, yaw) or None)."""
    train = place_train(build)
    place_depot(build, train)
    place_handoff(build)
    post = keepers_post(build)
    place_flame(build, post)
    return {'post': hob_on_post(build, post), 'hearse': hob_on_hearse(train)}
