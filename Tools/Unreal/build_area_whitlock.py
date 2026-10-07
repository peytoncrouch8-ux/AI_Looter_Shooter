"""Side 2, "Unfinished Business" (Docs/Areas/RansomsRest.md), for build_area_story.py: Whitlock Fields dressed and Side 2's
pieces, placed from the layout's zone, obstacles, roads and placements (Art/Levels/RansomsRest/layout.json: the whitlock
zone, amosFence, barnYardFence, fieldWall, the fieldsPath road, the whitlockBarn and windmill placements) and on the
terrain's tiles, so they follow the level whenever it's rebuilt. build_area_story's place() calls place() here (reloaded
each run, as the editor keeps modules between runs); it uses build_area_story's helpers. Everything goes in the area's
Gameplay folder, so a "gameplay" build places it all again; a piece whose model or class isn't there yet is left out
with a warning.

Whitlock Fields ("Hayfields split by a stone field wall, with hay bales, Amos's fence along the north, and the Whitlock
barn and the existing windmill inside a yard fence"; the barn and the windmill are build_area.py's):
- Amos's fence (Art/Models/Props/Fences.py's FenceRail, 3 m segments with posts every 1.5 m, a FencePost at each run's
  end) along the obstacle amosFence, with his gate left open where the fields path crosses it (3 m, the path's width).
- The barn yard's fence round the obstacle barnYardFence, its gate open at (48, -60) where the fields path ends.
- The stone field wall (StoneWall, StoneWallEnd at both ends) along the obstacle fieldWall, splitting the hay.
- Round bales lying out in the east field, the hay he never got in (dressing: HayBale_Round, nothing to do with them).
  Segments run along their chords between evenly spaced joints on the obstacle's line, broken at its corners, each
  stretched to its chord (93-103%; a corner's short leg 75%) and rolled with the ground; all tagged Obstacle (the minimap draws them, the scatter
  keeps grass off them). Each run's first segment starts on its own post; a FencePost closes it.

Side 2:
- Amos (AAmosWhitlock tagged Speaker_Amos) leaning on his fence by his gate: in the middle of the first span past the gate
  (its post at his right), on the fence's line, facing across it into his hayfield, the span laid at the kit's true 1.5 m
  so his pose (fitted to FenceRail) meets its rail. There from Main 4's finish; on the rail after Side 2. Talked to: the
  first meeting (Side 2's first step), the bales (its second), the hands (its third), his thanks (its last), and after it,
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

# The fence and wall kit (Fences.py): a segment runs from its pivot (its first post, on the ground) along the model's +X,
# which is the actor's -Y in Unreal, 3 m to where the next one starts; its second post is 1.5 m along.
FENCE_RAIL = '/Game/Art/Props/SM_FenceRail'
FENCE_POST = '/Game/Art/Props/SM_FencePost'
WALL = '/Game/Art/Props/SM_StoneWall'
WALL_END = '/Game/Art/Props/SM_StoneWallEnd'
ROUND_BALE = '/Game/Art/Props/SM_HayBale_Round'
SEGMENT = 300.0
SPAN = 150.0
# A line's corner turns more than this (degrees): its segments end there rather than cut across it.
CORNER = 20.0
# The gates: as wide as the fields path (cm); the barn yard's where its note puts it (layout cm).
GATE_WIDTH = 300.0
YARD_GATE = (-6000.0, 4800.0)
# Amos's gate where the fields path crosses his fence; without the road, where its note puts it.
AMOS_GATE = (-3800.0, 5300.0)

# The round bales lying out in the east field (layout cm, yaw), clear of the creek's bottom and the field wall.
ROUND_BALES = ((-4600.0, 6700.0, 30.0), (-5400.0, 7000.0, -15.0), (-6800.0, 6600.0, 70.0), (-7600.0, 6300.0, 10.0))

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


# ---------------------------------------------------------------------------
# Lines on the ground
# ---------------------------------------------------------------------------

def length(points):
    return sum(math.dist(a, b) for a, b in zip(points, points[1:]))


def point_at(points, d):
    """The point d along a polyline (clamped to its ends)."""
    last = len(points) - 2
    for index, (a, b) in enumerate(zip(points, points[1:])):
        step = math.dist(a, b)
        if d <= step or index == last:
            t = 0.0 if step < 1e-6 else max(0.0, min(1.0, d / step))
            return a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
        d -= step
    return points[-1]


def along(points, p):
    """How far along a polyline its point nearest p is."""
    best, best_d, walked = None, 0.0, 0.0
    for a, b in zip(points, points[1:]):
        dx, dy = b[0] - a[0], b[1] - a[1]
        step2 = dx * dx + dy * dy
        t = 0.0 if step2 < 1e-6 else max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / step2))
        gap = math.dist(p, (a[0] + dx * t, a[1] + dy * t))
        if best is None or gap < best:
            best, best_d = gap, walked + math.sqrt(step2) * t
        walked += math.sqrt(step2)
    return best_d


def cut(points, d0, d1):
    """The polyline from d0 along it to d1, with its corners between."""
    out = [point_at(points, d0)]
    walked = 0.0
    for a, b in zip(points, points[1:]):
        walked += math.dist(a, b)
        if d0 < walked < d1:
            out.append(b)
    out.append(point_at(points, d1))
    return out


def crossing(points, road):
    """Where a road's path first crosses a polyline, or None."""
    for a, b in zip(points, points[1:]):
        for c, d in zip(road, road[1:]):
            r = (b[0] - a[0], b[1] - a[1])
            s = (d[0] - c[0], d[1] - c[1])
            den = r[0] * s[1] - r[1] * s[0]
            if abs(den) < 1e-9:
                continue
            t = ((c[0] - a[0]) * s[1] - (c[1] - a[1]) * s[0]) / den
            u = ((c[0] - a[0]) * r[1] - (c[1] - a[1]) * r[0]) / den
            if 0.0 <= t <= 1.0 and 0.0 <= u <= 1.0:
                return a[0] + r[0] * t, a[1] + r[1] * t
    return None


