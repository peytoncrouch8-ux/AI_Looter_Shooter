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
  meeting (Side 2's offer), the bales (its first step), the hands (its second), his thanks (its last), and after it,
  sitting, waiting for the saint.
- Three hay bales (AHayBale tagged HayBale; BALES_USED of FIELD_BALES) lying out in the west field, each with its place in
  the stack's bottom row under the hoist by the Whitlock barn's big doors: loaded by holding Interact from Side 2's first
  step, all in the stack once it's done. Amos remarks on the first and the last (the last: "...Who's that out on the
  road?"), Hob on the second.
- His old hired hands (AEncounterSpawner WhitlockHands) in the barn yard: seven Unpaid and a Restless one, tagged
  Unpaid_WhitlockHands, on during Side 2's second step only, appearing round the barn as the player comes within 22 m of
  the yard's middle (its gate is 20 m off) and coming for them, hunting only inside the yard's fence (and its 2 m margin),
  so the fight stays in the yard. As the last falls Amos says who they were (DA_Lines_AmosRoad, its cleared_lines).
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

# Side 2's steps, counted from 0: load the bales, drive off the hands, talk to Amos (the missions' data; the road talk
# went into the fight's end on 2026-10-21: the hands' spawner says DA_Lines_AmosRoad as the last of them falls).
LOAD_STEP = 0
FIGHT_STEP = 1
THANKS_STEP = 2
# Three of the field's bales are loaded now (the user's cut of the chores): only these many are placed, in the stack's
# bottom row. The rest of FIELD_BALES stays written down for a longer chore another time.
BALES_USED = 3
# Who the hands were, said once they're beaten (AEncounterSpawner::ClearedLines; STORY phase 2).
HANDS_CLEARED_LINES = 'DA_Lines_AmosRoad'

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
# Seven and a Restless one since the user's bigger fights (2026-10-21; five until then).
HANDS_BASIC = 7
HANDS_ACTIVATION = 2200.0
# Amos's own barn (SM_RR_Barn_Whitlock, 2026-10-13, patches/town.json): 15 x 8 m, its pivot the middle of the hay bay's
# open front at (-6150, 3600) facing east, so its walls stand at x -7150 to -5650, y 2800 to 3600 and the yard opens in
# front of the bay toward the gate. The fight's spawner stands in that yard and its spots keep 3 m off the barn and the
# fence, 4.5 m off the windmill and out of the bay where the stack is. The bales' stack is in the bay (SOCKET_BaleStack).
RR_BARN_PREFIX = 'SM_RR_Barn'
BALE_STACK_SOCKET = 'BaleStack'
RR_HANDS_AT = (-6900.0, 4100.0)
RR_HANDS_SPOTS = ((-5750.0, 4350.0), (-6300.0, 4450.0), (-6850.0, 4350.0), (-7150.0, 4450.0), (-8000.0, 4450.0),
                  (-8000.0, 3400.0), (-7600.0, 3300.0), (-7700.0, 2600.0), (-6400.0, 4000.0))


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
    talk.set_editor_property('topics', story.texts_topics(build, AMOS_TAG))
    build.log(f'Amos at his fence ({x:.0f}, {y:.0f}, {z:.0f}) facing {yaw:.0f}, after {MAIN4}; on the rail after {SIDE2}')


def stack_spots(build, ground, barn):
    """The stack's six places by the barn's big doors (world location, yaw), bottom row first. Amos's own barn
    (SM_RR_Barn_Whitlock, since 2026-10-13) says where in its hay bay: its SOCKET_BaleStack is the stack's middle on the
    earth floor, its rows running along the bay's open front."""
    frame = barn.get_actor_transform()
    yaw = barn.get_actor_rotation().yaw
    mesh = barn.static_mesh_component
    if mesh is not None and mesh.does_socket_exist(BALE_STACK_SOCKET):
        local = mesh.get_socket_transform(BALE_STACK_SOCKET, unreal.RelativeTransformSpace.RTS_COMPONENT).translation
        middle = frame.transform_location(local)
        base = ground(middle.x, middle.y, middle.z)
        spots = []
        for out, side, up in STACK_SLOTS:
            at = frame.transform_location(unreal.Vector(local.x + out, local.y + side, 0.0))
            spots.append((unreal.Vector(at.x, at.y, base + up), yaw))
        return spots
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
    bales = FIELD_BALES[:BALES_USED]
    remarks = [each for each in (remark(build, 1, 'DA_Lines_AmosBaleFirst'), remark(build, 2, 'DA_Lines_HobBaleThird'),
                                 remark(build, len(bales), 'DA_Lines_AmosBaleLast')) if each is not None]
    for index, ((x, y, yaw), (stacked_at, stacked_yaw)) in enumerate(zip(bales, spots)):
        bale = build.place(cls, (x, y, ground(x, y)), yaw, label=f'Whitlock_HayBale_{index + 1}', folder='Gameplay',
                           tags=(BALE_TAG,))
        bale.set_editor_property('load_when', story.condition(during=SIDE2, from_step=LOAD_STEP))
        bale.set_editor_property('loaded_when', story.condition(after=[SIDE2]))
        bale.set_editor_property('load_remarks', remarks)
        stacked = bale.get_editor_property('stacked')
        stacked.set_world_location_and_rotation(stacked_at, unreal.Rotator(roll=0.0, pitch=0.0, yaw=stacked_yaw), False, True)
    build.log(f'{len(bales)} hay bales in the west field, their stack by the Whitlock barn\'s doors, loaded from '
              f'{SIDE2}\'s first step ({len(remarks)} remarks)')


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


# Barn Doors (FIGHTS lane 6, 2026-10-28; Main's call: no walls here): the hands' fight is a set piece in two pushes. First 5 and a
# Restless leader in the yard; then, once 3 are left or 30 s have gone, 4 more rise at the barn's front (the bay's mouth; the
# barn has no hay-loft door, BarnWhitlock.py, so its Bell-Ringer stands in the yard at the barn's end instead of 4 m up) and a
# Bell-Ringer tolls from there: 11 with it. The Restless leader's spots are the yard's; the barn's front is traced from the
# barn's own frame (its front faces local -Y), 3.5 m out, at these offsets along it (cm).
HANDS_PUSH_AFTER = 30.0
BARN_DOOR_OFFSETS = (-400.0, -150.0, 150.0, 400.0)
BARN_DOOR_OUT = 350.0
HANDS_RINGER_AT = (-6300.0, 4600.0)
HANDS_BEATS = (
    dict(wave=1, speaker='Hob', line="Amos's old hands, sunshine. Still working for no wages.",
         sting='Boss.Phase', cue='Creature.Unpaid.Shriek', shake=0.15, shakeSeconds=0.5),
    dict(wave=2, speaker='Hob', line='More out of the bay! There is a bell in the yard, too. Mind your back.',
         sting='Creature.RankSting', shake=0.15, shakeSeconds=0.5),
)


def place_hands(build, ground):
    """The hired hands' fight in the barn yard as the set piece Barn Doors: 5 Unpaid and a Restless leader, then 4 more out of
    the barn's front and a Bell-Ringer; during Side 2's third step."""
    yard = story.layout_entry(build, 'obstacles', 'barnYardFence')
    if yard is None:
        return
    # The yard round Amos's own barn when it stands (its bay faces the gate), else round the older barn.
    barn = next((actor for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
                 if actor.get_actor_label() == 'whitlockBarn' and unreal.Name(build.tag) in actor.tags), None)
    mesh = barn.static_mesh_component.static_mesh if barn is not None and barn.static_mesh_component else None
    own_barn = mesh is not None and mesh.get_name().startswith(RR_BARN_PREFIX)
    (hx, hy), spots = (RR_HANDS_AT, RR_HANDS_SPOTS) if own_barn else (HANDS_AT, HANDS_SPOTS)
    first = [list(spot) for spot in spots[:6]]
    if own_barn:
        frame = barn.get_actor_transform()
        front = []
        for along in BARN_DOOR_OFFSETS:
            at = frame.transform_location(unreal.Vector(along, -BARN_DOOR_OUT, 0.0))
            front.append([at.x, at.y])
    else:
        front = [list(spot) for spot in spots[6:10]] or first[:4]
    entry = {
        'id': HANDS, 'spawner': 'SetPieceSpawner', 'pass': 'valley',
        'where': 'the barn yard: Barn Doors, Side 2 (a set piece: five and a Restless leader in the yard, then four out of the barn and a Bell-Ringer)',
        # Side 2's second step only (from 0: from 1, before 2): they come up the road as the last bale goes in, and a fight
        # already won doesn't come back on a later load.
        'when': {'during': SIDE2, 'fromStep': FIGHT_STEP, 'beforeStep': THANKS_STEP},
        'groups': [{'class': 'UnpaidCreature', 'count': 5, 'rank': 'Basic', 'firstWave': 1, 'lastWave': 1},
                   {'class': 'UnpaidCreature', 'count': 1, 'rank': 'Rare', 'firstWave': 1, 'lastWave': 1, 'leader': True},
                   {'class': 'UnpaidCreature', 'count': 4, 'rank': 'Basic', 'firstWave': 2, 'lastWave': 2}],
        'tags': [HANDS_TAG], 'center': [hx, hy], 'spots': first + front, 'huntOnSpawn': True, 'entrance': 'Rise',
        'waves': {'count': 2, 'interval': HANDS_PUSH_AFTER, 'waitForClear': False, 'maxAlive': 0},
        # "Lives on the barn yard; gives up at the barn yard fence."
        'ground': {'polygon': yard['polygon']},
        'setPiece': {
            'doneFlag': 'SetPiece.BarnDoors', 'waves': 2, 'pushWhenLeft': 3, 'pushAfter': HANDS_PUSH_AFTER,
            'springRadius': HANDS_ACTIVATION,
            'beats': [dict(HANDS_BEATS[0], spots=first), dict(HANDS_BEATS[1], spots=front)],
            'placed': [{'class': 'BellRingerCreature', 'rank': 'Basic', 'wave': 2, 'at': list(HANDS_RINGER_AT)}],
            'reward': [hx, hy],
        },
    }
    fights = importlib.reload(importlib.import_module('build_area_fights_mountain'))
    fights.place_one(build, entry, None)
    # Who they were, said as the last of them falls, after the fight's noise (Main's call, 2026-10-21): the set piece is an
    # encounter spawner too. Guarded: a game module from before AEncounterSpawner::ClearedLines places them without it.
    said = story.lines(build, HANDS_CLEARED_LINES)
    hands = next((actor for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
                  if actor.get_actor_label() == f'Encounter_{HANDS}'), None)
    if said is not None and hands is not None:
        try:
            hands.set_editor_property('cleared_lines', said)
        except Exception as error:
            build.warn(f'no EncounterSpawner.cleared_lines ({error}): build the game module; Amos says nothing as they fall')
    build.log(f"the hired hands ({HANDS}, the set piece Barn Doors: 5 Unpaid and a Restless leader, then 4 and a Bell-Ringer) in the "
              f"barn yard at ({hx:.0f}, {hy:.0f}), during {SIDE2}'s second step")


def place(build):
    """Everything above, in the Gameplay folder. Returns Hob's Side 2 perches for build_area_story's place_hob: none."""
    ground = Ground(build)
    place_amos(build, amos_spot(build, ground))
    place_bales(build, ground)
    place_hands(build, ground)
    return {}
