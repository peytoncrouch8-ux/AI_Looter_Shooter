"""Side 2, "Unfinished Business" (Docs/Areas/RansomsRest.md), for build_area_story.py: Side 2's pieces in Whitlock
Fields, placed from the layout's obstacles and placements (Art/Levels/RansomsRest/layout.json: amosFence,
barnYardFence, the whitlockBarn placement) and on the terrain's tiles, so they follow the level whenever it's rebuilt.
build_area_story's place() calls place() here (reloaded each run, as the editor keeps modules between runs); it uses
build_area_story's helpers. Everything goes in the area's Gameplay folder, so a "gameplay" build places it all again; a
piece whose model or class isn't there yet is left out with a warning.

Whitlock Fields' dressing ("Hayfields split by a stone field wall, with hay bales, Amos's fence along the north, and
the Whitlock barn and the existing windmill inside a yard fence") is the level's, not the story's: Amos's fence with his
gate where the fields path crosses it, the barn yard's fence with its gate where the path ends, the stone field wall and
the round bales in the east field are build_area_dressing.py's (its LINES and SPOTS tables, instanced in the Dressing
folder), and the barn and the windmill are build_area.py's placements. The dressing lays the section just past Amos's
gate at the kit's own 3 m, unstretched, and this stands Amos in it from the same plan (build_area_dressing.run_head), so
he's always where his fence is.

Side 2:
- Amos (AAmosWhitlock tagged Speaker_Amos) leaning on his fence by his gate: in the middle of the first span past the gate
  (its post at his right), on the fence's line, facing across it into his hayfield, the span the kit's true 1.5 m so his
  pose (fitted to FenceRail) meets its rail. There from Main 4's finish; on the rail after Side 2. Talked to: the first
  meeting (Side 2's first step), the bales (its second), the hands (its third), his thanks (its last), and after it,
  sitting, waiting for the saint.
- Six hay bales (AHayBale tagged HayBale) lying out in the west field, each with its place in a stack of six under the
  hoist by the Whitlock barn's big doors (three, two, one, along the barn's front): loaded by holding Interact from Side 2's
  second step, all in the stack once it's done. Amos remarks on the first and the last, Hob on the third.
- His old hired hands (AEncounterSpawner WhitlockHands) in the barn yard: five Unpaid and a Restless one, tagged
  Unpaid_WhitlockHands, on during Side 2's third step only, appearing round the barn as the player comes within 22 m of
  the yard's middle (its gate is 20 m off) and coming for them, hunting only inside the yard's fence (and its 2 m margin),
  so the fight stays in the yard.
Hob has no Side 2 perches: side missions don't move him (Side 1 and Side 3 have none either; his perches follow the main
mission), so he has his word as a remark on the third bale.

Their words are line sets in /Game/Data/Story (Tools/Unreal/create_story_lines.py makes them first); a set that's missing
is left out with a warning.
"""
import importlib
import math

import unreal

import build_area
import build_area_story as story

# The story's ids and tags, as the C++ (Story/AmosWhitlock.h, World/HayBale.h) and the mission asset
# (Tools/Unreal/create_side_mission_assets.py, DA_Mission_Side2) name them.
MAIN4 = 'Main4'
MAIN6 = 'Main6'
SIDE2 = 'Side2'
AMOS_TAG = 'Speaker_Amos'
BALE_TAG = 'HayBale'
HANDS = 'WhitlockHands'
HANDS_TAG = 'Unpaid_WhitlockHands'

# Side 2's steps, counted from 0: talk to Amos, load the bales, drive off the hands, talk to Amos.
LOAD_STEP = 1
FIGHT_STEP = 2
THANKS_STEP = 3

# Amos's span (Fences.py's FenceRail): posts 1.5 m apart, the first on the section's pivot.
SPAN = 150.0

# Side 2's bales in the west field (layout cm, yaw): one near the gate where the path comes in, the rest spread over the
# field between Amos's fence and the barn yard, and one in the strip east of the yard; none on the path.
FIELD_BALES = ((-4300.0, 4700.0, 20.0), (-4700.0, 3900.0, -35.0), (-4400.0, 3000.0, 80.0), (-5000.0, 2600.0, 5.0),
               (-5100.0, 4400.0, -60.0), (-6600.0, 5400.0, 40.0))
# Their stack under the hoist, in the barn's frame (cm: +X out of its front, where the big doors and the loft door face,
# +Y along it): three bales end to end, two on them, one on top, each lying along the front (FarmProps.py's square bale
# is 0.95 m long along the model's X, its actor's Y; 0.46 m deep; 0.36 m tall). The barn's front is 6 m out from its
# pivot (Barn.py); the stack stands 0.75 m off it, under the hook.
BARN_FRONT = 600.0
STACK_OUT = 75.0
STACK_SLOTS = ((0.0, -97.0, 0.0), (0.0, 0.0, 0.0), (0.0, 97.0, 0.0), (0.0, -48.5, 36.0), (0.0, 48.5, 36.0), (0.0, 0.0, 72.0))

