"""Main 5, "Choke Point" (the Wardens story, 2026-11-01; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 7, lane
MAIN45), its new world pieces in and over the Sink, for build_area_story.py: its place() calls place() here (reloaded
each run, as the editor keeps modules between runs) after the Sink (build_area_sink.py: the floor, the sacs, the lantern,
the dens) and Landfall (the barge, its hose and smog, the Smog light rule), before Hob (who takes the spot this hands back).
Everything goes in the area's Gameplay/Main5 folder, so a "gameplay" build places it all again; a piece whose class, model
or ground isn't there yet is left out with a warning.

What it places, from the built level (the layout's Sink zone and ramp, Den Rock's mouth, Landfall's barge plan read again
with build_area_landfall's own functions, traces):
- Hob's canvas satchel with Junie's salt charges (AMissionItem SaltCharges, tagged Item_SaltCharges) on the rim beside the
  ramp head, during Main 5 until taken (story_texts.ITEMS); Hob waits beside it as Main 5 starts.
- The intake (ASinkIntake, Intake_Sink) at the hose's mouth on the floor: its draw (within 10 m it pulls Sully toward the
  mouth, a drain inside 3 m, the screen's edge frosted), three clamp spots round its collar (AIntakeCharge, Intake_Charge,
  a 1.5 s hold each), the fuse (5 s, the draw doubling), the blast (Intake.Choked), the hose's buck, swing (it knocks down
  whatever is in its arc) and limp shape, the barge's two dying stacks' cough, the smog pool at the den's mouth draining
  and the growl in the dark; then, at the rim, the barge's second hose let down onto Larkspur High's new air-shaft head
  (until Back Pay is done). Landfall's own hose and two of its three plumes are hidden from the choke on (their
  hidden_when, set here; the intake hides them itself the moment it blows).
- IntakeDrop (ARappelSpawner): two ropes beside the hose, 20 m up; two drops of two riflemen and a hose-man (the Sink's
  levels, 4-5): the first as the player comes to the intake in its step, the second as the second charge goes on.
- IntakeSpiders (AEncounterSpawner): three spiders of the area's ranks down the webbed ramp as the first charge clangs on.
- Vane's voice: the Sink fence's loudspeaker by the ramp head (ACompanyLoudspeaker Loudspeaker_SinkFence, a post) and the
  barge's own horn (Loudspeaker_Barge, a voice only, under the barge's drum: the intake's charge and choke lines).
- The air-shaft head on Larkspur High (an AStoryProp stand-in: the Tripo Company air-shaft head, TRIPO_MODELS.md M45-4) and
  its marker Place_AirShaft, shown from Main 5's rim (the second hose's end) until Back Pay is done.
- The SmogThin lighting rule: joins Landfall's AStoryLighting before its Smog rule, after Main 5 (one hose choked: the
  smog thins, and stays thin once the barge falls); the state is layout.json level.environment.states.SmogThin (lane
  MAIN45's exact edit E5b).
Returns Hob's Main 5 spot for build_area_story.place_hob: {'satchel': (x, y, z, yaw) or None}.
"""
import importlib
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Main5'
MAIN5, MAIN6, BACK_PAY = 'Main5', 'Main6', 'BackPay'

# ---------------------------------------------------------------------------
# Names (the C++ and the mission data's; one place)
# ---------------------------------------------------------------------------

INTAKE_TAG = 'Intake_Sink'
CHARGE_TAG = 'Intake_Charge'
SATCHEL_ID = 'SaltCharges'
DROP = 'IntakeDrop'
DROP_TAG = 'Dredger_IntakeDrop'
SPIDERS = 'IntakeSpiders'
SPIDERS_TAG = 'Spider_IntakeSpiders'
FENCE_SPEAKER = 'Loudspeaker_SinkFence'
BARGE_SPEAKER = 'Loudspeaker_Barge'
SHAFT_TAG = 'AirShaft_Providence'
SHAFT_PLACE = 'Place_AirShaft'
# Landfall's (build_area_landfall.py): its hose's label (baked: one; else _01..), its plumes' labels, its light's actor.
LANDFALL_HOSE = 'Landfall_BargeHose'
LANDFALL_KEPT_PLUME = 'Landfall_Smog_1'
LANDFALL_PLUMES = ('Landfall_Smog_', 'Landfall_SmogGlint', 'Landfall_SmogIntake')
# Line sets (create_story_lines.py).
CHARGE_LINES = 'DA_Lines_Main5VaneCharge'
CHOKED_LINES = 'DA_Lines_Main5VaneChoked'
FUSE_LINES = 'DA_Lines_HobMain5Fuse'
GROWL_LINES = 'DA_Lines_HobMain5Growl'

