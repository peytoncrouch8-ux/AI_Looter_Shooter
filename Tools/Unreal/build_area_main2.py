"""Main 2, "The Ferryman's Price" (the Wardens story, 2026-11-01; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 4,
lane MAIN23), its world pieces on Ransom's Rest, for build_area_story.py: its place() calls place() here (reloaded each
run, as the editor keeps modules between runs) after the deck (the biers, the Keeper's Gate) and Landfall, before Hob
(who takes the spots this hands back). Everything goes in the area's Gameplay/Main2 folder, so a "gameplay" build places
it all again; a piece whose class, model or ground isn't there yet is left out with a warning.

What it places, from the built level (the burial deck's sockets, the layout's features and zones, traces):
- The Three Widows' spider nest (AEncounterSpawner BluffNest, moved from Ransom's Point's top: Landfall climbs the Point
  now) in the slabs' lee by the keeper's path: four spiders and a Restless one in two pushes, two out at most (the user's
  "Soften early only" numbers kept), hunting only round the Widows; Hob's word on the blue one as the last falls.
- Keeper's Bridge's word: an unseen speaker point on Place_KeepersBridge that has Hob mutter, during Main 2, as the
  player crosses (the optional's reason: Abel walked the dead over it every sundown).
- The Company loudspeaker at the Keeper's Gate (ACompanyLoudspeaker, Loudspeaker_KeepersGate): beside the south rock on
  the cairns' side, its horns at the coming player; Main 2's optional shoots it out, and Vane complains from it when the
  cart is stopped.
- The week's dead on the boards: six shrouded bodies (AStoryProps; stand-in SM_Cocoon_Lying in linen until the Tripo
  shrouded dead) on six of the eight biers, from Main 2 until Main 7 (Main 7's stakes); Pa's and Hob's biers stay bare.
- The Company's hose-cart (ACompanyHoseCart, HoseCart_Boards) in the deck's aisle toward its entrance, its hose run into
  the nearest shrouded body: there from Main 2's start, stoppable in its cart step (1), stopped after.
- The boards' crew (AAmbushSpawner BoardsCrew, entrance Rise, level 2 against the deck's 5-6): three Dredger riflemen in
  the biers' cover on the open front's side and a hose-man at the cart, standing at work (not hunting) as the player comes
  up the cairns; then the cart's spill brings its second wave, four Unpaid rising out of the black round the cart.
- The Ferryman's landing and ferry (ASextonsFerry) hung under the deck's open west end, Mister Sexton (AMisterSexton,
  Speaker_Sexton) in its stern from Main 2 on (he stays at his ferry), his topics story_texts.TOPICS['Speaker_Sexton'];
  the rope ladder down to it (AFerryLadder) from the deck's edge.
- Any stale copies of what this replaces (the old rail Sexton, the Point's nest) are taken out of the level first, so a
  build_area_story.py without lane MAIN23's edits still ends with one of each.
Returns Hob's Main 2 spots for build_area_story.place_hob: {'widows': (x, y, z, yaw) or None, 'ladderTop': ...}.
"""
import importlib
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Main2'
MAIN1, MAIN2, MAIN7 = 'Main1', 'Main2', 'Main7'

# ---------------------------------------------------------------------------
# Names (the C++ and the mission data's; one place)
# ---------------------------------------------------------------------------

SEXTON_TAG = 'Speaker_Sexton'
CART_TAG = 'HoseCart_Boards'
CREW = 'BoardsCrew'
CREW_TAG = 'Dredger_BoardsCrew'
NEST = 'BluffNest'
NEST_TAG = 'Spider_BluffNest'
GATE_LOUDSPEAKER = 'Loudspeaker_KeepersGate'
BRIDGE_MUTTER = 'Mutter_KeepersBridge'
# Tags the C++ gives its own (a placed actor's tags are the script's: build.place replaces the class defaults).
LOUDSPEAKER_TAGS = ('CompanyLoudspeaker', 'Obstacle')
CART_TAGS = ('HoseCart', 'Obstacle')