# The hands' fight (layout cm): its spawner in the open yard south of the barn (whose walls stand at x -6800 to -6000, y
# 2600 to 3800: its doors face east, toward the yard's gate), the spots round the barn in the yard's open ground, at least
# 3 m from the barn's walls and the fence and 4.5 m from the windmill (-7600, 4100) and the stack by the doors (-6400,
# 3875). They come as the player comes within ACTIVATION: through the yard's gate, or loading the bales nearest it.
HANDS_AT = (-7300.0, 3300.0)
HANDS_SPOTS = ((-5700.0, 2700.0), (-5700.0, 3600.0), (-5700.0, 4400.0), (-6300.0, 4450.0), (-7300.0, 2700.0),
               (-8000.0, 2600.0), (-7400.0, 3400.0), (-8000.0, 3500.0), (-8000.0, 4450.0), (-7050.0, 4450.0))
HANDS_BASIC = 5
HANDS_ACTIVATION = 2200.0


class Ground:
    """The terrain's height (its tiles alone: never a tree, a rock or the barn) under a layout point."""

    def __init__(self, build):
        self.tiles = build_area.terrain_tiles(build.tag)
        if not self.tiles:
            build.warn('no terrain tiles tagged Ground (build the whole level first): Whitlock Fields stands at height 0')

    def __call__(self, x, y, default=0.0):
        hit = build_area.terrain_hit(self.tiles, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -20000.0))
        return default if hit is None else hit.z


def amos_spot(build, ground):
    """Where Amos leans: in the middle of the first span past his gate (its post, on the gate, at his right as he faces
    across the fence), on the fence's line as build_area_dressing lays it and at its rail's foot (the section's own
    heights, sunk with it under a dip), facing as the section's front does: across the fence into his hayfield.
    (x, y, z, yaw), or None (with a warning)."""
    # Reloaded, as the editor keeps modules between runs: a gameplay build may come after the dressing's tables changed.
    dressing = importlib.reload(importlib.import_module('build_area_dressing'))
    head = dressing.run_head(build.source, build.layout['placements'], 'amosFence')
    if head is None:
        build.warn("build_area_dressing lays no section past Amos's gate (layout.json amosFence and the fields path, "
                   'its LINES entry): no Amos')
        return None
    a, b = head
    za, zb = dressing.span_heights(a, b, ground)
    t = SPAN * 0.5 / math.dist(a, b)
    return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, za + (zb - za) * t,
            dressing.kit_yaw(b[0] - a[0], b[1] - a[1]))


# ---------------------------------------------------------------------------
# Side 2
# ---------------------------------------------------------------------------

def place_amos(build, leans):
    """Amos leaning on his fence by the gate, there from Main 4's finish, his topics by Side 2's step."""
    cls = story.actor_class('AmosWhitlock')
    if cls is None or leans is None:
        # Without his span, amos_spot has said why.
        if cls is None:
            build.warn('no AmosWhitlock class (build the game module first): no Amos at his fence')
        return
    x, y, z, yaw = leans
    amos = build.place(cls, (x, y, z), yaw, label='AmosWhitlock', folder='Gameplay', tags=(AMOS_TAG,))
    amos.set_editor_property('shown_when', story.condition(after=[MAIN4]))
    amos.set_editor_property('sit_when', story.condition(after=[SIDE2]))
    # The first that holds is said: Side 2's later steps before its earlier ones, then after it (after Main 6 first).
    # Without a topic (Side 2 not running yet), the first meeting.
    talk = amos.get_editor_property('speaker_point')
    meet = story.lines(build, 'DA_Lines_AmosMeet')
    if meet:
        talk.set_editor_property('line_set', meet)
    talk.set_editor_property('topics', story.topics(
        build,
        (story.condition(during=SIDE2, from_step=THANKS_STEP), 'DA_Lines_AmosThanks'),
        (story.condition(during=SIDE2, from_step=FIGHT_STEP), 'DA_Lines_AmosHands'),
        (story.condition(during=SIDE2, from_step=LOAD_STEP), 'DA_Lines_AmosBales'),
        (story.condition(during=SIDE2), 'DA_Lines_AmosMeet'),
        (story.condition(after=[SIDE2, MAIN6]), 'DA_Lines_AmosAfterMain6'),
        (story.condition(after=[SIDE2]), 'DA_Lines_AmosFence'),
    ))
    build.log(f'Amos at his fence ({x:.0f}, {y:.0f}, {z:.0f}) facing {yaw:.0f}, after {MAIN4}; on the rail after {SIDE2}')


def stack_spots(build, ground, barn):
    """The stack's six places by the barn's big doors (world location, yaw), bottom row first."""
    frame = barn.get_actor_transform()
    yaw = barn.get_actor_rotation().yaw
    middle = frame.transform_location(unreal.Vector(BARN_FRONT + STACK_OUT, 0.0, 0.0))
    base = ground(middle.x, middle.y, middle.z)
    spots = []
    for out, side, up in STACK_SLOTS:
        at = frame.transform_location(unreal.Vector(BARN_FRONT + STACK_OUT + out, side, 0.0))
        spots.append((unreal.Vector(at.x, at.y, base + up), yaw))
    return spots


