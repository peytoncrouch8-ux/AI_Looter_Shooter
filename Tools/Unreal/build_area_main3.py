"""Main 3, "Under New Management" (the Wardens story, 2026-11-01; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 5,
lane MAIN23), its world pieces on Ransom's Rest, for build_area_story.py: its place() calls place() here (reloaded each
run, as the editor keeps modules between runs) after Landfall (whose barge Ned's skiff leaves and whose Main Street
loudspeaker posts this gives a voice) and before Hob. Everything goes in the area's Gameplay/Main3 folder, so a
"gameplay" build places it all again; a piece whose class, model or ground isn't there yet is left out with a warning.

What it places (the town gate's posts and Main Street's buildings from the layout, the facades found with traces):
- The Company's checkpoint at the town gate: sandbag walls just inside it (SM_FlourSacks, solid: the riflemen's cover), a
  barrier pole across the gate on two posts (an ACompanySign board, gone once Main 3 is done: the checkpoint broken), and
  the board on the road side (ACompanySign: VANE CONSOLIDATED / ASSESSMENT IN PROGRESS / NO ENTRY. NO EXIT.).
- The gate's fight (AAmbushSpawner TownGate, entrance Rise; moved here from build_area_story.py, its id and its count of
  seven kept): three Dredger riflemen behind the sandbags and a hose-man at the barrier, standing guard as the player comes
  up the orchard (from Main 3's first step), then three Unpaid drawn by the smog (one Restless) rising north-east of the
  gate and on the street's south side as the men fall (four out at most: today's soft rule).
- The Company loudspeakers: one at the gate (ACompanyLoudspeaker Loudspeaker_TownGate: "Main Street is closed for
  assessment" as the checkpoint's fight begins), and a voice on Main Street's middle post (WORLD's instanced post, a voice
  only: "Anyone hiding a Warden will be assessed first" once the street is won).
- The UNDER NEW MANAGEMENT banner across Main Street by the Gilt Spur (ACompanyBanner Banner_MainStreet), tied to the
  facades either side; its two ropes are Main 3's optional.
- ASSESSED chalked on the street's doors (ACompanySign, chalk): each house counted, its dead next; not on Bright & Daughter.
- A hose-cart "assessing" the Gilt Spur (ACompanyHoseCart HoseCart_GiltSpur), its hose in the saloon's door, pumping
  during Main 3 and standing stopped after (no crew of its own: Main Street's set piece brings the men).
- Lucky Ned's skiff leaving (AGangSkiffFlyBy): off Vane's barge (the actor tagged Landfall_Barge) and away south over the
  Hogback's crest, as Main 3 starts, once a campaign (Main3.NedLeft).
Hob's Main 3 perches are build_area_story's spots (the gate's post, the undertaker's sign): nothing to hand back.
"""
import importlib
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Main3'
MAIN1, MAIN3, MAIN6 = 'Main1', 'Main3', 'Main6'

# ---------------------------------------------------------------------------
# Names (the C++ and the mission data's; one place)
# ---------------------------------------------------------------------------

GATE = 'TownGate'
GATE_TAGS = ('Unpaid_TownGate', 'Dredger_TownGate')
GATE_LOUDSPEAKER = 'Loudspeaker_TownGate'
STREET_LOUDSPEAKER = 'Loudspeaker_MainStreet'
BANNER_TAG = 'Banner_MainStreet'
BANNER_EVENT = 'MainStreet.BannerDown'
SALOON_CART = 'HoseCart_GiltSpur'
CART_TAGS = ('HoseCart', 'Obstacle')
# WORLD's Main Street loudspeaker posts carry this at their horns (build_area_landfall.py NAMES['loudspeaker_tag']).
WORLD_SPEAKER_TAG = 'Landfall_Loudspeaker'
BARGE_TAG = 'Landfall_Barge'
RADIO_LINES = 'DA_Lines_Main3Radio'
AFTER_LINES = 'DA_Lines_Main3NedAfter'

# ---------------------------------------------------------------------------
# Stand-ins (one path each; the Tripo checkpoint pieces, TRIPO_MODELS.md "Mains 2-3", replace them: the first that exists)
# ---------------------------------------------------------------------------

SANDBAG_MODELS = ('/Game/Art/Tripo/SM_CompanySandbags', '/Game/Art/Props/SM_FlourSacks')

