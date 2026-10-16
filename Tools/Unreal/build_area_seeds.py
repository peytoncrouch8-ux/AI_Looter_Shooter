"""The saga's seeds on Ransom's Rest (Docs/Story.md, "Seeds in Ransom's Rest"; built 2026-10-16), for build_area.py's
gameplay pass, placed from the buildings and props build_area.py stood (their sockets and frames), so they follow the
level whenever it's rebuilt. Everything goes in the area's Gameplay/Seeds folder (a gameplay build clears Gameplay first);
a missing building, socket, model or class is warned of and left out. Only the level whose layout has Ellis's grave
(Ransom's Rest) gets any of it. The words are Tools/Unreal/story_texts.py's (READABLES, LEDGER_TOWNS), copied on by its
apply_readable(); each readable sets its story flag when read (AReadableProp::SetsFlag).

- The Ledger's six loose pages (AReadableProp LedgerPage_<Town>, SM_LedgerPage, else SM_DeaconsSlip), reading one sets
  Ledger.Page.<Town> and opens its town's bestiary page (create_bestiary_pages.py):
    TheNeedles        the sheriff's lockup: on the cell floor in front of the bunk, by the winch crank (the office's frame)
    HighwaterLanding  the depot's baggage room: on the black trunk along the partition (from SOCKET_BaggageCoffin)
    RansomsRest       the step outside Aldana's vestry door (the chapel's SOCKET_Speaker, out and to the side)
    DustwaterMesa     Calder's Post: on Ruth's table, in front of the post's log (from RR_RangerCabin's SOCKET_Journal)
    FurnaceHollow     pinned on the Providence's tag board under IN's hooks (build_area_providence.on_board)
    SaintAgnesMire    on the Mooring Ledge beside the gang's mooring post
- The Deacon's two letters (SM_DeaconLetter, else SM_DeaconsSlip): DeaconLetter_Cleft on the ledge by the mooring post
  (never sent), DeaconLetter_Saloon standing in the third slot of the saloon's post rack (SM_PostRack, a plain mesh on the
  Gilt Spur's front right of its doors, 1.4 m up; the letter on its SOCKET_Slot_3 when the rack has one).
- The Reliquary's lid (ReliquaryLid, no model: the smashed Reliquary's own lid) at the Reliquary's SOCKET_Interact, shown
  after Main 4.
- The map of the Reaches (ReachesMap, SM_ReachesMap) flat on the ticket office's back wall over the agent's desk, 1.55 m
  over SOCKET_AgentDesk.
- The second light (AFarSignalLight SecondLight, steady): from Gravewind Point's place 9 km out to the west, nudged off any
  backdrop landform in the way, 40 m over the canyon fog; shown after Main 6, only at Dusk.
- The Seven (AWantedPoster, variant Seven) on the Rangers' notice board, in the strip between Ellis's poster and Calder's
  note (its crossed faces are the C++'s CrossedFlags defaults).

Hook in build_area.py's gameplay(), after the valley's mission pieces:
    importlib.reload(importlib.import_module('build_area_seeds')).place(self)
On its own, in the open editor (clears only Gameplay/Seeds, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_seeds.py RansomsRest"
"""
import importlib
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Seeds'
PROPS = '/Game/Art/Props/'
# The props art's models (Art/Models/Props/MissionProps.py); a page or letter without its own wears the Deacon's slip.
PAGE_MODEL = 'SM_LedgerPage'
LETTER_MODEL = 'SM_DeaconLetter'
RACK_MODEL = 'SM_PostRack'
MAP_MODEL = 'SM_ReachesMap'
FALLBACK_MODEL = 'SM_DeaconsSlip'
MAIN4, MAIN6 = 'Main4', 'Main6'
DUSK = 'Dusk'
READ_REACH = 250.0
# A page pinned upright or a letter stood in a slot: the flat model (its face +Z, its long side along X, as the slip's) is
# turned nose down so its face looks out along its yaw.
UPRIGHT = -90.0
# The folded letter's long side (SM_DeaconLetter, 11 x 18 cm): half of it lifts a stood letter clear of a slot's floor.
LETTER_LENGTH = 18.0

