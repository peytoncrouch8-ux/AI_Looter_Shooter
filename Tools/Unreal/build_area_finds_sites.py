"""The new finds' builders for build_area_finds.py (Docs/Work/2026-10-28-Core/FINDS.md section 2, N1-N5; N6's spot is
build_area_finds.place_elite). Each takes (build, entry, found): the layout's gameplay.finds spot and the level's built
actors by label, probes the built level by traces (the terrain is tagged Ground), places its pieces, and returns
(its AFindSpot, its (x, y, z)), or None after a warning. The two graves (N4, N5) are build_area_finds_graves.py's. Every choice is logged (FINDS lines) so a rebuild can be checked
without opening the level. Not run on its own: build_area_finds.py calls it.
"""
import importlib
import math
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

ROCK_MATERIALS = ('/Game/Art/Materials/MI_RockCliff_Ransom', '/Game/Art/Materials/MI_RockCliff')
OXIDE_FALLBACK = '/Game/Art/Materials/MI_PaintOxide'
# A grave chest's footprint (cm, half sizes along X, the grave's length, and Y; Lootables.py loot_grave: spoil heaps either
# side, the spade at its foot) and its height; the level a chest's footprint needs (cm, every corner to its middle).
GRAVE_HALF = (110.0, 65.0)
GRAVE_HEIGHT = 50.0
LEVEL = 25.0
BEARINGS = 16


def core():
    return importlib.import_module('build_area_finds')


def secrets():
    return core().secrets()


def world():
    return unreal.EditorLevelLibrary.get_editor_world()


def log(build, find_id, message):
    build.log(f'FINDS {find_id}: {message}')


def hit_at(x, y, top, bottom):
    """(height, is it the terrain) of the first thing straight down at (x, y), or None."""
    hit = secrets().trace_hit(x, y, top, bottom)
    if hit is None:
        return None
    return hit[0], hit[1] is not None and unreal.Name('Ground') in hit[1].tags


def ground_at(x, y, top=30000.0, bottom=-30000.0):
    """The terrain's height under (x, y) when the terrain is the first thing there, else None."""
    hit = hit_at(x, y, top, bottom)
    return hit[0] if hit is not None and hit[1] else None


def unit(v):
    length = math.hypot(v[0], v[1]) or 1.0
    return v[0] / length, v[1] / length


def turned(v, degrees):
    a = math.radians(degrees)
    return v[0] * math.cos(a) - v[1] * math.sin(a), v[0] * math.sin(a) + v[1] * math.cos(a)


def yaw_of(v):
    return math.degrees(math.atan2(v[1], v[0]))


def frame(x, y, yaw, along_x, along_y):
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return x + c * along_x - s * along_y, y + s * along_x + c * along_y


def mesh_box(mesh):
    box = mesh.get_bounding_box()
    return box.get_editor_property('min'), box.get_editor_property('max')


def rock_material():
    return next((unreal.load_asset(m) for m in ROCK_MATERIALS if unreal.EditorAssetLibrary.does_asset_exist(m)), None)


def blocked(centre, half, floors=()):
    """The solid actors in an axis-aligned box (centre, half: x, y, z cm), not counting the terrain, floors and the scatter."""
    types = [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2]
    try:
        found = unreal.SystemLibrary.box_overlap_actors(world(), unreal.Vector(*centre), unreal.Vector(*half), types, None, [])
    except Exception:  # noqa: BLE001
        return []
    hits = found[1] if isinstance(found, tuple) else found
    wide = secrets().WIDE_ACTORS
    return [a for a in (hits or []) if not secrets().is_floor(a, floors)
            and not any(word in str(a.get_actor_label()) for word in wide)]


def footprint_ok(x, y, z, half, yaw, height, floors=(), level=LEVEL):
    """Every corner and edge of a footprint is the terrain (or a floor) within level of z, and its box above is clear."""
    for u, v in ((0, 0), (1, 1), (1, -1), (-1, 1), (-1, -1), (1, 0), (-1, 0), (0, 1), (0, -1)):
        px, py = frame(x, y, yaw, u * half[0], v * half[1])
        hit = secrets().trace_hit(px, py, z + 250.0, z - 250.0)
        if hit is None or not secrets().is_floor(hit[1], floors) or abs(hit[0] - z) > level:
            return False
    r = max(half) + 10.0
    return not blocked((x, y, z + level + 6.0 + height * 0.5), (r, r, height * 0.5), floors)