# ---------------------------------------------------------------------------
# Numbers (cm, degrees; the gate's frame: "in" along Main Street from the gate, "side" across it)
# ---------------------------------------------------------------------------

# The sandbag walls just inside the gate (in, side), each this many sacks wide; the riflemen behind them, the hose-man at
# the barrier; then the Unpaid's spots (the old fight's, layout X north and Y east off the gate's middle: north-east of the
# gate and on the street's south side, two ways at once).
SANDBAG_WALLS = ((400.0, -330.0), (340.0, 0.0), (400.0, 330.0))
SANDBAGS_PER_WALL = 2
SANDBAG_STEP = 110.0
RIFLE_SPOTS = ((540.0, -330.0), (480.0, 0.0), (540.0, 330.0))
# Each rifleman keeps within this of his spot (FEncounterGroup.PostRadius): along the sandbags, never out into the field (one
# walked 20 m out in the play check of 2026-11-01).
RIFLE_POST = 600.0
HOSEMAN_SPOT = (200.0, 150.0)
UNPAID_SPOTS = ((600.0, 900.0), (1300.0, 1300.0), (-400.0, 700.0), (-1300.0, 300.0))
GATE_MAX_ALIVE = 4
GATE_ACTIVATION = 7000.0
GATE_GIVE_UP = 3000.0
# The barrier pole across the gate's opening (a long, thin ACompanySign board on two posts), and the road-side board.
BARRIER_IN = 100.0
# The gate's road height is looked for this far out and in from its line (clear of the arch over it).
GATE_CLEAR = 300.0
BARRIER_SIZE = (900.0, 14.0)
BARRIER_LIFT = 90.0
SIGN_AT = (-150.0, 260.0)
SIGN_SIZE = (300.0, 130.0)
SIGN_WORDS = 'VANE CONSOLIDATED\nASSESSMENT IN PROGRESS\nNO ENTRY. NO EXIT.'
BARRIER_WORDS = 'HALT  -  VANE CONSOLIDATED  -  HALT'
# The gate's loudspeaker just outside the south post (layout cm off the gate's middle), its horns at the farm road.
GATE_SPEAKER_AT = (-700.0, 150.0)
# Main Street's middle post (WORLD's), whose horns get a voice; searched for within this of it.
STREET_SPEAKER = (-700.0, 2600.0)
STREET_SPEAKER_REACH = 1500.0
# The banner: across the street here (layout Y), this high over it; its ropes tied where traces at that height meet the
# facades (else this far out each side).
BANNER_Y = 3200.0
BANNER_HIGH = 520.0
BANNER_FALLBACK = 850.0
BANNER_WORDS = 'UNDER NEW MANAGEMENT'
# The chalk: on these buildings (layout placement ids; not Bright & Daughter), the count of souls the Company wrote on each.
CHALKED = (('cabinNorthWest', 3), ('sheriff', 1), ('cottageNorth', 5), ('saloon', 4), ('cabinNorth', 2),
           ('cottageNorthEast', 6), ('cottageSouthWest', 4), ('cabinSouth', 2), ('store', 3), ('cottageSouth', 7))
CHALK_HIGH = 150.0
CHALK_LETTERS = 16.0
CHALK_SLANTS = (-6.0, 4.0, -3.0, 7.0, -5.0)
# The Gilt Spur's cart: this far out from its facade, a little up the street; its hose in the door.
SALOON_CART_OUT = 320.0
SALOON_CART_ALONG = 150.0
# Ned's skiff: from beside the barge (off its keel's middle), climbing away south over the Hogback's crest (layout X) and
# out of the valley; never lower than this over the ground under it.
SKIFF_OFF_BARGE = ((-2500.0, -1500.0, 300.0), (-6000.0, -3000.0, 1200.0))
SKIFF_HOGBACK = (-15000.0, 0.0)
SKIFF_OUT = (-32000.0, -3000.0)
SKIFF_CLEAR = 4500.0
SKIFF_SECONDS = 22.0


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load(path):
    return unreal.load_asset(path) if path and unreal.EditorAssetLibrary.does_asset_exist(path) else None


def first_model(paths):
    for path in paths:
        model = load(path)
        if model is not None:
            return model
    return None


def texts():
    import story_texts
    return importlib.reload(story_texts)


def main2():
    return importlib.reload(importlib.import_module('build_area_main2'))


def flat(d):
    length = math.hypot(d[0], d[1]) or 1.0
    return d[0] / length, d[1] / length


