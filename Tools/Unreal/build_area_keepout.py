"""Cliff pieces kept out of the buildings players walk into, for build_area.py (layout.json level.cliffs.keepOut: the
placement keys of the buildings with rooms; Ransom's Rest's depot, Tilly's workshop, the sheriff's office, the
mountain's lift and Calder's Post among them).

A cliff piece is placed from its dressing point and the kit's shape, never from what stands near it, so where a
building stands against (or across) a cliff run the rock can stand in its rooms: two East Shoulder faces stood in the
depot's baggage room and round the agent's desk, the LOST PROPERTY window behind the rock (2026-10-14). So once the
cliffs and the models stand, each building's insides are sampled (Rooms): points on a SAMPLE_STEP grid through its
model's bounds, SAMPLE_RISE apart up each column, under the model's topmost surface there (a room, an upper floor, the
attic, a canopy, the eaves; never the open ground round it nor the air over its roof). A cliff piece whose collision
comes within KEEP_CLEAR of any of them stands inside. A face or a panel with the building in front of it (most of the
points it covers ahead of its pivot) moves back into its wall MOVE_STEP at a time, KEEP_MOST at most (the building
hides the hollow it leaves); one under a building that stands over its wall's top is cut down from its pivot until its
top ends KEEP_CLEAR under the lowest point it covers, keeping at least KEEP_KEEP of its height (as build_area_platforms
does under the burial deck); each tries the other way when its own fails, then along its run, whichever way clears
sooner. An outcrop moves straight away from the building. One that nothing clears is left out. Each one is logged.

Saved/cliff_in_buildings.py (CLIFFIN) uses the same Rooms to list every rock inside any building, read-only.
"""
import math
import re

import unreal

# The grid through a building's bounds (cm): across, and up each column, starting SAMPLE_FLOOR over the bounds' bottom.
SAMPLE_STEP = 50.0
SAMPLE_RISE = 75.0
SAMPLE_FLOOR = 30.0
# A rock standing within this of a sampled point is inside (cm): a sphere overlap against its collision.
KEEP_CLEAR = 35.0
# A piece inside moves MOVE_STEP at a time, KEEP_MOST at most (cm), or is cut down keeping at least KEEP_KEEP of its
# height.
MOVE_STEP = 25.0
KEEP_MOST = 600.0
KEEP_KEEP = 0.3
# A cliff piece's label (cliffs(), panels): Cliff_<group>_<point>[_<course>], the point two digits or more (the
# boundary's rock runs past 99); a knob's freestanding rock: Outcrop_<group>.
CLIFF_LABEL = re.compile(r'^Cliff_(.+?)_(\d{2,})(?:_\d+)?$')
OUTCROP_LABEL = re.compile(r'^Outcrop_')
OBJECTS = [unreal.ObjectTypeQuery.ECC_WORLD_STATIC, unreal.ObjectTypeQuery.ECC_WORLD_DYNAMIC]


def editor_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def box_of(actor, grow=0.0):
    """An actor's colliding world bounds as (lo, hi) tuples, grown by grow (cm)."""
    origin, extent = actor.get_actor_bounds(True)
    return ((origin.x - extent.x - grow, origin.y - extent.y - grow, origin.z - extent.z - grow),
            (origin.x + extent.x + grow, origin.y + extent.y + grow, origin.z + extent.z + grow))


def boxes_meet(a, b):
    return all(a[0][k] <= b[1][k] and b[0][k] <= a[1][k] for k in range(3))


