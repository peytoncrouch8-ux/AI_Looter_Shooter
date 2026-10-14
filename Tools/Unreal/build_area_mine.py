"""The Providence silver mine on Ransom's Rest (Docs/Areas/RansomsRest_Expansion.md, section 6 "The Providence" and section
4 "Back Pay"), for build_area_mountain.py's pass: the mine's kit placed from Art/Models/Mine/Mine.layout.json in the mine's
frame, its lanterns and their few lights, the Foreman's timber props and rubble, the interior's visibility boxes and the
minimap's underground spaces. Its frame (frame()) is shared with build_area_missionprops_mountain.py and
build_area_fights_mountain.py, which stand the mine's mission props and fights from the same table.

The mine's frame: metres, x east, y north (into Larkspur High), z up; its origin the adit's mouth on its floor, design
(74, 230), +58 m: layout cm (23000, 7400, 5800). Unreal X = y * 100, Y = x * 100, Z = z * 100 from the origin; a piece
built at mine yaw 0 stands at Unreal yaw 180 (the layout's unreal_rotator gives each one), a socket's yaw_deg turns to
Unreal yaw 180 - yaw_deg, its front the rotator's forward. The origin and a turn of the whole frame come from layout.json's
gameplay.mountain.mine (origin [X, Y, Z], yaw), else a placement of kind RR_MinePortal standing at the mouth (its pivot is
the origin; its yaw less 180 the frame's turn), else the design's spot.

What it places (in the area's Mountain/Mine folder, so the whole build or build_area_mountain.py on its own replaces it):
- Every piece of the table as a static mesh actor tagged Interior_Providence (never Obstacle: the scatter and the minimap
  would read the mine's bounds as a building on the Bench over it), but the adit's own (before ADIT_SEEN_TO), which stay
  drawn from outside so the mouth shows a tunnel, and its lanterns' bodies (their lights stay the interior's), the rock pieces (adit, gallery, office, decline,
  drift, stope, manway, the fall), the timber sets and pillars, the barricade, the winch, the track and the ore cars; the
  Shell hidden in game, casting its shadow (the sun stays out while the outside is hidden). Not the six stope props (the
  Foreman's AForemanTimberProp stands at each, SOCKET_Prop_1..6, its models the kit's StopeProp and StopePropBroken; the
  static props only while that class isn't built), not the barricade's tags (an AStoryProp, gone once Back Pay's tags are
  taken), not the manway's gate (build_area_providence.py, in the gameplay pass: it opens from below in Back Pay).
- The 26 lanterns at their sockets. The stope's six and the manway's by its gate are the Foreman's (tagged StopeLantern,
  ForemanFight.h): the kit's dark lantern with its lit glass as a glow card over it (an engine cube in the lit lantern's
  LanternGlow material, its component tagged LanternGlow), so his dark puts out the glass and the light and leaves the
  iron. The rest are the lit lantern. A light (shadowless, warm, small reach) stands at a few of them only (LIGHTS: at most
  three in any view, section 6 "Performance"), the stope's tagged StopeLantern too.
- Foreman_Rubble: a target point at the fall's foot (SOCKET_Rise_3), where his shift rises.
- AInteriorVisibility at the origin: its Interior box the adit past the mouth, its Portal box the mouth, the rooms as its
  AdditionalInside; it hides Interior_Providence outside and HeavyOutside_Providence inside (build_area_mountain.py tags
  the Bench's buildings).
- Mine_MouthVeil: a black unlit card across the adit's far end, tagged MouthVeil_Providence: shown only while the view is
  outside, when the rooms past it are hidden (2026-10-14: from the Diggings' yard the mouth showed the sky and the far side
  of the mountain through the hidden rooms).
- One trigger box per room tagged Underground (no collision), past the portal's brow, for the minimap's UNDERGROUND plate.
Every missing model, socket or class is warned of and left out; nothing here fails the build.
"""
import json
import math
import os

import unreal

