"""Main 4, "Hallowed Ground" (Docs/Areas/RansomsRest.md), for build_area_story.py: the story's pieces at the Chapel of
Saint Ada, placed from the chapel build_area.py placed (SM_Chapel, Art/Models/Buildings/Chapel.py: its sockets) and from
the layout's zones, obstacles, roads and placements, so they follow the level whenever it's rebuilt. build_area_story's
place() calls place() here (reloaded each run, as the editor keeps modules between runs) and gets Hob's two chapel perches
back for his. It uses build_area_story's helpers. Everything goes in the area's Gameplay folder; a piece whose model or
class isn't there yet is left out with a warning.

- The chapel's place (a target point tagged Place_Chapel) at the chapel's middle: "Go up to the Chapel of Saint Ada" ends
  within 15 m of it.
- The chapel yard's fight (AEncounterSpawner ChapelYard) on the chapel's spot, turned as the chapel is, its creatures'
  spots round the chapel in the chapel's own frame (YARD_SPOTS): "12 Unpaid in two waves, and one Restless", two waves of
  six, the second once the first is down (and 4 s more) and with the Restless one, tagged Unpaid_ChapelYard. On during
  Main 4's second step only, coming for the player as they appear, hunting only inside the churchyard's iron fence (the
  obstacle churchyardFence) and its 2 m margin. The knoll falls about 8 m from the chapel's door to the fence's far
  corners, all one walkable slope with no drop in it, so the yard is one level of ground here: spots up to 9 m above or
  below the chapel's floor, and players up to 10 m, rather than the defaults (4 m and 6 m).
- The bell (AChapelBell tagged Bell_Chapel) at the rope's grip (SOCKET_Interact), its bell (SM_ChapelBell) hung on the
  belfry's SOCKET_Bell: "Ring the chapel bell", any time.
- The smashed Reliquary (AChapelReliquary tagged Reliquary_Chapel) on the apse's plinth (SOCKET_Reliquary), facing the
  nave, looked at from Main 4's fourth step on: its model (the first of RELIQUARY_MESHES imported: the art session's
  Art/Models/Props/Reliquary.py, export family RR_Reliquary, with SOCKET_Ember and SOCKET_Interact), else the class's
  stand-in.
- Father Aldana's vestry door (ASpeakerPoint tagged Speaker_Aldana) on SOCKET_Speaker: barred before the yard is quiet
  (and before Main 4), the bell and the Reliquary next, the doc's words at Main 4's last step, the Sink after, and the
  lantern found after Main 5 (take it home to Delia).
- The chapel yard's respawn grave (ARespawnMarker ChapelYard, open after Main 4) at the foot of its fresh mound (the
  placement graveChapelYard), facing the chapel.
- The roaming Unpaid after Main 4 ("From now on the Unpaid walk the north road and boot hill"): AEncounterSpawner BootHill
  on boot hill (the zone bootHill its ground) and NorthRoad between the hanging tree and the north road (25 m round it),
  three each, their ranks the area's (8% Restless, 2% Gravebound), standing about until they notice the player, tagged
  Unpaid_<id>. On from Main 4's finish for good; like every encounter, a level loaded again starts them over.
"""
import importlib
import math

import unreal

import build_area_story as story

# The story's ids and tags, as the C++ (AChapelBell, AChapelReliquary) and the mission asset
# (Tools/Unreal/create_mission_assets.py, DA_Mission_Main4) name them.
MAIN4 = 'Main4'
MAIN5 = 'Main5'
PLACE_CHAPEL = 'Place_Chapel'
YARD = 'ChapelYard'
YARD_TAG = 'Unpaid_ChapelYard'
BELL_TAG = 'Bell_Chapel'
RELIQUARY_TAG = 'Reliquary_Chapel'
ALDANA_TAG = 'Speaker_Aldana'
YARD_GRAVE = 'ChapelYard'
BOOT_HILL = 'BootHill'
NORTH_ROAD = 'NorthRoad'

# The models: the chapel's bell, and the smashed Reliquary. Its name is still to be confirmed by the art session: the
# planned SM_Reliquary_Smashed, or SM_Reliquary as Reliquary.py names its object today; the first imported is used.
BELL_MESH = '/Game/Art/Buildings/SM_ChapelBell'
RELIQUARY_MESHES = ('/Game/Art/Props/SM_Reliquary_Smashed', '/Game/Art/Props/SM_Reliquary')

