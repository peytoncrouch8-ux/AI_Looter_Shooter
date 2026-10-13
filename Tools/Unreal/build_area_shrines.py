"""The vigil stones (World/BossShrine: ABossShrine), for build_area.py's gameplay pass (after build_area_story.py has placed the
bosses), or on their own in the open level:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_shrines.py RansomsRest"
which replaces only the stones a build placed before, and saves the level.

The user's call (2026-10-12): "a pedestal with a menu that spawns next to the boss arenas after you kill the boss the first
time so you can respawn the boss". One stone beside each boss arena, where the player ends up after the fight and off its
ground, derived from the arena as it's built (nothing hand-placed):
  - The Gravemother (Side 3): at the Sink's ramp head, on the rim beside the warning fence's gap, where the climb out of the
    pit ends (the layout's theSink ramp, sinkRamp, its first point); off her ground (more than her lair's reach from the den's
    mouth, or the rim's 12 m over its floor), clear of the ramp, the road and the fence. Her lair (Encounter_Gravemother)
    brings her back.
  - Abel (Main 6): outside the Keeper's Gate (the layout's gateSouth and gateNorth), on the side away from the burial deck,
    beside the road where the player walks out after the fight and the scene; far enough from his spot that calling him
    back doesn't start his fight (his deck radius, 14 m), clear of the road, the gate's rocks and the cairns. Abel (the
    placed AAbelKeeper labelled Abel) is called back at his spot.
  - The Foreman (the Providence, Docs/Areas/RansomsRest_Expansion.md): on the surface by the mine's portal, at design metres
    (80, 224) (layout.json gameplay.mountain.foremanStone when it gives one), facing the portal (the mine's origin,
    gameplay.mountain.mine.origin); his fight is in the stope far under the ridge, so the stone is never on his ground. His lair
    (the spawner whose SpawnerId is Foreman) brings him back: found by its label (Encounter_Foreman) when the build gave it
    one, else by the stone itself at run time (ABossShrine::Lair's "none"). While the Diggings aren't built (no walkable
    ground at the stone's place) he gets no stone (a warning).
Each spot is chosen from candidates round its anchor: inside the playable boundary, on walkable ground (tagged Ground) level
enough under the stone's corners, clear of the layout's roads, ramps, obstacles, placements, rocks and pits; the nearest to
its anchor wins, facing it (the player coming up the ramp or out through the gate sees its front). Every choice is logged;
a boss whose arena or actor isn't in the level gets no stone (a warning).

Hook in build_area.py's gameplay() (after self.story.place(self), before the camps):
    importlib.reload(importlib.import_module('build_area_shrines')).place(self)
"""
import importlib
import math
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
CLASSES = '/Script/AI_Looter_Shooter.'
SHRINE_TAG = 'VigilStone'

# The stone's footprint (cm, from Art/Models/Props/VigilStone.py's lower step: 1.2 x 0.8 m): half its width and depth.
HALF_WIDTH = 60.0
HALF_DEPTH = 40.0
# The most its corners' ground may differ (cm): more, and it would hang off a slope.
MOST_SPREAD = 30.0

# Clearances (cm): from a road's edge, a ramp's walkway, an obstacle's line or outline, a placed model, a rock, a pit's rim,
# the playable boundary.
ROAD_CLEAR = 140.0
RAMP_CLEAR = 160.0
OBSTACLE_CLEAR = 170.0
PLACEMENT_CLEAR = 420.0
ROCK_CLEAR = 160.0
PIT_CLEAR = 300.0
BOUNDARY_CLEAR = 300.0

