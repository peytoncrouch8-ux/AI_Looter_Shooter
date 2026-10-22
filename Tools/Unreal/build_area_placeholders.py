"""Placeholder creatures where the story gives a reason for a different enemy on Ransom's Rest (the user's ask, 2026-10-21:
"if there is a reason in the story ... use a placeholder blob of clay with the name of the enemy"; Docs/Story.md,
"Creature roster"; the plan and its reasons: Saved/Roster/PLAN.md), for build_area.py's gameplay pass. Each is an
AEncounterSpawner of BP_Placeholder_<Kind> classes (Tools/Unreal/create_placeholder_creatures.py makes them), in the area's
Gameplay/Placeholders folder, labelled Encounter_Placeholder_<id>, its spawner id and creature tag Placeholder_<id>; its
creatures spawn in play, so killed they stay dead until the level loads again, and they pay XP and loot as any creature.
A missing class, actor or spot is warned of and left out. Only the level whose layout has Ellis's grave gets any of it.

Two kinds of placement (PLACEMENTS):
- 'like': beside an encounter another script placed (build_area_missionprops_valley.py's cellar and depot fights), its
  ground, height band, activation, spots and story condition copied, so the rats come and go with that fight; its own
  spawner id, so the fight's Clear objective counts only its own dead.
- 'center': a camp of its own on the layout's ground (cm, layout x north, y east), its spots on rings round it, its
  ground a radius or a layout zone, on after (and before) the missions it names, as the camps are (build_area_camps.py's
  helpers).

Hook in build_area.py's gameplay(), after the secrets (the valley's fights must stand first):
    importlib.reload(importlib.import_module('build_area_placeholders')).place(self)
On its own, in the open editor (clears only Gameplay/Placeholders, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_placeholders.py RansomsRest"
"""
import importlib
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Placeholders'
KINDS_FOLDER = '/Game/Data/Placeholders'

# Where each goes and why (Saved/Roster/PLAN.md). Groups: (kind, count, rank). Rings: (radius cm, how many spots) round
# the center, the first ring's spots for the first groups.
PLACEMENTS = [
    dict(id='CellarRats', where="Bright & Daughter's cellar, among the coffins waiting out their seven days (Knocking Below)",
         like='Encounter_TillyCellar', groups=[('CoffinRat', 4, 'Basic')]),
    dict(id='BaggageRats', where="the depot's baggage room, round the coffins bound for Gravesend (The Old Fare)",
         like='Encounter_DepotWaiting', near='Place_BaggageRoom', near_ring=(140.0, 4), groups=[('CoffinRat', 4, 'Basic')]),
    # The hounds' camp stands 20 m in from the Rim (its edge runs diagonally past the lantern house: at (2900, -10600) they
    # chased the player to the lip and fell, PIE 2026-10-22).
    dict(id='KeepersRoadHounds', where="the keeper's road between the cairns and the old lantern house, raiding the boards "
         "nobody sits up with (after Hallowed Ground, until Abel keeps his board after The Gravewind)",
         center=(2000.0, -10000.0), rings=[(450.0, 3), (750.0, 2)], ground={'radius': 1100.0, 'maxStep': 300.0},
         after=['Main4'], before=['Main6'], groups=[('BoneHound', 4, 'Basic'), ('BoneHound', 1, 'Rare')]),
    dict(id='TramwayKnockers', where="the ore tramway across the Bench, round the ore cars off the rails, picking at the "
         "spilled ore (after Dead Lift; clear of the Diggings' spoil ambush, the Dinner Pail camp and the crest patrol)",
         center=(17300.0, 3900.0), rings=[(300.0, 3), (600.0, 3)], ground={'radius': 1200.0, 'maxStep': 300.0},
         after=['DeadLift'], groups=[('Knocker', 5, 'Basic'), ('Knocker', 1, 'Rare')]),
    dict(id='BootHillDiggers', where="Boot Hill, grave robbers digging for grave gold with no keeper to stop them (after Cold "
         "Welcome; gone once the dead walk Boot Hill after Hallowed Ground)",
         center=(3200.0, 0.0), rings=[(400.0, 4), (950.0, 2)], ground={'zone': 'bootHill', 'maxStep': 400.0},
         after=['Main3'], before=['Main4'], groups=[('Gravepicker', 4, 'Basic'), ('GravepickerLookout', 2, 'Basic')]),
]


def camps():
    return importlib.reload(importlib.import_module('build_area_camps'))


def kind_class(kind):
    """BP_Placeholder_<kind>'s class, or None."""
    path = f'{KINDS_FOLDER}/BP_Placeholder_{kind}.BP_Placeholder_{kind}_C'
    return unreal.load_class(None, path) if unreal.EditorAssetLibrary.does_asset_exist(f'{KINDS_FOLDER}/BP_Placeholder_{kind}') else None