# The chapel's own frame for the yard's spots (cm): +X its front (the tower, the door, the road), -X its apse, +Y the side
# away from the vestry (the west grave rows), -Y the vestry's side (the east rows). The spots: the lawn before the door,
# the lanes either side of the nave between it and the grave rows, behind the apse, and behind the vestry, all inside the
# fence (X -1200 to 1000, Y -1700 to 1700 here). The spawner takes them in turn, skipping any within 8 m of the player.
# The dressing's graves, trees and props (build_area_dressing.py) keep YARD_CLEAR (cm) from them, so nobody rises in a
# headboard or a trunk; place_yard warns of any that doesn't (the fence's line is the yard's edge, and isn't counted).
YARD_SPOTS = (
    (800.0, 900.0), (800.0, -700.0), (850.0, 1500.0), (850.0, -1450.0), (100.0, 560.0), (-450.0, 560.0),
    (-950.0, 0.0), (-1000.0, 500.0), (-1000.0, -500.0), (150.0, -600.0), (-750.0, -700.0),
)
YARD_CLEAR = 150.0
# Each wave's Basic Unpaid; the second comes this long after the first is down (s).
YARD_WAVE = 6
YARD_WAVE_PAUSE = 4.0
# The yard as one level of ground (cm): spots this far above or below the chapel's floor, players this far.
YARD_STEP = 900.0
YARD_RISE = 1000.0

# The yard's respawn grave: this far out of the fresh mound's foot (cm), clear of its heap (1.95 m long).
MOUND_STAND_OUT = 140.0

# The roaming Unpaid after Main 4: how many on each stretch, how far the north road's runs round its spawner, and how
# far each spreads them out (cm).
ROAMING_COUNT = 3
ROAD_STRETCH = 2500.0
ROAD_SPREAD = 900.0
HILL_SPREAD = 600.0


def placed_key(build, key):
    """The actor build_area.py placed for a placement key (it labels each with its key), or None (with a warning)."""
    if key not in build.layout['placements']:
        build.warn(f'layout_computed.json has no placement {key}: what stands on it is left out')
        return None
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if actor.get_actor_label() == key and unreal.Name(build.tag) in actor.tags:
            return actor
    build.warn(f'no {key} placed (build the whole level first): what stands on it is left out')
    return None


def nearest_on_path(path, point):
    """The point of a polyline ((x, y) pairs) nearest point, seen from above."""
    best, best_d = None, None
    for (ax, ay), (bx, by) in zip(path, path[1:]):
        dx, dy = bx - ax, by - ay
        length = dx * dx + dy * dy
        t = 0.0 if length < 1e-6 else max(0.0, min(1.0, ((point[0] - ax) * dx + (point[1] - ay) * dy) / length))
        x, y = ax + dx * t, ay + dy * t
        d = (x - point[0]) ** 2 + (y - point[1]) ** 2
        if best_d is None or d < best_d:
            best, best_d = (x, y), d
    return best


def roaming_group(cls, count):
    """FEncounterGroup: count of cls, their ranks the area's promotions ("plus 8% of spawns" Restless, 2% Gravebound)."""
    made = unreal.EncounterGroup()
    made.set_editor_property('creature_class', cls)
    made.set_editor_property('count', count)
    made.set_editor_property('rank_roll', unreal.EncounterRankRoll.AREA)
    return made


# ---------------------------------------------------------------------------
# The chapel
# ---------------------------------------------------------------------------

def place_chapel_marker(build, chapel):
    """The chapel's middle, on the ground under it, where "Go up to the Chapel of Saint Ada" points."""
    at = chapel.get_actor_location()
    z = story.ground(at.x, at.y, at.z, ignore=[chapel]) + story.MARKER_UP
    build.place(unreal.TargetPoint, (at.x, at.y, z), 0.0, label=PLACE_CHAPEL, folder='Gameplay', tags=(PLACE_CHAPEL,))
    build.log(f'the chapel\'s middle at ({at.x:.0f}, {at.y:.0f}), tagged {PLACE_CHAPEL}')