# The Gravemother's lair (build_area_den.py): her ground reaches GIVE_UP from the den's mouth, MAX_RISE above or below it.
LAIR_LABEL = 'Encounter_Gravemother'
GIVE_UP = 3600.0
MAX_RISE = 300.0
# Abel (build_area_deck.py, AAbelKeeper::DeckRadius): a player this near his spot starts his fight once he's back.
ABEL_LABEL = 'Abel'
DECK_RADIUS = 1400.0
# The Foreman (the Providence's stope, ForemanFight.h's Foreman::LairId): his stone stands by the mine's portal at design
# metres (80, 224) (x east, y north), the portal at (74, 230) facing south. Level cm is X north = y * 100, Y east = x * 100.
FOREMAN_LABEL = 'Encounter_Foreman'
FOREMAN_STONE = (22400.0, 8000.0)
FOREMAN_PORTAL = (23000.0, 7400.0)

# Each boss's stone: its key (FBossRecallRecord's; Abel's is also his campaign boss id), its class, how it comes back, the
# menu's words, its answer from the arena after the bell, and the story that counts it beaten (a save from before the
# stones) and lets it be called.
SHRINES = (
    dict(key='Gravemother', boss_class='GravemotherCreature', kind='LAIR', name='The Gravemother',
         loot="Two guns from a Legendary monster's table, a chance of a legendary, and ammo",
         answer='Boss.Gravemother.Roar', beaten_after=['Side3'], callable_after=[]),
    dict(key='Abel', boss_class='AbelKeeper', kind='PLACED', name='Abel Ransom, the Keeper',
         loot="Three guns from a boss's table, a fair chance of a legendary, and ammo",
         answer='Boss.Abel.Wail', beaten_after=['Main6'], callable_after=['Main6']),
    dict(key='Foreman', boss_class='ForemanCreature', kind='LAIR', name='The Foreman',
         loot="Two guns from a Legendary monster's table, a chance of a legendary, and ammo",
         answer='Boss.Foreman.Intro', beaten_after=['BackPay'], callable_after=[]),
)

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


# ---------------------------------------------------------------------------
# Plain geometry (level cm, X north, Y east, as the layout's points are)
# ---------------------------------------------------------------------------

def segment_distance(p, a, b):
    ax, ay = b[0] - a[0], b[1] - a[1]
    length2 = ax * ax + ay * ay
    t = 0.0 if length2 < 1e-6 else max(0.0, min(1.0, ((p[0] - a[0]) * ax + (p[1] - a[1]) * ay) / length2))
    return math.hypot(p[0] - (a[0] + ax * t), p[1] - (a[1] + ay * t))


def path_distance(p, path):
    if len(path) == 1:
        return math.hypot(p[0] - path[0][0], p[1] - path[0][1])
    return min(segment_distance(p, a, b) for a, b in zip(path, path[1:]))


def inside(p, polygon):
    """Whether p is inside the polygon (even-odd)."""
    result = False
    for (x0, y0), (x1, y1) in zip(polygon, polygon[1:] + polygon[:1]):
        if (y0 > p[1]) != (y1 > p[1]) and p[0] < x0 + (p[1] - y0) * (x1 - x0) / (y1 - y0):
            result = not result
    return result


def outline_distance(p, polygon):
    return path_distance(p, list(polygon) + [polygon[0]])


def flat(vector):
    length = math.hypot(vector[0], vector[1])
    return (vector[0] / length, vector[1] / length) if length > 1e-6 else (1.0, 0.0)


def yaw_toward(frm, to):
    return math.degrees(math.atan2(to[1] - frm[1], to[0] - frm[0]))


# ---------------------------------------------------------------------------
# The level
# ---------------------------------------------------------------------------

def ground(x, y):
    """The walkable terrain (tagged Ground) under (x, y), or None: a fence rail or a rock never counts."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, 30000.0), unreal.Vector(x, y, -30000.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    for hit in hits or []:
        t = hit.to_tuple()
        if t[9] is not None and unreal.Name('Ground') in t[9].tags:
            return t[4].z
    return None


def footing(x, y, yaw):
    """The ground under the stone's middle and corners turned by yaw: (the lowest, the spread), or None off the ground."""
    fx, fy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    rx, ry = -fy, fx
    spots = [(x, y)] + [(x + fx * d + rx * w, y + fy * d + ry * w) for d in (-HALF_DEPTH, HALF_DEPTH) for w in (-HALF_WIDTH, HALF_WIDTH)]
    zs = [ground(sx, sy) for sx, sy in spots]
    if any(z is None for z in zs):
        return None
    return min(zs), max(zs) - min(zs)


