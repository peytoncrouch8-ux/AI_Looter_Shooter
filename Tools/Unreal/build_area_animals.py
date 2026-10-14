"""The small animals (the "make it alive" polish, look sheet 44; Source/AI_Looter_Shooter/World/ChickenYard, BarnCat,
CliffSwallowFlock): the chicken yards and the porch cat from the town's spots (town.json, written by the area's
make_town.py), and the cliff swallows at the nests the cliff life placed (build_area_cliff_life.py's SM_SwallowNests).

    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_animals.py RansomsRest" -Until "ANIMALS done"

places only these again (the area's Animals folder cleared first; the level is saved). build_area.py's full build can call
place(build) after the fauna (it reads the dressing's instances and the cliff life's nests, so it goes after both).
Everything it places is tagged with the area's build tag and Animals and sits in <area>/Animals, so a rebuild replaces
only what this placed. Run it again after build_area_cliff_life.py or a town rebuild: it reads what they placed.

- Chickens (spots of kind "chickens": the picket yard's inside 40 cm in from its fence, its coop's box as "avoid"): one
  AChickenYard per spot, at its middle on the ground. Its pens: the yard less the coop's box (where the coop stands; if
  the coop isn't placed yet the whole yard), and, when the coop stands, the coop's own lath run (from SM_ChickenCoop's
  SOCKET_Run: the run's middle; the run 1.24 x 2.02 m inside its laths, Art/Models/Props/FrontierClutter.py's
  chicken_coop, its ramp walked round). The spot's count of hens (4) and a rooster go to the pens by their room (about
  half a square metre a bird, no more than there's room for): the rooster with the most hens.
- The porch cat (spots of kind "cat": its spot on the porch, "lift" cm over the ground, its "yaw"): one ABarnCat, its
  porch found by traces. The deck: the first surface a trace down from over the spot meets. Its way off the porch: in 24
  directions, stepping out 5 cm at a time (and stopping at a wall), the first point where the deck falls away 25 cm or
  more; the nearest such edge is its exit (12 cm back from the edge), the ground 30 cm past it where it lands, and its
  hiding place 22 cm under the deck behind the edge, on that ground (the dark under the porch it slips into). The
  stretch of deck it walks: square to the exit, as far each way (up to 5 m) as the deck stays level and nothing stands
  in the way, 30 cm short of either end. A spot's own "hide": [x, y] (world) overrides the hiding place (and "exit":
  [x, y] the exit) where the traces get a porch wrong.
- Swallows: the placed nest clusters (CliffLife_SwallowNests' instances, SM_SwallowNests) grouped (clusters within
  GROUP_REACH of each other), one ACliffSwallowFlock per group, BIRDS_PER_NEST a cluster.

Every choice is seeded and sorted, so a rebuild places the same. place() needs the editor.
"""
import json
import math
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.append(HERE)
import build_area  # noqa: E402
import build_area_dressing  # noqa: E402

CLASSES = '/Script/AI_Looter_Shooter.'
FOLDER = 'Animals'
TAG = 'Animals'
CREATURES = '/Game/Art/Creatures'

# The yard: a bird's room in a pen (cm, as AChickenYard's BodyRoom) and the room each bird is given (m2).
BODY_ROOM = 12.0
ROOM_PER_BIRD = 0.5
# The coop's run (FrontierClutter.py's chicken_coop: inside RX0..RX1 x RY0..Y0, its ramp from the pop door at X0 + 0.25
# down 0.82 m into the run), in the coop's frame about SOCKET_Run (cm; Unreal X = Blender -Y, Y = Blender -X): half its
# depth and width 3 cm inside the laths, and its ramp's middle and half size.
RUN_HALF = (62.0, 101.0)
RAMP_CENTRE = (-26.0, -22.5)
RAMP_HALF = (41.0, 13.0)
# How near a placed coop must stand to the spot's coop box (cm) to be its.
COOP_MATCH = 200.0

