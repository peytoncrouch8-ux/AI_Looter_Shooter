"""Ransom's Rest's valley as wave 4 grew it (Docs/Areas/RansomsRest_Expansion.md, sections 7 and 9), for build_area.py's full
build: the level's own pieces that the models and their placements can't give by themselves, placed once the models and
the effects stand. The house swaps, the new lots (Pen Lane, the dugout, Amos's house, Tilly's workshop over its cellar, the
coal stage, the town's well), the depot's new pivot and the barn are layout data (Art/Levels/RansomsRest/patches/town.json:
placements, level.models, lights and smoke, the pads, the stair's hole); this places the rest, in the area's Town folder:

- Keeper's Bridge grounded (the user's bug, 2026-10-13: "it floats, and walking up the hill to it stops the player; it
  takes several jumps"). The generator lays the bridge level at the road's height where it crosses the Dry Wash, so on the
  keeper's path's high side its end was buried in the wash's 45-degree bank, 2 m under the road, and on the low side it
  hung 40 cm over the road. Here each end is found on the terrain itself, walking out from the wash's bed along the bridge
  until the ground is the road again (the slope eased under CREST_SLOPE for a metre and level across the deck's width),
  END_PAST further on: the deck runs from end to end, pitched with the ground (about 10 degrees, its ramps on the road at
  both ends, so the walk is the path's own both ways), stretched to the span, and its stringers' stones stand on piers of
  the quarry's dressed blocks down to the bank. Logged with each end's height and the step left at each ramp; with no
  bank found it stays as the generator laid it, with a warning.
- The depot's and Tilly's workshop's lamps: AHouseLights on the art's Light_* sockets (the depot's Light, the waiting
  room's two, the ticket office's, Lost Property's, the stair's and the agent's upstairs window; the workshop's, its
  cellar stair's, the cellar's two and the cold room's; Amos's hay bay and tack room), unshadowed and kept to their rooms.
  The depot's Light also brightens its windows at dusk (its WindowGlow, the upper window lit); the depot is a station
  actor, which build_area.py's house_lights doesn't look at.
- Smoke from a smoking building's second chimney (SOCKET_Smoke_2: the depot, the gallery house), as effects() puts it on
  SOCKET_Smoke.
- The windmill's own wheel (level.models WindmillFan: SM_RR_WindmillFan on SM_RR_Windmill's Fan socket) when build_area.py's
  models() still hung the shared one (its hook line is in this module's report).
- No-tree boxes (tagged NoTrees, as build_area.py's no_tree_zones) over the new back lots outside the town's zones: Pen
  Lane, the workshop's yard, the dugout, Amos's house, the coal stage. Run the scatter after this.
- Tilly's cellar marked for the minimap (a box tagged Underground over the cellar and the cold room, the UNDERGROUND plate)
  and checked: the terrain open over its stair (mesh.holes cellarStair) and nowhere over the vaults lower than 0.4 m
  under the workshop's pivot (the vaults' tops); a warning names any spot that fails.
Missing art or a missing placement is tolerated: each piece warns and is left out.

Hook in build_area.py's run() (the full build), right after self.effects(meshes):
    importlib.reload(importlib.import_module('build_area_town_rr')).place(self, meshes)
On its own, in the open editor (clears the Town folder, grounds the bridge again, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_town_rr.py RansomsRest"
Only the level whose layout has Ellis's grave (Ransom's Rest) gets any of it.
"""
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area  # noqa: E402

FOLDER = 'Town'
CLASSES = '/Script/AI_Looter_Shooter.'