def chest_size(build, chest):
    """(half x, half y, height) of a find's chest: a tier's model's bounds, else a grave's."""
    if chest['kind'] != 'Tier':
        return GRAVE_HALF[0], GRAVE_HALF[1], GRAVE_HEIGHT
    tier = importlib.reload(importlib.import_module('chest_tiers')).TIERS.get(chest['id'], 3)
    mesh = core().find_mesh(f'SM_ChestTier{tier}')
    if mesh is None:
        return 45.0, 60.0, 70.0
    low, high = mesh_box(mesh)
    return max(abs(low.x), high.x), max(abs(low.y), high.y), high.z - low.z


def ring_search(x0, y0, ring, prefer_yaws, test):
    """The first (x, y, yaw) round (x0, y0) out to ring cm that test(x, y, yaw) passes (returning its z), nearest first."""
    inner, outer = ring
    for step in range(5):
        radius = inner + (outer - inner) * step / 4.0
        for k in range(BEARINGS if radius > 0.0 else 1):
            a = 2.0 * math.pi * k / BEARINGS
            x, y = x0 + math.cos(a) * radius, y0 + math.sin(a) * radius
            for yaw in prefer_yaws:
                z = test(x, y, yaw)
                if z is not None:
                    return x, y, z, yaw
    return None


# ---------------------------------------------------------------------------
# N1, the tarn's lockbox
# ---------------------------------------------------------------------------

def tarn_water(build, water_id, feature):
    """(surface z, deep zone [(x, y)]) from the level's ASwimWater (build_area_swimwater.py), else the layout's waterLevel."""
    cls = getattr(unreal, 'SwimWater', None)
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if cls is not None and isinstance(actor, cls) and str(actor.get_editor_property('water_id')) == water_id:
            zone = [(p.x, p.y) for p in actor.get_editor_property('deep_zone')]
            return actor.get_actor_location().z, zone
    build.warn(f'no SwimWater {water_id} (build_area_swimwater.py): the layout\'s waterLevel, no deep zone check')
    return float(feature['waterLevel']), []


def inside(point, polygon):
    return importlib.import_module('build_area').inside(point, polygon)


