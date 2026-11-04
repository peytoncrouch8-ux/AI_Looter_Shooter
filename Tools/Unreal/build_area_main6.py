"""Main 6, "The Pilot Light" (the Wardens story, 2026-11-01; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 10, lane
MAIN67's W3: Docs/Work/2026-10-31-Wardens/main67_src/CONTRACT67.md), the barge's world pieces over the chapel knoll and in
Gravewind Canyon, for build_area_story.py: its place() calls place() here (reloaded each run, as the editor keeps modules
between runs) after Main 5's (build_area_main5.py) and Landfall's (the Sink barge), before Hob (who takes the spot this
hands back). Everything goes in the area's Gameplay/Main6 folder, so a "gameplay" build places it all again; a piece whose
class, model or ground isn't there yet is left out with a warning.

What it places, from the built level (the chapel's placed model, the Lamplighter's placed (moored) actor, the burial deck,
traces) and the barge's own C++ layout (ABargeBoarding's getters: nothing of her kit is copied here):
- Vane's barge brought down over the chapel (ABargeBoarding, tagged Barge_Pilot): her keel KEEL_OVER (25 m) over the
  knoll's ground, her hose drum (under her keel) a few metres beside the nave's ridge so her hose curves down to it
  (HoseTarget: the ridge, Chapel.py's eave + rise), turned broadside so her berth side (her -Y, where the Lamplighter
  grapples and the plank lands) faces the Mooring Ledge she comes from. Shown during Main 6 until Pilot.BargeFell (her
  C++). FallTarget: the wreck's spot below; BoardsPoint: the burial deck.
- BargeDeck (AEncounterSpawner): six Dredger riflemen and three hose-men at level 6 on her deck's fight spots, in two pushes,
  from Main 6's grapple step (riflemen at the rail fire across as the Lamplighter comes alongside) until the manifest's.
- FurnaceHall (AEncounterSpawner): the Chief Stoker (a hose-man, Hallowed, level 5 + the rank's 2 = 7, leading, x1.76
  health on Hallowed's 3.4: x6) with two riflemen and a hose-man, then two riflemen and two hose-men through the stokeholds,
  level 6, during the furnace's step.
- Four Company horns on her deck (ACompanyLoudspeaker posts, tagged Barge_Horn, attached to her): Main 6's optional
  "Silence the barge's horns" counts them; the bow's carries Vane's deck cue (DA_Lines_Main6VaneDeck, from step 3).
- The manifest (AMissionItem BargeManifest, Paid on Delivery's item) on the stoker's desk, attached to her: shown and taken
  during Main 6's manifest step (Main 6 step 4's Interact on Item_BargeManifest).
- The wreck in Gravewind Canyon below the Rim, south of the boards: the hull (Tripo 37's broken barge when it lands, else
  SM_DredgeBarge) pitched and rolled on the canyon's floor, and a smoke column (SM_SmokePlume in MI_Landfall_SoulSmog)
  rising past the Rim, shown once Pilot.BargeFell is set (BargeFall's end) or after Main 6; the smoke gone after Main 7.
- Landfall's barge over the Sink, its hose and plumes, hidden during Main 6 (she's come down over the chapel); the Sink
  intake's limp hose gone during Main 6 too (its gone_when).
- Then the Lamplighter's sortie (W2's build_area_main6_sortie.place(build, barge)): its spots come back as this module's.
Returns Hob's Main 6 spot for build_area_story.place_hob: {'wheel': (x, y, z, yaw) or None} (W2's).

Nothing of her deck's height or rectangle is written here: every spot on or in her (the deck's middle and fight spots, the hall's,
the horns, the desk, the hose drum, the berth) is read from her getters when this runs, so a change to BargeRules (DeckTop, the
rectangle, KitLift) reaches the level by running this again, and by nothing else: the pieces below are saved at the world
positions the getters gave then. Runs alone too: `py .../build_area_main6.py RansomsRest` clears Gameplay/Main6 only (the barge,
her spawners, horns, manifest, the wreck and its smoke, the sortie) and places it again.
"""
import importlib
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Main6'
MAIN6, MAIN7 = 'Main6', 'Main7'

