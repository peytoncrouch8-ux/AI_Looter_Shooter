"""Talus banking a pit wall's panel run (build_area_panels.py), for the area's story scripts (build_area_sink.py calls
it in the gameplay pass, once its sacs, lantern and blocks stand): layout.json level.cliffs.panels.<group>.talus.

The panels stand on the wall's apron, which rises a metre or two under them, and their faces bulge forward a metre and
more above their feet, so from the floor a dark band showed along the run where the apron runs in under the bulge.
Rocks of the cliffs' own layered rock (Rocks.py's Rock_C slab and Rock_D chunk) bank it in two rows:

- The foot (Feet.at): level lines from the open side at heights over the floor (cm over the floor 4 m out) meet the
  apron low down and the run's faces above it. The lowest line to meet a face marks where the face stands over the
  apron (its depth); the cavity under it is how far the apron met by the line just under that one runs in behind it.
  Where no line meets a face, the apron's lowest hit stands in.
- The first row: a rock every every (cm along the run, drawn; measured between the rocks, not along the wall's line,
  which stands further out) of models (each [model, weight, [least, most scale]]), turned any way, bury of its height
  under the ground under it (drawn). Its middle stands a tenth of its radius behind the face's foot, less out (drawn):
  it sits on the apron in the band and shows most of itself in front of the face (on bare apron, about half its
  radius in front of the apron's hit).
- The second row (front): where the cavity runs deep cm or more at either of two neighbouring rocks, a smaller chunk
  midway between them, ahead (drawn) past the first row's front: the band is broken up rather than dotted.

Every rock is tagged Obstacle with its own collision. None stands in the ramp's walkway (clear cm beyond its half width
and the rock's own size) or where the caller's blocked(x, y, radius) gives a reason (a short word: the Sink's sac,
spider, lantern, block, den); each spot kept clear is logged with its reason. Everything is drawn from the group's own
seed, every draw made for every rock, so a spot kept clear doesn't change the rest.
"""
import importlib
import math
import random
import re

import unreal

panels = importlib.import_module('build_area_panels')

FLOOR_REACH = 30000.0   # cm up and down a trace looks for the ground
STEP = 25.0             # cm the walk along the wall's line moves on by while a rock stands too close to the last

DEFAULTS = dict(
    models=[['Rock_C', 3.0, [2.2, 3.2]], ['Rock_D', 2.0, [3.6, 5.0]]],
    every=[150.0, 250.0],
    heights=[20.0, 40.0, 60.0, 80.0, 100.0, 130.0, 160.0, 200.0, 250.0],
    out=[-10.0, 20.0],
    bury=[0.3, 0.45],
    clear=50.0,
    front=dict(models=[['Rock_D', 1.0, [2.0, 3.0]], ['Rock_C', 1.0, [1.2, 1.8]]], deep=30.0, ahead=[0.0, 30.0],
               bury=[0.3, 0.45]),
)


def terrain_tiles(tag):
    """The placed terrain's tiles (tagged Ground and the build's tag) with their bounds in plan."""
    tiles = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if unreal.Name('Ground') in actor.tags and unreal.Name(tag) in actor.tags and \
                isinstance(actor, unreal.StaticMeshActor):
            origin, extent = actor.get_actor_bounds(False)
            tiles.append((actor.static_mesh_component, origin.x - extent.x, origin.x + extent.x, origin.y - extent.y,
                          origin.y + extent.y))
    return tiles


def component_hit(component, start, end):
    """Where a line first meets one component (complex), or None."""
    hit = component.line_trace_component(start, end, True, False, False)
    if not hit:
        return None
    if isinstance(hit, tuple):
        return next((h for h in hit if isinstance(h, unreal.Vector)), None)
    return hit.to_tuple()[5]


def tiles_hit(tiles, start, end):
    """Where a line first meets the terrain's tiles, or None."""
    best = None
    for component, x0, x1, y0, y1 in tiles:
        if max(start.x, end.x) < x0 or min(start.x, end.x) > x1 or max(start.y, end.y) < y0 or min(start.y, end.y) > y1:
            continue
        at = component_hit(component, start, end)
        if at is not None and (best is None or (at - start).length() < (best - start).length()):
            best = at
    return best


def ground(tiles, x, y):
    hit = tiles_hit(tiles, unreal.Vector(x, y, FLOOR_REACH), unreal.Vector(x, y, -FLOOR_REACH))
    return None if hit is None else hit.z


def run_pieces(build, group):
    """The group's panel run as placed (Cliff_<group>_<nn>), in label order (the seam first)."""
    label = re.compile(rf'^Cliff_{re.escape(group)}_(\d{{2}})$')
    level = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    return sorted((a for a in level if unreal.Name(build.tag) in a.tags and label.match(str(a.get_actor_label()))),
                  key=lambda a: str(a.get_actor_label()))


