"""Main 7, "Given to the Wind" (the Wardens story, 2026-11-01; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 11, lane
MAIN67's sub-lane W5 HOME: Docs/Work/2026-10-31-Wardens/main67_src/CONTRACT67.md), its world pieces outside Abel's fight,
for build_area_story.py: its place() calls place() here (reloaded each run, as the editor keeps modules between runs)
after the chapel (build_area_chapel.py: the Reliquary), Main 4 (build_area_main4.py: Finch at the Reliquary) and the deck
(build_area_deck.py), before Hob (who takes the spots this hands back). Everything goes in the area's Gameplay/Main7
folder, so a "gameplay" build places it all again; a piece whose class, model or ground isn't there yet is left out with
a warning.

What it places, from the built level (the layout's roads, the chapel's sockets, the Reliquary, traces):
- The barge's fallen soul coal (ASoulCoalField, one per stretch, instanced, smoking): along the west road and the keeper's
  path, across the valley under the barge's last line (Main 6's BargeFall: from the chapel knoll west to the Rim) and on
  the canyon's rim round Gravewind Point; there from Main 6's end until Saint Ada wakes (Main7.SaintAdaAwake).
- Twelve lumps with a soul in each (ASoulCoalLump, tagged SoulCoal, LumpId Road1..7 / Path1..5): along the two ways to the
  Point, just off the road, a hold of a second frees each (Main 7 step 0's optional, a lasting objective) and its soul
  follows Abel's lantern (UCarriedLanternSubsystem); cracked, it stays cracked (SoulCoal.<Id>).
- WestRoadCrew (AEncounterSpawner): the Company men who bailed out of the barge, three riflemen and two hose-men at level 6,
  on the west road halfway between the deck and the chapel, during Main 7's walk home (step 4) until the turn-in; their
  hose-men latch Abel's lantern, not Sully (the C++). Hob's word as they come into sight (an unseen speaker point's mutter).
- Abel's lantern's rest on Saint Ada's Reliquary (AReliquaryLantern, tagged Reliquary_Lantern: it seats itself on her
  ember), step 5's hold; the Reliquary lit for good once she wakes (its LitWhen: Main7.SaintAdaAwake).
- Abel's soul by the Reliquary (AAbelOnBoard standing, his soul's look, tagged Speaker_Abel), shown once she wakes, his
  words story_texts.TOPICS['Speaker_Abel'] (DA_Lines_AbelReliquary).
- Delia at the chapel's front door (ADeliaRansom, tagged Speaker_DeliaChapel: Main 7's turn-in, card then her words,
  story_texts.TOPICS['Speaker_DeliaChapel']), during Main 7 from step 5 (she has come up the north road).
Returns Hob's Main 7 spots for build_area_story.place_hob: {'chapelDoor': on the front door's head, 'roadCoal': by the
first soul coal on the west road} ((x, y, z, yaw) or None).
"""
import importlib
import math
import os
import random
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Main7'
MAIN6, MAIN7 = 'Main6', 'Main7'

# ---------------------------------------------------------------------------
# Names (the C++'s: World/CarriedLanternRules.h GivenToTheWind; the mission data's)
# ---------------------------------------------------------------------------

AWAKE_FLAG = 'Main7.SaintAdaAwake'
COAL_TAG = 'SoulCoal'
RELIQUARY_TAG = 'Reliquary_Chapel'
RELIQUARY_LANTERN_TAG = 'Reliquary_Lantern'
ABEL_TAG = 'Speaker_Abel'
DELIA_CHAPEL_TAG = 'Speaker_DeliaChapel'
ROAD_CREW = 'WestRoadCrew'
ROAD_CREW_TAG = 'Dredger_WestRoadCrew'
ROAD_WORD = 'Mutter_WestRoadCrew'
CARRY_STEP, RELIQUARY_STEP, READY_STEP = 4, 5, 6
# Line sets (create_story_lines.py).
ROAD_LINES = 'DA_Lines_Main7Road'
ABEL_LINES = 'DA_Lines_AbelReliquary'
ARRIVE_LINES = 'DA_Lines_Main7Arrive'
ARRIVE_WORD = 'Mutter_ReliquaryArrive'
# Finch says it as Sully comes in the door with the lantern (the nave is about 20 m).
ARRIVE_RADIUS = 2200.0

