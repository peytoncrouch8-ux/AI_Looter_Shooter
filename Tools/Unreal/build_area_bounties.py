"""The bounty board and its arenas on Ransom's Rest (Docs/Work/2026-10-22-Next/BOUNTY.md), for build_area.py's gameplay
pass: the user's pick (2026-10-22) of a board offering a rotating hard fight, paid at the board, so the Rest has
replayable fights before new regions. Everything goes in the area's Gameplay/Bounties folder; only the level whose layout
has Ellis's grave gets any of it. A class not built yet is warned of and left out.

- The board (ABountyBoard, BountyBoard_RansomsRest, board id RansomsRest): at the depot (the user's playtest, 2026-10-27,
  note 12: "by the train station"), BOARD_AT where the yard road meets the platform's west end, off the road's south
  edge, its face (local +X) turned to BOARD_YAW: toward the Depot roost (where fast travel and deaths arrive, the player
  waking facing the board: the roost's yaw is 180, due at it, since the playtest of 2026-10-27 found it 45 degrees off,
  patches/polish_1.json) and down the yard road (where walkers from Main Street arrive). The build traces from its face to both (check_sights) and warns when something hides it. Its own model
  (SM_NoticeBoard_Bounty, BOUNTIES on its header; SM_NoticeBoard_Town until imported) is the C++ class's. Open after
  Main 3, Dangerous bounties after Main 5, Deadly after Main 6.
- The arenas (ABountyArena, BountyArena_<id>, ARENAS): each a bounty fight's place, its ground (a radius or a layout
  obstacle's, zone's or polygon's corners), three spot rings round its middle (the first push ahead, the flank, the
  mark's push: the mark's own spot first, then its guards'), when it's open, the kinds it hosts, and the spawners it waits
  on (those of WAIT_FOR found in the level). Spots off its level of ground or inside a layout obstacle are warned of (the
  fight skips them in play). Nothing drops at a fight: the reward is handed over at the board (note 40).

Hook in build_area.py's gameplay(), after the placeholders (the camps, patrols and fights it waits on stand first):
    importlib.reload(importlib.import_module('build_area_bounties')).place(self)
On its own, in the open editor (clears only Gameplay/Bounties, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_bounties.py RansomsRest"
Check in play: Looter.Bounty.Arenas (draws them), Looter.Bounty.Make <slot> <Kind> <Tier> <Arena>, Looter.Bounty.Take.
"""
import importlib
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Bounties'
BOARD_LABEL = 'BountyBoard_RansomsRest'
BOARD_ID = 'RansomsRest'
# At the depot (layout cm, X north, Y east): 3.8 m south of the yard road's line where it meets the platform's west end
# ramp (the road's centre at about (250, 10250)), 3.5 m short of the rail bed's end, 8.8 m from the Depot roost at
# (750, 10250). Clear of the station house (its front wall on X 465 from Y 10450), the platform and its ramp (X 65-465
# from Y 10660), the coal stage (-470, 10760) and the buffer stop (-100, 11000).
BOARD_AT = (-130.0, 10250.0)
# Its face's way (degrees; a notice board's face is its local +X): between the Depot roost (0 degrees from here) and the
# yard road's way in from Main Street (about -76 degrees), so both see its face within about 40 degrees of square on.
BOARD_YAW = -35.0
# What must see its face (layout cm): the Depot roost (found by id in gameplay.roosts; this where it isn't) and the yard
# road 12 m back toward town; the sight lines run at eye height (cm over the ground).
DEPOT_ROOST = 'Depot'
DEPOT_ROOST_AT = (750.0, 10250.0)
ROAD_IN_AT = (180.0, 9000.0)
BOARD_EYE = 150.0

# Spawners an arena waits on when the level has them: only the story's own fights on its ground (no two fights stacked).
# Never a camp, patrol or roaming group: they're Engaged whenever the player is near (their creatures stand about), so
# waiting on one held the Scatter's bounty until the orchard's camp was cleared (PIE 2026-10-22).
WAIT_FOR = {
    'OldQuarry': ['QuarryCrew', 'AbeHaskett', 'RaisingSteam'],
    # Before Dead Lift is finished, so while it's played: its crew rises on this floor (Dead Lift's own fights first).
    'QuarryMouth': ['QuarryCrew', 'AbeHaskett', 'RaisingSteam'],
    'WhitlockYard': ['WhitlockHands'],
    # Main 2's nest (on during Main 2 only; the board opens after Main 3, so only a safeguard).
    'RansomsPoint': ['BluffNest'],
}