# ---------------------------------------------------------------------------
# Names (the C++ and the mission data's; one place)
# ---------------------------------------------------------------------------

BARGE_CLASS = 'BargeBoarding'
BARGE_TAG = 'Barge_Pilot'
DECK_FIGHT = 'BargeDeck'
HALL_FIGHT = 'FurnaceHall'
HORN_TAG = 'Barge_Horn'
MANIFEST_ID = 'BargeManifest'
FELL_FLAG = 'Pilot.BargeFell'
VANE_DECK_LINES = 'DA_Lines_Main6VaneDeck'
# Main 6's steps (from 0; the contract's): the grapples, the deck, the manifest, the furnace.
GRAPPLE_STEP, DECK_STEP, MANIFEST_STEP, FURNACE_STEP = 2, 3, 4, 5
# Landfall's barge pieces over the Sink (build_area_landfall.place_barge's labels) and Main 5's intake's tag.
LANDFALL_PIECES = ('Landfall_Barge', 'Landfall_BargeHose', 'Landfall_Smog')
INTAKE_TAG = 'Intake_Sink'

# ---------------------------------------------------------------------------
# Stand-ins (TRIPO_MODELS.md "Mains 6-7": the first path that exists)
# ---------------------------------------------------------------------------

WRECK_MODELS = ('/Game/Art/Tripo/SM_BargeWreck', '/Game/Art/Tripo/SM_DredgeBarge')
SMOKE_MODEL = '/Game/Art/Props/SM_SmokePlume'
SMOG_LOOK = '/Game/Art/Landfall/Materials/MI_Landfall_SoulSmog'
SCORCHED_LOOK = '/Game/Art/Materials/MI_GunScorched'
MANIFEST_MODELS = ('/Game/Art/Tripo/SM_BargeManifest', '/Game/Art/Props/SM_PayrollBook', '/Game/Art/Props/SM_Journal')

# ---------------------------------------------------------------------------
# Numbers (cm, degrees)
# ---------------------------------------------------------------------------

# Her keel over the knoll's ground (the design's "about 25 m over the chapel knoll"; the spire's cross is ~16 m).
KEEL_OVER = 2500.0
# The nave's ridge in the chapel's frame (Chapel.py: the eave 4.8 m, the roof's rise 3.8 m; the nave along X, the tower +X).
RIDGE_LOCAL = (-150.0, 0.0, 860.0)
# Her hose drum (under her keel; where it is on her is ABargeBoarding::GetHoseTop, read when she is placed) stands this far
# beside the ridge, off her berth side (her +Y).
DRUM_BESIDE = 700.0
# The fights.
DECK_GROUPS = (('DredgerRifleman', 4, 1), ('DredgerHoseMan', 1, 1), ('DredgerRifleman', 2, 2), ('DredgerHoseMan', 2, 2))
DECK_LEVEL = 6
DECK_ACTIVATION, DECK_WAVE_GAP, DECK_MAX_ALIVE = 3500.0, 20.0, 7
HALL_LEVEL = 6
CHIEF_LEVEL = 5        # + Hallowed's two levels: 7
CHIEF_HEALTH = 1.76    # x Hallowed's 3.4: about x6
HALL_ACTIVATION, HALL_MAX_ALIVE = 2200.0, 6
# The wreck: off the burial deck's open front (into the canyon) and south (the level's -X), on the canyon floor 90 m out
# (the Rim's lip near x -2800 is at y -11260: about 128 m out, by the river). Nothing past the Rim has collision
# (Docs/Areas/RansomsRest.md), so a trace finds no floor there: the floor is the escarpment's drop under the boards
# (layout.json region.escarpment.drop, 70 m; Gravewind Point stands 70 m over it). The fallback only if the layout has none.
WRECK_OUT, WRECK_SOUTH = 9000.0, 4000.0
WRECK_FALLBACK_DOWN = 7000.0
WRECK_TURN = (24.0, 30.0)      # pitch (bow down), roll
WRECK_SUNK = 500.0
SMOKE_SCALE = 6.0
SMOKE_STACK = 3                # plumes one over another up the column (SM_SmokePlume is 8 m tall)

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load(path):
    return unreal.load_asset(path) if path and unreal.EditorAssetLibrary.does_asset_exist(path) else None


