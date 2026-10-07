"""Main 5, "The Keeper's Lantern" (Docs/Areas/RansomsRest.md), for build_area_story.py: the Sink's floor dressed and the
story's pieces in it, placed from the layout's zone sink (its middle and outline), its obstacles (sinkFloorBlocks,
webwood, sinkRoadDeadTree), the computed ramp sinkRamp and Den Rock as build_area.py placed it, with the floor and the
walls found by traces at build time, so everything sits on what's really there. build_area_story's place() calls place()
here (reloaded each run, as the editor keeps modules between runs) and gets Hob's Main 5 perches back for his. It uses
build_area_story's helpers. Everything goes in the area's Gameplay folder; a piece whose model or class isn't there yet
is left out with a warning.

Everything in the Sink is placed in the Sink's own frame: centimetres from its middle (the zone's centroid), dx north
(+X) and dy east (+Y), heights over the floor under its middle, bearings as Unreal yaws (0 north, 90 east). The tables
below hold every number; Main changes them there after looking in the editor.

- The floor's collapsed blocks and old coffins (SM_DressedBlock_A/_B, SM_Coffin_Closed/_Broken, tagged Obstacle): three
  piles inside the obstacle sinkFloorBlocks (the crescent on the north side of the floor, clear of the ramp's foot),
  cover for the fight, and coffins lying loose about the floor. Each pile stands on the floor found under its middle.
- The web cards (Art/Models/Props/Sink.py's notes; one slot, no collision, no Nanite, no LODs, shadows OFF on every one,
  about 20 in any view): Web_Corner along the walls' feet (its pivot on the foot, its mat out over the floor), Web_Drape
  over the long face of a block or a coffin (scaled 1.1 over a B, 1.4 over an A, 0.88 over a coffin), Web_Ground on open
  floor, an orb web against the north wall, and Web_Tatters and Web_Strands hanging from the ramp's inner edge where it
  stands high over the floor. Web_Funnel, the den's silk lining, on Den Rock's SOCKET_DenMouth with its rotation (left
  out without the socket).
- The three egg sacs (AEggSac tagged EggSac, Main 5's second step): SM_EggSac_A and _C hanging by SOCKET_Silk from two
  Web_Lines each, glued to the wall either side; SM_EggSac_B in its Web_Sling against the north-west wall (one
  transform: the sling's SOCKET_Sac, the wall 66 cm behind). Shootable during Main 5's second step only; burst on the
  floor from the start after it. Their spiders (two each, Basic, tagged Spider_EggSac) fight on the floor.
- The Keeper's Lantern (AKeepersLantern tagged Lantern_Keeper): Web_Snare at the north wall's foot, the lantern hanging
  dark from its SOCKET_Lantern by its SOCKET_Grip, taken in Main 5's third step.
- The places' markers: the floor's middle (a target point tagged Place_SinkFloor: "Climb down into the Sink", within
  9 m, height counted) and the ramp head on the west rim by the fence's gap (Place_SinkRim: "Climb out", within 6 m,
  height counted).
- The floor's spiders (AEncounterSpawner SinkFloor, tagged Spider_SinkFloor): four of the area's ranks standing among
  the blocks, on during Main 5 until the lantern is taken (its steps 1-3), appearing as the player comes to the rim,
  hunting only on the floor (the zone sink, and no more than 2.5 m over the floor: they give up at the ramp's foot;
  nothing lives on the rim).
- The Webwood's dead, webbed trees north of the Sink and the Sink road's one (SM_DeadTree_A, tagged Obstacle, some
  wearing Web_Crown with the tree's own transform): nothing else places them (the scatter keeps dead trees to its forest
  mix), so they come with the Sink.
Hob's perches (returned to build_area_story.place_hob): on Den Rock over the den for the way down, on the north pile's
top block for the sacs and the lantern, on the Sink road's dead tree by the ramp head for the way out and after Main 5.
"""
import math

import unreal

import build_area_story as story

# The story's ids and tags, as the C++ (AEggSac, AKeepersLantern) and the mission asset
# (Tools/Unreal/create_mission_assets.py, DA_Mission_Main5) name them.
MAIN5 = 'Main5'
PLACE_FLOOR = 'Place_SinkFloor'
PLACE_RIM = 'Place_SinkRim'
FLOOR = 'SinkFloor'
FLOOR_TAG = 'Spider_SinkFloor'
SAC_TAG = 'EggSac'
SAC_SPIDER_TAG = 'Spider_EggSac'
LANTERN_TAG = 'Lantern_Keeper'
# Main 5's steps, from 0: down, the sacs, the lantern, out.
SAC_STEP = 1
LANTERN_STEP = 2
OUT_STEP = 3