# Each arena (layout cm, X north, Y east): its id (never renamed: saved postings name it), its name on a card, the kinds
# it hosts, when it's open (after these missions, and before those), its middle and ground, its rings (radius cm, how
# many, turn as a share of a step) and the mark's own spot (offset from the middle). Its levels are its zone's (layout
# gameplay.levels; never the player's). Before Main 5 the board draws from the Scatter (Unpaid), Ransom's Point (spiders)
# and the quarry's mouth (spiders and Venom Spitters), Whitlock's yard after Side 2 and the Webwood after its set piece:
# the user's playtest (2026-10-27, note 22) saw only the Unpaid at the Scatter. The kinds are those the story already
# puts there (Docs/Areas/RansomsRest.md, "Fights and set pieces"): the bluff's nest (Main 2), the quarry track's spiders
# and Spitter (after Main 3).
ARENAS = [
    dict(id='Scatter', name='the Scatter', kinds=['Unpaid'], after=['Main3'],
         where='round the Scatter\'s boulders between Coffin Rock and the barn yard (the boulders their cover)',
         center=(-5160.0, 840.0), ground={'radius': 1300.0, 'maxRise': 300.0, 'maxStep': 300.0},
         push=(950.0, 4, 0.0), flank=(1100.0, 3, 0.5), guards=(450.0, 2, 0.25), mark=(800.0, 0.0)),
    # The bluff top, between the nest ring (north) and the two 3 m rocks (south), east of the lookout: about 14 m across of
    # open ground, the ring and the rocks its cover (every spot 2.2 m or more from the ring and 1.7 m or more from the
    # rocks, checked against the layout). Its zone's levels, 1-2: the Rest's gentlest bounty, a spider one for the board's
    # first days. Kept back from the drop (the user's playtest, 2026-10-27: a player fell about 14 m off the bluff mid-fight
    # at (-8682, -7016), 2 m past the east lip): the hunting ground is no longer the whole plateau but a polygon at least
    # 5.9 m inside its lip (its margin included, 5 m), and every spot 6.6 m or more from the lip (checked with the plateau's
    # polygon in layout.json, zones.ransomsPoint). A target who leaves a hunting ground ends the chase at its edge, so the
    # spiders never follow a player to the lip; the bluff path's top (-8000, -7300) is outside it, the fight starts as
    # they walk in.
    dict(id='RansomsPoint', name='Ransom\'s Point', kinds=['Spider'], after=['Main3'],
         where='the bluff top round Main 2\'s nest ring, east of the lookout (the ring and the bluff rocks their cover), '
               'well back from the lip',
         center=(-8100.0, -8500.0),
         ground={'polygon': [[-6800, -10100], [-6800, -7750], [-8300, -7750], [-8950, -8350], [-8950, -10100]],
                 'margin': 100.0, 'maxRise': 300.0, 'maxStep': 300.0},
         push=(500.0, 4, 0.5), flank=(800.0, 3, 0.0), guards=(300.0, 2, 0.25), mark=(300.0, 300.0)),
    # The quarry floor's south half where the track comes in, before Dead Lift: the quarry track's spiders and Spitter are
    # the story's here from Main 3 (the dead miners only rise in Dead Lift). Its zone's levels, 5-6: ground to come back to,
    # as the track's patrol is. From Dead Lift on the floor is the Old Quarry's (its miners join the kinds).
    dict(id='QuarryMouth', name='the quarry\'s mouth', kinds=['Spider', 'Spitter'], after=['Main3'], before=['DeadLift'],
         where='the Old Quarry\'s floor where the quarry track comes in, round the fallen derrick (before Dead Lift)',
         center=(9725.0, 1400.0),
         ground={'polygon': [[9000, 250], [9000, 3850], [10450, 3850], [10450, 250]], 'maxRise': 400.0, 'maxStep': 400.0},
         push=(650.0, 4, 0.5), flank=(650.0, 3, 0.0), guards=(300.0, 2, 0.25), mark=(300.0, 1350.0)),
    dict(id='WhitlockYard', name='Whitlock\'s barn yard', kinds=['Unpaid'], after=['Side2'],
         where='inside the Whitlock barn yard\'s fence, round the barn and the windmill (Amos\'s hands put to rest first)',
         center=(-6850.0, 3500.0), ground={'obstacle': 'barnYardFence', 'margin': 200.0, 'maxRise': 300.0, 'maxStep': 300.0},
         push=(800.0, 4, 0.0), flank=(1050.0, 3, 0.5), guards=(350.0, 2, 0.0), mark=(-600.0, 0.0)),
    dict(id='Webwood', name='the Webwood', kinds=['Spider', 'Spitter'], after=['Main4'], with_flags=['SetPiece.Webwood'],
         where='the webbed band of dead trees north of the Sink: they drop round the player (its set piece won first)',
         center=None, ground={'obstacle': 'webwood', 'margin': 300.0, 'maxRise': 400.0, 'maxStep': 400.0},
         around_player=True, drop=True, spring_on_ground=True),
    dict(id='OldQuarry', name='the Old Quarry', kinds=['Spider', 'Spitter', 'DeadMiner'], after=['DeadLift'],
         where='the quarry floor at Larkspur Ridge\'s foot (Dead Lift\'s fights done)',
         center=(9725.0, 2050.0),
         ground={'polygon': [[9000, 250], [9000, 3850], [10450, 3850], [10450, 250]], 'maxRise': 400.0, 'maxStep': 400.0},
         push=(900.0, 4, 0.0), flank=(1300.0, 3, 0.5), guards=(450.0, 2, 0.0), mark=(500.0, 900.0)),
    dict(id='GraniteMeadow', name='the Bench\'s granite meadow', kinds=['DeadMiner', 'Spider'], after=['DeadLift'],
         where='Larkspur Bench\'s open ground between its granite outcrops D and E (clear of the Dinner Pail camp, the '
               'tramway and the spoil ambush)',
         center=(20900.0, 700.0), ground={'radius': 1300.0, 'maxRise': 300.0, 'maxStep': 300.0},
         push=(950.0, 4, 0.0), flank=(1150.0, 3, 0.5), guards=(450.0, 2, 0.0), mark=(800.0, 300.0)),
]


