"""The waters the player can swim in (World/SwimWater: ASwimWater), from the layout's ponds, in the open level:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_swimwater.py RansomsRest"
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_swimwater.py TutorialIsland"
which opens the area's level (AreaBuild.open_level: it stops if another level has unsaved edits), replaces only the waters a
build placed before (folder <area>/Water, tagged with the build's tag and SwimWater), and saves the level. It needs the C++
built (unreal.SwimWater) and the terrain in the level (its bed is measured by traces).

Round 4 (swimming, the user's pick 2026-10-27; note 30: "entering the pond up there, it is as if the water disappears"):
  - One ASwimWater per layout "pond" feature (Art/Levels/<Area>/layout.json features: Ransom's Rest's larkspurTarn on the
    Bench, the tutorial island's pond and the Wallow's pools), standing on its surface (waterLevel, cm; level cm are layout cm,
    X north, Y east, as every placement is) in the middle of its shore: the pond's ellipse (center, radii) SHORE_GROW wider,
    SHORE_POINTS corners (the waterline wanders about 7% either way round the ellipse: area_shape.py _ponds).
  - Its DeepZone: where the bed lies deeper than SWIM_DEPTH under the surface, measured on the built terrain along DEEP_RAYS
    rays from the middle (DEEP_STEP apart), the last deep point on each. Creatures never follow a player into it
    (USwimWaterSubsystem::IsDeepAt). None when the middle isn't that deep: a wading pool (the Wallow's, 0.3-0.6 m).
  - Its BedDepth: the deepest the traces found.
  - Creeks are logged and left out: a creek's water is CREEK_DEPTH (45 cm) deep in its middle (area_shape.py), wading only;
    Mill Creek's bottom is a dry floor 2 m down. A gorge's stream is 25 cm (GORGE_STREAM).
Every water and its numbers are logged (SWIMWATER lines); "SWIMWATER done" at the end.

Hook for build_area.py (Main): at the end of a full build's gameplay part and in its "water" mode, after the terrain stands:
    importlib.reload(importlib.import_module('build_area_swimwater')).place(self)
(a full build's open_level clears every tagged actor, these too).
"""
import importlib
import math
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
CLASSES = '/Script/AI_Looter_Shooter.'
WATER_TAG = 'SwimWater'
FOLDER = 'Water'

# The player's swim start: Looter.Swim.StartDepth (160 full-size cm) at the player's size (Player/PlayerSize.h Scale 0.85),
# World/SwimWaterRules.cpp. Kept equal by hand: a bed deeper than this under the surface is deep water.
SWIM_DEPTH = 160.0 * 0.85
# The shore drawn this much outside the layout's radii (the waterline wanders about 7% round the ellipse), with this many
# corners.
SHORE_GROW = 1.12
SHORE_POINTS = 32
# The deep zone's rays from the middle, and their step (cm).
DEEP_RAYS = 24
DEEP_STEP = 25.0
# The bed is traced from this far over the surface to this far under it (cm).
TRACE_OVER = 500.0
TRACE_UNDER = 3000.0

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def log(message):
    unreal.log(f'SWIMWATER {message}')


def warn(message):
    unreal.log_warning(f'SWIMWATER warning: {message}')