# ---------------------------------------------------------------------------
# Stand-ins (one path each; TRIPO_MODELS.md "Mains 4-5" replaces them: the first path that exists)
# ---------------------------------------------------------------------------

SATCHEL_MODELS = ('/Game/Art/Tripo/SM_SaltSatchel', '/Game/Art/Props/SM_FlourSacks')
SHAFT_MODELS = ('/Game/Art/Tripo/SM_AirShaftHead',)
SHAFT_STAND_IN = '/Engine/BasicShapes/Cube'
SHAFT_LOOK = '/Game/Art/Materials/MI_LanternWood'
HOSE_RUBBER = '/Game/Art/Landfall/Materials/MI_Landfall_HoseRubber'

# ---------------------------------------------------------------------------
# Numbers (cm, degrees)
# ---------------------------------------------------------------------------

# The satchel: on the rim this far off the ramp head, along the rim (away from the fence's gap), its mouth to the Sink.
SATCHEL_OFF = (180.0, 120.0)        # out from the Sink's middle, along the rim
SATCHEL_SCALE = 0.45
HOB_BESIDE = 70.0
# The intake's three clamp spots: round the collar at these bearings off the barge's long axis, this far out from the
# mouth's middle (the collar's face; measured from the intake model when it's in), this high up it.
CHARGE_BEARINGS = (90.0, 210.0, 330.0)
COLLAR_OUT = 185.0
COLLAR_UP_SHARE = 0.55
# The limp hose's end: lying on the floor this far from the mouth toward the floor's middle and past it (cm).
LIMP_OUT = 900.0
# The drop's ropes: hung beside the hose (this far off its line, across the barge's axis), from where the hose is this high
# over the floor; their feet on the floor under them.
ROPE_UP = 2000.0
ROPE_SIDE = (220.0, -220.0)
DROP_PUSHES = (('DredgerRifleman', 2, 1), ('DredgerHoseMan', 1, 1), ('DredgerRifleman', 2, 2), ('DredgerHoseMan', 1, 2))
DROP_ACTIVATION = 2500.0
DROP_SLIDE = 2.6
DROP_RISE = 1600.0
# The ramp's spiders: three of the area's ranks, on its lower half.
SPIDER_COUNT = 3
SPIDER_RAMP = (0.45, 0.6, 0.75)
# The Sink fence's loudspeaker: off the ramp head, out from the Sink and along the rim.
FENCE_SPEAKER_OFF = (420.0, 280.0)
# The barge's horn: under its drum, heard over the whole Sink.
BARGE_EARSHOT = 9000.0
# The smog pool: on the floor in front of Den Rock's mouth (out along the mouth's facing), its radius.
POOL_OUT = 320.0
POOL_RADIUS = 260.0
# The air-shaft head on Larkspur High over the Providence's galleries (layout cm): looked for round this spot for ground
# no steeper than SHAFT_SLOPE across its 3 m; its mouth this high over the ground (the second hose's end).
SHAFT_AT = (26500.0, 6600.0)
SHAFT_SEARCH = ((0.0, 0.0), (400.0, 0.0), (-400.0, 0.0), (0.0, 400.0), (0.0, -400.0), (800.0, 300.0), (-800.0, -300.0))
SHAFT_SLOPE = 180.0
SHAFT_SIZE = (3.2, 3.2, 1.0)
SHAFT_MOUTH_UP = 260.0
# The second hose leaves the drum this far along the barge from the first.
SECOND_ALONG = 800.0


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def texts():
    import story_texts
    return importlib.reload(story_texts)


def main2():
    return importlib.reload(importlib.import_module('build_area_main2'))


def landfall():
    return importlib.import_module('build_area_landfall')