import build_area_story as story

HERE = os.path.dirname(os.path.abspath(__file__))
LAYOUT_FILE = os.path.join(HERE, '..', '..', 'Art', 'Models', 'Mine', 'Mine.layout.json')
FOLDER = 'Mountain/Mine'

# The tags the game's code finds things by (World/InteriorVisibility.h, Creatures/ForemanFight.h, HudMinimapWidget).
INSIDE_TAG = 'Interior_Providence'
OUTSIDE_TAG = 'HeavyOutside_Providence'
VEIL_TAG = 'MouthVeil_Providence'
UNDERGROUND_TAG = 'Underground'
LANTERN_TAG = 'StopeLantern'
GLOW_TAG = 'LanternGlow'
RUBBLE_TAG = 'Foreman_Rubble'

PORTAL_KIND = 'RR_MinePortal'
# Design (74, 230), +58 m: the adit's mouth on its floor.
DESIGN_ORIGIN = (23000.0, 7400.0, 5800.0)

ENGINE_CUBE = '/Engine/BasicShapes/Cube'
GLOW_MATERIAL = '/Game/Art/Materials/MI_LanternGlow'
# The lantern's glass (Mine.py mine_lantern: 0.12 x 0.12 x 0.17 m about its pivot); the glow card stands 2-3 mm proud of
# it, inside the corner wires, so the lit glass never fights the dark one under it (cm).
GLASS = (12.5, 12.5, 17.5)

# The pieces the mine's own scripts stand instead of the table's static model.
TIMBER_PIECE = 'RR_Mine_StopeProp'
TAGS_PIECE = 'RR_Mine_BarricadeTags'
GATE_PIECE = 'RR_Mine_ManwayGate'
SHELL_PIECE = 'RR_Mine_Shell'

# The stope's lanterns (and the manway's by its gate) are the Foreman's: his dark puts them out.
DARK_LANTERNS = {('Stope', n) for n in range(1, 7)} | {('Manway', 1)}
# Which lanterns carry a real light (the rest are glass alone): enough to see the rooms by, never more than three in a
# view. layout.json gameplay.mountain.mine.lights ({piece: [n, ...]}) overrides.
LIGHTS = {'Adit': (1, 3), 'Gallery': (2, 3, 6), 'Office': (1,), 'Decline': (2,), 'Drift': (2, 3), 'Stope': (3, 4, 6),
          'Manway': (3,)}
# A lantern's light: candelas, reach (cm), colour temperature (K), and where it stands off the glass (cm, forward and up).
# layout.json gameplay.mountain.mine.lantern ({candela, radius, temperature}) overrides.
LANTERN_LIGHT = {'candela': 8.0, 'radius': 800.0, 'temperature': 2300.0}
LIGHT_OUT = 18.0
LIGHT_UP = -4.0
LIGHT_DRAW = 4500.0

# The Foreman's timber props: the art's prop is 6.3 m from its mudsill to its wedges and 0.48 m across (the stand-in's).
PROP_HEIGHT = 630.0
PROP_WIDTH = 48.0

# The interior: the mouth's box (mine metres: min, max), where both sides show; the rooms count from where it ends.
PORTAL_BOX = ((-5.0, -6.0, -1.0), (5.0, 6.0, 6.0))
INSIDE_FROM_Y = 6.0
# The adit's pieces and lanterns before this (mine y) stay drawn from outside: the mouth shows a dark tunnel. The veil
# stands just past them, before the gallery opens (y 30), across the whole bore (the rock's noise reaches 0.42 m in).
ADIT_SEEN_TO = 29.0
VEIL_AT_Y = 29.2
VEIL_SIZE = (7.0, 0.02, 5.0)
VEIL_FROM_Z = -0.6
VEIL_MATERIAL = '/Engine/EngineDebugMaterials/BlackUnlitMaterial'
# The minimap's underground starts under the portal's brow, past the mouth (mine y).
UNDERGROUND_FROM_Y = 0.5

