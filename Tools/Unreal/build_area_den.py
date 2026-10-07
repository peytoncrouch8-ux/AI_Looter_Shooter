"""Side 3, "The Gravemother" (Docs/Areas/RansomsRest.md), for build_area_story.py: the Gravemother's lair in the Sink's
den and the den's place, placed from Den Rock (SM_DenRock, Art/Models/Rocks/Outcrops.py: the den under its overhang,
its SOCKET_DenMouth on the den's floor at the mouth and SOCKET_Den toward its back, the mouth facing the rock's +X out
over the Sink) when build_area.py has placed it with those sockets, else from the layout (the Sink's zone and Den Rock's
placement), so they follow the level whenever it's rebuilt. build_area_story's place() calls place() here (reloaded each
run, as the editor keeps modules between runs); it uses build_area_story's helpers. Everything goes in the area's
Gameplay folder; a piece whose class isn't built yet is left out with a warning.

- The den's place (a target point tagged Place_Den) halfway into the den, at about a player's middle over its floor:
  Side 3's "Enter the den" (Tools/Unreal/create_side_mission_assets.py) ends within 4.5 m of it, measured with its
  height, so the Sink's rim over the den doesn't count. Without the den's model, at its mouth.
- The Gravemother's lair (AEncounterSpawner Gravemother, its LegendaryId Gravemother) on the den's floor at the mouth,
  facing out over the Sink: one AGravemotherCreature, Legendary, lurking a few metres into the den until she notices
  someone, tagged Gravemother (Side 3's "Kill the Gravemother" clears this encounter). On after Main 5 ("Side 3 opens
  after Main 5"); until Main 5 is made, the console brings her: Looter.Legendary.Return (or Looter.Encounter.Wave
  Gravemother force). She hunts the den and the Sink's floor (within 36 m of the mouth, and no more than 3 m above or
  below it), so she gives up a little way up the ramp and never hunts the rim. She comes back on an arrival at least 20
  minutes of play after her death: the lair (its LegendaryId) and the session keep that.
- The den's dressing (the art session's DenDressing kit, approved 2026-10-07): bones, the larder, cocoons hung from the
  brow, a coffin dragged in, the dead's hat and boot, at the places its placement table gives in SOCKET_DenMouth's space
  (Art/Models/Props/DenDressing.placement.json, written by DenDressing.py). Her clear way down the den's middle stays
  empty. Only with Den Rock's sockets: without them there's no den to dress.
"""
import json
import math
import os

import unreal

import build_area_story as story

# The story's ids and tags, as the C++ (Creatures/GravemotherCreature.h: Gravemother::LairId and DenPlaceTag) and the
# mission asset (DA_Mission_Side3) name them.
MAIN5 = 'Main5'
LAIR = 'Gravemother'
PLACE_DEN = 'Place_Den'

# Den Rock's placement kind (layout.json placements) and its sockets (Outcrops.py den_rock()).
DEN_ROCK = 'DenRock'
MOUTH_SOCKET = 'DenMouth'
BACK_SOCKET = 'Den'
# Without SOCKET_Den: the den's back this far in from the mouth along the rock's -X (cm; its pocket ends about 9 m in).
DEN_DEPTH = 730.0

# Where she lurks and where the den's place stands: these shares of the way from the mouth to the den's back.
LURK_SHARE = 0.4
PLACE_SHARE = 0.5

# Without Den Rock's sockets: the mouth this far out from Den Rock's pivot (on the lip over the mouth) toward the Sink's
# middle, on the Sink's floor, and her spot this far behind it (cm).
MOUTH_OUT = 100.0
LURK_IN = 350.0
# The Sink's floor is looked for round its middle, this far out, and the lowest found taken (cm): a block or a coffin
# lying there isn't the floor.
FLOOR_PROBE = 300.0

# Her lair (cm): she appears as the player comes near the Sink; her ground is the den and the floor round the mouth, a
# player more than MAX_RISE above or below the mouth's floor isn't on it (a little way up the ramp; the rim is 12 m up);
# her spots stay on the den's level.
ACTIVATION = 4000.0
GIVE_UP = 3600.0
MAX_RISE = 300.0
GROUND_STEP = 250.0