def load(path):
    return unreal.load_asset(path) if path and unreal.EditorAssetLibrary.does_asset_exist(path) else None


def first_model(paths):
    for path in paths:
        model = load(path)
        if model is not None:
            return model, path
    return None, None


def remove_stale(build, label):
    return main2().remove_stale(build, label)


def level_actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def labelled(build, prefix):
    """This build's actors whose label is prefix or starts with it."""
    return [a for a in level_actors() if unreal.Name(build.tag) in a.tags and str(a.get_actor_label()).startswith(prefix)]


def trace_down(x, y, top, bottom):
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, top), unreal.Vector(x, y, bottom),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    return None if hit is None else hit.to_tuple()[5].z


def bezier(p0, p1, p2, p3, t):
    u = 1.0 - t
    return tuple(u * u * u * p0[c] + 3 * u * u * t * p1[c] + 3 * u * t * t * p2[c] + t * t * t * p3[c] for c in range(3))


def vec(p):
    return unreal.Vector(p[0], p[1], p[2])


class Hose:
    """Landfall's hose as its build lays it (build_area_landfall.hose_parts' curve, read with its own plan and stand-ins:
    keep in step with it), and the barge's stacks: the intake's rest shape, its stacks' tops and its sections' look."""

    def __init__(self, build):
        lf = landfall()
        plan = lf.Plan()
        self.ok = False
        try:
            if not lf.plan_barge(build, plan):
                build.warn('no Sink in the layout: Choke Point\'s intake is left out')
                return
            _, tops, _ = lf.barge_parts(build, plan)
        except Exception as error:
            build.warn(f'Landfall\'s barge plan failed ({error}): Choke Point\'s intake is left out')
            return
        self.plan = plan
        self.tops = tops
        section = lf.load(lf.STAND_INS['hose_section'])
        intake = lf.load(lf.STAND_INS['hose_intake'])
        if section is None:
            build.warn('no hose section mesh: Choke Point\'s intake is left out')
            return
        own_hose = lf.is_tripo(lf.STAND_INS['hose_section'])
        own_intake = intake is not None and lf.is_tripo(lf.STAND_INS['hose_intake'])
        self.section = section
        self.rubber = None if own_hose else load(HOSE_RUBBER)
        if own_hose:
            box = section.get_bounding_box()
            size = box.get_editor_property('max') - box.get_editor_property('min')
            self.native = max(size.z, 1.0)
            self.girth_scale = lf.HOSE_GIRTH * 100.0 / max(size.x, size.y, 1.0)
        else:
            self.native = 100.0
            self.girth_scale = lf.HOSE_GIRTH
        top = plan.drum_bottom
        if own_intake:
            mouth = lf.socket_at(intake, 'Hose')
            rise = mouth.z if mouth is not None else intake.get_bounding_box().get_editor_property('max').z
            bottom = (plan.intake[0], plan.intake[1], plan.intake[2] + rise)
            box = intake.get_bounding_box()
            lo, hi = box.get_editor_property('min'), box.get_editor_property('max')
            self.collar_out = max(hi.x - lo.x, hi.y - lo.y) * 0.5 + 10.0
            self.collar_up = (hi.z - lo.z) * COLLAR_UP_SHARE
        else:
            bottom = (plan.intake[0], plan.intake[1], plan.intake[2] + lf.INTAKE_SCALE[2] * 100.0)
            self.collar_out = lf.INTAKE_SCALE[0] * 50.0 + 10.0
            self.collar_up = lf.INTAKE_SCALE[2] * 100.0 * COLLAR_UP_SHARE
        drop = top[2] - bottom[2]
        side = plan.barge_axis
        self.top, self.bottom = top, bottom
        self.c1 = (top[0] + side[0] * lf.HOSE_SAG, top[1] + side[1] * lf.HOSE_SAG, top[2] - drop * 0.45)
        self.c2 = (bottom[0] + side[0] * lf.HOSE_SAG * 0.6, bottom[1] + side[1] * lf.HOSE_SAG * 0.6, bottom[2] + drop * 0.3)
        self.mouth = plan.intake
        self.ok = True

    def at_height(self, z):
        """The hose's point (and its t) where it passes height z, from its mouth up."""
        best = None
        for i in range(201):
            t = i / 200.0
            p = bezier(self.top, self.c1, self.c2, self.bottom, t)
            if best is None or abs(p[2] - z) < abs(best[0][2] - z):
                best = (p, t)
        return best