# --- Keeper's Bridge ---
BRIDGE_LABEL = 'Bridge'
BRIDGE_ID = 'keepersBridge'
STEP = 10.0                # cm between samples along the bridge
SEARCH = 1600.0            # how far either way of the bridge's middle the wash's bed is looked for (cm)
MOST_REACH = 1500.0        # how far out from the bed a bank's top may be (cm)
CREST_SLOPE = 0.45         # the ground is the road again once its slope stays under this (rise per run) ...
CREST_RUN = 100.0          # ... for this far outward (cm)
CREST_ACROSS = 20.0        # ... and the deck's two edges stand within this of each other (cm)
BANK_LEAST = 150.0         # a bank's top stands at least this over the bed (cm): the bed's own gentle floor isn't it
DECK_HALF = 110.0          # the deck's half width (Bridge.py WIDE)
END_PAST = 50.0            # the ramp's end this far past the bank's top, on the road (cm)
STEEPEST = 15.0            # a deck pitched more than this (degrees) is warned about
# Bridge.py: the stones under the stringers' ends at HALF - 0.35 of the half length HALF + RAMP + 0.05 (3.15 of 4.35 m),
# their bottoms 6 cm under the pivot line.
STONE_SHARE = 3.15 / 4.35
STONE_UNDER = 6.0
# The piers: the quarry's dressed blocks (Ruins.py DressedBlock_B: 1.7 x 0.8 x 0.55 m, its pivot at its foot; its 1.7 m
# along its actor's Y), turned as the bridge is so they lie across the deck, stacked from a hand under the lowest ground
# beneath them to the stones; none where the gap is under PIER_LEAST.
PIER_MESH = 'DressedBlock_B'
PIER_COURSE = 52.0
PIER_SINK = 12.0
PIER_LEAST = 25.0
PIER_FOOT = ((-40.0, -85.0), (-40.0, 85.0), (40.0, -85.0), (40.0, 85.0), (0.0, 0.0))   # along, across (cm)

# --- Lamps (AHouseLights): socket, candelas by day and at dusk, reach (cm), and whether it brightens the windows ---
DEPOT_LAMPS = (
    ('Light', (2.5, 8.0), 320.0, True),            # the lamp by the station board, under the canopy
    ('Light_Waiting_1', (3.0, 6.0), 450.0, False),
    ('Light_Waiting_2', (3.0, 6.0), 450.0, False),
    ('Light_Ticket', (2.0, 4.0), 300.0, False),
    ('Light_LostProperty', (2.5, 5.0), 300.0, False),
    ('Light_Stair', (1.5, 3.0), 250.0, False),
    ('Light_Upstairs', (2.0, 6.0), 350.0, False),   # Mr. Sloane's room: the upper window lit
)
WORKSHOP_LAMPS = (
    ('Light_Workshop', (2.0, 4.0), 450.0, False),
    ('Light_CellarStair', (1.5, 3.0), 280.0, False),
    # The cellar's two: low and warm, enough to fight by; kept under 5 m so little reaches the yard's walls over them.
    ('Light_Cellar_1', (4.0, 4.0), 480.0, False),
    ('Light_Cellar_2', (4.0, 4.0), 480.0, False),
    ('Light_ColdRoom', (2.5, 2.5), 330.0, False),
)
BARN_LAMPS = (
    ('Light_Bay', (1.0, 3.0), 380.0, False),
    ('Light_Tack', (1.0, 2.5), 300.0, False),
)
LAMP_SETS = (('depot', DEPOT_LAMPS), ('workshop', WORKSHOP_LAMPS), ('whitlockBarn', BARN_LAMPS))

# --- No-tree boxes over the back lots (layout cm): label, middle, half sizes, yaw ---
NO_TREES = (
    ('PenLane', (-3000.0, 2950.0), (800.0, 1050.0), 0.0),
    ('WorkshopYard', (-2700.0, 6400.0), (900.0, 1000.0), 0.0),
    ('Dugout', (2350.0, -1400.0), (700.0, 800.0), 0.0),
    ('AmosHouse', (-7050.0, 1650.0), (700.0, 700.0), 0.0),
    ('CoalStage', (-470.0, 10740.0), (450.0, 400.0), 0.0),
)