# Line sets (create_story_lines.py).
NEST_CLEARED = 'DA_Lines_HobMain2Blue'
CART_SPILL = 'DA_Lines_HobMain2Spill'
CART_COMPLAINT = 'DA_Lines_Main2VaneCart'

# ---------------------------------------------------------------------------
# Stand-ins (one path each; the Tripo models, TRIPO_MODELS.md "Mains 2-3", replace them: the first path that exists)
# ---------------------------------------------------------------------------

SHROUD_MODELS = ('/Game/Art/Tripo/SM_ShroudedDead', '/Game/Art/Props/SM_Cocoon_Lying')
SHROUD_LOOK = '/Game/Art/Materials/MI_ClothLinen'

# ---------------------------------------------------------------------------
# Numbers (cm, degrees)
# ---------------------------------------------------------------------------

# The nest (moved, numbers kept: the user's "Soften early only", 2026-10-23): four Basic and a Restless one in two pushes,
# two out at most, a breath between, lighter than their class; it comes as the player comes within 30 m.
NEST_PUSHES = ((2, 'BASIC', 1), (2, 'BASIC', 2), (1, 'RARE', 2))
NEST_MAX_ALIVE = 2
NEST_BREATH = 3.0
NEST_HEALTH = {'BASIC': 0.7, 'RARE': 0.6}
NEST_ACTIVATION = 3000.0
NEST_SPREAD = 900.0
# Its ground: round the three slabs and the keeper's path past them, this far out from them.
NEST_GROUND_PAD = 900.0
# The nest's middle: from the slabs' middle this share of the way toward the path's nearest point (their lee, by the path).
NEST_TOWARD_PATH = 0.35
WIDOWS = ('widow1', 'widow2', 'widow3')

# The shrouded dead: on these biers (BurialDeck.py's 1-8; build_area_deck's ABEL_BIER 3 and HOB_BIER 2 stay bare), lying
# along the bier, about this long, on its top (build_area_deck's BIER_TOP over the socket).
SHROUD_BIERS = (1, 4, 5, 6, 7, 8)
SHROUD_LENGTH = 175.0
# The cart: in the aisle, this far from its middle toward the entrance (the deck's back); its hose ends this far over the
# shrouded body's bier top.
CART_BACK = 350.0
HOSE_OVER_BIER = 25.0
# The spill's dead rise along the aisle round the cart (cart frame: X its handles' way, toward the open front).
SPILL_SPOTS = ((260.0, 60.0), (-260.0, -60.0), (430.0, -40.0), (-430.0, 40.0))
# The crew: riflemen behind the three biers nearest the open front, this far past each toward it (clear of the 3.1 m bier's
# end and a man's width: at 130 they stood on the biers, 2026-11-01); the hose-man beside the cart.
RIFLE_BEHIND = 220.0
HOSEMAN_SIDE = 95.0
# The deck is tagged Obstacle (the minimap draws it solid) and a spawner's creatures never stand on an obstacle's top: this
# tag makes its boards ground for them (FEncounterGroundProbe; the play-through of 2026-11-01 found no spot for the crew).
FLOOR_TAG = 'Floor'
CREW_LEVEL = 2
CREW_MAX_ALIVE = 6
CREW_ACTIVATION = 4500.0
# The loudspeaker: beside the Keeper's Gate's south rock, on the cairns' side (layout cm off the rock's middle), its horns
# turned to the cairns.
GATE_SPEAKER_OFF = (60.0, 380.0)
# The landing hangs under the deck's open front: its middle this far out past the front's edge (ASextonsFerry.DeckFrontX
# is minus this), its planks ASextonsFerry::ClimbDrop under the boards; the ladder this far along the edge from its middle.
LANDING_OUT = 100.0
CLIMB_DROP = 380.0
LADDER_ALONG = 150.0
# The ferry's fog bank (SM_CanyonFog_B round its waterline) is placed and sized by ASextonsFerry itself (FogAt, FogScale).
# Where Sexton's seat is from the landing's middle (ASextonsFerry: the ferry 330 out and 250 along, its seat 320 toward its
# stern, 8 up on a frame hung 20 under the planks), for placing him in the editor (play snaps him to the seat).
SEAT_FROM_LANDING = (330.0, -70.0, -12.0)
# Hob on the ladder's crossbar (AFerryLadder: 8 back from the edge, 113 up).
LADDER_PERCH = (-8.0, 0.0, 113.0)


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