def legs(points):
    """The polyline in legs between its corners (turns sharper than CORNER)."""
    out, leg = [], [points[0]]
    for i in range(1, len(points) - 1):
        leg.append(points[i])
        a, b, c = points[i - 1], points[i], points[i + 1]
        h0 = math.atan2(b[1] - a[1], b[0] - a[0])
        h1 = math.atan2(c[1] - b[1], c[0] - b[0])
        turn = abs(math.degrees((h1 - h0 + math.pi) % (2.0 * math.pi) - math.pi))
        if turn > CORNER:
            out.append(leg)
            leg = [points[i]]
    leg.append(points[-1])
    out.append(leg)
    return out


def joints(leg, first=0.0):
    """Evenly spaced joints along a leg, about SEGMENT apart; with first, the first joint pair exactly that far apart."""
    total = length(leg)
    marks = [0.0]
    start = 0.0
    if first and total > first + SPAN:
        marks.append(first)
        start = first
    count = max(1, round((total - start) / SEGMENT))
    marks += [start + (total - start) * k / count for k in range(1, count + 1)]
    return [point_at(leg, d) for d in marks]


def kit_yaw(dx, dy):
    """The yaw that runs a kit segment (along its actor's -Y) from its pivot toward (dx, dy)."""
    return math.degrees(math.atan2(dx, -dy))


class Ground:
    """The terrain's height (its tiles alone: never a tree, a rock or the barn) under a layout point."""

    def __init__(self, build):
        self.tiles = build_area.terrain_tiles(build.tag)
        if not self.tiles:
            build.warn('no terrain tiles tagged Ground (build the whole level first): Whitlock Fields stands at height 0')

    def __call__(self, x, y, default=0.0):
        hit = build_area.terrain_hit(self.tiles, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -20000.0))
        return default if hit is None else hit.z