def first_model(paths):
    for path in paths:
        model = load(path)
        if model is not None:
            return model, path
    return None, None


def labelled(build, prefix):
    """This build's actors whose label starts with prefix."""
    return [a for a in actors.get_all_level_actors()
            if str(a.get_actor_label()).startswith(prefix) and unreal.Name(build.tag) in a.tags]


def remove_stale(build, label):
    gone = [a for a in actors.get_all_level_actors() if str(a.get_actor_label()) == label and unreal.Name(build.tag) in a.tags]
    for actor in gone:
        actors.destroy_actor(actor)


def of_class(name):
    cls = story.actor_class(name)
    if cls is None:
        return []
    # A loaded unreal.Class isn't a Python type (isinstance refuses it): ask the world for its actors.
    return list(unreal.GameplayStatics.get_all_actors_of_class(unreal.EditorLevelLibrary.get_editor_world(), cls))


def attach(child, parent):
    keep = unreal.AttachmentRule.KEEP_WORLD
    child.attach_to_actor(parent, '', keep, keep, keep, False)


def story_prop(build, model, location, rotator, scale, label, shown=(), hidden=(), look=None):
    cls = story.actor_class('StoryProp')
    if cls is None or model is None:
        if cls is None:
            build.warn(f'no StoryProp class (build the game module first): no {label}')
        return None
    remove_stale(build, label)
    prop = build.place(cls, location, 0.0, label=label, folder=FOLDER)
    prop.set_actor_rotation(rotator, False)
    prop.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    mesh = prop.get_editor_property('mesh')
    mesh.set_static_mesh(model)
    if look is not None:
        for slot in range(max(len(model.get_editor_property('static_materials')), 1)):
            mesh.set_material(slot, look)
    mesh.set_editor_property('cast_shadow', False)
    prop.set_editor_property('shown_when', list(shown))
    prop.set_editor_property('hidden_when', list(hidden))
    return prop


def trace_down(x, y, top, bottom):
    """The first world-static ground straight down from top to bottom at (x, y), or None."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, top), unreal.Vector(x, y, bottom),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], unreal.DrawDebugTrace.NONE, True)
    return None if hit is None else hit.to_tuple()[5].z


# ---------------------------------------------------------------------------
# The barge over the chapel
# ---------------------------------------------------------------------------

class Chapel:
    """The chapel's frame: points (cm, its own axes) to the world."""

    def __init__(self, chapel):
        at = chapel.get_actor_location()
        self.x, self.y, self.z = at.x, at.y, at.z
        self.yaw = chapel.get_actor_rotation().yaw
        self.c, self.s = math.cos(math.radians(self.yaw)), math.sin(math.radians(self.yaw))

    def world(self, fx, fy, up=0.0):
        return (self.x + self.c * fx - self.s * fy, self.y + self.s * fx + self.c * fy, self.z + up)


def ledge_spot(build):
    """Where the Lamplighter lies moored on the Mooring Ledge (her placed actor stands there in the editor), or None."""
    ships = of_class('LamplighterShip')
    if not ships:
        return None
    at = ships[0].get_actor_location()
    return at.x, at.y