# --- Where things lie (cm; a building's frame is Unreal's: +X its front, +Y its right) ---
# The sheriff's lockup (build_area_missionprops_valley.py's frame: the cell X 44..154, Y 80..220, the bunk along its back
# with the crank at (66, 168)): on the floor in front of the bunk, by the crank.
LOCKUP_PAGE = (118.0, 172.0)
LOCKUP_TRACE = (220.0, -50.0)
# The baggage room (DepotRR.py): the black trunk along the partition stands 2.15 m west and 0.16 m north of the coffin's
# skid in the model, which is 16 cm forward and 215 cm right of SOCKET_BaggageCoffin; its top 0.8 m over the socket.
BAGGAGE_PAGE = (16.0, 215.0)
BAGGAGE_TRACE = (120.0, -40.0)
# The vestry door (Chapel.py SOCKET_Speaker, 1.6 m over the sanctuary floor, facing out): on its step, this far out and
# to the door's right, clear of the knock (the page lies at the feet, the door's talk is at the face).
VESTRY_OUT = 45.0
VESTRY_SIDE = 40.0
# Ruth's table (CalderPost.py): in front of the post's log, short of the lamp: 36 cm forward and 22 cm right of
# SOCKET_Journal in the cabin's frame.
TABLE_PAGE = (36.0, 22.0)
# The tag board's frame (build_area_providence.on_board: metres, x along its face, y into it, z up): under IN's hooks, its
# face 2.5 cm in front of the board's middle line.
TAGBOARD_PAGE = (-0.45, 0.02, 1.05)
# The Mooring Ledge: from the post toward the ledge's middle (layout labels "Mooring Ledge"), the page and the letter a
# little apart, each on ground within LEDGE_RISE of the post's foot (else the next try round the post).
LEDGE_LABEL = 'Mooring Ledge'
LEDGE_FALLBACK = (-10700.0, -9700.0)
LEDGE_PAGE = (170.0, 70.0)          # along toward the ledge, and to its side
LEDGE_LETTER = (110.0, -90.0)
LEDGE_RISE = 120.0
# The saloon's post rack: on the Gilt Spur's front (FalseFronts.py: the facade's face 7 m out from the pivot) right of the
# doors as one faces them, between the door's casing and the right window's, under the mourning board; 1.4 m over the
# ground. Snapped onto the siding as the posters are.
RACK_AT = (700.0, -110.0, 140.0)
RACK_SLOT = 3
RACK_SLOTS = 8
# The Reaches map: 1.55 m over the agent's desk socket, on the wall behind the desk (the back wall, 68 cm behind the
# socket in DepotRR.py), found by a line back from there.
MAP_UP = 155.0
MAP_REACH = 300.0
MAP_TRACE = 150.0
# The second light: west (bearing 270: 0 north, 90 east) from Gravewind Point's place, 9 km out, 40 m over the canyon fog
# (its banks' tops about 10 m under the deck's boards: build_area_deck.FOGS); nudged off the backdrop's landforms (layout
# region.backdrop: nearer layers only) by up to LIGHT_NUDGE degrees.
LIGHT_BEARING = 270.0
LIGHT_OUT = 900000.0
LIGHT_OVER_FOG = 4000.0
FOG_UNDER_BOARDS = 1000.0
LIGHT_NUDGE = 25.0
LIGHT_CLEAR = 1.0
LIGHT_SIZE = 5500.0
LIGHT_STRENGTH = 30.0
# A pale, cold white: not a saint's orange, nor the sunset's.
LIGHT_COLOR = (0.88, 0.94, 1.0)
# The Seven on the Rangers' board (layout gameplay.posters: Ellis's poster on SOCKET_Decal at +4..+54, Calder's note at
# -52..-27 along it): in the strip between, else under the note.
SEVEN_OFFSET = (0.0, -12.0, 0.0)
SEVEN_FALLBACK = (0.0, -40.0, -30.0)
SEVEN_STRIP = (-27.0, 4.0)
SEVEN_WIDTH = 25.0

_MESHES = {}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def valley():
    """The valley's helpers (level_actors, socket, onto: they read a station's building component too)."""
    return importlib.reload(importlib.import_module('build_area_missionprops_valley'))


def texts():
    return importlib.reload(importlib.import_module('story_texts'))


def find_mesh(name):
    """A static mesh by asset name: /Game/Art/Props first, else anywhere under /Game/Art; None when not imported."""
    path = PROPS + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    if not _MESHES:
        _MESHES.update(valley().mesh_index())
    found = _MESHES.get(name[3:] if name.startswith('SM_') else name)
    return unreal.load_asset(found) if found else None