# ---------------------------------------------------------------------------
# Numbers (cm, degrees)
# ---------------------------------------------------------------------------

SEED = 7031
# The interactable lumps: along each road between these shares of its length, this far off its middle (either side).
ROAD_LUMPS = (('westRoad', 'Road', 7, (0.1, 0.92)), ('keepersPath', 'Path', 5, (0.12, 0.95)))
LUMP_OFF = (230.0, 340.0)
LUMP_SIZE = (28.0, 38.0)
# The fields: (label, how many lumps, how they're spread). Roads: off their middle up to this far; the valley: a corridor
# this wide round the straight line from the chapel knoll to the deck; the rim: a ring round the Point.
FIELD_ROADS = (('westRoad', 'WestRoad', 36, 650.0), ('keepersPath', 'KeepersPath', 24, 500.0))
VALLEY_LUMPS, VALLEY_HALF = 34, 3500.0
RIM_LUMPS, RIM_RING = 18, (1500.0, 4200.0)
FIELD_SIZE = (16.0, 42.0)
# A lump never sits more than this over or under the ground found round it (a rock, a step): moved along, else left out.
LUMP_STEP = 80.0
# The west road's crew: halfway along the west road (share), spread this far, active from this far.
CREW_AT = 0.55
CREW_SPREAD, CREW_ACTIVATION, CREW_GIVE_UP = 900.0, 5000.0, 5500.0
CREW_GROUPS = (('DredgerRifleman', 3), ('DredgerHoseMan', 2))
CREW_LEVEL = 6
CREW_WORD_RADIUS = 3800.0
# Abel's soul beside the Reliquary: off her side (the plinth's right), a little toward the nave; facing the nave.
ABEL_SIDE, ABEL_FORWARD = 115.0, 55.0
# Delia just inside the front door (the Crepe socket is on its head casing, 2.3 m up): this far in, on the floor.
DELIA_IN = 120.0
DELIA_HALF_HEIGHT = 68.0
DELIA_REACH = 350.0
DOOR_HEAD_UP = 230.0


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def texts():
    import story_texts
    return importlib.reload(story_texts)


def main2():
    return importlib.reload(importlib.import_module('build_area_main2'))


def remove_stale(build, label):
    return main2().remove_stale(build, label)


def find_tagged(tag):
    return main2().find_tagged(tag)


def load(path):
    return unreal.load_asset(path) if path and unreal.EditorAssetLibrary.does_asset_exist(path) else None


def road_path(build, road_id):
    entry = story.layout_entry(build, 'roads', road_id)
    return [tuple(p[:2]) for p in entry['path']] if entry and len(entry.get('path', [])) >= 2 else None


def along(path, share):
    """The point share of the way along a polyline, and its direction there (flat, unit)."""
    lengths = [math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(path, path[1:])]
    total = sum(lengths) or 1.0
    want = max(0.0, min(1.0, share)) * total
    for (a, b), length in zip(zip(path, path[1:]), lengths):
        if want <= length or (a, b) == (path[-2], path[-1]):
            t = want / length if length > 0 else 0.0
            d = ((b[0] - a[0]) / (length or 1.0), (b[1] - a[1]) / (length or 1.0))
            return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t), d
        want -= length
    return path[-1], (1.0, 0.0)


def ground_near(x, y, near_z):
    """The ground at (x, y) traced from just over near_z (the road's height beside it), never a tree's crown."""
    return story.ground(x, y, near_z)


def settled(x, y, near_z):
    """(x, y, z) on the ground, or None where it isn't level enough for a lump (a rock's top, a step, a drop)."""
    z = ground_near(x, y, near_z)
    if abs(z - near_z) > 300.0:
        return None
    probes = [ground_near(x + dx, y + dy, z) for dx, dy in ((40.0, 0.0), (-40.0, 0.0), (0.0, 40.0), (0.0, -40.0))]
    if any(abs(p - z) > LUMP_STEP for p in probes):
        return None
    return x, y, z