def make_groups(build, entry, level=0):
    made = []
    for kind, count, rank in entry['groups']:
        cls = kind_class(kind)
        if cls is None:
            build.warn(f'no BP_Placeholder_{kind} (run create_placeholder_creatures.py first): no {entry["id"]}')
            return None
        group = camps().group(cls, count, rank)
        if level:
            group.set_editor_property('level', level)
        made.append(group)
    return made


def labelled(label):
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def new_spawner(build, label, at):
    cls = story.actor_class('EncounterSpawner')
    if cls is None:
        build.warn(f'no EncounterSpawner class (build the game module first): no {label}')
        return None
    made = build.place(cls, (at.x, at.y, at.z), 0.0, label=f'Encounter_{label}', folder=FOLDER)
    made.set_editor_property('spawner_id', unreal.Name(label))
    made.set_editor_property('creature_tags', [unreal.Name(label)])
    return made


def ring_points(cx, cy, rings, turn=0.0):
    """Spots on rings round (cx, cy): each ring's count evenly round it, the rings turned against each other."""
    spots = []
    for index, (radius, count) in enumerate(rings):
        offset = turn + index * 0.5 * (2.0 * math.pi / max(count, 1))
        for k in range(count):
            angle = offset + 2.0 * math.pi * k / count
            spots.append((cx + math.cos(angle) * radius, cy + math.sin(angle) * radius))
    return spots


def place_like(build, entry):
    """Beside another script's encounter: its ground, band, activation, spots and story condition copied."""
    label = f'Placeholder_{entry["id"]}'
    model = labelled(entry['like'])
    if model is None:
        build.warn(f'no {entry["like"]} in the level (its script places it first): no {label}')
        return False
    model_groups = model.get_editor_property('groups')
    level = model_groups[0].get_editor_property('level') if model_groups else 0
    groups = make_groups(build, entry, level)
    if groups is None:
        return False
    at = model.get_actor_location()
    made = new_spawner(build, label, at)
    if made is None:
        return False
    made.set_editor_property('groups', groups)
    for prop in ('active_when', 'ground_corners', 'ground_max_rise', 'activation_radius', 'hunt_on_spawn'):
        made.set_editor_property(prop, model.get_editor_property(prop))
    spots = model.get_editor_property('spawn_points')
    near = labelled(entry['near']) if entry.get('near') else None
    if entry.get('near') and near is None:
        build.warn(f'{label}: no {entry["near"]}: its rats use {entry["like"]}\'s spots')
    if near is not None:
        n = near.get_actor_location()
        radius, count = entry['near_ring']
        spots = [unreal.Vector(x - at.x, y - at.y, n.z - at.z) for x, y in ring_points(n.x, n.y, [(radius, count)])]
    made.set_editor_property('spawn_points', spots)
    build.log(f'{label} ({entry["where"]}): {", ".join(f"{c} {r} {k}" for k, c, r in entry["groups"])}, with '
              f'{entry["like"]}' + (f', level {level}' if level else ''))
    return True


def place_camp(build, entry):
    """A camp of its own: spots on rings round its center, its ground, on after (and before) its missions."""
    label = f'Placeholder_{entry["id"]}'
    groups = make_groups(build, entry)
    if groups is None:
        return False
    helpers = camps()
    x, y = entry['center']
    z = helpers.ground_z(build, x, y)
    if z is None:
        build.warn(f'no terrain under {label}\'s middle ({x:.0f}, {y:.0f}): left out')
        return False
    made = new_spawner(build, label, unreal.Vector(x, y, z + 50.0))
    if made is None:
        return False
    made.set_editor_property('groups', groups)
    made.set_editor_property('active_when', story.condition(after=entry.get('after', ()), before=entry.get('before', ())))
    # Turned by its id (the same every build: Python's own string hash changes between runs).
    spots = ring_points(x, y, entry['rings'], turn=math.radians(sum(ord(c) for c in entry['id']) % 360))
    made.set_editor_property('spawn_points', helpers.relative(build, spots, x, y, z))
    made.set_editor_property('hunt_on_spawn', False)
    helpers.set_ground(build, made, entry['ground'], z)
    helpers.check_levels(build, label, (x, y, z), spots, float(entry['ground'].get('maxStep', helpers.DEFAULT_STEP)))
    build.log(f'{label} ({entry["where"]}): {", ".join(f"{c} {r} {k}" for k, c, r in entry["groups"])} at ({x:.0f}, {y:.0f}), '
              f'after {", ".join(entry.get("after", ())) or "the start"}'
              + (f', before {", ".join(entry["before"])}' if entry.get('before') else ''))
    return True


def place(build):
    """Every placement (PLACEMENTS). Ransom's Rest only."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    placed = 0
    for entry in PLACEMENTS:
        if (place_like if entry.get('like') else place_camp)(build, entry):
            placed += 1
    build.log(f'placeholder creatures: {placed} of {len(PLACEMENTS)} placed')


def run(name):
    """Places Gameplay/Placeholders again on its own, saves."""
    import build_area
    build = importlib.reload(build_area).AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the placeholder creatures placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_placeholders.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