def place_barge(build):
    cls = story.actor_class(BARGE_CLASS)
    chapel_actor = story.placed(build, 'Chapel')
    if cls is None or chapel_actor is None:
        build.warn('no BargeBoarding class (build the game module first): no barge over the chapel' if cls is None
                   else 'no chapel: the barge has nowhere to come down (build the whole level first)')
        return None
    chapel = Chapel(chapel_actor)
    ridge = chapel.world(*RIDGE_LOCAL)
    # Broadside: her berth side (her -Y) toward the ledge, so the Lamplighter comes straight in to grapple.
    ledge = ledge_spot(build)
    if ledge is None:
        build.warn('no Lamplighter placed (build_area_landfall.py first): the barge faces the chapel\'s own way')
        toward = (chapel.c, chapel.s)
    else:
        dx, dy = ledge[0] - ridge[0], ledge[1] - ridge[1]
        length = math.hypot(dx, dy) or 1.0
        toward = (dx / length, dy / length)
    # Her -Y is (sin yaw, -cos yaw): set to the ledge's way.
    yaw = math.degrees(math.atan2(toward[0], -toward[1]))
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))

    def turn(lx, ly):
        return c * lx - s * ly, s * lx + c * ly

    # The knoll's ground is the chapel's own foot (Chapel.py stands it on the ground; a trace here would meet its roof).
    ground = chapel.z
    remove_stale(build, 'Barge_Pilot')
    # 'Floor' as well as 'Obstacle' (the minimap draws her solid): an obstacle's top is no ground for encounter spots
    # unless it carries Floor, and BargeDeck's nine found nowhere to stand on her deck without it (Demo47, as Main 2's boards).
    barge = build.place(cls, (ridge[0], ridge[1], ground + KEEL_OVER), yaw, label='Barge_Pilot', folder=FOLDER,
                        tags=(BARGE_TAG, 'Obstacle', 'Floor'))
    # Her drum DRUM_BESIDE off the ridge, away from the berth side: placed and turned first, then slid by where her own
    # getter puts the drum (world, her yaw already in it), so the drum's place on her is hers and not copied here.
    here, drum, beside = barge.get_actor_location(), barge.get_hose_top(), turn(0.0, DRUM_BESIDE)
    mx, my = ridge[0] + beside[0] - (drum.x - here.x), ridge[1] + beside[1] - (drum.y - here.y)
    barge.set_actor_location(unreal.Vector(mx, my, here.z), False, True)
    barge.set_editor_property('hose_target', unreal.Vector(*ridge))
    hull = barge.get_editor_property('hull').static_mesh
    size = hull.get_bounding_box().get_editor_property('max') if hull else None
    top = size.z if size else 0.0
    # The kit's deck over her keel, as the placed actor has it: the hatch spot is the deck's walking surface in her frame
    # (BargeRules::DeckTop plus her KitLift, through her own ToWorld), so this follows the C++ and not a number copied here.
    deck = barge.get_hatch_spot().z - here.z
    try:
        lift = f'{barge.get_editor_property("kit_lift"):.0f}'
    except Exception:
        lift = 'none'
    build.log(f'the barge over the chapel at ({mx:.0f}, {my:.0f}, {here.z:.0f}), yaw {yaw:.0f} (her berth side to the '
              f'ledge), her hose to the ridge ({ridge[0]:.0f}, {ridge[1]:.0f}, {ridge[2]:.0f}); hull mesh top {top:.0f} cm over her '
              f'keel (stacks and bridge included), the kit\'s deck {deck:.0f} cm over her keel as the placed actor reports it '
              f'(kit_lift {lift}), {top - deck:.0f} cm under the mesh top (the mesh top is the stacks\'; the deck is fitted to the '
              f'hull\'s own open deck in BargeRules, not here)')
    return barge


# ---------------------------------------------------------------------------
# Her fights, horns and manifest
# ---------------------------------------------------------------------------