def labelled(build, label):
    """The actor this build placed with this label, or None."""
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() == label and unreal.Name(build.tag) in actor.tags:
            return actor
    return None


def blockers(build):
    """What a stone must keep clear of, from the layout: (kind, data, clearance) for each road, ramp, obstacle, placement, rock
    and pit."""
    found = []
    for road in build.source.get('roads', []):
        if road.get('path'):
            found.append(('path', road['path'], road.get('width', 300.0) * 0.5 + ROAD_CLEAR))
    for feature in build.source.get('features', []):
        ramp = feature.get('ramp')
        if ramp and ramp.get('path'):
            found.append(('path', ramp['path'], ramp.get('width', 400.0) * 0.5 + RAMP_CLEAR))
        if feature.get('center') and feature.get('radius') and feature.get('type') in ('knob', 'rock', 'hill') and feature.get('rock'):
            found.append(('point', [feature['center']], feature['radius'] + ROCK_CLEAR))
        if feature.get('polygon') and feature.get('type') in ('pit', 'mesa'):
            found.append(('area', feature['polygon'], PIT_CLEAR))
    for obstacle in build.source.get('obstacles', []):
        if obstacle.get('path'):
            found.append(('path', obstacle['path'], OBSTACLE_CLEAR))
        elif obstacle.get('polygon'):
            found.append(('area', obstacle['polygon'], OBSTACLE_CLEAR))
        elif obstacle.get('center'):
            found.append(('point', [obstacle['center']], obstacle.get('radius', 0.0) + OBSTACLE_CLEAR))
    for spot in build.layout.get('placements', {}).values():
        found.append(('point', [spot['location'][:2]], PLACEMENT_CLEAR))
    for prop in build.props:
        if prop.get('location'):
            found.append(('point', [prop['location'][:2]], PLACEMENT_CLEAR))
    return found


def blocked_by(p, found):
    """The first blocker p is too near (its kind), or None."""
    for kind, data, clearance in found:
        if kind == 'area':
            if inside(p, data) or outline_distance(p, data) < clearance:
                return kind
        elif path_distance(p, data) < clearance:
            return kind
    return None


def in_bounds(build, p):
    corners = build.layout.get('boundary', {}).get('corners', [])
    if len(corners) < 3:
        return True
    pts = [c[:2] for c in corners]
    return inside(p, pts) and outline_distance(p, pts) >= BOUNDARY_CLEAR


def choose(build, name, candidates, anchor, off_fight, score):
    """The best candidate spot (x, y, z, yaw) for name's stone, facing anchor: in bounds, on level ground, clear of the layout's
    blockers and off the boss's fight (off_fight(x, y, z)), lowest score first. None, with a warning, when none will do."""
    found = blockers(build)
    reasons = {}
    best = None
    for x, y in candidates:
        why = None
        if not in_bounds(build, (x, y)):
            why = 'outside the playable area'
        else:
            why = blocked_by((x, y), found)
        if why is None:
            yaw = yaw_toward((x, y), anchor)
            feet = footing(x, y, yaw)
            if feet is None:
                why = 'no ground'
            elif feet[1] > MOST_SPREAD:
                why = 'too steep'
            elif not off_fight(x, y, feet[0]):
                why = "on the boss's ground"
            else:
                s = score(x, y)
                if best is None or s < best[0]:
                    best = (s, (x, y, feet[0], yaw))
                continue
        reasons[why] = reasons.get(why, 0) + 1
    if best is None:
        build.warn(f"{name}'s vigil stone: no spot near ({anchor[0]:.0f}, {anchor[1]:.0f}) will do ({reasons}): left out")
        return None
    return best[1]


# ---------------------------------------------------------------------------
# Each boss's spot
# ---------------------------------------------------------------------------