def yaw_of(d):
    return math.degrees(math.atan2(d[1], d[0]))


def world():
    return unreal.EditorLevelLibrary.get_editor_world()


def trace(a, b, ignore=()):
    """The first thing on the line from a to b ((x, y, z) each): (point, normal, actor), or None."""
    hit = unreal.SystemLibrary.line_trace_single(world(), unreal.Vector(*a), unreal.Vector(*b),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, list(ignore),
                                                 unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        return None
    t = hit.to_tuple()
    return t[4], t[7], t[9]


def placement(build, key):
    spot = build.layout['placements'].get(key)
    return spot


class Gate:
    """The town gate between its posts, and its frame: in along Main Street, side across it."""

    def __init__(self, build):
        posts = [story.layout_entry(build, 'obstacles', key) for key in ('townGateNorth', 'townGateSouth')]
        self.ok = all(post is not None for post in posts)
        if not self.ok:
            return
        middles = [story.centroid(post['polygon']) for post in posts]
        self.at = ((middles[0][0] + middles[1][0]) * 0.5, (middles[0][1] + middles[1][1]) * 0.5)
        street = story.layout_entry(build, 'roads', 'mainStreet')
        ahead = street['path'][1] if street and len(street['path']) > 1 else (self.at[0], self.at[1] + 1000.0)
        self.inward = flat((ahead[0] - self.at[0], ahead[1] - self.at[1]))
        self.side = (-self.inward[1], self.inward[0])
        # The road's height either side of the gate's line, not under its middle: straight down there the first thing met
        # is the arch's top beam, and the barrier hung 4 m up on it (the play-through of 2026-11-01).
        self.z = sum(story.ground_at(*self.point(along, 0.0)) for along in (-GATE_CLEAR, GATE_CLEAR)) * 0.5

    def point(self, along_in, along_side):
        return (self.at[0] + self.inward[0] * along_in + self.side[0] * along_side,
                self.at[1] + self.inward[1] * along_in + self.side[1] * along_side)


def company_sign(build, x, y, yaw, label, words, look='BOARD', size=(260.0, 120.0), lift=90.0, letters=24.0, slant=0.0,
                 shown=(), hidden=(), z=None):
    cls = story.actor_class('CompanySign')
    if cls is None:
        build.warn(f'no CompanySign class (build the game module first): no {label}')
        return None
    gz = story.ground_at(x, y) if z is None else z
    sign = build.place(cls, (x, y, gz), yaw, label=label, folder=FOLDER)
    sign.set_editor_property('text', unreal.Text(words))
    sign.set_editor_property('look', getattr(unreal.CompanySignLook, look))
    sign.set_editor_property('board_size', unreal.Vector2D(size[0], size[1]))
    sign.set_editor_property('board_lift', lift)
    sign.set_editor_property('letter_size', letters)
    sign.set_editor_property('slant', slant)
    sign.set_editor_property('shown_when', list(shown))
    sign.set_editor_property('hidden_when', list(hidden))
    return sign


# ---------------------------------------------------------------------------
# The town gate: the checkpoint and its fight
# ---------------------------------------------------------------------------

def place_checkpoint(build, gate):
    """The sandbags (solid, always: what's left of the checkpoint after), the barrier (gone once Main 3 is done) and the
    road-side board."""
    sacks = first_model(SANDBAG_MODELS)
    count = 0
    if sacks is None:
        build.warn(f'no sandbag model ({", ".join(SANDBAG_MODELS)}): the checkpoint has no sandbags')
    else:
        # The Tripo wall is 2 m long: one to a spot (the stand-in's sacks are shorter: two overlap).
        per_wall = 1 if sacks.get_path_name().startswith(SANDBAG_MODELS[0]) else SANDBAGS_PER_WALL
        for along_in, along_side in SANDBAG_WALLS:
            for k in range(per_wall):
                offset = (k - (per_wall - 1) * 0.5) * SANDBAG_STEP
                x, y = gate.point(along_in, along_side + offset)
                build.place(sacks, (x, y, story.ground_at(x, y)), yaw_of(gate.side) + (k * 17.0 % 30.0),
                            label=f'Main3_Sandbags_{count}', folder=FOLDER, tags=('Obstacle',))
                count += 1
    company = [story.condition(after=[MAIN1])]
    bx, by = gate.point(BARRIER_IN, 0.0)
    company_sign(build, bx, by, yaw_of((-gate.inward[0], -gate.inward[1])), 'Main3_Barrier', BARRIER_WORDS, size=BARRIER_SIZE,
                 lift=BARRIER_LIFT, letters=9.0, shown=company, hidden=[story.condition(after=[MAIN3])], z=gate.z)
    sx, sy = gate.point(*SIGN_AT)
    company_sign(build, sx, sy, yaw_of((-gate.inward[0], -gate.inward[1])), 'Main3_CheckpointSign', SIGN_WORDS, size=SIGN_SIZE,
                 lift=80.0, letters=20.0, shown=company)
    build.log(f'the Company\'s checkpoint at the town gate: {count} sandbags, the barrier (until {MAIN3} is done), the board')


def place_gate_fight(build, gate):
    """The gate's fight (TownGate, seven): the checkpoint's men at their posts, then the dead drawn by the smog."""
    cls = story.actor_class('AmbushSpawner')
    # The riflemen hold their posts behind the sandbags (RIFLE_POST); the hose-man and the dead come for the player.
    plan = (('DredgerRifleman', 3, 'BASIC', False, RIFLE_POST), ('DredgerHoseMan', 1, 'BASIC', False, 0.0),
            ('UnpaidCreature', 2, 'BASIC', False, 0.0), ('UnpaidCreature', 1, 'RARE', True, 0.0))
    groups, missing = [], []
    for name, count, rank, leader, post in plan:
        creature = story.actor_class(name)
        if creature is None:
            missing.append(name)
            continue
        made = story.group(creature, count, rank, leader=leader)
        if post > 0.0:
            made.set_editor_property('post_radius', post)
        groups.append(made)
    if cls is None or not groups:
        build.warn(f'no AmbushSpawner class or no creature classes ({", ".join(missing)}): no fight at the town gate')
        return
    if missing:
        build.warn(f'{GATE}: no {", ".join(sorted(set(missing)))} class yet: those are left out')
    main2().remove_stale(build, f'Encounter_{GATE}')
    gx, gy = gate.at
    spawner = build.place(cls, (gx, gy, gate.z + 50.0), 0.0, label=f'Encounter_{GATE}', folder=FOLDER)
    spawner.set_editor_property('spawner_id', unreal.Name(GATE))
    spawner.set_editor_property('groups', groups)
    spawner.set_editor_property('max_alive', GATE_MAX_ALIVE)
    spawner.set_editor_property('creature_tags', [unreal.Name(tag) for tag in GATE_TAGS])
    spawner.set_editor_property('active_when', texts().condition(texts().ENCOUNTERS[GATE]))
    spawner.set_editor_property('activation_radius', GATE_ACTIVATION)
    # Guarding, not hunting: they hold the checkpoint until the player starts it (the dead rise as the men fall).
    spawner.set_editor_property('hunt_on_spawn', False)
    spawner.set_editor_property('entrance', unreal.AmbushEntrance.RISE)
    spawner.set_editor_property('give_up_radius', GATE_GIVE_UP)
    # The spots in the groups' order: the riflemen behind the sandbags, the hose-man at the barrier, then the dead's.
    dredgers = [gate.point(*spot) for spot in RIFLE_SPOTS] + [gate.point(*HOSEMAN_SPOT)]
    spots = [unreal.Vector(x - gx, y - gy, 0.0) for x, y in dredgers] + [unreal.Vector(x, y, 0.0) for x, y in UNPAID_SPOTS]
    spawner.set_editor_property('spawn_points', spots)
    build.log(f'the gate\'s fight ({GATE}: three riflemen, a hose-man, three Unpaid, {GATE_MAX_ALIVE} out at most) at '
              f'({gx:.0f}, {gy:.0f}), from {MAIN3}\'s road')


# ---------------------------------------------------------------------------
# Main Street: the loudspeakers, the banner, the chalk, the Gilt Spur's cart
# ---------------------------------------------------------------------------

def place_loudspeakers(build, gate):
    builder = main2()
    x, y = gate.at[0] + GATE_SPEAKER_AT[0], gate.at[1] + GATE_SPEAKER_AT[1]
    road = story.layout_entry(build, 'roads', 'farmRoad')
    toward = road['path'][-2] if road and len(road['path']) > 1 else (x - 1000.0, y - 1000.0)
    builder.place_loudspeaker(build, x, y, story.facing((x, y), toward), GATE_LOUDSPEAKER, folder=FOLDER)
    # Main Street's voice on WORLD's middle post (its horns' marker), else a post of its own there.
    best = None
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if unreal.Name(WORLD_SPEAKER_TAG) not in actor.tags:
            continue
        at = actor.get_actor_location()
        d = math.hypot(at.x - STREET_SPEAKER[0], at.y - STREET_SPEAKER[1])
        if d <= STREET_SPEAKER_REACH and (best is None or d < best[0]):
            best = (d, at)
    if best is not None:
        builder.place_loudspeaker(build, best[1].x, best[1].y, 0.0, STREET_LOUDSPEAKER, folder=FOLDER, show_post=False)
    else:
        builder.place_loudspeaker(build, STREET_SPEAKER[0], STREET_SPEAKER[1], 0.0, STREET_LOUDSPEAKER, folder=FOLDER)


def facade(x, y, z, way, reach=2500.0):
    """Where a horizontal trace from (x, y, z) along way (unit (x, y)) first meets a building: (point, normal) or None."""
    hit = trace((x, y, z), (x + way[0] * reach, y + way[1] * reach, z))
    if hit is None:
        return None
    return hit[0], hit[1]


def place_banner(build):
    cls = story.actor_class('CompanyBanner')
    if cls is None:
        build.warn('no CompanyBanner class (build the game module first): no banner over Main Street')
        return
    gz = story.ground_at(0.0, BANNER_Y)
    z = gz + BANNER_HIGH
    ends = []
    for way in ((1.0, 0.0), (-1.0, 0.0)):
        found = facade(0.0, BANNER_Y, z, way)
        ends.append(found[0] if found else unreal.Vector(way[0] * BANNER_FALLBACK, BANNER_Y, z))
    middle_x = (ends[0].x + ends[1].x) * 0.5
    # Its length across the street (its local Y along the layout's X): yaw -90 turns its local Y onto +X. It falls front
    # face up with its words' top toward its local -X: at -90 that's up the street (+Y, toward Tilly), so the player coming
    # from the gate reads it the right way up (at 90 it lay upside down to them: the play-through of 2026-11-01).
    banner = build.place(cls, (middle_x, BANNER_Y, z), -90.0, label=BANNER_TAG, folder=FOLDER, tags=(BANNER_TAG,))
    frame = banner.get_actor_transform()
    local = [frame.inverse_transform_location(end) for end in ends]
    left, right = sorted(local, key=lambda p: p.y)
    banner.set_editor_property('anchor_left', unreal.Vector(0.0, left.y, 40.0))
    banner.set_editor_property('anchor_right', unreal.Vector(0.0, right.y, 40.0))
    banner.set_editor_property('street_below', BANNER_HIGH)
    banner.set_editor_property('text', unreal.Text(BANNER_WORDS))
    banner.set_editor_property('down_event', unreal.Name(BANNER_EVENT))
    banner.set_editor_property('down_flag', unreal.Name(texts().BANNER_DOWN_FLAG))
    banner.set_editor_property('shown_when', story.condition(after=[MAIN1]))
    banner.set_editor_property('hidden_when', [story.condition(after=[MAIN6])])
    build.log(f'the banner over Main Street at ({middle_x:.0f}, {BANNER_Y:.0f}, {z:.0f}), tied {right.y - left.y:.0f} cm apart')


def place_chalk(build):
    """ASSESSED on the street's doors: a chalk ACompanySign on each listed building's street face."""
    made = 0
    for index, (key, souls) in enumerate(CHALKED):
        spot = placement(build, key)
        if spot is None:
            continue
        bx, by = spot['location'][0], spot['location'][1]
        way = (1.0 if bx > 0.0 else -1.0, 0.0)
        gz = story.ground_at(0.0, by)
        found = facade(0.0, by, gz + CHALK_HIGH, way)
        if found is None:
            continue
        point, normal = found
        out = flat((normal.x, normal.y))
        words = f'ASSESSED\n{souls} SOUL{"" if souls == 1 else "S"}\nV.C.'
        company_sign(build, point.x + out[0] * 2.0, point.y + out[1] * 2.0, yaw_of(out), f'Main3_Chalk_{key}', words,
                     look='CHALK', lift=0.0, letters=CHALK_LETTERS, slant=CHALK_SLANTS[index % len(CHALK_SLANTS)],
                     shown=[story.condition(after=[MAIN1])], z=point.z)
        made += 1
    build.log(f'ASSESSED chalked on {made} of Main Street\'s doors')


def place_saloon_cart(build):
    cls = story.actor_class('CompanyHoseCart')
    saloon = placement(build, 'saloon')
    if cls is None or saloon is None:
        build.warn('no CompanyHoseCart class or no saloon: no hose-cart at the Gilt Spur')
        return
    sx, sy = saloon['location'][0], saloon['location'][1]
    way = (1.0 if sx > 0.0 else -1.0, 0.0)
    gz = story.ground_at(0.0, sy)
    found = facade(0.0, sy, gz + 100.0, way)
    fx = found[0].x if found else sx - way[0] * 600.0
    x, y = fx - way[0] * SALOON_CART_OUT, sy + SALOON_CART_ALONG
    z = story.ground_at(x, y)
    cart = build.place(cls, (x, y, z), yaw_of(way), label=SALOON_CART, folder=FOLDER, tags=CART_TAGS + (SALOON_CART,))
    door = unreal.Vector(fx - way[0] * 8.0, sy, z + 80.0)
    cart.set_editor_property('hose_end', cart.get_actor_transform().inverse_transform_location(door))
    during = story.condition(during=MAIN3)
    cart.set_editor_property('shown_when', during)
    cart.set_editor_property('pumping_when', during)
    cart.set_editor_property('stopped_when', [story.condition(after=[MAIN3])])
    build.log(f'a hose-cart assessing the Gilt Spur at ({x:.0f}, {y:.0f}, {z:.0f}), during {MAIN3}, stopped after')


# ---------------------------------------------------------------------------
# Lucky Ned's skiff
# ---------------------------------------------------------------------------

def place_flyby(build):
    cls = story.actor_class('GangSkiffFlyBy')
    if cls is None:
        build.warn('no GangSkiffFlyBy class (build the game module first): Ned doesn\'t leave the Rest in sight')
        return
    barge = main2().find_tagged(BARGE_TAG)
    if barge is None:
        build.warn(f'no {BARGE_TAG} (build_area_landfall.py): Ned\'s skiff leaves from the Sink\'s sky instead')
        b = unreal.Vector(4600.0, 6500.0, story.ground_at(4600.0, 6500.0) + 4500.0)
    else:
        b = barge.get_actor_location()
    points = [(b.x + dx, b.y + dy, b.z + dz) for dx, dy, dz in SKIFF_OFF_BARGE]
    for x, y in (SKIFF_HOGBACK, SKIFF_OUT):
        points.append((x, y, max(b.z + 1500.0, story.ground_at(x, y, warn=False) + SKIFF_CLEAR)))
    # Never under SKIFF_CLEAR over whatever is under it (the town, the Hogback's crest).
    points = [(x, y, max(z, story.ground_at(x, y, warn=False) + SKIFF_CLEAR * 0.6)) for x, y, z in points]
    skiff = build.place(cls, points[0], 0.0, label='Main3_NedsSkiff', folder=FOLDER)
    skiff.set_editor_property('course', [unreal.Vector(*p) for p in points])
    skiff.set_editor_property('seconds', SKIFF_SECONDS)
    skiff.set_editor_property('play_when', story.condition(during=MAIN3, before_step=1))
    skiff.set_editor_property('done_flag', unreal.Name(texts().NED_LEFT_FLAG))
    radio = story.lines(build, RADIO_LINES)
    if radio is not None:
        skiff.set_editor_property('radio_lines', radio)
    after = story.lines(build, AFTER_LINES)
    if after is not None:
        skiff.set_editor_property('after_lines', after)
    build.log(f'Lucky Ned\'s skiff: off the barge at ({points[0][0]:.0f}, {points[0][1]:.0f}, {points[0][2]:.0f}), south over '
              f'the Hogback, as {MAIN3} starts (once: {texts().NED_LEFT_FLAG})')


# ---------------------------------------------------------------------------

def place(build):
    """Everything above (build_area_story.place calls this after Landfall and Main 2, before Hob)."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return {}
    gate = Gate(build)
    if gate.ok:
        place_checkpoint(build, gate)
        place_gate_fight(build, gate)
        place_loudspeakers(build, gate)
    else:
        build.warn('no town gate posts in the layout: Main 3\'s checkpoint and fight are left out')
    place_banner(build)
    place_chalk(build)
    place_saloon_cart(build)
    place_flyby(build)
    return {}