# --- Tilly's cellar (SM_RR_Workshop_Bright, Art/Models/Buildings/WorkshopBright.py: from its pivot, cm) ---
CELLAR = (-725.0, 155.0, -825.0, 455.0)        # X from..to, Y from..to
COLD_ROOM = (-1165.0, -685.0, -25.0, 455.0)
CELLAR_Z = (-360.0, -40.0)                      # nothing may stand in it; the vaults' tops 40 cm under the pivot
STAIRWELL = (-700.0, -273.0, -800.0, -548.0)    # the terrain opens over it (or lies under -3.6 m)
VAULT_TOP = -40.0
FLOOR_OVER = 30.0                               # the workshop's floor over its pivot: ground above it buries the floor

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def level_actors(build):
    """The actors the area's builds placed, by label (the first of each)."""
    found = {}
    for actor in actors.get_all_level_actors():
        if unreal.Name(build.tag) in actor.tags:
            found.setdefault(str(actor.get_actor_label()), actor)
    return found


def actor_class(name):
    return unreal.load_class(None, CLASSES + name)


def building_mesh(actor):
    """The component carrying a building's model: a placed mesh's own, or a station's Building."""
    if actor is None:
        return None
    if isinstance(actor, unreal.StaticMeshActor):
        return actor.static_mesh_component
    try:
        return actor.get_editor_property('building')
    except Exception:
        return actor.get_component_by_class(unreal.StaticMeshComponent)


def socket_transform(component, name):
    if component is None or not component.does_socket_exist(name):
        return None
    return component.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)


class Terrain:
    """The terrain's tiles alone (tagged Ground): its height under a point, never a rock, a fence or the bridge."""

    def __init__(self, build):
        self.tiles = build_area.terrain_tiles(build.tag)

    def __call__(self, x, y):
        hit = build_area.terrain_hit(self.tiles, unreal.Vector(x, y, 30000.0), unreal.Vector(x, y, -30000.0))
        return None if hit is None else hit.z


# ---------------------------------------------------------------------------
# Keeper's Bridge
# ---------------------------------------------------------------------------

def bridge_spec(build):
    bridges = build.layout.get('bridges') or {}
    return bridges.get(BRIDGE_ID) or build.layout.get('bridge')


def find_bank_tops(build, terrain, spec):
    """Where the road is whole again on each side of the wash, along the bridge's line: ((s, z) toward -X, (s, z) toward
    +X) in cm from the generator's middle, or None for a side with no such ground within MOST_REACH of the bed."""
    cx, cy = spec['location'][0], spec['location'][1]
    yaw = math.radians(spec['yaw'])
    ax, ay = math.cos(yaw), math.sin(yaw)
    sx, sy = -ay, ax

    def at(s, across=0.0):
        return terrain(cx + ax * s + sx * across, cy + ay * s + sy * across)

    # The bed: the lowest ground along the line.
    samples = []
    s = -SEARCH
    while s <= SEARCH:
        z = at(s)
        if z is not None:
            samples.append((z, s))
        s += STEP
    if not samples:
        return None, None, None
    bed_z, bed_s = min(samples)

    def crest(sign):
        # Out of the bed's flat first, up the bank (steeper than CREST_SLOPE), then the first ground the road's again.
        k = 1
        climbed = False
        last_here = bed_z
        while k * STEP <= MOST_REACH:
            s = bed_s + sign * k * STEP
            here = at(s)
            if here is None:
                return None
            if abs(here - last_here) / STEP > CREST_SLOPE:
                climbed = True
            last_here = here
            if not climbed or here - bed_z < BANK_LEAST:
                k += 1
                continue
            ok = True
            run = 0.0
            last = here
            while run < CREST_RUN:
                run += STEP
                nxt = at(s + sign * run)
                if nxt is None or abs(nxt - last) / STEP > CREST_SLOPE:
                    ok = False
                    break
                last = nxt
            if ok:
                left, right = at(s, DECK_HALF), at(s, -DECK_HALF)
                if left is not None and right is not None and abs(left - right) <= CREST_ACROSS:
                    return s
            k += 1
        return None
    return (bed_s, bed_z), crest(-1.0), crest(1.0)


