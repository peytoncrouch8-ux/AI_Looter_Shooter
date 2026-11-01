"""Main 4, "A Coal Under Ash" (the Wardens story, 2026-11-01; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 6, lane
MAIN45), its new world pieces at the Chapel of Saint Ada, for build_area_story.py: its place() calls place() here (reloaded
each run, as the editor keeps modules between runs) after the chapel (build_area_chapel.py: the yard's set piece, the bell,
the Reliquary, Aldana's door) and Landfall (the barge the tender comes from). Everything goes in the area's Gameplay/Main4
folder, so a "gameplay" build places it all again; a piece whose class, model or ground isn't there yet is left out with a
warning.

What it places, from the built chapel (SM_Chapel, Art/Models/Buildings/Chapel.py: its sockets and its west wall's first
window, found by traces), the layout's churchyard fence, the dressing's footprints and Landfall's barge plan:
- The Company's hose in Saint Ada's side window (AChapelWindowHose, Hose_ChapelWindow): over the sill of the west wall's
  window nearest the apse (Chapel.py LEFT_BAYS 1), in through the broken glass and across the sanctuary to the Reliquary's
  ember, her light drawn out along it in pulses. Shown during Main 4; pulled with a 2 s hold at the sill in its step 1 (it
  whips out of the window and coughs smog), or freed by its cart stopping; out of the window after.
- Its cart (ACompanyHoseCart, HoseCart_ChapelYard) on the west lane by the window, its reel to the wall, its hose run to the
  sill: pumping during Main 4, stoppable (valve or tank) in step 1, stopped after; Vane complains from the lychgate's horn.
- Vane's loudspeaker by the lychgate (ACompanyLoudspeaker, Loudspeaker_Lychgate) just outside the churchyard fence's
  gateway, its horns at the chapel: his word as the barge answers the bell (story_texts.LOUDSPEAKER_CUES), and as the hose
  comes out (the cart's stop_broadcast).
- The barge's answer: a Company tender (ACompanyTenderDrop, Tender_Chapel) that swings off the barge's side as Main 4's
  step 4 begins and hangs over the yard, 15 m up, clear of the spire; its four ropes the drop's (ARappelSpawner
  BellAnswered): four riflemen (level 3) and two hose-men (level 4) slide down onto the yard's open ground and come for the
  player, hunting inside the fence. The hover spot is searched so every rope's foot is inside the fence, off the chapel
  and clear of the dressing's graves and trees (DROP_CLEAR).
- Brother Finch at the Reliquary (AWardenCrewMember, Keeper, at his station facing it; Speaker_FinchChapel) from Main 4's
  turn-in: he keeps the saint alive by hand until Abel is back; Main 5 is turned in to him.
- That night's marks: the hose-cart's drag furrows from the gateway to the cart (MI_Decal_Furrow) and a soot mark on the
  front door Abel held (stand-in MI_Decal_ImpactScar until a soot handprint is drawn), from Main 1 on.
Returns Hob's Main 4 spots for build_area_story.place_hob: {'lychgate': (x, y, z, yaw) or None} (the post's top; unused by
story_texts.MAIN45_PERCHES today, kept for a later perch).
"""
import importlib
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Main4'
MAIN1, MAIN4, MAIN5 = 'Main1', 'Main4', 'Main5'

# ---------------------------------------------------------------------------
# Names (the C++ and the mission data's; one place)
# ---------------------------------------------------------------------------

HOSE_TAG = 'Hose_ChapelWindow'
CART_TAG = 'HoseCart_ChapelYard'
CART_TAGS = ('HoseCart', 'Obstacle')
LYCHGATE = 'Loudspeaker_Lychgate'
TENDER_TAG = 'Tender_Chapel'
DROP = 'BellAnswered'
DROP_TAG = 'Dredger_BellAnswered'
FINCH_TAG = 'Speaker_FinchChapel'
RELIQUARY_TAG = 'Reliquary_Chapel'
# Line sets (create_story_lines.py).
HOSE_COMPLAINT = 'DA_Lines_Main4VaneHose'

