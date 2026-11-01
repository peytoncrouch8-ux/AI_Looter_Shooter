"""Ransom Farm's pieces for build_area_story.py (Docs/Areas/RansomsRest.md: Delia's porch, Main 1 "Landfall", 6 and 7),
placed on the lived-in farmhouse's sockets (SM_Farmhouse_Ransom, Art/Models/Buildings/Farmhouse.py, which layout.json's
level.models stands where the farmhouse is), so they follow the house whenever the level is rebuilt. build_area_story's
place() calls place() here (reloaded each run, as the editor keeps modules between runs). Everything goes in the area's
Gameplay folder, so a "gameplay" build places it all again; a piece whose socket, model or class isn't there yet is left
out with a warning.

- Delia Ransom (ADeliaRansom, lane CREW, tagged Speaker_Delia; the Wardens story of 2026-10-31: Abel's mother, on her
  porch with a shotgun, fighting whatever comes within 18 m of it, the turn-in giver of Landfall) standing on the porch
  in front of her screen door (SOCKET_Speaker, 1.5 m over the porch floor, facing out), facing out, her capsule's middle
  68 cm over the floor. Her talk (on her own speaker_point; the tag is on her alone, no screen-door speaker point):
  Landfall's topics first (story_texts.TOPICS['Speaker_Delia']: the turn-in, the hat's variant, a word mid-fight), then
  the mine's main missions and Main 7's porch word (Main 6 is Holloway's since lane MAIN67; Main 7's turn-in is the chapel
  door Delia's); her porch words (DA_Lines_DeliaPorch) otherwise. Until CREW's class is built she's the old
  screen-door speaker point (same tag, name and topics), so the game keeps working.
- The screen door (SM_ScreenDoor) hung closed on SOCKET_ScreenDoor, the hinge line at its foot.
- The plate Delia sets out (SM_PorchPlate) on the porch stool's SOCKET_Plate (build_area_missionprops_valley.py makes it
  the APorchPlate).
- The porch barricade (stand-ins until the user's Tripo pieces, TRIPO_MODELS.md 17: an overturned kitchen table, boards
  nailed across a gap, a flour sack pile): what the farm had to hand, in BARRICADE below, on the porch's front edge and the
  yard in front of it, solid and tagged Obstacle (cover for Delia, the player and the Dredgers), the steps kept clear.
  Each piece checks its ground and keeps off the roads and whatever stands there already; one that can't is left out.
Her hall lamp and lamplit windows (AHouseLights on SOCKET_Light) are build_area.py's, as every lived-in house's are.
"""
import importlib
import math

import unreal

import build_area_story as story

DELIA_TAG = 'Speaker_Delia'
DELIA_NAME = 'Delia Ransom'
# CREW's class and its speaker component's Python name (ASpeakerPoint's is speaker_point; confirm on ADeliaRansom).
DELIA_CLASS = 'DeliaRansom'
DELIA_SPEAKER = 'speaker_point'
# She stands this far out from the screen door's face, on the porch floor (the socket is 1.5 m over it).
DELIA_OUT = 110.0
DELIA_SOCKET_UP = 150.0
DELIA_REACH = 350.0
# ADeliaRansom's actor is her capsule's middle (DeliaRansom.cpp: radius 26, half-height 68): placed this far over the floor.
DELIA_HALF_HEIGHT = 68.0
# Main 6 starts with Delia's word: her topic sends this event (create_mission_assets.py's StartEvent).
MAIN5 = 'Main5'
MAIN6 = 'Main6'
MAIN6_EVENT = 'Delia.Main6'
MAIN7 = 'Main7'
SCREEN_DOOR = '/Game/Art/Buildings/SM_ScreenDoor'
PORCH_PLATE = '/Game/Art/Buildings/SM_PorchPlate'
# The old farmhouse's front door, in its model's frame (Farmhouse.py: the door in the middle of the front wall, which
# stands 3.1 m out from the pivot, its sill on the 0.6 m foundation): the speaker point stands on its face.
OLD_DOOR = unreal.Vector(312.0, 0.0, 60.0)

