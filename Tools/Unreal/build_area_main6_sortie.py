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
# Then straight up over Moored to this before she turns for the farm: the course's first leg (fixed, 11 m up) ran through the
# lookout on Ransom's Point 15 m off her bow and no raise could move it (Demo47: the "14 raises" warning, seen in PIE). An
# inner control point the sweep can raise further. It leans CLIMB_LEAD toward the farm: a dead-vertical first leg has no
# heading, and her lift slewed her round to it (PilotSortieRules::LiftPose turns her to the course's start) the wrong way.
CLIMB_OUT = 4500.0
CLIMB_LEAD = 600.0
# Her course over the valley: this high over the ground under each control point at least (30 m: over the farm's trees and
# the town's false fronts with the tenders' fall still long), and never lower than the ledge's height plus this.
OVER_GROUND = 3000.0
OVER_LEDGE = 1500.0
# The course's end: this far short of the berth on the line from the town, at the berth's height (her keel's when she lies
# grappled: ABargeBoarding::GetBerth, which rides the barge's deck height) plus this (the C++ glides her the rest, ~9 s, to her
# standoff 15 m off the barge's side).
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


def remove_stale(build, label):
    """This build's actors labelled label, destroyed (a second run of this module on its own, or a place() called twice, would
    otherwise leave a second sortie with the first one's course)."""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in [a for a in actors.get_all_level_actors() if str(a.get_actor_label()) == label and unreal.Name(build.tag) in a.tags]:
        actors.destroy_actor(actor)


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


def barge_berth(build, barge):
    """Where the berth is: ABargeBoarding::GetBerth (a UFUNCTION, so Python calls it), world, from her placed pose and her
    deck's height. If it can't be called the barge's keel middle stands in, loudly: that is a berth some metres too low and
    off her side (the C++ would glide her the rest, but the course's end would be wrong)."""
    try:
        berth = barge.get_berth()
        if isinstance(berth, unreal.Transform):
            return berth.translation
        build.warn(f'ABargeBoarding::GetBerth answered {berth!r}, not a transform')
    except Exception as error:
        build.warn(f'ABargeBoarding::GetBerth can\'t be called from Python ({error})')
    build.warn('the sortie\'s course end is aimed at the barge\'s keel middle, not her berth: build the game module and run again')
    return barge.get_actor_location()


def plan(build, ship, barge, berth=None):
    """The controls of her course out: over the ledge, the farm, the town, short of the barge's berth."""
    moored = ship.get_editor_property(NAMES['moored'])
    m = moored.translation
    ledge_floor = m.z + OVER_LEDGE
    start = (m.x, m.y, m.z + LIFT_TOP)
    controls = [start, (m.x, m.y, m.z + CLIMB_OUT)]
    for kind in (FARM_KIND, TOWN_KIND):
        spot = story.placed(build, kind)
        if spot is None:
            build.warn(f'no {kind} placed: her course doesn\'t pass over it')
            continue
        at = spot.get_actor_location()
        controls.append((at.x, at.y, over_ground(at.x, at.y, ledge_floor)))
    if berth is None:
        berth = barge_berth(build, barge)
    last = controls[-1]
    dx, dy = berth.x - last[0], berth.y - last[1]
    length = math.hypot(dx, dy) or 1.0
    short = min(END_SHORT, length * 0.6)
    ex, ey = berth.x - dx / length * short, berth.y - dy / length * short
    controls.append((ex, ey, max(berth.z + END_OVER_KEEL, over_ground(ex, ey, ledge_floor))))
    # The climb leans toward the next control so the first leg has a heading. At least a sixth of the way: a shorter lean
    # under a far next control bows the Catmull-Rom curve back behind the start (her lift turned her to face away).
    nx, ny = controls[2][0] - m.x, controls[2][1] - m.y
    far = math.hypot(nx, ny) or 1.0
    lean = max(CLIMB_LEAD, far / 6.0)
    controls[1] = (m.x + nx / far * lean, m.y + ny / far * lean, controls[1][2])
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
    berth = barge_berth(build, barge)
    controls, moored = plan(build, ship, barge, berth)
    course, raised_out = sweep(build, controls, 'course out to the barge', ignore=(ship,))
    home_controls = list(reversed(controls[1:]))
    home_controls.append((moored.translation.x, moored.translation.y, moored.translation.z + HOME_OVER))
    home, raised_home = sweep(build, home_controls, 'course home to the ledge', ignore=(ship,))
    first = course[0]
    remove_stale(build, 'PilotSortie')
    sortie = build.place(cls, (first.x, first.y, first.z), 0.0, label='PilotSortie', folder=FOLDER, tags=(NAMES['sortie_tag'],))
    landfall().set_or_warn(build, sortie, NAMES['course'], course, 'the sortie')
    landfall().set_or_warn(build, sortie, NAMES['return_course'], home, 'the sortie')
    landfall().set_or_warn(build, sortie, NAMES['ship'], ship, 'the sortie')
    landfall().set_or_warn(build, sortie, NAMES['barge'], barge, 'the sortie')
    length = sum((course[i + 1] - course[i]).length() for i in range(len(course) - 1))
    end = course[-1]
    keel = barge.get_actor_location().z
    build.log(f'the Main 6 sortie: her course out {len(course)} points ({length / 100.0:.0f} m, raised {raised_out}x) ending at '
              f'({end.x:.0f}, {end.y:.0f}, {end.z:.0f}), {math.hypot(end.x - berth.x, end.y - berth.y) / 100.0:.0f} m from the '
              f'barge\'s berth at ({berth.x:.0f}, {berth.y:.0f}, {berth.z:.0f}; {berth.z - keel:.0f} cm over her keel); home '
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