# The cat's porch: how far it looks for the deck's edge and along the deck (cm), and its steps.
EDGE_REACH = 400.0
EDGE_STEP = 5.0
EDGE_DROP = 25.0
WALK_REACH = 500.0
WALK_STEP = 10.0
WALK_LEVEL = 6.0
DIRECTIONS = 24

# Swallows.
GROUP_REACH = 4000.0
BIRDS_PER_NEST = 3
SWALLOW_PIECES = dict(perched_mesh='CliffSwallow_Perched', head_mesh='CliffSwallow_Head', flying_mesh='CliffSwallow_Flying',
                      wing_inner_mesh_l='CliffSwallow_WingInnerL', wing_inner_mesh_r='CliffSwallow_WingInnerR',
                      wing_outer_mesh_l='CliffSwallow_WingOuterL', wing_outer_mesh_r='CliffSwallow_WingOuterR')


def vec(v):
    return unreal.Vector(float(v[0]), float(v[1]), float(v[2]))


def struct(cls_name, **values):
    made = getattr(unreal, cls_name)()
    for key, value in values.items():
        made.set_editor_property(key, value)
    return made


# ---------------------------------------------------------------------------------------------------------------------
# Pens (plain Python: the same rules as World/ChickenYardRules.cpp)
# ---------------------------------------------------------------------------------------------------------------------

def in_block(block, p, margin):
    (cx, cy), (hx, hy), yaw = block
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    dx, dy = p[0] - cx, p[1] - cy
    lx, ly = dx * c + dy * s, -dx * s + dy * c
    return abs(lx) < hx + margin and abs(ly) < hy + margin


def segment_distance(p, a, b):
    abx, aby = b[0] - a[0], b[1] - a[1]
    length2 = abx * abx + aby * aby
    t = 0.0 if length2 < 1e-9 else max(0.0, min(1.0, ((p[0] - a[0]) * abx + (p[1] - a[1]) * aby) / length2))
    return math.hypot(p[0] - (a[0] + abx * t), p[1] - (a[1] + aby * t))


def pen_inside(outline, blocks, p, margin):
    if not build_area.inside(p, outline):
        return False
    if any(segment_distance(p, outline[i - 1], outline[i]) < margin for i in range(len(outline))):
        return False
    return not any(in_block(b, p, margin) for b in blocks)


def pen_area(outline, blocks, step=10.0):
    """The pen's room for birds' feet (m2): the part of it inside with a body's room to spare."""
    xs, ys = [p[0] for p in outline], [p[1] for p in outline]
    count = 0
    x = min(xs) + step * 0.5
    while x < max(xs):
        y = min(ys) + step * 0.5
        while y < max(ys):
            count += pen_inside(outline, blocks, (x, y), BODY_ROOM)
            y += step
        x += step
    return count * step * step / 10000.0


def box_of(corners):
    """A box from its four corners in order: (centre, half sizes, yaw)."""
    cx = sum(c[0] for c in corners) / len(corners)
    cy = sum(c[1] for c in corners) / len(corners)
    ax, ay = corners[1][0] - corners[0][0], corners[1][1] - corners[0][1]
    bx, by = corners[3][0] - corners[0][0], corners[3][1] - corners[0][1]
    return (cx, cy), (math.hypot(ax, ay) * 0.5, math.hypot(bx, by) * 0.5), math.degrees(math.atan2(ay, ax))


def turned(local, origin, yaw):
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return origin[0] + local[0] * c - local[1] * s, origin[1] + local[0] * s + local[1] * c


def share_birds(pens, hens, log, name):
    """Hens and the rooster to the pens by their room: (hens, rooster) per pen, as many as they have room for."""
    rooms = [max(0.0, area) for _, _, area in pens]
    capacity = [int(r / ROOM_PER_BIRD) for r in rooms]
    birds = [0] * len(pens)
    wanted = hens + 1
    # One at a time to the pen with the most room left per bird (each a bird at least, where there's room for one).
    for _ in range(wanted):
        best, best_room = None, 0.0
        for k, room in enumerate(rooms):
            if birds[k] >= capacity[k]:
                continue
            left = room / (birds[k] + 1)
            if left > best_room:
                best, best_room = k, left
        if best is None:
            break
        birds[best] += 1
    placed = sum(birds)
    if placed < wanted:
        log(f'animals: {name}: room for {placed} of {wanted} birds only ({", ".join(f"{r:.1f} m2" for r in rooms)})')
    if placed == 0:
        return []
    # The rooster with the most hens (the roomier pen of two as many).
    rooster = max(range(len(pens)), key=lambda k: (birds[k], rooms[k], -k))
    return [(birds[k] - (1 if k == rooster else 0), k == rooster) for k in range(len(pens))]