def ground_bridge(build, meshes, found, terrain):
    """Keeper's Bridge from bank to bank on the road, pitched with it, on stone piers. Returns its ends' world points or
    None."""
    spec = bridge_spec(build)
    bridge = found.get(BRIDGE_LABEL)
    if spec is None or bridge is None:
        build.warn('no Keeper\'s Bridge in the layout or the level (build the whole level first): it isn\'t grounded')
        return None
    bed, low, high = find_bank_tops(build, terrain, spec)
    if bed is None or low is None or high is None:
        build.warn(f'Keeper\'s Bridge: no road found on {"either side" if bed is None else "one side"} of the Dry Wash '
                   f'within {MOST_REACH / 100.0:.0f} m: it stays as the generator laid it')
        return None
    cx, cy = spec['location'][0], spec['location'][1]
    yaw = math.radians(spec['yaw'])
    ax, ay = math.cos(yaw), math.sin(yaw)
    sx, sy = -ay, ax
    ends = []
    for s in (low - END_PAST, high + END_PAST):
        x, y = cx + ax * s, cy + ay * s
        heights = [terrain(x + sx * a, y + sy * a) for a in (-DECK_HALF, 0.0, DECK_HALF)]
        heights = [h for h in heights if h is not None]
        if not heights:
            build.warn('Keeper\'s Bridge: no ground under one end: it stays as the generator laid it')
            return None
        ends.append(unreal.Vector(x, y, sum(heights) / len(heights)))
    a, b = ends
    run = math.hypot(b.x - a.x, b.y - a.y)
    length = math.sqrt(run * run + (b.z - a.z) ** 2)
    pitch = math.degrees(math.atan2(b.z - a.z, run))
    heading = math.degrees(math.atan2(b.y - a.y, b.x - a.x))
    mesh = bridge.static_mesh_component.static_mesh
    box = mesh.get_bounding_box()
    model = max(box.max.x - box.min.x, 1.0)
    middle = unreal.Vector((a.x + b.x) * 0.5, (a.y + b.y) * 0.5, (a.z + b.z) * 0.5)
    bridge.set_actor_location_and_rotation(middle, unreal.Rotator(roll=0.0, pitch=pitch, yaw=heading), False, True)
    bridge.set_actor_scale3d(unreal.Vector(length / model, 1.0, 1.0))
    build.log(f'Keeper\'s Bridge grounded: {length / 100.0:.1f} m from ({a.x:.0f}, {a.y:.0f}, {a.z:.0f}) to ({b.x:.0f}, '
              f'{b.y:.0f}, {b.z:.0f}), pitched {pitch:.1f} degrees (the wash\'s bed {bed[1]:.0f} at {bed[0]:.0f} cm along; '
              f'the generator\'s deck was level at {spec["location"][2]:.0f} over {spec["span"] / 100.0:.1f} m)')
    if abs(pitch) > STEEPEST:
        build.warn(f'Keeper\'s Bridge is pitched {pitch:.1f} degrees: steeper than {STEEPEST:.0f} (check the walk with '
                   'path_probe.py both ways)')
    check_ends(build, bridge, ends, heading)
    place_piers(build, meshes, terrain, a, b, heading)
    return ends