PROPS = '/Game/Art/Props/'
DEAD_TREE = '/Game/Art/Vegetation/SM_DeadTree_A'

# ---------------------------------------------------------------------------
# The tables (the Sink's frame: cm from its middle, dx north, dy east; z over the floor; degrees)
# ---------------------------------------------------------------------------

# The floor's piles, inside sinkFloorBlocks: (name, (dx, dy) of its middle, pieces). A piece: (model, px, py, z, yaw, pitch,
# roll), offsets from the pile's middle in the world's axes, z over the floor there (a B's top is 52 over its foot, an A's
# 67: their pivots sit 3 cm down in the ground). A tipped piece sinks one edge a little into the floor.
PILES = (
    ('West', (650.0, -750.0), (
        ('DressedBlock_B', 0.0, 0.0, 0.0, 20.0, 0.0, 0.0),
        ('DressedBlock_A', 10.0, -15.0, 52.0, 72.0, 0.0, 4.0),
        ('DressedBlock_A', -60.0, 115.0, 0.0, -18.0, 0.0, 14.0),
        ('Coffin_Broken', 150.0, -120.0, 0.0, 35.0, 0.0, 0.0),
    )),
    ('North', (850.0, -250.0), (
        ('DressedBlock_B', 0.0, -45.0, 0.0, -8.0, 0.0, 0.0),
        ('DressedBlock_B', 82.0, -35.0, 0.0, -4.0, 0.0, 0.0),
        ('DressedBlock_A', 40.0, -40.0, 52.0, 25.0, 0.0, 3.0),
        ('Coffin_Closed', -120.0, 110.0, 0.0, 80.0, 0.0, 0.0),
    )),
    ('East', (900.0, 400.0), (
        ('DressedBlock_A', 0.0, 0.0, 0.0, 50.0, 0.0, 0.0),
        ('DressedBlock_B', 75.0, -20.0, 0.0, 140.0, 0.0, -10.0),
        ('Coffin_Closed', -95.0, -95.0, 0.0, 140.0, 0.0, 0.0),
    )),
)
# Old coffins lying loose on the floor, clear of the ramp's foot (frame (-133, 728)): (model, dx, dy, yaw).
COFFINS = (
    ('Coffin_Broken', -250.0, -450.0, 75.0),
    ('Coffin_Closed', 350.0, 900.0, -20.0),
    ('Coffin_Closed', 1350.0, -950.0, 10.0),
)
# Hob's pile and piece for the sacs and the lantern (on top of it).
HOB_PILE = ('North', 2)
# The models' long faces are their X sides (the blocks' and coffins' lengths run along their Y): half their depth (cm),
# and a drape's Z scale over them (built for a 0.5 m edge) and its length's (built 1.8 m long, along its Y).
DEPTH = {'DressedBlock_A': 37.5, 'DressedBlock_B': 40.0, 'Coffin_Closed': 31.0, 'Coffin_Broken': 31.0}
DRAPE_FIT = {'DressedBlock_A': (1.4, 0.67), 'DressedBlock_B': (1.1, 0.94), 'Coffin_Closed': (0.88, 1.0)}
# Drapes over a floor-standing piece's long face: (pile, piece index, face +1 its +X side or -1 its -X side).
DRAPES = (('West', 0, 1), ('North', 1, 1), ('East', 2, 1))

# The three egg sacs. Hanging (A, C): its silk at dist out from the middle along its bearing, silk_up over the floor, two
# Web_Lines from the wall at the anchors' bearings, anchor_up over the floor; its mouth turned to the middle. Slung (B): its
# pivot (its bottom) pivot_up over the floor against the wall at its bearing; it lands burst_out in front of the wall.
SACS = (
    dict(model='EggSac_B', bearing=-75.0, kind='sling', pivot_up=280.0, burst_out=40.0),
    dict(model='EggSac_A', bearing=-40.0, kind='hang', dist=1350.0, silk_up=520.0, anchors=(-58.0, -22.0), anchor_up=850.0),
    dict(model='EggSac_C', bearing=32.0, kind='hang', dist=1300.0, silk_up=560.0, anchors=(16.0, 48.0), anchor_up=850.0),
)
# The sling's wall: the plane y = 0.66 m behind its pivot (Sink.py), so the pivot stands this far out of the wall (cm).
SLING_OFF_WALL = 66.0

# The Keeper's Lantern: the snare's middle out from the north wall's foot along its bearing, and over the floor (cm). Its
# second card crosses the first at 55 degrees and reaches about 1 m back: to the wall.
LANTERN = dict(bearing=-8.0, off_wall=100.0, middle_up=175.0)