# ---------------------------------------------------------------------------
# The satchel
# ---------------------------------------------------------------------------

def ramp_points(build):
    ramp = build.layout.get('ramps', {}).get('sinkRamp')
    return ramp.get('points') if ramp else None


def place_satchel(build, sink_middle):
    """Junie's charges in Hob's satchel on the rim by the ramp head. Returns Hob's spot beside it, or None."""
    cls = story.actor_class('MissionItem')
    points = ramp_points(build)
    if cls is None or not points:
        build.warn('no MissionItem class (build the game module first) or no sinkRamp: no satchel (Main 5\'s first step '
                   'can\'t finish)')
        return None
    hx, hy = points[0][0], points[0][1]
    out = (hx - sink_middle[0], hy - sink_middle[1])
    length = math.hypot(*out) or 1.0
    out = (out[0] / length, out[1] / length)
    along = (-out[1], out[0])
    x = hx + out[0] * SATCHEL_OFF[0] + along[0] * SATCHEL_OFF[1]
    y = hy + out[1] * SATCHEL_OFF[0] + along[1] * SATCHEL_OFF[1]
    z = story.ground_at(x, y)
    yaw = story.facing((x, y), sink_middle)
    remove_stale(build, f'Item_{SATCHEL_ID}')
    spec = texts().ITEMS[SATCHEL_ID]
    item = build.place(cls, (x, y, z + 2.0), yaw, label=f'Item_{SATCHEL_ID}', folder=FOLDER,
                       tags=('MissionItem', f'Item_{SATCHEL_ID}'))
    item.set_editor_property('item_id', unreal.Name(SATCHEL_ID))
    item.set_editor_property('display_name', unreal.Text(spec['name']))
    item.set_editor_property('prompt', unreal.Text(spec['prompt']))
    item.set_editor_property('shown_when', texts().condition(spec['shown_when']))
    model, path = first_model(SATCHEL_MODELS)
    if model is not None:
        mesh = item.get_editor_property('mesh')
        mesh.set_static_mesh(model)
        if path != SATCHEL_MODELS[0]:
            mesh.set_relative_scale3d(unreal.Vector(SATCHEL_SCALE, SATCHEL_SCALE, SATCHEL_SCALE))
    build.log(f'Hob\'s satchel with Junie\'s charges on the rim at ({x:.0f}, {y:.0f}, {z:.0f}) ({path or "a stand-in"})')
    return (x + along[0] * HOB_BESIDE, y + along[1] * HOB_BESIDE, z, yaw)


# ---------------------------------------------------------------------------
# The intake, its charges, its fights, its voices
# ---------------------------------------------------------------------------

def floor_at(sink, x, y):
    z = trace_down(x, y, sink.fz + 600.0, sink.fz - 800.0)
    return sink.fz if z is None else z