def texts():
    import story_texts
    return importlib.reload(story_texts)


def deck_module():
    return importlib.import_module('build_area_deck')


def flat(d):
    length = math.hypot(d[0], d[1]) or 1.0
    return d[0] / length, d[1] / length


def yaw_of(d):
    return math.degrees(math.atan2(d[1], d[0]))


def remove_stale(build, label):
    """Takes out actors of this build labelled label (a copy another script placed, or an earlier run's). How many."""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    gone = [a for a in actors.get_all_level_actors() if str(a.get_actor_label()) == label and unreal.Name(build.tag) in a.tags]
    for actor in gone:
        actors.destroy_actor(actor)
    return len(gone)


def find_tagged(tag):
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if unreal.Name(tag) in actor.tags:
            return actor
    return None


def story_prop(build, model, location, rotator, scale, label, shown=(), hidden=(), look=None):
    """An AStoryProp (World/StoryProp.h): seen only, shown while any of shown holds and none of hidden."""
    cls = story.actor_class('StoryProp')
    if cls is None or model is None:
        if cls is None:
            build.warn(f'no StoryProp class (build the game module first): no {label}')
        return None
    prop = build.place(cls, location, 0.0, label=label, folder=FOLDER)
    prop.set_actor_rotation(rotator, False)
    prop.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    mesh = prop.get_editor_property('mesh')
    mesh.set_static_mesh(model)
    if look is not None:
        for slot in range(max(len(model.get_editor_property('static_materials')), 1)):
            mesh.set_material(slot, look)
    prop.set_editor_property('shown_when', list(shown))
    prop.set_editor_property('hidden_when', list(hidden))
    return prop


class Deck:
    """The burial deck as build_area.py placed it: its frame (its front the open end over the canyon), boards, sockets."""

    def __init__(self, build):
        self.actor = story.placed(build, 'BurialDeck')
        self.ok = self.actor is not None
        if not self.ok:
            return
        mod = deck_module()
        at = self.actor.get_actor_location()
        self.at = (at.x, at.y)
        self.boards = at.z + mod.DECK_SURFACE
        self.forward = flat(self.actor.get_actor_forward_vector().to_tuple())
        self.right = (-self.forward[1], self.forward[0])
        self.yaw = yaw_of(self.forward)
        self.front = (at.x + self.forward[0] * mod.DECK_HALF_LENGTH, at.y + self.forward[1] * mod.DECK_HALF_LENGTH)
        self.biers = {}
        for k in range(1, mod.BIERS + 1):
            spot = story.socket(self.actor, f'Bier_{k}')
            if spot is not None:
                self.biers[k] = spot
        aisle = [self.biers[k].translation for k in mod.AISLE_BIERS if k in self.biers]
        self.aisle = (sum(p.x for p in aisle) / len(aisle), sum(p.y for p in aisle) / len(aisle)) if aisle else self.at
        self.bier_top = mod.BIER_TOP

    def point(self, along, across, origin=None):
        """(x, y) along the deck's front way and across it from origin (the deck's middle)."""
        ox, oy = origin if origin else self.at
        return (ox + self.forward[0] * along + self.right[0] * across, oy + self.forward[1] * along + self.right[1] * across)

    def toward_front(self, xy):
        """How far xy is along the deck's front way from its middle."""
        return (xy[0] - self.at[0]) * self.forward[0] + (xy[1] - self.at[1]) * self.forward[1]


# ---------------------------------------------------------------------------
# The keeper's path: the Widows' nest, the bridge's word, the gate's loudspeaker
# ---------------------------------------------------------------------------

