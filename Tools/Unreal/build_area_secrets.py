"""Things to find off the path on Ransom's Rest (the user's call, 2026-10-21: "nothing to find off the path"), for
build_area.py's gameplay pass, placed from the layout's obstacles and features and the actors build_area.py stood, so they
follow the level whenever it's rebuilt. Everything goes in the area's Gameplay/Secrets folder (a gameplay build clears
Gameplay first); a missing class, actor, layout entry or spot is warned of and left out. Only the level whose layout has
Ellis's grave (Ransom's Rest) gets any of it. The words, loot and anchors are Tools/Unreal/story_texts.py's (SECRETS,
READABLES); each find's readable sets its Secret.<Id> story flag when read (AReadableProp::SetsFlag).

- Each secret is an AChest (its chest_id kept by the session: never rename one; tagged Chest, its kind, Obstacle and
  Secret) with its FChestLootOverride, and, where it has words, an AReadableProp beside it ("Look" or "Read").
  The spot: round its anchor (a layout obstacle's middle, a placed actor, the Mooring Ledge, Coffin Rock's top) on a ring
  (story_texts SECRETS 'ring', cm), the side 'prefer' (layout x north, y east) tried first, on ground within
  MAX_RISE of the anchor's own and with nothing solid in the chest's box (WorldStatic and WorldDynamic overlaps).
- Coffin Rock's way up: STEP_COUNT boulders (SM_Boulder_A, scaled so each top is STEP_RISE over the last) climbing to its
  north edge, each in mantle reach (TraversalRules::MaxMantleHeight 150 cm). Tagged Obstacle. Not yet walked in PIE.
- What Ada gave up: Father Aldana's page (READABLES AldanaPage, sets Ada.Told; shown after Main 4) on the vestry door's
  step, on the other side of the door from the Ledger's Rest page (build_area_seeds.py VESTRY_*).

Hook in build_area.py's gameplay(), after the seeds:
    importlib.reload(importlib.import_module('build_area_secrets')).place(self)
On its own, in the open editor (clears only Gameplay/Secrets, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_secrets.py RansomsRest"
"""
import importlib
import json
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Secrets'
SECRET_TAG = 'Secret'
# AChest's kinds as Python names them, and each one's footprint's half sizes (cm, along X and Y: build_area_loot's).
CHEST_KINDS = {'Footlocker': 'FOOTLOCKER', 'Strongbox': 'STRONGBOX', 'SupplyCrate': 'SUPPLY_CRATE'}
CHEST_HALF = {'Footlocker': (25.0, 42.0), 'Strongbox': (31.0, 42.0), 'SupplyCrate': (45.0, 60.0)}
CHEST_HEIGHT = 60.0
# A footprint is level when every corner's floor is within this of its middle's (cm).
LEVEL = 18.0
# A spot is good when its ground is within this of the anchor's (cm) and the chest's box, lifted this much off it, meets
# nothing solid.
MAX_RISE = 200.0
LIFT = 6.0
# How a ring is searched: these many bearings, and its radius in these many steps.
BEARINGS = 16
RADIUS_STEPS = 4
# The readable beside its chest: this far to the chest's right (cm), facing as the chest does.
READABLE_ASIDE = 70.0
READ_REACH = 250.0
# Traces for the ground: from this high to this low over the anchor (cm).
TRACE_TOP = 30000.0
TRACE_BOTTOM = -30000.0
# Coffin Rock's way up (layout features coffinRock: an 8 m butte, cliff kit 2.5 m wide all round): steps from beyond the
# cliff kit at its north edge, each STEP_SPACING further out and STEP_RISE lower than the one before it.
STEP_MESH = 'SM_Boulder_A'
STEP_COUNT = 6
STEP_RISE = 125.0
STEP_SPACING = 200.0
STEP_OUT = 250.0
# How far over a step's wanted top its highest point stands (its rounded crown; the walkable top is lower).
STEP_CROWN = 45.0
STEP_SIZE = 0.75        # the boulder's footprint (3.0 x 2.4 m) at this share
STEP_MESH_HEIGHT = 165.0
STEP_SINK = 30.0
# Aldana's page on the vestry step: the chapel's SOCKET_Speaker frame, out and to the door's left (the Ledger page lies
# 40 cm to its right).
VESTRY_OUT = 45.0
VESTRY_SIDE = -45.0
PAGE_FALLBACK = 'SM_DeaconsSlip'


def texts():
    return importlib.reload(importlib.import_module('story_texts'))


