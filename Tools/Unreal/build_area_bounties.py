"""The bounty board and its arenas on Ransom's Rest (Docs/Work/2026-10-22-Next/BOUNTY.md), for build_area.py's gameplay
pass: the user's pick (2026-10-22) of a board offering a rotating hard fight with a strongbox, so the Rest has replayable
fights before new regions. Everything goes in the area's Gameplay/Bounties folder; only the level whose layout has
Ellis's grave gets any of it. A class not built yet is warned of and left out.

- The board (ABountyBoard, BountyBoard_RansomsRest, board id RansomsRest): BOARD_SIDE cm beside the Rim Rangers' board
  (NoticeBoard_Rangers, the sheriff's office's on Main Street) along its face, at its height and yaw; the other side when
  a wall is in the way. Its own model (SM_NoticeBoard_Bounty, BOUNTIES on its header; SM_NoticeBoard_Town until imported) is the C++ class's. Open after Main 3,
  Dangerous bounties after Main 5, Deadly after Main 6.
- The arenas (ABountyArena, BountyArena_<id>, ARENAS): each a bounty fight's place, its ground (a radius or a layout
  obstacle's or polygon's corners), three spot rings round its middle (the first push ahead, the flank, the mark's push:
  the mark's own spot first, then its guards'), its strongbox's spot (the first candidate on clear ground: not inside a
  layout obstacle, within its step of the middle's ground), when it's open, the kinds it hosts, and the spawners it waits
  on (those of WAIT_FOR found in the level). Spots off its level of ground are warned of (the fight skips them in play).

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
RANGERS_BOARD = 'NoticeBoard_Rangers'
# Beside the Rangers' board along its face (cm, its local Y), and how high a wall check looks (cm over its foot).
BOARD_SIDE = 280.0
BOARD_WALL_CHECK = 120.0
# The Rangers' board's half width and a little (cm): the wall check starts past it.
BOARD_CLEAR = 110.0

# Spawners an arena waits on when the level has them: only the story's own fights on its ground (no two fights stacked).
# Never a camp, patrol or roaming group: they're Engaged whenever the player is near (their creatures stand about), so
# waiting on one held the Scatter's bounty until the orchard's camp was cleared (PIE 2026-10-22).
WAIT_FOR = {
    'OldQuarry': ['QuarryCrew', 'AbeHaskett', 'RaisingSteam'],
    'WhitlockYard': ['WhitlockHands'],
}

# Each arena (layout cm, X north, Y east): its id (never renamed: saved postings name it), its name on a card, the kinds
# it hosts, when it's open, its middle and ground, its rings (radius cm, how many, turn as a share of a step), the mark's
# own spot (offset from the middle) and the strongbox's candidates (offsets, the first clear one wins).
ARENAS = [
    dict(id='Scatter', name='the Scatter', kinds=['Unpaid'], after=['Main3'],
         where='round the Scatter\'s boulders between Coffin Rock and the barn yard (the boulders their cover)',
         center=(-5160.0, 840.0), ground={'radius': 1300.0, 'maxRise': 300.0, 'maxStep': 300.0},
         push=(950.0, 4, 0.0), flank=(1100.0, 3, 0.5), guards=(450.0, 2, 0.25), mark=(800.0, 0.0),
         reward=[(-850.0, -400.0), (-900.0, 300.0), (900.0, 500.0)]),
    dict(id='WhitlockYard', name='Whitlock\'s barn yard', kinds=['Unpaid'], after=['Side2'],
         where='inside the Whitlock barn yard\'s fence, round the barn and the windmill (Amos\'s hands put to rest first)',
         center=(-6850.0, 3500.0), ground={'obstacle': 'barnYardFence', 'margin': 200.0, 'maxRise': 300.0, 'maxStep': 300.0},
         push=(800.0, 4, 0.0), flank=(1050.0, 3, 0.5), guards=(350.0, 2, 0.0), mark=(-600.0, 0.0),
         reward=[(500.0, 900.0), (600.0, -800.0), (0.0, 1000.0)]),
    dict(id='Webwood', name='the Webwood', kinds=['Spider', 'Spitter'], after=['Main4'], with_flags=['SetPiece.Webwood'],
         where='the webbed band of dead trees north of the Sink: they drop round the player (its set piece won first)',
         center=None, ground={'obstacle': 'webwood', 'margin': 300.0, 'maxRise': 400.0, 'maxStep': 400.0},
         around_player=True, drop=True, spring_on_ground=True,
         reward_at=(7034.0, 7157.0)),
    dict(id='OldQuarry', name='the Old Quarry', kinds=['Spider', 'Spitter', 'DeadMiner'], after=['DeadLift'],
         where='the quarry floor at Larkspur Ridge\'s foot (Dead Lift\'s fights done)',
         center=(9725.0, 2050.0),
         ground={'polygon': [[9000, 250], [9000, 3850], [10450, 3850], [10450, 250]], 'maxRise': 400.0, 'maxStep': 400.0},
         push=(900.0, 4, 0.0), flank=(1300.0, 3, 0.5), guards=(450.0, 2, 0.0), mark=(500.0, 900.0),
         reward=[(-400.0, -600.0), (-500.0, 300.0), (300.0, -900.0)]),
    dict(id='GraniteMeadow', name='the Bench\'s granite meadow', kinds=['DeadMiner', 'Spider'], after=['DeadLift'],
         where='Larkspur Bench\'s open ground between its granite outcrops D and E (clear of the Dinner Pail camp, the '
               'tramway and the spoil ambush)',
         center=(20900.0, 700.0), ground={'radius': 1300.0, 'maxRise': 300.0, 'maxStep': 300.0},
         push=(950.0, 4, 0.0), flank=(1150.0, 3, 0.5), guards=(450.0, 2, 0.0), mark=(800.0, 300.0),
         reward=[(-700.0, -500.0), (-700.0, 500.0), (600.0, -700.0)]),
]


def camps():
    return importlib.reload(importlib.import_module('build_area_camps'))


def placeholders():
    return importlib.reload(importlib.import_module('build_area_placeholders'))


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


def place_board(build):
    """The board beside the Rangers' board, the side without a wall; or at the layout's board spot with the offset."""
    cls = story.actor_class('BountyBoard')
    if cls is None:
        build.warn('no BountyBoard class (build the game module first): no bounty board')
        return None
    rangers = placeholders().labelled(RANGERS_BOARD)
    if rangers is not None:
        at = rangers.get_actor_location()
        yaw = rangers.get_actor_rotation().yaw
    else:
        boards = [b for b in build.settings.get('boards', []) if b.get('label') == RANGERS_BOARD]
        if not boards:
            build.warn(f'no {RANGERS_BOARD} in the level or the layout: no bounty board')
            return None
        x, y = boards[0]['at']
        z = story.ground_at(x, y)
        at = unreal.Vector(x, y, z)
        yaw = float(boards[0].get('yaw', 0.0))
        build.warn(f'no {RANGERS_BOARD} actor: the bounty board stands by the layout\'s spot')
    side = unreal.Vector(-math.sin(math.radians(yaw)), math.cos(math.radians(yaw)), 0.0)
    world = unreal.EditorLevelLibrary.get_editor_world()
    chosen = None
    for sign in (1.0, -1.0):
        spot = unreal.Vector(at.x + side.x * BOARD_SIDE * sign, at.y + side.y * BOARD_SIDE * sign, at.z)
        # From just past the Rangers' board's own edge (its model is the dressing's, not the actor's), out to the spot.
        start = unreal.Vector(at.x + side.x * BOARD_CLEAR * sign, at.y + side.y * BOARD_CLEAR * sign, at.z + BOARD_WALL_CHECK)
        end = unreal.Vector(spot.x, spot.y, spot.z + BOARD_WALL_CHECK)
        hit = unreal.SystemLibrary.line_trace_single(world, start, end, unreal.TraceTypeQuery.ECC_VISIBILITY, False,
                                                     [rangers] if rangers else [], unreal.DrawDebugTrace.NONE, True)
        if hit is None:
            chosen = spot
            break
    if chosen is None:
        build.warn('a wall both sides of the Rangers\' board: the bounty board stands on its right side anyway')
        chosen = unreal.Vector(at.x + side.x * BOARD_SIDE, at.y + side.y * BOARD_SIDE, at.z)
    made = build.place(cls, (chosen.x, chosen.y, chosen.z), yaw, label=BOARD_LABEL, folder=FOLDER, tags=('BountyBoard',))
    made.set_editor_property('board_id', unreal.Name(BOARD_ID))
    made.set_editor_property('open_when', story.condition(after=['Main3']))
    made.set_editor_property('dangerous_when', story.condition(after=['Main5']))
    made.set_editor_property('deadly_when', story.condition(after=['Main6']))
    build.log(f'the bounty board beside {RANGERS_BOARD} at ({chosen.x:.0f}, {chosen.y:.0f}, {chosen.z:.0f}), yaw {yaw:.0f}')
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
    when = story.condition(after=entry.get('after', ()))
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

    # Its strongbox's spot: the first candidate on clear ground at its level.
    candidates = [entry['reward_at']] if entry.get('reward_at') else [(x + dx, y + dy) for dx, dy in entry['reward']]
    reward = None
    for rx, ry in candidates:
        rz = helpers.ground_z(build, rx, ry, z)
        if rz is not None and abs(rz - z) <= step and not in_obstacle(build, rx, ry):
            reward = (rx, ry, rz)
            break
    if reward is None:
        build.warn(f'{label}: no clear spot for its strongbox: it drops at the middle')
        reward = (x, y, z)
    made.set_editor_property('reward_spot', unreal.Vector(reward[0] - x, reward[1] - y, reward[2] - (z + 50.0)))
    made.set_editor_property('reward_yaw', math.degrees(math.atan2(y - reward[1], x - reward[0])))
    build.log(f'{label} ({entry["where"]}): {"/".join(entry["kinds"])}, after {", ".join(entry.get("after", ())) or "the start"}'
              + (f' with {", ".join(entry["with_flags"])}' if entry.get('with_flags') else '')
              + (f', waits on {", ".join(waits)}' if waits else '') + f'; {len(spots)} spots, strongbox at '
              f'({reward[0]:.0f}, {reward[1]:.0f})')
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
