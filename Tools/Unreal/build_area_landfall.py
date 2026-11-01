"""Landfall's world pieces on Ransom's Rest (Main 1 "Landfall", the Wardens story of 2026-10-31:
Docs/Work/2026-10-31-Wardens/CONTRACT.md, lane WORLD). build_area_story.place() calls place() here (reloaded each run,
as the editor keeps modules between runs), after the deck (whose AStoryLighting the Smog rule joins) and before Hob
(who takes the perches this hands back). Everything goes in the area's Gameplay/Landfall folder, so a "gameplay" build
places it all again; a piece whose class, model or ground isn't there yet is left out with a warning.

What it places, measured from the built level with traces (nothing below is a guessed coordinate it can't check):
- The Lamplighter (ALamplighterShip, lane FLIGHT): its Course from about 350 m west over the plains past Gravewind
  Canyon's far side, down into the canyon and up toward the Mooring Ledge, every segment swept for the hull and raised
  where it would touch rock; GrappleStop, the Course index about 120 m short of the ledge over open air; Moored, the hull
  laid on the ledge (found on a traced height grid: the bow on the ledge, the stern out over the drop, clear of the cliff
  face, the cliff stairs' foot, the gang's mooring post and the seeds' page, letter and locker beside it). In the editor
  she stands Moored (the ledge's look in the tours); FLIGHT's code puts her at Course[0] for a new game.
- The anchor winch (ALandfallWinch, tagged Landfall_AnchorWinch) on the ledge by the bow, on the cliff side.
- LedgeClimbers (AAmbushSpawner, entrance Rise): four Unpaid rising at the ledge's canyon and notch edges during the
  anchor step, away from the hull, the post and the stairs.
- The spyglass (ALandfallSpyglass, tagged Landfall_Spyglass) on the lookout's deck, at the spot with the clearest line
  to the barge (the hut blocks the rest), facing it.
- Vane's dredge barge (stand-in) over the Sink, 45 m over its rim, broadside to the lookout: SM_Skiff_A_Gang scaled to
  63 m in iron, three smokestacks, furnace vents glowing, a hose drum under the keel (one AStoryProp tagged
  Landfall_Barge, its parts baked into one mesh); the hose (cylinder sections and collars chained down in a sag to an
  intake on the Sink's floor, baked into one mesh); soul-smog plumes (SM_SmokePlume in MI_Landfall_SoulSmog, a faint
  cyan glint in one) from the stacks and the intake; the barge's drone (AAmbientEmitter, Ambience.Smog). All shown
  until Main 6 is done (BARGE_UNTIL).
- The Smog lighting state's rule: joins build_area_deck's AStoryLighting (after its Dusk rules), Smog before Main 6 is
  done; the state itself is layout.json level.environment.states.Smog (build_area.py RansomsRest environment).
- Abel's trail: gold-white Gravewind wisps (MI_Landfall_AbelTrail) from the lookout's foot down the bluff path, the
  bluff approach, through the family plot past Abel's grave and down to Delia's porch steps, flowing toward the farm
  (one AStoryProp baked into one mesh, shown during Main 1 from the trail's step).
- Abel's hat (an AReadableProp tagged Landfall_AbelsHat, story_texts.READABLES/READABLE_MODELS 'AbelsHat': a stand-in
  hat until the Tripo one) on the bluff path at its bend, inner side; taking it (a hold) sets the flag Landfall.AbelsHat
  (Delia's best line), and it then hangs on her rocking chair (an AStoryProp).
- FarmPorch (AEncounterSpawner): three Dredger riflemen, two hose-men and three Unpaid in two waves, coming at the porch
  from the farm road and the orchard as the player comes within 45 m, during Main 1's trail and porch steps.
- Company loudspeaker posts (stand-ins: a tall post, two brass horns, a black plate; instanced) on Ransom's Point and on
  Main Street, each with a TargetPoint tagged Landfall_Loudspeaker at its horns (for UVaneBroadcast's hiss).
Hob's Landfall perches go back to build_area_story.place_hob: on the winch (the anchor step) and on the porch beam
(the porch); the lookout's rail and Abel's headboard are build_area_story's own spots.

The names other lanes own are in NAMES and SHIP below, in one place: Main checks them against the C++ when it lands.
The stand-in meshes are in STAND_INS, one path each (the user's Tripo models replace them: TRIPO_MODELS.md).
Materials: Tools/Unreal/build_landfall_materials.py (run it first). Without them the pieces wear their meshes' own.

On its own, in the open editor (clears Gameplay/Landfall, places, saves; Hob's perches and Delia need the whole gameplay
pass): Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_landfall.py RansomsRest"
Read-only plan and measurements: Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Saved/landfall_probe.py"
"""
import importlib
import json
import math
import os
import random
import sys
import zlib

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Landfall'
MAIN1, MAIN6 = 'Main1', 'Main6'

# ---------------------------------------------------------------------------
# Names other lanes own (Main: confirm each against the C++ and the mission data; one place)
# ---------------------------------------------------------------------------

NAMES = dict(
    # FLIGHT: the ship and its properties (Python names of UPROPERTY Course, GrappleStop, Moored).
    ship_class='LamplighterShip', ship_course='course', ship_grapple_stop='grapple_stop', ship_moored='moored',
    # Tags the ship carries: StationBoard::FirstArrivalLanding()'s landing tag, and its own name. Set here because a
    # script's tags replace the C++ defaults on the placed actor.
    ship_tags=('Landing_Lamplighter', 'Lamplighter'),
    winch_class='LandfallWinch', winch_tag='Landfall_AnchorWinch',
    spyglass_class='LandfallSpyglass', spyglass_tag='Landfall_Spyglass',
    barge_tag='Landfall_Barge', loudspeaker_tag='Landfall_Loudspeaker',
    # DREDGER and the Unpaid (spawners name these classes).
    rifleman_class='DredgerRifleman', hoseman_class='DredgerHoseMan', unpaid_class='UnpaidCreature',
    # The contract's spawner ids and Abel's hat.
    ledge_spawner='LedgeClimbers', porch_spawner='FarmPorch', hat_tag='Landfall_AbelsHat',
    # STORY's readable id for the hat (story_texts.READABLES and READABLE_MODELS key).
    hat_readable='AbelsHat',
    # The flag taking the hat sets (STORY picks Delia's variant line with it: with_flags in her turn-in topic).
    hat_flag='Landfall.AbelsHat',
)

# FLIGHT's ALamplighterShip as this script assumes it (cm; confirm in the C++): the pivot under the hull's middle at the
# keel (the skiff's), the bow along the actor's +X, the bag's top this far over the pivot, the hull this long and wide.
# The user's Tripo Lamplighter (SM_Lamplighter at her real size, measured in Blender: lanes/IMPORT.md): 31 m from the bag's
# nose to its tail (the hull 28 m), 10.5 m in the beam (the bag; the hull's propeller mounts 10.2 m), the main deck 4.22 m
# over the keel's middle (FLIGHT's ALamplighterShip::DeckTop), the bag's top 20.8 m over it; bow +X. (The stand-in was
# length 3240, beam 1130, top 980, deck 56.)
SHIP = dict(length=3100.0, beam=1054.0, keel_below_pivot=0.0, top_over_pivot=2077.0, deck_over_pivot=422.0,
            forward_yaw=0.0)

# Main 1's steps (from 0, the contract's table), used when the mission asset can't say (Steps finds them in it first).
STEPS = dict(anchor=4, point=5, spyglass=6, trail=7, porch=8)

# The barge, its hose and smog are there until this mission is done (the Rot lifts later in the story).
BARGE_UNTIL = MAIN6

# ---------------------------------------------------------------------------
# Stand-ins (one path each: the Tripo models replace them; TRIPO_MODELS.md numbers in brackets)
# ---------------------------------------------------------------------------

# A path under TRIPO_FOLDER is one of the user's Tripo models (lanes/IMPORT.md): real size, its own materials, and what the
# stand-in added in shapes is modelled on it (the barge's stacks, vents and drum, the hose's collars, the post's horns and
# plate), so those shapes are left out; put the stand-in's path back to go back.
TRIPO_FOLDER = '/Game/Art/Tripo/'
STAND_INS = dict(
    barge_hull='/Game/Art/Tripo/SM_DredgeBarge',               # [12] Vane's dredge barge (stand-in SM_Skiff_A_Gang)
    barge_stack='/Engine/BasicShapes/Cylinder',
    barge_vent='/Engine/BasicShapes/Cube',
    barge_drum='/Engine/BasicShapes/Cylinder',
    hose_section='/Game/Art/Tripo/SM_DredgeHose',               # [13] dredge hose section (stand-in the engine Cylinder)
    hose_collar='/Engine/BasicShapes/Cylinder',
    hose_intake='/Game/Art/Tripo/SM_HoseIntake',                # [13] its iron intake mouth (stand-in the engine Cone)
    smog_plume='/Game/Art/Props/SM_SmokePlume',
    trail_wisps=('/Game/Art/Props/SM_GravewindWisp_B', '/Game/Art/Props/SM_GravewindWisp_C',
                 '/Game/Art/Props/SM_GravewindWisp_A'),
    # [19] Abel's hat: only a fallback; story_texts.READABLE_MODELS['AbelsHat'] names the hat (under HAT_MODELS). Not the
    # ghost Abel's SM_AbelHat: its vertex colours are M_Ghost's zones (Abel.py), a ghost's hat on a sunlit path.
    abel_hat='/Game/Art/Props/SM_SheriffHat',
    speaker_post='/Game/Art/Tripo/SM_LoudspeakerPost',          # [18] company loudspeaker post (stand-in SM_FencePost)
    speaker_horn='/Engine/BasicShapes/Cone',
    speaker_plate='/Engine/BasicShapes/Cube',
)
# Where story_texts.READABLE_MODELS' names live (build_area_secrets' readables are found there too).
HAT_MODELS = '/Game/Art/Props/'
MATERIALS = dict(
    iron='/Game/Art/Landfall/Materials/MI_Landfall_BargeIron',
    glow='/Game/Art/Materials/MI_CinderGlow',
    rubber='/Game/Art/Landfall/Materials/MI_Landfall_HoseRubber',
    smog='/Game/Art/Landfall/Materials/MI_Landfall_SoulSmog',
    glint='/Game/Art/Landfall/Materials/MI_Landfall_SoulSmogGlint',
    trail='/Game/Art/Landfall/Materials/MI_Landfall_AbelTrail',
    brass='/Game/Art/Materials/MI_BrassWorn',
    plate='/Game/Art/Materials/MI_PaintBlack',
)
# Baked meshes (the barge, the hose, the trail): named by a hash of their parts, so a build with the same level reuses
# the asset and a changed one makes a new asset (delete the unused ones now and then: Main's README note).
BAKED = '/Game/Art/Landfall/Baked'

# ---------------------------------------------------------------------------
# Numbers (cm, degrees)
# ---------------------------------------------------------------------------