# ---------------------------------------------------------------------------
# Decals (existing materials; the soot handprint is a stand-in until drawn)
# ---------------------------------------------------------------------------

FURROW_DECAL = '/Game/Art/Effects/GraveFight/MI_Decal_Furrow'
SOOT_DECAL = '/Game/Art/Effects/GraveFight/MI_Decal_ImpactScar'

# ---------------------------------------------------------------------------
# Numbers (the chapel's own frame, cm: +X its front, the tower and door; -X its apse; +Y the west side, away from the
# vestry; heights over the chapel's pivot on the ground)
# ---------------------------------------------------------------------------

# The west wall's window nearest the apse (Chapel.py: the left wall at x -3.2 m, bay 1 from the sanctuary end, its middle
# 4.4 m back of the nave's middle line; the sill about 1.75 m over the ground): its middle along the wall, and the outer
# face. What's left of its glass (Chapel.py lancet: a few whole panes and shards, a glazing bar down the middle) blocks
# most of the opening: probed 2026-11-01, the lowest gap is 20 cm off the middle at 1.8 m, the rest is glass. So the sill is
# found by lines from the yard across the window's width (WINDOW_HALF either side of its middle, every WINDOW_STEP) at
# SILL_PROBE's heights, and the hose goes in at the lowest gap; SILL_UP and the middle when none passes.
WINDOW = (-440.0, 320.0)
WINDOW_HALF = 70.0
WINDOW_STEP = 5.0
SILL_UP = 175.0
SILL_PROBE = (100.0, 320.0, 5.0)           # heights tried, from, to, step
# The hose inside: from the sill down to the nave's floor this far under it, then along to the Reliquary's ember.
INSIDE_FLOOR_DROP = 132.0
# Pulled, its end lies on the ground outside here (from the sill, the hose's own frame: +X out of the wall).
PULLED_REST = (260.0, -120.0, -170.0)
HOSE_HOLD = 2.0
# The cart on the west lane, its reel toward the window (yaw +90 in the chapel's frame: its handles away from the wall).
CART_AT = (-440.0, 950.0)
CART_TURN = 90.0
# The drop: the tender hangs this high over the yard's ground, its hull this long and wide (SM_CompanyTender: 12 m by
# 4.4 m, 4.7 m high, its pivot under its keel); its ropes hang from these points under its keel (the hull's frame, cm: +X
# its bow) and their feet must be inside the fence by DROP_MARGIN, off the chapel's body (CHAPEL_BOX) and DROP_CLEAR from
# the dressing and the cart. The yard is mostly graves (rows 1.6 m apart, every 3 m: nothing 1.5 m clear among them), so
# the open ground is the lanes beside the nave and the strip behind the apse; the west lane (2 m clear between the wall's
# footing and the first grave row) takes the drop: the ropes in a staggered line along the keel, 8.4 m from first to
# last, so the hull hangs along the lane. The spire (SPIRE_BOX, SPIRE_PAD clear: the hull rolls as it bobs) is the only
# thing at the tender's height.
HOVER_UP = 1500.0
ROPE_ANCHORS = ((420.0, 35.0, 10.0), (140.0, -35.0, 10.0), (-140.0, 35.0, 10.0), (-420.0, -35.0, 10.0))
HULL_HALF = (600.0, 222.0)
CHAPEL_BOX = (-820.0, 820.0, -660.0, 420.0)        # X min/max, Y min/max: nave, tower, apse, vestry and their footing
SPIRE_BOX = (330.0, 790.0, -230.0, 230.0)
SPIRE_PAD = 150.0
DROP_MARGIN = 150.0
DROP_CLEAR = 150.0
HOVER_GRID = (-1100.0, 1000.0, -1600.0, 1600.0, 50.0)
# Its run in: from the barge's side toward the chapel, this far out from the barge's keel and this much higher.
TENDER_FROM_OUT = 2600.0
TENDER_FROM_UP = 400.0
TENDER_FLY = 9.0
TENDER_LEAVE_AFTER = 8.0
# BellAnswered: four riflemen (level 3), two hose-men (level 4), one drop; it waits for the tender to hover.
DROP_GROUPS = (('DredgerRifleman', 4, 3), ('DredgerHoseMan', 2, 4))
DROP_ACTIVATION = 6000.0
DROP_SLIDE = 2.2
# The lychgate's loudspeaker: outside the fence's gateway, this far out and along it (layout cm off the gateway's middle,
# away from the yard), its horns at the chapel.
GATE_SPEAKER_OUT = 320.0
GATE_SPEAKER_ALONG = 260.0
# Finch at the Reliquary: this far in front of the plinth (toward the nave), facing it; his capsule's middle this far over
# the floor; the sanctuary's floor over the pivot when no trace finds it (Chapel.py FLOOR + SANCT).
FINCH_OUT = 140.0
FINCH_HALF_HEIGHT = 90.0
SANCTUARY_UP = 79.0
# The marks: furrows every FURROW_STEP from the gateway toward the cart; the soot mark this far under the door's head.
FURROW_STEP = 380.0
FURROW_SIZE = (40.0, 160.0, 70.0)
SOOT_UNDER = 120.0
SOOT_SIZE = (12.0, 34.0, 26.0)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def texts():
    import story_texts
    return importlib.reload(story_texts)