def make_group(cls_name, count, wave, level, rank='BASIC', health=1.0, leader=False, name=''):
    creature = story.actor_class(cls_name)
    if creature is None:
        return None
    made = story.group(creature, count, rank, wave, wave, health_scale=health, leader=leader)
    made.set_editor_property('level', level)
    if name:
        made.set_editor_property('display_name', unreal.Text(name))
    return made


def place_spawner(build, barge, fight, groups, middle, activation, when, waves, wave_gap, wait, most):
    cls = story.actor_class('EncounterSpawner')
    groups = [g for g in groups if g is not None]
    spots = list(barge.get_fight_spots(unreal.Name(fight)))
    if cls is None or not groups or not spots:
        build.warn(f'{fight}: no EncounterSpawner class, no Dredger classes or no spots: left out')
        return None
    label = f'Encounter_{fight}'
    remove_stale(build, label)
    spawner = build.place(cls, (middle.x, middle.y, middle.z + 50.0), 0.0, label=label, folder=FOLDER)
    spawner.set_editor_property('spawner_id', unreal.Name(fight))
    spawner.set_editor_property('groups', groups)
    spawner.set_editor_property('num_waves', waves)
    spawner.set_editor_property('wave_interval', wave_gap)
    spawner.set_editor_property('wait_for_clear', wait)
    spawner.set_editor_property('max_alive', most)
    spawner.set_editor_property('creature_tags', [unreal.Name(f'Dredger_{fight}')])
    spawner.set_editor_property('active_when', when)
    spawner.set_editor_property('activation_radius', activation)
    spawner.set_editor_property('hunt_on_spawn', True)
    # Her deck and her hall are high over the knoll: the ground the spots stand on is hers, at their own height (each spot is
    # kept relative to the spawner, which stands 50 cm over the getter's middle).
    spawner.set_editor_property('spawn_points', [unreal.Vector(p.x - middle.x, p.y - middle.y, p.z - middle.z - 50.0) for p in spots])
    spawner.set_editor_property('ground_max_rise', 300.0)
    attach(spawner, barge)
    count = sum(g.get_editor_property('count') for g in groups)
    # Heights over her keel: what to look for in the editor after a rebuild (they come from her getters, not from here).
    keel = barge.get_actor_location().z
    low, high = min(p.z for p in spots) - keel, max(p.z for p in spots) - keel
    build.log(f'{fight}: {count} Dredgers on the barge ({len(spots)} spots {low:.0f}..{high:.0f} cm over her keel, spawner '
              f'{middle.z + 50.0 - keel:.0f} cm over it, {waves} pushes)')
    return spawner


def place_fights(build, barge):
    deck = [make_group(name, count, wave, DECK_LEVEL) for name, count, wave in DECK_GROUPS]
    place_spawner(build, barge, DECK_FIGHT, deck, barge.get_deck_middle(), DECK_ACTIVATION,
                  story.condition(during=MAIN6, from_step=GRAPPLE_STEP, before_step=MANIFEST_STEP), 2, DECK_WAVE_GAP, False,
                  DECK_MAX_ALIVE)
    hall = [
        make_group('DredgerHoseMan', 1, 1, CHIEF_LEVEL, rank='HALLOWED', health=CHIEF_HEALTH, leader=True, name='The Chief Stoker'),
        make_group('DredgerRifleman', 2, 1, HALL_LEVEL),
        make_group('DredgerHoseMan', 1, 1, HALL_LEVEL),
        make_group('DredgerRifleman', 2, 2, HALL_LEVEL),
        make_group('DredgerHoseMan', 2, 2, HALL_LEVEL),
    ]
    place_spawner(build, barge, HALL_FIGHT, hall, barge.get_hall_middle(), HALL_ACTIVATION,
                  story.condition(during=MAIN6, from_step=FURNACE_STEP, before_step=FURNACE_STEP + 1), 2, 0.0, True, HALL_MAX_ALIVE)