def lump_transform(rng, mesh, at, size_range):
    """A lump's transform: its foot on the ground, a random turn and tilt, sized to size_range across (by its mesh)."""
    box = mesh.get_bounding_box()
    lo, hi = box.get_editor_property('min'), box.get_editor_property('max')
    across = max(hi.x - lo.x, hi.y - lo.y, 1.0)
    size = rng.uniform(*size_range)
    scale = size / across
    x, y, z = at
    # Half sunk: it fell, it didn't land on a plate.
    foot = z - lo.z * scale * 0.8 - size * 0.18
    rotation = unreal.Rotator(roll=rng.uniform(-18.0, 18.0), pitch=rng.uniform(-18.0, 18.0), yaw=rng.uniform(0.0, 360.0))
    return unreal.Transform(unreal.Vector(x, y, foot), rotation, unreal.Vector(scale, scale, scale * 0.8))


# ---------------------------------------------------------------------------
# The soul coal
# ---------------------------------------------------------------------------

def coal_mesh(build):
    """The fields' mesh as ASoulCoalField shows it (the Tripo soul coal, else its stand-in rock), for sizing."""
    return load('/Game/Art/Tripo/SM_SoulCoal') or load('/Game/Art/Rocks/SM_Rock_B')


def field_spots(build, rng):
    """{label: [(x, y, z)]}: each field's lumps on the ground."""
    fields = {}
    for road_id, label, count, spread in FIELD_ROADS:
        path = road_path(build, road_id)
        if path is None:
            continue
        spots = []
        for k in range(count * 3):
            if len(spots) >= count:
                break
            (cx, cy), d = along(path, rng.random())
            side = (-d[1], d[0])
            off = rng.uniform(-spread, spread)
            mid_z = story.ground_at(cx, cy, warn=False)
            spot = settled(cx + side[0] * off, cy + side[1] * off, mid_z)
            if spot:
                spots.append(spot)
        fields[label] = spots
    chapel = find_tagged(RELIQUARY_TAG)
    point = find_tagged('Place_GravewindPoint')
    if chapel is not None and point is not None:
        a, b = chapel.get_actor_location(), point.get_actor_location()
        line = (b.x - a.x, b.y - a.y)
        length = math.hypot(*line) or 1.0
        d = (line[0] / length, line[1] / length)
        side = (-d[1], d[0])
        spots = []
        for k in range(VALLEY_LUMPS * 4):
            if len(spots) >= VALLEY_LUMPS:
                break
            t = rng.uniform(0.1, 0.92)
            off = rng.uniform(-VALLEY_HALF, VALLEY_HALF)
            x, y = a.x + line[0] * t + side[0] * off, a.y + line[1] * t + side[1] * off
            top = story.ground_at(x, y, warn=False)
            spot = settled(x, y, top)
            if spot:
                spots.append(spot)
        fields['Valley'] = spots
        ring = []
        for k in range(RIM_LUMPS * 4):
            if len(ring) >= RIM_LUMPS:
                break
            angle = rng.uniform(0.0, math.tau)
            radius = rng.uniform(*RIM_RING)
            x, y = b.x + math.cos(angle) * radius, b.y + math.sin(angle) * radius
            top = story.ground_at(x, y, default=-1.0e6, warn=False)
            # Over the canyon nothing is found: the coal fell down there, out of sight.
            if top < b.z - 1500.0:
                continue
            spot = settled(x, y, top)
            if spot:
                ring.append(spot)
        fields['Rim'] = ring
    else:
        build.warn('no Reliquary or Place_GravewindPoint: no coal across the valley or on the rim')
    return fields


def place_fields(build, rng):
    cls = story.actor_class('SoulCoalField')
    mesh = coal_mesh(build)
    if cls is None or mesh is None:
        build.warn('no SoulCoalField class (build the game module first) or no coal mesh: no coal over the Rest')
        return 0
    total = 0
    for label, spots in field_spots(build, rng).items():
        if not spots:
            continue
        remove_stale(build, f'SoulCoal_{label}')
        cx = sum(s[0] for s in spots) / len(spots)
        cy = sum(s[1] for s in spots) / len(spots)
        cz = sum(s[2] for s in spots) / len(spots)
        field = build.place(cls, (cx, cy, cz), 0.0, label=f'SoulCoal_{label}', folder=FOLDER)
        field.set_lumps([lump_transform(rng, mesh, spot, FIELD_SIZE) for spot in spots])
        total += len(spots)
    build.log(f'the barge\'s soul coal: {total} lumps over the Rest (after {MAIN6}, until Saint Ada wakes)')
    return total