# ---------------------------------------------------------------------------------------------------------------------
# The level: traces, the dressing's coops, the cliff life's nests
# ---------------------------------------------------------------------------------------------------------------------

def world():
    return unreal.EditorLevelLibrary.get_editor_world()


def volumes():
    return [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a, unreal.Volume)]


def trace(start, end, ignore):
    """Where a line first meets something solid (visibility): (point, normal) or None."""
    hit = unreal.SystemLibrary.line_trace_single(world(), vec(start), vec(end), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True,
                                                 ignore, unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        return None
    parts = hit.to_tuple()
    if not parts[0]:
        return None
    return parts[5], parts[7]


def instances_of(mesh_name):
    """Every placed instance of SM_<mesh_name> in the level's instanced props: (transform, mesh asset)."""
    found = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if actor.get_class().get_name() != 'InstancedProps':
            continue
        component = actor.get_editor_property('instances')
        mesh = component.static_mesh if component is not None else None
        if mesh is None or mesh.get_name() != 'SM_' + mesh_name:
            continue
        for i in range(component.get_instance_count()):
            found.append((component.get_instance_transform(i, True), mesh))
    return found


def skeletal(name):
    path = f'{CREATURES}/{name}'
    return unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None


def static(name, meshes, missing):
    if name not in meshes:
        missing.add(name)
        return None
    return unreal.load_asset(meshes[name])


# ---------------------------------------------------------------------------------------------------------------------
# Chickens
# ---------------------------------------------------------------------------------------------------------------------

def coop_for(spot, coops):
    """The placed coop standing in this yard (nearest the spot's coop box), or None."""
    corners = spot.get('avoid') or []
    centre = (sum(c[0] for c in corners) / len(corners), sum(c[1] for c in corners) / len(corners)) if corners else tuple(spot['at'])
    best, best_d = None, COOP_MATCH
    for transform, mesh in coops:
        loc = transform.translation
        d = math.hypot(loc.x - centre[0], loc.y - centre[1])
        if d < best_d:
            best, best_d = (transform, mesh), d
    return best


def yard_pens(spot, coops, log):
    """The yard's pens: (outline, blocks, room m2) for the picket yard (less the coop) and the coop's run."""
    outline = [tuple(p[:2]) for p in spot['polygon']]
    coop = coop_for(spot, coops)
    pens = []
    if coop is None:
        log(f'animals: {spot["id"]}: no coop placed in it yet: the whole yard is theirs')
        pens.append((outline, [], pen_area(outline, [])))
        return pens
    blocks = [box_of([tuple(c[:2]) for c in spot['avoid']])] if spot.get('avoid') else []
    pens.append((outline, blocks, pen_area(outline, blocks)))
    transform, mesh = coop
    socket = mesh.find_socket('Run')
    if socket is None:
        log(f'animals: {spot["id"]}: its coop has no SOCKET_Run (reimport ChickenCoop): the run stays empty')
        return pens
    middle = transform.transform_location(socket.get_editor_property('relative_location'))
    yaw = transform.rotation.rotator().yaw
    centre = (middle.x, middle.y)
    hx, hy = RUN_HALF
    run = [turned(c, centre, yaw) for c in ((-hx, -hy), (hx, -hy), (hx, hy), (-hx, hy))]
    ramp = (turned(RAMP_CENTRE, centre, yaw), RAMP_HALF, yaw)
    pens.append((run, [ramp], pen_area(run, [ramp])))
    log(f'animals: {spot["id"]}: the yard round the coop ({pens[0][2]:.1f} m2 for feet) and its run ({pens[1][2]:.1f} m2), '
        f'the run\'s middle at ({middle.x:.0f}, {middle.y:.0f}) turned {yaw:.0f}')
    return pens


def place_yard(build, cls, spot, coops, ground, seed, log):
    pens = yard_pens(spot, coops, log)
    shares = share_birds(pens, int(spot.get('count', 4)), log, spot['id'])
    if not shares:
        build.warn(f'animals: {spot["id"]}: no room for a bird in its pens: no chickens')
        return 0
    x, y = spot['at']
    hit = ground(x, y)
    z = hit[0].z if hit else 0.0
    actor = build.place(cls, (x, y, z), label=f'Animals_{spot["id"]}', folder=FOLDER, tags=(TAG,))
    made = []
    for (outline, blocks, _), (hens, rooster) in zip(pens, shares):
        if hens <= 0 and not rooster:
            continue
        made.append(struct('ChickenPen',
                           outline=[unreal.Vector2D(float(p[0]), float(p[1])) for p in outline],
                           blocks=[struct('ChickenPenBlock', center=unreal.Vector2D(float(c[0]), float(c[1])),
                                          half_size=unreal.Vector2D(float(h[0]), float(h[1])), yaw=float(yaw))
                                   for c, h, yaw in blocks],
                           hens=int(hens), rooster=bool(rooster)))
    actor.set_editor_property('pens', made)
    actor.set_editor_property('seed', seed)
    for prop, name in (('hen_brown_mesh', 'SK_HenBrown'), ('hen_white_mesh', 'SK_HenWhite'), ('rooster_mesh', 'SK_Rooster')):
        asset = skeletal(name)
        if asset is not None:
            actor.set_editor_property(prop, asset)
        else:
            build.warn(f'animals: no {CREATURES}/{name} yet (import Chicken.py): {spot["id"]} draws without it')
    birds = sum(h + (1 if r else 0) for h, r in shares)
    log(f'animals: {spot["id"]}: {birds} chickens in {len(made)} pens ('
        + ', '.join(f'{h} hens' + (' and the rooster' if r else '') for h, r in shares) + ')')
    return birds


# ---------------------------------------------------------------------------------------------------------------------
# The cat
# ---------------------------------------------------------------------------------------------------------------------

def find_porch(spot, ground, ignore, log):
    """The cat's porch by traces: (loaf, exit, step, hide, end a, end b), world points."""
    x, y = spot['at']
    lift = float(spot.get('lift', 50.0))
    yaw = float(spot.get('yaw', 0.0))
    hit = ground(x, y)
    gz = hit[0].z if hit else 0.0
    deck = trace((x, y, gz + lift + 150.0), (x, y, gz - 50.0), ignore)
    if deck is None or deck[0].z < gz + lift * 0.5:
        log(f'animals: {spot["id"]}: no deck found under its spot: it loafs {lift:.0f} cm over the ground')
        deck_z = gz + lift
    else:
        deck_z = deck[0].z
    loaf = (x, y, deck_z)

    def edge_along(dx, dy):
        """How far out the deck ends this way (and the ground there), or None (a wall first, or no edge in reach)."""
        wall = trace((x, y, deck_z + 15.0), (x + dx * EDGE_REACH, y + dy * EDGE_REACH, deck_z + 15.0), ignore)
        reach = EDGE_REACH if wall is None else math.hypot(wall[0].x - x, wall[0].y - y) - 10.0
        s = EDGE_STEP
        while s <= reach:
            px, py = x + dx * s, y + dy * s
            down = trace((px, py, deck_z + 40.0), (px, py, deck_z - 200.0), ignore)
            if down is None or down[0].z < deck_z - EDGE_DROP:
                past = trace((px + dx * 30.0, py + dy * 30.0, deck_z + 40.0), (px + dx * 30.0, py + dy * 30.0, deck_z - 300.0), ignore)
                floor = past[0].z if past else (down[0].z if down else gz)
                return s, floor
            s += EDGE_STEP
        return None

    best = None
    for k in range(DIRECTIONS):
        angle = math.radians(yaw + k * 360.0 / DIRECTIONS)
        dx, dy = math.cos(angle), math.sin(angle)
        found = edge_along(dx, dy)
        if found and (best is None or found[0] < best[0]):
            best = (found[0], found[1], dx, dy)
    if best is None:
        log(f'animals: {spot["id"]}: no edge to its porch found within {EDGE_REACH / 100.0:.0f} m: it jumps off in front')
        angle = math.radians(yaw)
        best = (60.0, deck_z - lift, math.cos(angle), math.sin(angle))
    s, floor, dx, dy = best
    exit_point = (x + dx * (s - 12.0), y + dy * (s - 12.0), deck_z)
    step = (x + dx * (s + 30.0), y + dy * (s + 30.0), floor)
    hide = (x + dx * (s - 22.0), y + dy * (s - 22.0), floor)
    if spot.get('exit'):
        ex, ey = spot['exit'][:2]
        exit_point = (ex, ey, deck_z)
    if spot.get('hide'):
        hx, hy = spot['hide'][:2]
        hide = (hx, hy, floor)

    # The deck it walks: square to the way off it, each way as far as it stays level and clear.
    px, py = -dy, dx
    ends = []
    for sign in (-1.0, 1.0):
        last, t = 0.0, WALK_STEP
        prev = (x, y)
        while t <= WALK_REACH:
            qx, qy = x + px * sign * t, y + py * sign * t
            down = trace((qx, qy, deck_z + 40.0), (qx, qy, deck_z - 60.0), ignore)
            blocked = trace((prev[0], prev[1], deck_z + 15.0), (qx, qy, deck_z + 15.0), ignore)
            if down is None or abs(down[0].z - deck_z) > WALK_LEVEL or blocked is not None:
                break
            last, prev, t = t, (qx, qy), t + WALK_STEP
        reach = max(0.0, last - 30.0)
        ends.append((x + px * sign * reach, y + py * sign * reach, deck_z))
    log(f'animals: {spot["id"]}: deck at {deck_z:.0f} ({deck_z - gz:.0f} over the ground), its edge {s:.0f} cm off '
        f'toward {math.degrees(math.atan2(dy, dx)):.0f} degrees, the ground there {deck_z - floor:.0f} cm down, the deck walked '
        f'{math.dist(ends[0][:2], (x, y)):.0f} and {math.dist(ends[1][:2], (x, y)):.0f} cm either way')
    return loaf, exit_point, step, hide, ends[0], ends[1]


def place_cat(build, cls, spot, ground, ignore, seed, log):
    loaf, exit_point, step, hide, end_a, end_b = find_porch(spot, ground, ignore, log)
    yaw = float(spot.get('yaw', 0.0))
    actor = build.place(cls, loaf, yaw, label=f'Animals_{spot["id"]}', folder=FOLDER, tags=(TAG,))
    for prop, value in (('loaf_spot', loaf), ('porch_exit', exit_point), ('porch_step', step), ('hide_point', hide),
                        ('porch_a', end_a), ('porch_b', end_b)):
        actor.set_editor_property(prop, vec(value))
    actor.set_editor_property('loaf_yaw', yaw)
    actor.set_editor_property('seed', seed)
    if skeletal('SK_BarnCat') is None:
        build.warn(f'animals: no {CREATURES}/SK_BarnCat yet (import Cat.py): {spot["id"]} draws nothing until it is')
    return 1


# ---------------------------------------------------------------------------------------------------------------------
# Swallows
# ---------------------------------------------------------------------------------------------------------------------

def group_nests(transforms):
    """The nest clusters in groups: each joined to any within GROUP_REACH (sorted, so a rebuild groups the same)."""
    points = sorted(transforms, key=lambda t: (round(t.translation.x), round(t.translation.y), round(t.translation.z)))
    groups = []
    for t in points:
        near = [g for g in groups if any(math.dist((t.translation.x, t.translation.y), (o.translation.x, o.translation.y))
                                         < GROUP_REACH for o in g)]
        merged = [t]
        for g in near:
            merged.extend(g)
            groups.remove(g)
        groups.append(merged)
    return groups


def place_swallows(build, cls, meshes, seed, log):
    nests = instances_of('SwallowNests')
    if not nests:
        log('animals: no SM_SwallowNests placed (build_area_cliff_life.py places them): no swallows')
        return 0
    missing = set()
    nest_mesh = nests[0][1]
    pieces = {prop: static(name, meshes, missing) for prop, name in SWALLOW_PIECES.items()}
    if missing:
        build.warn(f'animals: no SM_{", SM_".join(sorted(missing))} yet (import Swallow.py): those pieces are not drawn')
    birds = 0
    for n, group in enumerate(group_nests([t for t, _ in nests])):
        cx = sum(t.translation.x for t in group) / len(group)
        cy = sum(t.translation.y for t in group) / len(group)
        cz = sum(t.translation.z for t in group) / len(group)
        actor = build.place(cls, (cx, cy, cz), label=f'Animals_Swallows_{n + 1:02d}', folder=FOLDER, tags=(TAG,))
        actor.set_editor_property('nests', group)
        actor.set_editor_property('nest_mesh', nest_mesh)
        actor.set_editor_property('birds_per_nest', BIRDS_PER_NEST)
        actor.set_editor_property('seed', seed + 31 * n)
        for prop, mesh in pieces.items():
            if mesh is not None:
                actor.set_editor_property(prop, mesh)
        birds += BIRDS_PER_NEST * len(group)
        log(f'animals: swallows {n + 1}: {len(group)} nest clusters about ({cx:.0f}, {cy:.0f}, {cz:.0f})')
    return birds


# ---------------------------------------------------------------------------------------------------------------------
# The run
# ---------------------------------------------------------------------------------------------------------------------

def place(build):
    """The area's small animals (the module's docstring), in its Animals folder."""
    log = build.log
    yard_cls = unreal.load_class(None, CLASSES + 'ChickenYard')
    cat_cls = unreal.load_class(None, CLASSES + 'BarnCat')
    swallow_cls = unreal.load_class(None, CLASSES + 'CliffSwallowFlock')
    if yard_cls is None or cat_cls is None or swallow_cls is None:
        build.warn('animals: no ChickenYard, BarnCat or CliffSwallowFlock class (build the game module first): none placed')
        return
    town, problem = build_area_dressing.load_town(build.source)
    if problem:
        build.warn(f'animals: {problem}')
    spots = sorted((town or {}).get('spots', []), key=lambda s: s.get('id', ''))
    seed = sum(ord(c) for c in build.name)
    ground = build_area_fauna_ground(build)
    ignore = volumes()
    coops = instances_of('ChickenCoop')
    chickens = cats = 0
    for k, spot in enumerate(spots):
        if spot.get('deferred'):
            # The animals are approved (look sheet 44); the town keeps its spots pending until its own approval.
            log(f'animals: {spot["id"]}: its spot is still marked pending in town.json: placed all the same')
        if spot.get('kind') == 'chickens' and spot.get('polygon'):
            chickens += place_yard(build, yard_cls, spot, coops, ground, seed + 101 * (k + 1), log)
        elif spot.get('kind') == 'cat':
            cats += place_cat(build, cat_cls, spot, ground, ignore, seed + 211 * (k + 1), log)
    swallows = place_swallows(build, swallow_cls, build_area.mesh_index(), seed + 401, log)
    log(f'animals: {chickens} chickens, {cats} cat(s), {swallows} swallows placed')


def build_area_fauna_ground(build):
    """The terrain under a point (its tiles alone), as the fauna's placer finds it."""
    import build_area_fauna  # noqa: E402 - the editor keeps modules; this one only reads
    return build_area_fauna.Ground(build)


def run(name):
    """Places only the small animals again: the area's Animals folder cleared, placed, the level saved."""
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    build_area.levels.save_current_level()
    build.log('animals placed and saved')
    unreal.log('ANIMALS done')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_animals.py <Area> (a folder under Art/Levels whose layout names a town with spots)')
    run(sys.argv[1])