# The porch barricade, in the farmhouse's frame (Farmhouse.py, cm: front +X; the porch floor 60 cm up, its front edge at
# X 530, across Y -360..360; the steps X 530-590, Y -80..80, kept clear; Delia at X ~420, Y 0; the stool by the door and
# the rocking chair at X ~410, Y +-250 stand on the porch already). Each: (stand-in mesh, x, y, yaw in the house's frame,
# on the porch floor or the ground, what it stands for). One path each: a Tripo piece replaces it in place.
BARRICADE = (
    # The user's Tripo pieces (lanes/IMPORT.md; long axis along their X like the stand-ins: SM_Bench, SM_LumberStack,
    # SM_FlourSacks x2).
    ('/Game/Art/Tripo/SM_BarricadeTable', 500.0, -235.0, 90.0, 'porch', 'the overturned kitchen table [17]'),
    ('/Game/Art/Tripo/SM_BarricadeBoards', 500.0, 245.0, 90.0, 'porch', 'boards nailed across a gap [17]'),
    ('/Game/Art/Tripo/SM_BarricadeSacks', 640.0, -175.0, 10.0, 'ground', 'the flour sack pile [17]'),
    ('/Game/Art/Tripo/SM_BarricadeSacks', 645.0, 180.0, 195.0, 'ground', 'the flour sack pile [17]'),
    ('/Game/Art/Props/SM_Crate_B', 1150.0, -420.0, 25.0, 'ground', 'crates dragged out as cover'),
    ('/Game/Art/Props/SM_Cart', 1500.0, 520.0, 70.0, 'ground', 'the cart turned across the yard as cover'),
)
# A piece keeps this far off a road's edge (cm), and off anything solid already there (a sphere this share of its size).
BARRICADE_ROAD_CLEAR = 80.0
BARRICADE_SOLID_SHARE = 0.45


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


def delia_topics(build):
    """Her talk: Landfall's topics first (story_texts.TOPICS['Speaker_Delia']: the turn-in, its hat variant, a word
    mid-fight; Main 1's turn-in has no lines of its own, so the first of these that holds is what she says as it's turned
    in, the card first: lane TURNIN), then the missions after Landfall's as they were. Returns (topics, default set)."""
    import story_texts
    texts = importlib.reload(story_texts)
    spec = texts.TOPICS.get(DELIA_TAG, {})
    # Landfall's topics (STORY's words); the later ones below come after them.
    said = story.texts_topics(build, DELIA_TAG) if spec else []
    # While the mine's main missions run (Dead Lift, Back Pay: Cyrus Teague before Abel, the user's 2026-10-27 call).
    mine = story.lines(build, 'DA_Lines_DeliaMine')
    if mine:
        said.append(story.topic(story.condition(after=[MAIN5], before=['BackPay']), mine))
    # Main 6 is Holloway's now (her event Delia.Main6 started nothing since 2026-10-22).
    # Main 7 (Given to the Wind): a word on the porch; her turn-in is at the chapel door (build_area_main7.py).
    said += story.topics(build, (story.condition(during=MAIN7), 'DA_Lines_DeliaMain7After'))
    return said, spec.get('default') or 'DA_Lines_DeliaPorch'


def place_delia(build, house):
    door = story.socket(house, 'Speaker')
    said, porch_words = delia_topics(build)
    cls = story.actor_class(DELIA_CLASS)
    if cls is None or door is None:
        # Until CREW's class is built (or on a house without the socket): the screen door speaks for her, as before.
        point_cls = story.actor_class('SpeakerPoint')
        if point_cls is None:
            return
        if cls is None:
            build.warn(f'no {DELIA_CLASS} class (lane CREW; build the game module): Delia talks through her screen door')
        if door is not None:
            at, yaw, reach = door.translation, door.rotation.rotator().yaw, story.TALK_REACH
        else:
            at, yaw, reach = house.get_actor_transform().transform_location(OLD_DOOR), house.get_actor_rotation().yaw, 0.0
        story.speaker(build, point_cls, (at.x, at.y, at.z), yaw, DELIA_TAG, DELIA_TAG, name=DELIA_NAME, reach=reach,
                      line_set=story.lines(build, porch_words), topics=said)
        return
    # On the porch floor in front of the door, facing out over the yard and the farm road (where FarmPorch's enemies come
    # from: she turns back to this facing when calm). The tag is on her alone: no screen-door speaker point is placed now
    # (the talk, reach and turn-in objectives take the first actor with Speaker_Delia).
    face = door.rotation.rotator()
    out = face.get_forward_vector()
    x, y = door.translation.x + out.x * DELIA_OUT, door.translation.y + out.y * DELIA_OUT
    floor = story.onto_host(house, unreal.Vector(x, y, door.translation.z), unreal.Vector(x, y, door.translation.z - 300.0))
    z = (floor.z if floor is not None else door.translation.z - DELIA_SOCKET_UP) + DELIA_HALF_HEIGHT
    delia = build.place(cls, (x, y, z), face.yaw, label='DeliaRansom', folder='Gameplay', tags=(DELIA_TAG,))
    try:
        talk = delia.get_editor_property(DELIA_SPEAKER)
    except Exception as error:
        build.warn(f'{DELIA_CLASS} has no {DELIA_SPEAKER} ({error}): her words aren\'t set (check DELIA_SPEAKER against '
                   f'CREW\'s C++)')
        return
    talk.set_editor_property('speaker_name', unreal.Text(DELIA_NAME))
    talk.set_editor_property('reach', DELIA_REACH)
    porch = story.lines(build, porch_words)
    if porch:
        talk.set_editor_property('line_set', porch)
    talk.set_editor_property('topics', list(said))
    build.log(f'Delia Ransom on her porch at ({x:.0f}, {y:.0f}, {z:.0f}), facing out, {len(said)} topics')