def nest_spot(build):
    """The nest's middle in the Widows' lee by the path, its ground round the slabs and the path, and the middle Widow's top
    for Hob. (None, None, None) without the layout's slabs."""
    rocks = [story.layout_entry(build, 'features', key) for key in WIDOWS]
    if any(rock is None for rock in rocks):
        return None, None, None
    xs = [rock['center'][0] for rock in rocks]
    ys = [rock['center'][1] for rock in rocks]
    middle = (sum(xs) / 3.0, sum(ys) / 3.0)
    road = story.layout_entry(build, 'roads', 'keepersPath')
    nearest = middle
    if road is not None:
        best = None
        for (ax, ay), (bx, by) in zip(road['path'], road['path'][1:]):
            dx, dy = bx - ax, by - ay
            t = max(0.0, min(1.0, ((middle[0] - ax) * dx + (middle[1] - ay) * dy) / max(dx * dx + dy * dy, 1.0)))
            px, py = ax + dx * t, ay + dy * t
            d = math.hypot(px - middle[0], py - middle[1])
            if best is None or d < best[0]:
                best = (d, (px, py))
        nearest = best[1]
    at = (middle[0] + (nearest[0] - middle[0]) * NEST_TOWARD_PATH, middle[1] + (nearest[1] - middle[1]) * NEST_TOWARD_PATH)
    lo_x = min(xs + [nearest[0]]) - NEST_GROUND_PAD
    hi_x = max(xs + [nearest[0]]) + NEST_GROUND_PAD
    lo_y = min(ys + [nearest[1]]) - NEST_GROUND_PAD
    hi_y = max(ys + [nearest[1]]) + NEST_GROUND_PAD
    ground = [(lo_x, lo_y), (hi_x, lo_y), (hi_x, hi_y), (lo_x, hi_y)]
    # Hob on the middle slab's top, looking down at the path.
    mx, my = rocks[1]['center']
    hob = (mx, my, story.ground_at(mx, my), story.facing((mx, my), nearest))
    return at, ground, hob


def place_nest(build):
    """The Three Widows' spiders (BluffNest), and Hob's spot on the middle slab."""
    remove_stale(build, f'Encounter_{NEST}')
    at, ground, hob = nest_spot(build)
    spider = story.actor_class('SpiderCreature')
    cls = story.actor_class('EncounterSpawner')
    if at is None or spider is None or cls is None:
        build.warn('no Three Widows in the layout, or no EncounterSpawner/SpiderCreature class: no nest on the keeper\'s path')
        return hob
    nx, ny = at
    nz = story.ground_at(nx, ny)
    nest = build.place(cls, (nx, ny, nz + 50.0), 0.0, label=f'Encounter_{NEST}', folder=FOLDER)
    nest.set_editor_property('spawner_id', unreal.Name(NEST))
    nest.set_editor_property('groups', [story.group(spider, count, rank, wave, wave, NEST_HEALTH[rank], leader=rank == 'RARE')
                                        for count, rank, wave in NEST_PUSHES])
    nest.set_editor_property('num_waves', max(wave for _, _, wave in NEST_PUSHES))
    nest.set_editor_property('wait_for_clear', True)
    nest.set_editor_property('wave_interval', NEST_BREATH)
    nest.set_editor_property('max_alive', NEST_MAX_ALIVE)
    nest.set_editor_property('creature_tags', [unreal.Name(NEST_TAG)])
    nest.set_editor_property('active_when', texts().condition(texts().ENCOUNTERS[NEST]))
    nest.set_editor_property('activation_radius', NEST_ACTIVATION)
    nest.set_editor_property('spawn_radius', NEST_SPREAD)
    nest.set_editor_property('ground_corners', [unreal.Vector(x, y, nz) for x, y in ground])
    cleared = story.lines(build, NEST_CLEARED)
    if cleared is not None:
        nest.set_editor_property('cleared_lines', cleared)
    build.log(f'the Three Widows\' nest ({NEST}: four spiders and a Restless one, {NEST_MAX_ALIVE} out at most) at '
              f'({nx:.0f}, {ny:.0f}, {nz:.0f}), on the keeper\'s path during {MAIN2}')
    return hob