def place_segment(build, mesh, a, b, za, zb, label, tags=('Obstacle',)):
    """One kit segment from a to b on the ground: turned along the chord, rolled with the slope, stretched to its length."""
    dx, dy = b[0] - a[0], b[1] - a[1]
    chord = math.hypot(dx, dy)
    yaw = kit_yaw(dx, dy)
    stretch = math.hypot(chord, zb - za) / SEGMENT
    made = build.place(mesh, (a[0], a[1], za), yaw, label=label, folder='Gameplay', scale=(1.0, stretch, 1.0), tags=tags)
    # The segment's far end (its -Y) rises with a positive roll.
    roll = math.degrees(math.atan2(zb - za, chord))
    if abs(roll) > 0.05:
        made.set_actor_rotation(unreal.Rotator(roll=roll, pitch=0.0, yaw=yaw), False)
    return made, yaw


def lay_run(build, ground, points, mesh, name, first=0.0, end=None, start=None):
    """A run of segments along points (broken at its corners), the end piece closing it (end: (mesh, turned)) and one at
    its start (start: (mesh, turned)). Returns how many pieces, and the first segment's (pivot, far joint, yaw)."""
    count = 0
    head = None
    for leg_index, leg in enumerate(legs(points)):
        spots = joints(leg, first if leg_index == 0 else 0.0)
        for a, b in zip(spots, spots[1:]):
            za, zb = ground(*a), ground(*b)
            _, yaw = place_segment(build, mesh, a, b, za, zb, f'{name}_{count + 1:02d}')
            if head is None:
                head = (a, b, yaw, za, zb)
            count += 1
    tail = points[-1]
    if end is not None and end[0] is not None:
        prev = points[-2]
        yaw = kit_yaw(tail[0] - prev[0], tail[1] - prev[1]) + (180.0 if end[1] else 0.0)
        build.place(end[0], (tail[0], tail[1], ground(*tail)), yaw, label=f'{name}_End', folder='Gameplay', tags=('Obstacle',))
        count += 1
    if start is not None and start[0] is not None:
        lead, nxt = points[0], points[1]
        yaw = kit_yaw(nxt[0] - lead[0], nxt[1] - lead[1]) + (180.0 if start[1] else 0.0)
        build.place(start[0], (lead[0], lead[1], ground(*lead)), yaw, label=f'{name}_Start', folder='Gameplay', tags=('Obstacle',))
        count += 1
    return count, head


def load_mesh(build, path, what):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        build.warn(f'no {path} yet (import its model): {what} left out')
        return None
    return unreal.load_asset(path)


# ---------------------------------------------------------------------------
# Whitlock Fields dressed
# ---------------------------------------------------------------------------

def fence_runs(points, gate, closed):
    """A fence's runs either side of its gate (3 m open where the path crosses), as point lists; a closed one is a single
    run from the gate's far side round to its near side. Returns the runs, and which one starts at the gate's far post."""
    if closed:
        loop = list(points) + [points[0]]
        total = length(loop)
        at = along(loop, gate)
        # Round the loop from the gate's far post back to its near one.
        doubled = loop + loop[1:]
        return [cut(doubled, at + GATE_WIDTH * 0.5, at + total - GATE_WIDTH * 0.5)], None
    total = length(points)
    at = along(points, gate)
    runs, after = [], None
    if at - GATE_WIDTH * 0.5 > SPAN:
        runs.append(cut(points, 0.0, at - GATE_WIDTH * 0.5))
    if total - (at + GATE_WIDTH * 0.5) > SPAN:
        after = len(runs)
        runs.append(cut(points, at + GATE_WIDTH * 0.5, total))
    return runs, after