def check_ends(build, bridge, ends, heading):
    """The step left at each ramp's end: the bridge's own top there against the terrain beside it (logged)."""
    out = math.radians(heading)
    for index, end in enumerate(ends):
        sign = -1.0 if index == 0 else 1.0
        # 20 cm in from the end, onto the ramp's planks.
        x, y = end.x - sign * math.cos(out) * 20.0, end.y - sign * math.sin(out) * 20.0
        top = None
        for mesh in bridge.get_components_by_class(unreal.StaticMeshComponent):
            hit = mesh.line_trace_component(unreal.Vector(x, y, end.z + 300.0), unreal.Vector(x, y, end.z - 300.0),
                                            True, False, False)
            if hit:
                location = hit[0] if isinstance(hit, tuple) else hit.to_tuple()[5]
                top = location.z if top is None else max(top, location.z)
        if top is None:
            build.warn(f'Keeper\'s Bridge: nothing of the bridge found over its {"west" if index == 0 else "east"} end')
            continue
        build.log(f'Keeper\'s Bridge, {"first" if index == 0 else "second"} end: its ramp {top - end.z:+.0f} cm over the '
                  f'road at ({end.x:.0f}, {end.y:.0f})')


def place_piers(build, meshes, terrain, a, b, heading):
    """Stone piers under the stringers' ends, from a hand under the bank to the stones."""
    if PIER_MESH not in meshes:
        build.warn(f'no SM_{PIER_MESH} yet (Art/Models/Props/Ruins.py): Keeper\'s Bridge stands without its piers')
        return
    block = unreal.load_asset(meshes[PIER_MESH])
    box = block.get_bounding_box()
    tall = max(box.max.z - box.min.z, 1.0)
    out = math.radians(heading)
    ax, ay = math.cos(out), math.sin(out)
    sx, sy = -ay, ax
    middle = ((a.x + b.x) * 0.5, (a.y + b.y) * 0.5, (a.z + b.z) * 0.5)
    half = (b.x - middle[0], b.y - middle[1], b.z - middle[2])
    placed = 0
    for index, sign in enumerate((-1.0, 1.0)):
        px = middle[0] + half[0] * sign * STONE_SHARE
        py = middle[1] + half[1] * sign * STONE_SHARE
        top = middle[2] + half[2] * sign * STONE_SHARE - STONE_UNDER
        grounds = [terrain(px + ax * along + sx * across, py + ay * along + sy * across) for along, across in PIER_FOOT]
        grounds = [g for g in grounds if g is not None]
        if not grounds:
            continue
        base = min(grounds) - PIER_SINK
        gap = top - min(grounds)
        if gap < PIER_LEAST:
            build.log(f'Keeper\'s Bridge: its {"first" if index == 0 else "second"} stones sit on the bank ({gap:.0f} cm): '
                      'no pier')
            continue
        courses = max(1, int(math.ceil((top - base) / PIER_COURSE)))
        course = (top - base) / courses
        for k in range(courses):
            # Each course turned a little and set a little off the last, as a dry-laid pier is.
            turn = (3.0 if (k % 2) else -2.0) * (1.0 if index else -1.0)
            nudge = 4.0 if (k % 2) else -3.0
            build.place(block, (px + sx * nudge, py + sy * nudge, base + k * course), heading + turn,
                        label=f'KeepersBridge_Pier{index + 1}_{k + 1}', folder=FOLDER,
                        scale=(1.0, 1.0, course / tall), tags=('Obstacle',))
            placed += 1
        build.log(f'Keeper\'s Bridge: a pier of {courses} courses under its {"first" if index == 0 else "second"} '
                  f'stones, {gap / 100.0:.1f} m from the bank')
    if placed:
        build.log(f'{placed} dressed blocks under Keeper\'s Bridge')


# ---------------------------------------------------------------------------
# Lamps, smoke and the windmill's wheel
# ---------------------------------------------------------------------------