# Web_Corner along the walls' feet (bearings: the north-west and north walls, and the ramp's inner cliff under its head).
CORNERS = (-90.0, -55.0, -8.0, 40.0)
# A corner's pivot stands this far out of the wall's face, over its rough foot (cm).
CORNER_OUT = 8.0
# Web_Ground on open floor: (dx, dy, yaw).
GROUND_WEBS = ((150.0, 120.0, 30.0), (500.0, 300.0, 75.0))
# An orb web against the north wall: its hub's bearing, how far out of the wall and how high (cm).
ORB = dict(bearing=20.0, off_wall=120.0, hub_up=230.0)
# Web_Tatters and Web_Strands hanging from the ramp's inner edge: the computed ramp's points by index (0 at its head on
# the rim, 24 at its foot), where it stands 4-7 m over the floor.
TATTERS = (12, 16)
STRANDS = (9, 14)
# They hang this far out of the ramp's inner face (cm).
EDGE_OUT = 12.0
# Den Rock's den mouth, where the funnel's pivot goes (Outcrops.py; Sink.py fits Web_Funnel to it).
DEN_MOUTH = 'DenMouth'

# The floor's spiders: four of the area's ranks, their spots round the Sink's middle among the piles (z ignored), and how
# far over their spawner a player is still on their ground (cm): the ramp's first 10 m or so.
FLOOR_COUNT = 4
FLOOR_SPOTS = ((350.0, -450.0), (1150.0, -450.0), (550.0, 100.0), (1300.0, 300.0))
FLOOR_RISE = 250.0

# The Webwood's dead trees (frame offsets, inside the obstacle webwood north of the Sink), each with a yaw and a uniform
# scale, and whether it wears Web_Crown; then the Sink road's one (in the middle of sinkRoadDeadTree).
WEBWOOD = (
    ((2200.0, -1900.0), 15.0, 1.0, True),
    ((2450.0, -1100.0), 140.0, 0.92, False),
    ((2600.0, -300.0), 260.0, 1.1, True),
    ((2500.0, 500.0), 75.0, 0.95, True),
    ((2300.0, 1300.0), 200.0, 1.05, False),
    ((2050.0, 2000.0), 320.0, 1.0, True),
    ((1700.0, 2700.0), 110.0, 0.9, False),
    ((1300.0, 3300.0), 230.0, 1.08, True),
    ((900.0, 3900.0), 35.0, 0.96, False),
)
ROAD_TREE = dict(yaw=-60.0, scale=1.05, crown=True)
# Hob on the road tree: where its first limb is thick and level, a metre from the trunk (Sink.py's DEAD_TREE_LIMBS, limb 1
# at about 3 m: Blender (0.92, -0.10, 3.00), radius 0.11, in the tree's Unreal frame, on its top) (cm).
ROAD_TREE_PERCH = unreal.Vector(10.0, -92.0, 312.0)

# Traces for the walls start at the Sink's middle and reach this far (cm); floors are looked for this far down.
WALL_REACH = 3000.0
FLOOR_ABOVE = 200.0
FLOOR_BELOW = 500.0


# ---------------------------------------------------------------------------
# The Sink's frame, and finding the floor and the walls
# ---------------------------------------------------------------------------

class Sink:
    """The Sink's frame: its middle, the floor's height there, its outline, and what this script has placed (traces look
    past it)."""

    def __init__(self, build, zone):
        self.build = build
        self.zone = zone['polygon']
        self.cx, self.cy = story.centroid(self.zone)
        self.fz = story.ground_at(self.cx, self.cy)
        self.placed = []

    def at(self, dx, dy):
        """The world (x, y) of a frame offset."""
        return self.cx + dx, self.cy + dy

    def along(self, bearing, dist):
        """The world (x, y) dist out from the middle along a bearing."""
        b = math.radians(bearing)
        return self.cx + math.cos(b) * dist, self.cy + math.sin(b) * dist

    def floor(self, x, y):
        """The floor's height under (x, y), past what's placed here (the blocks, the sacs)."""
        hit = trace(unreal.Vector(x, y, self.fz + FLOOR_ABOVE), unreal.Vector(x, y, self.fz - FLOOR_BELOW), self.placed)
        return self.fz if hit is None else hit[0].z

    def wall(self, bearing, up):
        """Where a level line from the middle, up over the floor, meets the wall along a bearing: (point, distance from the
        middle), or None."""
        start = unreal.Vector(self.cx, self.cy, self.fz + up)
        b = math.radians(bearing)
        end = unreal.Vector(self.cx + math.cos(b) * WALL_REACH, self.cy + math.sin(b) * WALL_REACH, self.fz + up)
        hit = trace(start, end, self.placed)
        if hit is None:
            self.build.warn(f'no wall along {bearing:.0f} degrees, {up:.0f} cm over the Sink\'s floor: what hangs there is '
                            f'left out')
            return None
        point = hit[0]
        return point, math.hypot(point.x - self.cx, point.y - self.cy)

    def keep(self, actor):
        if actor is not None:
            self.placed.append(actor)
        return actor