class Rooms:
    """A building's insides: the points under its own model on a grid through its mesh components' bounds (the depot's
    station carries its model in a component; instanced components are left out)."""

    def __init__(self, building):
        self.building = building
        self.label = str(building.get_actor_label())
        self.parts = [c for c in building.get_components_by_class(unreal.StaticMeshComponent)
                      if c.static_mesh is not None and not isinstance(c, unreal.InstancedStaticMeshComponent)]
        corners = []
        for part in self.parts:
            box, transform = part.static_mesh.get_bounding_box(), part.get_world_transform()
            for x in (box.min.x, box.max.x):
                for y in (box.min.y, box.max.y):
                    for z in (box.min.z, box.max.z):
                        corners.append(transform.transform_location(unreal.Vector(x, y, z)))
        self.box = None
        if corners:
            self.box = ((min(c.x for c in corners), min(c.y for c in corners), min(c.z for c in corners)),
                        (max(c.x for c in corners), max(c.y for c in corners), max(c.z for c in corners)))
        self._points = None

    def top(self, x, y, above, below):
        """The model's topmost surface over (x, y) between above and below (traced down onto its parts), or None."""
        best = None
        for part in self.parts:
            hit = part.line_trace_component(unreal.Vector(x, y, above), unreal.Vector(x, y, below), True, False, False)
            if hit:
                z = (hit[0] if isinstance(hit, tuple) else hit.to_tuple()[5]).z
                best = z if best is None else max(best, z)
        return best

    def points(self):
        """Every sampled point under the model (worked out once): the grid's columns through each part's own box
        (turned with it), each from SAMPLE_FLOOR over the box's bottom up to the model's topmost surface there."""
        if self._points is not None:
            return self._points
        self._points = []
        if self.box is None:
            return self._points
        above, below = self.box[1][2] + 100.0, self.box[0][2] - 100.0
        for part in self.parts:
            box, transform = part.static_mesh.get_bounding_box(), part.get_world_transform()
            scale = transform.scale3d
            size = (box.max.x - box.min.x, box.max.y - box.min.y)
            nx = max(1, int(math.ceil(size[0] * abs(scale.x) / SAMPLE_STEP)))
            ny = max(1, int(math.ceil(size[1] * abs(scale.y) / SAMPLE_STEP)))
            floor = transform.transform_location(unreal.Vector(0.0, 0.0, box.min.z)).z + SAMPLE_FLOOR
            for i in range(nx):
                for j in range(ny):
                    at = transform.transform_location(unreal.Vector(box.min.x + (i + 0.5) * size[0] / nx,
                                                                    box.min.y + (j + 0.5) * size[1] / ny, box.min.z))
                    roof = self.top(at.x, at.y, above, below)
                    z = floor
                    while roof is not None and z < roof - 10.0:
                        self._points.append(unreal.Vector(at.x, at.y, z))
                        z += SAMPLE_RISE
        return self._points

    def hits(self, world, piece, path, box, first=False):
        """The sampled points inside box (lo, hi) that piece's collision comes within KEEP_CLEAR of (path: its path
        name); with first, stops at the first."""
        found = []
        for p in self.points():
            if not (box[0][0] <= p.x <= box[1][0] and box[0][1] <= p.y <= box[1][1] and box[0][2] <= p.z <= box[1][2]):
                continue
            for c in unreal.SystemLibrary.sphere_overlap_components(world, p, KEEP_CLEAR, OBJECTS, None, []) or []:
                owner = c.get_owner()
                if owner is not None and owner.get_path_name() == path:
                    found.append(p)
                    break
            if first and found:
                break
        return found


def inside(world, piece, rooms, first=True):
    """The rooms piece stands in, with the points it covers: [(rooms, [points])]."""
    path = piece.get_path_name()
    box = box_of(piece, KEEP_CLEAR)
    out = []
    for room in rooms:
        if room.box is None or not boxes_meet(box, room.box):
            continue
        found = room.hits(world, piece, path, box, first)
        if found:
            out.append((room, found))
            if first:
                break
    return out


def moved_clear(world, piece, rooms, way, start):
    """Moves piece along way (a unit horizontal Vector) from start, MOVE_STEP at a time, until it stands in none of the
    rooms: how far, or None past KEEP_MOST (put back at start)."""
    distance = 0.0
    while distance < KEEP_MOST:
        distance += MOVE_STEP
        piece.set_actor_location(start + way * distance, False, True)
        if not inside(world, piece, rooms):
            return distance
    piece.set_actor_location(start, False, True)
    return None