def place_horns(build, barge):
    cls = story.actor_class('CompanyLoudspeaker')
    if cls is None:
        build.warn('no CompanyLoudspeaker class: no horns on the barge')
        return
    main2 = importlib.import_module('build_area_main2')
    lines = story.lines(build, VANE_DECK_LINES)
    for index, spot in enumerate(barge.get_horn_spots()):
        label = f'{HORN_TAG}_{index + 1}'
        remove_stale(build, label)
        at = spot.translation
        horn = build.place(cls, (at.x, at.y, at.z), spot.rotation.rotator().yaw, label=label, folder=FOLDER,
                           tags=main2.LOUDSPEAKER_TAGS + (HORN_TAG,))
        cues = []
        if index == 0 and lines is not None:
            # Vane welcomes the boarders from the bow's horn as the plank lands.
            cue = unreal.LoudspeakerCue()
            cue.set_editor_property('when', story.condition(during=MAIN6, from_step=DECK_STEP))
            cue.set_editor_property('lines', lines)
            cues.append(cue)
        horn.set_editor_property('cues', cues)
        horn.set_editor_property('earshot_radius', 5000.0)
        attach(horn, barge)
    build.log(f'four Company horns on the barge (tagged {HORN_TAG}; the bow\'s carries {VANE_DECK_LINES})')


def place_manifest(build, barge):
    cls = story.actor_class('MissionItem')
    if cls is None:
        build.warn('no MissionItem class: no manifest on the stoker\'s desk')
        return
    remove_stale(build, 'Item_BargeManifest')
    at = barge.get_office_spot()
    item = build.place(cls, (at.x, at.y, at.z + 3.0), barge.get_actor_rotation().yaw, label='Item_BargeManifest', folder=FOLDER)
    item.set_editor_property('item_id', unreal.Name(MANIFEST_ID))
    when = story.condition(during=MAIN6, from_step=MANIFEST_STEP, before_step=MANIFEST_STEP + 1)
    item.set_editor_property('shown_when', when)
    item.set_editor_property('take_when', when)
    model, path = first_model(MANIFEST_MODELS)
    if model is not None:
        item.get_editor_property('mesh').set_static_mesh(model)
    attach(item, barge)
    look = path or "the item's stand-in"
    build.log(f'the barge\'s manifest ({MANIFEST_ID}, {look}) on the stoker\'s desk at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')


# ---------------------------------------------------------------------------
# The wreck in the canyon; Landfall's barge put away
# ---------------------------------------------------------------------------

def place_wreck(build, barge):
    deck = story.placed(build, 'BurialDeck')
    if deck is None:
        build.warn('no burial deck: the barge falls on west, and there\'s no wreck to see')
        return
    at = deck.get_actor_location()
    forward = deck.get_actor_forward_vector()
    flat = math.hypot(forward.x, forward.y) or 1.0
    fx, fy = forward.x / flat, forward.y / flat
    wx, wy = at.x + fx * WRECK_OUT - WRECK_SOUTH, at.y + fy * WRECK_OUT
    # A collidable floor if one is ever added; else the escarpment's drop (the canyon floor's mesh has no collision).
    floor = trace_down(wx, wy, at.z - 500.0, at.z - 40000.0)
    if floor is None:
        drop = build.source.get('region', {}).get('escarpment', {}).get('drop')
        if drop is None:
            build.warn(f'no canyon floor under ({wx:.0f}, {wy:.0f}) and no escarpment drop in the layout: the wreck lies '
                       f'{WRECK_FALLBACK_DOWN / 100:.0f} m under the boards')
            drop = WRECK_FALLBACK_DOWN
        floor = at.z - float(drop)
    barge.set_editor_property('fall_target', unreal.Vector(wx, wy, floor + 300.0))
    barge.set_editor_property('boards_point', unreal.Vector(at.x, at.y, at.z + 200.0))
    import story_texts
    texts = importlib.reload(story_texts)
    shown = [texts.condition(dict(with_flags=[FELL_FLAG])), story.condition(after=[MAIN6])]
    model, path = first_model(WRECK_MODELS)
    look = load(SCORCHED_LOOK) if path != WRECK_MODELS[0] else None
    story_prop(build, model, (wx, wy, floor - WRECK_SUNK), unreal.Rotator(roll=WRECK_TURN[1], pitch=WRECK_TURN[0], yaw=barge.get_actor_rotation().yaw + 35.0),
               1.0, 'Main6_BargeWreck', shown=shown, look=look)
    smoke, smog = load(SMOKE_MODEL), load(SMOG_LOOK)
    for k in range(SMOKE_STACK):
        story_prop(build, smoke, (wx, wy, floor + k * 800.0 * SMOKE_SCALE * 0.8), unreal.Rotator(roll=0.0, pitch=0.0, yaw=19.0 + 40.0 * k),
                   SMOKE_SCALE, f'Main6_WreckSmoke_{k + 1}', shown=shown, hidden=[story.condition(after=[MAIN7])], look=smog)
    build.log(f'the barge\'s wreck in Gravewind Canyon at ({wx:.0f}, {wy:.0f}, {floor:.0f}) ({path or "no hull model"}), its smoke '
              f'column rising past the Rim, shown from {FELL_FLAG} or after {MAIN6}; her fall ends there')


