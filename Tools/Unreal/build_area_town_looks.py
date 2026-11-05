"""The town's buildings re-finished with no new draw (Docs/Work/2026-11-04-Towns/PLAN.md 4.2 item 1): each building's roof
tint, siding tint and porch (the trim boards swapped or not) from Art/Levels/<Area>/town_looks.json, written into its
model's custom primitive data, which M_World's TrimRecolor reads on MI_HouseTrim (Tools/Unreal/build_world_materials.py:
TRIM_DATA, 0-2 the roof tint, 3 the swap, 4-6 the siding stain, 7 the paint's cover, 8-10 its colour, 11 the roof's grey;
zeros change nothing). Tints: a roof is a tint list or {tint, grey}; a siding is {tint (the bare boards' stain), paint
(sRGB hex; none is whitewash cream), wash (how much paint covers the boards)}.

First the table is checked against the neighbour rule: buildings next to each other along a side (town_looks.json sides)
differ in roof (its tint, or a roof kit that isn't the shake strip) AND siding AND porch; buildings facing each other across
Main Street (facing: the two sides, a pair whose placements stand within facingReach cm along the street) differ in at
least 2 of the 3. A table that breaks either is refused (nothing is written) and every breaking pair is printed. A
building named in the table but not in the level is warned about and left out.

The buildings are the area build's actors by label (the placement id; the depot's station by its Building component).
After build_area.py's full build, or on its own in the open editor (sets the data, saves the level):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_town_looks.py RansomsRest"
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_town_looks.py RansomsRest check"  (the rule only)
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_town_looks.py RansomsRest clear"  (back to defaults)
The per-instance data of AInstancedProps (stacked crates and barrels) is later work (the LIGHTS lane's C++).
"""
import json
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
REPO = os.path.dirname(os.path.dirname(HERE))
# M_World's TrimRecolor indices (build_world_materials.TRIM_DATA; kept here, not imported: that module builds as it runs).
ROOF, SWAP, SIDING, WASH, PAINT, GREY = 0, 3, 4, 7, 8, 11
DEFAULTS = (1.0, 1.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0)


def linear(hex_colour):
    """An sRGB hex colour's linear values (the paint is listed as it looks, the material wants linear)."""
    out = []
    for i in (1, 3, 5):
        c = int(hex_colour[i:i + 2], 16) / 255.0
        out.append(c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4)
    return out


def log(text):
    unreal.log(f'Town looks: {text}')


def warn(text):
    unreal.log_warning(f'Town looks: {text}')


def roof_of(spec):
    return f'kit:{spec["roofKit"]}' if spec.get('roofKit') else spec['roof']


def check(looks, locations):
    """The neighbour rule's breaking pairs (empty when the table keeps it)."""
    buildings = looks['buildings']
    broken = []

    def differences(a, b):
        sa, sb = buildings[a], buildings[b]
        return [roof_of(sa) != roof_of(sb), sa['siding'] != sb['siding'], bool(sa['porchSwap']) != bool(sb['porchSwap'])]

    for side, ids in looks['sides'].items():
        for a, b in zip(ids, ids[1:]):
            diff = differences(a, b)
            if not all(diff):
                same = [name for name, d in zip(('roof', 'siding', 'porch'), diff) if not d]
                broken.append(f'{side}: {a} and {b} next to each other share their {" and ".join(same)}')
    facing = looks.get('facing')
    if facing:
        first, second = (looks['sides'][s] for s in facing['sides'])
        axis = 0 if facing.get('along', 'y') == 'x' else 1
        reach = float(facing.get('facingReach', 800.0))
        for a in first:
            for b in second:
                if a not in locations or b not in locations:
                    continue
                if abs(locations[a][axis] - locations[b][axis]) > reach:
                    continue
                diff = differences(a, b)
                if sum(diff) < 2:
                    same = [name for name, d in zip(('roof', 'siding', 'porch'), diff) if not d]
                    broken.append(f'facing: {a} and {b} share their {" and ".join(same)} (2 of 3 must differ)')
    return broken


def data_of(looks, spec):
    """The eight custom data floats for a building's looks."""
    values = list(DEFAULTS)
    if not spec.get('roofKit'):
        roof = looks['tints']['roof'][spec['roof']]
        roof = roof if isinstance(roof, dict) else {'tint': roof}
        values[ROOF:ROOF + 3] = [float(v) for v in roof['tint']]
        values[GREY] = float(roof.get('grey', 0.0))
    values[SWAP] = 1.0 if spec.get('porchSwap') else 0.0
    siding = looks['tints']['siding'][spec['siding']]
    values[SIDING:SIDING + 3] = [float(v) for v in siding.get('tint', (1.0, 1.0, 1.0))]
    values[WASH] = float(siding.get('wash', 0.0))
    if siding.get('paint'):
        values[PAINT:PAINT + 3] = linear(siding['paint'])
    return values


def components(actor):
    """The static mesh components that may wear the house trim: a placed mesh's own, or every one of a station's."""
    if isinstance(actor, unreal.StaticMeshActor):
        return [actor.static_mesh_component]
    return [c for c in actor.get_components_by_class(unreal.StaticMeshComponent)]


def run(area, mode='set'):
    import build_area
    build = build_area.AreaBuild(area)
    path = os.path.join(REPO, 'Art', 'Levels', area, 'town_looks.json')
    if not os.path.exists(path):
        warn(f'no {path}: nothing to do')
        return
    with open(path, encoding='utf-8') as source:
        looks = json.load(source)
    with open(build.computed_path, encoding='utf-8') as source:
        placements = json.load(source)['placements']
    locations = {key: spot['location'] for key, spot in placements.items()}
    unknown = [b for ids in looks['sides'].values() for b in ids if b not in looks['buildings']]
    if unknown:
        raise RuntimeError(f'town_looks.json sides name buildings it has no looks for: {unknown}')
    broken = check(looks, locations)
    if broken:
        for line in broken:
            warn(line)
        raise RuntimeError(f'town_looks.json breaks the neighbour rule ({len(broken)} pairs, above): nothing written')
    log(f'the table keeps the neighbour rule ({len(looks["buildings"])} buildings)')
    if mode == 'check':
        return
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if world.get_path_name().split('.')[0] != build.level:
        raise RuntimeError(f'open {build.level} first (the open level is {world.get_path_name()})')
    found = {}
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if unreal.Name(build.tag) in actor.tags:
            found.setdefault(str(actor.get_actor_label()), actor)
    written = 0
    for key, spec in looks['buildings'].items():
        actor = found.get(key)
        if actor is None:
            warn(f'no {key} in the level: left out')
            continue
        values = list(DEFAULTS) if mode == 'clear' else data_of(looks, spec)
        for component in components(actor):
            component.modify()
            for index, value in enumerate(values):
                # The defaults are what the level saves (set_custom_primitive_data_* is run-time only, never saved);
                # setting them resets the run-time data to them.
                component.set_default_custom_primitive_data_float(index, value)
        written += 1
        log(f'{key}: roof {roof_of(spec)}, siding {spec["siding"]}, porch {"swapped" if spec.get("porchSwap") else "as built"}'
            + (' (cleared)' if mode == 'clear' else ''))
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    log(f'{written} buildings {"cleared" if mode == "clear" else "finished"}; the level saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_town_looks.py <Area> [check|clear]')
    run(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else 'set')