def place_lumps(build, rng):
    """The twelve lumps with souls; returns the first west-road one's spot (Hob's)."""
    cls = story.actor_class('SoulCoalLump')
    if cls is None:
        build.warn('no SoulCoalLump class (build the game module first): no souls to free in the coal')
        return None
    first = None
    placed = 0
    for road_id, prefix, count, (start, end) in ROAD_LUMPS:
        path = road_path(build, road_id)
        if path is None:
            continue
        for k in range(count):
            share = start + (end - start) * (k + 0.5) / count
            spot = None
            # Off one side of the road, the next off the other; nudged along until the ground takes it.
            for nudge in (0.0, 0.015, -0.015, 0.03, -0.03):
                (cx, cy), d = along(path, share + nudge)
                side = (-d[1], d[0])
                sign = 1.0 if k % 2 == 0 else -1.0
                off = sign * rng.uniform(*LUMP_OFF)
                spot = settled(cx + side[0] * off, cy + side[1] * off, story.ground_at(cx, cy, warn=False))
                if spot:
                    break
            if spot is None:
                build.warn(f'soul coal {prefix}{k + 1}: no level ground off {road_id} there: left out')
                continue
            lump_id = f'{prefix}{k + 1}'
            remove_stale(build, f'SoulCoal_{lump_id}')
            lump = build.place(cls, spot, rng.uniform(0.0, 360.0), label=f'SoulCoal_{lump_id}', folder=FOLDER, tags=(COAL_TAG,))
            lump.set_editor_property('lump_id', unreal.Name(lump_id))
            lump.set_editor_property('size', rng.uniform(*LUMP_SIZE))
            placed += 1
            if first is None and road_id == 'westRoad':
                first = (spot[0], spot[1], spot[2], math.degrees(math.atan2(d[1], d[0])))
    if placed != 12:
        build.warn(f'{placed} soul coal lumps placed, not 12: Main 7\'s optional "free the souls" counts 12')
    build.log(f'{placed} lumps of soul coal with souls in them along the west road and the keeper\'s path (tagged {COAL_TAG})')
    return first


# ---------------------------------------------------------------------------
# The west road's Company men
# ---------------------------------------------------------------------------

def place_crew(build):
    spawner_cls = story.actor_class('EncounterSpawner')
    path = road_path(build, 'westRoad')
    if spawner_cls is None or path is None:
        build.warn('no EncounterSpawner class or no westRoad: no Company men on the way home')
        return
    groups, missing = [], []
    for name, count in CREW_GROUPS:
        creature = story.actor_class(name)
        if creature is None:
            missing.append(name)
            continue
        made = story.group(creature, count, 'BASIC', 1, 1)
        made.set_editor_property('level', CREW_LEVEL)
        groups.append(made)
    if not groups:
        build.warn(f'no {", ".join(missing)} class yet: nobody on the west road')
        return
    (x, y), d = along(path, CREW_AT)
    z = story.ground_at(x, y)
    remove_stale(build, f'Encounter_{ROAD_CREW}')
    spawner = build.place(spawner_cls, (x, y, z + 50.0), math.degrees(math.atan2(d[1], d[0])), label=f'Encounter_{ROAD_CREW}',
                          folder=FOLDER)
    spawner.set_editor_property('spawner_id', unreal.Name(ROAD_CREW))
    spawner.set_editor_property('groups', groups)
    spawner.set_editor_property('num_waves', 1)
    spawner.set_editor_property('max_alive', sum(count for _, count in CREW_GROUPS))
    spawner.set_editor_property('creature_tags', [unreal.Name(ROAD_CREW_TAG)])
    when = texts().ENCOUNTERS.get(ROAD_CREW, dict(during=MAIN7, from_step=CARRY_STEP, before_step=READY_STEP))
    spawner.set_editor_property('active_when', texts().condition(when))
    spawner.set_editor_property('activation_radius', CREW_ACTIVATION)
    spawner.set_editor_property('give_up_radius', CREW_GIVE_UP)
    spawner.set_editor_property('spawn_radius', CREW_SPREAD)
    spawner.set_editor_property('hunt_on_spawn', True)
    # Hob's word as they come into sight (an unseen speaker point's mutter, once a pass).
    point_cls = story.actor_class('SpeakerPoint')
    word = story.lines(build, ROAD_LINES)
    if point_cls is not None and word is not None:
        remove_stale(build, ROAD_WORD)
        point = build.place(point_cls, (x, y, z + 120.0), 0.0, label=ROAD_WORD, folder=FOLDER, tags=(ROAD_WORD,))
        talk = point.get_editor_property('speaker_point')
        talk.set_editor_property('speaker_name', unreal.Text('Hob'))
        talk.set_editor_property('talkable', False)
        talk.set_editor_property('mutter_radius', CREW_WORD_RADIUS)
        talk.set_editor_property('mutters', [story.topic(texts().condition(when), word)])
    build.log(f'{ROAD_CREW}: three riflemen and two hose-men (level {CREW_LEVEL}) on the west road at ({x:.0f}, {y:.0f}), during '
              f'{MAIN7} from step {CARRY_STEP}{" (no " + ", ".join(missing) + ")" if missing else ""}')