def place_intake(build, hose, sink, den):
    """The intake at the hose's mouth (ASinkIntake), everything it drives. Returns it, or None."""
    cls = story.actor_class('SinkIntake')
    if cls is None:
        build.warn('no SinkIntake class (build the game module first): no intake set piece (Main 5\'s fourth step can\'t '
                   'finish)')
        return None
    remove_stale(build, INTAKE_TAG)
    mx, my, mz = hose.mouth
    axis_yaw = math.degrees(math.atan2(hose.plan.barge_axis[1], hose.plan.barge_axis[0]))
    intake = build.place(cls, (mx, my, mz), axis_yaw, label=INTAKE_TAG, folder=FOLDER, tags=(INTAKE_TAG, 'Obstacle'))
    intake.set_editor_property('hose_top', vec(hose.top))
    intake.set_editor_property('hose_c1', vec(hose.c1))
    intake.set_editor_property('hose_c2', vec(hose.c2))
    intake.set_editor_property('hose_bottom', vec(hose.bottom))
    intake.set_editor_property('hose_mesh', hose.section)
    intake.set_editor_property('hose_girth_scale', hose.girth_scale)
    intake.set_editor_property('hose_native_length', hose.native)
    if hose.rubber is not None:
        intake.set_editor_property('hose_material', hose.rubber)
    # Limp, its end lies on the floor toward the floor's middle and past it.
    to_middle = (sink.cx - mx, sink.cy - my)
    length = math.hypot(*to_middle) or 1.0
    lx, ly = mx + to_middle[0] / length * LIMP_OUT, my + to_middle[1] / length * LIMP_OUT
    intake.set_editor_property('limp_rest', unreal.Vector(lx, ly, floor_at(sink, lx, ly) + 60.0))
    intake.set_editor_property('rest_hose_tag', unreal.Name(LANDFALL_HOSE))
    intake.set_editor_property('dying_stacks', [vec(p) for p in hose.tops[1:]])
    intake.set_editor_property('loudspeaker_tag', unreal.Name(BARGE_SPEAKER))
    intake.set_editor_property('drop_spawner_id', unreal.Name(DROP))
    intake.set_editor_property('spider_spawner_id', unreal.Name(SPIDERS))
    for prop, name in (('charge_lines', CHARGE_LINES), ('choked_lines', CHOKED_LINES), ('fuse_lines', FUSE_LINES),
                       ('growl_lines', GROWL_LINES)):
        line_set = story.lines(build, name)
        if line_set is not None:
            intake.set_editor_property(prop, line_set)
    # The draw in its step (from 0: 3) only; choked from the climb on (4) and after; gone when the barge falls.
    intake.set_editor_property('draw_when', story.condition(during=MAIN5, from_step=3, before_step=4))
    intake.set_editor_property('choked_when', [story.condition(during=MAIN5, from_step=4), story.condition(after=[MAIN5])])
    intake.set_editor_property('gone_when', [story.condition(after=[MAIN6])])
    # The smog pool at the den's mouth, and the growl from inside it.
    mouth = story.socket(den, 'DenMouth') if den is not None else None
    if mouth is not None:
        facing = mouth.rotation.rotator().get_forward_vector()
        px, py = mouth.translation.x + facing.x * POOL_OUT, mouth.translation.y + facing.y * POOL_OUT
        intake.set_editor_property('pool_at', unreal.Vector(px, py, floor_at(sink, px, py) + 3.0))
        intake.set_editor_property('pool_radius', POOL_RADIUS)
        intake.set_editor_property('den_at', mouth.translation)
    else:
        build.warn('no Den Rock with its SOCKET_DenMouth: no smog pool to drain, no growl')
    build.log(f'the intake at the hose\'s mouth ({mx:.0f}, {my:.0f}, {mz:.0f}): its draw in {MAIN5}\'s step 4, limp after, '
              f'{len(hose.tops) - 1} stack(s) dying at the choke')
    return intake


def place_charges(build, hose, intake):
    cls = story.actor_class('IntakeCharge')
    if cls is None or intake is None:
        build.warn('no IntakeCharge class (build the game module first): no clamp spots on the intake')
        return
    mx, my, mz = hose.mouth
    axis = math.degrees(math.atan2(hose.plan.barge_axis[1], hose.plan.barge_axis[0]))
    out = max(hose.collar_out, COLLAR_OUT * 0.6)
    for k, bearing in enumerate(CHARGE_BEARINGS):
        label = f'IntakeCharge_{k + 1}'
        remove_stale(build, label)
        b = math.radians(axis + bearing)
        x, y = mx + math.cos(b) * out, my + math.sin(b) * out
        # Facing out of the collar: the player sets it from outside.
        charge = build.place(cls, (x, y, mz + hose.collar_up), axis + bearing, label=label, folder=FOLDER,
                             tags=(CHARGE_TAG,))
        charge.set_editor_property('intake_tag', unreal.Name(INTAKE_TAG))
        charge.set_editor_property('set_when', story.condition(during=MAIN5, from_step=3, before_step=4))
    build.log(f'{len(CHARGE_BEARINGS)} clamp spots round the intake\'s collar, {out:.0f} cm out, {hose.collar_up:.0f} up')


