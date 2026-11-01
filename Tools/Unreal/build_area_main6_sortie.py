"""Main 6, "The Pilot Light" (the Wardens story, 2026-11-01; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 10, lane
MAIN67 W2): the Lamplighter's part of it, for build_area_main6.py (W3's: its place() places the barge over the chapel knoll,
then calls place(build, barge) here; reloaded each run, as the editor keeps modules between runs). It runs after
build_area_landfall.py (the ship, her Moored pose and the winch must stand). Everything goes in the area's Gameplay/Main6
folder; a piece whose class or ground isn't there yet is left out with a warning.

What it places and sets, measured from the built level with traces:
- The sortie (APilotSortie, tagged PilotSortie) with its ship and barge, its Course out and its ReturnCourse home:
  from over the Mooring Ledge (the lift's top) east over the farm and the town to about where the ship comes up to her
  standoff beside the barge, and back the same way to over the ledge; every segment swept for her hull (Landfall's
  hull_clear: keel, bag top, both rails) and its control points raised over the ground (and over anything in the way)
  until it's clear. The C++ glides her the last stretch to the standoff (ABargeBoarding::GetBerth) and settles her from
  the home course's end onto Moored.
- The anchor winch's cast_off_when (Main 6's first step: "Cast off: hold {Interact} on the anchor winch").
- The ship's gone_when (after Main 7: she lifts off dark at dawn; a later load finds no ship on the ledge).
Returns Hob's spot on her wheel housing (valid while she's moored: Main 6's first step, and after it), for
build_area_story.place_hob: {'wheel': (x, y, z, yaw) or None}.
"""
import importlib
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Main6'
MAIN6, MAIN7 = 'Main6', 'Main7'
CAST_OFF_STEP = 0

# The C++ names (APilotSortie, ALamplighterShip, ALandfallWinch; their Python property names).
NAMES = dict(sortie_class='PilotSortie', ship_class='LamplighterShip', winch_class='LandfallWinch', sortie_tag='PilotSortie',
             course='course', return_course='return_course', ship='ship', barge='barge', cast_off_when='cast_off_when',
             gone_when='gone_when', moored='moored')
# Where the farm and the town are (build_area.py's placements, by kind): the course passes over each.
FARM_KIND = 'Farmhouse'
TOWN_KIND = 'FalseFront_Undertaker'

# ---------------------------------------------------------------------------
# Numbers (cm)
# ---------------------------------------------------------------------------

# The lift's top over Moored (PilotLight::LiftRise 900, and a little more so the course starts above it).
LIFT_TOP = 1100.0
# Her course over the valley: this high over the ground under each control point at least (30 m: over the farm's trees and
# the town's false fronts with the tenders' fall still long), and never lower than the ledge's height plus this.
OVER_GROUND = 3000.0
OVER_LEDGE = 1500.0
# The course's end: this far short of the barge on the line from the town, at the barge's keel height plus this (the C++
# glides her the rest, ~9 s, to her standoff 15 m off its side).
END_SHORT = 6500.0
END_OVER_KEEL = 600.0
# The home course's end: over Moored this high (the C++ settles her from there over 7 s).
HOME_OVER = 1500.0
# Samples along the curve, and the sweep's raises.
STEP = 1500.0
RAISE, TRIES = 500.0, 14
# Hob on the wheel housing (ALamplighterShip's WheelSpot (-1050, 0, 142) over her deck, the wheel 1 m across), her frame.
WHEEL_PERCH = (-1050.0, 0.0, 422.0 + 142.0 + 55.0)


def landfall():
    return importlib.import_module('build_area_landfall')


def class_actors(cls):
    world = unreal.EditorLevelLibrary.get_editor_world()
    return list(unreal.GameplayStatics.get_all_actors_of_class(world, cls)) if cls else []


def first_actor(name):
    cls = story.actor_class(name)
    found = class_actors(cls)
    return found[0] if found else None


def over_ground(x, y, at_least):
    hit = landfall().trace(x, y, 60000.0, -60000.0)
    return max((hit[0] if hit else -60000.0) + OVER_GROUND, at_least)