def main2():
    return importlib.reload(importlib.import_module('build_area_main2'))


def chapel_module():
    return importlib.import_module('build_area_chapel')


def load(path):
    return unreal.load_asset(path) if path and unreal.EditorAssetLibrary.does_asset_exist(path) else None


def remove_stale(build, label):
    return main2().remove_stale(build, label)


class Frame:
    """The chapel's frame: points (cm, its own axes) to the world and back, its yaw, its pivot's height."""

    def __init__(self, chapel):
        self.chapel = chapel
        at = chapel.get_actor_location()
        self.x, self.y, self.z = at.x, at.y, at.z
        self.yaw = chapel.get_actor_rotation().yaw
        self.c, self.s = math.cos(math.radians(self.yaw)), math.sin(math.radians(self.yaw))

    def world(self, fx, fy, up=0.0):
        return (self.x + self.c * fx - self.s * fy, self.y + self.s * fx + self.c * fy, self.z + up)

    def local(self, wx, wy):
        dx, dy = wx - self.x, wy - self.y
        return (self.c * dx + self.s * dy, -self.s * dx + self.c * dy)


def inside(point, polygon):
    x, y = point
    hit = False
    for (ax, ay), (bx, by) in zip(polygon, list(polygon[1:]) + [polygon[0]]):
        if (ay > y) != (by > y) and x < ax + (y - ay) * (bx - ax) / ((by - ay) or 1e-9):
            hit = not hit
    return hit


def edge_distance(point, polygon):
    best = None
    for (ax, ay), (bx, by) in zip(polygon, list(polygon[1:]) + [polygon[0]]):
        dx, dy = bx - ax, by - ay
        t = max(0.0, min(1.0, ((point[0] - ax) * dx + (point[1] - ay) * dy) / max(dx * dx + dy * dy, 1e-6)))
        d = math.hypot(ax + dx * t - point[0], ay + dy * t - point[1])
        best = d if best is None else min(best, d)
    return best or 0.0


def footprints(build):
    """The dressing's pieces (graves, dead trees, props) as (x, y, yaw, half_y, half_x), lines (fences) left out."""
    dressing = importlib.reload(importlib.import_module('build_area_dressing'))
    return [(x, y, yaw, hy, hx) for x, y, yaw, hy, hx, line in dressing.footprints(build.source, build.layout['placements'])
            if not line]


def clear_of(point, pieces, margin):
    """Whether point is at least margin outside every footprint's box."""
    for x, y, piece_yaw, half_y, half_x in pieces:
        turn = math.radians(piece_yaw)
        dx, dy = point[0] - x, point[1] - y
        ahead, right = dx * math.cos(turn) + dy * math.sin(turn), -dx * math.sin(turn) + dy * math.cos(turn)
        if math.hypot(max(abs(ahead) - half_x, 0.0), max(abs(right) - half_y, 0.0)) < margin:
            return False
    return True