_LAYOUT = {}


# ---------------------------------------------------------------------------
# The frame
# ---------------------------------------------------------------------------

def read_layout():
    """Mine.layout.json, read once a session (the art agent writes it; the editor keeps modules, so a reload reads it again)."""
    if 'data' not in _LAYOUT:
        with open(LAYOUT_FILE, encoding='utf-8') as f:
            _LAYOUT['data'] = json.load(f)
    return _LAYOUT['data']


class MineFrame:
    """The mine's frame in the level: its origin (cm) and its turn (degrees about Z; 0 = the design's, mine north = +X)."""

    def __init__(self, origin, yaw=0.0):
        self.origin = tuple(float(v) for v in origin)
        self.yaw = float(yaw)
        self._c = math.cos(math.radians(self.yaw))
        self._s = math.sin(math.radians(self.yaw))

    def offset(self, dx, dy, dz=0.0):
        """An offset in the portal's Unreal axes (cm; Mine.layout.json's unreal_cm) as a world spot."""
        ox, oy, oz = self.origin
        return (ox + dx * self._c - dy * self._s, oy + dx * self._s + dy * self._c, oz + dz)

    def at(self, x, y, z=0.0):
        """A spot in the mine's metres (x east, y north, z up from the mouth's floor) as a world spot (cm)."""
        return self.offset(y * 100.0, x * 100.0, z * 100.0)

    def turn(self, yaw):
        """An Unreal yaw in the portal's frame as a world yaw."""
        return yaw + self.yaw

    def socket_yaw(self, yaw_deg):
        """A socket's yaw (the mine's, Mine.layout.json sockets) as a world yaw: its front the rotator's forward."""
        return self.turn(180.0 - yaw_deg)

    def local_box(self, lo, hi):
        """A box in the mine's metres as an actor-local box at the origin (cm, X from y, Y from x): (min, max)."""
        return ((lo[1] * 100.0, lo[0] * 100.0, lo[2] * 100.0), (hi[1] * 100.0, hi[0] * 100.0, hi[2] * 100.0))

    def polygon(self, lo, hi):
        """A box's footprint in the mine's metres as a world polygon (cm, corners in order)."""
        return [self.at(x, y)[:2] for x, y in ((lo[0], lo[1]), (hi[0], lo[1]), (hi[0], hi[1]), (lo[0], hi[1]))]


def frame(build, quiet=False):
    """The mine's frame for this level: layout.json gameplay.mountain.mine (origin, yaw), else the portal's placement, else
    the design's mouth. Warns when the layout's origin and a placed portal disagree."""
    spec = build.settings.get('mountain', {}).get('mine', {})
    portal = next((s for s in build.layout.get('placements', {}).values() if s['kind'] == PORTAL_KIND), None)
    # A placement keeps its own height only when the layout gives one ([X, Y, Z]); else it stands on the terrain, which at
    # the mouth is the notch's top over the portal, not the adit's floor.
    source = next((p for p in build.source.get('placements', []) if p['kind'] == PORTAL_KIND), None)
    portal_z = len(source['location']) > 2 if source else False
    origin, yaw = spec.get('origin'), spec.get('yaw')
    if origin is None and portal is not None:
        px, py = portal['location'][:2]
        origin = (px, py, portal['location'][2] if portal_z else DESIGN_ORIGIN[2])
        yaw = portal.get('yaw', 180.0) - 180.0
    if origin is None:
        origin = DESIGN_ORIGIN
        if not quiet:
            build.warn('no gameplay.mountain.mine.origin nor RR_MinePortal placement in the layout: the mine stands at the '
                       f'design\'s mouth {DESIGN_ORIGIN}')
    if len(origin) < 3:
        origin = (origin[0], origin[1], DESIGN_ORIGIN[2])
        if not quiet:
            build.warn(f'the mine\'s origin has no height: its floor taken as {DESIGN_ORIGIN[2] / 100.0:.0f} m')
    made = MineFrame(origin, yaw or 0.0)
    if portal is not None and not quiet:
        px, py, pz = portal['location'][:3]
        if math.hypot(px - made.origin[0], py - made.origin[1]) > 5.0 or abs(pz - made.origin[2]) > 5.0:
            build.warn(f'the RR_MinePortal placement ({px:.0f}, {py:.0f}, {pz:.0f}) isn\'t at the mine\'s origin '
                       f'{tuple(round(v) for v in made.origin)}: the portal is moved there (its pivot is the origin)')
    return made