def place_bridge_word(build):
    """Hob's word on Keeper's Bridge: an unseen speaker point that mutters as the player crosses (story_texts.MUTTER_POINTS)."""
    spec = texts().MUTTER_POINTS.get(BRIDGE_MUTTER)
    bridge = find_tagged('Place_KeepersBridge')
    cls = story.actor_class('SpeakerPoint')
    if spec is None or bridge is None or cls is None:
        build.warn('no Place_KeepersBridge (build_area_missionprops_valley.py) or SpeakerPoint class: no word on Keeper\'s Bridge')
        return
    at = bridge.get_actor_location()
    point = build.place(cls, (at.x, at.y, at.z + 60.0), 0.0, label=BRIDGE_MUTTER, folder=FOLDER, tags=(BRIDGE_MUTTER,))
    talk = point.get_editor_property('speaker_point')
    talk.set_editor_property('speaker_name', unreal.Text(spec['name']))
    talk.set_editor_property('talkable', False)
    talk.set_editor_property('mutter_radius', spec['radius'])
    talk.set_editor_property('mutters', story.topics(build, *[(texts().condition(when), name) for when, name in spec['mutters']]))
    build.log(f'Hob\'s word on Keeper\'s Bridge at ({at.x:.0f}, {at.y:.0f}), during {MAIN2}')


def loudspeaker_cues(build, tag):
    """FLoudspeakerCues from story_texts.LOUDSPEAKER_CUES[tag], each its condition and line set (a missing set left out)."""
    made = []
    for when, name in texts().LOUDSPEAKER_CUES.get(tag, []):
        line_set = story.lines(build, name)
        if line_set is None:
            continue
        cue = unreal.LoudspeakerCue()
        cue.set_editor_property('when', texts().condition(when))
        cue.set_editor_property('lines', line_set)
        made.append(cue)
    return made


def place_loudspeaker(build, x, y, yaw, tag, folder=FOLDER, show_post=True):
    """An ACompanyLoudspeaker on the ground at (x, y), its horns toward yaw, tagged tag, its cues from story_texts."""
    cls = story.actor_class('CompanyLoudspeaker')
    if cls is None:
        build.warn(f'no CompanyLoudspeaker class (build the game module first): no {tag}')
        return None
    remove_stale(build, tag)
    z = story.ground_at(x, y)
    horn = build.place(cls, (x, y, z), yaw, label=tag, folder=folder, tags=LOUDSPEAKER_TAGS + (tag,))
    horn.set_editor_property('show_post', show_post)
    horn.set_editor_property('cues', loudspeaker_cues(build, tag))
    build.log(f'the Company loudspeaker {tag} at ({x:.0f}, {y:.0f}, {z:.0f}), horns at yaw {yaw:.0f}'
              f'{"" if show_post else " (a voice only)"}')
    return horn


def place_gate_loudspeaker(build, deck):
    south = story.layout_entry(build, 'features', 'gateSouth')
    if south is None:
        return None
    sx, sy = south['center']
    x, y = sx + GATE_SPEAKER_OFF[0], sy + GATE_SPEAKER_OFF[1]
    # Its horns at the cairns the player comes up by (cairn3, the last before the gate), else away from the deck.
    cairn = next((spot for spot in build.layout['placements'].values() if spot['kind'] == 'Cairn_C'), None)
    if cairn is not None:
        yaw = story.facing((x, y), (cairn['location'][0], cairn['location'][1]))
    elif deck.ok:
        yaw = yaw_of((-deck.forward[0], -deck.forward[1]))
    else:
        yaw = 90.0
    return place_loudspeaker(build, x, y, yaw, GATE_LOUDSPEAKER)


# ---------------------------------------------------------------------------
# The boards: the dead, the cart, the crew
# ---------------------------------------------------------------------------