def place_tarn(build, entry, found):
    spec, chest = entry['tarn'], entry['chest']
    feature = story.layout_entry(build, 'features', spec['water'])
    if feature is None:
        return None
    surface, deep = tarn_water(build, spec['water'], feature)
    cx, cy = feature['center']
    sx, sy = spec['stake']
    d = unit((cx - sx, cy - sy))
    # The stake on dry ground: out from the water along the line until the bank stands over the surface.
    stake_z = None
    for k in range(24):
        px, py = sx - d[0] * 25.0 * k, sy - d[1] * 25.0 * k
        g = ground_at(px, py, surface + 2000.0, surface - 3000.0)
        if g is not None and g >= surface + spec.get('dryAbove', 15.0):
            sx, sy, stake_z = px, py, g
            break
    if stake_z is None:
        build.warn(f'{entry["id"]}: no dry bank behind ({spec["stake"][0]}, {spec["stake"][1]}): left out')
        return None
    hx, hy, height = chest_size(build, chest)
    need = height + spec.get('coverTop', 40.0) + 10.0
    span = math.hypot(cx - sx, cy - sy)
    first = None
    for k in range(int(span / 25.0) + 1):
        px, py = sx + d[0] * 25.0 * k, sy + d[1] * 25.0 * k
        bed = ground_at(px, py, surface + 300.0, surface - 3000.0)
        if bed is not None and surface - bed >= need:
            first = 25.0 * k
            break
    if first is None:
        build.warn(f'{entry["id"]}: the tarn is never {need:.0f} cm deep along the rope\'s line: left out')
        return None
    best = None
    t0 = min(first + spec.get('inward', 120.0), span)
    for t, across in ((t0, 0.0), (t0 + 60.0, 0.0), (t0, 70.0), (t0, -70.0), (t0 + 120.0, 0.0), (first, 0.0)):
        px, py = sx + d[0] * t - d[1] * across, sy + d[1] * t + d[0] * across
        bed = ground_at(px, py, surface + 300.0, surface - 3000.0)
        if bed is None or surface - bed < need or (deep and not inside((px, py), deep)):
            continue
        yaw = yaw_of((sx - px, sy - py))
        if footprint_ok(px, py, bed, (hx, hy), yaw, height, level=30.0):
            best = (px, py, bed, yaw)
            break
    if best is None:
        build.warn(f'{entry["id"]}: no level, clear bed {need:.0f} cm under the surface by the rope: left out')
        return None
    x, y, z, yaw = best
    box = core().place_chest(build, chest, x, y, z, yaw, entry['id'])
    if box is None:
        return None
    # The stake on the bank, the rope from it down the bank to the box, the Ranger tag (readable) on the stake's top.
    stake = core().place_prop(build, 'SM_FencePost', (sx, sy, stake_z), yaw_of(d), entry['id'], f'{entry["id"]}_Stake')
    post_top = stake_z + (mesh_box(stake.static_mesh_component.static_mesh)[1].z if stake else 120.0)
    rope_mesh = core().find_mesh('SM_HangingRope')
    if rope_mesh is not None:
        start = unreal.Vector(sx + d[0] * 9.0, sy + d[1] * 9.0, stake_z + 55.0)
        end = unreal.Vector(x - d[0] * hx, y - d[1] * hx, z + height * 0.7)
        run = end - start
        low, high = mesh_box(rope_mesh)
        rope = core().place_prop(build, 'SM_HangingRope', (start.x, start.y, start.z), 0.0, entry['id'],
                                 f'{entry["id"]}_Rope', scale=(1.0, 1.0, run.length() / max(high.z - low.z, 1.0)),
                                 solid=False)
        if rope is not None:
            # Its pivot is its top and it hangs down -Z: -Z along the run.
            rope.set_actor_rotation(unreal.MathLibrary.make_rot_from_z(unreal.Vector(-run.x, -run.y, -run.z)), False)
    core().place_readable(build, spec['readable'], sx, sy, post_top + 0.5, yaw_of(d), entry['id'], spec.get('readableModel'))
    log(build, entry['id'], f'lockbox on the bed {surface - z - height:.0f} cm under the surface ({surface - z:.0f} deep), '
        f'rope {math.hypot(x - sx, y - sy):.0f} cm from the stake at ({sx:.0f}, {sy:.0f})')
    entry = dict(entry, glint=entry.get('glint', [0.0, 0.0, height]))
    return core().place_spot(build, entry, (x, y, z), box), (x, y, z)


# ---------------------------------------------------------------------------
# N2, the bolt-hole in the Tumble
# ---------------------------------------------------------------------------

def line_distance(points, p):
    best = float('inf')
    for (ax, ay), (bx, by) in zip(points, points[1:]):
        dx, dy = bx - ax, by - ay
        t = max(0.0, min(1.0, ((p[0] - ax) * dx + (p[1] - ay) * dy) / ((dx * dx + dy * dy) or 1.0)))
        best = min(best, math.hypot(p[0] - ax - dx * t, p[1] - ay - dy * t))
    return best


def trace_side(start, direction, reach):
    """How far a level line from start (x, y, z) along direction meets something solid, or None."""
    end = unreal.Vector(start[0] + direction[0] * reach, start[1] + direction[1] * reach, start[2])
    hit = unreal.SystemLibrary.line_trace_single(world(), unreal.Vector(*start), end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                                                 True, [], unreal.DrawDebugTrace.NONE, True)
    return None if hit is None else hit.to_tuple()[3]