def model_for(build, name, what):
    """The model, else the Deacon's slip (warned), else None (warned)."""
    model = find_mesh(name)
    if model is None:
        model = find_mesh(FALLBACK_MODEL)
        build.warn(f'no {name} yet (the props art\'s): {what} wears {FALLBACK_MODEL if model else "nothing"} until it\'s '
                   f'imported and this runs again')
    return model


def flat_axes(actor_or_rotator):
    """An actor's (or a rotator's) forward and right, level."""
    rot = actor_or_rotator if isinstance(actor_or_rotator, unreal.Rotator) else actor_or_rotator.get_actor_rotation()
    yaw = math.radians(rot.yaw)
    return unreal.Vector(math.cos(yaw), math.sin(yaw), 0.0), unreal.Vector(-math.sin(yaw), math.cos(yaw), 0.0)


def surface(host, spot, above, below):
    """The host's own surface under spot (from above to below it), else the ground there, else spot."""
    hit = story.onto_host(host, unreal.Vector(spot.x, spot.y, spot.z + above), unreal.Vector(spot.x, spot.y, spot.z + below)) \
        if host is not None else None
    if hit is not None:
        return hit
    z = story.ground(spot.x, spot.y, spot.z)
    return unreal.Vector(spot.x, spot.y, z)


def readable(build, readable_id, at, yaw, model=None, pitch=0.0, reach=READ_REACH, label=None):
    """An AReadableProp with its words (story_texts.READABLES, its flag among them), its model and its turn."""
    cls = story.actor_class('ReadableProp')
    if cls is None:
        build.warn('no ReadableProp class (build the game module first): no ' + readable_id)
        return None
    words = texts()
    if readable_id not in words.READABLES:
        build.warn(f'no words for {readable_id} in story_texts.py: left out')
        return None
    prop = build.place(cls, (at.x, at.y, at.z), yaw, label=label or f'Readable_{readable_id}', folder=FOLDER,
                       tags=('Readable', f'Readable_{readable_id}'))
    if pitch:
        prop.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw), False)
    words.apply_readable(prop, readable_id)
    prop.set_editor_property('reach', reach)
    if model is not None:
        prop.get_editor_property('mesh').set_static_mesh(model)
    build.log(f'{readable_id} at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')
    return prop


def page(build, town, at, yaw, pitch=0.0):
    return readable(build, 'LedgerPage_' + town, at, yaw, model_for(build, PAGE_MODEL, f'the {town} page'), pitch)


def bounds_of(mesh):
    """A static mesh's box in its own frame: (min, max) Vectors, or None."""
    try:
        box = mesh.get_bounding_box()
        return box.get_editor_property('min'), box.get_editor_property('max')
    except Exception:
        pass
    try:
        bounds = mesh.get_bounds()
        origin, extent = bounds.get_editor_property('origin'), bounds.get_editor_property('box_extent')
        return origin - extent, origin + extent
    except Exception:
        return None