def place_shrouds(build, deck):
    """Six shrouded dead on their biers; returns their tops {bier number: (x, y, z)}."""
    model, path = first_model(SHROUD_MODELS)
    look = None if path == SHROUD_MODELS[0] else load(SHROUD_LOOK)
    tops = {}
    if model is None:
        build.warn(f'no shrouded dead model ({", ".join(SHROUD_MODELS)}): the boards are bare')
        return tops
    box = model.get_bounding_box()
    lo, hi = box.get_editor_property('min'), box.get_editor_property('max')
    along_x = (hi.x - lo.x) >= (hi.y - lo.y)
    length = max(hi.x - lo.x, hi.y - lo.y, 1.0)
    scale = SHROUD_LENGTH / length
    shown = [story.condition(after=[MAIN1])]
    hidden = [story.condition(after=[MAIN7])]
    for k in SHROUD_BIERS:
        spot = deck.biers.get(k)
        if spot is None:
            continue
        top = spot.translation.z + deck.bier_top
        turn = spot.rotation.rotator()
        rotator = unreal.Rotator(roll=0.0, pitch=0.0, yaw=turn.yaw + (0.0 if along_x else 90.0))
        at = (spot.translation.x, spot.translation.y, top - lo.z * scale)
        story_prop(build, model, at, rotator, scale, f'Main2_Shroud_{k}', shown, hidden, look)
        tops[k] = (spot.translation.x, spot.translation.y, top + (hi.z - lo.z) * scale)
    build.log(f'{len(tops)} shrouded dead on the boards (biers {", ".join(str(k) for k in tops)}), from {MAIN2} until {MAIN7}')
    return tops


def place_cart(build, deck, tops):
    """The hose-cart in the aisle toward the entrance, its hose in the nearest body. Returns (cart, its (x, y, z))."""
    cls = story.actor_class('CompanyHoseCart')
    if cls is None:
        build.warn('no CompanyHoseCart class (build the game module first): no hose-cart on the boards')
        return None, None
    x, y = deck.point(-CART_BACK, 0.0, deck.aisle)
    z = deck.boards
    cart = build.place(cls, (x, y, z), deck.yaw, label=CART_TAG, folder=FOLDER, tags=CART_TAGS + (CART_TAG,))
    frame = cart.get_actor_transform()
    if tops:
        k, (tx, ty, tz) = min(tops.items(), key=lambda item: math.hypot(item[1][0] - x, item[1][1] - y))
        cart.set_editor_property('hose_end', frame.inverse_transform_location(unreal.Vector(tx, ty, tz - HOSE_OVER_BIER * 0.4)))
    cart.set_editor_property('shown_when', story.condition(during=MAIN2))
    # Stoppable in its own step (from 0: 1), so the step listening for it is the one being played.
    cart.set_editor_property('pumping_when', story.condition(during=MAIN2, from_step=1, before_step=2))
    cart.set_editor_property('stopped_when', [story.condition(during=MAIN2, from_step=2), story.condition(after=[MAIN2])])
    cart.set_editor_property('crew_spawner_id', unreal.Name(CREW))
    cart.set_editor_property('spill_spots', [unreal.Vector(sx, sy, 0.0) for sx, sy in SPILL_SPOTS])
    cart.set_editor_property('loudspeaker_tag', unreal.Name(GATE_LOUDSPEAKER))
    complaint = story.lines(build, CART_COMPLAINT)
    if complaint is not None:
        cart.set_editor_property('stop_broadcast', complaint)
    spill = story.lines(build, CART_SPILL)
    if spill is not None:
        cart.set_editor_property('spill_lines', spill)
    build.log(f'the Company\'s hose-cart in the boards\' aisle at ({x:.0f}, {y:.0f}, {z:.0f}), its hose in a body on bier '
              f'{min(tops, key=lambda k: math.hypot(tops[k][0] - x, tops[k][1] - y)) if tops else "(none)"}')
    return cart, (x, y, z)