def find_meshes(names):
    """The static meshes under /Game/Art with these names (SM_<name>), by name."""
    wanted = {f'SM_{n}': n for n in names}
    found = {}
    for path in unreal.EditorAssetLibrary.list_assets('/Game/Art', recursive=True, include_folder=False):
        name = path.rsplit('/', 1)[-1].split('.')[0]
        if name in wanted:
            found[wanted[name]] = unreal.load_asset(path.split('.')[0])
    return found


def kit(build, group, specs, meshes):
    """[(mesh, weight, [scales], radius (its larger half side), height)] for the models found."""
    out = []
    for name, weight, scales in specs:
        if name not in meshes:
            build.warn(f'{group}: no SM_{name} for its talus')
            continue
        box = meshes[name].get_bounding_box()
        out.append((meshes[name], float(weight), scales, max(box.max.x - box.min.x, box.max.y - box.min.y) * 0.5,
                    box.max.z))
    return out


def pick(models, rnd):
    left = rnd.uniform(0.0, sum(m[1] for m in models))
    for model in models:
        left -= model[1]
        if left <= 0.0:
            return model
    return models[-1]


class Feet:
    """The wall's foot along a run: its line, the run's faces and the terrain."""

    def __init__(self, line, faces, tiles, heights):
        self.line, self.faces, self.tiles, self.heights = line, faces, tiles, heights

    def at(self, s):
        """At s along the line (the module's docstring): dict(x, y, n (into the pit), t (along), floor (its height 4 m
        out), hits ([(height, depth, 'face' or 'terrain')] for each line that met anything; depth in cm behind the
        line, along the way out of the pit), face (depth where the face stands over the apron, or None), cavity (cm the
        apron runs in under it), foot (the depth a rock goes from: the face's, else the apron's lowest hit)), or None
        where no line meets anything."""
        (x, y), (nx, ny), (tx, ty) = self.line.at(s)
        floor = ground(self.tiles, x + nx * 400.0, y + ny * 400.0)
        if floor is None:
            return None
        hits = []
        for h in self.heights:
            start = unreal.Vector(x + nx * 800.0, y + ny * 800.0, floor + h)
            end = unreal.Vector(x - nx * 1500.0, y - ny * 1500.0, floor + h)
            best = None
            for at, kind in [(component_hit(f, start, end), 'face') for f in self.faces] + \
                    [(tiles_hit(self.tiles, start, end), 'terrain')]:
                if at is not None and (best is None or (at - start).length() < best[0]):
                    best = ((at - start).length(), kind)
            if best is not None:
                hits.append((h, best[0] - 800.0, best[1]))
        if not hits:
            return None
        faces = [i for i, hit in enumerate(hits) if hit[2] == 'face']
        face = cavity = None
        if faces:
            face = hits[faces[0]][1]
            under = [hit[1] for hit in hits[:faces[0]] if hit[2] == 'terrain']
            cavity = max(under[-1] - face, 0.0) if under else 0.0
        return dict(x=x, y=y, n=(nx, ny), t=(tx, ty), floor=floor, hits=hits, face=face, cavity=cavity or 0.0,
                    foot=face if face is not None else hits[0][1])