def trace(start, end, ignore=()):
    """The first thing a line meets (the Visibility channel, complex): (impact point, impact normal), or None."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, start, end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True,
                                                 list(ignore), unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        return None
    fields = hit.to_tuple()
    return fields[5], fields[7]


def mesh(build, name):
    """A model under /Game/Art/Props (SM_<name>), or None (with a warning)."""
    path = PROPS + 'SM_' + name
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        build.warn(f'no {path} yet (Art/Models/Props: import it): it\'s left out')
        return None
    return unreal.load_asset(path)


def turn(actor, yaw, pitch=0.0, roll=0.0):
    actor.set_actor_rotation(unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw), False)


def quiet(actor):
    """A web card by the art's rules: nothing meets it, and it casts no shadow."""
    component = actor.static_mesh_component
    component.set_editor_property('cast_shadow', False)
    component.set_editor_property('use_default_collision', False)
    component.set_collision_profile_name('NoCollision')
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return actor


def card(build, sink, name, location, yaw, label, pitch=0.0, roll=0.0, scale=None):
    """A web card (SM_Web_<name>), quiet."""
    model = mesh(build, 'Web_' + name)
    if model is None:
        return None
    made = build.place(model, location, yaw, label=label, folder='Gameplay', scale=scale)
    if pitch or roll:
        turn(made, yaw, pitch, roll)
    return sink.keep(quiet(made))


def facing(bearing):
    """Facing back toward the middle from a bearing out of it."""
    return bearing + 180.0


# ---------------------------------------------------------------------------
# The floor: blocks and coffins
# ---------------------------------------------------------------------------

def place_floor(build, sink, blocks_zone):
    """The piles and the loose coffins. Returns the pieces placed per pile ({name: [(model, actor)]})."""
    piles = {}
    for name, (dx, dy), pieces in PILES:
        x, y = sink.at(dx, dy)
        if blocks_zone is not None and not inside_polygon((x, y), blocks_zone['polygon']):
            build.warn(f'the {name} pile\'s middle ({x:.0f}, {y:.0f}) is outside sinkFloorBlocks')
        base = sink.floor(x, y)
        placed = []
        for index, (model_name, px, py, z, yaw, pitch, roll) in enumerate(pieces):
            model = mesh(build, model_name)
            if model is None:
                placed.append((model_name, None))
                continue
            made = build.place(model, (x + px, y + py, base + z), yaw, label=f'Sink_{name}_{index + 1}_{model_name}',
                               folder='Gameplay', tags=('Obstacle',))
            if pitch or roll:
                turn(made, yaw, pitch, roll)
            placed.append((model_name, sink.keep(made)))
        piles[name] = placed
    for index, (model_name, dx, dy, yaw) in enumerate(COFFINS):
        model = mesh(build, model_name)
        if model is None:
            continue
        x, y = sink.at(dx, dy)
        sink.keep(build.place(model, (x, y, sink.floor(x, y)), yaw, label=f'Sink_Coffin_{index + 1}', folder='Gameplay',
                              tags=('Obstacle',)))
    count = sum(len(pieces) for _, _, pieces in PILES) + len(COFFINS)
    build.log(f'the Sink\'s floor: {count} blocks and coffins in {len(PILES)} piles and loose, floor at {sink.fz:.0f}')
    return piles


def inside_polygon(point, polygon):
    x, y = point
    result = False
    for i in range(len(polygon)):
        (x1, y1), (x2, y2) = polygon[i], polygon[i - 1]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
            result = not result
    return result


# ---------------------------------------------------------------------------
# The web cards
# ---------------------------------------------------------------------------