def place_amos_fence(build, ground):
    """Amos's fence with his gate where the fields path crosses it. Returns where Amos leans: (x, y, z, yaw), or None."""
    fence = story.layout_entry(build, 'obstacles', 'amosFence')
    rail = load_mesh(build, FENCE_RAIL, "Amos's fence")
    post = load_mesh(build, FENCE_POST, 'the fence posts that close a run')
    if fence is None or rail is None:
        return None
    path = [tuple(p) for p in fence['path']]
    road = story.layout_entry(build, 'roads', 'fieldsPath')
    gate = crossing(path, road['path']) if road else None
    if gate is None:
        build.warn(f'the fields path doesn\'t cross Amos\'s fence: his gate stands where its note puts it {AMOS_GATE}')
        gate = AMOS_GATE
    runs, after = fence_runs(path, gate, closed=False)
    pieces = 0
    leans = None
    for index, run in enumerate(runs):
        # The run past the gate starts with a span at the kit's own 1.5 m (no stretch): Amos leans in it.
        count, head = lay_run(build, ground, run, rail, f'AmosFence_{index + 1}', first=SEGMENT if index == after else 0.0,
                              end=(post, False))
        pieces += count
        if index == after and head is not None:
            a, b, yaw, za, zb = head
            dx, dy = (b[0] - a[0]) / math.dist(a, b), (b[1] - a[1]) / math.dist(a, b)
            # The middle of the first span: its post (on the gate) at his right as he faces across the fence.
            x, y = a[0] + dx * SPAN * 0.5, a[1] + dy * SPAN * 0.5
            z = za + (zb - za) * (SPAN * 0.5) / math.dist(a, b)
            leans = (x, y, z, yaw)
    build.log(f'Amos\'s fence: {pieces} pieces in {len(runs)} runs, his gate at ({gate[0]:.0f}, {gate[1]:.0f})')
    return leans


def place_yard_fence(build, ground):
    """The barn yard's fence round the Whitlock barn and the windmill, its gate open where the fields path ends."""
    fence = story.layout_entry(build, 'obstacles', 'barnYardFence')
    rail = load_mesh(build, FENCE_RAIL, 'the barn yard fence')
    post = load_mesh(build, FENCE_POST, 'the fence posts that close a run')
    if fence is None or rail is None:
        return
    runs, _ = fence_runs([tuple(p) for p in fence['polygon']], YARD_GATE, closed=True)
    pieces = sum(lay_run(build, ground, run, rail, f'YardFence_{index + 1}', end=(post, False))[0]
                 for index, run in enumerate(runs))
    build.log(f'the barn yard fence: {pieces} pieces, its gate at {YARD_GATE}')


def place_field_wall(build, ground):
    """The low stone field wall splitting the hay, finished at both ends."""
    wall = story.layout_entry(build, 'obstacles', 'fieldWall')
    piece = load_mesh(build, WALL, 'the field wall')
    end = load_mesh(build, WALL_END, "the field wall's finished ends")
    if wall is None or piece is None:
        return
    count, _ = lay_run(build, ground, [tuple(p) for p in wall['path']], piece, 'FieldWall', end=(end, False),
                       start=(end, True))
    build.log(f'the field wall: {count} pieces')


def place_round_bales(build, ground):
    """The hay he never got in: round bales lying out in the east field."""
    bale = load_mesh(build, ROUND_BALE, 'the round bales in the east field')
    if bale is None:
        return
    for index, (x, y, yaw) in enumerate(ROUND_BALES):
        build.place(bale, (x, y, ground(x, y)), yaw, label=f'Whitlock_RoundBale_{index + 1}', folder='Gameplay',
                    tags=('Obstacle',))
    build.log(f'{len(ROUND_BALES)} round bales in the east field')


# ---------------------------------------------------------------------------
# Side 2
# ---------------------------------------------------------------------------

def place_amos(build, leans):
    """Amos leaning on his fence by the gate, there from Main 4's finish, his topics by Side 2's step."""
    cls = story.actor_class('AmosWhitlock')
    if cls is None or leans is None:
        if cls is None:
            build.warn('no AmosWhitlock class (build the game module first): no Amos at his fence')
        else:
            build.warn('Amos\'s fence isn\'t placed: no Amos')
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
    leans = place_amos_fence(build, ground)
    place_yard_fence(build, ground)
    place_field_wall(build, ground)
    place_round_bales(build, ground)
    place_amos(build, leans)
    place_bales(build, ground)
    place_hands(build, ground)
    return {}