def place_lamps(build, found):
    cls = actor_class('HouseLights')
    if cls is None:
        build.warn('no HouseLights class (build the game module first): the depot and the workshop stay dark inside')
        return
    count = 0
    for key, lamps in LAMP_SETS:
        house = found.get(key)
        component = building_mesh(house)
        if component is None:
            build.warn(f'no {key} placed (build the whole level first): its lamps are left out')
            continue
        for socket, (day, dusk), reach, windows in lamps:
            at = socket_transform(component, socket)
            if at is None:
                build.warn(f'{key} has no SOCKET_{socket} (its new model isn\'t imported yet?): that lamp is left out')
                continue
            where = at.translation
            lights = build.place(cls, (where.x, where.y, where.z), at.rotation.rotator().yaw, label=f'Lamp_{key}_{socket}',
                                 folder=f'{FOLDER}/Lamps')
            if windows:
                lights.set_editor_property('house', house)
            lights.set_editor_property('day_lamp', day)
            lights.set_editor_property('dusk_lamp', dusk)
            lamp = lights.get_editor_property('lamp')
            lamp.set_editor_property('intensity', day)
            lamp.set_editor_property('attenuation_radius', reach)
            count += 1
    build.log(f'{count} lamps in the depot, Tilly\'s workshop and cellar, and Amos\'s barn')


def place_second_smoke(build, meshes, found):
    """Smoke from SOCKET_Smoke_2 on the buildings level.smoke names (effects() puts it on SOCKET_Smoke)."""
    if 'SmokePlume' not in meshes:
        return
    smoking = build.source.get('level', {}).get('smoke') or []
    plume = unreal.load_asset(meshes['SmokePlume'])
    count = 0
    for key in smoking:
        component = building_mesh(found.get(key))
        at = socket_transform(component, 'Smoke_2')
        if at is None:
            continue
        where = at.translation
        build.place(plume, (where.x, where.y, where.z), build_area.WIND_YAW, label=f'Smoke2_{key}', folder=f'{FOLDER}/Smoke')
        count += 1
    if count:
        build.log(f'{count} second chimneys smoking')


def fit_windmill(build, meshes, found):
    """The windmill's wheel as the area names it (level.models WindmillFan), on its tower's Fan socket."""
    fan_name = build.model_names.get('WindmillFan')
    windmill = found.get('windmill')
    if not fan_name or windmill is None:
        return
    if fan_name not in meshes:
        build.warn(f'no SM_{fan_name} yet: the windmill keeps the wheel it has')
        return
    try:
        tower = windmill.get_editor_property('tower')
        fan = windmill.get_editor_property('fan')
    except Exception:
        return
    model = unreal.load_asset(meshes[fan_name])
    if fan.static_mesh == model:
        return
    fan.set_static_mesh(model)
    snap = unreal.AttachmentRule.SNAP_TO_TARGET
    fan.attach_to_component(tower, 'Fan', snap, snap, unreal.AttachmentRule.KEEP_RELATIVE, False)
    fan.set_relative_location(unreal.Vector(0.0, 0.0, 1.0), False, True)
    fan.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, True)
    build.log(f'the windmill turns its own wheel, SM_{fan_name}')


# ---------------------------------------------------------------------------
# No trees on the back lots; the cellar
# ---------------------------------------------------------------------------

def place_no_tree_boxes(build):
    for label, (x, y), (hx, hy), yaw in NO_TREES:
        box = build.place(unreal.TriggerBox, (x, y, 0.0), yaw, label=f'NoTrees_{label}', folder=f'{FOLDER}/Scatter',
                          tags=('NoTrees',), scale=(hx / 32.0, hy / 32.0, 20000.0 / 64.0))
        box.set_actor_enable_collision(False)
    build.log(f'{len(NO_TREES)} no-tree boxes over the back lots (run the scatter after this)')


def local_box_corners(rect):
    x0, x1, y0, y1 = rect
    return ((x0, y0), (x1, y0), (x1, y1), (x0, y1))