def talus(build, group, blocked=None, folder='Gameplay', label='Talus'):
    """The group's talus (the module's docstring). Returns the rocks of both rows: [dict(actor, x, y, z (the ground
    under it), radius, row (1 or 2), foot (x, y: the foot behind it), facing and along (unit, in plan), overhang (cm the
    cavity runs in under the face there))], the first row in order along the run."""
    look = build.cliff_look.get('panels', {}).get(group, {}).get('talus')
    if look is None:
        return []           # an empty block takes every default
    front = dict(DEFAULTS['front'], **look.get('front', {}))
    look = dict(DEFAULTS, **{k: v for k, v in look.items() if k != 'front'})
    feature = panels.feature_of(build, group)
    run = run_pieces(build, group)
    tiles = terrain_tiles(build.tag)
    if feature is None or not run or not tiles:
        build.warn(f'{group}: no panel run, wall line or terrain for its talus: none')
        return []
    meshes = find_meshes({m[0] for m in look['models'] + front['models']})
    first_kit, front_kit = kit(build, group, look['models'], meshes), kit(build, group, front['models'], meshes)
    if not first_kit:
        return []
    line = panels.WallLine(feature['polygon'])
    feet = Feet(line, [p.static_mesh_component for p in run], tiles, look['heights'])
    # The run's extent along the line: its pieces' places, unwrapped in order, out to their ends.
    along, halves = [], []
    for piece in run:
        loc = piece.get_actor_location()
        s = line.nearest(loc.x, loc.y)
        if along:
            s += line.length * round((along[-1] - s) / line.length)
        along.append(s)
        box = piece.static_mesh_component.static_mesh.get_bounding_box()
        halves.append(abs(piece.get_actor_scale3d().y) * (box.max.y - box.min.y) * 0.5)
    lo, hi = min(a - h for a, h in zip(along, halves)), max(a + h for a, h in zip(along, halves))
    ramp = feature.get('ramp')
    rnd = random.Random(f'{group} talus')
    kept = {}

    def free(x, y, r):
        """Whether a rock of radius r may stand at (x, y); logs what keeps it clear if not."""
        reason = None
        if ramp and ramp.get('path') and \
                panels.path_distance(ramp['path'], x, y) < ramp['width'] * 0.5 + r + look['clear']:
            reason = 'walkway'
        elif blocked is not None:
            reason = blocked(x, y, r)
        if reason:
            what = reason.split(' ')[0]
            kept[what] = kept.get(what, 0) + 1
            build.log(f'{group}: a talus spot at ({x:.0f}, {y:.0f}) kept clear: {reason}')
        return not reason

    rocks = []

    def place(mesh, x, y, r, height, scale, yaw, bury, row, foot, overhang, facing, along):
        z = ground(tiles, x, y)
        if z is None:
            return None
        number = sum(1 for rock in rocks if rock['row'] == row) + 1
        name = f'{label}_{number:02d}' if row == 1 else f'{label}_F{number:02d}'
        actor = build.place(mesh, (x, y, z - bury * height * scale), yaw, label=name, folder=folder,
                            scale=(scale, scale, scale), tags=('Obstacle',))
        rock = dict(actor=actor, x=x, y=y, z=z, radius=r, row=row, foot=foot, facing=facing, along=along,
                    overhang=overhang)
        rocks.append(rock)
        return rock

    firsts, last, gap = [], None, 0.0
    s = lo + rnd.uniform(0.0, look['every'][0])
    while s <= hi:
        mesh, _, scales, radius, height = pick(first_kit, rnd)
        scale, yaw = rnd.uniform(*scales), rnd.uniform(-180.0, 180.0)
        out, bury, step = rnd.uniform(*look['out']), rnd.uniform(*look['bury']), rnd.uniform(*look['every'])
        r = radius * scale
        spot = None
        for _ in range(8):
            foot_at = feet.at(s)
            if foot_at is None:
                break
            (nx, ny), (fx, fy) = foot_at['n'], (foot_at['x'], foot_at['y'])
            # Its middle's depth behind the line: a tenth of its radius behind the face's foot, or on bare apron
            # about half its radius in front of its hit.
            if foot_at['face'] is not None:
                depth = foot_at['face'] + 0.1 * r - out
            else:
                depth = foot_at['foot'] - 0.45 * r - out
            x, y = fx - nx * depth, fy - ny * depth
            short = 0.0 if last is None else gap - math.hypot(x - last[0], y - last[1])
            if short <= 0.05 * gap:
                spot = (x, y, foot_at)
                break
            s += max(short, STEP)
        s += step
        gap = step
        if spot is None:
            continue
        x, y, foot_at = spot
        deep = foot_at['foot']
        foot = (foot_at['x'] - foot_at['n'][0] * deep, foot_at['y'] - foot_at['n'][1] * deep)
        overhang = foot_at['cavity']
        if not free(x, y, r):
            firsts.append(None)
            continue
        rock = place(mesh, x, y, r, height, scale, yaw, bury, 1, foot, overhang, foot_at['n'], foot_at['t'])
        firsts.append(rock)
        if rock is not None:
            last = (x, y)
    # The second row: between two neighbouring first-row rocks where the band runs deep, a chunk past their front.
    if front_kit:
        for a, b in zip(firsts, firsts[1:]):
            mesh, _, scales, radius, height = pick(front_kit, rnd)
            scale, yaw = rnd.uniform(*scales), rnd.uniform(-180.0, 180.0)
            ahead, bury = rnd.uniform(*front['ahead']), rnd.uniform(*front['bury'])
            if a is None or b is None or max(a['overhang'], b['overhang']) < front['deep']:
                continue
            nx, ny = a['facing'][0] + b['facing'][0], a['facing'][1] + b['facing'][1]
            length = math.hypot(nx, ny) or 1.0
            nx, ny = nx / length, ny / length
            r = radius * scale
            push = max(a['radius'], b['radius']) * 0.55 + ahead + r * 0.3
            x, y = (a['x'] + b['x']) * 0.5 + nx * push, (a['y'] + b['y']) * 0.5 + ny * push
            if free(x, y, r):
                place(mesh, x, y, r, height, scale, yaw, bury, 2, a['foot'], max(a['overhang'], b['overhang']),
                      (nx, ny), a['along'])
    first = sum(1 for rock in rocks if rock['row'] == 1)
    clear = ', '.join(f'{n} by the {k}' for k, n in sorted(kept.items()))
    build.log(f'{group}: {first} talus rocks along the run\'s feet and {len(rocks) - first} in front of them'
              + (f'; spots kept clear: {clear}' if kept else ''))
    return rocks