def remark(build, count, line_set):
    """A bale's remark for the level's count-th bale loaded, or None while its set isn't made."""
    lines = story.lines(build, line_set)
    if lines is None:
        return None
    made = unreal.HayBaleRemark()
    made.set_editor_property('loaded_count', count)
    made.set_editor_property('line_set', lines)
    return made


def place_bales(build, ground):
    """Side 2's six bales in the west field, each with its place in the stack by the barn."""
    cls = story.actor_class('HayBale')
    # Not story.placed(build, 'Barn'): that's the first barn, Ransom Farm's.
    barn = barn_actor(build)
    if cls is None or barn is None:
        if cls is None:
            build.warn('no HayBale class (build the game module first): no hay to load')
        return
    spots = stack_spots(build, ground, barn)
    remarks = [each for each in (remark(build, 1, 'DA_Lines_AmosBaleFirst'), remark(build, 3, 'DA_Lines_HobBaleThird'),
                                 remark(build, len(FIELD_BALES), 'DA_Lines_AmosBaleLast')) if each is not None]
    for index, ((x, y, yaw), (stacked_at, stacked_yaw)) in enumerate(zip(FIELD_BALES, spots)):
        bale = build.place(cls, (x, y, ground(x, y)), yaw, label=f'Whitlock_HayBale_{index + 1}', folder='Gameplay',
                           tags=(BALE_TAG,))
        bale.set_editor_property('load_when', story.condition(during=SIDE2, from_step=LOAD_STEP))
        bale.set_editor_property('loaded_when', story.condition(after=[SIDE2]))
        bale.set_editor_property('load_remarks', remarks)
        stacked = bale.get_editor_property('stacked')
        stacked.set_world_location_and_rotation(stacked_at, unreal.Rotator(roll=0.0, pitch=0.0, yaw=stacked_yaw), False, True)
    build.log(f'{len(FIELD_BALES)} hay bales in the west field, their stack by the Whitlock barn\'s doors, loaded from '
              f'{SIDE2}\'s second step ({len(remarks)} remarks)')


def barn_actor(build):
    """The Whitlock barn build_area.py placed (labelled whitlockBarn), or None (with a warning)."""
    if 'whitlockBarn' not in build.layout['placements']:
        build.warn('layout_computed.json has no placement whitlockBarn: the bales have no stack')
        return None
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if actor.get_actor_label() == 'whitlockBarn' and unreal.Name(build.tag) in actor.tags:
            return actor
    build.warn('no whitlockBarn placed (build the whole level first): the bales have no stack')
    return None


def place_hands(build, ground):
    """The hired hands' fight in the barn yard: five Unpaid and a Restless one, during Side 2's third step."""
    unpaid = story.actor_class('UnpaidCreature')
    cls = story.actor_class('EncounterSpawner')
    yard = story.layout_entry(build, 'obstacles', 'barnYardFence')
    if unpaid is None or cls is None or yard is None:
        if unpaid is None or cls is None:
            build.warn('no EncounterSpawner or UnpaidCreature class (build the game module first): no hired hands')
        return
    hx, hy = HANDS_AT
    hz = ground(hx, hy)
    hands = build.place(cls, (hx, hy, hz + 50.0), 0.0, label=f'Encounter_{HANDS}', folder='Gameplay')
    hands.set_editor_property('spawner_id', unreal.Name(HANDS))
    # "6, one of them Restless": five Basic and a Restless one, all in one wave.
    hands.set_editor_property('groups', [story.group(unpaid, HANDS_BASIC, 'BASIC'), story.group(unpaid, 1, 'RARE')])
    hands.set_editor_property('creature_tags', [unreal.Name(HANDS_TAG)])
    # Side 2's third step only (from 0: from 2, before 3): a fight already won doesn't come back on a later load.
    hands.set_editor_property('active_when', story.condition(during=SIDE2, from_step=FIGHT_STEP, before_step=THANKS_STEP))
    hands.set_editor_property('spawn_points', [unreal.Vector(x - hx, y - hy, 0.0) for x, y in HANDS_SPOTS])
    hands.set_editor_property('activation_radius', HANDS_ACTIVATION)
    hands.set_editor_property('hunt_on_spawn', True)
    # "Lives on the barn yard; gives up at the barn yard fence."
    hands.set_editor_property('ground_corners', [unreal.Vector(x, y, hz) for x, y in yard['polygon']])
    build.log(f'the hired hands ({HANDS}: {HANDS_BASIC} Unpaid and a Restless one) in the barn yard at ({hx:.0f}, {hy:.0f}), '
              f'during {SIDE2}\'s third step')


def place(build):
    """Everything above, in the Gameplay folder. Returns Hob's Side 2 perches for build_area_story's place_hob: none."""
    ground = Ground(build)
    place_amos(build, amos_spot(build, ground))
    place_bales(build, ground)
    place_hands(build, ground)
    return {}