def camps():
    return importlib.reload(importlib.import_module('build_area_camps'))


def ring(cx, cy, spec, turn_seed):
    """Spots on a ring round (cx, cy): spec (radius, how many, turn as a share of one step), turned by the arena's id."""
    radius, count, turn = spec
    step = 2.0 * math.pi / max(count, 1)
    start = turn_seed + turn * step
    return [(cx + math.cos(start + k * step) * radius, cy + math.sin(start + k * step) * radius) for k in range(count)]


def inside(polygon, x, y):
    """Whether (x, y) is inside a polygon of [x, y] corners (even-odd)."""
    hit = False
    for i in range(len(polygon)):
        x1, y1 = polygon[i]
        x2, y2 = polygon[(i + 1) % len(polygon)]
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / ((y2 - y1) or 1e-9):
            hit = not hit
    return hit


def in_obstacle(build, x, y):
    """The layout obstacle (rock, ruin, fence's yard is fine: only solid kinds) a spot stands in, or None."""
    for each in build.layout.get('obstacles', []):
        if each.get('kind') in ('fence', 'graves', 'trees', 'cairns', 'biers', 'wall') or 'polygon' not in each:
            continue
        if inside(each['polygon'], x, y):
            return each['id']
    return None


def spawner_ids():
    """Every encounter spawner's id in the level (to wait on the ones that stand)."""
    ids = set()
    cls = getattr(unreal, 'EncounterSpawner', None)
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if cls is not None and isinstance(actor, cls):
            ids.add(str(actor.get_editor_property('spawner_id')))
    return ids


def depot_roost_at(build):
    """The Depot roost's spot (layout cm) from gameplay.roosts, else DEPOT_ROOST_AT."""
    for roost in build.settings.get('roosts', []):
        if roost.get('id') == DEPOT_ROOST:
            return tuple(roost['at'])
    build.warn(f'no {DEPOT_ROOST} roost in the layout: the bounty board\'s sight line is checked to {DEPOT_ROOST_AT}')
    return DEPOT_ROOST_AT