def in_box(fx, fy, box, pad=0.0):
    return box[0] - pad <= fx <= box[1] + pad and box[2] - pad <= fy <= box[3] + pad


def barge_plan(build):
    """Landfall's barge as its build plans it (build_area_landfall.plan_barge: read only), or None."""
    landfall = importlib.import_module('build_area_landfall')
    plan = landfall.Plan()
    try:
        if not landfall.plan_barge(build, plan):
            return None
    except Exception as error:   # a level without the Sink or the lookout
        build.warn(f'Landfall\'s barge plan failed ({error}): the tender comes from the north-east instead')
        return None
    return plan


# ---------------------------------------------------------------------------
# The window's hose and its cart
# ---------------------------------------------------------------------------

def find_sill(build, frame):
    """Where the hose goes in at the window: (its spot along the wall in the chapel's frame, the sill's height in cm over
    the chapel's pivot). The lowest height at which a line from the yard passes the west wall anywhere across the window's
    width (the wall is solid past the windows, and what glass is left blocks the rest): the gap nearest the window's
    middle at that height."""
    lo, hi, step = SILL_PROBE
    z = lo
    while z <= hi:
        gaps = []
        along = WINDOW[0] - WINDOW_HALF
        while along <= WINDOW[0] + WINDOW_HALF + 0.1:
            out_x, out_y, _ = frame.world(along, WINDOW[1] + 250.0)
            in_x, in_y, _ = frame.world(along, WINDOW[1] - 120.0)
            if story.onto_host(frame.chapel, unreal.Vector(out_x, out_y, frame.z + z),
                               unreal.Vector(in_x, in_y, frame.z + z)) is None:
                gaps.append(along)
            along += WINDOW_STEP
        if gaps:
            along = min(gaps, key=lambda a: abs(a - WINDOW[0]))
            return along, z + 2.0
        z += step
    build.warn(f'the chapel\'s west window: no opening found across it between {lo:.0f} and {hi:.0f} cm (check WINDOW, '
               f'WINDOW_HALF): the hose rests on {SILL_UP:.0f} at its middle')
    return WINDOW[0], SILL_UP


def place_cart(build, frame, sill):
    """The yard's hose-cart by the window, its hose to the sill ((along, height): find_sill); returns it (or None)."""
    along, sill = sill
    cls = story.actor_class('CompanyHoseCart')
    if cls is None:
        build.warn('no CompanyHoseCart class (build the game module first): no hose-cart in the chapel yard')
        return None
    remove_stale(build, CART_TAG)
    x, y, _ = frame.world(*CART_AT)
    z = story.ground_at(x, y)
    cart = build.place(cls, (x, y, z), frame.yaw + CART_TURN, label=CART_TAG, folder=FOLDER, tags=CART_TAGS + (CART_TAG,))
    sx, sy, _ = frame.world(along, WINDOW[1] + 12.0)
    cart.set_editor_property('hose_end', cart.get_actor_transform().inverse_transform_location(
        unreal.Vector(sx, sy, frame.z + sill)))
    cart.set_editor_property('shown_when', story.condition(during=MAIN4))
    # Stoppable in the yard's step (from 0: 1) only, so the step listening for the hose is the one being played.
    cart.set_editor_property('pumping_when', story.condition(during=MAIN4, from_step=1, before_step=2))
    cart.set_editor_property('stopped_when', [story.condition(during=MAIN4, from_step=2), story.condition(after=[MAIN4])])
    # No crew of its own to raise (the yard's set piece brings its hose-men); Vane on the lychgate's horn.
    cart.set_editor_property('crew_spawner_id', unreal.Name(''))
    cart.set_editor_property('loudspeaker_tag', unreal.Name(LYCHGATE))
    complaint = story.lines(build, HOSE_COMPLAINT)
    if complaint is not None:
        cart.set_editor_property('stop_broadcast', complaint)
    build.log(f'the chapel yard\'s hose-cart at ({x:.0f}, {y:.0f}, {z:.0f}), its hose to the west window\'s sill')
    return cart