def place_webs(build, sink, piles):
    """The corners, drapes, ground mats, the orb, and the ramp edge's tatters and strands."""
    placed = 0
    for index, bearing in enumerate(CORNERS):
        wall = sink.wall(bearing, 30.0)
        if wall is None:
            continue
        point, dist = wall
        x, y = sink.along(bearing, dist - CORNER_OUT)
        # The pivot on the wall's foot, the mat out over the floor (its front, +X), the rock behind.
        made = card(build, sink, 'Corner', (x, y, sink.floor(x, y)), facing(bearing), f'Sink_Web_Corner_{index + 1}')
        placed += made is not None
    for index, (pile, piece, face) in enumerate(DRAPES):
        pieces = piles.get(pile, [])
        if piece >= len(pieces) or pieces[piece][1] is None:
            continue
        model_name, block = pieces[piece]
        fit = DRAPE_FIT.get(model_name)
        if fit is None:
            continue
        # At the foot of the long face, mid-length, facing out of it; over its top edge.
        yaw = block.get_actor_rotation().yaw + (0.0 if face > 0 else 180.0)
        out = unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw).get_forward_vector()
        at = block.get_actor_location()
        x, y = at.x + out.x * DEPTH[model_name], at.y + out.y * DEPTH[model_name]
        placed += card(build, sink, 'Drape', (x, y, sink.floor(x, y)), yaw, f'Sink_Web_Drape_{index + 1}',
                       scale=(1.0, fit[1], fit[0])) is not None
    for index, (dx, dy, yaw) in enumerate(GROUND_WEBS):
        x, y = sink.at(dx, dy)
        placed += card(build, sink, 'Ground', (x, y, sink.floor(x, y)), yaw, f'Sink_Web_Ground_{index + 1}') is not None
    wall = sink.wall(ORB['bearing'], ORB['hub_up'])
    if wall is not None:
        x, y = sink.along(ORB['bearing'], wall[1] - ORB['off_wall'])
        made = card(build, sink, 'Orb', (x, y, sink.fz + ORB['hub_up']), facing(ORB['bearing']), 'Sink_Web_Orb')
        placed += made is not None
    placed += place_ramp_edge(build, sink)
    build.log(f'{placed} web cards on the Sink\'s floor, walls and ramp (shadows off, no collision)')
    return placed


def place_ramp_edge(build, sink):
    """Tatters and strands hanging from the ramp's inner edge, out of its face, in front of the cliff under it."""
    ramp = build.layout.get('ramps', {}).get('sinkRamp')
    if ramp is None:
        build.warn('layout_computed.json has no ramp sinkRamp: no webs hang from the ramp\'s edge')
        return 0
    points = ramp['points']
    half = ramp.get('width', 400.0) * 0.5
    placed = 0
    for name, indices in (('Tatters', TATTERS), ('Strands', STRANDS)):
        for index in indices:
            if index + 1 >= len(points):
                continue
            (ax, ay, az), (bx, by, _) = points[index], points[index + 1]
            length = math.hypot(bx - ax, by - ay) or 1.0
            # The ramp's left side drops to the floor (its "drop": "left"): toward the middle.
            nx, ny = (by - ay) / length, -(bx - ax) / length
            if nx * (sink.cx - ax) + ny * (sink.cy - ay) < 0.0:
                nx, ny = -nx, -ny
            # Its inner face, found by a level line from over the floor back toward the ramp, 60 cm under the edge.
            start = unreal.Vector(ax + nx * (half + 400.0), ay + ny * (half + 400.0), az - 60.0)
            end = unreal.Vector(ax - nx * 100.0, ay - ny * 100.0, az - 60.0)
            hit = trace(start, end, sink.placed)
            if hit is not None:
                fx, fy = hit[0].x + nx * EDGE_OUT, hit[0].y + ny * EDGE_OUT
            else:
                fx, fy = ax + nx * (half + EDGE_OUT), ay + ny * (half + EDGE_OUT)
            yaw = math.degrees(math.atan2(ny, nx))
            placed += card(build, sink, name, (fx, fy, az - 5.0), yaw, f'Sink_Web_{name}_{index}') is not None
    return placed


def place_line(build, sink, a, b, label):
    """A Web_Line from a (glued) to b: built 4 m long from its pivot along its -Y, sagging 10 cm; stretched (Y and Z) to
    span."""
    model = mesh(build, 'Web_Line')
    if model is None:
        return None
    span = b - a
    length = span.length()
    if length < 1.0:
        return None
    stretch = length / 400.0
    back = unreal.Vector(-span.x / length, -span.y / length, -span.z / length)
    rotation = unreal.MathLibrary.make_rot_from_yz(back, unreal.Vector(0.0, 0.0, 1.0))
    made = build.place(model, (a.x, a.y, a.z), 0.0, label=label, folder='Gameplay', scale=(1.0, stretch, stretch))
    made.set_actor_rotation(rotation, False)
    return sink.keep(quiet(made))


# ---------------------------------------------------------------------------
# The egg sacs and the lantern
# ---------------------------------------------------------------------------