def mark_cellar(build, found, terrain):
    """The cellar and the cold room as one Underground box; the terrain over them checked."""
    workshop = found.get('workshop')
    if workshop is None:
        build.warn('no workshop placed (patches/town.json): no cellar to mark')
        return
    frame = workshop.get_actor_transform()
    pivot = workshop.get_actor_location()
    x0, x1 = min(CELLAR[0], COLD_ROOM[0]), max(CELLAR[1], COLD_ROOM[1])
    y0, y1 = min(CELLAR[2], COLD_ROOM[2]), max(CELLAR[3], COLD_ROOM[3])
    middle = frame.transform_location(unreal.Vector((x0 + x1) * 0.5, (y0 + y1) * 0.5, (CELLAR_Z[0] + CELLAR_Z[1]) * 0.5))
    half = ((x1 - x0) * 0.5, (y1 - y0) * 0.5, (CELLAR_Z[1] - CELLAR_Z[0]) * 0.5 + 60.0)
    box = build.place(unreal.TriggerBox, (middle.x, middle.y, middle.z), workshop.get_actor_rotation().yaw,
                      label='Underground_TillyCellar', folder=FOLDER, tags=('Underground',),
                      scale=(half[0] / 32.0, half[1] / 32.0, half[2] / 32.0))
    box.set_actor_enable_collision(False)
    build.log('Tilly\'s cellar marked Underground for the minimap')

    # The stair's way down: no terrain over it above the cellar's floor.
    sx = (STAIRWELL[0] + STAIRWELL[1]) * 0.5
    sy = (STAIRWELL[2] + STAIRWELL[3]) * 0.5
    stair = frame.transform_location(unreal.Vector(sx, sy, 0.0))
    over = terrain(stair.x, stair.y)
    if over is not None and over > pivot.z + CELLAR_Z[0]:
        build.warn(f'the terrain closes over the cellar\'s stair at ({stair.x:.0f}, {stair.y:.0f}) ({over - pivot.z:+.0f} cm '
                   'from the workshop\'s pivot): layout.json mesh.holes needs cellarStair (patches/town.json)')
    # The vaults under the ground everywhere, and the workshop's floor over it.
    low = high = 0
    for rect in (CELLAR, COLD_ROOM):
        x0, x1, y0, y1 = rect
        for i in range(7):
            for j in range(7):
                lx, ly = x0 + (x1 - x0) * i / 6.0, y0 + (y1 - y0) * j / 6.0
                if STAIRWELL[0] <= lx <= STAIRWELL[1] and STAIRWELL[2] <= ly <= STAIRWELL[3]:
                    continue
                p = frame.transform_location(unreal.Vector(lx, ly, 0.0))
                z = terrain(p.x, p.y)
                if z is None:
                    continue
                if z < pivot.z + VAULT_TOP:
                    low += 1
                elif z > pivot.z + FLOOR_OVER and -700.0 <= lx <= 0.0 and -800.0 <= ly <= 200.0:
                    high += 1
    if low:
        build.warn(f'{low} spots over Tilly\'s cellar lie lower than its vaults\' tops (pivot {VAULT_TOP:.0f} cm): they show '
                   'through (the workshop pad, patches/town.json)')
    if high:
        build.warn(f'{high} spots of the workshop\'s floor lie under the terrain')
    if not low and not high:
        build.log('the terrain over Tilly\'s cellar keeps its vaults under the ground')


# ---------------------------------------------------------------------------
# All of it
# ---------------------------------------------------------------------------

def place(build, meshes=None):
    """Everything above (build_area.py's run() calls this after the effects). Only Ransom's Rest has it."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    meshes = meshes if meshes is not None else build_area.mesh_index()
    found = level_actors(build)
    terrain = Terrain(build)
    if not terrain.tiles:
        build.warn('no terrain tiles tagged Ground (build the whole level first): the bridge and the cellar aren\'t checked')
    else:
        ground_bridge(build, meshes, found, terrain)
    place_lamps(build, found)
    place_second_smoke(build, meshes, found)
    fit_windmill(build, meshes, found)
    place_no_tree_boxes(build)
    if terrain.tiles:
        mark_cellar(build, found, terrain)


def run(name):
    """Places the Town folder again on its own (and grounds the bridge again), saves."""
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the valley\'s town pieces placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_town_rr.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