def place_drop(build, hose, sink):
    cls = story.actor_class('RappelSpawner')
    if cls is None:
        build.warn('no RappelSpawner class (build the game module first): nobody comes down the hose')
        return
    groups, missing = [], []
    for name, count, wave in DROP_PUSHES:
        creature = story.actor_class(name)
        if creature is None:
            missing.append(name)
            continue
        # The Sink's levels (the zone's 4-5): no group level of their own.
        groups.append(story.group(creature, count, 'BASIC', wave, wave))
    if not groups:
        build.warn(f'no {", ".join(missing)} class yet: nobody comes down the hose')
        return
    point, _ = hose.at_height(sink.fz + ROPE_UP)
    across = (-hose.plan.barge_axis[1], hose.plan.barge_axis[0])
    tops, feet = [], []
    for side in ROPE_SIDE:
        tx, ty = point[0] + across[0] * side, point[1] + across[1] * side
        tops.append(unreal.Vector(tx, ty, point[2]))
        feet.append((tx, ty, floor_at(sink, tx, ty)))
    remove_stale(build, f'Encounter_{DROP}')
    sx, sy, sz = hose.mouth
    spawner = build.place(cls, (sx, sy, sz + 50.0), 0.0, label=f'Encounter_{DROP}', folder=FOLDER)
    spawner.set_editor_property('spawner_id', unreal.Name(DROP))
    spawner.set_editor_property('groups', groups)
    spawner.set_editor_property('num_waves', 2)
    # The second drop only on the intake's word (its second charge), never a timer.
    spawner.set_editor_property('wave_interval', 0.0)
    spawner.set_editor_property('wait_for_clear', False)
    spawner.set_editor_property('max_alive', 6)
    spawner.set_editor_property('creature_tags', [unreal.Name(DROP_TAG)])
    spawner.set_editor_property('active_when', texts().condition(texts().ENCOUNTERS[DROP]))
    spawner.set_editor_property('activation_radius', DROP_ACTIVATION)
    spawner.set_editor_property('hunt_on_spawn', True)
    spawner.set_editor_property('spawn_points', [unreal.Vector(x - sx, y - sy, 0.0) for x, y, _ in feet])
    spawner.set_editor_property('ground_corners', [unreal.Vector(x, y, sink.fz) for x, y in sink.zone])
    spawner.set_editor_property('ground_max_rise', DROP_RISE)
    spawner.set_editor_property('rope_tops', tops)
    spawner.set_editor_property('slide_seconds', DROP_SLIDE)
    build.log(f'{DROP}: two ropes beside the hose from {ROPE_UP / 100:.0f} m up; two drops of two riflemen and a hose-man'
              f'{" (no " + ", ".join(missing) + ")" if missing else ""}')