# ---------------------------------------------------------------------------
# The chapel: the Reliquary's lantern, Abel's soul, Delia at the door
# ---------------------------------------------------------------------------

def place_reliquary(build):
    """The lantern's rest on her lid, and her light the waking's for good. Returns the Reliquary (or None)."""
    reliquary = find_tagged(RELIQUARY_TAG)
    cls = story.actor_class('ReliquaryLantern')
    if reliquary is None or cls is None:
        build.warn('no Reliquary_Chapel (build_area_chapel.py) or no ReliquaryLantern class: Main 7\'s last step has nowhere '
                   'to set the lantern')
        return reliquary
    reliquary.set_editor_property('lit_when', texts().condition(dict(with_flags=[AWAKE_FLAG])))
    at = reliquary.get_actor_location()
    remove_stale(build, 'Reliquary_Lantern')
    build.place(cls, (at.x, at.y, at.z + 60.0), reliquary.get_actor_rotation().yaw, label='Reliquary_Lantern', folder=FOLDER,
                tags=(RELIQUARY_LANTERN_TAG,))
    # Finch's word as Sully comes in with the lantern (step 5): an unseen speaker point at her, muttering once a pass.
    point_cls = story.actor_class('SpeakerPoint')
    word = story.lines(build, ARRIVE_LINES)
    if point_cls is not None and word is not None:
        remove_stale(build, ARRIVE_WORD)
        point = build.place(point_cls, (at.x, at.y, at.z + 150.0), 0.0, label=ARRIVE_WORD, folder=FOLDER, tags=(ARRIVE_WORD,))
        talk = point.get_editor_property('speaker_point')
        talk.set_editor_property('speaker_name', unreal.Text('Brother Finch'))
        talk.set_editor_property('talkable', False)
        talk.set_editor_property('mutter_radius', ARRIVE_RADIUS)
        talk.set_editor_property('mutters', [story.topic(texts().condition(dict(during=MAIN7, from_step=RELIQUARY_STEP,
                                                                                before_step=READY_STEP)), word)])
    build.log(f'Abel\'s lantern\'s rest on Saint Ada\'s Reliquary at ({at.x:.0f}, {at.y:.0f}); her light lit once {AWAKE_FLAG}')
    return reliquary


