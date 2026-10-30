"""The Stone Sentinels on Ransom's Rest (Docs/Work/2026-10-29-ElementsMatter/CONTRACT.md, lane E): four grave statues that
wake (AStoneSentinelCreature), two on Boot Hill and two among the churchyard's west grave rows, outside Saint Ada's Yard's
spots and the Churchyard ambush's, there after Main 4 ("Hallowed Ground": from then the dead walk Boot Hill and the
churchyard). For build_area.py's gameplay pass.

Each is an AEncounterSpawner of its own (one creature, one spot: a statue stands where it was carved), labelled
Encounter_Sentinel_<id>, its spawner id and creature tag Sentinel_<id> (and every one tagged Sentinel), in the area's
Gameplay/Sentinels folder. Its group's level is 0, the zone's (the user's rule: levels from the area's zones, never the
player's); its rank the area's promotions. It spawns far out (SPAWN_REACH), so the statue is standing before the player
can see the spot, and stands passive until it wakes (its own code: a player near or a shot). The creature raises its own
plinth (SM_StoneSentinelPlinth) under itself as play begins (AStoneSentinelCreature::bBringsPlinth), so no plinth is
placed here. Before Main 4 nothing stands there.

The spots (layout cm: x north, y east, as the layout's zones and obstacles; chosen 2026-10-29 from layout.json):
  BootHillWest      (3100, -820)  past the west ends of boot hill's first two grave rows, inside the bootHill zone, 8 m
                                  from its respawn grave (3200, 0)
  BootHillNorth     (3850, -450)  beside the third row's west half, on the hill's north side, 8 m from the grave
  ChurchyardWest    (6700, -3450) between the westmost grave row (y -3300) and the fence (y -3600), halfway along it
  ChurchyardCorner  (7650, -2850) past the north ends of the west rows, by the fence's north side (x 7800)
The checks (warnings, nothing left out for them): the ground's rise across the plinth's footprint (PLINTH_HALF) over
MAX_TILT; within CLEAR of a respawn grave, of Saint Ada's Yard's spots (build_area_chapel.YARD_SPOTS turned with the
chapel) or of the Churchyard ambush's spots. Move a spot in SENTINELS if one warns.

Hook in build_area.py's gameplay(), after the chapel's pieces and the camps (the ambush stands first):
    importlib.reload(importlib.import_module('build_area_sentinels')).place(self)
On its own, in the open editor (clears only Gameplay/Sentinels, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_sentinels.py RansomsRest"
"""
import importlib
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Sentinels'
CLASS = 'StoneSentinelCreature'
TAG = 'Sentinel'
AFTER = ['Main4']

# id, (x, y) layout cm, its hunting ground (build_area_camps.set_ground's form), what it is.
SENTINELS = [
    dict(id='BootHillWest', at=(3100.0, -820.0), ground={'zone': 'bootHill', 'radius': 2500.0, 'maxStep': 400.0},
         where="Boot Hill, past the west ends of its first two rows"),
    dict(id='BootHillNorth', at=(3850.0, -450.0), ground={'zone': 'bootHill', 'radius': 2500.0, 'maxStep': 400.0},
         where="Boot Hill, beside the third row on the hill's north side"),
    dict(id='ChurchyardWest', at=(6700.0, -3450.0),
         ground={'obstacle': 'churchyardFence', 'maxStep': 900.0, 'maxRise': 1000.0},
         where="the churchyard, between the westmost grave row and the fence"),
    dict(id='ChurchyardCorner', at=(7650.0, -2850.0),
         ground={'obstacle': 'churchyardFence', 'maxStep': 900.0, 'maxRise': 1000.0},
         where="the churchyard, past the north ends of the west rows by the fence"),
]

SPAWN_REACH = 12000.0        # cm: the spawner's ActivationRadius (a statue appearing in view would give it away)
DESPAWN_REACH = 16000.0      # cm: its DespawnRadius
PLINTH_HALF = 70.0           # cm: half the plinth's footing (SM_StoneSentinelPlinth, 1.4 m square)
MAX_TILT = 40.0              # cm: the most the ground may rise across the footprint before it warns
CLEAR = 300.0                # cm: kept from respawn graves and the yard's and the ambush's spots


def camps():
    return importlib.reload(importlib.import_module('build_area_camps'))


def chapel_module():
    return importlib.reload(importlib.import_module('build_area_chapel'))