def on_wall(mesh, hit, yaw):
    """Where a wall piece's pivot goes so its back lies on the wall at hit and its middle is there (its front +X out of
    the wall, as every model's front): the art's pivot, wherever it is, doesn't matter."""
    box = bounds_of(mesh) if mesh is not None else None
    if box is None:
        return hit
    low, high = box
    forward, right = flat_axes(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    back, side, up = -low.x, -(low.y + high.y) * 0.5, -(low.z + high.z) * 0.5
    return hit + forward * back + right * side + unreal.Vector(0.0, 0.0, up)


# ---------------------------------------------------------------------------
# The pages
# ---------------------------------------------------------------------------

def place_lockup_page(build, kit, found):
    office = found.get('sheriff')
    if office is None:
        build.warn('no sheriff\'s office placed: no page in the lockup (TheNeedles)')
        return
    frame = office.get_actor_transform()
    base = office.get_actor_location().z
    at = frame.transform_location(unreal.Vector(LOCKUP_PAGE[0], LOCKUP_PAGE[1], 0.0))
    floor = kit.onto(office, at.x, at.y, base + LOCKUP_TRACE[0], base + LOCKUP_TRACE[1])
    if floor is None:
        build.warn('nothing of the sheriff\'s office under the lockup\'s page: it lies at the office\'s floor height')
        floor = unreal.Vector(at.x, at.y, base + 40.0)
    page(build, 'TheNeedles', floor, office.get_actor_rotation().yaw + 35.0)


def place_baggage_page(build, kit, found):
    station = found.get('depot')
    rest = kit.socket(station, 'BaggageCoffin') if station is not None else None
    if rest is None:
        build.warn('no depot with its SOCKET_BaggageCoffin: no page in the baggage room (HighwaterLanding)')
        return
    forward, right = flat_axes(station)
    spot = rest.translation + forward * BAGGAGE_PAGE[0] + right * BAGGAGE_PAGE[1]
    top = kit.onto(station, spot.x, spot.y, rest.translation.z + BAGGAGE_TRACE[0], rest.translation.z + BAGGAGE_TRACE[1])
    if top is None:
        build.warn('nothing of the depot under the baggage room\'s page: it lies at the coffin\'s height')
        top = unreal.Vector(spot.x, spot.y, rest.translation.z)
    page(build, 'HighwaterLanding', top, station.get_actor_rotation().yaw - 20.0)


def place_vestry_page(build, kit, found):
    chapel = found.get('chapel')
    door = kit.socket(chapel, 'Speaker') if chapel is not None else None
    if door is None:
        build.warn('no chapel with its SOCKET_Speaker: no page on the vestry step (RansomsRest)')
        return
    turn = door.rotation.rotator()
    out, side = flat_axes(turn)
    spot = door.translation + out * VESTRY_OUT + side * VESTRY_SIDE
    step = surface(chapel, spot, -20.0, -260.0)
    page(build, 'RansomsRest', step, turn.yaw + 150.0)


def place_table_page(build):
    mountain = importlib.reload(importlib.import_module('build_area_mountain'))
    if not mountain.is_mountain(build):
        return
    cabins = mountain.placed_all(build, ('RR_RangerCabin',))
    cabin = cabins[0] if cabins else None
    journal = story.socket(cabin, 'Journal') if cabin is not None else None
    if journal is None:
        build.warn('no Ranger cabin with its SOCKET_Journal: no page at Calder\'s Post (DustwaterMesa)')
        return
    forward, right = flat_axes(cabin)
    spot = journal.translation + forward * TABLE_PAGE[0] + right * TABLE_PAGE[1]
    top = surface(cabin, spot, 30.0, -30.0)
    page(build, 'DustwaterMesa', top, cabin.get_actor_rotation().yaw + 80.0)


def place_tagboard_page(build):
    mountain = importlib.reload(importlib.import_module('build_area_mountain'))
    if not mountain.is_mountain(build):
        return
    mine_kit = importlib.reload(importlib.import_module('build_area_mine'))
    providence = importlib.reload(importlib.import_module('build_area_providence'))
    mine = mine_kit.frame(build, quiet=True)
    if mine is None:
        build.warn('no mine frame: no page on the tag board (FurnaceHollow)')
        return
    x, y, z = providence.on_board(mine, *TAGBOARD_PAGE)
    page(build, 'FurnaceHollow', unreal.Vector(x, y, z), providence.board_facing(mine), pitch=UPRIGHT)


def ledge_middle(build):
    """The Mooring Ledge's label in the layout (anywhere under a "labels" block), else the design's spot."""
    def search(node):
        if isinstance(node, dict):
            labels = node.get('labels')
            if isinstance(labels, dict) and LEDGE_LABEL in labels:
                return labels[LEDGE_LABEL]
            for value in node.values():
                hit = search(value)
                if hit:
                    return hit
        return None
    return search(build.source) or LEDGE_FALLBACK


def on_ledge(build, post_at, toward, along, aside):
    """A spot on the ledge near the post: along toward the ledge's middle and aside of it, on ground within LEDGE_RISE of
    the post's foot; tried round the post (the line turned, then shortened) until one holds. None: nowhere."""
    for turn in (0.0, 30.0, -30.0, 60.0, -60.0, 90.0, -90.0):
        for share in (1.0, 0.6):
            a = math.radians(turn)
            fx, fy = toward[0] * math.cos(a) - toward[1] * math.sin(a), toward[0] * math.sin(a) + toward[1] * math.cos(a)
            x = post_at.x + (fx * along - fy * aside) * share
            y = post_at.y + (fy * along + fx * aside) * share
            z = story.ground(x, y, post_at.z)
            if abs(z - post_at.z) <= LEDGE_RISE:
                return unreal.Vector(x, y, z)
    return None


def place_cleft(build, found):
    """The Saint Agnes Mire page and the Deacon's letter never sent, on the Mooring Ledge by the gang's post."""
    post = found.get('mooringPost')
    if post is None:
        build.warn('no mooringPost placed: no page or letter on the Mooring Ledge')
        return
    at = post.get_actor_location()
    lx, ly = ledge_middle(build)
    dx, dy = lx - at.x, ly - at.y
    length = math.hypot(dx, dy) or 1.0
    toward = (dx / length, dy / length)
    yaw = math.degrees(math.atan2(toward[1], toward[0]))
    spot = on_ledge(build, at, toward, *LEDGE_PAGE)
    if spot is not None:
        page(build, 'SaintAgnesMire', spot, yaw + 110.0)
    else:
        build.warn('no ground on the Mooring Ledge round the post: no SaintAgnesMire page')
    spot = on_ledge(build, at, toward, *LEDGE_LETTER)
    if spot is not None:
        readable(build, 'DeaconLetter_Cleft', spot, yaw - 40.0, model_for(build, LETTER_MODEL, 'the letter never sent'))
    else:
        build.warn('no ground on the Mooring Ledge round the post: no DeaconLetter_Cleft')


# ---------------------------------------------------------------------------
# The saloon's post rack, the Reliquary's lid, the Reaches map
# ---------------------------------------------------------------------------

def place_post_rack(build, found):
    saloon = found.get('saloon')
    if saloon is None:
        build.warn('no saloon placed: no post rack, no DeaconLetter_Saloon')
        return
    posters = importlib.reload(importlib.import_module('build_area_posters'))
    hit, yaw = posters.on_face(build, saloon, dict(id='PostRack', host='saloon', at=list(RACK_AT), yaw=0.0))
    rack_mesh = find_mesh(RACK_MODEL)
    forward, right = flat_axes(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    letter_model = model_for(build, LETTER_MODEL, 'the letter in the post rack')
    if rack_mesh is None:
        build.warn(f'no {RACK_MODEL} yet (the props art\'s): the saloon\'s letter is pinned to the siding alone')
        readable(build, 'DeaconLetter_Saloon', hit + forward * 1.0, yaw, letter_model, pitch=UPRIGHT)
        return
    at = on_wall(rack_mesh, hit, yaw)
    rack = build.place(rack_mesh, (at.x, at.y, at.z), yaw, label='PostRack_Saloon', folder=FOLDER, tags=('PostRack',))
    # Its third slot: the rack's own socket when it has one (the letter turned as the socket is), else its share of the
    # rack's width from the reader's left (+Y), standing up out of the top.
    component = rack.static_mesh_component
    for name in (f'Slot_{RACK_SLOT}', f'Slot{RACK_SLOT}', 'Letter'):
        if component.does_socket_exist(name):
            slot = component.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)
            lean = slot.rotation.rotator()
            # The socket is where a standing letter's bottom middle rests, its +Z up the slot (leaning back a little). The
            # model lies flat with its pivot at the middle of its footprint, so it stands nose down in its own frame first
            # (UPRIGHT), then takes the slot's lean, and rises half its length so it doesn't sink into the shelf.
            up = unreal.MathLibrary.get_up_vector(lean)
            spot = slot.translation + up * (LETTER_LENGTH * 0.5)
            letter = readable(build, 'DeaconLetter_Saloon', spot, lean.yaw, letter_model)
            if letter is not None:
                stood = unreal.MathLibrary.compose_rotators(unreal.Rotator(roll=0.0, pitch=UPRIGHT, yaw=0.0), lean)
                letter.set_actor_rotation(stood, False)
            build.log(f'the post rack on the Gilt Spur, the letter on its SOCKET_{name}')
            return
    # on_wall put the rack's back on the siding at the hit, its middle there: the slot is half its depth out, its share
    # of the width across, the letter's middle a little under the rack's top (it stands up out of the slot).
    box = bounds_of(rack_mesh)
    width = (box[1].y - box[0].y) if box else 60.0
    depth = (box[1].x - box[0].x) if box else 8.0
    height = (box[1].z - box[0].z) if box else 40.0
    across = width * (0.5 - (RACK_SLOT - 0.5) / RACK_SLOTS)
    spot = hit + forward * (depth * 0.5) + right * across + unreal.Vector(0.0, 0.0, height * 0.5 - 2.0)
    readable(build, 'DeaconLetter_Saloon', spot, yaw, letter_model, pitch=UPRIGHT)
    build.warn(f'{RACK_MODEL} has no SOCKET_Slot_{RACK_SLOT}: the letter stands where slot {RACK_SLOT} of {RACK_SLOTS} '
               f'should be (look at it on a tour)')


def place_reliquary_lid(build):
    level = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    reliquary = next((a for a in level if unreal.Name('Reliquary_Chapel') in a.tags and unreal.Name(build.tag) in a.tags),
                     None)
    if reliquary is None:
        build.warn('no Reliquary (Reliquary_Chapel) placed: no ReliquaryLid')
        return
    mesh = None
    try:
        mesh = reliquary.get_editor_property('mesh')
    except Exception:
        pass
    spot = None
    if mesh is not None and mesh.does_socket_exist('Interact'):
        spot = mesh.get_socket_transform('Interact', unreal.RelativeTransformSpace.RTS_WORLD).translation
    else:
        build.warn('the Reliquary\'s model has no SOCKET_Interact: its lid is looked at over its middle')
        spot = reliquary.get_actor_location() + unreal.Vector(0.0, 0.0, 90.0)
    readable(build, 'ReliquaryLid', spot, reliquary.get_actor_rotation().yaw)


def place_reaches_map(build, kit, found):
    station = found.get('depot')
    desk = kit.socket(station, 'AgentDesk') if station is not None else None
    if desk is None:
        build.warn('no depot with its SOCKET_AgentDesk: no map of the Reaches')
        return
    forward, _ = flat_axes(station)
    start = desk.translation + unreal.Vector(0.0, 0.0, MAP_UP)
    wall = story.onto_host(station, start, start - forward * MAP_TRACE)
    if wall is None:
        build.warn('no wall behind the agent\'s desk: the map hangs where the wall should be')
        wall = start - forward * 68.0
    yaw = station.get_actor_rotation().yaw
    model = find_mesh(MAP_MODEL)
    if model is None:
        build.warn(f'no {MAP_MODEL} yet (the props art\'s): the map is read off the bare wall until it\'s imported')
    at = on_wall(model, wall, yaw)
    readable(build, 'ReachesMap', at, yaw, model, reach=MAP_REACH)


# ---------------------------------------------------------------------------
# The second light
# ---------------------------------------------------------------------------

def landforms(build, nearer_than):
    """The backdrop's landforms in layers nearer than nearer_than (cm): (x, y, half width in degrees as seen from the
    level's middle, distance)."""
    found = []
    for layer in build.source.get('region', {}).get('backdrop', {}).get('layers', []):
        distance = layer.get('distance', 0.0)
        if distance >= nearer_than:
            continue
        for entry in layer.get('mesas', []):
            azimuth, width = (entry['azimuth'], entry['width']) if isinstance(entry, dict) else (entry[0], entry[1])
            a = math.radians(azimuth)
            found.append((math.cos(a) * distance, math.sin(a) * distance, width * 0.5, distance))
    return found


def clear_bearing(build, x, y, wanted):
    """The bearing nearest wanted (degrees) from (x, y) that no nearer landform stands in, within LIGHT_NUDGE."""
    forms = landforms(build, LIGHT_OUT)
    blocked = []
    for mx, my, half, distance in forms:
        seen = math.degrees(math.atan2(my - y, mx - x))
        away = math.hypot(mx - x, my - y) or 1.0
        blocked.append((seen, half * distance / away + LIGHT_CLEAR))
    for step in range(int(LIGHT_NUDGE * 2) + 1):
        for sign in ((1,) if step == 0 else (-1, 1)):
            bearing = wanted + sign * step * 0.5
            if all(abs((bearing - seen + 180.0) % 360.0 - 180.0) > half for seen, half in blocked):
                return bearing
    build.warn(f'the backdrop stands in every bearing within {LIGHT_NUDGE} degrees of {wanted}: the light stays due west')
    return wanted


def place_second_light(build):
    cls = story.actor_class('FarSignalLight')
    level = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    point = next((a for a in level if unreal.Name('Place_GravewindPoint') in a.tags and unreal.Name(build.tag) in a.tags),
                 None)
    if cls is None or point is None:
        build.warn('no FarSignalLight class (build the game module first): no second light' if cls is None
                   else 'no Place_GravewindPoint (build_area_deck.py places it): no second light')
        return
    at = point.get_actor_location()
    deck = story.placed(build, 'BurialDeck')
    boards = deck.get_actor_location().z + 40.0 if deck is not None else at.z - 90.0
    bearing = clear_bearing(build, at.x, at.y, LIGHT_BEARING)
    b = math.radians(bearing)
    z = boards - FOG_UNDER_BOARDS + LIGHT_OVER_FOG
    where = (at.x + math.cos(b) * LIGHT_OUT, at.y + math.sin(b) * LIGHT_OUT, z)
    light = build.place(cls, where, 0.0, label='SecondLight', folder=FOLDER, tags=('SecondLight',))
    # Its constructor shows it as The Last Watch's far light: shown after Main 6 instead, only at dusk, steady.
    for name, value in (('shown_when', [story.condition(after=[MAIN6])]), ('shown_state', unreal.Name(DUSK)),
                        ('steady', True), ('breath_hz', 0.5), ('breath_depth', 0.08), ('size', LIGHT_SIZE),
                        ('strength', LIGHT_STRENGTH), ('color', unreal.LinearColor(*LIGHT_COLOR, 1.0))):
        try:
            light.set_editor_property(name, value)
        except Exception as error:
            build.warn(f'SecondLight: no {name} ({error}): build the game module (FarSignalLight\'s steady mode)')
    build.log(f'the second light at bearing {bearing:.1f} from Gravewind Point, {LIGHT_OUT / 100000.0:.0f} km out, '
              f'{z / 100.0:.0f} m up; after {MAIN6}, at {DUSK} (Looter.FarLight 1 shows it on a tour)')


# ---------------------------------------------------------------------------
# The Seven
# ---------------------------------------------------------------------------

def place_seven(build, found):
    cls = story.actor_class('WantedPoster')
    board = found.get('noticeBoard')
    if cls is None or board is None:
        build.warn('no WantedPoster class: no Seven' if cls is None else 'no noticeBoard placed: no Seven')
        return
    half = SEVEN_WIDTH * 0.5
    offset = SEVEN_OFFSET if SEVEN_STRIP[0] <= SEVEN_OFFSET[1] - half and SEVEN_OFFSET[1] + half <= SEVEN_STRIP[1] \
        else SEVEN_FALLBACK
    posters = importlib.reload(importlib.import_module('build_area_posters'))
    where = posters.on_socket(build, board, dict(id='posterSeven', host='noticeBoard', socket='Decal', offset=list(offset)))
    if where is None:
        return
    location, yaw = where
    poster = build.place(cls, (location.x, location.y, location.z), yaw, label='posterSeven', folder=FOLDER,
                         tags=('Obstacle', 'SevenPoster'))
    variant = getattr(unreal.WantedPosterVariant, 'SEVEN', None)
    if variant is None:
        build.warn('no WantedPosterVariant.SEVEN (build the game module first): the Seven stands as a plain poster')
    else:
        poster.set_editor_property('variant', variant)
    build.log(f'the Seven on the Rangers\' board at offset {list(offset)}')


# ---------------------------------------------------------------------------
# All of it
# ---------------------------------------------------------------------------

def place(build):
    """Everything above (build_area.py's gameplay() calls this after the valley's mission pieces). Ransom's Rest only."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    kit = valley()
    found = kit.level_actors(build)
    place_lockup_page(build, kit, found)
    place_baggage_page(build, kit, found)
    place_vestry_page(build, kit, found)
    place_table_page(build)
    place_tagboard_page(build)
    place_cleft(build, found)
    place_post_rack(build, found)
    place_reliquary_lid(build)
    place_reaches_map(build, kit, found)
    place_second_light(build)
    place_seven(build, found)
    build.log('the saga\'s seeds placed')


def run(name):
    """Places Gameplay/Seeds again on its own, saves."""
    import build_area
    build = importlib.reload(build_area).AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the seeds placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_seeds.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