def gravemother_spot(build):
    """At the Sink's ramp head on the rim, off her ground. Returns (the spot, the lair) or (None, None)."""
    sink = next((f for f in build.source.get('features', []) if f.get('id') == 'theSink'), None)
    ramp = (sink or {}).get('ramp', {}).get('path')
    lair = labelled(build, LAIR_LABEL)
    if not ramp or lair is None:
        build.warn('no Sink ramp in the layout or no Gravemother lair placed (build_area_den.py): no vigil stone for her')
        return None, None
    head = ramp[0]
    mouth = lair.get_actor_location()
    candidates = [(head[0] + math.cos(math.radians(a)) * r, head[1] + math.sin(math.radians(a)) * r)
                  for r in range(350, 760, 60) for a in range(0, 360, 12)]

    def off_fight(x, y, z):
        # Off her ground: past her lair's reach from the den's mouth, or out of its level (the rim, 12 m over the floor).
        return math.hypot(x - mouth.x, y - mouth.y) > GIVE_UP + 200.0 or abs(z - mouth.z) > MAX_RISE + 100.0

    spot = choose(build, 'The Gravemother', candidates, head, off_fight, lambda x, y: math.hypot(x - head[0], y - head[1]))
    return spot, lair


def abel_spot(build):
    """Outside the Keeper's Gate, away from the deck, beside the road. Returns (the spot, Abel) or (None, None)."""
    rocks = [next((f for f in build.source.get('features', []) if f.get('id') == key), None) for key in ('gateSouth', 'gateNorth')]
    abel = labelled(build, ABEL_LABEL)
    if any(rock is None for rock in rocks) or abel is None:
        build.warn("no Keeper's Gate in the layout or no Abel placed (build_area_deck.py): no vigil stone for him")
        return None, None
    (sx, sy), (nx, ny) = rocks[0]['center'], rocks[1]['center']
    middle = ((sx + nx) * 0.5, (sy + ny) * 0.5)
    across = flat((nx - sx, ny - sy))
    spot_abel = abel.get_actor_location()
    out = (-across[1], across[0])
    # Away from the deck (where Abel stands).
    if out[0] * (spot_abel.x - middle[0]) + out[1] * (spot_abel.y - middle[1]) > 0.0:
        out = (-out[0], -out[1])
    candidates = [(middle[0] + out[0] * d + across[0] * s, middle[1] + out[1] * d + across[1] * s)
                  for d in range(300, 1000, 80) for s in range(-800, 820, 80)]

    def off_fight(x, y, z):
        # Outside the gate, and far enough from his spot that calling him back doesn't start his fight.
        beyond = (x - middle[0]) * out[0] + (y - middle[1]) * out[1] > 150.0
        return beyond and math.hypot(x - spot_abel.x, y - spot_abel.y) > DECK_RADIUS + 300.0

    def score(x, y):
        along = abs((x - middle[0]) * across[0] + (y - middle[1]) * across[1])
        return math.hypot(x - middle[0], y - middle[1]) + 0.5 * along

    spot = choose(build, 'Abel', candidates, middle, off_fight, score)
    return spot, abel


def foreman_anchor(build):
    """The Foreman's stone's anchor and the mouth it faces: layout.json's gameplay.mountain (foremanStone, and the mine's
    origin, which build_area_mine.py places the mine from), else the design's (FOREMAN_STONE, FOREMAN_PORTAL)."""
    mountain = build.settings.get('mountain', {})
    stone = tuple(mountain.get('foremanStone', FOREMAN_STONE)[:2])
    portal = tuple(mountain.get('mine', {}).get('origin', FOREMAN_PORTAL)[:2])
    return stone, portal