def place_yard(build, chapel):
    """The chapel yard's fight: two waves of six Unpaid round the chapel, the Restless one with the second."""
    unpaid = story.actor_class('UnpaidCreature')
    cls = story.actor_class('EncounterSpawner')
    fence = story.layout_entry(build, 'obstacles', 'churchyardFence')
    if unpaid is None or cls is None or fence is None:
        if unpaid is None or cls is None:
            build.warn('no EncounterSpawner or UnpaidCreature class (build the game module first): no fight in the chapel '
                       'yard')
        return
    at = chapel.get_actor_location()
    # On the chapel's spot and turned as it is, so the spots (in its frame) stand round it wherever it's placed.
    yard = build.place(cls, (at.x, at.y, at.z + 50.0), chapel.get_actor_rotation().yaw, label=f'Encounter_{YARD}',
                       folder='Gameplay')
    yard.set_editor_property('spawner_id', unreal.Name(YARD))
    # "12 Unpaid in two waves, and one Restless": six Basic each wave, and the Restless one with the second.
    yard.set_editor_property('groups', [story.group(unpaid, YARD_WAVE, 'BASIC'),
                                        story.group(unpaid, 1, 'RARE', first_wave=2, last_wave=2)])
    yard.set_editor_property('num_waves', 2)
    yard.set_editor_property('wait_for_clear', True)
    yard.set_editor_property('wave_interval', YARD_WAVE_PAUSE)
    yard.set_editor_property('creature_tags', [unreal.Name(YARD_TAG)])
    # Main 4's second step only (from 0: from 1, before 2): a yard already cleared doesn't fill again on a later load.
    yard.set_editor_property('active_when', story.condition(during=MAIN4, from_step=1, before_step=2))
    yard.set_editor_property('spawn_points', [unreal.Vector(x, y, 0.0) for x, y in YARD_SPOTS])
    yard.set_editor_property('hunt_on_spawn', True)
    # "Inside the iron fence; gives up at the fence."
    yard.set_editor_property('ground_corners', [unreal.Vector(x, y, at.z) for x, y in fence['polygon']])
    yard.set_editor_property('max_ground_step', YARD_STEP)
    yard.set_editor_property('ground_max_rise', YARD_RISE)
    build.log(f'the chapel yard\'s fight ({YARD}: 2 waves of {YARD_WAVE} Unpaid, the Restless one with the second) round '
              f'the chapel, inside the churchyard fence, during {MAIN4}\'s second step')
    check_yard_clear(build, chapel)


def check_yard_clear(build, chapel):
    """Warns of the dressing's pieces (build_area_dressing.footprints: the churchyard's graves and dead trees among
    them) standing within YARD_CLEAR of a yard spot, as the spawner turns the spots with the chapel. The fence's line,
    the yard's edge, isn't counted."""
    # Reloaded, as the editor keeps modules between runs: a gameplay build may come after the dressing's tables changed.
    dressing = importlib.reload(importlib.import_module('build_area_dressing'))
    at = chapel.get_actor_location()
    yaw = math.radians(chapel.get_actor_rotation().yaw)
    spots = [(at.x + math.cos(yaw) * x - math.sin(yaw) * y, at.y + math.sin(yaw) * x + math.cos(yaw) * y)
             for x, y in YARD_SPOTS]
    crowded = []
    for x, y, piece_yaw, half_y, half_x, line in dressing.footprints(build.source, build.layout['placements']):
        if line:
            continue
        turn = math.radians(piece_yaw)
        for sx, sy in spots:
            # The spot in the piece's own frame (its X its front), and how far it is outside the piece's box.
            dx, dy = sx - x, sy - y
            ahead, right = dx * math.cos(turn) + dy * math.sin(turn), -dx * math.sin(turn) + dy * math.cos(turn)
            if math.hypot(max(abs(ahead) - half_x, 0.0), max(abs(right) - half_y, 0.0)) < YARD_CLEAR:
                crowded.append(f'({sx:.0f}, {sy:.0f})')
    if crowded:
        build.warn(f'the dressing stands within {YARD_CLEAR / 100.0:.1f} m of the chapel yard\'s spots at '
                   f'{", ".join(sorted(set(crowded)))}: move the piece (build_area_dressing.py) or the spot (YARD_SPOTS)')


def place_bell(build, chapel):
    """The bell rung from the rope's grip, the bell itself hung on the belfry's socket."""
    cls = story.actor_class('ChapelBell')
    grip = story.socket(chapel, 'Interact')
    axis = story.socket(chapel, 'Bell')
    if cls is None or grip is None or axis is None:
        build.warn('no ChapelBell class (build the game module first): no bell to ring' if cls is None
                   else 'the chapel has no Interact or Bell socket: no bell to ring')
        return
    at = grip.translation
    bell = build.place(cls, (at.x, at.y, at.z), chapel.get_actor_rotation().yaw, label='ChapelBell', folder='Gameplay',
                       tags=(BELL_TAG,))
    swing = bell.get_editor_property('bell')
    if unreal.EditorAssetLibrary.does_asset_exist(BELL_MESH):
        swing.set_static_mesh(unreal.load_asset(BELL_MESH))
    else:
        build.warn(f'no {BELL_MESH} yet (import Chapel.py): the bell rings unseen')
    # Its origin on the headstock's axis, turned as the socket is: its X along the axis, as the code swings it.
    swing.set_world_location_and_rotation(axis.translation, axis.rotation.rotator(), False, True)
    build.log(f'the chapel bell, rung from ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}), hung at {axis.translation.z:.0f} cm')