def place_spiders(build, sink):
    cls = story.actor_class('EncounterSpawner')
    spider = story.actor_class('SpiderCreature')
    points = ramp_points(build)
    if cls is None or spider is None or not points:
        build.warn('no EncounterSpawner/SpiderCreature class or no sinkRamp: no spiders down the ramp')
        return
    spots = []
    for share in SPIDER_RAMP:
        p = points[min(len(points) - 1, int(share * (len(points) - 1)))]
        spots.append((p[0], p[1], p[2] if len(p) > 2 else story.ground_at(p[0], p[1])))
    remove_stale(build, f'Encounter_{SPIDERS}')
    ax, ay, az = spots[len(spots) // 2]
    made = build.place(cls, (ax, ay, az + 50.0), 0.0, label=f'Encounter_{SPIDERS}', folder=FOLDER)
    group = unreal.EncounterGroup()
    group.set_editor_property('creature_class', spider)
    group.set_editor_property('count', SPIDER_COUNT)
    group.set_editor_property('rank_roll', unreal.EncounterRankRoll.AREA)
    made.set_editor_property('spawner_id', unreal.Name(SPIDERS))
    made.set_editor_property('groups', [group])
    made.set_editor_property('num_waves', 1)
    made.set_editor_property('creature_tags', [unreal.Name(SPIDERS_TAG)])
    made.set_editor_property('active_when', texts().condition(texts().ENCOUNTERS[SPIDERS]))
    # Only on the intake's word (its first charge's clang): never on approach.
    made.set_editor_property('spawn_on_approach', False)
    made.set_editor_property('hunt_on_spawn', True)
    made.set_editor_property('spawn_points', [unreal.Vector(x - ax, y - ay, z - az) for x, y, z in spots])
    made.set_editor_property('ground_corners', [unreal.Vector(x, y, sink.fz) for x, y in sink.zone])
    made.set_editor_property('ground_max_rise', DROP_RISE)
    build.log(f'{SPIDERS}: {SPIDER_COUNT} spiders down the webbed ramp, on the intake\'s first charge')


def place_voices(build, hose, sink):
    points = ramp_points(build)
    if points:
        hx, hy = points[0][0], points[0][1]
        out = (hx - sink.cx, hy - sink.cy)
        length = math.hypot(*out) or 1.0
        out = (out[0] / length, out[1] / length)
        along = (-out[1], out[0])
        x = hx + out[0] * FENCE_SPEAKER_OFF[0] - along[0] * FENCE_SPEAKER_OFF[1]
        y = hy + out[1] * FENCE_SPEAKER_OFF[0] - along[1] * FENCE_SPEAKER_OFF[1]
        main2().place_loudspeaker(build, x, y, story.facing((x, y), (sink.cx, sink.cy)), FENCE_SPEAKER, folder=FOLDER)
    # The barge's own horn: a voice only, under its drum, heard over the whole Sink.
    cls = story.actor_class('CompanyLoudspeaker')
    if cls is None:
        return
    remove_stale(build, BARGE_SPEAKER)
    tx, ty, tz = hose.top
    horn = build.place(cls, (tx, ty, tz), 0.0, label=BARGE_SPEAKER, folder=FOLDER,
                       tags=main2().LOUDSPEAKER_TAGS + (BARGE_SPEAKER,))
    horn.set_editor_property('show_post', False)
    horn.set_editor_property('earshot_radius', BARGE_EARSHOT)
    horn.set_editor_property('cues', main2().loudspeaker_cues(build, BARGE_SPEAKER))
    build.log(f'the barge\'s horn (a voice only) under its drum at ({tx:.0f}, {ty:.0f}, {tz:.0f})')


# ---------------------------------------------------------------------------
# Landfall's hose and plumes, the light, the air shaft
# ---------------------------------------------------------------------------

def hide_landfall_pieces(build):
    """Landfall's hose and two of its three plumes (and the glint and the intake's) hidden from the choke on: the intake
    draws the hose itself from then, limp; one stack still smokes. Tags the hose's props so the intake finds them."""
    when = [story.condition(during=MAIN5, from_step=4), story.condition(after=[MAIN5])]
    hoses = labelled(build, LANDFALL_HOSE)
    plumes = [a for prefix in LANDFALL_PLUMES for a in labelled(build, prefix) if str(a.get_actor_label()) != LANDFALL_KEPT_PLUME]
    for actor in hoses + plumes:
        hidden = [c for c in actor.get_editor_property('hidden_when')]
        actor.set_editor_property('hidden_when', hidden + when)
    for actor in hoses:
        tags = [str(t) for t in actor.tags]
        if LANDFALL_HOSE not in tags:
            actor.set_editor_property('tags', [unreal.Name(t) for t in tags + [LANDFALL_HOSE]])
    if not hoses:
        build.warn(f'no {LANDFALL_HOSE} placed (build_area_landfall.py runs first): the intake\'s choke has no hose to swap')
    build.log(f'Landfall\'s hose ({len(hoses)} prop(s)) and {len(plumes)} plume(s) hidden from {MAIN5}\'s choke on')


def place_smog_thin(build):
    """SmogThin after Main 5, before Landfall's Smog rule (the first rule that holds wins)."""
    lighting = next((a for a in level_actors() if str(a.get_actor_label()) == 'StoryLighting' and unreal.Name(build.tag) in a.tags),
                    None)
    if lighting is None:
        build.warn('no StoryLighting placed (build_area_deck/landfall run first): the smog never thins')
        return
    rules = [r for r in lighting.get_editor_property('rules') if str(r.get_editor_property('state')) != 'SmogThin']
    rule = unreal.StoryLightingRule()
    rule.set_editor_property('when', story.condition(after=[MAIN5], before=['Main7']))
    rule.set_editor_property('state', unreal.Name('SmogThin'))
    smog = next((i for i, r in enumerate(rules) if str(r.get_editor_property('state')) == 'Smog'), len(rules))
    lighting.set_editor_property('rules', rules[:smog] + [rule] + rules[smog:])
    build.log(f'the story\'s light: SmogThin after {MAIN5} until Main7, before the Smog rule ({smog} rule(s) before it)')


def place_shaft(build, intake, hose):
    """The air-shaft head on Larkspur High (the second hose's end) and its marker, from Main 5's rim until Back Pay is
    done; the intake's second hose aims at its mouth."""
    best = None
    for dx, dy in SHAFT_SEARCH:
        x, y = SHAFT_AT[0] + dx, SHAFT_AT[1] + dy
        heights = [story.ground_at(x + ox, y + oy, warn=False) for ox, oy in ((0, 0), (150, 0), (-150, 0), (0, 150), (0, -150))]
        spread = max(heights) - min(heights)
        if best is None or spread < best[0]:
            best = (spread, x, y, heights[0])
    spread, x, y, z = best
    if spread > SHAFT_SLOPE:
        build.warn(f'the air shaft\'s ground falls {spread:.0f} cm across its 3 m at its best spot ({x:.0f}, {y:.0f}): move '
                   f'SHAFT_AT')
    shown = [story.condition(during=MAIN5, from_step=5), story.condition(after=[MAIN5])]
    gone = [story.condition(after=[BACK_PAY])]
    cls = story.actor_class('StoryProp')
    model, path = first_model(SHAFT_MODELS)
    remove_stale(build, SHAFT_TAG)
    if cls is not None:
        prop = build.place(cls, (x, y, z), 0.0, label=SHAFT_TAG, folder=FOLDER, tags=(SHAFT_TAG, 'Obstacle'))
        mesh = prop.get_editor_property('mesh')
        if model is not None:
            mesh.set_static_mesh(model)
        else:
            # A timbered collar until the Tripo shaft head (M45-4): a squat block of the lantern house's wood.
            mesh.set_static_mesh(load(SHAFT_STAND_IN))
            prop.set_actor_scale3d(unreal.Vector(*SHAFT_SIZE))
            prop.set_actor_location(unreal.Vector(x, y, z + SHAFT_SIZE[2] * 50.0), False, True)
            look = load(SHAFT_LOOK)
            if look is not None:
                mesh.set_material(0, look)
        prop.set_editor_property('shown_when', shown)
        prop.set_editor_property('hidden_when', gone)
    build.place(unreal.TargetPoint, (x, y, z + story.MARKER_UP), 0.0, label=SHAFT_PLACE, folder=FOLDER, tags=(SHAFT_PLACE,))
    if intake is not None:
        intake.set_editor_property('second_hose_to', unreal.Vector(x, y, z + SHAFT_MOUTH_UP))
        ax, ay = hose.plan.barge_axis
        tx, ty, tz = hose.top
        intake.set_editor_property('second_hose_from', unreal.Vector(tx + ax * SECOND_ALONG, ty + ay * SECOND_ALONG, tz))
        intake.set_editor_property('second_hose_when', shown)
        intake.set_editor_property('second_hose_gone_when', gone)
    build.log(f'the air-shaft head on Larkspur High at ({x:.0f}, {y:.0f}, {z:.0f}) ({path or "a stand-in"}), the second '
              f'hose\'s end, from {MAIN5}\'s rim until {BACK_PAY} is done')


# ---------------------------------------------------------------------------

def place(build):
    """Everything above (build_area_story.place calls this after the Sink and Landfall, before Hob). Returns Hob's spot."""
    spots = {'satchel': None}
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return spots
    zone = story.layout_entry(build, 'zones', 'sink')
    if zone is None:
        return spots
    sink = importlib.import_module('build_area_sink').Sink(build, zone)
    spots['satchel'] = place_satchel(build, (sink.cx, sink.cy))
    hose = Hose(build)
    den = story.placed(build, 'DenRock')
    intake = None
    if hose.ok:
        intake = place_intake(build, hose, sink, den)
        place_charges(build, hose, intake)
        place_drop(build, hose, sink)
        place_voices(build, hose, sink)
        place_shaft(build, intake, hose)
    place_spiders(build, sink)
    hide_landfall_pieces(build)
    place_smog_thin(build)
    return spots