# The den's dressing: where its pieces lie in SOCKET_DenMouth's own space (+X out of the den, +Y right looking out, cm),
# and the folder its models import to.
DRESSING_TABLE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'Art', 'Models', 'Props',
                              'DenDressing.placement.json')
DRESSING_FOLDER = '/Game/Art/Props'


def lerp(a, b, share):
    return tuple(a[i] + (b[i] - a[i]) * share for i in range(3))


def to_local(origin, yaw, point):
    """Point (x, y) in the frame of something at origin turned yaw degrees (its +X along yaw), as a spawner's own points
    are (AEncounterSpawner::SpawnPoints)."""
    dx, dy = point[0] - origin[0], point[1] - origin[1]
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return dx * c + dy * s, -dx * s + dy * c


def sink_floor(build):
    """The Sink's middle (x, y) and its floor's height there, or None without the layout's zone sink."""
    zone = story.layout_entry(build, 'zones', 'sink')
    if zone is None:
        return None
    cx, cy = story.centroid(zone['polygon'])
    probes = [(cx, cy)] + [(cx + dx, cy + dy) for dx, dy in ((FLOOR_PROBE, 0.0), (-FLOOR_PROBE, 0.0), (0.0, FLOOR_PROBE),
                                                             (0.0, -FLOOR_PROBE))]
    return cx, cy, min(story.ground_at(x, y) for x, y in probes)


def den_rock(build):
    """The Den Rock build_area.py placed (labelled with its placement key), or None, quietly: the layout stands in."""
    keys = [key for key, spot in build.layout['placements'].items() if spot['kind'] == DEN_ROCK]
    if not keys:
        return None
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if actor.get_actor_label() == keys[0] and unreal.Name(build.tag) in actor.tags:
            return actor
    return None


def den_from_rock(build, rock):
    """(mouth, yaw out of the den, her spot, the den's middle) from Den Rock's sockets, or None without SOCKET_DenMouth."""
    mouth = story.socket(rock, MOUTH_SOCKET)
    if mouth is None:
        build.warn(f'{rock.get_actor_label()} has no {MOUTH_SOCKET} socket (import the reworked Den Rock): the lair stands '
                   f'where the layout puts the den')
        return None
    m = mouth.translation
    yaw = rock.get_actor_rotation().yaw
    back = story.socket(rock, BACK_SOCKET)
    if back is not None:
        b = back.translation
        deep = (b.x, b.y, b.z)
    else:
        ahead = rock.get_actor_forward_vector()
        deep = (m.x - ahead.x * DEN_DEPTH, m.y - ahead.y * DEN_DEPTH, m.z)
    at = (m.x, m.y, m.z)
    return at, yaw, lerp(at, deep, LURK_SHARE), lerp(at, deep, PLACE_SHARE)


def den_from_layout(build):
    """The same from the layout: the mouth on the Sink's wall under Den Rock's placement, facing the Sink's middle, on its
    floor; her spot behind it; the den's place at the mouth. None without the Sink or Den Rock in the layout."""
    floor = sink_floor(build)
    spot = next((s for s in build.layout['placements'].values() if s['kind'] == DEN_ROCK), None)
    if floor is None or spot is None:
        build.warn('the layout has no Sink zone or no Den Rock placement: no Gravemother')
        return None
    cx, cy, fz = floor
    rx, ry = spot['location'][:2]
    yaw = story.facing((rx, ry), (cx, cy))
    out_x, out_y = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    at = (rx + out_x * MOUTH_OUT, ry + out_y * MOUTH_OUT, fz)
    lurk = (at[0] - out_x * LURK_IN, at[1] - out_y * LURK_IN, fz)
    return at, yaw, lurk, at


def place_den_marker(build, inside):
    """The den's place, at about a player's middle over its floor (traced from above, it would find Den Rock's dome)."""
    x, y, z = inside
    build.place(unreal.TargetPoint, (x, y, z + story.MARKER_UP), 0.0, label=PLACE_DEN, folder='Gameplay', tags=(PLACE_DEN,))
    build.log(f'the den\'s place at ({x:.0f}, {y:.0f}, {z:.0f}), tagged {PLACE_DEN}')