def place_hose(build, frame, sill):
    """The hose over the sill ((along, height): find_sill) and in to the Reliquary's ember (AChapelWindowHose), pulled in
    Main 4's step 1."""
    along, sill = sill
    cls = story.actor_class('ChapelWindowHose')
    if cls is None:
        build.warn('no ChapelWindowHose class (build the game module first): no hose in the chapel\'s window')
        return None
    remove_stale(build, HOSE_TAG)
    x, y, z = frame.world(along, WINDOW[1], sill)
    # Its +X out of the wall (the chapel's +Y side).
    hose = build.place(cls, (x, y, z), frame.yaw + 90.0, label=HOSE_TAG, folder=FOLDER, tags=(HOSE_TAG,))
    plinth = story.socket(frame.chapel, 'Reliquary')
    if plinth is not None:
        # A guess at the ember for the editor's look (play aims at the Reliquary's own ember, found by its tag).
        end = plinth.translation + unreal.Vector(0.0, 0.0, 70.0)
        hose.set_editor_property('inside_end', hose.get_actor_transform().inverse_transform_location(end))
    else:
        build.warn('the chapel has no Reliquary socket: the window\'s hose ends on the nave\'s floor')
    hose.set_editor_property('inside_floor_drop', INSIDE_FLOOR_DROP)
    hose.set_editor_property('pulled_rest', unreal.Vector(*PULLED_REST))
    hose.set_editor_property('reliquary_tag', unreal.Name(RELIQUARY_TAG))
    hose.set_editor_property('cart_tag', unreal.Name(CART_TAG))
    hose.set_editor_property('hold_seconds', HOSE_HOLD)
    hose.set_editor_property('shown_when', story.condition(during=MAIN4))
    hose.set_editor_property('pullable_when', story.condition(during=MAIN4, from_step=1, before_step=2))
    hose.set_editor_property('pulled_when', [story.condition(during=MAIN4, from_step=2)])
    build.log(f'the Company\'s hose in the chapel\'s west window, its sill {sill:.0f} cm up at ({x:.0f}, {y:.0f})')
    return hose


def place_marks(build, frame):
    """The cart's drag furrows from the gateway to the cart, and the soot mark on the front door Abel held."""
    furrow = load(FURROW_DECAL)
    gates = chapel_module().YARD_GATES
    if furrow is not None and gates:
        (ax, ay), (bx, by) = gates[0]
        gx, gy = (ax + bx) * 0.5, (ay + by) * 0.5
        cx, cy, _ = frame.world(*CART_AT)
        length = math.hypot(cx - gx, cy - gy)
        yaw = math.degrees(math.atan2(cy - gy, cx - gx))
        count = max(1, int(length // FURROW_STEP) - 1)
        for k in range(count):
            t = (k + 1) / (count + 1)
            x, y = gx + (cx - gx) * t, gy + (cy - gy) * t
            decal = build.place(unreal.DecalActor, (x, y, story.ground_at(x, y)), yaw, label=f'Main4_Furrow_{k + 1}',
                                folder=FOLDER)
            # Projected straight down, its length along the drag.
            decal.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-90.0, yaw=yaw), False)
            part = decal.get_editor_property('decal')
            part.set_editor_property('decal_material', furrow)
            part.set_editor_property('decal_size', unreal.Vector(*FURROW_SIZE))
        build.log(f'{count} drag furrows from the churchyard gateway to the hose-cart')
    else:
        build.warn(f'no {FURROW_DECAL} or no gateway: no drag furrows in the chapel yard')
    soot = load(SOOT_DECAL)
    head = story.socket(frame.chapel, 'Crepe')
    if soot is not None and head is not None:
        turn = head.rotation.rotator()
        at = head.translation - unreal.Vector(0.0, 0.0, SOOT_UNDER)
        decal = build.place(unreal.DecalActor, (at.x, at.y, at.z), turn.yaw + 180.0, label='Main4_DoorSoot', folder=FOLDER)
        part = decal.get_editor_property('decal')
        part.set_editor_property('decal_material', soot)
        part.set_editor_property('decal_size', unreal.Vector(*SOOT_SIZE))
        build.log('the soot mark on the front door Abel held (a stand-in scorch)')
    else:
        build.warn('no soot decal or no SOCKET_Crepe on the chapel: no mark on the front door')