def socket(layout, piece, name):
    """A rock piece's socket from the table: ((x, y, z) metres, yaw_deg), or None."""
    full = name if name.startswith('SOCKET_') else 'SOCKET_' + name
    for entry in layout.get('sockets', {}).get(piece, []):
        if entry['name'] == full:
            return tuple(entry['mine_m']), float(entry['yaw_deg'])
    return None


def room(layout, name):
    """A room's box from the table: ((min x, y, z), (max x, y, z)) metres, or None."""
    for entry in layout.get('rooms', []):
        if entry['name'] == name:
            return tuple(entry['box_min_m']), tuple(entry['box_max_m'])
    return None


def rooms_box(layout, names):
    """The box round some rooms (metres), or None when none is in the table."""
    boxes = [room(layout, name) for name in names]
    boxes = [b for b in boxes if b]
    if not boxes:
        return None
    return (tuple(min(b[0][i] for b in boxes) for i in range(3)), tuple(max(b[1][i] for b in boxes) for i in range(3)))


def mesh_named(meshes, asset):
    """A static mesh by its asset name (SM_...) from build_area.mesh_index()'s table, or None."""
    path = meshes.get(asset[3:] if asset.startswith('SM_') else asset)
    return unreal.load_asset(path) if path else None


def add_tags(actor, *tags):
    """Adds tags to an actor that has some already (the area's), each once."""
    have = list(actor.tags)
    for tag in tags:
        if unreal.Name(tag) not in have:
            have.append(unreal.Name(tag))
    actor.set_editor_property('tags', have)


def turned(actor, pitch, yaw, roll):
    if pitch or roll:
        actor.set_actor_rotation(unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw), False)


# The rock pieces: walked on and walled in by their own triangles (the art's note: complex as simple).
ROCK_PIECES = ('RR_Mine_Adit', 'RR_Mine_Gallery', 'RR_Mine_Office', 'RR_Mine_Decline', 'RR_Mine_Drift', 'RR_Mine_Stope',
               'RR_Mine_Manway', 'RR_Mine_Fall')


def complex_collision(mesh):
    """Whether a mesh collides with its own triangles (complex as simple); None when the editor won't say."""
    try:
        body = mesh.get_editor_property('body_setup')
        flag = body.get_editor_property('collision_trace_flag')
        return flag == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
    except Exception:  # noqa: BLE001 (an older editor's Python names it otherwise: only a check)
        return None


# ---------------------------------------------------------------------------
# The pieces
# ---------------------------------------------------------------------------