def spider_ground(sink):
    """The floor's spiders' and the sacs' spiders' hunting ground: the zone sink, and FLOOR_RISE over their homes."""
    ground = unreal.HuntingGround()
    ground.set_editor_property('corners', [unreal.Vector(x, y, sink.fz) for x, y in sink.zone])
    ground.set_editor_property('margin', 200.0)
    ground.set_editor_property('max_rise', FLOOR_RISE)
    ground.set_editor_property('around_home', True)
    return ground


def hang_sac(build, sink, cls, spec, index):
    """One egg sac: its lines or sling, the sac (its story set last, so the editor shows it as play will)."""
    model = mesh(build, spec['model'])
    if model is None:
        return None
    bearing = spec['bearing']
    yaw = facing(bearing)
    if spec['kind'] == 'sling':
        wall = sink.wall(bearing, spec['pivot_up'] + 60.0)
        if wall is None:
            return None
        x, y = sink.along(bearing, wall[1] - SLING_OFF_WALL)
        pivot = unreal.Vector(x, y, sink.fz + spec['pivot_up'])
        sling = card(build, sink, 'Sling', (pivot.x, pivot.y, pivot.z), yaw, f'Sink_Web_Sling_{index + 1}')
        if sling is not None and sling.static_mesh_component.does_socket_exist('Sac'):
            # The sac sits on the sling's SOCKET_Sac (its pivot, as Sink.py makes it).
            pivot = sling.static_mesh_component.get_socket_location('Sac')
        silk = None
    else:
        x, y = sink.along(bearing, spec['dist'])
        silk = unreal.Vector(x, y, sink.fz + spec['silk_up'])
        pivot = unreal.Vector(x, y, silk.z - 220.0)
    sac = sink.keep(build.place(cls, (pivot.x, pivot.y, pivot.z), yaw, label=f'Sink_{spec["model"]}', folder='Gameplay',
                                tags=(SAC_TAG,)))
    body = sac.get_editor_property('sac')
    body.set_static_mesh(model)
    if silk is not None:
        # Hung by its silk: SOCKET_Silk on the lines' meeting point, two lines out to the wall either side.
        if body.does_socket_exist('Silk'):
            offset = silk - body.get_socket_location('Silk')
            sac.set_actor_location(sac.get_actor_location() + offset, False, True)
        else:
            build.warn(f'SM_{spec["model"]} has no SOCKET_Silk: it hangs by its top instead')
        for side, anchor in enumerate(spec['anchors']):
            wall = sink.wall(anchor, spec['anchor_up'])
            if wall is not None:
                place_line(build, sink, wall[0], silk, f'Sink_Web_Line_{index + 1}{"ab"[side]}')
    at = sac.get_actor_location()
    sac.set_editor_property('drop_height', max(at.z - sink.floor(at.x, at.y), 50.0))
    sac.set_editor_property('burst_out', spec.get('burst_out', 0.0))
    sac.set_editor_property('spider_ground', spider_ground(sink))
    sac.set_editor_property('spider_tags', [unreal.Name(SAC_SPIDER_TAG)])
    # Main 5's second step only; burst from the start after it, and after Main 5 (from 0: 1, 2).
    sac.set_editor_property('down_when', [story.condition(during=MAIN5, from_step=LANTERN_STEP),
                                          story.condition(after=[MAIN5])])
    sac.set_editor_property('shootable_when', story.condition(during=MAIN5, from_step=SAC_STEP,
                                                              before_step=LANTERN_STEP))
    build.log(f'{spec["model"]} ({spec["kind"]}) at ({at.x:.0f}, {at.y:.0f}), {at.z - sink.fz:.0f} cm over the floor')
    return sac


def place_sacs(build, sink):
    cls = story.actor_class('EggSac')
    if cls is None:
        build.warn('no EggSac class (build the game module first): no egg sacs')
        return []
    sacs = [hang_sac(build, sink, cls, spec, index) for index, spec in enumerate(SACS)]
    return [sac for sac in sacs if sac is not None]


def place_lantern(build, sink):
    """The snare at the north wall's foot, the lantern hanging dark from it, taken in Main 5's third step."""
    cls = story.actor_class('KeepersLantern')
    if cls is None:
        build.warn('no KeepersLantern class (build the game module first): no Keeper\'s Lantern')
        return None
    wall = sink.wall(LANTERN['bearing'], 120.0)
    if wall is None:
        return None
    x, y = sink.along(LANTERN['bearing'], wall[1] - LANTERN['off_wall'])
    lantern = sink.keep(build.place(cls, (x, y, sink.floor(x, y) + LANTERN['middle_up']), facing(LANTERN['bearing']),
                                    label='KeepersLantern', folder='Gameplay', tags=(LANTERN_TAG,)))
    for part, name in (('snare', 'Web_Snare'), ('lantern', 'KeepersLantern')):
        if mesh(build, name) is None:
            build.warn(f'the lantern\'s {part} shows nothing until SM_{name} is imported')
    # Set last, so the editor builds it again and hangs it from the snare's cord, dark.
    lantern.set_editor_property('take_when', story.condition(during=MAIN5, from_step=LANTERN_STEP,
                                                             before_step=OUT_STEP))
    build.log(f'the Keeper\'s Lantern in its snare at ({x:.0f}, {y:.0f}), {LANTERN["off_wall"]:.0f} cm out of the north '
              f'wall')
    return lantern