def place_lair(build, mouth, yaw, lurk):
    """The Gravemother's lair at the den's mouth: her, Legendary, after Main 5, back 20 minutes of play after her death."""
    cls = story.actor_class('EncounterSpawner')
    creature = story.actor_class('GravemotherCreature')
    if cls is None or creature is None:
        build.warn('no EncounterSpawner or GravemotherCreature class (build the game module first): no Gravemother')
        return
    mx, my, mz = mouth
    lair = build.place(cls, (mx, my, mz + 50.0), yaw, label=f'Encounter_{LAIR}', folder='Gameplay')
    lair.set_editor_property('spawner_id', unreal.Name(LAIR))
    # A Legendary monster's lair: her death is kept in the session, and she's away until an arrival 20 minutes of play on.
    lair.set_editor_property('legendary_id', unreal.Name(LAIR))
    # One Gravemother, Legendary (her name in orange on her tag), at her own size (her class's: 1.8 times a spider's).
    lair.set_editor_property('groups', [story.group(creature, 1, 'LEGENDARY')])
    lair.set_editor_property('creature_tags', [unreal.Name(LAIR)])
    lair.set_editor_property('active_when', story.condition(after=[MAIN5]))
    # She lurks a few metres into the den, in its gloom, and comes out when she notices someone.
    lx, ly = to_local(mouth, yaw, lurk)
    lair.set_editor_property('spawn_points', [unreal.Vector(lx, ly, 0.0)])
    lair.set_editor_property('hunt_on_spawn', False)
    lair.set_editor_property('activation_radius', ACTIVATION)
    # "The den and the Sink floor; gives up at the foot of the ramp."
    lair.set_editor_property('give_up_radius', GIVE_UP)
    lair.set_editor_property('ground_max_rise', MAX_RISE)
    lair.set_editor_property('max_ground_step', GROUND_STEP)
    build.log(f'the Gravemother\'s lair ({LAIR}, Legendary) at the den\'s mouth ({mx:.0f}, {my:.0f}, {mz:.0f}) facing yaw '
              f'{yaw:.0f}, after {MAIN5}, back 20 minutes of play after her death')


def place_dressing(build, rock):
    """The den's dressing from its placement table, each piece in the mouth socket's frame (a mirrored one by its scale)."""
    mouth = story.socket(rock, MOUTH_SOCKET)
    if mouth is None:
        return
    with open(DRESSING_TABLE, encoding='utf-8') as f:
        pieces = json.load(f)['placements']
    turn = mouth.rotation.rotator().yaw
    placed, missing = 0, set()
    for piece in pieces:
        model = unreal.load_asset(f'{DRESSING_FOLDER}/{piece["asset"]}')
        if model is None:
            missing.add(piece['asset'])
            continue
        at = mouth.transform_location(unreal.Vector(*piece['unreal_cm']))
        build.place(model, (at.x, at.y, at.z), turn + piece['yaw_unreal_deg'], label=f'Den_{piece["asset"][3:]}',
                    folder='Gameplay', scale=tuple(piece['scale_unreal']))
        placed += 1
    if missing:
        build.warn(f"the den's dressing lacks {', '.join(sorted(missing))} (import Art/Models/Props/DenDressing.py)")
    build.log(f"the den's dressing: {placed} pieces round the mouth and down its walls")


def place(build):
    """The den's place and the Gravemother's lair, in the Gameplay folder (build_area_story's place() calls this). Only a
    level whose layout has Den Rock (Ransom's Rest) gets them."""
    if not any(spot['kind'] == DEN_ROCK for spot in build.layout['placements'].values()):
        return
    rock = den_rock(build)
    den = den_from_rock(build, rock) if rock is not None else None
    if den is None:
        if rock is None:
            build.warn('Den Rock isn\'t placed (build the whole level with SM_DenRock imported): the lair stands where the '
                       'layout puts the den')
        den = den_from_layout(build)
    if den is None:
        return
    mouth, yaw, lurk, inside = den
    place_den_marker(build, inside)
    place_lair(build, mouth, yaw, lurk)
    if rock is not None:
        place_dressing(build, rock)
