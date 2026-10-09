"""The level's sound emitters (Audio/AmbientEmitter: AAmbientEmitter, each holding a UAmbientEmitterComponent), placed
from the area's layout so the places that make sound are heard where they are:

  creeks       World.Creek along each creek's line (layout_computed.json creeks), the sound following the point on
               it nearest the listener, a little over the water
  ponds        World.Pond round each pond's shore (ponds, closed paths; the Wallow's little pools are left quiet)
  waterfalls   World.Waterfall a few metres down from each lip (waterfalls)
  the Rim      World.Rim.Wind along every run of three or more open edges of the playable boundary (boundary
               openEdges: the drops that are part of play; a grounded area's canyon side)
  pits         World.Sink.Drip on each pit's floor (layout.json features of type pit: the Sink), with the webs' creaks
               (World.Sink.Creak) now and then round it
  streets      World.Town.Hush along the area's mourning street (AREAS below: Ransom's Rest's Main Street), with its
               boards, a loose shutter and the living behind the walls (World.Town.Creak, World.Shutter.Tap,
               World.Town.Murmur) now and then either side of it

They go in the area's Sound folder, tagged with the area's tag and Ambience. The windmill's sounds are its own
components (World/Windmill), and the chapel bell's hum and the warm train's steam are added in play (UAmbienceSubsystem),
so none of those is placed here.

From build_area.py's full build, after effects() (the waterfalls stand then; nothing here needs them, but it keeps the
level's effects together):
    importlib.reload(build_area_sound).place(self)
On its own, in the open editor (clears the Sound folder, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_sound.py RansomsRest"
Without the game module's class yet it places nothing and says so.
"""
import json
import math
import os
import sys

import unreal

CLASSES = '/Script/AI_Looter_Shooter.'
FOLDER = 'Sound'

# What each area's layout doesn't say: its mourning street (roads by id) and whether its open edges are a canyon's rim.
# An area not listed gets its water only.
AREAS = {
    'RansomsRest': {'streets': ['mainStreet'], 'rim': True, 'pits': True},
    'TutorialIsland': {'streets': [], 'rim': False, 'pits': False},
}

# Heights over what each sound sits on (cm), and the smallest pond with a voice (its mean radius, cm).
OVER_WATER = 40.0
OVER_GROUND = 150.0
SMALLEST_POND = 600.0
# A waterfall's roar sits this far down from its lip (or half its drop, if that's less).
FALL_DEPTH = 400.0
# Runs of open edges this many or more long are a rim; shorter ones (a gorge's mouth) are left to their own sounds.
RIM_EDGES = 3