# ---------------------------------------------------------------------------
# The places, the floor's spiders, the Webwood
# ---------------------------------------------------------------------------

def place_markers(build, sink):
    """The floor's middle, and the ramp head on the rim. Returns the ramp head (x, y, z) or None."""
    build.place(unreal.TargetPoint, (sink.cx, sink.cy, sink.fz + story.MARKER_UP), 0.0, label=PLACE_FLOOR,
                folder='Gameplay', tags=(PLACE_FLOOR,))
    ramp = build.layout.get('ramps', {}).get('sinkRamp')
    if ramp is None or not ramp.get('points'):
        build.warn('layout_computed.json has no ramp sinkRamp: no place at its head for "Climb out"')
        return None
    hx, hy = ramp['points'][0][0], ramp['points'][0][1]
    hz = story.marker(build, hx, hy, PLACE_RIM)
    build.log(f'the Sink\'s floor at ({sink.cx:.0f}, {sink.cy:.0f}) and the ramp head at ({hx:.0f}, {hy:.0f}), tagged '
              f'{PLACE_FLOOR} and {PLACE_RIM}')
    return hx, hy, hz - story.MARKER_UP


def place_floor_spiders(build, sink):
    spider = story.actor_class('SpiderCreature')
    cls = story.actor_class('EncounterSpawner')
    if spider is None or cls is None:
        build.warn('no EncounterSpawner or SpiderCreature class (build the game module first): no spiders on the Sink\'s '
                   'floor')
        return
    made = build.place(cls, (sink.cx, sink.cy, sink.fz + 50.0), 0.0, label=f'Encounter_{FLOOR}', folder='Gameplay')
    made.set_editor_property('spawner_id', unreal.Name(FLOOR))
    group = unreal.EncounterGroup()
    group.set_editor_property('creature_class', spider)
    group.set_editor_property('count', FLOOR_COUNT)
    group.set_editor_property('rank_roll', unreal.EncounterRankRoll.AREA)
    made.set_editor_property('groups', [group])
    made.set_editor_property('creature_tags', [unreal.Name(FLOOR_TAG)])
    # Main 5 until the lantern is taken (from 0: before 3): a session loaded climbing out finds the floor quiet.
    made.set_editor_property('active_when', story.condition(during=MAIN5, before_step=OUT_STEP))
    made.set_editor_property('spawn_points', [unreal.Vector(x, y, 0.0) for x, y in FLOOR_SPOTS])
    # They stand among the blocks until they notice the player on the floor.
    made.set_editor_property('hunt_on_spawn', False)
    made.set_editor_property('ground_corners', [unreal.Vector(x, y, sink.fz) for x, y in sink.zone])
    made.set_editor_property('ground_max_rise', FLOOR_RISE)
    build.log(f'the floor\'s spiders ({FLOOR}: {FLOOR_COUNT} of the area\'s ranks) among the blocks, during {MAIN5} until '
              f'the lantern is taken')


def place_tree(build, model, crown, x, y, yaw, scale, label):
    """A dead tree on the ground at (x, y), tagged Obstacle, and its Web_Crown with its transform (uniform scale)."""
    tree = build.place(model, (x, y, story.ground_at(x, y)), yaw, label=label, folder='Gameplay', scale=(scale,) * 3,
                       tags=('Obstacle',))
    if crown is not None:
        at = tree.get_actor_location()
        quiet(build.place(crown, (at.x, at.y, at.z), yaw, label=f'{label}_Crown', folder='Gameplay', scale=(scale,) * 3))
    return tree