# The ledge's height grid: a box round the mooring post, traced every LEDGE_STEP.
LEDGE_BOX = (-1100.0, 900.0, -2000.0, 2300.0)   # X min/max, Y min/max off the post (the ledge is about 9 x 25 m)
LEDGE_STEP = 50.0
LEDGE_BAND = 120.0        # ground within this of the post's foot is the ledge
ROCK_OVER = 150.0         # anything higher than this over the ledge is rock (the face, the Tumble, the post itself)
DROP_UNDER = 400.0        # nothing within this under the ledge: the drop
# The hull on the ledge: its bow third all on ground, at least this share of it on ground, the rest over the drop.
BOW_SHARE, MIN_ON_LEDGE = 0.35, 0.4
HULL_MARGIN = 30.0
ARRIVAL_YAW = 90.0        # she comes in heading east, out of the canyon
MOORED_TILT = (2.5, 1.5)  # roll, pitch of a hard landing (degrees; small: the deck stays walkable)
# What the hull keeps clear of: the post and the seeds' page, letter and locker beside it (build_area_seeds LEDGE_*,
# build_area_secrets ledge_spot: all within 2.7 m of the post), and the cliff stairs' foot (its bottom landing).
POST_CLEAR = 420.0
STAIRS_FOOT_OUT = 160.0   # the stairs' bottom landing stands this far out from the face (Lookout.py CliffStairs)
STAIRS_CLEAR = 300.0
# The winch: on ground beside the bow, on the cliff side, this far off the hull.
WINCH_OFF = 260.0
WINCH_TOP = 125.0         # Hob stands on it (FLIGHT's winch is about 1.2 m)
# The course (FLIGHT moves the ship along it): control points by distance west of the ledge, densified every STEP.
COURSE_START = 35000.0    # about 350 m out
COURSE_LOW = 3500.0       # over the plains
COURSE_CANYON = 19000.0   # down in the canyon, under the Rim
COURSE_UNDER_RIM = 2500.0
GRAPPLE_BACK = 12000.0    # the tender stops her about 120 m short of the ledge
GRAPPLE_OVER = 800.0      # over the ledge's height
APPROACH_BACK, APPROACH_OVER = 4000.0, 1200.0   # the landing's glide (FLIGHT may use it or its own)
COURSE_STEP = 1500.0
COURSE_CLEAR = 600.0      # under the keel, between the points
COURSE_RAISE, COURSE_TRIES = 500.0, 14
# The barge: over the Sink, broadside to the lookout.
BARGE_SCALE = 7.0         # the 9 m skiff to 63 m
BARGE_OVER_RIM = 4500.0
BARGE_VENTS = 6           # glowing vents a side
# The skiff's own measures (Art/Models/Vehicles/Skiff.py, cm): its deck over the keel, its widest gunwale's half-breadth,
# and where along its side's height the vents go.
SKIFF_DECK, SKIFF_HALF_BEAM, SKIFF_VENT_Z = 43.0, 118.0, 90.0
STACK_SCALE = (3.0, 3.0)  # smokestacks' girth (BasicShapes cylinder: 1 m)
STACK_OVER_BAG = 600.0
DRUM_SCALE = 5.0
# The hose: 1.5 m across, sections of about 10 m, a sag sideways, its intake off the floor's middle toward the lookout.
HOSE_GIRTH, HOSE_SECTION, HOSE_SAG = 1.5, 1000.0, 600.0
COLLAR_GIRTH, COLLAR_LONG = 1.9, 0.35
INTAKE_OFF, INTAKE_SCALE = 450.0, (3.4, 3.4, 2.2)
# Smog plumes (SM_SmokePlume is 8 m tall): from each stack's top, and the intake on the floor.
STACK_PLUME, INTAKE_PLUME, GLINT_PLUME = 4.0, 2.2, 2.8
WIND_YAW = 19.0           # build_area.py's: the plumes lean downwind
# Abel's trail.
TRAIL_STEP = 340.0
TRAIL_LIFT, TRAIL_PITCH = 110.0, 22.0   # the ribbons fall toward their tails: lifted and pitched to lie over the ground
TRAIL_SCALE = (0.55, 0.75)
TRAIL_TURN = 8.0
TRAIL_SIDE = 60.0
TRAIL_FROM_LOOKOUT = 520.0
HOUSE_MARGIN = 160.0      # the trail keeps this far off the farmhouse's walls and porch
TRAIL_FALLBACK_MOST, TRAIL_FALLBACK_DRAW = 30, 6000.0
SEED = 1031
# FarmPorch: spawns this far from the porch (cm), the dredgers along the farm road, the dead in the orchard; inside the
# 26 m level zone round the porch Main is asked to add (lanes/WORLD.md: levels 1-2, clear of the OrchardEdge camp's spots).
PORCH_RING = (1500.0, 2300.0)
PORCH_ACTIVATION, PORCH_GIVE_UP = 4500.0, 6000.0
PORCH_WAVES = ((NAMES['rifleman_class'], 2, 1), (NAMES['unpaid_class'], 3, 1),
               (NAMES['rifleman_class'], 1, 2), (NAMES['hoseman_class'], 2, 2))
PORCH_MAX_ALIVE, PORCH_BREATH = 5, 2.5
# The porch's front edge and steps in the farmhouse's frame (Farmhouse.py: front +X, the porch to 5.3 m, steps to 5.9 m).
PORCH_FRONT = 620.0
# LedgeClimbers.
LEDGE_CLIMBERS, LEDGE_ACTIVATION, LEDGE_EDGE_KEEP, LEDGE_GIVE_UP = 4, 3500.0, 250.0, 3000.0
# Abel's hat on the bluff path's bend, this far in from its middle toward the bluff (the path is 4 m wide).
HAT_IN = 110.0
HAT_CHAIR = (410.0, 250.0, (85.0, 125.0))   # Delia's rocking chair in the house's frame (Farmhouse.py CHAIR), its seat
# Loudspeaker posts: the post stretched to this tall, the horns' size, the plate's.
SPEAKER_TALL = 420.0
HORN_SCALE = (0.55, 0.55, 0.8)
PLATE_SCALE = (0.04, 0.34, 0.24)
SPEAKER_CULL = 15000.0
POINT_SPEAKER_RING = (650.0, 1100.0)
# The lookout's deck: its walking surface this far over the lookout's pivot (Lookout.py FLOOR 6.5 m).
DECK_UP = (560.0, 760.0)
TOWN_SPEAKERS = ((700.0, 900.0), (-700.0, 2600.0), (700.0, 4300.0))   # Main Street, off its middle line (X, Y)
TOWN_MIDDLE = (0.0, 2600.0)   # the Point's post turns its horns toward the town

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def world():
    return unreal.EditorLevelLibrary.get_editor_world()


def load(path):
    return unreal.load_asset(path) if path and unreal.EditorAssetLibrary.does_asset_exist(path) else None


def material(key):
    return load(MATERIALS[key])


def flat(d):
    length = math.hypot(d[0], d[1]) or 1.0
    return d[0] / length, d[1] / length


def yaw_of(d):
    return math.degrees(math.atan2(d[1], d[0]))