def valley():
    return importlib.reload(importlib.import_module('build_area_missionprops_valley'))


def world():
    return unreal.EditorLevelLibrary.get_editor_world()


def trace_hit(x, y, top=TRACE_TOP, bottom=TRACE_BOTTOM):
    """The first thing a line straight down meets at (x, y): (its height, its actor), or None."""
    hit = unreal.SystemLibrary.line_trace_single(world(), unreal.Vector(x, y, top), unreal.Vector(x, y, bottom),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        return None
    t = hit.to_tuple()
    return t[5].z, t[9]


def trace_down(x, y, top=TRACE_TOP, bottom=TRACE_BOTTOM):
    """The height of the first thing a line straight down meets at (x, y), or None."""
    hit = trace_hit(x, y, top, bottom)
    return None if hit is None else hit[0]


def is_floor(actor, floors):
    """What a chest may stand on: the terrain (tagged Ground), or one of floors (a ruin's own floor)."""
    return actor is not None and (unreal.Name('Ground') in actor.tags or actor in floors)


# The PCG scatter's actor (and any like it) spans the whole area: its bounds overlap everything, so it never counts; a
# tree or rock of it under the chest's corners is caught by their traces instead.
WIDE_ACTORS = ('Scatter',)


def is_clear(x, y, z, kind, yaw, floors=()):
    """The chest's footprint is floor (its corners and edges traced down: each on the terrain or a floor in floors, within
    LEVEL of its middle) and nothing else solid stands in its box."""
    hx, hy = CHEST_HALF[kind]
    a = math.radians(yaw)
    fx, fy, rx, ry = math.cos(a), math.sin(a), -math.sin(a), math.cos(a)
    for u, v in ((0, 0), (1, 1), (1, -1), (-1, 1), (-1, -1), (1, 0), (-1, 0), (0, 1), (0, -1)):
        px, py = x + (fx * u * hx + rx * v * hy), y + (fy * u * hx + ry * v * hy)
        hit = trace_hit(px, py, z + 250.0, z - 250.0)
        if hit is None or not is_floor(hit[1], floors) or abs(hit[0] - z) > LEVEL:
            return False
    box = unreal.Vector(hx + 10.0, hy + 10.0, CHEST_HEIGHT * 0.5)
    middle = unreal.Vector(x, y, z + LIFT + LEVEL + CHEST_HEIGHT * 0.5)
    types = [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2]
    try:
        found = unreal.SystemLibrary.box_overlap_actors(world(), middle, box, types, None, [])
    except Exception:
        return True
    hits = found[1] if isinstance(found, tuple) else found
    return not [each for each in (hits or []) if not is_floor(each, floors)
                and not any(word in str(each.get_actor_label()) for word in WIDE_ACTORS)]


def anchor_of(build, spec, found):
    """The anchor's middle (x, y, ground z), or None (warned)."""
    what, name = spec['anchor']
    if what in ('obstacle', 'top'):
        section = 'obstacles' if what == 'obstacle' else 'features'
        entry = story.layout_entry(build, section, name)
        if entry is None or not entry.get('polygon'):
            return None
        x, y = story.centroid(entry['polygon'])
    elif what in ('actor', 'ledge'):
        actor = found.get(name)
        if actor is None:
            build.warn(f'no {name} placed: its secret is left out')
            return None
        at = actor.get_actor_location()
        x, y = at.x, at.y
        if what == 'ledge':
            return x, y, at.z
    else:
        build.warn(f'an anchor kind {what} isn\'t known: left out')
        return None
    z = trace_down(x, y)
    if z is None:
        build.warn(f'no ground under {name}: its secret is left out')
        return None
    return x, y, z


def floors_of(spec, found):
    """The ruins a secret may stand inside (spec 'inside': its anchor's own actor, by label)."""
    actor = found.get(spec['anchor'][1]) if spec.get('inside') else None
    return (actor,) if actor is not None else ()


def ring_spot(build, spec, middle, floors=()):
    """The first good spot on the anchor's ring, the preferred side first: (x, y, z, yaw facing the middle), or None. Its
    floor is the terrain (or a ruin's in floors) under the spot, whatever the anchor's own top is."""
    x0, y0, z0 = middle
    inner, outer = spec.get('ring', (0.0, 200.0))
    px, py = spec.get('prefer', (0.0, 0.0))
    preferred = math.atan2(py, px) if (px or py) else 0.0
    bearings = sorted((preferred + 2.0 * math.pi * k / BEARINGS for k in range(BEARINGS)),
                      key=lambda a: abs(math.atan2(math.sin(a - preferred), math.cos(a - preferred))))
    for step in range(RADIUS_STEPS + 1):
        radius = inner + (outer - inner) * step / RADIUS_STEPS
        for angle in bearings if radius > 0.0 else [preferred]:
            x, y = x0 + math.cos(angle) * radius, y0 + math.sin(angle) * radius
            hit = trace_hit(x, y)
            if hit is None or not is_floor(hit[1], floors):
                continue
            z = hit[0]
            yaw = math.degrees(math.atan2(y0 - y, x0 - x)) if radius > 0.0 else 0.0
            if is_clear(x, y, z, spec['kind'], yaw, floors):
                return x, y, z, yaw
    return None


def ledge_spot(build, spec, middle):
    """On the Mooring Ledge by the gang's post, a little way from the seeds' page and letter."""
    seeds = importlib.reload(importlib.import_module('build_area_seeds'))
    x0, y0, z0 = middle
    lx, ly = seeds.ledge_middle(build)
    dx, dy = lx - x0, ly - y0
    length = math.hypot(dx, dy) or 1.0
    toward = (dx / length, dy / length)
    spot = seeds.on_ledge(build, unreal.Vector(x0, y0, z0), toward, 260.0, -40.0)
    if spot is None:
        return None
    return spot.x, spot.y, spot.z, math.degrees(math.atan2(-toward[1], -toward[0]))


def top_spot(build, spec, middle):
    """On Coffin Rock's top, by its middle (its dead tree stands there too: a step off it)."""
    x0, y0, z0 = middle
    for dx, dy in ((250.0, 150.0), (250.0, -150.0), (-200.0, 200.0), (-200.0, -200.0), (0.0, 300.0), (150.0, 0.0),
                   (-150.0, 0.0), (0.0, -250.0), (100.0, 100.0)):
        z = trace_down(x0 + dx, y0 + dy)
        yaw = math.degrees(math.atan2(-dy, -dx))
        if z is not None and z > z0 - 150.0 and is_clear(x0 + dx, y0 + dy, z, spec['kind'], yaw):
            return x0 + dx, y0 + dy, z, yaw
    return None


def place_chest(build, spec, x, y, z, yaw):
    cls = story.actor_class('Chest')
    if cls is None:
        build.warn('no Chest class (build the game module first): no secrets')
        return None
    kind = spec['kind']
    chest = build.place(cls, (x, y, z), yaw, label=spec['chest_id'], folder=FOLDER,
                        tags=('Chest', kind, 'Obstacle', SECRET_TAG))
    chest.set_editor_property('kind', getattr(unreal.ChestKind, CHEST_KINDS[kind]))
    if spec.get('loot'):
        loot = unreal.ChestLootOverride()
        loot.set_editor_property('override', True)
        for name, value in spec['loot'].items():
            loot.set_editor_property(name, value)
        chest.set_editor_property('loot_override', loot)
    # Set last, so the session finds it by its id.
    chest.set_editor_property('chest_id', unreal.Name(spec['chest_id']))
    return chest


def find_mesh(name):
    index = valley().mesh_index()
    path = index.get(name[3:] if name.startswith('SM_') else name)
    return unreal.load_asset(path) if path else None


def readable(build, readable_id, x, y, z, yaw):
    cls = story.actor_class('ReadableProp')
    words = texts()
    if cls is None or readable_id not in words.READABLES:
        build.warn(f'no ReadableProp class or no words for {readable_id}: left out')
        return None
    prop = build.place(cls, (x, y, z), yaw, label=f'Readable_{readable_id}', folder=FOLDER,
                       tags=('Readable', f'Readable_{readable_id}', SECRET_TAG))
    words.apply_readable(prop, readable_id)
    prop.set_editor_property('reach', READ_REACH)
    model_name = words.READABLE_MODELS.get(readable_id)
    model = find_mesh(model_name) if model_name else None
    if model_name and model is None:
        model = find_mesh(PAGE_FALLBACK)
    if model is not None:
        prop.get_editor_property('mesh').set_static_mesh(model)
    return prop


def place_steps(build, spec):
    """Coffin Rock's way up its north edge: boulders, each STEP_RISE over the last, the last at the top's edge."""
    entry = story.layout_entry(build, 'features', spec['anchor'][1])
    step_mesh = find_mesh(STEP_MESH)
    if entry is None or step_mesh is None:
        build.warn(f'no {spec["anchor"][1]} feature or no {STEP_MESH}: no way up (its secret waits on top regardless)')
        return
    direction, count, rise = spec['steps']
    # The edge's point furthest that way (layout x north).
    edge = max(entry['polygon'], key=lambda p: p[0]) if direction == 'north' else max(entry['polygon'], key=lambda p: p[1])
    out = (1.0, 0.0) if direction == 'north' else (0.0, 1.0)
    top = trace_down(*story.centroid(entry['polygon']))
    if top is None:
        return
    # The boulder's own box (unscaled): its long side along X, its walkable top a little under its highest point.
    box = step_mesh.get_bounding_box()
    low, high = box.get_editor_property('min'), box.get_editor_property('max')
    height_of = max(high.z - low.z, 1.0)
    placed = 0
    for k in range(count):
        # From the top's edge outward, each overlapping the next a little (no gap to fall through): the k-th step's top
        # is (k + 1) rises under the butte's top, its long side pointing at the butte.
        reach = STEP_OUT + STEP_SPACING * k
        x, y = edge[0] + out[0] * reach, edge[1] + out[1] * reach
        ground = trace_down(x, y, top - 50.0, top - 3000.0)
        if ground is None:
            continue
        wanted = top - rise * (k + 1) + STEP_CROWN
        if wanted <= ground + 20.0:
            break
        scale_z = (wanted - ground + STEP_SINK) / height_of
        z = ground - STEP_SINK - low.z * scale_z
        build.place(step_mesh, (x, y, z), 0.0 if direction == 'north' else 90.0, label=f'Secret_CoffinRockStep_{k + 1}',
                    folder=FOLDER, scale=(STEP_SIZE, STEP_SIZE, scale_z), tags=('Obstacle', SECRET_TAG))
        placed += 1
    build.log(f'Coffin Rock\'s way up: {placed} steps of {rise:.0f} cm up its {direction} edge (walk it in PIE)')


def place_aldana_page(build, found):
    kit = valley()
    chapel = found.get('chapel')
    door = kit.socket(chapel, 'Speaker') if chapel is not None else None
    if door is None:
        build.warn('no chapel with its SOCKET_Speaker: no AldanaPage (What Ada gave up)')
        return
    turn = door.rotation.rotator()
    yaw = math.radians(turn.yaw)
    out, side = (math.cos(yaw), math.sin(yaw)), (-math.sin(yaw), math.cos(yaw))
    x = door.translation.x + out[0] * VESTRY_OUT + side[0] * VESTRY_SIDE
    y = door.translation.y + out[1] * VESTRY_OUT + side[1] * VESTRY_SIDE
    hit = story.onto_host(chapel, unreal.Vector(x, y, door.translation.z - 20.0), unreal.Vector(x, y, door.translation.z - 260.0))
    z = hit.z if hit is not None else door.translation.z - 160.0
    readable(build, 'AldanaPage', x, y, z, turn.yaw - 150.0)


def place(build):
    """Every secret (story_texts.SECRETS) and Aldana's page. Ransom's Rest only."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    words = texts()
    found = valley().level_actors(build)
    placed = 0
    for secret_id, spec in getattr(words, 'SECRETS', {}).items():
        middle = anchor_of(build, spec, found)
        if middle is None:
            continue
        what = spec['anchor'][0]
        if what == 'ledge':
            spot = ledge_spot(build, spec, middle)
        elif what == 'top':
            spot = top_spot(build, spec, middle)
        else:
            spot = ring_spot(build, spec, middle, floors_of(spec, found))
        if spot is None:
            build.warn(f'{secret_id}: no level, clear spot round {spec["anchor"][1]}: left out')
            continue
        x, y, z, yaw = spot
        if place_chest(build, spec, x, y, z, yaw) is None:
            return
        if spec.get('readable'):
            a = math.radians(yaw)
            rx, ry = x - math.sin(a) * READABLE_ASIDE, y + math.cos(a) * READABLE_ASIDE
            rz = trace_down(rx, ry, z + 200.0, z - 200.0)
            readable(build, spec['readable'], rx, ry, z if rz is None else rz, yaw)
        if spec.get('steps'):
            place_steps(build, spec)
        placed += 1
        build.log(f'{secret_id}: {spec["kind"]} {spec["chest_id"]} at ({x:.0f}, {y:.0f}, {z:.0f})')
    place_aldana_page(build, found)
    build.log(f'things to find off the path: {placed} of {len(getattr(words, "SECRETS", {}))} placed')


def run(name):
    """Places Gameplay/Secrets again on its own, saves."""
    import build_area
    build = importlib.reload(build_area).AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the secrets placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_secrets.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