# ---------------------------------------------------------------------------
# The lychgate's loudspeaker
# ---------------------------------------------------------------------------

def place_lychgate(build, frame):
    gates = chapel_module().YARD_GATES
    if not gates:
        return None
    (ax, ay), (bx, by) = gates[0]
    gx, gy = (ax + bx) * 0.5, (ay + by) * 0.5
    # Out of the yard: away from the chapel, along the gateway's line a little to its side.
    away = (gx - frame.x, gy - frame.y)
    length = math.hypot(*away) or 1.0
    away = (away[0] / length, away[1] / length)
    along = ((bx - ax), (by - ay))
    along_len = math.hypot(*along) or 1.0
    along = (along[0] / along_len, along[1] / along_len)
    x = gx + away[0] * GATE_SPEAKER_OUT + along[0] * GATE_SPEAKER_ALONG
    y = gy + away[1] * GATE_SPEAKER_OUT + along[1] * GATE_SPEAKER_ALONG
    yaw = story.facing((x, y), (frame.x, frame.y))
    speaker = main2().place_loudspeaker(build, x, y, yaw, LYCHGATE, folder=FOLDER)
    if speaker is None:
        return None
    top = speaker.get_actor_location()
    return (top.x, top.y, top.z + 420.0, yaw)


# ---------------------------------------------------------------------------
# The barge's answer: the tender and its drop
# ---------------------------------------------------------------------------

def plan_hover(build, frame, fence):
    """The tender's hover spot (frame x, y, its yaw in the frame) whose ropes' feet all stand on the yard's open ground, or
    None. Returns (fx, fy, turn, feet [(world x, y)])."""
    pieces = footprints(build)
    cart = frame.world(*CART_AT)[:2]
    lo_x, hi_x, lo_y, hi_y, step = HOVER_GRID
    best = None
    fx = lo_x
    while fx <= hi_x:
        fy = lo_y
        while fy <= hi_y:
            for turn in (0.0, 90.0):
                c, s = math.cos(math.radians(turn)), math.sin(math.radians(turn))
                # The hull (square to the chapel: turned 0 or 90) clear of the spire by SPIRE_PAD.
                half_x, half_y = (HULL_HALF if turn == 0.0 else (HULL_HALF[1], HULL_HALF[0]))
                if fx + half_x > SPIRE_BOX[0] - SPIRE_PAD and fx - half_x < SPIRE_BOX[1] + SPIRE_PAD \
                        and fy + half_y > SPIRE_BOX[2] - SPIRE_PAD and fy - half_y < SPIRE_BOX[3] + SPIRE_PAD:
                    continue
                feet, ok = [], True
                for ax, ay, _ in ROPE_ANCHORS:
                    lx, ly = fx + c * ax - s * ay, fy + s * ax + c * ay
                    wx, wy, _ = frame.world(lx, ly)
                    if in_box(lx, ly, CHAPEL_BOX) or not inside((wx, wy), fence) or edge_distance((wx, wy), fence) < DROP_MARGIN \
                            or not clear_of((wx, wy), pieces, DROP_CLEAR) or math.hypot(wx - cart[0], wy - cart[1]) < 300.0:
                        ok = False
                        break
                    feet.append((wx, wy))
                if not ok:
                    continue
                # Prefer the front lawn and the west lane (where the player stands after the bell: the door, the window).
                score = math.hypot(fx - 700.0, fy - 500.0)
                if best is None or score < best[0]:
                    best = (score, fx, fy, turn, feet)
            fy += step
        fx += step
    return None if best is None else best[1:]