def bolt_hole_spot(build, spec):
    """The hollow's middle, its ground and the gap's bearing: (x, y, z, f, lane floor top), or None."""
    tumble = story.layout_entry(build, 'obstacles', spec['rockfall'])
    point = story.layout_entry(build, 'features', 'ransomsPoint')
    if tumble is None:
        return None
    path = (point or {}).get('ramp', {}).get('path', [])
    half, lane_w, lane_len = spec['hollow'] * 0.5, spec['laneWidth'], spec['laneLength']
    nx, ny = spec['near']
    grid = [(nx + i * 100.0, ny + j * 100.0) for i in range(-7, 8) for j in range(-7, 8)]
    grid.sort(key=lambda p: math.hypot(p[0] - nx, p[1] - ny))
    facing = unit(spec['facing'])
    for x, y in grid:
        if not inside((x, y), tumble['polygon']) or (path and line_distance(path, (x, y)) < spec['pathClear']):
            continue
        z = ground_at(x, y)
        if z is None:
            continue
        floor = [ground_at(x + i * (half - 30.0), y + j * (half - 30.0), z + 300.0, z - 300.0)
                 for i in (-1, 0, 1) for j in (-1, 0, 1)]
        if any(g is None or abs(g - z) > spec['level'] for g in floor):
            continue
        if blocked((x, y, z + 30.0 + spec['clear'] * 0.5), (half, half, spec['clear'] * 0.5)):
            continue
        for turn in (0, 15, -15, 30, -30, 45, -45):
            f = turned(facing, turn)
            side = (-f[1], f[0])
            lane = [ground_at(x + f[0] * (half + t), y + f[1] * (half + t), z + 300.0, z - 300.0)
                    for t in (0.0, lane_len * 0.25, lane_len * 0.5, lane_len * 0.75, lane_len)]
            if any(g is None or abs(g - z) > spec['level'] + 10.0 for g in lane):
                continue
            room = lane_w * 0.5 + spec['jamb'] + 10.0
            if any(blocked((x + f[0] * (half + t) + side[0] * s, y + f[1] * (half + t) + side[1] * s, z + 110.0),
                           (60.0, 60.0, 80.0)) for t in (40.0, lane_len * 0.5, lane_len - 40.0) for s in (-room + 60.0, 0.0, room - 60.0)):
                continue
            mouth = (x + f[0] * (half + lane_len + 30.0), y + f[1] * (half + lane_len + 30.0), z + 110.0)
            seen = trace_side(mouth, f, spec['view'])
            if seen is not None:
                continue
            return x, y, z, f, max(lane) + 2.0
    return None