def cut_clear(world, piece, rooms):
    """Cuts piece down from its pivot (its height scaled) until its top ends KEEP_CLEAR under the lowest point it
    covers, again while it still covers one: the share of its height it keeps, or None (its scale put back) when that
    would be under KEEP_KEEP."""
    scale = piece.get_actor_scale3d()
    pivot = piece.get_actor_location().z
    origin, extent = piece.get_actor_bounds(True)
    height = max(origin.z + extent.z - pivot, 1.0)  # its reach over its pivot as placed
    share = 1.0
    for _ in range(8):
        standing = inside(world, piece, rooms, first=False)
        if not standing:
            return share
        lowest = min(p.z for _, found in standing for p in found)
        # At least a step lower each time: its top is broken rock, not the bounds' flat top.
        share = min((lowest - KEEP_CLEAR - pivot) / height, share - MOVE_STEP / height)
        if share < KEEP_KEEP:
            break
        piece.set_actor_scale3d(unreal.Vector(scale.x, scale.y, scale.z * share))
    piece.set_actor_scale3d(scale)
    return None


def flat(vector):
    norm = math.hypot(vector.x, vector.y) or 1.0
    return unreal.Vector(vector.x / norm, vector.y / norm, 0.0)


def keep_out(build):
    """Moves (or leaves out) the cliff pieces and outcrops standing in level.cliffs.keepOut's buildings (see the
    module's docstring); returns (moved, left out)."""
    keys = build.cliff_look.get('keepOut', [])
    if not keys:
        return 0, 0
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = editor_world()
    tag = unreal.Name(build.tag)
    level = [a for a in actors.get_all_level_actors() if tag in a.tags]
    by_label = {str(a.get_actor_label()): a for a in level}
    missing = [k for k in keys if k not in by_label]
    if missing:
        build.warn(f'no {", ".join(missing)} placed for the cliffs to keep out of (level.cliffs.keepOut)')
    rooms = [Rooms(by_label[k]) for k in keys if k in by_label]
    rooms = [r for r in rooms if r.box is not None]
    pieces = [a for a in level if isinstance(a, unreal.StaticMeshActor)
              and (CLIFF_LABEL.match(str(a.get_actor_label())) or OUTCROP_LABEL.match(str(a.get_actor_label())))]
    moved = left = 0
    for piece in pieces:
        label = str(piece.get_actor_label())
        if not any(boxes_meet(box_of(piece, KEEP_CLEAR), r.box) for r in rooms):
            continue
        standing = inside(world, piece, rooms, first=False)
        if not standing:
            continue
        where = ', '.join(f'{room.label} ({len(found)} points)' for room, found in standing)
        start = piece.get_actor_location()
        done = None
        if CLIFF_LABEL.match(label):
            rotation = piece.get_actor_rotation()
            forward, right = flat(rotation.get_forward_vector()), flat(rotation.get_right_vector())
            ahead = sorted((p.x - start.x) * forward.x + (p.y - start.y) * forward.y
                           for _, found in standing for p in found)

            def back():
                d = moved_clear(world, piece, rooms, forward * -1.0, start)
                return None if d is None else f'moved {d:.0f} cm back into its wall'

            def cut():
                share = cut_clear(world, piece, rooms)
                return None if share is None else f'cut down to {share:.2f} of its height'
            # The building in front of it (most of what it covers ahead of its pivot): back into its wall, where the
            # building hides the hollow; a building over the wall's top: cut down under its floor.
            for attempt in ((back, cut) if ahead[len(ahead) // 2] >= 0.0 else (cut, back)):
                done = attempt()
                if done:
                    break
            if not done:
                # Along its run, whichever way clears sooner.
                best = None
                for way in (right, right * -1.0):
                    d = moved_clear(world, piece, rooms, way, start)
                    if d is not None and (best is None or d < best[0]):
                        best = (d, way)
                if best is not None:
                    piece.set_actor_location(start + best[1] * best[0], False, True)
                    done = f'moved {best[0]:.0f} cm along its run'
        else:
            room = standing[0][0]
            middle = unreal.Vector((room.box[0][0] + room.box[1][0]) * 0.5, (room.box[0][1] + room.box[1][1]) * 0.5,
                                   start.z)
            d = moved_clear(world, piece, rooms, flat(start - middle), start)
            done = None if d is None else f'moved {d:.0f} cm away from it'
        if not done:
            build.log(f'{label} left out: it stands in {where}, and no move up to {KEEP_MOST:.0f} cm nor cut down to '
                      f'{KEEP_KEEP:.2f} of its height clears it')
            actors.destroy_actor(piece)
            left += 1
            continue
        build.log(f'{label} {done}: it stood in {where}')
        moved += 1
    return moved, left
