"""Main 7, "Given to the Wind" (the Wardens story since 2026-11-01, was "The Lantern Leans"; lane MAIN67,
Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 11; Docs/Areas/RansomsRest.md: Stations and the train; The exit and the
unlock), for build_area_story.py: the train at the depot's platform and the depot's place, placed from the layout's
placements and the placed models' sockets, so they follow the level whenever it's rebuilt. build_area_story's place()
calls place() here (reloaded each run, as the editor keeps modules between runs), after the deck's pieces, and gets Hob's
Main 7 perches back for his. It uses build_area_story's helpers. Everything goes in the area's Gameplay folder, so a
"gameplay" build places it all again; a piece whose model, socket or class isn't there yet is left out with a warning.

- The train (ATrain, tagged Train and Obstacle) where layout.json parks the hearse car (hearseCar), facing as it does:
  Locomotive B, the passenger car and Tilly's hearse car coupled on their wheels, cold and shut until Main 7 is turned in
  (Abel home: "nobody searches a hearse car"), then steam up and the hearse car's door open. It takes the place of the
  plain bodies build_area.py's models() puts at the hearseCar, passengerCar and locomotive placements, and of the steam
  its effects() puts on the locomotive's stack (Smoke_locomotive): those are removed here (the train has its own steam,
  from Main 7's turn-in on). A "gameplay" build finds them gone already and places the train from the layout alone.
- The depot's place (a target point tagged Place_Depot) at the hearse car's door on the platform (its SOCKET_Arrival, the
  Landing_Depot placement without the train).
Retired with the Wardens story (2026-11-01): Delia's hand-off through her screen door (ADoorHandoff: Heirloom is Main 7's
by-hand reward at its turn-in, at the chapel door, now), and the leaning flame on the deck's Keeper's Lantern (the lantern
is carried home to Saint Ada in Main 7: build_area_main7.py). A build removes what an older one placed of them.
Hob's spots (returned for build_area_story.place_hob): 'post', on the keeper's post's top by the lantern; 'hearse', on
Tilly's car's roof rail over its door, facing the platform.
"""
import unreal

import build_area_story as story

# The story's ids and tags, as the C++ (ATrain) and the mission asset (Tools/Unreal/create_mission_assets.py,
# DA_Mission_Main7) name them.
MAIN7 = 'Main7'
PLACE_DEPOT = 'Place_Depot'
TRAIN_TAG = 'Train'

# What build_area.py places that the train replaces: the bodies at these placements, and the steam on the stack.
BODY_KEYS = ('hearseCar', 'passengerCar', 'locomotive')
STEAM_LABEL = 'Smoke_locomotive'
# What older builds placed that the Wardens story retired (their labels): Delia's hand-off, the deck lantern's flame.
RETIRED_LABELS = ('Handoff_Delia', 'LanternFlame_Keeper')

# The keeper's post the deck stands by its entrance (its socket on the deck).
KEEPERS_POST_SOCKET = 'LanternPost_3'

# Without the hearse car's Arrival socket: the door on the platform in the train's frame (ATrain's own fallback), cm.
ARRIVAL = (67.5, -235.0, 40.0)
# Hob on Tilly's car's roof rail over its door, in the train's frame (Train.py: the rail at 4.13 m, 0.68 m to the
# platform's side; the doorway's middle 0.675 m along), cm; facing the platform (the train's -Y).
HEARSE_ROOF = (67.5, -68.0, 415.0)
# Hob on the keeper's post's top without its model: the post is 3.2 m tall (BurialDeck.py), cm.
POST_HEIGHT = 320.0
NORTH_EAST = 45.0


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


def remove_retired(build):
    """What an older build placed that the Wardens story retired (Delia's hand-off, the deck lantern's flame)."""
    doomed = [actor for actor in level_actors(build) if str(actor.get_actor_label()) in RETIRED_LABELS]
    if doomed:
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actors(doomed)
        build.log(f'retired pieces removed: {", ".join(sorted(str(a.get_actor_label()) for a in doomed))}')


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
    # Cold and shut until Main 7 is turned in (Abel home at Saint Ada's: the hearse car coupled for the next region).
    train.set_editor_property('warm_when', story.condition(after=[MAIN7]))
    x, y, z = spot['location']
    build.log(f'the train at the platform ({x:.0f}, {y:.0f}, {z:.0f}), warm after {MAIN7}')
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
        build.warn('no train and no Landing_Depot placement: no depot place')
        return
    z = story.marker(build, at.x, at.y, PLACE_DEPOT)
    build.log(f'the depot\'s place at the hearse car\'s door ({at.x:.0f}, {at.y:.0f}, {z:.0f}), tagged {PLACE_DEPOT}')


# ---------------------------------------------------------------------------
# Hob
# ---------------------------------------------------------------------------

def keepers_post(build):
    """The keeper's post on the burial deck: its socket's transform (world), or None."""
    deck = story.placed(build, 'BurialDeck')
    return story.socket(deck, KEEPERS_POST_SOCKET) if deck is not None else None


def hob_on_post(build, post):
    """On the keeper's post's top, by the lantern, facing north-east over the ridges."""
    if post is None:
        return None
    at = post.translation
    top = at.z + POST_HEIGHT
    # The post's own model, if the deck's pieces stand it there: the top of what's built over the socket.
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
    remove_retired(build)
    train = place_train(build)
    place_depot(build, train)
    post = keepers_post(build)
    return {'post': hob_on_post(build, post), 'hearse': hob_on_hearse(train)}