def place_bolt_hole(build, entry, found):
    spec, chest = entry['boltHole'], entry['chest']
    fid = entry['id']
    chosen = bolt_hole_spot(build, spec)
    if chosen is None:
        build.warn(f'{fid}: no level, clear {spec["hollow"]:.0f} cm hollow in the Tumble with a gap seen from outside: left out')
        return None
    x, y, z, f, floor_top = chosen
    side = (-f[1], f[0])
    half, lane_w, lane_len, gap, jamb = spec['hollow'] * 0.5, spec['laneWidth'], spec['laneLength'], spec['gap'], spec['jamb']
    yaw = yaw_of(f)
    mx, my = x + f[0] * (half + lane_len * 0.5), y + f[1] * (half + lane_len * 0.5)
    rock = rock_material()
    rocks = core().Rocks(fid)
    # The crawl: a hidden floor along the lane (its top the lane's highest ground) and a hidden ceiling gap cm over it.
    core().hidden_box(build, f'{fid}_Floor', (mx, my, floor_top - 10.0), (lane_len + 80.0, lane_w + 60.0, 20.0), yaw, fid,
                      'FindBoltHoleFloor')
    ceiling = floor_top + gap
    slab = core().find_mesh('SM_FallenSlab_A')
    across = lane_w + 2.0 * jamb + 40.0
    if slab is not None:
        low, high = mesh_box(slab)
        scale = (across / max(high.x - low.x, 1.0), (lane_len + 30.0) / max(high.y - low.y, 1.0), 1.0)
        rot = unreal.Rotator(roll=spec['lean'], pitch=0.0, yaw=yaw_of(side))
        probe = unreal.Transform(unreal.Vector(0.0, 0.0, 0.0), rot, unreal.Vector(*scale))
        under = min(probe.transform_location(unreal.Vector(cx_, cy_, low.z)).z for cx_ in (low.x, high.x) for cy_ in (low.y, high.y))
        mid = probe.transform_location(unreal.Vector((low.x + high.x) * 0.5, (low.y + high.y) * 0.5, 0.0))
        piece = core().place_prop(build, 'SM_FallenSlab_A', (mx - mid.x, my - mid.y, ceiling + 4.0 - under), 0.0, fid,
                                  f'{fid}_Slab', scale=scale, solid=False)
        if piece is not None:
            piece.set_actor_rotation(rot, False)
            if rock is not None:
                piece.static_mesh_component.set_material(0, rock)
    core().hidden_box(build, f'{fid}_Ceiling', (mx, my, ceiling + 20.0), (lane_len + 30.0, across, 40.0), yaw, fid,
                      'FindBoltHoleCeiling')
    # The jambs: two of Boulder_D on edge either side of the lane, from under the ground to over the slab.
    jamb_mesh = core().find_mesh('SM_Boulder_D')
    if jamb_mesh is not None:
        low, high = mesh_box(jamb_mesh)
        for sign in (-1.0, 1.0):
            off = (lane_w * 0.5 + jamb * 0.5 + 5.0) * sign
            jx, jy = mx + side[0] * off, my + side[1] * off
            g = ground_at(jx, jy, z + 400.0, z - 400.0)
            g = z if g is None else g
            sz = (ceiling + 25.0 - (g - 25.0)) / max(high.z - low.z, 1.0)
            rocks.add('SM_Boulder_D', (jx, jy, g - 25.0 - low.z * sz), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw),
                      ((lane_len + 40.0) / max(high.x - low.x, 1.0), jamb / max(high.y - low.y, 1.0), sz), material=rock)
    # The walls: where the hollow's sides are open (a level line from its middle meets nothing within reach), a boulder.
    fill = core().find_mesh(spec['fill'])
    fills = []
    if fill is not None:
        low, high = mesh_box(fill)
        for k in range(BEARINGS):
            b = (math.cos(2.0 * math.pi * k / BEARINGS), math.sin(2.0 * math.pi * k / BEARINGS))
            if abs(math.degrees(math.atan2(f[0] * b[1] - f[1] * b[0], f[0] * b[0] + f[1] * b[1]))) < 40.0:
                continue
            if trace_side((x, y, z + 140.0), b, half + 180.0) is not None:
                continue
            px, py = x + b[0] * (half + 100.0), y + b[1] * (half + 100.0)
            if any(math.hypot(px - qx, py - qy) < 170.0 for qx, qy in fills):
                continue
            g = ground_at(px, py, z + 400.0, z - 400.0)
            g = z if g is None else g
            sz = (spec['wall'] + 20.0) / max(high.z - low.z, 1.0)
            rocks.add(spec['fill'], (px, py, g - 20.0 - low.z * sz), unreal.Rotator(roll=0.0, pitch=0.0, yaw=k * 47.0),
                      (1.0, 1.0, sz))
            fills.append((px, py))
    rocks.place(build)
    hollow = [ground_at(x + i * 120.0, y + j * 120.0, z + 300.0, z - 300.0) for i in (-1, 0, 1) for j in (-1, 0, 1)]
    top = max(g for g in hollow if g is not None)
    if top - min(g for g in hollow if g is not None) > 15.0:
        core().hidden_box(build, f'{fid}_HollowFloor', (x, y, top - 8.0), (spec['hollow'], spec['hollow'], 20.0), yaw, fid,
                          'FindBoltHoleHollow')
    else:
        top = z
    cx, cy = x - f[0] * 80.0, y - f[1] * 80.0
    box = core().place_chest(build, chest, cx, cy, top, yaw, fid)
    if box is None:
        return None
    core().place_readable(build, spec['readable'], cx + side[0] * 90.0 + f[0] * 30.0, cy + side[1] * 90.0 + f[1] * 30.0,
                          top, yaw, fid, spec.get('readableModel'))
    # The lantern on the back wall, its arm out toward the gap (PorchLantern's plate faces +Y): glow seen through the gap.
    back = trace_side((x, y, top + 150.0), (-f[0], -f[1]), half + 150.0)
    reach = (back - 8.0) if back is not None else half
    lx, ly = x - f[0] * reach, y - f[1] * reach
    lantern = core().place_prop(build, 'SM_PorchLantern', (lx, ly, top + 150.0), math.degrees(math.atan2(f[0], -f[1])), fid,
                                f'{fid}_Lantern', solid=False, tags=('Flicker',))
    log(build, fid, f'hollow at ({x:.0f}, {y:.0f}, {z:.0f}), gap facing {yaw:.0f} deg, clearance {gap:.0f} cm over the '
        f'lane floor at {floor_top:.0f}, {len(fills)} wall boulders, lantern {"on the back wall" if back else "free-standing"}')
    return core().place_spot(build, entry, (cx, cy, top), box, tells=[lantern]), (cx, cy, top)


