"""The gunsmith's benches (World/GunsmithBench: AGunsmithBench), for build_area.py's gameplay pass: one at each spot of
layout.json gameplay.benches ({"id", "at": [X, Y], "yaw"}, the yaw the way its front faces, where the player stands to
use it), labeled by its id in the Gameplay folder and tagged Obstacle for the minimap. The user's call (2026-10-08): a
plain bench now, in Skyreach's village and at Ransom Farm, until Ozias Penhallow takes over after the Gilded Lily.

A bench stands on the lowest ground under its four corners, so no leg hangs in the air on a slope (the higher ones sink
a little into the ground). Without the game module's class yet it's left out with a warning.
"""
import math

import unreal

CLASSES = '/Script/AI_Looter_Shooter.'
# The bench's footprint (cm, from Art/Models/Props/GunsmithBench.py): half its width along its right, half its depth.
HALF_WIDTH = 105.0
HALF_DEPTH = 45.0


def ground(x, y):
    """The walkable terrain (tagged Ground) under (x, y), or None: a bale or a fence rail never counts."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, 30000.0), unreal.Vector(x, y, -30000.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    for hit in hits or []:
        t = hit.to_tuple()
        if t[9] is not None and unreal.Name('Ground') in t[9].tags:
            return t[4].z
    return None


def place(build):
    benches = build.settings.get('benches', [])
    if not benches:
        return
    cls = unreal.load_class(None, CLASSES + 'GunsmithBench')
    if cls is None:
        build.warn('no GunsmithBench class (build the game module first): the benches are left out')
        return
    for spot in benches:
        x, y = spot['at']
        yaw = math.radians(spot['yaw'])
        fx, fy = math.cos(yaw), math.sin(yaw)
        rx, ry = -fy, fx
        corners = [ground(x + rx * a + fx * d, y + ry * a + fy * d)
                   for a in (-HALF_WIDTH, HALF_WIDTH) for d in (-HALF_DEPTH, HALF_DEPTH)]
        corners = [z for z in corners if z is not None]
        if not corners:
            build.warn(f"no ground under the bench {spot['id']} at ({x}, {y})")
            continue
        build.place(cls, (x, y, min(corners)), spot['yaw'], label=spot['id'], folder='Gameplay', tags=('Obstacle',))
        build.log(f"bench {spot['id']} at ({x}, {y}), facing {spot['yaw']}, its feet within "
                  f"{max(corners) - min(corners):.0f} cm")