def place_pieces(build, mine, layout, meshes, timber_cls, story_cls):
    """Every piece of the table but the ones stood otherwise; returns the placed barricade (its sockets) or None."""
    counts, missing, barricade, simple = {}, set(), None, set()
    for piece in layout.get('pieces', []):
        asset = piece['asset']
        name = asset[3:]
        if name == TIMBER_PIECE and timber_cls is not None:
            continue
        if name == GATE_PIECE:
            continue
        mesh = mesh_named(meshes, asset)
        if mesh is None:
            missing.add(asset)
            continue
        counts[name] = counts.get(name, 0) + 1
        if name in ROCK_PIECES and complex_collision(mesh) is False:
            simple.add(asset)
        pitch, yaw, roll = piece['unreal_rotator_pitch_yaw_roll']
        at = mine.offset(*piece['unreal_cm'])
        label = f'Mine_{name.replace("RR_Mine_", "").replace("RR_", "")}_{counts[name]:02d}'
        if name == TAGS_PIECE:
            place_barricade_tags(build, mine, story_cls, mesh, at, yaw, label)
            continue
        seen = name == SHELL_PIECE or piece.get('mine_m', (0.0, 99.0))[1] < ADIT_SEEN_TO
        actor = build.place(mesh, at, mine.turn(yaw), label=label, folder=FOLDER, tags=() if seen else (INSIDE_TAG,))
        turned(actor, pitch, mine.turn(yaw), roll)
        if name == SHELL_PIECE:
            # Never seen and never touched, only its shadow: the sun stays out of the rooms while the ridge over them is
            # hidden from inside.
            part = actor.static_mesh_component
            part.set_editor_property('hidden_in_game', True)
            part.set_editor_property('cast_shadow', True)
            part.set_editor_property('cast_hidden_shadow', True)
            actor.set_actor_enable_collision(False)
        elif name == 'RR_Mine_Barricade':
            barricade = actor
    if missing:
        build.warn(f'the mine lacks {", ".join(sorted(missing))} (import Art/Models/Mine/Mine.py and the tramway kit): '
                   'left out')
    if simple:
        build.warn(f'{", ".join(sorted(simple))} don\'t collide as their own triangles (set Collision Complexity to Use '
                   'Complex Collision As Simple): the player and the dead would walk through the mine\'s rock')
    build.log(f'the mine: {sum(counts.values())} pieces ({", ".join(f"{n} {k}" for k, n in sorted(counts.items()))})')
    return barricade


def barricade_tags_when(build):
    """The eleven's tags hang on the barricade's nail until Back Pay's step that takes them is done, and after Back Pay
    (build_area_missionprops_mountain.py keeps the missions' ids and steps)."""
    import build_area_missionprops_mountain as props
    taken = props.step(build, props.BACK_PAY, 'barricade') + 1
    return [props.when(during=props.BACK_PAY, from_step=taken), props.when(after=[props.BACK_PAY])]


def place_barricade_tags(build, mine, story_cls, mesh, at, yaw, label):
    if story_cls is None:
        build.warn('no StoryProp class (build the game module first): the barricade\'s tags hang there for good')
        build.place(mesh, at, mine.turn(yaw), label=label, folder=FOLDER, tags=(INSIDE_TAG,))
        return
    # Not tagged Interior_Providence: the story hides it, and the interior's visibility would show it again on the way in.
    tags = build.place(story_cls, at, mine.turn(yaw), label=label, folder=FOLDER)
    tags.get_editor_property('mesh').set_static_mesh(mesh)
    tags.set_editor_property('hidden_when', barricade_tags_when(build))
    build.log(f'the eleven\'s tags on the barricade ({label}), gone once Back Pay\'s tags are taken')


def place_timber_props(build, mine, layout, cls):
    """The Foreman's six timber props on the stope's Prop sockets (the kit's prop, else his stand-in)."""
    placed = 0
    for n in range(1, 7):
        spot = socket(layout, 'Stope', f'Prop_{n}')
        if spot is None:
            build.warn(f'the mine table has no Stope SOCKET_Prop_{n}: one timber prop short')
            continue
        (x, y, z), yaw = spot
        # The build's tags replace the actor's own: TimberProp (AForemanTimberProp::PropTag, the console's) goes back on.
        prop = build.place(cls, mine.at(x, y, z), mine.socket_yaw(yaw), label=f'Mine_TimberProp_{n}', folder=FOLDER,
                           tags=(INSIDE_TAG, 'TimberProp'))
        prop.set_editor_property('height', PROP_HEIGHT)
        prop.set_editor_property('width', PROP_WIDTH)
        placed += 1
    build.log(f'the stope: {placed} timber props (AForemanTimberProp, tagged TimberProp)')