def sweep(build, controls, what, ignore=()):
    """Raises the control points (never the first or the last) round each touch until the hull clears every segment.
    Returns the sampled points (unreal.Vector) and how many raises it took. ignore: the ship herself (placed on the ledge
    by Landfall: the course starts 11 m and ends 15 m over her mooring, so her own 21 m bag stood in its first and last
    segments, which no raise can move; 2026-11-01's '14 raises' warnings)."""
    lf = landfall()
    controls = [list(c) for c in controls]
    raised = 0
    for _ in range(TRIES):
        samples, marks = lf.catmull([tuple(c) for c in controls], STEP)
        bad = [i for i in range(len(samples) - 1) if not lf.hull_clear(samples[i], samples[i + 1], ignore)]
        if not bad:
            break
        # The control point at or before the first touch, and the next (inner points only).
        touched = max([k for k in range(len(marks)) if marks[k] <= bad[0]] or [0])
        for k in {touched, touched + 1}:
            if 0 < k < len(controls) - 1:
                controls[k][2] += RAISE
        raised += 1
    else:
        build.warn(f'the Lamplighter\'s {what} still touches something after {TRIES} raises: look along it in the editor')
    samples, _ = lf.catmull([tuple(c) for c in controls], STEP)
    return [unreal.Vector(*p) for p in samples], raised


def barge_berth_guess(barge):
    """Where the berth is: ABargeBoarding::GetBerth if Python can call it, else the barge's middle (the C++ glides her the
    rest of the way to the real one)."""
    try:
        berth = barge.call_method('GetBerth')
        if isinstance(berth, unreal.Transform):
            return berth.translation
    except Exception:
        pass
    return barge.get_actor_location()


def plan(build, ship, barge):
    """The controls of her course out: over the ledge, the farm, the town, short of the barge."""
    moored = ship.get_editor_property(NAMES['moored'])
    m = moored.translation
    ledge_floor = m.z + OVER_LEDGE
    start = (m.x, m.y, m.z + LIFT_TOP)
    controls = [start]
    for kind in (FARM_KIND, TOWN_KIND):
        spot = story.placed(build, kind)
        if spot is None:
            build.warn(f'no {kind} placed: her course doesn\'t pass over it')
            continue
        at = spot.get_actor_location()
        controls.append((at.x, at.y, over_ground(at.x, at.y, ledge_floor)))
    berth = barge_berth_guess(barge)
    last = controls[-1]
    dx, dy = berth.x - last[0], berth.y - last[1]
    length = math.hypot(dx, dy) or 1.0
    short = min(END_SHORT, length * 0.6)
    ex, ey = berth.x - dx / length * short, berth.y - dy / length * short
    controls.append((ex, ey, max(berth.z + END_OVER_KEEL, over_ground(ex, ey, ledge_floor))))
    return controls, moored


def place(build, barge):
    """The sortie, the winch's cast-off condition and the ship's gone condition. Returns Hob's wheel spot."""
    out = {'wheel': None}
    if barge is None:
        build.warn('no barge (build_area_main6.py places it first): no Main 6 sortie')
        return out
    ship = first_actor(NAMES['ship_class'])
    if ship is None:
        build.warn('no Lamplighter (build_area_landfall.py places her first): no Main 6 sortie')
        return out
    cls = story.actor_class(NAMES['sortie_class'])
    if cls is None:
        build.warn('no PilotSortie class (build the game module first): no Main 6 sortie')
        return out
    controls, moored = plan(build, ship, barge)
    course, raised_out = sweep(build, controls, 'course out to the barge', ignore=(ship,))
    home_controls = list(reversed(controls[1:]))
    home_controls.append((moored.translation.x, moored.translation.y, moored.translation.z + HOME_OVER))
    home, raised_home = sweep(build, home_controls, 'course home to the ledge', ignore=(ship,))
    first = course[0]
    sortie = build.place(cls, (first.x, first.y, first.z), 0.0, label='PilotSortie', folder=FOLDER, tags=(NAMES['sortie_tag'],))
    landfall().set_or_warn(build, sortie, NAMES['course'], course, 'the sortie')
    landfall().set_or_warn(build, sortie, NAMES['return_course'], home, 'the sortie')
    landfall().set_or_warn(build, sortie, NAMES['ship'], ship, 'the sortie')
    landfall().set_or_warn(build, sortie, NAMES['barge'], barge, 'the sortie')
    length = sum((course[i + 1] - course[i]).length() for i in range(len(course) - 1))
    build.log(f'the Main 6 sortie: her course out {len(course)} points ({length / 100.0:.0f} m, raised {raised_out}x), home '
              f'{len(home)} points (raised {raised_home}x)')

    winch = first_actor(NAMES['winch_class'])
    if winch is None:
        build.warn('no anchor winch on the ledge: Main 6 can\'t cast off (the ship spawns one moored, without the condition)')
    else:
        landfall().set_or_warn(build, winch, NAMES['cast_off_when'],
                               story.condition(during=MAIN6, before_step=CAST_OFF_STEP + 1), 'the anchor winch')
    landfall().set_or_warn(build, ship, NAMES['gone_when'], story.condition(after=[MAIN7]), 'the Lamplighter')

    at = moored.transform_location(unreal.Vector(*WHEEL_PERCH))
    out['wheel'] = (at.x, at.y, at.z, moored.rotation.rotator().yaw)
    return out