def trace(x, y, top, bottom, ignore=()):
    """The first thing between (x, y, top) and (x, y, bottom): (z, actor), or None."""
    hit = unreal.SystemLibrary.line_trace_single(world(), unreal.Vector(x, y, top), unreal.Vector(x, y, bottom),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, list(ignore),
                                                 unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        return None
    t = hit.to_tuple()
    return t[5].z, t[9]


def blocked_line(a, b, ignore=()):
    """Whether anything stands on the line from a to b ((x, y, z) each)."""
    hit = unreal.SystemLibrary.line_trace_single(world(), unreal.Vector(*a), unreal.Vector(*b),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, list(ignore),
                                                 unreal.DrawDebugTrace.NONE, True)
    return hit is not None


def ground_z(x, y, default=0.0):
    return story.ground_at(x, y, default)


def solid_near(x, y, z, radius, ignore_labels=()):
    """A solid actor's label at (x, y, z) within radius (the terrain, the scatter's volume and triggers don't count)."""
    try:
        found = unreal.SystemLibrary.sphere_overlap_actors(world(), unreal.Vector(x, y, z), radius,
                                                           [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], None, [])
    except Exception as error:   # a failed check never stops the build
        unreal.log_warning(f'landfall: ({x:.0f}, {y:.0f}) could not be checked for solids ({error})')
        return None
    hits = found[1] if isinstance(found, tuple) else found
    for actor in hits or []:
        if actor is None or unreal.Name('Ground') in actor.tags or unreal.Name('Beyond') in actor.tags:
            continue
        if isinstance(actor, (unreal.PCGVolume, unreal.TriggerBox)):
            continue
        label = str(actor.get_actor_label())
        if label in ignore_labels:
            continue
        return label
    return None


def road_distance(build, x, y):
    """How far (x, y) is from the nearest road's edge (negative: on it)."""
    best = 1e9
    for road in build.source.get('roads', []):
        path, half = road.get('path', []), road.get('width', 300.0) * 0.5
        for (x0, y0), (x1, y1) in zip(path, path[1:]):
            dx, dy = x1 - x0, y1 - y0
            t = max(0.0, min(1.0, ((x - x0) * dx + (y - y0) * dy) / max(dx * dx + dy * dy, 1.0)))
            best = min(best, math.hypot(x - x0 - dx * t, y - y0 - dy * t) - half)
    return best


def polyline_points(path, step):
    """Points every step along a polyline ((x, y) pairs), with the direction there."""
    out = []
    for (x0, y0), (x1, y1) in zip(path, path[1:]):
        length = math.hypot(x1 - x0, y1 - y0)
        if length < 1.0:
            continue
        d = ((x1 - x0) / length, (y1 - y0) / length)
        count = max(1, int(length // step))
        for k in range(count):
            t = k / count
            out.append((x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, d))
    if path:
        last = path[-1]
        d = out[-1][2] if out else (1.0, 0.0)
        out.append((last[0], last[1], d))
    return out


def frange(start, stop, step):
    out, v = [], start
    while v <= stop:
        out.append(v)
        v += step
    return out


def inside(point, polygon):
    x, y = point
    result = False
    for (x0, y0), (x1, y1) in zip(polygon, polygon[1:] + polygon[:1]):
        if (y0 > y) != (y1 > y) and x < (x1 - x0) * (y - y0) / ((y1 - y0) or 1e-9) + x0:
            result = not result
    return result


def hull_2d(points):
    """The convex hull of (x, y) points (monotone chain), in order."""
    pts = sorted(set(points))
    if len(pts) < 3:
        return pts

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def rect_distance(px, py, bow, d, length, beam):
    """How far (px, py) is from the hull's footprint (its bow point, heading d, length back from it, beam across)."""
    rx, ry = px - bow[0], py - bow[1]
    along = -(rx * d[0] + ry * d[1])          # back from the bow
    across = rx * -d[1] + ry * d[0]
    da = max(0.0, -along, along - length)
    dc = max(0.0, abs(across) - beam * 0.5)
    return math.hypot(da, dc)


def transform(location, rotation=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0)):
    """unreal.Transform from (x, y, z), (roll, pitch, yaw) or a Rotator, and a scale."""
    rot = rotation if isinstance(rotation, unreal.Rotator) else unreal.Rotator(roll=rotation[0], pitch=rotation[1],
                                                                               yaw=rotation[2])
    return unreal.Transform(location=unreal.Vector(*location), rotation=rot, scale=unreal.Vector(*scale))


def set_or_warn(build, actor, prop, value, what):
    try:
        actor.set_editor_property(prop, value)
        return True
    except Exception as error:
        build.warn(f'{what}: no property {prop} ({error}): check NAMES in build_area_landfall.py against the C++')
        return False


# ---------------------------------------------------------------------------
# Baking several meshes into one (fewer draws: the barge, the hose, the trail)
# ---------------------------------------------------------------------------

class Part:
    def __init__(self, mesh, xform, materials=()):
        self.mesh, self.xform, self.materials = mesh, xform, list(materials)

    def signature(self):
        t, r, s = self.xform.translation, self.xform.rotation.rotator(), self.xform.scale3d
        mats = ','.join(m.get_path_name() if m else '-' for m in self.materials)
        return (f'{self.mesh.get_path_name()}|{t.x:.0f},{t.y:.0f},{t.z:.0f}|{r.roll:.1f},{r.pitch:.1f},{r.yaw:.1f}|'
                f'{s.x:.3f},{s.y:.3f},{s.z:.3f}|{mats}')


def merge_settings():
    options = unreal.MergeStaticMeshActorsOptions()
    settings = options.get_editor_property('mesh_merging_settings')
    for prop, value in (('pivot_point_at_zero', False), ('merge_materials', False), ('merge_physics_data', False),
                        ('generate_light_map_uv', False), ('merge_equivalent_materials', True),
                        ('allow_distance_field', False)):
        try:
            settings.set_editor_property(prop, value)
        except Exception:
            pass
    options.set_editor_property('mesh_merging_settings', settings)
    options.set_editor_property('destroy_source_actors', True)
    options.set_editor_property('spawn_merged_actor', False)
    return options


# Off (2026-11-01): in 5.8 the merge made no asset and its temporary actors destroyed the barge's placed hull and stacks
# ("DestroyActors. An actor to destroy is not part of the world editor"); the parts are placed one by one instead.
BAKE = False


def bake(build, name, parts, check_vertex_data=False):
    """One mesh asset made of parts (their meshes at their transforms, wearing their materials), its pivot the first
    part's place; reused when a mesh of the same parts exists. None when the editor can't merge (the caller places the
    parts one by one instead)."""
    if not parts:
        return None
    if not BAKE:
        return None
    digest = format(zlib.crc32('\n'.join(p.signature() for p in parts).encode()) & 0xffffffff, '08x')
    path = f'{BAKED}/SM_Landfall_{name}_{digest}'
    try:
        subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    except Exception as error:
        build.warn(f'{name}: no StaticMeshEditorSubsystem ({error}): placed one by one')
        return None
    made_now = False
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        temps = []
        try:
            for part in parts:
                t = part.xform
                temp = actors.spawn_actor_from_object(part.mesh, t.translation, t.rotation.rotator())
                temp.set_actor_scale3d(t.scale3d)
                component = temp.static_mesh_component
                for slot, worn in enumerate(part.materials):
                    if worn is not None:
                        component.set_material(slot, worn)
                temps.append(temp)
            options = merge_settings()
            options.set_editor_property('base_package_name', path)
            subsystem.merge_static_mesh_actors(temps, options)
        except Exception as error:
            build.warn(f'{name}: the editor could not bake its {len(parts)} parts into one mesh ({error}): placed one by '
                       f'one')
            return None
        finally:
            for temp in temps:
                try:
                    actors.destroy_actor(temp)
                except Exception:
                    pass
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            build.warn(f'{name}: the merge made no {path}: placed one by one')
            return None
        made_now = True
    baked = unreal.load_asset(path)
    if check_vertex_data:
        # The wisps' material reads their vertex alpha and their second UV set: a merge that dropped either would show
        # hard-edged cards, so it's deleted and the wisps stand one by one.
        try:
            colours = subsystem.has_vertex_colors(baked)
            channels = subsystem.get_num_uv_channels(baked, 0)
        except Exception:
            colours, channels = True, 2
        if not colours or channels < 2:
            build.warn(f'{name}: the merged mesh lost its vertex colours or second UVs ({colours}, {channels} UV sets): '
                       f'placed one by one')
            try:
                unreal.EditorAssetLibrary.delete_asset(path)
            except Exception:
                pass
            return None
    if made_now:
        unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
        build.log(f'{name}: {len(parts)} parts baked into {path}')
    return baked


def story_prop(build, model, xform, label, shown=(), hidden=(), tags=(), materials=(), cast_shadow=False,
               draw_distance=0.0):
    """An AStoryProp (World/StoryProp.h): seen only, shown while any of shown holds and none of hidden."""
    cls = story.actor_class('StoryProp')
    if cls is None or model is None:
        if cls is None:
            build.warn(f'no StoryProp class (build the game module first): no {label}')
        return None
    at = xform.translation
    prop = build.place(cls, (at.x, at.y, at.z), 0.0, label=label, folder=FOLDER, tags=tags)
    prop.set_actor_rotation(xform.rotation.rotator(), False)
    prop.set_actor_scale3d(xform.scale3d)
    mesh = prop.get_editor_property('mesh')
    mesh.set_static_mesh(model)
    for slot, worn in enumerate(materials):
        if worn is not None:
            mesh.set_material(slot, worn)
    mesh.set_editor_property('cast_shadow', cast_shadow)
    if draw_distance:
        mesh.set_editor_property('ld_max_draw_distance', float(draw_distance))
    prop.set_editor_property('shown_when', list(shown))
    prop.set_editor_property('hidden_when', list(hidden))
    return prop


def baked_or_parts(build, name, parts, label, shown, tags=(), cast_shadow=False, draw_distance=0.0, check=False,
                   fallback_most=0):
    """The parts as one baked AStoryProp, or (no merge) each its own, at most fallback_most of them (0: all)."""
    baked = bake(build, name, parts, check_vertex_data=check)
    if baked is not None:
        # Its pivot is the first part's place (the merge's, without pivot_point_at_zero). Never distance-culled: its
        # bounds span the whole piece (draw_distance is for the parts standing alone).
        at = parts[0].xform.translation
        return [story_prop(build, baked, transform((at.x, at.y, at.z)), label, shown=shown, tags=tags,
                           cast_shadow=cast_shadow)]
    chosen = parts if not fallback_most or len(parts) <= fallback_most else \
        [parts[int(i * len(parts) / fallback_most)] for i in range(fallback_most)]
    made = []
    for i, part in enumerate(chosen):
        made.append(story_prop(build, part.mesh, part.xform, f'{label}_{i + 1:02d}', shown=shown,
                               tags=tags if i == 0 else (), materials=part.materials, cast_shadow=cast_shadow,
                               draw_distance=draw_distance))
    return made


# ---------------------------------------------------------------------------
# The Mooring Ledge: its height grid, the hull's place, the winch, the climbers' spots
# ---------------------------------------------------------------------------

class Ledge:
    """The ledge round the gang's mooring post, traced on a grid: each cell ground, drop or rock."""

    def __init__(self, build, post, stairs):
        self.build = build
        at = post.get_actor_location()
        self.post = (at.x, at.y)
        # The post stands on the ledge: its foot is the ledge's height (its top is no ground).
        self.z = at.z
        self.cells = {}
        x0, x1, y0, y1 = LEDGE_BOX
        self.nx, self.ny = int((x1 - x0) // LEDGE_STEP) + 1, int((y1 - y0) // LEDGE_STEP) + 1
        self.x0, self.y0 = at.x + x0, at.y + y0
        for i in range(self.nx):
            for j in range(self.ny):
                x, y = self.x0 + i * LEDGE_STEP, self.y0 + j * LEDGE_STEP
                hit = trace(x, y, self.z + 6000.0, self.z - 6000.0)
                self.cells[(i, j)] = None if hit is None else hit[0]
        # The cliff stairs' foot: out from the face (the stairs' +X) at the bottom landing.
        self.stairs_foot = None
        if stairs is not None:
            s = stairs.get_actor_location()
            out = stairs.get_actor_forward_vector()
            self.stairs_foot = (s.x + out.x * STAIRS_FOOT_OUT, s.y + out.y * STAIRS_FOOT_OUT)
        self.keep_out = [(self.post[0], self.post[1], POST_CLEAR)]
        if self.stairs_foot:
            self.keep_out.append((self.stairs_foot[0], self.stairs_foot[1], STAIRS_CLEAR))

    def kind_at(self, x, y):
        i, j = int(round((x - self.x0) / LEDGE_STEP)), int(round((y - self.y0) / LEDGE_STEP))
        if (i, j) not in self.cells:
            return 'drop'
        z = self.cells[(i, j)]
        if z is None or z < self.z - DROP_UNDER:
            return 'drop'
        if z > self.z + ROCK_OVER:
            return 'rock'
        if abs(z - self.z) <= LEDGE_BAND:
            return 'ground'
        return 'rough'

    def z_at(self, x, y):
        i, j = int(round((x - self.x0) / LEDGE_STEP)), int(round((y - self.y0) / LEDGE_STEP))
        z = self.cells.get((i, j))
        return self.z if z is None else z

    def ground_cells(self):
        return [(self.x0 + i * LEDGE_STEP, self.y0 + j * LEDGE_STEP) for (i, j), z in self.cells.items()
                if z is not None and abs(z - self.z) <= LEDGE_BAND]

    def counts(self):
        kinds = {}
        for (i, j) in self.cells:
            kind = self.kind_at(self.x0 + i * LEDGE_STEP, self.y0 + j * LEDGE_STEP)
            kinds[kind] = kinds.get(kind, 0) + 1
        return kinds

    def hull_fit(self, bow, d, tier):
        """How the hull lies with its bow at bow heading d, or None if it can't. tier 'on': its bow third on the ledge
        (BOW_SHARE, 90% ground) and at least MIN_ON_LEDGE of it, the rest out over the drop; 'relaxed': 60% and three
        quarters of that; 'alongside': all of it over the drop beside the ledge, its deck level with it, one side within
        1.5 m of ground along at least 35% of its length (an airship moored at a quay: the winch's line holds her).
        Returns (share on ground, ground z's under the bow, the ledge's side: +1 left of the heading, -1 right) or None."""
        length, beam = SHIP['length'], SHIP['beam'] + HULL_MARGIN * 2.0
        for cx, cy, r in self.keep_out:
            if rect_distance(cx, cy, bow, d, length, beam) < r:
                return None
        ground = total = 0
        bow_ground = bow_total = 0
        bow_z = []
        across = (-d[1], d[0])
        steps_along, steps_across = int(length // 150.0) + 1, int(beam // 160.0) + 1
        for a in range(steps_along):
            s = length * a / (steps_along - 1)
            for c in range(steps_across):
                t = -beam * 0.5 + beam * c / (steps_across - 1)
                x, y = bow[0] - d[0] * s + across[0] * t, bow[1] - d[1] * s + across[1] * t
                kind = self.kind_at(x, y)
                if kind == 'rock' or (tier == 'alongside' and kind != 'drop'):
                    return None
                total += 1
                in_bow = s <= length * BOW_SHARE
                if kind == 'ground':
                    ground += 1
                    if in_bow:
                        bow_z.append(self.z_at(x, y))
                if in_bow:
                    bow_total += 1
                    bow_ground += kind == 'ground'
        if tier == 'alongside':
            best_side, best_share = 0.0, 0.0
            for side in (1.0, -1.0):
                near = 0
                for a in range(steps_along):
                    s = length * a / (steps_along - 1)
                    t = side * (beam * 0.5 + 75.0)
                    near += self.kind_at(bow[0] - d[0] * s + across[0] * t, bow[1] - d[1] * s + across[1] * t) == 'ground'
                if near / steps_along > best_share:
                    best_side, best_share = side, near / steps_along
            if best_share < 0.35:
                return None
            return best_share, [], best_side
        need_bow = 0.6 if tier == 'relaxed' else 0.9
        need_all = MIN_ON_LEDGE * (0.75 if tier == 'relaxed' else 1.0)
        if bow_total == 0 or bow_ground / bow_total < need_bow or ground / total < need_all:
            return None
        return ground / total, bow_z, 0.0

    def moor(self):
        """The hull's place: (bow (x, y), heading (dx, dy), keel z, score, share, tier, ledge side), the best found in the
        first tier that has one, or None."""
        cells = self.ground_cells()
        if not cells:
            return None
        xs, ys = [c[0] for c in cells], [c[1] for c in cells]
        for tier in ('on', 'relaxed', 'alongside'):
            best = None
            if tier == 'alongside':
                # Bows over the drop round the ledge, out to a hull's beam past its ground.
                pad = SHIP['beam'] + 200.0
                seeds = [(x, y) for x in frange(min(xs) - pad, max(xs) + pad, 100.0)
                         for y in frange(min(ys) - pad, max(ys) + pad, 100.0) if self.kind_at(x, y) == 'drop']
            else:
                seeds = cells[::2]
            for bx, by in seeds:
                for k in range(48):
                    yaw = k * 7.5
                    d = (math.cos(math.radians(yaw)), math.sin(math.radians(yaw)))
                    fit = self.hull_fit((bx, by), d, tier)
                    if fit is None:
                        continue
                    share, bow_z, side = fit
                    turn = math.cos(math.radians(yaw - ARRIVAL_YAW))
                    near = 0.0
                    if self.stairs_foot:
                        mid = (bx - d[0] * SHIP['length'] * 0.5, by - d[1] * SHIP['length'] * 0.5)
                        near = 1.0 - min(math.dist(mid, self.stairs_foot) / 2500.0, 1.0)
                    score = share + 0.6 * turn + 0.3 * near
                    if best is None or score > best[3]:
                        if tier == 'alongside':
                            keel = self.z - SHIP['deck_over_pivot'] + 10.0 - SHIP['keel_below_pivot']
                        else:
                            keel = sorted(bow_z)[len(bow_z) // 2] if bow_z else self.z
                        best = ((bx, by), d, keel, score, share, tier, side)
            if best is not None:
                return best
        return None

    def spot_near(self, around, rings, clear_of=(), keep=150.0):
        """A ground cell near around, on rings (cm) round it, clear of the keep-outs and of each (bow, d) hull."""
        for radius in rings:
            for k in range(16 if radius else 1):
                angle = 2.0 * math.pi * k / 16.0
                x, y = around[0] + math.cos(angle) * radius, around[1] + math.sin(angle) * radius
                if self.kind_at(x, y) != 'ground':
                    continue
                if any(math.dist((x, y), (cx, cy)) < r * 0.6 for cx, cy, r in self.keep_out):
                    continue
                if any(rect_distance(x, y, bow, d, SHIP['length'], SHIP['beam']) < keep for bow, d in clear_of):
                    continue
                return x, y, self.z_at(x, y)
        return None

    def edge_spots(self, count, hull):
        """count spots on ground at the ledge's drop edges (where the dead climb up), spread apart, off the hull and the
        keep-outs."""
        edge = []
        for (i, j), z in self.cells.items():
            x, y = self.x0 + i * LEDGE_STEP, self.y0 + j * LEDGE_STEP
            if self.kind_at(x, y) != 'ground':
                continue
            near_drop = any(self.kind_at(x + dx * LEDGE_STEP, y + dy * LEDGE_STEP) == 'drop'
                            for dx in (-2, 0, 2) for dy in (-2, 0, 2))
            if not near_drop:
                continue
            if hull and rect_distance(x, y, hull[0], hull[1], SHIP['length'], SHIP['beam']) < LEDGE_EDGE_KEEP:
                continue
            if any(math.dist((x, y), (cx, cy)) < r for cx, cy, r in self.keep_out):
                continue
            if not inside_boundary(x, y):
                continue
            edge.append((x, y, z))
        if not edge:
            # The drop's edge lies on the boundary here: they rise out of the ledge's open ground instead (still the
            # dead coming for the ship), anywhere inside the boundary off the hull and the keep-outs.
            for (i, j), z in self.cells.items():
                x, y = self.x0 + i * LEDGE_STEP, self.y0 + j * LEDGE_STEP
                if self.kind_at(x, y) != 'ground' or not inside_boundary(x, y):
                    continue
                if hull and rect_distance(x, y, hull[0], hull[1], SHIP['length'], SHIP['beam']) < LEDGE_EDGE_KEEP:
                    continue
                if any(math.dist((x, y), (cx, cy)) < r for cx, cy, r in self.keep_out):
                    continue
                edge.append((x, y, z))
        if not edge:
            return []
        far = max(edge, key=lambda p: math.dist(p[:2], self.stairs_foot or self.post))
        chosen = [far]
        while len(chosen) < count and len(chosen) < len(edge):
            chosen.append(max(edge, key=lambda p: min(math.dist(p[:2], c[:2]) for c in chosen)))
        return chosen


# The playable boundary's corners (layout.json boundary), set as the plan is made; spots must lie inside it, this far in.
BOUNDARY = []
BOUNDARY_KEEP = 60.0


def inside_boundary(x, y):
    """Whether (x, y) lies inside the playable boundary and BOUNDARY_KEEP in from each of its edges (none known: yes)."""
    if len(BOUNDARY) < 3:
        return True
    inside = False
    n = len(BOUNDARY)
    for k in range(n):
        (ax, ay), (bx, by) = BOUNDARY[k][:2], BOUNDARY[(k + 1) % n][:2]
        if (ay > y) != (by > y) and x < ax + (y - ay) * (bx - ax) / (by - ay):
            inside = not inside
    if not inside:
        return False
    for k in range(n):
        (ax, ay), (bx, by) = BOUNDARY[k][:2], BOUNDARY[(k + 1) % n][:2]
        dx, dy = bx - ax, by - ay
        t = max(0.0, min(1.0, ((x - ax) * dx + (y - ay) * dy) / max(dx * dx + dy * dy, 1e-6)))
        if math.dist((x, y), (ax + t * dx, ay + t * dy)) < BOUNDARY_KEEP:
            return False
    return True


# ---------------------------------------------------------------------------
# The plan (measured, nothing placed): the probe prints it
# ---------------------------------------------------------------------------

class Plan:
    pass


def mission_steps(build):
    """Main 1's steps from its data (the objective asking for each thing), else the contract's (STEPS)."""
    valley = importlib.reload(importlib.import_module('build_area_missionprops_valley'))
    steps = valley.Steps(build)
    return dict(
        anchor=steps.find_any(MAIN1, (('interact', NAMES['winch_tag']), ('clear', NAMES['ledge_spawner'])),
                              STEPS['anchor']),
        point=steps.find(MAIN1, 'reach', story.PLACE_POINT, STEPS['point']),
        spyglass=steps.find(MAIN1, 'interact', NAMES['spyglass_tag'], STEPS['spyglass']),
        trail=steps.find(MAIN1, 'reach', 'Speaker_Delia', STEPS['trail']),
        porch=steps.find(MAIN1, 'clear', NAMES['porch_spawner'], STEPS['porch']),
    )


def plan_moored(build, plan):
    post = story.placed(build, 'MooringPost')
    if post is None:
        return False
    plan.ledge = Ledge(build, post, story.placed(build, 'CliffStairs'))
    found = plan.ledge.moor()
    if found is None:
        build.warn(f'the Mooring Ledge has no room for the Lamplighter\'s hull on it or beside it ({plan.ledge.counts()} '
                   f'cells): she moors at the design\'s spot; check it in the editor')
        bow, d, keel, share, tier, ledge_side = (-10050.0, -9000.0), (0.0, 1.0), plan.ledge.z, 0.0, 'design', 0.0
    else:
        bow, d, keel, _, share, tier, ledge_side = found
    plan.bow, plan.heading, plan.share, plan.tier = bow, d, share, tier
    middle = (bow[0] - d[0] * SHIP['length'] * 0.5, bow[1] - d[1] * SHIP['length'] * 0.5)
    yaw = yaw_of(d) + SHIP['forward_yaw']
    # Laid on the ledge after a hard landing she lists a little; moored alongside she floats level.
    roll, pitch = MOORED_TILT if tier in ('on', 'relaxed') else (0.0, 0.0)
    plan.moored = transform((middle[0], middle[1], keel + SHIP['keel_below_pivot']), (roll, pitch, yaw))
    plan.moored_xyz = (middle[0], middle[1], keel + SHIP['keel_below_pivot'])
    plan.moored_yaw = yaw
    # The winch: beside the bow on the ledge's side (alongside), else the cliff side (nearer the stairs' foot or the
    # post), on ground.
    across = (-d[1], d[0])
    toward = plan.ledge.stairs_foot or plan.ledge.post
    side = ledge_side or (1.0 if (toward[0] - bow[0]) * across[0] + (toward[1] - bow[1]) * across[1] > 0 else -1.0)
    # Her sides for FLIGHT (+1 starboard, across = her right): the gangway faces the ledge; the tender and the slide the drop.
    plan.ledge_side = side
    plan.canyon_side = (-across[0] * side, -across[1] * side) if tier == 'alongside' else None
    off = SHIP['beam'] * 0.5 + WINCH_OFF
    seek = (bow[0] - d[0] * 300.0 + across[0] * side * off, bow[1] - d[1] * 300.0 + across[1] * side * off)
    plan.winch = plan.ledge.spot_near(seek, (0.0, 100.0, 200.0, 300.0, 450.0), clear_of=[(bow, d)])
    if plan.winch is None:
        plan.winch = plan.ledge.spot_near(bow, (400.0, 600.0, 800.0), clear_of=[(bow, d)])
    plan.winch_yaw = yaw_of((middle[0] - plan.winch[0], middle[1] - plan.winch[1])) if plan.winch else yaw
    plan.climbers = plan.ledge.edge_spots(LEDGE_CLIMBERS, (bow, d))
    cells = plan.ledge.ground_cells()
    plan.ledge_corners = hull_2d([(round(x), round(y)) for x, y in cells]) if cells else []
    plan.ledge_middle = (sum(c[0] for c in cells) / len(cells), sum(c[1] for c in cells) / len(cells)) if cells \
        else plan.ledge.post
    return True


def hull_clear(a, b):
    """Whether the hull can sail the segment a -> b (pivots): its keel, bag top and both sides."""
    d = flat((b[0] - a[0], b[1] - a[1]))
    across = (-d[1], d[0])
    half = SHIP['beam'] * 0.5
    offsets = ((0.0, 0.0, -SHIP['keel_below_pivot'] - COURSE_CLEAR), (0.0, 0.0, SHIP['top_over_pivot']),
               (across[0] * half, across[1] * half, SHIP['deck_over_pivot']),
               (-across[0] * half, -across[1] * half, SHIP['deck_over_pivot']))
    for ox, oy, oz in offsets:
        if blocked_line((a[0] + ox, a[1] + oy, a[2] + oz), (b[0] + ox, b[1] + oy, b[2] + oz)):
            return False
    return True


def catmull(points, step):
    """A Catmull-Rom curve through points ((x, y, z) each) sampled every step or so; returns (samples, the sample index
    of each control point)."""
    out, marks = [], []
    padded = [points[0]] + list(points) + [points[-1]]
    for k in range(1, len(padded) - 2):
        p0, p1, p2, p3 = padded[k - 1], padded[k], padded[k + 1], padded[k + 2]
        marks.append(len(out))
        count = max(1, int(math.dist(p1, p2) // step))
        for i in range(count):
            t = i / count
            t2, t3 = t * t, t * t * t
            out.append(tuple(0.5 * (2 * p1[c] + (-p0[c] + p2[c]) * t + (2 * p0[c] - 5 * p1[c] + 4 * p2[c] - p3[c]) * t2
                                    + (-p0[c] + 3 * p1[c] - 3 * p2[c] + p3[c]) * t3) for c in range(3)))
    marks.append(len(out))
    out.append(tuple(points[-1]))
    return out, marks


def plan_course(build, plan):
    """The course: control points west of the ledge, sampled, each segment swept for the hull and raised where it would
    touch."""
    mx, my, mz = plan.moored_xyz
    ledge_z = plan.ledge.z
    # She comes in along her moored heading if open air lies that way back from the ledge (the canyon, the notch), else
    # straight out of the west (the landing then turns her).
    d = plan.heading
    for way in (plan.heading, (0.0, 1.0)):
        sx, sy = mx - way[0] * GRAPPLE_BACK, my - way[1] * GRAPPLE_BACK
        if trace(sx, sy, ledge_z + GRAPPLE_OVER, ledge_z + GRAPPLE_OVER - 3000.0) is None:
            d = way
            break
    plan.approach_dir = d
    stop = (mx - d[0] * GRAPPLE_BACK, my - d[1] * GRAPPLE_BACK, ledge_z + GRAPPLE_OVER)
    approach = (mx - d[0] * APPROACH_BACK, my - d[1] * APPROACH_BACK, ledge_z + APPROACH_OVER)

    def over_ground(x, y, height, at_least):
        hit = trace(x, y, 60000.0, -60000.0)
        return max((hit[0] if hit else -60000.0) + height, at_least)
    far_x = stop[0] + 2500.0
    start = (far_x, my - COURSE_START, 0.0)
    start = (start[0], start[1], over_ground(start[0], start[1], COURSE_LOW, ledge_z + 3000.0))
    mid = (far_x - 500.0, my - (COURSE_START + COURSE_CANYON) * 0.5, 0.0)
    mid = (mid[0], mid[1], over_ground(mid[0], mid[1], COURSE_LOW, ledge_z + 2000.0))
    canyon = (stop[0] + 800.0, my - COURSE_CANYON, ledge_z - COURSE_UNDER_RIM)
    controls = [list(start), list(mid), list(canyon), list(stop), list(approach), [mx, my, mz + 300.0]]
    raised = 0
    for _ in range(COURSE_TRIES):
        samples, marks = catmull([tuple(c) for c in controls], COURSE_STEP)
        bad = [i for i in range(marks[3]) if not hull_clear(samples[i], samples[i + 1])]
        if not bad:
            break
        # Raise the control points round the first touch (never the stop, the approach or the landing).
        touched = max(k for k in range(3) if marks[k] <= bad[0])
        for k in {touched, min(touched + 1, 2)}:
            controls[k][2] += COURSE_RAISE
        raised += 1
    else:
        build.warn(f'the Lamplighter\'s course still touches rock after {COURSE_TRIES} raises: check it with the probe')
    samples, marks = catmull([tuple(c) for c in controls], COURSE_STEP)
    plan.course = [unreal.Vector(*p) for p in samples]
    plan.grapple_stop = marks[3]
    plan.course_controls = controls
    plan.course_raised = raised
    plan.course_length = sum(math.dist(samples[i], samples[i + 1]) for i in range(len(samples) - 1))
    # Over open air at the stop: nothing within 30 m under the keel.
    under = trace(stop[0], stop[1], stop[2], stop[2] - 3000.0)
    plan.stop_open = under is None


def plan_barge(build, plan):
    sink = story.layout_entry(build, 'zones', 'sink')
    lookout = story.placed(build, 'Lookout')
    if sink is None:
        return False
    cx, cy = story.centroid(sink['polygon'])
    rim = max(ground_z(x, y) for x, y in sink['polygon'])
    floors = [ground_z(cx + dx, cy + dy, -1200.0) for dx, dy in ((0, 0), (300, 0), (-300, 0), (0, 300), (0, -300))]
    plan.sink = (cx, cy, rim, min(floors))
    view = flat((cx - lookout.get_actor_location().x, cy - lookout.get_actor_location().y)) if lookout else (0.6, 0.8)
    plan.view_dir = view
    long_axis = (-view[1], view[0])
    plan.barge_axis = long_axis
    # The skiff's bow is its +X: turned so its length lies along the long axis.
    plan.barge_yaw = yaw_of(long_axis)
    plan.barge_keel = (cx, cy, rim + BARGE_OVER_RIM)
    plan.intake = (cx - view[0] * INTAKE_OFF, cy - view[1] * INTAKE_OFF,
                   ground_z(cx - view[0] * INTAKE_OFF, cy - view[1] * INTAKE_OFF, plan.sink[3]))
    return True


def plan_spyglass(build, plan):
    """The lookout deck's spot with a clear line to the barge, nearest the deck's edge that way."""
    lookout = story.placed(build, 'Lookout')
    plan.spyglass = None
    if lookout is None:
        return
    at = lookout.get_actor_location()
    seat = story.socket(lookout, 'Sit')
    frame = lookout.get_actor_transform()
    target = (plan.barge_keel[0], plan.barge_keel[1], plan.barge_keel[2] + 800.0)
    spots = []
    top = at.z + 900.0
    for i in range(-8, 13):
        for j in range(-8, 9):
            p = frame.transform_location(unreal.Vector(i * 40.0, j * 40.0, 0.0))
            hit = story.onto_host(lookout, unreal.Vector(p.x, p.y, top), unreal.Vector(p.x, p.y, at.z + 400.0))
            if hit is not None:
                spots.append((p.x, p.y, hit.z))
    if not spots:
        build.warn('no deck found on the lookout: no spyglass')
        return
    # The walking surface is the commonest height about 6.5 m up (Lookout.py FLOOR; the hut's roof and the rails are
    # higher and smaller).
    heights = [round(z / 10.0) for _, _, z in spots if DECK_UP[0] <= z - at.z <= DECK_UP[1]]
    if not heights:
        build.warn('no deck found on the lookout about 6.5 m up: no spyglass')
        return
    common = max(set(heights), key=heights.count) * 10.0
    floor = [s for s in spots if abs(s[2] - common) < 25.0]
    avoid = [(seat.translation.x, seat.translation.y)] if seat else []
    avoid += [(-8815.0, -10210.0), (-8700.0, -10100.0)]   # the ranger cache's two spots (build_area_caches)
    best = None
    centre = (sum(s[0] for s in floor) / len(floor), sum(s[1] for s in floor) / len(floor))
    for x, y, z in floor:
        if any(math.dist((x, y), a) < 130.0 for a in avoid):
            continue
        if blocked_line((x, y, z + 160.0), target, ignore=()):
            continue
        reach = (x - centre[0]) * plan.view_dir[0] + (y - centre[1]) * plan.view_dir[1]
        if best is None or reach > best[3]:
            best = (x, y, z, reach)
    if best is None:
        build.warn('no spot on the lookout\'s deck sees the barge: the spyglass stands at the deck\'s middle')
        best = (centre[0], centre[1], common, 0.0)
    plan.spyglass = best[:3]
    plan.spyglass_yaw = yaw_of((target[0] - best[0], target[1] - best[1]))


def trail_ramp(build):
    point = story.layout_entry(build, 'features', 'ransomsPoint')
    return point.get('ramp', {}).get('path') if point else None


def plan_trail(build, plan, house):
    """The trail's line: the lookout's foot, the bluff path down (its ramp reversed), the bluff approach, the family plot
    past Abel's grave, the path to the farmyard and Delia's steps."""
    route = []
    lookout = story.placed(build, 'Lookout')
    ramp = trail_ramp(build)
    if lookout is not None and ramp:
        # From the lookout's foot on the side toward the bluff path's top.
        at = lookout.get_actor_location()
        toward = flat((ramp[-1][0] - at.x, ramp[-1][1] - at.y))
        route.append((at.x + toward[0] * TRAIL_FROM_LOOKOUT, at.y + toward[1] * TRAIL_FROM_LOOKOUT))
    if ramp:
        route += [tuple(p) for p in reversed(ramp)]
    approach = story.layout_entry(build, 'roads', 'bluffApproach')
    if approach:
        route += [tuple(p) for p in reversed(approach['path'])][1:]
    abel = story.placed(build, 'Grave_Abel')
    to_farm = story.layout_entry(build, 'roads', 'familyPlotPath')
    if abel is not None and to_farm:
        # Past his grave on the way through the plot (1.5 m off it, toward the path).
        g = abel.get_actor_location()
        route.append((g.x - 150.0, g.y))
        route += [tuple(p) for p in to_farm['path']]
    if house is not None:
        route = round_house(route, house)
    plan.trail_route = route
    rng = random.Random(SEED)
    wisps = []
    for x, y, d in polyline_points(route, TRAIL_STEP):
        side = rng.uniform(-TRAIL_SIDE, TRAIL_SIDE)
        px, py = x - d[1] * side, y + d[0] * side
        z = ground_z(px, py, 0.0)
        yaw = yaw_of(d) + rng.uniform(-TRAIL_TURN, TRAIL_TURN)
        scale = rng.uniform(*TRAIL_SCALE)
        wisps.append((rng.randrange(len(STAND_INS['trail_wisps'])),
                      transform((px, py, z + TRAIL_LIFT), (0.0, TRAIL_PITCH, yaw), (scale, scale, scale))))
    plan.trail = wisps
    # Abel's hat: at the bluff path's bend nearest the middle of the ramp, a step in from its middle toward the bluff.
    plan.hat = None
    if ramp and len(ramp) >= 3:
        bend = ramp[len(ramp) // 2]
        before, after = ramp[len(ramp) // 2 - 1], ramp[len(ramp) // 2 + 1]
        d = flat((after[0] - before[0], after[1] - before[1]))
        centre_z = ground_z(bend[0], bend[1])
        best = None
        for sign in (1.0, -1.0):
            x, y = bend[0] - d[1] * HAT_IN * sign, bend[1] + d[0] * HAT_IN * sign
            z = ground_z(x, y)
            if abs(z - centre_z) < 60.0 and (best is None or z > best[2]):
                best = (x, y, z)
        if best:
            plan.hat = best
            plan.hat_yaw = yaw_of(d) + 70.0


def round_house(route, house):
    """The route's end led round the farmhouse to its porch steps: points inside the house (with a margin) are left out,
    and from the last one outside it goes round the house's nearer corner, out past the porch's front and in to the
    steps (Farmhouse.py, in the house's frame: front +X, the walls at X +-310 and Y +-420, the porch to 530)."""
    frame = house.get_actor_transform()
    inverse = frame.inverse()

    def local(p):
        v = inverse.transform_location(unreal.Vector(p[0], p[1], house.get_actor_location().z))
        return v.x, v.y

    def world_xy(lx, ly):
        v = frame.transform_location(unreal.Vector(lx, ly, 0.0))
        return v.x, v.y
    margin = HOUSE_MARGIN
    kept = [p for p in route if not (abs(local(p)[0]) < 310.0 + margin and abs(local(p)[1]) < 420.0 + margin)
            and not (310.0 - margin < local(p)[0] < PORCH_FRONT + margin and abs(local(p)[1]) < 360.0 + margin)]
    if not kept:
        return route
    lx, ly = local(kept[-1])
    side = 1.0 if ly >= 0.0 else -1.0
    corner_y = side * (420.0 + margin)
    if lx < PORCH_FRONT:
        # Behind or beside the house: round its back corner (if behind it) and along its side to the porch's front.
        if lx < -310.0:
            kept.append(world_xy(-310.0 - margin, corner_y))
        kept.append(world_xy(PORCH_FRONT + margin, corner_y))
    kept.append(world_xy(PORCH_FRONT + 150.0, 0.0))
    return kept


def plan_porch(build, plan, house):
    """FarmPorch's spots: the dredgers along the farm road, the dead in the orchard, PORCH_RING from the porch."""
    plan.porch = None
    if house is None:
        return
    front = house.get_actor_transform().transform_location(unreal.Vector(PORCH_FRONT + 100.0, 0.0, 0.0))
    plan.porch = (front.x, front.y, ground_z(front.x, front.y))
    road = story.layout_entry(build, 'roads', 'farmRoad')
    orchard = story.layout_entry(build, 'zones', 'orchard')
    lo, hi = PORCH_RING
    dredgers = []
    if road:
        for x, y, d in polyline_points(road['path'], 300.0):
            for side in (-250.0, 250.0):
                px, py = x - d[1] * side, y + d[0] * side
                if lo <= math.dist((px, py), plan.porch[:2]) <= hi:
                    dredgers.append((px, py))
    dead = []
    if orchard:
        xs, ys = [p[0] for p in orchard['polygon']], [p[1] for p in orchard['polygon']]
        for i in range(int(min(xs)), int(max(xs)), 300):
            for j in range(int(min(ys)), int(max(ys)), 300):
                if inside((i, j), orchard['polygon']) and lo <= math.dist((i, j), plan.porch[:2]) <= hi:
                    dead.append((float(i), float(j)))
    plan.porch_dredgers = spread(dredgers, 3 + 2)
    plan.porch_dead = spread(dead, 3)


def spread(points, count):
    if not points:
        return []
    chosen = [points[len(points) // 2]]
    while len(chosen) < min(count, len(points)):
        chosen.append(max(points, key=lambda p: min(math.dist(p, c) for c in chosen)))
    return chosen


def make_plan(build):
    """Everything measured, nothing placed (the probe prints it; place() places it)."""
    plan = Plan()
    BOUNDARY[:] = [c[:2] for c in (build.layout or {}).get('boundary', {}).get('corners', [])]
    plan.steps = mission_steps(build)
    house = story.placed(build, 'Farmhouse')
    plan.ok_ledge = plan_moored(build, plan)
    if plan.ok_ledge:
        plan_course(build, plan)
    plan.ok_barge = plan_barge(build, plan)
    if plan.ok_barge:
        plan_spyglass(build, plan)
    else:
        plan.spyglass, plan.lookout_foot = None, None
    plan_trail(build, plan, house)
    plan_porch(build, plan, house)
    return plan


def describe(plan):
    """The plan in lines (the probe and the build log)."""
    out = [f'steps {plan.steps}']
    if plan.ok_ledge:
        m = plan.moored_xyz
        out.append(f'ledge z {plan.ledge.z:.0f}, cells {plan.ledge.counts()}, middle ({plan.ledge_middle[0]:.0f}, '
                   f'{plan.ledge_middle[1]:.0f}), stairs foot {plan.ledge.stairs_foot}')
        out.append(f'moored ({m[0]:.0f}, {m[1]:.0f}, {m[2]:.0f}) yaw {plan.moored_yaw:.1f}, bow ({plan.bow[0]:.0f}, '
                   f'{plan.bow[1]:.0f}), tier {plan.tier} ({plan.share * 100:.0f}% on the ledge, or alongside its edge)')
        out.append(f'winch {plan.winch}, climbers {[(round(x), round(y)) for x, y, _ in plan.climbers]}')
        out.append(f'course {len(plan.course)} points, {plan.course_length / 100:.0f} m, grapple stop at {plan.grapple_stop} '
                   f'({plan.course[plan.grapple_stop]}), raised {plan.course_raised} times, open air under the stop: '
                   f'{plan.stop_open}; controls {[[round(c) for c in p] for p in plan.course_controls]}')
    if plan.ok_barge:
        out.append(f'sink {[round(v) for v in plan.sink]}, barge keel {[round(v) for v in plan.barge_keel]} yaw '
                   f'{plan.barge_yaw:.1f}, intake {[round(v) for v in plan.intake]}')
        out.append(f'spyglass {plan.spyglass} yaw {getattr(plan, "spyglass_yaw", 0.0):.1f}')
    out.append(f'trail {len(plan.trail)} wisps along {len(plan.trail_route)} turns; hat {plan.hat}')
    if plan.porch:
        out.append(f'porch {[round(v) for v in plan.porch]}, dredgers {plan.porch_dredgers}, dead {plan.porch_dead}')
    return out


# ---------------------------------------------------------------------------
# Placing
# ---------------------------------------------------------------------------

def place_ship(build, plan):
    cls = story.actor_class(NAMES['ship_class'])
    if cls is None:
        build.warn(f'no {NAMES["ship_class"]} class (lane FLIGHT; build the game module): no Lamplighter. Her plan: '
                   f'moored {plan.moored_xyz}, grapple stop {plan.grapple_stop} of {len(plan.course)}')
        return None
    x, y, z = plan.moored_xyz
    ship = build.place(cls, (x, y, z), plan.moored_yaw, label='Lamplighter', folder=FOLDER, tags=NAMES['ship_tags'])
    ship.set_actor_rotation(plan.moored.rotation.rotator(), False)
    set_or_warn(build, ship, NAMES['ship_course'], plan.course, 'the Lamplighter')
    set_or_warn(build, ship, NAMES['ship_grapple_stop'], plan.grapple_stop, 'the Lamplighter')
    set_or_warn(build, ship, NAMES['ship_moored'], plan.moored, 'the Lamplighter')
    side = getattr(plan, 'ledge_side', -1.0)
    set_or_warn(build, ship, 'gangway_side', float(side), 'the Lamplighter')
    set_or_warn(build, ship, 'tender_side', float(-side), 'the Lamplighter')
    canyon = getattr(plan, 'canyon_side', None)
    if canyon:
        set_or_warn(build, ship, 'canyon_side', unreal.Vector(canyon[0], canyon[1], 0.0), 'the Lamplighter')
    build.log(f'the Lamplighter moored at ({x:.0f}, {y:.0f}, {z:.0f}) yaw {plan.moored_yaw:.0f} ({plan.tier}), her course {len(plan.course)} points ({plan.course_length / 100:.0f} m) stopping at '
              f'{plan.grapple_stop}')
    return ship


def place_winch(build, plan):
    cls = story.actor_class(NAMES['winch_class'])
    if cls is None or plan.winch is None:
        build.warn('no LandfallWinch class (lane FLIGHT) or no ground by the bow: no anchor winch' if cls is None
                   else 'no ground by the bow: no anchor winch')
        return None
    x, y, z = plan.winch
    build.place(cls, (x, y, z), plan.winch_yaw, label='Landfall_AnchorWinch', folder=FOLDER, tags=(NAMES['winch_tag'],))
    build.log(f'the anchor winch at ({x:.0f}, {y:.0f}, {z:.0f}), facing the hull')
    return x, y, z + WINCH_TOP, plan.winch_yaw


def group(cls_name, count, rank, wave):
    cls = story.actor_class(cls_name)
    if cls is None:
        return None
    return story.group(cls, count, rank, first_wave=wave, last_wave=wave)


def encounter_when(build, spawner_id, from_step, before_step):
    """A Landfall fight's active_when: story_texts.ENCOUNTERS[spawner_id] (STORY's, the one source, as every mission
    fight's), else Main 1's steps as its data counts them; a mismatch between the two is logged for Main to fix."""
    import story_texts
    texts = importlib.reload(story_texts)
    spec = texts.ENCOUNTERS.get(spawner_id)
    if spec is None:
        return story.condition(during=MAIN1, from_step=from_step, before_step=before_step)
    if (spec.get('from_step', 0), spec.get('before_step', 0)) != (from_step, before_step):
        build.warn(f'{spawner_id}: story_texts.ENCOUNTERS says steps {spec.get("from_step", 0)}..'
                   f'{spec.get("before_step", 0)}, Main 1\'s data {from_step}..{before_step}: ENCOUNTERS is used')
    return texts.condition(spec)


def place_ledge_climbers(build, plan):
    cls = story.actor_class('AmbushSpawner')
    made = group(NAMES['unpaid_class'], LEDGE_CLIMBERS, 'BASIC', 1)
    if cls is None or made is None or not plan.climbers:
        build.warn('no AmbushSpawner or UnpaidCreature class, or no edge on the ledge: no LedgeClimbers')
        return
    mx, my = plan.ledge_middle
    z = plan.ledge.z
    spawner = build.place(cls, (mx, my, z + 50.0), 0.0, label=f'Encounter_{NAMES["ledge_spawner"]}', folder=FOLDER)
    spawner.set_editor_property('spawner_id', unreal.Name(NAMES['ledge_spawner']))
    spawner.set_editor_property('groups', [made])
    spawner.set_editor_property('creature_tags', [unreal.Name('Unpaid_' + NAMES['ledge_spawner'])])
    anchor = plan.steps['anchor']
    spawner.set_editor_property('active_when', encounter_when(build, NAMES['ledge_spawner'], anchor, anchor + 1))
    # No ambush corners: it springs on approach (the player lands on the ledge: a walk-in never comes), rising.
    spawner.set_editor_property('entrance', unreal.AmbushEntrance.RISE)
    spawner.set_editor_property('activation_radius', LEDGE_ACTIVATION)
    spawner.set_editor_property('hunt_on_spawn', True)
    spawner.set_editor_property('spawn_points', [unreal.Vector(x - mx, y - my, 0.0) for x, y, _ in plan.climbers])
    # Their ground is a ring round the ledge, not its outline: the moored deck (flush with the ledge, or on it) is theirs
    # too; the Point's top, 20 m up, isn't (the rise).
    spawner.set_editor_property('give_up_radius', LEDGE_GIVE_UP)
    spawner.set_editor_property('ground_max_rise', 600.0)
    build.log(f'LedgeClimbers: {LEDGE_CLIMBERS} Unpaid rising at the ledge\'s edges {[(round(x), round(y)) for x, y, _ in plan.climbers]} '
              f'during {MAIN1}\'s step {anchor}')


def place_spyglass(build, plan):
    cls = story.actor_class(NAMES['spyglass_class'])
    if cls is None or plan.spyglass is None:
        if cls is None:
            build.warn('no LandfallSpyglass class (lane FLIGHT): no spyglass on the lookout')
        return
    x, y, z = plan.spyglass
    build.place(cls, (x, y, z), plan.spyglass_yaw, label='Landfall_Spyglass', folder=FOLDER,
                tags=(NAMES['spyglass_tag'],))
    build.log(f'the spyglass on the lookout\'s deck at ({x:.0f}, {y:.0f}, {z:.0f}), facing the barge ({plan.spyglass_yaw:.0f})')


def barge_parts(build, plan):
    """The barge's parts (the hull, stacks, vents, drum) and where the stacks' tops are."""
    hull = load(STAND_INS['barge_hull'])
    if hull is None:
        build.warn(f'no {STAND_INS["barge_hull"]}: no barge')
        return [], [], None
    if is_tripo(STAND_INS['barge_hull']):
        return tripo_barge(plan, hull)
    iron, glow = material('iron'), material('glow')
    box = hull.get_bounding_box()
    lo, hi = box.get_editor_property('min'), box.get_editor_property('max')
    s = BARGE_SCALE
    # The skiff's bow is its +X in Unreal (Blender's front, -Y: Art/README.md), its beam along Y.
    length, width, top = (hi.x - lo.x) * s, (hi.y - lo.y) * s, hi.z * s
    kx, ky, kz = plan.barge_keel
    yaw = plan.barge_yaw
    frame = transform((kx, ky, kz), (0.0, 0.0, yaw))

    def at(lx, ly, lz):
        p = frame.transform_location(unreal.Vector(lx, ly, lz))
        return p.x, p.y, p.z
    # Every slot of the skiff in iron (the bag too: from afar the barge reads as one dark mass).
    slots = len(hull.get_editor_property('static_materials'))
    parts = [Part(hull, transform((kx, ky, kz), (0.0, 0.0, yaw), (s, s, s)), [iron] * max(slots, 1))]
    stack = load(STAND_INS['barge_stack'])
    tops = []
    if stack is not None:
        # Three abreast at the stern quarter (the skiff's stern is -X), from the deck up through the bag.
        bottom, high = SKIFF_DECK * s, top + STACK_OVER_BAG
        lx = -0.34 * length
        for ly in (-0.15 * width, 0.0, 0.15 * width):
            cx, cy, cz = at(lx, ly, (bottom + high) * 0.5)
            parts.append(Part(stack, transform((cx, cy, cz), (0.0, 0.0, yaw),
                                               (STACK_SCALE[0], STACK_SCALE[1], (high - bottom) / 100.0)), [iron]))
            tops.append(at(lx, ly, high))
    vent = load(STAND_INS['barge_vent'])
    if vent is not None:
        # Along both sides of the hull (the skiff's widest gunwale is 1.2 m out), halfway up its side.
        for side in (-1.0, 1.0):
            for k in range(BARGE_VENTS):
                lx = (-0.3 + 0.6 * k / max(BARGE_VENTS - 1, 1)) * length
                parts.append(Part(vent, transform(at(lx, side * SKIFF_HALF_BEAM * s, SKIFF_VENT_Z * s),
                                                  (0.0, 0.0, yaw), (1.4, 0.3, 0.55)), [glow]))
    drum = load(STAND_INS['barge_drum'])
    if drum is not None:
        # Its axis across the hull (the skiff's own Y turned by the yaw), under the keel.
        across = unreal.Vector(-math.sin(math.radians(yaw)), math.cos(math.radians(yaw)), 0.0)
        rot = unreal.MathLibrary.make_rot_from_z(across)
        parts.append(Part(drum, transform(at(0.0, 0.0, -DRUM_SCALE * 50.0), rot,
                                          (DRUM_SCALE, DRUM_SCALE, width * 0.7 / 100.0)), [iron]))
    plan.drum_bottom = at(0.0, 0.0, -DRUM_SCALE * 100.0)
    plan.barge_size = (length, width, top)
    return parts, tops, hull


def is_tripo(path):
    return bool(path) and path.startswith(TRIPO_FOLDER)


def socket_at(mesh, name):
    """A static mesh socket's place in the mesh's frame, or None."""
    socket = mesh.find_socket(name)
    return socket.get_editor_property('relative_location') if socket is not None else None


def tripo_barge(plan, hull):
    """The Tripo barge (lanes/IMPORT.md): one mesh at its real 75 m, its pivot under its middle at the bottom, bow +X, its
    stacks, vents and hose drum modelled, in its own materials. Its sockets: Stack_1/2 (the smog's tops), HoseDrum."""
    box = hull.get_bounding_box()
    lo, hi = box.get_editor_property('min'), box.get_editor_property('max')
    frame = transform(plan.barge_keel, (0.0, 0.0, plan.barge_yaw))

    def at(v):
        p = frame.transform_location(v)
        return p.x, p.y, p.z
    tops = [at(p) for p in (socket_at(hull, f'Stack_{k}') for k in range(1, 7)) if p is not None]
    drum = socket_at(hull, 'HoseDrum')
    plan.drum_bottom = at(drum if drum is not None else unreal.Vector(0.0, 0.0, lo.z))
    plan.barge_size = (hi.x - lo.x, hi.y - lo.y, hi.z)
    return [Part(hull, frame)], tops, hull


def hose_parts(build, plan):
    section, collar, intake = load(STAND_INS['hose_section']), load(STAND_INS['hose_collar']), load(STAND_INS['hose_intake'])
    if section is None:
        return []
    rubber, iron = material('rubber'), material('iron')
    # The Tripo section (its axis Z, its middle the pivot, collars on it) is scaled to HOSE_GIRTH across and keeps its
    # proportions (~5.4 m a section), in its own material; the stand-in cylinder is 1 m both ways and stretched to 10 m.
    own_hose, own_intake = is_tripo(STAND_INS['hose_section']), intake is not None and is_tripo(STAND_INS['hose_intake'])
    native, girth, section_len = 100.0, HOSE_GIRTH, HOSE_SECTION
    if own_hose:
        size = section.get_bounding_box().get_editor_property('max') - section.get_bounding_box().get_editor_property('min')
        native = max(size.z, 1.0)
        girth = HOSE_GIRTH * 100.0 / max(size.x, size.y, 1.0)
        section_len = native * girth
        collar = None
    top = plan.drum_bottom
    if own_intake:
        mouth = socket_at(intake, 'Hose')
        rise = mouth.z if mouth is not None else intake.get_bounding_box().get_editor_property('max').z
        bottom = (plan.intake[0], plan.intake[1], plan.intake[2] + rise)
    else:
        bottom = (plan.intake[0], plan.intake[1], plan.intake[2] + INTAKE_SCALE[2] * 100.0)
    drop = top[2] - bottom[2]
    side = plan.barge_axis
    c1 = (top[0] + side[0] * HOSE_SAG, top[1] + side[1] * HOSE_SAG, top[2] - drop * 0.45)
    c2 = (bottom[0] + side[0] * HOSE_SAG * 0.6, bottom[1] + side[1] * HOSE_SAG * 0.6, bottom[2] + drop * 0.3)

    def bez(t):
        u = 1.0 - t
        return tuple(u * u * u * top[c] + 3 * u * u * t * c1[c] + 3 * u * t * t * c2[c] + t * t * t * bottom[c]
                     for c in range(3))
    rough = sum(math.dist(bez(i / 40.0), bez((i + 1) / 40.0)) for i in range(40))
    count = max(6, int(math.ceil(rough / section_len)))
    points = [bez(i / count) for i in range(count + 1)]
    parts = []
    for a, b in zip(points, points[1:]):
        mid = tuple((a[c] + b[c]) * 0.5 for c in range(3))
        u = unreal.Vector(b[0] - a[0], b[1] - a[1], b[2] - a[2])
        rot = unreal.MathLibrary.make_rot_from_z(u)
        length = math.dist(a, b)
        parts.append(Part(section, transform(mid, rot, (girth, girth, length * (1.02 if own_hose else 1.04) / native)),
                          [] if own_hose else [rubber]))
        if collar is not None:
            parts.append(Part(collar, transform(a, rot, (COLLAR_GIRTH, COLLAR_GIRTH, COLLAR_LONG)), [iron]))
    if own_intake:
        # Its mouth down on the floor (its pivot), its side pipe toward the barge's long axis.
        parts.append(Part(intake, transform(plan.intake, (0.0, 0.0, yaw_of(plan.barge_axis)))))
    elif intake is not None:
        parts.append(Part(intake, transform((plan.intake[0], plan.intake[1], plan.intake[2] + INTAKE_SCALE[2] * 50.0),
                                            (0.0, 0.0, 0.0), INTAKE_SCALE), [iron]))
    return parts


def place_barge(build, plan):
    """The barge, its hose, its smog and its drone, until BARGE_UNTIL is done."""
    shown = [story.condition(before=[BARGE_UNTIL])]
    parts, tops, hull = barge_parts(build, plan)
    if not parts:
        return
    made = baked_or_parts(build, 'Barge', parts, 'Landfall_Barge', shown, tags=(NAMES['barge_tag'],), cast_shadow=True,
                          fallback_most=5)
    hose = hose_parts(build, plan)
    baked_or_parts(build, 'Hose', hose, 'Landfall_BargeHose', shown, cast_shadow=False, fallback_most=8)
    plume = load(STAND_INS['smog_plume'])
    smog, glint = material('smog'), material('glint')
    if plume is not None:
        for i, (x, y, z) in enumerate(tops):
            story_prop(build, plume, transform((x, y, z), (0.0, 0.0, WIND_YAW), (STACK_PLUME,) * 3), f'Landfall_Smog_{i + 1}',
                       shown=shown, materials=[smog])
        if tops:
            x, y, z = tops[len(tops) // 2]
            story_prop(build, plume, transform((x, y, z + 50.0), (0.0, 0.0, WIND_YAW + 25.0), (GLINT_PLUME,) * 3),
                       'Landfall_SmogGlint', shown=shown, materials=[glint])
        story_prop(build, plume, transform(plan.intake, (0.0, 0.0, WIND_YAW), (INTAKE_PLUME,) * 3), 'Landfall_SmogIntake',
                   shown=shown, materials=[smog])
    else:
        build.warn(f'no {STAND_INS["smog_plume"]}: no soul smog')
    emitter_cls = story.actor_class('AmbientEmitter')
    if emitter_cls is not None:
        half = plan.barge_size[0] * 0.5
        kx, ky, kz = plan.barge_keel
        ax, ay = plan.barge_axis
        drone = build.place(emitter_cls, (kx, ky, kz), 0.0, label='Landfall_SmogDrone', folder=FOLDER, tags=('Ambience',))
        component = drone.get_editor_property('emitter')
        component.set_editor_property('loop_cue', unreal.Name('Ambience.Smog'))
        component.set_editor_property('path', [unreal.Vector(kx - ax * half, ky - ay * half, kz),
                                               unreal.Vector(kx + ax * half, ky + ay * half, kz)])
    length, width, _ = plan.barge_size
    build.log(f'Vane\'s barge over the Sink ({length / 100:.0f} m, its keel at {plan.barge_keel[2]:.0f}, {len(parts)} parts, '
              f'{"baked" if len(made) == 1 else "one by one"}), its hose ({len(hose)} parts) into the floor, {len(tops)} '
              f'stacks smoking, until {BARGE_UNTIL} is done')


def place_smog_rule(build, plan):
    """The Smog lighting state before Main 6 is done, after the deck's Dusk rules (their own missions come first)."""
    lighting = next((a for a in actors.get_all_level_actors()
                     if str(a.get_actor_label()) == 'StoryLighting' and unreal.Name(build.tag) in a.tags), None)
    if lighting is None:
        cls = story.actor_class('StoryLighting')
        if cls is None:
            build.warn('no StoryLighting class: the valley stays clear of smog')
            return
        lighting = build.place(cls, (0.0, 0.0, 3000.0), 0.0, label='StoryLighting', folder=FOLDER)
    rules = [r for r in lighting.get_editor_property('rules') if str(r.get_editor_property('state')) != 'Smog']
    rule = unreal.StoryLightingRule()
    rule.set_editor_property('when', story.condition(before=[BARGE_UNTIL]))
    rule.set_editor_property('state', unreal.Name('Smog'))
    lighting.set_editor_property('rules', rules + [rule])
    build.log(f'the story\'s light: Smog before {BARGE_UNTIL} is done, after the {len(rules)} rule(s) before it')


def place_trail(build, plan):
    wisps = [load(p) for p in STAND_INS['trail_wisps']]
    trail = material('trail')
    if not plan.trail or not any(wisps):
        build.warn('no Gravewind wisps or no trail route: no Abel\'s trail')
        return
    parts = [Part(wisps[k] or next(w for w in wisps if w), xform, [trail]) for k, xform in plan.trail]
    shown = [story.condition(during=MAIN1, from_step=plan.steps['trail'])]
    made = baked_or_parts(build, 'AbelTrail', parts, 'Landfall_AbelTrail', shown, check=True,
                          fallback_most=TRAIL_FALLBACK_MOST, draw_distance=TRAIL_FALLBACK_DRAW)
    build.log(f'Abel\'s trail: {len(parts)} wisps from the lookout to Delia\'s steps ({len(made)} prop(s)), during {MAIN1} '
              f'from step {plan.steps["trail"]}')


def place_hat(build, plan, house):
    """Abel's hat on the bluff path (taken: the flag, and it hangs on Delia's chair). Its words, when and flag are
    story_texts.READABLES['AbelsHat'] (STORY's: shown during Main 1 from the trail step until taken), its model
    READABLE_MODELS['AbelsHat'] (one name to swap for the Tripo hat)."""
    import story_texts
    texts = importlib.reload(story_texts)
    cls = story.actor_class('ReadableProp')
    readable = NAMES['hat_readable']
    model = texts.READABLE_MODELS.get(readable)
    hat = load(HAT_MODELS + model if model else STAND_INS['abel_hat']) or load(STAND_INS['abel_hat'])
    if cls is None or plan.hat is None or readable not in texts.READABLES:
        build.warn(f'no ReadableProp class, no spot on the bluff path or no story_texts.READABLES[{readable!r}]: no '
                   f'Abel\'s hat')
        return
    x, y, z = plan.hat
    # AReadableProp tags itself Readable_<id> too; the mission's optional Interact counts it by Landfall_AbelsHat.
    prop = build.place(cls, (x, y, z + 4.0), plan.hat_yaw, label='Landfall_AbelsHat', folder=FOLDER,
                       tags=(NAMES['hat_tag'], 'Readable', 'Readable_' + readable))
    texts.apply_readable(prop, readable)
    flag = texts.READABLES[readable].get('sets_flag') or NAMES['hat_flag']
    if not texts.READABLES[readable].get('shown_when'):
        # Gone once taken.
        prop.set_editor_property('shown_when', texts.condition(dict(without_flags=[flag])))
    # A step's key action takes a short hold, not a tap (the mission gates' rule).
    try:
        prop.set_editor_property('hold_to_read', True)
    except Exception as error:
        build.warn(f'no ReadableProp.hold_to_read ({error}): the hat is picked up with a tap')
    if hat is not None:
        prop.get_editor_property('mesh').set_static_mesh(hat)
    build.log(f'Abel\'s hat on the bluff path at ({x:.0f}, {y:.0f}, {z:.0f}), setting {flag}')
    if house is None or hat is None:
        return
    # On Delia's rocking chair once she has it (the seat found by a trace onto the house, either side of the door).
    frame = house.get_actor_transform()
    lx, ly, (low, high) = HAT_CHAIR
    for sign in (1.0, -1.0):
        p = frame.transform_location(unreal.Vector(lx, ly * sign, 0.0))
        base = house.get_actor_location().z
        seat = story.onto_host(house, unreal.Vector(p.x, p.y, base + 250.0), unreal.Vector(p.x, p.y, base + 40.0))
        if seat is not None and low <= seat.z - base <= high:
            story_prop(build, hat, transform((seat.x, seat.y, seat.z), (0.0, 0.0, house.get_actor_rotation().yaw + 160.0)),
                       'Landfall_AbelsHat_OnChair', shown=[texts.condition(dict(with_flags=[flag]))])
            build.log('Abel\'s hat on Delia\'s rocking chair once it\'s taken')
            return
    build.warn('no seat found on Delia\'s rocking chair: the hat isn\'t seen at the farm after it\'s taken')


def place_porch_fight(build, plan):
    cls = story.actor_class('EncounterSpawner')
    if cls is None or plan.porch is None:
        build.warn('no EncounterSpawner class or no farmhouse: no FarmPorch')
        return
    groups, missing = [], []
    for name, count, wave in PORCH_WAVES:
        made = group(name, count, 'BASIC', wave)
        if made is None:
            missing.append(name)
        else:
            groups.append(made)
    if missing:
        build.warn(f'FarmPorch: no {", ".join(sorted(set(missing)))} class yet (lane DREDGER): those are left out')
    if not groups:
        return
    px, py, pz = plan.porch
    spawner = build.place(cls, (px, py, pz + 50.0), 0.0, label=f'Encounter_{NAMES["porch_spawner"]}', folder=FOLDER)
    spawner.set_editor_property('spawner_id', unreal.Name(NAMES['porch_spawner']))
    spawner.set_editor_property('groups', groups)
    spawner.set_editor_property('num_waves', 2)
    spawner.set_editor_property('wait_for_clear', True)
    spawner.set_editor_property('wave_interval', PORCH_BREATH)
    spawner.set_editor_property('max_alive', PORCH_MAX_ALIVE)
    spawner.set_editor_property('creature_tags', [unreal.Name('Landfall_' + NAMES['porch_spawner'])])
    trail, porch = plan.steps['trail'], plan.steps['porch']
    spawner.set_editor_property('active_when', encounter_when(build, NAMES['porch_spawner'], trail, porch + 1))
    spawner.set_editor_property('activation_radius', PORCH_ACTIVATION)
    spawner.set_editor_property('give_up_radius', PORCH_GIVE_UP)
    spawner.set_editor_property('hunt_on_spawn', True)
    # The groups take the spots in their order: the first wave's riflemen on the road, its dead in the orchard, then the
    # second wave's on the road again.
    order = plan.porch_dredgers[:2] + plan.porch_dead + plan.porch_dredgers[2:]
    spawner.set_editor_property('spawn_points', [unreal.Vector(x - px, y - py, 0.0) for x, y in order])
    build.log(f'FarmPorch: {sum(c for _, c, _ in PORCH_WAVES)} at the porch from the farm road and the orchard, during '
              f'{MAIN1} steps {trail}-{porch}')


def place_loudspeakers(build, plan):
    """The company's loudspeaker posts (instanced: posts, horns, plates) on the Point and on Main Street."""
    post, horn, plate = load(STAND_INS['speaker_post']), load(STAND_INS['speaker_horn']), load(STAND_INS['speaker_plate'])
    cls = story.actor_class('InstancedProps')
    if cls is None or post is None or horn is None:
        build.warn('no InstancedProps class or no post or horn mesh: no company loudspeakers')
        return
    spots = []   # (x, y, the way its horns face)
    lookout = story.placed(build, 'Lookout')
    if lookout is not None:
        at = lookout.get_actor_location()
        ramp = trail_ramp(build)
        toward = flat((ramp[-1][0] - at.x, ramp[-1][1] - at.y)) if ramp else (1.0, 0.0)
        heading = math.atan2(toward[1], toward[0])
        found = None
        for radius in POINT_SPEAKER_RING:
            for turn in (0.0, 0.4, -0.4, 0.8, -0.8, 1.2, -1.2, 1.6, -1.6, 2.2, -2.2, math.pi):
                x, y = at.x + math.cos(heading + turn) * radius, at.y + math.sin(heading + turn) * radius
                if speaker_spot_ok(build, x, y):
                    found = (x, y, yaw_of((TOWN_MIDDLE[0] - x, TOWN_MIDDLE[1] - y)))
                    break
            if found:
                break
        if found:
            spots.append(found)
    for x, y in TOWN_SPEAKERS:
        out = 1.0 if x > 0 else -1.0
        for nudge in (0.0, 200.0, -200.0, 400.0):
            if speaker_spot_ok(build, x + nudge * out, y):
                # Its horns face across the street (toward its middle line, X 0).
                spots.append((x + nudge * out, y, 180.0 if out > 0 else 0.0))
                break
    if not spots:
        build.warn('no clear ground for the company loudspeakers')
        return
    box = post.get_bounding_box()
    tall = max(box.get_editor_property('max').z - box.get_editor_property('min').z, 1.0)
    posts, horns, plates = [], [], []
    if is_tripo(STAND_INS['speaker_post']):
        # The Tripo post (4 m, its horns and plate on it, toward its +X): placed whole, turned to face; the target point
        # at its socket Horns.
        horns_at = socket_at(post, 'Horns') or unreal.Vector(0.0, 0.0, tall - 40.0)
        for x, y, facing in spots:
            z = ground_z(x, y)
            posts.append(transform((x, y, z), (0.0, 0.0, facing)))
            p = transform((x, y, z), (0.0, 0.0, facing)).transform_location(horns_at)
            build.place(unreal.TargetPoint, (p.x, p.y, p.z), facing, label='Landfall_Loudspeaker', folder=FOLDER,
                        tags=(NAMES['loudspeaker_tag'],))
        horn = plate = None
        spots_done = True
    else:
        spots_done = False
    for x, y, facing in ([] if spots_done else spots):
        z = ground_z(x, y)
        stretch = SPEAKER_TALL / tall
        posts.append(transform((x, y, z), (0.0, 0.0, 0.0), (1.4, 1.4, stretch)))
        for turn in (-55.0, 55.0):
            yaw = facing + turn
            d = (math.cos(math.radians(yaw)), math.sin(math.radians(yaw)))
            # The cone's tip is its +Z: pitched 90 its tip points back at the post (-X) and its bell out (+X), then yawed.
            horns.append(transform((x + d[0] * 45.0, y + d[1] * 45.0, z + SPEAKER_TALL - 40.0), (0.0, 90.0, yaw),
                                   HORN_SCALE))
        plates.append(transform((x + math.cos(math.radians(facing)) * 12.0, y + math.sin(math.radians(facing)) * 12.0,
                                 z + SPEAKER_TALL - 150.0), (0.0, 0.0, facing), PLATE_SCALE))
        build.place(unreal.TargetPoint, (x, y, z + SPEAKER_TALL - 40.0), facing, label='Landfall_Loudspeaker',
                    folder=FOLDER, tags=(NAMES['loudspeaker_tag'],))
    for label, model, transforms, solid, worn in (('Landfall_SpeakerPosts', post, posts, True, None),
                                                  ('Landfall_SpeakerHorns', horn, horns, False, material('brass')),
                                                  ('Landfall_SpeakerPlates', plate, plates, False, material('plate'))):
        if model is None:
            continue
        actor = build.place(cls, (0.0, 0.0, 0.0), label=label, folder=FOLDER)
        actor.set_editor_property('cull_distance', SPEAKER_CULL)
        actor.set_editor_property('solid', solid)
        actor.set_instances(model, transforms)
        if worn is not None:
            actor.get_editor_property('instances').set_material(0, worn)
    build.log(f'{len(spots)} company loudspeaker posts: {[(round(x), round(y)) for x, y, _ in spots]}')


def speaker_spot_ok(build, x, y):
    if road_distance(build, x, y) < 120.0:
        return False
    z = ground_z(x, y, None)
    if z is None:
        return False
    hit = trace(x, y, z + 500.0, z - 100.0)
    if hit is None or hit[1] is None or unreal.Name('Ground') not in hit[1].tags:
        return False
    return solid_near(x, y, z + 200.0, 110.0) is None


def place(build):
    """Everything above (build_area_story.place calls this after the deck). Returns Hob's Landfall perches and Main 1's
    steps: {'winch': (x, y, z, yaw) or None, 'porch': ..., 'steps': {'anchor': 4, ...}}."""
    spots = {'winch': None, 'porch': None, 'steps': dict(STEPS)}
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return spots
    plan = make_plan(build)
    spots['steps'] = plan.steps
    for line in describe(plan):
        build.log('landfall plan: ' + line)
    house = story.placed(build, 'Farmhouse')
    if plan.ok_ledge:
        place_ship(build, plan)
        spots['winch'] = place_winch(build, plan)
        place_ledge_climbers(build, plan)
    if plan.ok_barge:
        place_barge(build, plan)
        place_spyglass(build, plan)
    place_smog_rule(build, plan)
    place_trail(build, plan)
    place_hat(build, plan, house)
    place_porch_fight(build, plan)
    place_loudspeakers(build, plan)
    spots['porch'] = hob_on_porch(house)
    return spots


def hob_on_porch(house):
    """On the porch beam's end over the steps' side (Farmhouse.py: posts at +-3.45 m, the beam 3.1 m up, 5.13 m out)."""
    if house is None:
        return None
    frame = house.get_actor_transform()
    base = house.get_actor_location().z
    for side in (1.0, -1.0):
        p = frame.transform_location(unreal.Vector(513.0, 345.0 * side, 0.0))
        top = story.onto_host(house, unreal.Vector(p.x, p.y, base + 420.0), unreal.Vector(p.x, p.y, base + 250.0))
        if top is not None:
            return top.x, top.y, top.z, house.get_actor_rotation().yaw
    return None


def run(name):
    """Places Gameplay/Landfall again on its own, saves (Hob's perches and Delia need the whole gameplay pass)."""
    import build_area  # noqa: E402 (here: build_area imports build_area_story, which this module uses)
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('Landfall\'s pieces placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_landfall.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