def ground(x, y):
    """The walkable terrain (tagged Ground) under (x, y), or None."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, 30000.0), unreal.Vector(x, y, -30000.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    for hit in hits or []:
        t = hit.to_tuple()
        if t[9] is not None and unreal.Name('Ground') in t[9].tags:
            return t[4].z
    return None


def emitter(build, cls, label, path, loop=None, shots=(), gap=(6.0, 14.0), scatter=400.0, closed=False, volume=1.0,
            at=None):
    """One AAmbientEmitter: its loop and one-shots, along path (world points; one point: there)."""
    where = at or path[0]
    actor = build.place(cls, where, 0.0, label=label, folder=FOLDER, tags=('Ambience',))
    component = actor.get_editor_property('emitter')
    if loop:
        component.set_editor_property('loop_cue', unreal.Name(loop))
    component.set_editor_property('one_shot_cues', [unreal.Name(s) for s in shots])
    component.set_editor_property('one_shot_gap', unreal.Vector2D(*gap))
    component.set_editor_property('scatter', float(scatter))
    component.set_editor_property('closed_path', bool(closed))
    component.set_editor_property('volume_scale', float(volume))
    component.set_editor_property('path', [unreal.Vector(*p) for p in path] if len(path) > 1 else [])
    return actor


def open_runs(corners, open_edges):
    """Runs of consecutive open edges (edge i runs from corner i to corner i + 1, the last back to corner 0), as lists of
    the corners they pass, wrapping round the start."""
    count = len(corners)
    if not count or not any(open_edges):
        return []
    if all(open_edges):
        return [corners + [corners[0]]]
    # Start just after a closed edge, so a run crossing the list's end stays whole.
    start = next(i for i in range(count) if not open_edges[i]) + 1
    runs, run = [], []
    for k in range(count):
        i = (start + k) % count
        if open_edges[i]:
            if not run:
                run = [corners[i]]
            run.append(corners[(i + 1) % count])
        elif run:
            runs.append(run)
            run = []
    if run:
        runs.append(run)
    return runs


def place(build):
    """Places the area's emitters (build: build_area.AreaBuild, its layout read)."""
    cls = unreal.load_class(None, CLASSES + 'AmbientEmitter')
    if cls is None:
        build.warn('no AmbientEmitter class (build the game module first): the level has no places that sound')
        return
    layout = build.layout
    area = AREAS.get(build.name, {'streets': [], 'rim': False, 'pits': False})
    counts = {}

    def count(kind):
        counts[kind] = counts.get(kind, 0) + 1

    for key, creek in (layout.get('creeks') or {}).items():
        points = [(x, y, z + OVER_WATER) for x, y, z in creek['points']]
        if len(points) >= 2:
            emitter(build, cls, f'Sound_Creek_{key}', points, loop='World.Creek')
            count('creek')

    for key, pond in (layout.get('ponds') or {}).items():
        (cx, cy), (rx, ry) = pond['center'], pond['radii']
        if 0.5 * (rx + ry) < SMALLEST_POND:
            continue
        z = pond['waterZ'] + OVER_WATER
        shore = [(cx + rx * math.cos(a), cy + ry * math.sin(a), z) for a in (2 * math.pi * k / 16 for k in range(16))]
        # A bigger pond laps a little louder.
        volume = max(0.6, min(1.2, 0.5 * (rx + ry) / 1200.0))
        emitter(build, cls, f'Sound_Pond_{key}', shore, loop='World.Pond', closed=True, volume=volume, at=(cx, cy, z))
        count('pond')

    falls = layout.get('waterfalls') or ({'waterfall': layout['waterfall']} if layout.get('waterfall') else {})
    for key, fall in falls.items():
        x, y, z = fall['location']
        drop = z - fall.get('dropTo', z - 2 * FALL_DEPTH)
        emitter(build, cls, f'Sound_Waterfall_{key}', [(x, y, z - min(FALL_DEPTH, 0.5 * drop))], loop='World.Waterfall')
        count('waterfall')

    boundary = layout.get('boundary') or {}
    if area.get('rim') and boundary.get('corners'):
        for i, run in enumerate(open_runs(boundary['corners'], boundary.get('openEdges', []))):
            if len(run) - 1 < RIM_EDGES:
                continue
            emitter(build, cls, f'Sound_Rim_{i + 1}', [(x, y, z + OVER_GROUND) for x, y, z in run], loop='World.Rim.Wind')
            count('rim')

    if area.get('pits'):
        for feature in build.source.get('features', []):
            if feature.get('type') != 'pit' or not feature.get('polygon'):
                continue
            xs = [p[0] for p in feature['polygon']]
            ys = [p[1] for p in feature['polygon']]
            cx, cy = sum(xs) / len(xs), sum(ys) / len(ys)
            radius = 0.25 * ((max(xs) - min(xs)) + (max(ys) - min(ys)))
            floor = ground(cx, cy)
            if floor is None:
                build.warn(f"no ground under the pit {feature['id']}: its drips are left out")
                continue
            emitter(build, cls, f"Sound_Pit_{feature['id']}", [(cx, cy, floor + OVER_GROUND)], loop='World.Sink.Drip',
                    shots=('World.Sink.Creak',), gap=(7.0, 16.0), scatter=0.6 * radius)
            count('pit')

    roads = layout.get('roads') or {}
    for street in area.get('streets', []):
        road = roads.get(street)
        if not road:
            build.warn(f'no road {street} in the layout: its hush is left out')
            continue
        points = [(x, y, z + OVER_GROUND) for x, y, z in road['points']]
        # The false fronts stand either side of the street's middle, about half its width out.
        emitter(build, cls, f'Sound_Street_{street}', points, loop='World.Town.Hush',
                shots=('World.Town.Creak', 'World.Shutter.Tap', 'World.Town.Murmur'), gap=(5.0, 12.0),
                scatter=0.6 * road.get('width', 1000.0))
        count('street')

    build.log('sound emitters: ' + (', '.join(f'{n} {k}' for k, n in sorted(counts.items())) or 'none'))


def run(name):
    """Places the area's emitters again on their own: clears the Sound folder, places, saves."""
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    import build_area  # noqa: E402 (here, not at the top: build_area imports this module)
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('sound emitters placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_sound.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