def place_drop(build, frame):
    """The tender over the yard and its four ropes' drop (BellAnswered), on from Main 4's step 4."""
    tender_cls = story.actor_class('CompanyTenderDrop')
    spawner_cls = story.actor_class('RappelSpawner')
    fence_entry = story.layout_entry(build, 'obstacles', 'churchyardFence')
    if tender_cls is None or spawner_cls is None or fence_entry is None:
        build.warn('no CompanyTenderDrop or RappelSpawner class (build the game module first) or no churchyardFence: no '
                   'answer from the barge (the bell\'s step has no fight)')
        return
    fence = [tuple(p[:2]) for p in fence_entry['polygon']]
    plan = plan_hover(build, frame, fence)
    if plan is None:
        build.warn('the tender: no hover spot over the chapel yard has every rope\'s foot on open ground (check ROPE_ANCHORS, '
                   'DROP_CLEAR): no drop')
        return
    fx, fy, turn, feet = plan
    hx, hy, _ = frame.world(fx, fy)
    ground = story.ground_at(hx, hy)
    hover = (hx, hy, ground + HOVER_UP)
    yaw = frame.yaw + turn
    remove_stale(build, TENDER_TAG)
    tender = build.place(tender_cls, hover, yaw, label=TENDER_TAG, folder=FOLDER, tags=(TENDER_TAG,))
    barge = barge_plan(build)
    if barge is not None:
        kx, ky, kz = barge.barge_keel
        to = (hx - kx, hy - ky)
        length = math.hypot(*to) or 1.0
        come = (kx + to[0] / length * TENDER_FROM_OUT, ky + to[1] / length * TENDER_FROM_OUT, kz + TENDER_FROM_UP)
    else:
        come = (hx + 9000.0, hy + 9000.0, ground + HOVER_UP + 2500.0)
    tender.set_editor_property('come_from', unreal.Vector(*come))
    tender.set_editor_property('hover_at', unreal.Vector(*hover))
    tender.set_editor_property('hover_yaw', yaw)
    tender.set_editor_property('fly_seconds', TENDER_FLY)
    tender.set_editor_property('rope_anchors', [unreal.Vector(*a) for a in ROPE_ANCHORS])
    tender.set_editor_property('drop_spawner_id', unreal.Name(DROP))
    tender.set_editor_property('leave_after', TENDER_LEAVE_AFTER)
    tender.set_editor_property('come_when', texts().condition(texts().ENCOUNTERS[DROP]))

    groups, missing = [], []
    for name, count, level in DROP_GROUPS:
        creature = story.actor_class(name)
        if creature is None:
            missing.append(name)
            continue
        made = story.group(creature, count, 'BASIC', 1, 1)
        made.set_editor_property('level', level)
        groups.append(made)
    if not groups:
        build.warn(f'no {", ".join(missing)} class yet: the tender comes with nobody on its ropes')
        return
    remove_stale(build, f'Encounter_{DROP}')
    sx, sy = sum(x for x, _ in feet) / len(feet), sum(y for _, y in feet) / len(feet)
    sz = story.ground_at(sx, sy)
    spawner = build.place(spawner_cls, (sx, sy, sz + 50.0), 0.0, label=f'Encounter_{DROP}', folder=FOLDER)
    spawner.set_editor_property('spawner_id', unreal.Name(DROP))
    spawner.set_editor_property('groups', groups)
    spawner.set_editor_property('num_waves', 1)
    spawner.set_editor_property('max_alive', sum(count for _, count, _ in DROP_GROUPS))
    spawner.set_editor_property('creature_tags', [unreal.Name(DROP_TAG)])
    spawner.set_editor_property('active_when', texts().condition(texts().ENCOUNTERS[DROP]))
    spawner.set_editor_property('activation_radius', DROP_ACTIVATION)
    spawner.set_editor_property('hunt_on_spawn', True)
    # One foot a rope: the spawner takes them in turn, two men down each of the first two.
    spawner.set_editor_property('spawn_points', [unreal.Vector(x - sx, y - sy, 0.0) for x, y in feet])
    spawner.set_editor_property('ground_corners', [unreal.Vector(x, y, sz) for x, y in fence])
    spawner.set_editor_property('ground_max_rise', 600.0)
    spawner.set_editor_property('anchor_tag', unreal.Name(TENDER_TAG))
    spawner.set_editor_property('slide_seconds', DROP_SLIDE)
    build.log(f'the barge\'s answer: the tender hangs over the chapel yard at ({hx:.0f}, {hy:.0f}, {hover[2]:.0f}), its ropes\' '
              f'feet at {", ".join(f"({x:.0f}, {y:.0f})" for x, y in feet)}; {DROP}: four riflemen and two hose-men, '
              f'from {MAIN4}\'s step 4{" (no " + ", ".join(missing) + ")" if missing else ""}')


