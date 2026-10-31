"""The spiders' lurks (the user's pick, 2026-10-31: spiders ambush from walls, ledges and trees; Saved/CreatureCode/SPIDER/README.md
until it's merged, then Docs/Pipeline/Enemies.md "The spiders' ambush"): the layout's gameplay.encounters.lurks as
AAmbushSpawners with Entrance Lurk, in the area's Gameplay/Lurks folder, labelled Encounter_Lurk_<id>, spawner id and creature
tag Lurk_<id>. Each brings its spiders out as the player comes within its activation radius, onto their perches (FSpiderPerchSpot:
ASpiderCreature::BeginLurk finds the real surface near each), where they wait still until the player passes near or under
them, and spring.

A lurk entry (layout cm, X north, Y east; Looter.Creatures.Spider.Ambush.Placement checks them):
    {"id": "GoatSteps", "where": "...", "when": {"after": [...], "withFlags": [...], ...} (build_area_fights_mountain's "when"),
     "groups": [{"class": "SpiderCreature", "count": 3, "rank": "Area"}] (build_area_fights_mountain's groups),
     "perches": [{"kind": "Wall" | "Ledge" | "Tree", "at": [x, y], "face": yaw, "up": cm, "out": cm, "springRadius": cm}],
     "ground": {"radius" | "polygon" | "obstacle" | "zone", "maxRise", "margin", "maxStep"} (build_area_camps.set_ground's),
     "activation": cm (6000), "despawn": cm (9000), "tags": [...]}
- A wall's "at" is a point on the wall's foot line (layout_computed.json's cliffs), "face" the way the wall faces (degrees,
  0 = +X), "up" the perch's height over the ground in front of it; a tree's "at" is its trunk, "face" the side it hangs on; a
  ledge's "at" is over its top, "face" the way it looks over the edge.
- Each spider is brought out "out" cm out along "face" from "at" (a wall 250, a tree 200 unless the entry says), on the ground
  there: its spot (SpawnPoints), from which it takes the perch nearest it.
- The perch is looked for by the spider in play, so a rebuilt rock still holds it; this only gives it the place to look:
  a wall's spot 30 cm out of its foot line at its height, a tree's on the trunk's axis at its height, a ledge's top found by
  a trace down onto it here.

Hook in build_area.py's gameplay(), after build_area_camps (it uses its helpers and those of build_area_fights_mountain):
    importlib.reload(importlib.import_module('build_area_lurks')).place(self)
On its own, in the open editor (clears only Gameplay/Lurks, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_lurks.py RansomsRest"
"""
import importlib
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Lurks'
KINDS = {'Wall': 'WALL', 'Ledge': 'LEDGE', 'Tree': 'TREE'}
# How far out along "face" a spider is brought out (cm), unless the perch says ("out").
OUT = {'Wall': 250.0, 'Ledge': 250.0, 'Tree': 200.0}
# The spot a wall's perch is looked for from: this far out of its foot line (cm), so the spider's look back at the face
# (from 1.5 m out to 3 m in) starts in the open even where the face leans out a little.
WALL_SPOT_OUT = 30.0
# A ledge's top is looked for this far over and under the ground in front of it (cm).
LEDGE_LOOK = 1500.0
DEFAULT_UP = 300.0
DEFAULT_ACTIVATION = 6000.0
DEFAULT_DESPAWN = 9000.0


def face_way(yaw):
    radians = math.radians(yaw)
    return math.cos(radians), math.sin(radians)