def others(build):
    """(what, (x, y)) of the places a Sentinel keeps CLEAR of: the respawn graves, Saint Ada's Yard's spots (turned with
    the chapel's placement) and the Churchyard ambush's spots."""
    found = []
    for key, spot in build.layout['placements'].items():
        if spot['kind'].startswith('Grave_'):
            found.append((f'the grave {key}', tuple(spot['location'][:2])))
        if spot['kind'] == 'Chapel':
            cx, cy = spot['location'][:2]
            yaw = math.radians(spot.get('yaw', 0.0))
            c, s = math.cos(yaw), math.sin(yaw)
            for i, (fx, fy) in enumerate(chapel_module().YARD_SPOTS):
                found.append((f"Saint Ada's Yard spot {i}", (cx + fx * c - fy * s, cy + fx * s + fy * c)))
    encounters = build.settings.get('encounters', {})
    for ambush in encounters.get('ambushes', []):
        if ambush.get('id') == 'Churchyard':
            for i, (x, y) in enumerate(ambush.get('spots', [])):
                found.append((f'the Churchyard ambush spot {i}', (x, y)))
    return found


def check(build, entry, z, near):
    label = f'Sentinel_{entry["id"]}'
    x, y = entry['at']
    heights = [camps().ground_z(build, x + dx, y + dy, z) for dx in (-PLINTH_HALF, PLINTH_HALF)
               for dy in (-PLINTH_HALF, PLINTH_HALF)]
    heights = [h for h in heights if h is not None]
    if len(heights) < 4:
        build.warn(f'{label}: no ground under part of its plinth at ({x:.0f}, {y:.0f})')
    elif max(heights) - min(heights) > MAX_TILT:
        build.warn(f'{label}: the ground rises {max(heights) - min(heights):.0f} cm across its plinth (over {MAX_TILT:.0f}): '
                   f'move it in SENTINELS')
    for what, (ox, oy) in near:
        d = math.hypot(ox - x, oy - y)
        if d < CLEAR:
            build.warn(f'{label}: {d / 100.0:.1f} m from {what} (under {CLEAR / 100.0:.0f} m): move it in SENTINELS')


def place_one(build, entry, cls, near):
    label = f'Sentinel_{entry["id"]}'
    helpers = camps()
    x, y = entry['at']
    z = helpers.ground_z(build, x, y)
    if z is None:
        build.warn(f'no terrain under {label} ({x:.0f}, {y:.0f}): left out')
        return False
    made = build.place(cls['spawner'], (x, y, z + 50.0), 0.0, label=f'Encounter_{label}', folder=FOLDER)
    made.set_editor_property('spawner_id', unreal.Name(label))
    made.set_editor_property('creature_tags', [unreal.Name(label), unreal.Name(TAG)])
    # One statue, its level the zone's (0), its rank the area's promotions.
    made.set_editor_property('groups', [helpers.group(cls['sentinel'], 1, 'Area')])
    made.set_editor_property('active_when', story.condition(after=AFTER))
    made.set_editor_property('spawn_points', [unreal.Vector(0.0, 0.0, -50.0)])
    made.set_editor_property('spawn_radius', 0.0)
    made.set_editor_property('hunt_on_spawn', False)
    made.set_editor_property('activation_radius', SPAWN_REACH)
    made.set_editor_property('despawn_radius', DESPAWN_REACH)
    helpers.set_ground(build, made, entry['ground'], z)
    check(build, entry, z, near)
    build.log(f'{label} ({entry["where"]}) at ({x:.0f}, {y:.0f}, {z:.0f}), after {", ".join(AFTER)}')
    return True


def place(build):
    """Every Sentinel (SENTINELS). Ransom's Rest only (the level whose layout has Ellis's grave)."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    cls = {'spawner': story.actor_class('EncounterSpawner'), 'sentinel': story.actor_class(CLASS)}
    for key, value in cls.items():
        if value is None:
            build.warn(f'no {"EncounterSpawner" if key == "spawner" else CLASS} class (build the game module first): '
                       f'no Stone Sentinels')
            return
    near = others(build)
    placed = sum(1 for entry in SENTINELS if place_one(build, entry, cls, near))
    build.log(f'Stone Sentinels: {placed} of {len(SENTINELS)} placed')


def run(name):
    """Places Gameplay/Sentinels again on its own, saves."""
    import build_area
    build = importlib.reload(build_area).AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the Stone Sentinels placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_sentinels.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