# ---------------------------------------------------------------------------
# Brother Finch at the Reliquary
# ---------------------------------------------------------------------------

def place_finch(build, frame):
    cls = story.actor_class('WardenCrewMember')
    plinth = story.socket(frame.chapel, 'Reliquary')
    crew = getattr(unreal, 'WardenCrew', None)
    if cls is None or plinth is None or crew is None:
        build.warn('no WardenCrewMember class (build the game module first) or no Reliquary socket: Finch isn\'t at the '
                   'Reliquary (Main 5\'s turn-in has no one to take it)')
        return
    remove_stale(build, 'Finch_Reliquary')
    turn = plinth.rotation.rotator()
    # In front of the plinth toward the nave (the Reliquary faces the nave along the socket's +X), facing it.
    ahead = turn.get_forward_vector()
    at = plinth.translation + ahead * FINCH_OUT
    # On the sanctuary's floor there (traced onto the chapel), his capsule's middle over it.
    floor = story.onto_host(frame.chapel, unreal.Vector(at.x, at.y, at.z + 120.0), unreal.Vector(at.x, at.y, at.z - 250.0))
    z = (floor.z if floor is not None else frame.z + SANCTUARY_UP) + FINCH_HALF_HEIGHT
    finch = build.place(cls, (at.x, at.y, z), turn.yaw + 180.0, label='Finch_Reliquary', folder=FOLDER, tags=(FINCH_TAG,))
    finch.set_editor_property('crew', crew.KEEPER)
    finch.set_editor_property('at_station', True)
    finch.set_editor_property('shown_when', story.condition(after=[MAIN4], before=['Main7']))
    spec = texts().TOPICS[FINCH_TAG]
    talk = finch.get_editor_property('speaker_point')
    talk.set_editor_property('speaker_name', unreal.Text(spec['name']))
    default = story.lines(build, spec['default'])
    if default is not None:
        talk.set_editor_property('line_set', default)
    talk.set_editor_property('topics', story.texts_topics(build, FINCH_TAG))
    try:
        talk.set_editor_property('missions_only', False)
    except Exception:
        pass
    build.log(f'Brother Finch at the Reliquary ({at.x:.0f}, {at.y:.0f}), after {MAIN4}: Main 5\'s turn-in is his')


# ---------------------------------------------------------------------------

def place(build):
    """Everything above (build_area_story.place calls this after the chapel and Landfall, before Hob). Returns Hob's spots."""
    spots = {'lychgate': None}
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return spots
    chapel = story.placed(build, 'Chapel')
    if chapel is None:
        build.warn('no chapel: A Coal Under Ash\'s new pieces are left out (build the whole level first)')
        return spots
    frame = Frame(chapel)
    sill = find_sill(build, frame)
    place_cart(build, frame, sill)
    place_hose(build, frame, sill)
    place_marks(build, frame)
    spots['lychgate'] = place_lychgate(build, frame)
    place_drop(build, frame)
    place_finch(build, frame)
    return spots