def same_condition(a, b):
    try:
        return bool(a == b)
    except Exception:
        return False


def tag_conditions(actor, prop, when):
    try:
        held = [c for c in actor.get_editor_property(prop)]
    except Exception:
        return False
    # Only what isn't there yet: this module runs alone too (run() below) on pieces Landfall's build left, and a second
    # run must not stack the same condition again.
    actor.set_editor_property(prop, held + [c for c in when if not any(same_condition(c, each) for each in held)])
    return True


def hide_sink_barge(build):
    """Landfall's barge over the Sink, its hose and plumes hidden during Main 6 (she's over the chapel), and Main 5's intake
    (its limp hose hung from her drum) gone during it too."""
    during = [story.condition(during=MAIN6)]
    pieces = [a for prefix in LANDFALL_PIECES for a in labelled(build, prefix)]
    hidden = sum(1 for a in pieces if tag_conditions(a, 'hidden_when', during))
    intakes = [a for a in actors.get_all_level_actors() if unreal.Name(INTAKE_TAG) in a.tags]
    for intake in intakes:
        tag_conditions(intake, 'gone_when', during)
    if not pieces:
        build.warn('no Landfall barge pieces found (build_area_landfall.py runs first): the Sink barge stays during Main 6')
    build.log(f'Landfall\'s barge over the Sink ({hidden} prop(s)) hidden during {MAIN6}; {len(intakes)} intake(s) gone during it '
              f'(its drone, an ambient emitter, keeps droning: no story condition on it)')


def place(build):
    """Everything above (build_area_story.place calls this after Main 5's pieces, before Hob). Returns Hob's spot."""
    spots = {'wheel': None}
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return spots
    barge = place_barge(build)
    if barge is None:
        return spots
    place_fights(build, barge)
    place_horns(build, barge)
    place_manifest(build, barge)
    place_wreck(build, barge)
    hide_sink_barge(build)
    try:
        sortie = importlib.reload(importlib.import_module('build_area_main6_sortie'))
    except ImportError as error:
        build.warn(f'no build_area_main6_sortie ({error}): the Lamplighter has no course to the barge')
        return spots
    made = sortie.place(build, barge) or {}
    spots.update(made)
    return spots


def run(name):
    """Places Gameplay/Main6 again on its own (the barge and what stands on and in her, the wreck and its smoke, the sortie) and
    saves; Hob's wheel spot is not placed again (it doesn't move). Needs the rest of the gameplay pass standing: the chapel, the
    Lamplighter and her winch, Landfall's barge, the burial deck."""
    import build_area  # noqa: E402 (here: build_area imports build_area_story, which this module uses)
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('Main 6\'s pieces placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_main6.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