def place_reliquary(build, chapel):
    """The smashed Reliquary on the apse's plinth, facing the nave: its model when imported, else its stand-in."""
    cls = story.actor_class('ChapelReliquary')
    plinth = story.socket(chapel, 'Reliquary')
    if cls is None or plinth is None:
        build.warn('no ChapelReliquary class (build the game module first): no Reliquary' if cls is None
                   else 'the chapel has no Reliquary socket: no Reliquary')
        return
    at = plinth.translation
    reliquary = build.place(cls, (at.x, at.y, at.z), plinth.rotation.rotator().yaw, label='Reliquary', folder='Gameplay',
                            tags=(RELIQUARY_TAG,))
    # Snapped to the socket: its whole turn, not only its yaw.
    reliquary.set_actor_rotation(plinth.rotation.rotator(), False)
    model = next((path for path in RELIQUARY_MESHES if unreal.EditorAssetLibrary.does_asset_exist(path)), None)
    if model:
        reliquary.get_editor_property('mesh').set_static_mesh(unreal.load_asset(model))
    else:
        build.warn(f'no {" or ".join(RELIQUARY_MESHES)} yet (Art/Models/Props/Reliquary.py, the art session\'s): the '
                   f'Reliquary stands in plain shapes until it\'s imported and this runs again')
    # From Main 4's fourth step on (from 0: 3). Set last, so the editor builds it again and shows what play will.
    reliquary.set_editor_property('look_when', story.condition(during=MAIN4, from_step=3))
    build.log(f'the smashed Reliquary ({model or "its stand-in"}) on the plinth at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}), '
              f'looked at from {MAIN4}\'s fourth step')


def place_aldana(build, chapel):
    """Father Aldana's vestry door: barred, the bell and the Reliquary, the doc's words, the Sink, the lantern found."""
    cls = story.actor_class('SpeakerPoint')
    door = story.socket(chapel, 'Speaker')
    if cls is None or door is None:
        if cls is not None:
            build.warn('the chapel has no Speaker socket: Father Aldana can\'t be talked to')
        return
    at = door.translation
    # The first that holds is said: Main 4's last step (from 0: 4), the bell and the Reliquary (2 and 3), after Main 5 (the
    # lantern found), after Main 4 (the Sink); with none (before Main 4, and while the yard is held), the door stays barred.
    story.speaker(build, cls, (at.x, at.y, at.z), door.rotation.rotator().yaw, 'Speaker_Aldana', ALDANA_TAG,
                  name='Father Aldana', reach=story.TALK_REACH, line_set=story.lines(build, 'DA_Lines_AldanaBarred'),
                  topics=story.topics(build,
                                      (story.condition(during=MAIN4, from_step=4), 'DA_Lines_AldanaMain4'),
                                      (story.condition(during=MAIN4, from_step=2), 'DA_Lines_AldanaWaiting'),
                                      (story.condition(after=[MAIN5]), 'DA_Lines_AldanaAfterMain5'),
                                      (story.condition(after=[MAIN4]), 'DA_Lines_AldanaAfterMain4')))
    build.log(f'Father Aldana\'s vestry door at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')


def place_yard_grave(build, chapel):
    """The chapel yard's respawn grave at the foot of its fresh mound, open after Main 4, facing the chapel."""
    grave = placed_key(build, 'graveChapelYard')
    marker_cls = story.actor_class('RespawnMarker')
    if grave is None or marker_cls is None:
        return
    rise = story.socket(grave, 'Respawn')
    heap = rise.translation if rise else grave.get_actor_location()
    # Out of the grave's foot (the kit's front), as the family plot's.
    foot = grave.get_actor_forward_vector()
    x, y = heap.x + foot.x * MOUND_STAND_OUT, heap.y + foot.y * MOUND_STAND_OUT
    if chapel is not None:
        door = chapel.get_actor_location()
        yaw = story.facing((x, y), (door.x, door.y))
    else:
        yaw = grave.get_actor_rotation().yaw
    marker = build.place(marker_cls, (x, y, story.ground(x, y, heap.z, ignore=[grave])), yaw, label='Respawn_ChapelYard',
                         folder='Gameplay')
    marker.set_editor_property('marker_id', unreal.Name(YARD_GRAVE))
    # What the map calls it (the layout has no label for it; the design's words: "the chapel yard's respawn grave").
    marker.set_editor_property('display_name', unreal.Text('Chapel yard'))
    marker.set_editor_property('active_after_mission', unreal.Name(MAIN4))
    build.log(f'the chapel yard\'s respawn grave at ({x:.0f}, {y:.0f}), open after {MAIN4}')