# ---------------------------------------------------------------------------
# N3, Ranger Post Three on the Rimrock
# ---------------------------------------------------------------------------

def place_ranger_post(build, entry, found):
    spec, chest = entry['steps'], entry['chest']
    fid = entry['id']
    rimrock = story.layout_entry(build, 'obstacles', spec['rock'])
    if rimrock is None:
        return None
    xs, ys = [p[0] for p in rimrock['polygon']], [p[1] for p in rimrock['polygon']]
    u = {'east': (0.0, 1.0), 'west': (0.0, -1.0), 'north': (1.0, 0.0), 'south': (-1.0, 0.0)}[spec['face']]
    side = (-u[1], u[0])
    # The top: the east-most of the highest samples whose neighbours are within 70 cm of it.
    tops = {}
    for i in range(int((max(xs) - min(xs)) / 50.0) + 1):
        for j in range(int((max(ys) - min(ys)) / 50.0) + 1):
            px, py = min(xs) + 50.0 * i, min(ys) + 50.0 * j
            hit = hit_at(px, py, 30000.0, -30000.0)
            if hit is not None and not hit[1]:
                tops[(i, j)] = (px, py, hit[0])
    if not tops:
        build.warn(f'{fid}: no rock under {spec["rock"]}: left out')
        return None
    highest = max(t[2] for t in tops.values())
    best = None
    for (i, j), (px, py, pz) in tops.items():
        near = [tops.get((i + a, j + b)) for a in (-1, 0, 1) for b in (-1, 0, 1)]
        if pz < highest - 80.0 or any(n is None or abs(n[2] - pz) > 70.0 for n in near):
            continue
        if best is None or px * u[0] + py * u[1] > best[0] * u[0] + best[1] * u[1]:
            best = (px, py, pz)
    if best is None:
        build.warn(f'{fid}: no flat patch on {spec["rock"]}\'s top: left out')
        return None
    tx, ty, tz = best
    foot = None
    for k in range(1, 80):
        px, py = tx + u[0] * 25.0 * k, ty + u[1] * 25.0 * k
        hit = hit_at(px, py, tz + 300.0, tz - 3000.0)
        if hit is not None and hit[1]:
            foot = 25.0 * k
            break
    if foot is None:
        build.warn(f'{fid}: no ground off the {spec["face"]} face: left out')
        return None
    depth, width, into, bury = spec['depth'], spec['width'], spec['into'], spec['bury']
    # The deck: a hidden plank from behind the top's patch to over the face's foot, its top over every rock bump under it.
    run_from, run_to = -110.0, foot - into + 10.0
    heights = []
    for a in range(int((run_to - run_from) / 40.0) + 1):
        for b in (-90.0, -45.0, 0.0, 45.0, 90.0):
            d_ = run_from + 40.0 * a
            hit = hit_at(tx + u[0] * d_ + side[0] * b, ty + u[1] * d_ + side[1] * b, tz + 300.0, tz - 1000.0)
            if hit is not None and not hit[1]:
                heights.append(hit[0])
    deck_top = max(heights + [tz]) + 3.0
    mid = (run_from + run_to) * 0.5
    core().hidden_box(build, f'{fid}_Deck', (tx + u[0] * mid, ty + u[1] * mid, deck_top - 10.0),
                      (run_to - run_from, 200.0, 20.0), yaw_of(u), fid, 'FindRangerDeck')
    fx, fy = tx + u[0] * foot, ty + u[1] * foot

    def outer_ground(n):
        d_ = -into + n * depth
        return ground_at(fx + u[0] * d_, fy + u[1] * d_, deck_top + 300.0, deck_top - 3000.0)
    n = spec['least']
    while True:
        z_out = outer_ground(n)
        if z_out is None:
            build.warn(f'{fid}: no ground {n * depth:.0f} cm out from the face: left out')
            return None
        rise = (deck_top - z_out) / (n + 1)
        if rise <= spec['maxRise'] or n >= 9:
            break
        n += 1
    block = core().find_mesh('SM_Boulder_D')
    rock = rock_material()
    rocks = core().Rocks(fid)
    for k in range(1, n + 1):
        d_ = -into + (n - k) * depth + depth * 0.5
        sx, sy = fx + u[0] * d_, fy + u[1] * d_
        top = z_out + rise * k
        under = [ground_at(sx + u[0] * a + side[0] * b, sy + u[1] * a + side[1] * b, top + 200.0, top - 3000.0)
                 for a in (-depth * 0.4, depth * 0.4) for b in (-width * 0.4, width * 0.4)]
        low_ground = min([g for g in under if g is not None] + [z_out])
        if block is not None:
            low, high = mesh_box(block)
            sz = (top - (low_ground - bury)) / max(high.z - low.z, 1.0)
            rocks.add('SM_Boulder_D', (sx, sy, top - high.z * sz),
                      unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw_of(u) + (k * 37 % 9) - 4.0),
                      ((depth + 15.0) / max(high.x - low.x, 1.0), width / max(high.y - low.y, 1.0), sz), material=rock)
        core().hidden_box(build, f'{fid}_Step_{k}', (sx, sy, top + 3.0 - 10.0), (depth, width - 20.0, 20.0), yaw_of(u), fid,
                          'FindRangerStep')
    rocks.place(build)
    box = core().place_chest(build, chest, tx, ty, deck_top, yaw_of(u), fid)
    if box is None:
        return None
    core().place_readable(build, spec['readable'], tx + side[0] * 80.0, ty + side[1] * 80.0, deck_top, yaw_of(u), fid,
                          spec.get('readableModel'))
    # The tell at the foot: a stake with a Ranger cache tag on its top, beside the lowest step.
    out = -into + n * depth + spec['stakeOut']
    px, py = fx + u[0] * out + side[0] * spec['stakeSide'], fy + u[1] * out + side[1] * spec['stakeSide']
    pz = ground_at(px, py, deck_top + 300.0, deck_top - 3000.0)
    if pz is not None:
        stake = core().place_prop(build, 'SM_FencePost', (px, py, pz), yaw_of(u), fid, f'{fid}_Stake')
        if stake is not None:
            post_top = pz + mesh_box(stake.static_mesh_component.static_mesh)[1].z
            core().place_prop(build, 'SM_CacheTag', (px, py, post_top + 0.3), yaw_of(u), fid, f'{fid}_Tag', solid=False)
    log(build, fid, f'top ({tx:.0f}, {ty:.0f}) deck at {deck_top:.0f} ({deck_top - min(heights + [tz]):.0f} cm over its '
        f'lowest rock), {n} steps of {rise:.0f} cm from {z_out:.0f}, {foot:.0f} cm to the face\'s foot')
    return core().place_spot(build, entry, (tx, ty, deck_top), box), (tx, ty, deck_top)


def graves():
    return importlib.reload(importlib.import_module('build_area_finds_graves'))


BUILDERS = {
    'tarn': place_tarn,
    'boltHole': place_bolt_hole,
    'rangerPost': place_ranger_post,
    'pikesCache': lambda build, entry, found: graves().place_pikes_cache(build, entry, found),
    'miserGrave': lambda build, entry, found: graves().place_miser_grave(build, entry, found),
}