def place_crew(build, deck, cart_at):
    """The boards' crew: three riflemen behind the front-most biers, a hose-man at the cart (wave 1); the cart's spill brings
    four Unpaid (wave 2, on the cart's word: no timer)."""
    cls = story.actor_class('AmbushSpawner')
    names = (('DredgerRifleman', 3, 1), ('DredgerHoseMan', 1, 1), ('UnpaidCreature', 4, 2))
    groups, missing = [], []
    for name, count, wave in names:
        creature = story.actor_class(name)
        if creature is None:
            missing.append(name)
            continue
        made = story.group(creature, count, 'BASIC', wave, wave)
        # The Rest's second story fight at the area's level, not the deck's 5-6 (Abel's adds'): the user's levels rule.
        made.set_editor_property('level', CREW_LEVEL)
        groups.append(made)
    if cls is None or not groups or cart_at is None:
        build.warn(f'no AmbushSpawner class, no crew classes ({", ".join(missing) or "-"}) or no cart: no crew on the boards')
        return
    if missing:
        build.warn(f'{CREW}: no {", ".join(missing)} class yet: those are left out')
    remove_stale(build, f'Encounter_{CREW}')
    tags = [str(t) for t in deck.actor.tags]
    if FLOOR_TAG not in tags:
        deck.actor.set_editor_property('tags', [unreal.Name(t) for t in tags + [FLOOR_TAG]])
    sx, sy = deck.aisle
    sz = deck.boards
    spawner = build.place(cls, (sx, sy, sz + 50.0), 0.0, label=f'Encounter_{CREW}', folder=FOLDER)
    # The wave-1 spots in the groups' order: the riflemen behind the three biers nearest the open front, then the hose-man.
    front_biers = sorted(deck.biers.values(), key=lambda spot: -deck.toward_front((spot.translation.x, spot.translation.y)))[:3]
    spots = [deck.point(RIFLE_BEHIND, 0.0, (spot.translation.x, spot.translation.y)) for spot in front_biers]
    spots.append(deck.point(0.0, HOSEMAN_SIDE, (cart_at[0], cart_at[1])))
    spawner.set_editor_property('spawner_id', unreal.Name(CREW))
    spawner.set_editor_property('groups', groups)
    spawner.set_editor_property('num_waves', 2)
    # The second wave only on the cart's word (ACompanyHoseCart::RaiseSpill), never a timer.
    spawner.set_editor_property('wave_interval', 0.0)
    spawner.set_editor_property('wait_for_clear', False)
    spawner.set_editor_property('max_alive', CREW_MAX_ALIVE)
    spawner.set_editor_property('creature_tags', [unreal.Name(CREW_TAG)])
    spawner.set_editor_property('active_when', texts().condition(texts().ENCOUNTERS[CREW]))
    spawner.set_editor_property('activation_radius', CREW_ACTIVATION)
    # At work, not hunting, as the player comes up the cairns: the player picks the moment (the spill's dead come on anyway).
    spawner.set_editor_property('hunt_on_spawn', False)
    spawner.set_editor_property('entrance', unreal.AmbushEntrance.RISE)
    spawner.set_editor_property('spawn_points', [unreal.Vector(x - sx, y - sy, 0.0) for x, y in spots])
    zone = story.layout_entry(build, 'zones', 'gravewindPoint')
    if zone is not None:
        spawner.set_editor_property('ground_corners', [unreal.Vector(x, y, sz) for x, y in zone['polygon']])
    spawner.set_editor_property('ground_max_rise', 600.0)
    build.log(f'the boards\' crew ({CREW}: three riflemen and a hose-man, then the spill\'s four dead, level {CREW_LEVEL}) at '
              f'({sx:.0f}, {sy:.0f}), during {MAIN2}')


# ---------------------------------------------------------------------------
# Under the boards: the landing, the ferry, Sexton, the ladder
# ---------------------------------------------------------------------------

def check_landing(build, deck, middle, z):
    """Warns if the cliff or the deck's own frame stands in the landing's room (its corners, from under the boards down)."""
    for along in (-200.0, 200.0):
        for across in (-300.0, 300.0):
            x, y = deck.point(along, across, middle)
            hit = unreal.SystemLibrary.line_trace_single(unreal.EditorLevelLibrary.get_editor_world(),
                                                         unreal.Vector(x, y, deck.boards - 60.0), unreal.Vector(x, y, z - 100.0),
                                                         unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [deck.actor],
                                                         unreal.DrawDebugTrace.NONE, True)
            if hit is not None:
                t = hit.to_tuple()
                build.warn(f'the Ferryman\'s landing: something ({t[9].get_actor_label() if t[9] else "?"}) stands in its '
                           f'room at its corner ({x:.0f}, {y:.0f}), {t[5].z:.0f} high: move LANDING_OUT')