def place_rubble(build, mine, layout):
    """Where the Foreman's shift rises: the fall's foot, under its middle rise."""
    spot = socket(layout, 'Stope', 'Rise_3')
    x, y = (spot[0][0], spot[0][1]) if spot else (-17.5, 96.3)
    floor = (socket(layout, 'Stope', 'FightCenter') or ((0.0, 0.0, -8.0), 0.0))[0][2]
    build.place(unreal.TargetPoint, mine.at(x, y, floor), 0.0, label='Foreman_Rubble', folder=FOLDER, tags=(RUBBLE_TAG,))
    build.log(f'the fall\'s rubble ({RUBBLE_TAG}) at the mine\'s ({x:.1f}, {y:.1f})')


# ---------------------------------------------------------------------------
# The lanterns
# ---------------------------------------------------------------------------

def glow_material(lit_mesh):
    """The lit lantern's glass material (its LanternGlow slot), else MI_LanternGlow."""
    if lit_mesh is not None:
        for slot in lit_mesh.get_editor_property('static_materials'):
            material = slot.get_editor_property('material_interface')
            names = (str(slot.get_editor_property('material_slot_name')), material.get_name() if material else '')
            if material is not None and any('Glow' in n for n in names):
                return material
    if unreal.EditorAssetLibrary.does_asset_exist(GLOW_MATERIAL):
        return unreal.load_asset(GLOW_MATERIAL)
    return None


def lantern_light(build, at, yaw, label, tags, settings):
    """A lantern's light: shadowless, warm, a small reach, a little in front of the glass."""
    forward = unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw).get_forward_vector()
    where = (at[0] + forward.x * LIGHT_OUT, at[1] + forward.y * LIGHT_OUT, at[2] + LIGHT_UP)
    actor = build.place(unreal.PointLight, where, yaw, label=label, folder=FOLDER, tags=tags)
    light = actor.get_editor_property('point_light_component')
    # A flame: ULightFlickerSubsystem breathes it while the player is near.
    light.set_editor_property('component_tags', [unreal.Name('Flicker')])
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_editor_property('intensity_units', unreal.LightUnits.CANDELAS)
    light.set_editor_property('intensity', float(settings['candela']))
    light.set_editor_property('attenuation_radius', float(settings['radius']))
    light.set_editor_property('cast_shadows', False)
    light.set_editor_property('use_temperature', True)
    light.set_editor_property('temperature', float(settings['temperature']))
    light.set_editor_property('source_radius', 3.0)
    light.set_editor_property('max_draw_distance', LIGHT_DRAW)
    light.set_editor_property('max_distance_fade_range', 1000.0)
    return actor