def foreman_spot(build):
    """By the Providence's portal, facing it. Returns (the spot, his lair or None), or (None, None) while the Diggings aren't
    built. His lair is optional: a stone without one finds the spawner whose SpawnerId is Foreman when the game runs."""
    stone, portal = foreman_anchor(build)
    if ground(*stone) is None:
        build.warn(f"no walkable ground at the Providence's portal ({stone[0]:.0f}, {stone[1]:.0f}) (the Diggings "
                   "aren't built yet): no vigil stone for the Foreman")
        return None, None
    lair = labelled(build, FOREMAN_LABEL)
    sx, sy = stone
    candidates = [stone] + [(sx + math.cos(math.radians(a)) * r, sy + math.sin(math.radians(a)) * r)
                            for r in range(150, 760, 100) for a in range(0, 360, 15)]

    def off_fight(x, y, z):
        # His stope lies 40-60 m under the ridge: a stone on the surface is never on his ground.
        return True

    spot = choose(build, 'The Foreman', candidates, portal, off_fight,
                  lambda x, y: math.hypot(x - stone[0], y - stone[1]))
    return spot, lair


# Where each boss's stone goes (SHRINES' keys).
SPOTS = {'Gravemother': gravemother_spot, 'Abel': abel_spot, 'Foreman': foreman_spot}


# ---------------------------------------------------------------------------
# Placing them
# ---------------------------------------------------------------------------

def condition(after):
    made = unreal.StoryCondition()
    made.set_editor_property('after_missions', [unreal.Name(m) for m in after])
    return made


def place(build):
    """A vigil stone beside each boss arena this level has, in the Gameplay folder (a gameplay build clears it first)."""
    cls = unreal.load_class(None, CLASSES + 'BossShrine')
    if cls is None:
        build.warn('no BossShrine class (build the game module first): the vigil stones are left out')
        return
    placed = 0
    for spec in SHRINES:
        boss_class = unreal.load_class(None, CLASSES + spec['boss_class'])
        if boss_class is None:
            continue
        spot, boss = SPOTS[spec['key']](build)
        if spot is None:
            continue
        x, y, z, yaw = spot
        stone = build.place(cls, (x, y, z), yaw, label=f"VigilStone_{spec['key']}", folder='Gameplay', tags=(SHRINE_TAG,))
        stone.set_editor_property('boss_key', unreal.Name(spec['key']))
        stone.set_editor_property('boss_class', boss_class)
        if spec['kind'] == 'LAIR':
            stone.set_editor_property('call_kind', unreal.BossShrineCall.LAIR)
            if boss is not None:
                stone.set_editor_property('lair', boss)
        else:
            stone.set_editor_property('call_kind', unreal.BossShrineCall.PLACED)
            stone.set_editor_property('placed_boss', boss)
        stone.set_editor_property('boss_name', unreal.Text(spec['name']))
        stone.set_editor_property('loot_hint', unreal.Text(spec['loot']))
        stone.set_editor_property('beaten_when', condition(spec['beaten_after']))
        stone.set_editor_property('callable_when', condition(spec['callable_after']))
        stone.set_editor_property('answer_cue', unreal.Name(spec['answer']))
        placed += 1
        build.log(f"{spec['name']}'s vigil stone at ({x:.0f}, {y:.0f}, {z:.0f}), facing yaw {yaw:.0f}")
    build.log(f'{placed} vigil stones beside the boss arenas')


def run(area):
    """The stones again in the area's open level, on their own: the ones a build placed before go first."""
    sys.path.append(HERE)
    import build_area
    build = importlib.reload(build_area).AreaBuild(area)
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world.get_path_name().split('.')[0] != build.level:
        raise RuntimeError(f'open {build.level} first (the editor has {world.get_path_name()}): nothing was placed')
    import json
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    cls = unreal.load_class(None, CLASSES + 'BossShrine')
    if cls is None:
        raise RuntimeError('no BossShrine class: build the game module first')
    old = [a for a in actors.get_all_level_actors() if a.get_class() == cls and unreal.Name(build.tag) in a.tags]
    if old:
        actors.destroy_actors(old)
        build.log(f'removed the {len(old)} vigil stones placed before')
    place(build)
    levels.save_current_level()


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_shrines.py <Area> (a folder under Art/Levels whose level has its bosses placed)')
    run(sys.argv[1])