def place_ferry(build, deck):
    """The landing and the ferry under the deck's open west end, Sexton in its stern, the ladder down to it. Returns Hob's
    spot on the ladder's crossbar."""
    ferry_cls = story.actor_class('SextonsFerry')
    ladder_cls = story.actor_class('FerryLadder')
    sexton_cls = story.actor_class('MisterSexton')
    if ferry_cls is None or ladder_cls is None:
        build.warn('no SextonsFerry or FerryLadder class (build the game module first): no Ferryman\'s landing')
        return None
    mx, my = deck.point(LANDING_OUT, 0.0, deck.front)
    mz = deck.boards - CLIMB_DROP
    check_landing(build, deck, (mx, my), mz)
    ferry = build.place(ferry_cls, (mx, my, mz), deck.yaw, label='SextonsFerry', folder=FOLDER, tags=('Obstacle', 'SextonsFerry'))
    ferry.set_editor_property('deck_front_x', -LANDING_OUT)

    # Sexton in the stern (play snaps him to the seat, so he sways with it); there from Main 2 on: he stays at his ferry.
    remove_stale(build, 'MisterSexton')
    remove_stale(build, 'MisterSexton_Ferry')
    if sexton_cls is None:
        build.warn('no MisterSexton class (build the game module first): the ferry has no Ferryman')
    else:
        seat = ferry.get_actor_transform().transform_location(unreal.Vector(*SEAT_FROM_LANDING))
        sexton = build.place(sexton_cls, (seat.x, seat.y, seat.z), deck.yaw + 180.0, label='MisterSexton_Ferry', folder=FOLDER,
                             tags=(SEXTON_TAG,))
        sexton.set_editor_property('shown_when', story.condition(after=[MAIN1]))
        talk = sexton.get_editor_property('speaker_point')
        spec = texts().TOPICS[SEXTON_TAG]
        talk.set_editor_property('speaker_name', unreal.Text(spec['name']))
        default = story.lines(build, spec['default']) if spec.get('default') else None
        if default is not None:
            talk.set_editor_property('line_set', default)
        talk.set_editor_property('topics', story.texts_topics(build, SEXTON_TAG))
        ferry.set_editor_property('passenger', sexton)

    lx, ly = deck.point(0.0, LADDER_ALONG, deck.front)
    ladder = build.place(ladder_cls, (lx, ly, deck.boards), deck.yaw, label='FerryLadder', folder=FOLDER, tags=('FerryLadder',))
    ladder.set_editor_property('drop', CLIMB_DROP)
    perch = ladder.get_actor_transform().transform_location(unreal.Vector(*LADDER_PERCH))
    build.log(f'the Ferryman\'s landing and ferry under the deck\'s west end at ({mx:.0f}, {my:.0f}, {mz:.0f}), Sexton in its '
              f'stern from {MAIN2}, the ladder at ({lx:.0f}, {ly:.0f})')
    return perch.x, perch.y, perch.z, deck.yaw


# ---------------------------------------------------------------------------

def place(build):
    """Everything above (build_area_story.place calls this after Landfall, before Hob). Returns Hob's Main 2 spots."""
    spots = {'widows': None, 'ladderTop': None}
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return spots
    spots['widows'] = place_nest(build)
    # Hob's word on Keeper's Bridge waits for the bridge's marker: build_area.py calls place_bridge_word after the valley's
    # mission pieces (build_area_missionprops_valley.py) place Place_KeepersBridge.
    deck = Deck(build)
    place_gate_loudspeaker(build, deck)
    if not deck.ok:
        build.warn('no burial deck: Main 2\'s boards, cart, crew and ferry are left out (build the whole level first)')
        return spots
    tops = place_shrouds(build, deck)
    _, cart_at = place_cart(build, deck, tops)
    place_crew(build, deck, cart_at)
    spots['ladderTop'] = place_ferry(build, deck)
    return spots