def place_house(build, house):
    """The screen door and the plate: only the lived-in house has their sockets. (Its lights are build_area.py's, with
    every lived-in house's: layout.json level.lights.)"""
    if story.socket(house, 'ScreenDoor') is None:
        return
    # Tagged as the house is, so the minimap draws them with it.
    on_socket(build, house, 'ScreenDoor', SCREEN_DOOR, 'Farmhouse_ScreenDoor', tags=('Obstacle',))
    on_socket(build, house, 'Plate', PORCH_PLATE, 'Farmhouse_Plate', tags=('Obstacle',))
    build.log('the farmhouse\'s screen door and plate on their sockets')


def road_clear(build, x, y):
    """How far (x, y) is from the nearest road's edge (negative: on it)."""
    best = 1e9
    for road in build.source.get('roads', []):
        path, half = road.get('path', []), road.get('width', 300.0) * 0.5
        for (x0, y0), (x1, y1) in zip(path, path[1:]):
            dx, dy = x1 - x0, y1 - y0
            t = max(0.0, min(1.0, ((x - x0) * dx + (y - y0) * dy) / max(dx * dx + dy * dy, 1.0)))
            best = min(best, math.hypot(x - x0 - dx * t, y - y0 - dy * t) - half)
    return best


def solid_there(x, y, z, radius, house):
    """Something solid (not the terrain, the scatter's volume or the house itself) within radius of (x, y, z)."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    try:
        found = unreal.SystemLibrary.sphere_overlap_actors(world, unreal.Vector(x, y, z), radius,
                                                           [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], None, [house])
    except Exception:
        return None
    hits = found[1] if isinstance(found, tuple) else found
    for actor in hits or []:
        if actor is None or unreal.Name('Ground') in actor.tags or isinstance(actor, (unreal.PCGVolume, unreal.TriggerBox)):
            continue
        return str(actor.get_actor_label())
    return None


def place_barricade(build, house):
    """Delia's barricade on the porch's front edge and in the yard (BARRICADE), solid cover, the steps clear."""
    frame = house.get_actor_transform()
    base = house.get_actor_location().z
    yaw0 = house.get_actor_rotation().yaw
    placed = []
    for index, (path, lx, ly, lyaw, on, what) in enumerate(BARRICADE):
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            build.warn(f'no {path}: the barricade\'s {what} is left out')
            continue
        model = unreal.load_asset(path)
        p = frame.transform_location(unreal.Vector(lx, ly, 0.0))
        if on == 'porch':
            floor = story.onto_host(house, unreal.Vector(p.x, p.y, base + 200.0), unreal.Vector(p.x, p.y, base))
            if floor is None:
                build.warn(f'no porch floor under the barricade\'s {what} at ({p.x:.0f}, {p.y:.0f}): left out')
                continue
            z = floor.z
        else:
            if road_clear(build, p.x, p.y) < BARRICADE_ROAD_CLEAR:
                build.warn(f'the barricade\'s {what} at ({p.x:.0f}, {p.y:.0f}) would stand on a road: left out')
                continue
            z = story.ground_at(p.x, p.y, base)
        box = model.get_bounding_box()
        size = (box.get_editor_property('max') - box.get_editor_property('min')).length()
        blocker = solid_there(p.x, p.y, z + size * 0.3, size * BARRICADE_SOLID_SHARE, house)
        if blocker and blocker not in placed:
            build.warn(f'the barricade\'s {what} at ({p.x:.0f}, {p.y:.0f}) would stand in {blocker}: left out')
            continue
        label = f'Barricade_{index + 1}'
        build.place(model, (p.x, p.y, z), yaw0 + lyaw, label=label, folder='Gameplay', tags=('Obstacle', 'Barricade'))
        placed.append(label)
    build.log(f'Delia\'s porch barricade: {len(placed)} of {len(BARRICADE)} pieces')


def place(build):
    house = story.placed(build, 'Farmhouse')
    if house is None:
        return
    place_delia(build, house)
    place_house(build, house)
    if story.socket(house, 'ScreenDoor') is not None:
        place_barricade(build, house)