def place_webwood(build, sink):
    """The Webwood's dead trees north of the Sink and the Sink road's one. Returns the road's tree, or None."""
    if not unreal.EditorAssetLibrary.does_asset_exist(DEAD_TREE):
        build.warn(f'no {DEAD_TREE} yet: no Webwood')
        return None
    model = unreal.load_asset(DEAD_TREE)
    crown = mesh(build, 'Web_Crown')
    wood = story.layout_entry(build, 'obstacles', 'webwood')
    crowned = 0
    for index, ((dx, dy), yaw, scale, wears) in enumerate(WEBWOOD):
        x, y = sink.at(dx, dy)
        if wood is not None and not inside_polygon((x, y), wood['polygon']):
            build.warn(f'the Webwood\'s tree {index + 1} at ({x:.0f}, {y:.0f}) is outside the obstacle webwood')
        place_tree(build, model, crown if wears else None, x, y, yaw, scale, f'Webwood_DeadTree_{index + 1}')
        crowned += 1 if wears and crown is not None else 0
    road = story.layout_entry(build, 'obstacles', 'sinkRoadDeadTree')
    tree = None
    if road is not None:
        x, y = story.centroid(road['polygon'])
        tree = place_tree(build, model, crown if ROAD_TREE['crown'] else None, x, y, ROAD_TREE['yaw'], ROAD_TREE['scale'],
                          'SinkRoad_DeadTree')
    build.log(f'the Webwood: {len(WEBWOOD)} dead trees ({crowned} webbed) and the Sink road\'s')
    return tree


# ---------------------------------------------------------------------------
# Hob's perches, and everything
# ---------------------------------------------------------------------------

def place_funnel(build, sink, den):
    """The den's silk lining on Den Rock's SOCKET_DenMouth, turned as the socket is: its pivot is the socket (Sink.py)."""
    mouth = story.socket(den, DEN_MOUTH) if den is not None else None
    if mouth is None:
        if den is not None:
            build.warn(f'Den Rock has no SOCKET_{DEN_MOUTH}: no web funnel in the den\'s mouth')
        return None
    at = mouth.translation
    funnel = card(build, sink, 'Funnel', (at.x, at.y, at.z), mouth.rotation.rotator().yaw, 'Sink_Web_Funnel')
    if funnel is not None:
        funnel.set_actor_rotation(mouth.rotation.rotator(), False)
        build.log(f'the den\'s web funnel on its mouth at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')
    return funnel


def hob_spots(build, sink, piles, head, road_tree, den):
    """Hob's Main 5 perches, (x, y, z, yaw) each or None: on Den Rock over the den, looking across at the ramp head for the
    way down; on the north pile's top block, looking at the ramp's foot, for the sacs and the lantern; on the Sink road's
    dead tree (else the ground at the ramp head) looking into the Sink, for the way out and after."""
    spots = {'way': None, 'block': None, 'rim': None}
    ramp = build.layout.get('ramps', {}).get('sinkRamp')
    foot = tuple(ramp['points'][-1][:2]) if ramp and ramp.get('points') else (sink.cx, sink.cy)
    if den is not None:
        origin, _ = den.get_actor_bounds(False)
        top = story.ground_at(origin.x, origin.y, origin.z)
        toward = head[:2] if head else (sink.cx, sink.cy)
        spots['way'] = (origin.x, origin.y, top, story.facing((origin.x, origin.y), toward))
    pile, piece = HOB_PILE
    pieces = piles.get(pile, [])
    if piece < len(pieces) and pieces[piece][1] is not None:
        at = pieces[piece][1].get_actor_location()
        spots['block'] = (at.x, at.y, story.ground_at(at.x, at.y), story.facing((at.x, at.y), foot))
    if road_tree is not None:
        perch = road_tree.get_actor_transform().transform_location(ROAD_TREE_PERCH)
        spots['rim'] = (perch.x, perch.y, perch.z, story.facing((perch.x, perch.y), (sink.cx, sink.cy)))
    elif head is not None:
        spots['rim'] = (head[0], head[1], head[2], story.facing(head[:2], (sink.cx, sink.cy)))
    return spots


def place(build):
    """Everything above, in the Gameplay folder. Returns Hob's Main 5 perches for build_area_story's place_hob."""
    zone = story.layout_entry(build, 'zones', 'sink')
    if zone is None:
        build.warn('no Sink: Main 5 has nowhere to happen')
        return {'way': None, 'block': None, 'rim': None}
    sink = Sink(build, zone)
    # The story's pieces first (their walls traced before anything stands on the floor), then the floor and its webs.
    place_sacs(build, sink)
    place_lantern(build, sink)
    piles = place_floor(build, sink, story.layout_entry(build, 'obstacles', 'sinkFloorBlocks'))
    place_webs(build, sink, piles)
    head = place_markers(build, sink)
    place_floor_spiders(build, sink)
    road_tree = place_webwood(build, sink)
    den = story.placed(build, 'DenRock')
    place_funnel(build, sink, den)
    return hob_spots(build, sink, piles, head, road_tree, den)