# ---------------------------------------------------------------------------
# After Main 4: the Unpaid walk boot hill and the north road
# ---------------------------------------------------------------------------

def roaming(build, cls, unpaid, spawner_id, x, y, z):
    """A roaming stretch's spawner: three Unpaid of the area's ranks, on from Main 4's finish, standing about."""
    made = build.place(cls, (x, y, z + 50.0), 0.0, label=f'Encounter_{spawner_id}', folder='Gameplay')
    made.set_editor_property('spawner_id', unreal.Name(spawner_id))
    made.set_editor_property('groups', [roaming_group(unpaid, ROAMING_COUNT)])
    made.set_editor_property('creature_tags', [unreal.Name(f'Unpaid_{spawner_id}')])
    made.set_editor_property('active_when', story.condition(after=[MAIN4]))
    # They walk their stretch until someone comes near, rather than come at the player as they appear.
    made.set_editor_property('hunt_on_spawn', False)
    return made


def place_roaming(build):
    """Boot hill's Unpaid (the zone its ground) and the north road's, between the hanging tree and the road."""
    unpaid = story.actor_class('UnpaidCreature')
    cls = story.actor_class('EncounterSpawner')
    if unpaid is None or cls is None:
        build.warn('no EncounterSpawner or UnpaidCreature class (build the game module first): no Unpaid on boot hill or '
                   'the north road')
        return
    hill = story.layout_entry(build, 'zones', 'bootHill')
    if hill is not None:
        hx, hy = story.centroid(hill['polygon'])
        hz = story.ground_at(hx, hy)
        spawner = roaming(build, cls, unpaid, BOOT_HILL, hx, hy, hz)
        spawner.set_editor_property('spawn_radius', HILL_SPREAD)
        spawner.set_editor_property('ground_corners', [unreal.Vector(x, y, hz) for x, y in hill['polygon']])
        build.log(f'boot hill\'s Unpaid ({BOOT_HILL}: {ROAMING_COUNT}) at ({hx:.0f}, {hy:.0f}), after {MAIN4}')
    tree = story.layout_entry(build, 'obstacles', 'hangingTree')
    road = story.layout_entry(build, 'roads', 'northRoad')
    if tree is not None and road is not None and len(road['path']) >= 2:
        tx, ty = story.centroid(tree['polygon'])
        rx, ry = nearest_on_path(road['path'], (tx, ty))
        # Between the tree and the road where it passes nearest, so the stretch holds both.
        sx, sy = (tx + rx) * 0.5, (ty + ry) * 0.5
        spawner = roaming(build, cls, unpaid, NORTH_ROAD, sx, sy, story.ground_at(sx, sy))
        spawner.set_editor_property('give_up_radius', ROAD_STRETCH)
        spawner.set_editor_property('spawn_radius', ROAD_SPREAD)
        build.log(f'the north road\'s Unpaid ({NORTH_ROAD}: {ROAMING_COUNT}) by the hanging tree at ({sx:.0f}, {sy:.0f}), '
                  f'after {MAIN4}')


# ---------------------------------------------------------------------------
# Hob's perches, and everything
# ---------------------------------------------------------------------------

def hob_spots(chapel):
    """Hob's chapel perches, (x, y, z, yaw) each or None without its socket: on the door hood's ridge, facing out of the
    front as the socket does; on the vestry lantern's bracket, facing out of the vestry door as Aldana's socket does."""
    spots = {'hood': None, 'lantern': None}
    if chapel is None:
        return spots
    hood = story.socket(chapel, 'Perch_Hood')
    lantern = story.socket(chapel, 'Perch_Lantern')
    door = story.socket(chapel, 'Speaker')
    if hood is not None:
        at = hood.translation
        spots['hood'] = (at.x, at.y, at.z, hood.rotation.rotator().yaw)
    if lantern is not None:
        at = lantern.translation
        spots['lantern'] = (at.x, at.y, at.z, (door if door is not None else lantern).rotation.rotator().yaw)
    return spots


def place(build):
    """Everything above, in the Gameplay folder. Returns Hob's chapel perches for build_area_story's place_hob."""
    chapel = story.placed(build, 'Chapel')
    if chapel is not None:
        place_chapel_marker(build, chapel)
        place_yard(build, chapel)
        place_bell(build, chapel)
        place_reliquary(build, chapel)
        place_aldana(build, chapel)
    else:
        build.warn('no chapel: Main 4 has nowhere to happen (import Chapel.py and build the whole level)')
    place_yard_grave(build, chapel)
    place_roaming(build)
    return hob_spots(chapel)