def top_z(x, y, near):
    """The top of whatever stands at (x, y) (a rock's, a ledge's: any geometry, not just the terrain), looked for down from
    LEDGE_LOOK over near; None when nothing's there."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, near + LEDGE_LOOK), unreal.Vector(x, y, near - LEDGE_LOOK),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], unreal.DrawDebugTrace.NONE, True)
    return None if hit is None else hit.to_tuple()[5].z


def place_one(build, entry):
    """One lurk's spawner. False (with a warning) when its class, its groups or its perches are missing."""
    camps = importlib.import_module('build_area_camps')
    fights = importlib.reload(importlib.import_module('build_area_fights_mountain'))
    label = f'Lurk_{entry["id"]}'
    cls = story.actor_class('AmbushSpawner')
    made_groups = fights.groups(build, entry)
    perches = entry.get('perches', [])
    if cls is None or made_groups is None or not perches:
        build.warn(f'{label}: no AmbushSpawner class, groups or perches: left out')
        return False
    # Where each spider is brought out: out along its perch's way from its foot, on the ground there.
    feet = []
    for perch in perches:
        kind = perch.get('kind', 'Wall')
        fx, fy = face_way(float(perch.get('face', 0.0)))
        out = float(perch.get('out', OUT.get(kind, 250.0)))
        ax, ay = perch['at']
        feet.append((ax + fx * out, ay + fy * out))
    mx = sum(f[0] for f in feet) / len(feet)
    my = sum(f[1] for f in feet) / len(feet)
    mz = camps.ground_z(build, mx, my)
    if mz is None:
        build.warn(f'{label}: no terrain under its middle ({mx:.0f}, {my:.0f}): it stands at 0')
        mz = 0.0
    made_perches = []
    for perch, (footx, footy) in zip(perches, feet):
        kind = perch.get('kind', 'Wall')
        if kind not in KINDS:
            build.warn(f'{label}: a perch of kind {kind}: left out')
            continue
        face = float(perch.get('face', 0.0))
        fx, fy = face_way(face)
        ax, ay = perch['at']
        ground = camps.ground_z(build, footx, footy, mz)
        if ground is None:
            build.warn(f'{label}: no ground in front of its perch at ({ax:.0f}, {ay:.0f}): its height is its middle\'s')
            ground = mz
        if kind == 'Ledge':
            top = top_z(ax, ay, ground)
            spot = unreal.Vector(ax, ay, (ground if top is None else top) + 50.0)
        elif kind == 'Tree':
            spot = unreal.Vector(ax, ay, ground + float(perch.get('up', DEFAULT_UP)))
        else:
            spot = unreal.Vector(ax + fx * WALL_SPOT_OUT, ay + fy * WALL_SPOT_OUT, ground + float(perch.get('up', DEFAULT_UP)))
        made = unreal.SpiderPerchSpot()
        made.set_editor_property('kind', getattr(unreal.SpiderPerchKind, KINDS[kind]))
        made.set_editor_property('spot', spot)
        made.set_editor_property('face_yaw', face)
        if 'springRadius' in perch:
            made.set_editor_property('spring_radius', float(perch['springRadius']))
        made_perches.append(made)
    spawner = build.place(cls, (mx, my, mz + 50.0), 0.0, label=f'Encounter_{label}', folder=FOLDER)
    spawner.set_editor_property('spawner_id', unreal.Name(label))
    spawner.set_editor_property('groups', made_groups)
    spawner.set_editor_property('creature_tags', [unreal.Name(label)] + [unreal.Name(tag) for tag in entry.get('tags', ())])
    when = fights.when_spec(entry)
    spawner.set_editor_property('active_when', fights.condition(when))
    spawner.set_editor_property('entrance', unreal.AmbushEntrance.LURK)
    # They wait on their perches: never set on the player as they're brought out (AAmbushSpawner::BeginPlay says so too).
    spawner.set_editor_property('hunt_on_spawn', False)
    spawner.set_editor_property('perches', made_perches)
    spawner.set_editor_property('spawn_points', camps.relative(build, feet, mx, my, mz))
    spawner.set_editor_property('activation_radius', float(entry.get('activation', DEFAULT_ACTIVATION)))
    spawner.set_editor_property('despawn_radius', float(entry.get('despawn', DEFAULT_DESPAWN)))
    camps.set_ground(build, spawner, entry.get('ground', {}), mz)
    camps.check_levels(build, f'Encounter_{label}', (mx, my, mz), feet, float(entry.get('ground', {}).get('maxStep', camps.DEFAULT_STEP)))
    build.log(f'Encounter_{label} ({entry.get("where", "")}): {len(made_perches)} perches, '
              f'{", ".join(str(g["count"]) + " " + g["rank"] + " " + g["class"] for g in entry["groups"])} at ({mx:.0f}, {my:.0f}, {mz:.0f}); '
              f'{fights.condition_words(when)}')
    return True


def place(build):
    """Every lurk of the layout's gameplay.encounters.lurks. A layout without any places nothing."""
    encounters = build.settings.get('encounters') or {}
    lurks = encounters.get('lurks', [])
    if not lurks:
        return
    placed = sum(1 for entry in lurks if place_one(build, entry))
    build.log(f'spider lurks: {placed} of {len(lurks)} placed')


def run(name):
    """Places Gameplay/Lurks again on its own, saves."""
    import build_area
    build = importlib.reload(build_area).AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the spider lurks placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_lurks.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