def editor_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def bed(x, y, surface):
    """The bed under (x, y) (cm), or None: the walkable terrain (tagged Ground) first, else the first solid thing that isn't
    the water's own mesh (an area's <Area>_Water has no collision anyway)."""
    hits = unreal.SystemLibrary.line_trace_multi(editor_world(), unreal.Vector(x, y, surface + TRACE_OVER),
                                                 unreal.Vector(x, y, surface - TRACE_UNDER),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    other = None
    for hit in hits or []:
        t = hit.to_tuple()
        actor = t[9]
        if actor is None:
            continue
        if unreal.Name('Ground') in actor.tags:
            return t[4].z
        label = str(actor.get_actor_label())
        if other is None and not label.endswith('_Water') and not isinstance(actor, unreal.Volume):
            other = t[4].z
    return other


def ellipse_reach(rx, ry, angle):
    """How far the ellipse's edge is from its middle along angle (cm)."""
    c, s = math.cos(angle), math.sin(angle)
    return 1.0 / math.sqrt((c / rx) ** 2 + (s / ry) ** 2)


def shore(cx, cy, rx, ry):
    return [(cx + math.cos(a) * rx * SHORE_GROW, cy + math.sin(a) * ry * SHORE_GROW)
            for a in (2.0 * math.pi * i / SHORE_POINTS for i in range(SHORE_POINTS))]


def deep_zone(pond_id, cx, cy, rx, ry, surface):
    """(the deep zone's corners, the deepest bed found under the surface): the last deep point along each ray from the
    middle; no corners when the middle itself isn't deep (a wading pool)."""
    middle = bed(cx, cy, surface)
    if middle is None:
        warn(f'{pond_id}: no bed found under its middle at ({cx:.0f}, {cy:.0f}): is the terrain in the level?')
        return [], 0.0
    deepest = surface - middle
    corners = []
    for i in range(DEEP_RAYS):
        angle = 2.0 * math.pi * i / DEEP_RAYS
        dx, dy = math.cos(angle), math.sin(angle)
        reach = ellipse_reach(rx, ry, angle) * SHORE_GROW
        last, d = 0.0, 0.0
        while d <= reach:
            z = bed(cx + dx * d, cy + dy * d, surface)
            if z is None:
                break
            deepest = max(deepest, surface - z)
            if surface - z <= SWIM_DEPTH:
                break
            last = d
            d += DEEP_STEP
        corners.append((cx + dx * last, cy + dy * last))
    if surface - middle <= SWIM_DEPTH:
        return [], deepest
    return corners, deepest


def place(build):
    """A swim water per pond in the layout, in the <area>/Water folder (a build clears it first)."""
    cls = unreal.load_class(None, CLASSES + 'SwimWater')
    if cls is None:
        build.warn('no SwimWater class (build the game module first): the swim waters are left out')
        return 0
    placed = 0
    for feature in build.source.get('features', []):
        kind = feature.get('type')
        if kind == 'creek':
            log(f"{feature.get('id')}: a creek, wading only (about 45 cm deep mid-stream): no swim water")
            continue
        if kind != 'pond':
            continue
        pond_id = feature['id']
        (cx, cy), (rx, ry) = feature['center'], feature['radii']
        surface = float(feature['waterLevel'])
        corners, deepest = deep_zone(pond_id, cx, cy, rx, ry, surface)
        water = build.place(cls, (cx, cy, surface), 0.0, label=f'SwimWater_{pond_id}', folder=FOLDER, tags=(WATER_TAG,))
        water.set_editor_property('shore', [unreal.Vector2D(x, y) for x, y in shore(cx, cy, rx, ry)])
        water.set_editor_property('deep_zone', [unreal.Vector2D(x, y) for x, y in corners])
        water.set_editor_property('bed_depth', max(deepest, 0.0))
        water.set_editor_property('water_id', unreal.Name(pond_id))
        placed += 1
        if corners:
            reach = max(math.hypot(x - cx, y - cy) for x, y in corners)
            log(f'{pond_id}: surface {surface / 100.0:.2f} m, bed {deepest / 100.0:.2f} m deep at most, deep (swims) out to '
                f'{reach / 100.0:.1f} m from its middle')
        else:
            log(f'{pond_id}: surface {surface / 100.0:.2f} m, bed {deepest / 100.0:.2f} m deep at most: a wading pool '
                f'(no deep zone; a swim starts at {SWIM_DEPTH / 100.0:.2f} m)')
    build.log(f'{placed} swim waters')
    return placed


def run(area):
    """The swim waters again in the area's level, on their own: the ones a build placed before go first."""
    sys.path.append(HERE)
    import build_area
    build = importlib.reload(build_area).AreaBuild(area)
    build.open_level(FOLDER)
    if unreal.load_class(None, CLASSES + 'SwimWater') is None:
        raise RuntimeError('no SwimWater class: build the game module first')
    placed = place(build)
    levels.save_current_level()
    log(f'done: {placed} swim waters in {build.level}')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_swimwater.py <Area> (a folder under Art/Levels whose level has its terrain built)')
    run(sys.argv[1])