def place_abel(build, chapel):
    cls = story.actor_class('AbelOnBoard')
    plinth = story.socket(chapel, 'Reliquary') if chapel is not None else None
    if cls is None or plinth is None:
        build.warn('no AbelOnBoard class or no chapel Reliquary socket: Abel\'s soul isn\'t by her')
        return
    turn = plinth.rotation.rotator()
    ahead, right = turn.get_forward_vector(), turn.get_right_vector()
    at = plinth.translation + right * ABEL_SIDE + ahead * ABEL_FORWARD
    floor = story.onto_host(chapel, unreal.Vector(at.x, at.y, at.z + 150.0), unreal.Vector(at.x, at.y, at.z - 300.0))
    z = floor.z if floor is not None else at.z - 100.0
    remove_stale(build, 'Abel_Soul')
    abel = build.place(cls, (at.x, at.y, z), turn.yaw, label='Abel_Soul', folder=FOLDER, tags=(ABEL_TAG,))
    abel.set_editor_property('standing', True)
    abel.set_editor_property('soul_look', True)
    abel.set_editor_property('shown_when', texts().condition(dict(with_flags=[AWAKE_FLAG])))
    talk = abel.get_editor_property('speaker_point')
    spec = texts().TOPICS.get(ABEL_TAG, {})
    default = story.lines(build, spec.get('default') or ABEL_LINES)
    if default is not None:
        talk.set_editor_property('line_set', default)
    if spec:
        talk.set_editor_property('topics', story.texts_topics(build, ABEL_TAG))
    build.log(f'Abel\'s soul by the Reliquary at ({at.x:.0f}, {at.y:.0f}, {z:.0f}), standing, once {AWAKE_FLAG}')


def place_delia(build, chapel):
    """Delia just inside the chapel's front door, facing in: Main 7's turn-in. Returns Hob's spot over the door."""
    cls = story.actor_class('DeliaRansom')
    head = story.socket(chapel, 'Crepe') if chapel is not None else None
    if cls is None or head is None:
        build.warn('no DeliaRansom class or no chapel Crepe socket (the front door\'s head): Delia isn\'t at the chapel '
                   'door (Main 7\'s turn-in has no one to take it)')
        return None
    out = head.rotation.rotator()
    facing = out.get_forward_vector()
    at = head.translation - facing * DELIA_IN
    floor = story.onto_host(chapel, unreal.Vector(at.x, at.y, at.z), unreal.Vector(at.x, at.y, at.z - DOOR_HEAD_UP - 200.0))
    z = (floor.z if floor is not None else head.translation.z - DOOR_HEAD_UP) + DELIA_HALF_HEIGHT
    remove_stale(build, 'Delia_ChapelDoor')
    # Facing in, down the nave to the Reliquary, where her son is.
    delia = build.place(cls, (at.x, at.y, z), out.yaw + 180.0, label='Delia_ChapelDoor', folder=FOLDER, tags=(DELIA_CHAPEL_TAG,))
    delia.set_editor_property('shown_when', texts().condition(dict(during=MAIN7, from_step=RELIQUARY_STEP)))
    spec = texts().TOPICS.get(DELIA_CHAPEL_TAG, {})
    try:
        talk = delia.get_editor_property('speaker_point')
        talk.set_editor_property('speaker_name', unreal.Text(spec.get('name', 'Delia Ransom')))
        talk.set_editor_property('reach', DELIA_REACH)
        default = story.lines(build, spec['default']) if spec.get('default') else None
        if default is not None:
            talk.set_editor_property('line_set', default)
        talk.set_editor_property('topics', story.texts_topics(build, DELIA_CHAPEL_TAG) if spec else [])
    except Exception as error:
        build.warn(f'DeliaRansom\'s speaker point couldn\'t be set ({error}): her turn-in words come from the mission alone')
    build.log(f'Delia at the chapel door ({at.x:.0f}, {at.y:.0f}), during {MAIN7} from step {RELIQUARY_STEP}')
    top = head.translation + unreal.Vector(0.0, 0.0, 15.0)
    return top.x, top.y, top.z, out.yaw


def place(build):
    """Everything above (build_area_story.place calls this after the chapel, Main 4 and the deck, before Hob). Returns Hob's spots."""
    spots = {'chapelDoor': None, 'roadCoal': None}
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return spots
    rng = random.Random(SEED)
    place_fields(build, rng)
    spots['roadCoal'] = place_lumps(build, rng)
    place_crew(build)
    place_reliquary(build)
    chapel = story.placed(build, 'Chapel')
    place_abel(build, chapel)
    spots['chapelDoor'] = place_delia(build, chapel)
    return spots