def place_lanterns(build, mine, layout, meshes):
    """The 26 lanterns: the stope's dark lantern with a glow card for the Foreman's dark, the rest lit; lights at a few."""
    lit = mesh_named(meshes, 'SM_RR_Mine_Lantern')
    dark = mesh_named(meshes, 'SM_RR_Mine_LanternDark')
    cube = unreal.load_asset(ENGINE_CUBE)
    glow = glow_material(lit)
    spec = build.settings.get('mountain', {}).get('mine', {})
    lights = {piece: tuple(ns) for piece, ns in spec.get('lights', LIGHTS).items()}
    settings = dict(LANTERN_LIGHT, **spec.get('lantern', {}))
    if lit is None:
        build.warn('no SM_RR_Mine_Lantern (import Art/Models/Mine/Mine.py): the mine\'s lanterns are left out; their lights '
                   'stand alone')
    if glow is None:
        build.warn(f'no lantern glass material ({GLOW_MATERIAL}): the stope\'s lanterns have no glow cards')
    counts = {'lanterns': 0, 'lights': 0, 'stope': 0}
    for lamp in layout.get('lanterns', []):
        note = lamp.get('note', '')
        piece, _, name = note.partition(' SOCKET_Light_')
        n = int(name) if name.isdigit() else 0
        at = mine.offset(*lamp['unreal_cm'])
        pitch, yaw, roll = lamp['unreal_rotator_pitch_yaw_roll']
        yaw = mine.turn(yaw)
        stope = (piece, n) in DARK_LANTERNS
        label = f'Mine_Lantern_{piece}_{n}'
        body = dark if stope and dark is not None else lit
        if body is not None:
            # The adit's lanterns show from outside, their glass glowing down the dark tunnel; their lights don't.
            actor = build.place(body, at, yaw, label=label, folder=FOLDER, tags=() if piece == 'Adit' else (INSIDE_TAG,))
            actor.set_actor_enable_collision(False)
            actor.static_mesh_component.set_editor_property('cast_shadow', False)
            counts['lanterns'] += 1
        if stope and body is dark and dark is not None and glow is not None:
            card = build.place(cube, at, yaw, label=f'{label}_Glass', folder=FOLDER, tags=(INSIDE_TAG, LANTERN_TAG),
                               scale=tuple(v / 100.0 for v in GLASS))
            part = card.static_mesh_component
            part.set_material(0, glow)
            part.set_editor_property('cast_shadow', False)
            part.set_editor_property('component_tags', [unreal.Name(GLOW_TAG)])
            card.set_actor_enable_collision(False)
            counts['stope'] += 1
        if n in lights.get(piece, ()):
            lantern_light(build, at, yaw, f'{label}_Light', (INSIDE_TAG, LANTERN_TAG) if stope else (INSIDE_TAG,), settings)
            counts['lights'] += 1
    build.log(f'the mine\'s lanterns: {counts["lanterns"]} ({counts["stope"]} of them the Foreman\'s, tagged {LANTERN_TAG}), '
              f'{counts["lights"]} lights ({settings["candela"]:.0f} cd, {settings["radius"] / 100.0:.0f} m, shadowless)')


# ---------------------------------------------------------------------------
# The interior and the underground
# ---------------------------------------------------------------------------

def make_box(lo, hi):
    made = unreal.Box()
    made.set_editor_property('min', unreal.Vector(*lo))
    made.set_editor_property('max', unreal.Vector(*hi))
    try:
        made.set_editor_property('is_valid', True)
    except TypeError:
        made.set_editor_property('is_valid', 1)
    return made


def clipped_rooms(layout, from_y):
    """The table's rooms cut back to start at from_y (mine y), the ones left: [(name, lo, hi)]."""
    made = []
    for entry in layout.get('rooms', []):
        lo, hi = list(entry['box_min_m']), list(entry['box_max_m'])
        lo[1] = max(lo[1], from_y)
        if hi[1] - lo[1] > 0.5:
            made.append((entry['name'], tuple(lo), tuple(hi)))
    return made


def place_interior(build, mine, layout):
    """AInteriorVisibility at the origin: the adit as its Interior box, the mouth as its Portal box, every room past it."""
    cls = story.actor_class('InteriorVisibility')
    if cls is None:
        build.warn('no InteriorVisibility class (build the game module first): the mine and the Bench draw each other')
        return
    rooms = clipped_rooms(layout, INSIDE_FROM_Y)
    adit = next(((lo, hi) for name, lo, hi in rooms if name == 'adit'), None)
    if adit is None:
        build.warn('the mine table has no adit room: no interior visibility')
        return
    actor = build.place(cls, mine.origin, mine.yaw, label='InteriorVisibility_Providence', folder=FOLDER)
    actor.set_editor_property('inside_tag', unreal.Name(INSIDE_TAG))
    actor.set_editor_property('outside_tag', unreal.Name(OUTSIDE_TAG))
    actor.set_editor_property('veil_tag', unreal.Name(VEIL_TAG))
    for prop, (lo, hi) in (('interior', adit), ('portal', PORTAL_BOX)):
        (x0, y0, z0), (x1, y1, z1) = mine.local_box(lo, hi)
        box = actor.get_editor_property(prop)
        box.set_relative_location(unreal.Vector((x0 + x1) * 0.5, (y0 + y1) * 0.5, (z0 + z1) * 0.5), False, True)
        box.set_box_extent(unreal.Vector((x1 - x0) * 0.5, (y1 - y0) * 0.5, (z1 - z0) * 0.5), False)
    actor.set_editor_property('additional_inside', [make_box(*mine.local_box(lo, hi)) for name, lo, hi in rooms
                                                    if name != 'adit'])
    build.log(f'the interior\'s visibility at the mouth: the adit and {len(rooms) - 1} rooms inside, the mouth both ways '
              f'({INSIDE_TAG} hidden outside, {OUTSIDE_TAG} inside)')