def check_sights(build, board, x, y, z):
    """Warns when something stands between the board's face and where players arrive (BOARD_SIGHTS), at eye height."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    face = unreal.Vector(x, y, z + BOARD_EYE)
    hidden = []
    for name, (tx, ty) in (('the Depot roost', depot_roost_at(build)), ('the yard road in', ROAD_IN_AT)):
        tz = story.ground_at(tx, ty, z, warn=False)
        hit = unreal.SystemLibrary.line_trace_single(world, face, unreal.Vector(tx, ty, tz + BOARD_EYE),
                                                     unreal.TraceTypeQuery.ECC_VISIBILITY, False, [board],
                                                     unreal.DrawDebugTrace.NONE, True)
        if hit is not None:
            hidden.append(f'{name} ({tx:.0f}, {ty:.0f})')
    if hidden:
        build.warn(f'something stands between the bounty board and {", ".join(hidden)}: move BOARD_AT')


def place_board(build):
    """The board at the depot (BOARD_AT, BOARD_YAW) on the ground, its sight lines to the arrivals checked."""
    cls = story.actor_class('BountyBoard')
    if cls is None:
        build.warn('no BountyBoard class (build the game module first): no bounty board')
        return None
    x, y = BOARD_AT
    z = story.ground_at(x, y)
    made = build.place(cls, (x, y, z), BOARD_YAW, label=BOARD_LABEL, folder=FOLDER, tags=('BountyBoard',))
    made.set_editor_property('board_id', unreal.Name(BOARD_ID))
    made.set_editor_property('open_when', story.condition(after=['Main3']))
    made.set_editor_property('dangerous_when', story.condition(after=['Main5']))
    made.set_editor_property('deadly_when', story.condition(after=['Main6']))
    check_sights(build, made, x, y, z)
    build.log(f'the bounty board at the depot at ({x:.0f}, {y:.0f}, {z:.0f}), yaw {BOARD_YAW:.0f}')
    return made


def centroid_of(build, ground):
    corners = camps().polygon_of(build, ground)
    return story.centroid(corners) if corners else None


def place_arena(build, entry, standing):
    cls = story.actor_class('BountyArena')
    if cls is None:
        build.warn('no BountyArena class (build the game module first): no bounty arenas')
        return False
    helpers = camps()
    label = f'BountyArena_{entry["id"]}'
    center = entry['center'] or centroid_of(build, entry['ground'])
    if center is None:
        build.warn(f'{label}: no middle and no ground polygon: left out')
        return False
    x, y = center
    z = helpers.ground_z(build, x, y)
    if z is None:
        build.warn(f'no terrain under {label}\'s middle ({x:.0f}, {y:.0f}): left out')
        return False
    made = build.place(cls, (x, y, z + 50.0), 0.0, label=label, folder=FOLDER, tags=('BountyArena',))
    made.set_editor_property('arena_id', unreal.Name(entry['id']))
    made.set_editor_property('display_name', unreal.Text(entry['name']))
    made.set_editor_property('kinds', [unreal.Name(k) for k in entry['kinds']])
    when = story.condition(after=entry.get('after', ()), before=entry.get('before', ()))
    if entry.get('with_flags'):
        when.set_editor_property('with_flags', [unreal.Name(f) for f in entry['with_flags']])
    made.set_editor_property('open_when', when)
    helpers.set_ground(build, made, entry['ground'], z)
    made.set_editor_property('spring_on_ground', bool(entry.get('spring_on_ground', False)))
    made.set_editor_property('around_player', bool(entry.get('around_player', False)))
    made.set_editor_property('drop_from_above', bool(entry.get('drop', False)))
    waits = [w for w in WAIT_FOR.get(entry['id'], []) if w in standing]
    made.set_editor_property('wait_for_spawners', [unreal.Name(w) for w in waits])

    step = float(entry['ground'].get('maxStep', helpers.DEFAULT_STEP))
    spots = []
    if not entry.get('around_player'):
        # Turned by its id (the same every build: Python's own string hash changes between runs).
        turn = math.radians(sum(ord(c) for c in entry['id']) % 360)
        push = ring(x, y, entry['push'], turn)
        flank = ring(x, y, entry['flank'], turn)
        mx, my = x + entry['mark'][0], y + entry['mark'][1]
        mark = [(mx, my)] + ring(mx, my, entry['guards'], turn)
        made.set_editor_property('push_spots', helpers.relative(build, push, x, y, z))
        made.set_editor_property('flank_spots', helpers.relative(build, flank, x, y, z))
        made.set_editor_property('mark_spots', helpers.relative(build, mark, x, y, z))
        spots = push + flank + mark
        blocked = [f'({sx:.0f}, {sy:.0f}) in {in_obstacle(build, sx, sy)}' for sx, sy in spots if in_obstacle(build, sx, sy)]
        if blocked:
            build.warn(f'{label}: spots inside obstacles (the fight skips them): {", ".join(blocked)}')
        helpers.check_levels(build, label, (x, y, z), spots, step)

    build.log(f'{label} ({entry["where"]}): {"/".join(entry["kinds"])}, after {", ".join(entry.get("after", ())) or "the start"}'
              + (f', before {", ".join(entry["before"])}' if entry.get('before') else '')
              + (f' with {", ".join(entry["with_flags"])}' if entry.get('with_flags') else '')
              + (f', waits on {", ".join(waits)}' if waits else '') + f'; {len(spots)} spots')
    return True


def place(build):
    """The board and every arena (ARENAS). Ransom's Rest only."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    place_board(build)
    standing = spawner_ids()
    placed = sum(1 for entry in ARENAS if place_arena(build, entry, standing))
    build.log(f'bounty arenas: {placed} of {len(ARENAS)} placed')


def run(name):
    """Places Gameplay/Bounties again on its own, saves."""
    import build_area
    build = importlib.reload(build_area).AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the bounty board and its arenas placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_bounties.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