def place_veil(build, mine):
    """The black card across the adit's far end, shown only while the view is outside (AInteriorVisibility's VeilTag)."""
    material = unreal.load_asset(VEIL_MATERIAL) if unreal.EditorAssetLibrary.does_asset_exist(VEIL_MATERIAL) else None
    if material is None:
        build.warn(f'no {VEIL_MATERIAL}: no veil in the adit (the mouth shows through the hidden rooms)')
        return
    width, depth, height = VEIL_SIZE
    at = mine.at(0.0, VEIL_AT_Y, VEIL_FROM_Z + height * 0.5)
    card = build.place(unreal.load_asset(ENGINE_CUBE), at, mine.turn(0.0), label='Mine_MouthVeil', folder=FOLDER,
                       tags=(VEIL_TAG,), scale=(depth, width, height))
    part = card.static_mesh_component
    part.set_material(0, material)
    part.set_editor_property('cast_shadow', False)
    card.set_actor_enable_collision(False)
    build.log(f'the veil across the adit at mine y {VEIL_AT_Y} (shown outside only)')


def place_underground(build, mine, layout):
    """A trigger box per room, tagged Underground, without collision: the minimap's plate inside them."""
    placed = 0
    for name, lo, hi in clipped_rooms(layout, UNDERGROUND_FROM_Y):
        center = mine.at((lo[0] + hi[0]) * 0.5, (lo[1] + hi[1]) * 0.5, (lo[2] + hi[2]) * 0.5)
        size = ((hi[1] - lo[1]) * 100.0, (hi[0] - lo[0]) * 100.0, (hi[2] - lo[2]) * 100.0)
        box = build.place(unreal.TriggerBox, center, mine.yaw, label=f'Underground_{name}', folder=FOLDER,
                          tags=(UNDERGROUND_TAG,), scale=tuple(s / 64.0 for s in size))
        box.set_actor_enable_collision(False)
        placed += 1
    build.log(f'the minimap\'s underground: {placed} rooms tagged {UNDERGROUND_TAG}')


# ---------------------------------------------------------------------------
# All of it
# ---------------------------------------------------------------------------

def place(build, meshes):
    """The mine, its lanterns, the Foreman's props and rubble, its visibility and underground (build_area_mountain.py's
    place() calls this). Returns its frame."""
    try:
        layout = read_layout()
    except (OSError, ValueError) as error:
        build.warn(f'no mine table ({LAYOUT_FILE}: {error}): no mine')
        return None
    mine = frame(build)
    timber_cls = story.actor_class('ForemanTimberProp')
    story_cls = story.actor_class('StoryProp')
    if timber_cls is None:
        build.warn('no ForemanTimberProp class (build the game module first): the stope\'s props stand as plain timber')
    place_pieces(build, mine, layout, meshes, timber_cls, story_cls)
    if timber_cls is not None:
        place_timber_props(build, mine, layout, timber_cls)
    place_rubble(build, mine, layout)
    place_lanterns(build, mine, layout, meshes)
    place_interior(build, mine, layout)
    place_veil(build, mine)
    place_underground(build, mine, layout)
    x, y, z = mine.origin
    build.log(f'the Providence at ({x:.0f}, {y:.0f}, {z:.0f}), turned {mine.yaw:.1f}')
    return mine
