"""A pit's wall dressed with the narrow cliff panels and the seam wedge (Art/Models/Rocks/Cliffs.py: CliffPanel_A, _B,
_C and CliffSeam_A), for build_area.py, in place of a cliff group's faces (layout.json level.cliffs.panels: the group's
id to its run's settings; the Sink's wall, theSink).

A face is a 12 m slab with a flat back: on a pit's curving, noisy wall its middle stood proud while its ends sank into
the rock, and the terrain showed between the pieces. The panels are 4-6 m wide with backs that bulge, round over at the
top and draw in at the ends, so a run of them follows the wall in short steps. Following the art's rules:

- The run follows the wall's line, the pit feature's outline smoothed (its points every STEP), from the seam at one end
  to the other end of the stretch of wall the group's dressing points stand on: the outline less its cliffGaps (the
  rock that stands in for the wall, Den Rock) and its ramp's walkway with rampMargin on either side.
- Pieces go in the order of sequence (each a model and whether it's mirrored across its width; three panels, six with
  mirroring), each facing the pit square to the wall's line, neighbours overlapping by overlap (cm) so the joint reads
  as a shallow gully.
- Each sinks by its own depth (sink, cm) and is stretched so its top reaches its own share (top) of the wall under the
  wall's rounded lip (build_area.wall_lean's ceiling), within stretch, never into the lip.
- It leans back with the wall (wall_lean) less its own front's lean (pieces[model].lean, degrees), and stands with its
  median front (pieces[model].front, cm in front of the pivot at the foot, leaning with it) proud (cm) in front of the
  wall's most forward point, traced across the middle of its width (between its curved ends) at heights of its own.
- The seam (seam: the wedge and the rock it meets, Den Rock): the rock's flank is found along the wall's line where the
  rock first stands at or in front of the wall; the run starts gap past it, and the seam's pivot stands along past it,
  turned turn degrees from the wall's facing away from the rock, out in front of the wall at its height at, or further
  out where the wall falls back into the corner, its median front proud in front of the run's first: its 3 m back
  reaches about 0.3 m into the rock and 1.3 m behind the first panel's end, its front about a metre proud of the run.
  It goes last, once the run stands.

Every piece is labelled Cliff_<group>_<nn> (the seam 00, the panels from 01 along the run) and tagged Obstacle like
the faces, so the walkways and the platforms treat them as cliff pieces. Everything is drawn from seeds of the group's
own, so a rebuild places it the same.

The run's feet are banked with talus by build_area_talus.py (the group's talus block), from the gameplay pass.
"""
import math
import random

import unreal

STEP = 25.0             # cm between the wall line's points
FLOOR_REACH = 30000.0   # cm up and down a trace looks for the ground

# The run's settings, each overridable by the group's block in level.cliffs.panels (cm and degrees).
DEFAULTS = dict(
    overlap=160.0,              # neighbours overlap by this much along the wall
    proud=55.0,                 # the median front stands this far in front of the wall's most forward point
    sink=[30.0, 150.0],         # how far each piece sinks under the floor at its foot
    stretch=[0.85, 1.2],        # its height scale
    top=[0.9, 1.0],             # the share of the wall under the lip its top reaches
    heights=[0.3, 0.6, 0.9],    # where up a piece the wall is traced for its most forward point (shares of its height)
    across=9,                   # and at how many points across the middle of its width
    footOut=270.0,              # the floor at a piece's foot: this far in front of the wall's line
    topIn=250.0,                # the rim behind it: this far behind the line
    rampMargin=200.0,           # the run keeps this far off its ramp's walkway, either side
    minWall=300.0,              # no piece where the wall is lower
)
SEAM_DEFAULTS = dict(
    gap=140.0,                  # the run starts this far along the wall past the rock's flank
    along=120.0,                # the seam's pivot stands this far along past the flank
    out=5.0,                    # and this far in front of the wall at the height at
    at=300.0,
    proud=100.0,                # its median front at least this far in front of the run's first (cm)
    turn=8.0,                   # turned from the wall's facing away from the rock
    sink=60.0,
    stretch=[0.85, 1.25],
    heights=[0.3, 0.6],         # where up the wall the rock's flank is looked for (shares of the wall)
    searchIn=1000.0,            # looked for from this far inside the run's end
    seek=2000.0,                # up to this far past it
)

def vec(x, y, z=0.0):
    return unreal.Vector(float(x), float(y), float(z))


class WallLine:
    """A pit's wall line: its feature's outline (cm), smoothed (closed Catmull-Rom) into points every STEP, with their
    distance along it, the way into the pit at each (the inward normal) and the way along it."""

    def __init__(self, polygon):
        pts = []
        n = len(polygon)
        for i in range(n):
            p0, p1, p2, p3 = polygon[i - 1], polygon[i], polygon[(i + 1) % n], polygon[(i + 2) % n]
            count = max(int(math.dist(p1, p2) / STEP), 1)
            for j in range(count):
                t = j / count
                pts.append(tuple(0.5 * (2.0 * p1[k] + (p2[k] - p0[k]) * t
                                        + (2.0 * p0[k] - 5.0 * p1[k] + 4.0 * p2[k] - p3[k]) * t * t
                                        + (3.0 * p1[k] - p0[k] - 3.0 * p2[k] + p3[k]) * t * t * t) for k in range(2)))
        self.pts = pts
        self.s = [0.0]
        for a, b in zip(pts, pts[1:] + pts[:1]):
            self.s.append(self.s[-1] + math.dist(a, b))
        self.length = self.s[-1]
        area = sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(pts, pts[1:] + pts[:1]))
        self.left_in = area > 0.0      # counter-clockwise: the inside is on the left of the way along

    def index(self, s):
        s %= self.length
        lo, hi = 0, len(self.pts)
        while hi - lo > 1:
            mid = (lo + hi) // 2
            if self.s[mid] <= s:
                lo = mid
            else:
                hi = mid
        return lo

    def at(self, s):
        """(x, y), the way into the pit (unit) and the way along (unit, increasing s) at a distance along the line."""
        s %= self.length
        i = self.index(s)
        a, b = self.pts[i], self.pts[(i + 1) % len(self.pts)]
        seg = max(self.s[i + 1] - self.s[i], 1e-6)
        t = (s - self.s[i]) / seg
        x, y = a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
        p, q = self.pts[i - 1], self.pts[(i + 2) % len(self.pts)]
        tx, ty = q[0] - p[0], q[1] - p[1]
        tl = math.hypot(tx, ty) or 1.0
        tx, ty = tx / tl, ty / tl
        nx, ny = (-ty, tx) if self.left_in else (ty, -tx)
        return (x, y), (nx, ny), (tx, ty)

    def nearest(self, x, y):
        """The distance along the line of its point nearest (x, y)."""
        best = min(range(len(self.pts)), key=lambda i: (self.pts[i][0] - x) ** 2 + (self.pts[i][1] - y) ** 2)
        return self.s[best]


def path_distance(path, x, y):
    best = float('inf')
    for (ax, ay), (bx, by) in zip(path, path[1:]):
        dx, dy = bx - ax, by - ay
        t = min(max(((x - ax) * dx + (y - ay) * dy) / max(dx * dx + dy * dy, 1e-6), 0.0), 1.0)
        best = min(best, math.dist((x, y), (ax + dx * t, ay + dy * t)))
    return best


def stretch_of_wall(line, feature, points, margin):
    """The stretch of the wall line the run may take, (start, end) as distances along it (end past start, maybe past
    the line's length where it wraps), round the group's dressing points: the line less the feature's cliffGaps and its
    ramp's walkway (with margin either side). None when none is free."""
    gaps = [(g['center'], g['radius']) for g in feature.get('cliffGaps', [])]
    ramp = feature.get('ramp')

    def free(i):
        x, y = line.pts[i]
        if any(math.dist((x, y), c) < r for c, r in gaps):
            return False
        return not (ramp and ramp.get('path') and path_distance(ramp['path'], x, y) < ramp['width'] * 0.5 + margin)
    ok = [free(i) for i in range(len(line.pts))]
    if all(ok):
        start = line.nearest(*points[0]['location'][:2]) if points else 0.0
        return start, start + line.length
    if not any(ok):
        return None
    # Spans of free points, walked from a blocked one so none wraps.
    first = ok.index(False)
    order = list(range(first, len(ok))) + list(range(first))
    spans, current = [], None
    for k, i in enumerate(order):
        if ok[i] and current is None:
            current = [k, k]
        elif ok[i]:
            current[1] = k
        elif current is not None:
            spans.append(current)
            current = None
    if current is not None:
        spans.append(current)
    base = line.s[first]

    def along(k):
        i = order[k]
        s = line.s[i]
        return s if s >= base else s + line.length
    marks = []
    for p in points:
        s = line.nearest(*p['location'][:2])
        marks.append(s if s >= base else s + line.length)
    best = max(spans, key=lambda sp: (sum(along(sp[0]) <= m <= along(sp[1]) for m in marks), sp[1] - sp[0]))
    return along(best[0]), along(best[1])


class Run:
    """One group's run: its settings, the wall line, the terrain's traces and what the build gives it."""

    def __init__(self, build, group, spec, meshes, tiles, terrain_hit, wall_lean):
        self.build, self.group, self.tiles = build, group, tiles
        self.terrain_hit, self.wall_lean = terrain_hit, wall_lean
        self.spec = dict(DEFAULTS, **{k: v for k, v in spec.items()
                                      if k not in ('pieces', 'sequence', 'seam', 'talus')})
        self.seam_spec = dict(SEAM_DEFAULTS, **spec.get('seam', {})) if spec.get('seam') else None
        self.pieces = {}
        for name, look in spec.get('pieces', {}).items():
            mesh = unreal.load_asset(meshes[name])
            box = mesh.get_bounding_box()
            width = box.max.y - box.min.y
            self.pieces[name] = dict(mesh=mesh, height=box.max.z, width=width, front=look['front'],
                                     lean=look.get('lean', 0.0), end=look.get('end', min(240.0, width * 0.24)))
        self.sequence = [(name, bool(mirror)) for name, mirror in spec.get('sequence', [])] or \
            [(name, False) for name in self.pieces]
        if self.seam_spec:
            name = self.seam_spec['piece']
            mesh = unreal.load_asset(meshes[name])
            box = mesh.get_bounding_box()
            self.seam_piece = dict(mesh=mesh, height=box.max.z, width=box.max.y - box.min.y,
                                   front=self.seam_spec.get('front', 95.0), lean=self.seam_spec.get('lean', 0.0))

    def ground(self, x, y):
        hit = self.terrain_hit(self.tiles, vec(x, y, FLOOR_REACH), vec(x, y, -FLOOR_REACH))
        return None if hit is None else hit.z

    def wall_at(self, s):
        """The wall where the line is at s: (x, y), into the pit, along, the floor and the rim's heights, or None."""
        (x, y), (nx, ny), (tx, ty) = self.line.at(s)
        floor = self.ground(x + nx * self.spec['footOut'], y + ny * self.spec['footOut'])
        rim = self.ground(x - nx * self.spec['topIn'], y - ny * self.spec['topIn'])
        if floor is None or rim is None:
            return None
        return (x, y), (nx, ny), (tx, ty), floor, rim

    def depth(self, x, y, nx, ny, z, ox=0.0, oy=0.0):
        """How far behind (x, y) along the way into the pit the terrain's wall stands at height z, on a line offset
        (ox, oy) across; None where a level line finds none."""
        sx, sy = x + ox, y + oy
        hit = self.terrain_hit(self.tiles, vec(sx + nx * 600.0, sy + ny * 600.0, z),
                               vec(sx - nx * 1500.0, sy - ny * 1500.0, z))
        if hit is None:
            return None
        return -((hit.x - sx) * nx + (hit.y - sy) * ny)

    def height_scale(self, piece, rnd, wall, ceiling, stretch, sink):
        """The piece's height scale: its top at its own share of the wall under the lip, within stretch, never into
        the lip."""
        share = rnd.uniform(*self.spec['top']) * ceiling
        scale = min(max((share * wall + sink) / piece['height'], stretch[0]), stretch[1])
        return min(scale, (ceiling * wall + sink) / piece['height'])

    def place_piece(self, piece, location, yaw, tilt, mirror, scale_z, index):
        made = self.build.place(piece['mesh'], location, yaw, label=f'Cliff_{self.group}_{index:02d}',
                                folder=f'Cliffs/{self.group}', scale=(1.0, -1.0 if mirror else 1.0, scale_z),
                                tags=('Obstacle',))
        if tilt > 0.0:
            # Pitched up its top tilts back, away from its face (+X, out of the wall), into the wall.
            made.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=tilt, yaw=yaw), False)
        return made

    def panel(self, s, name, mirror, index):
        """One panel centred at s along the line (the art's setting); the piece placed, or None."""
        piece = self.pieces[name]
        wall = self.wall_at(s)
        if wall is None:
            return None
        (x, y), (nx, ny), _, floor, rim = wall
        height = rim - floor
        if height < self.spec['minWall']:
            return None
        yaw = math.degrees(math.atan2(ny, nx))
        lean, _, ceiling = self.wall_lean(self.tiles, x, y, floor, height, yaw)[:3]
        tilt = max(lean - piece['lean'], 0.0)
        rnd = random.Random(f'{self.group} panel {index}')
        sink = rnd.uniform(*self.spec['sink'])
        scale_z = self.height_scale(piece, rnd, height, ceiling, self.spec['stretch'], sink)
        # The median front at local height h stands front - h * (tan(own lean) + tan(tilt)) in front of the pivot.
        k = math.tan(math.radians(piece['lean'])) + math.tan(math.radians(tilt))
        half = max(piece['width'] * 0.5 - piece['end'], 0.0)
        across = max(int(self.spec['across']), 2)
        rx, ry = -ny, nx
        candidates = []
        for i in range(across):
            off = -half + 2.0 * half * i / (across - 1)
            for share in self.spec['heights']:
                h = share * piece['height'] * scale_z
                behind = self.depth(x, y, nx, ny, floor - sink + h, rx * off, ry * off)
                if behind is not None:
                    candidates.append(behind + piece['front'] - k * h)
        if not candidates:
            self.build.warn(f'{self.group}: no wall found behind the panel at {s:.0f} cm along its line: left out')
            return None
        inset = min(candidates) - self.spec['proud']
        if self.first is None:
            # The run's first median front at the foot, which the seam stands proud of.
            self.first = ((x, y), (nx, ny), inset - piece['front'])
        return self.place_piece(piece, (x - nx * inset, y - ny * inset, floor - sink), yaw, tilt, mirror, scale_z,
                                index)

    def flank(self, rock, end, way):
        """Where along the line the rock (its static mesh components) first stands at or in front of the wall, going
        from searchIn inside the run's end at end out past it by seek, the way (+1 or -1) that leads toward the rock;
        None if it doesn't within that."""
        seam = self.seam_spec
        s = end - way * seam['searchIn']
        last = end + way * seam['seek']
        while (last - s) * way >= 0.0:
            wall = self.wall_at(s)
            if wall is not None:
                (x, y), (nx, ny), _, floor, rim = wall
                for share in seam['heights']:
                    z = floor + share * (rim - floor)
                    start, stop = vec(x + nx * 800.0, y + ny * 800.0, z), vec(x - nx * 1500.0, y - ny * 1500.0, z)
                    near = None
                    for component in rock:
                        hit = component.line_trace_component(start, stop, True, False, False)
                        if hit:
                            if isinstance(hit, tuple):
                                at = next((h for h in hit if isinstance(h, unreal.Vector)), None)
                            else:
                                at = hit.to_tuple()[5]
                            if at is not None:
                                d = (at - start).length()
                                near = d if near is None else min(near, d)
                    if near is None:
                        continue
                    ground = self.terrain_hit(self.tiles, start, stop)
                    if ground is None or near <= (ground - start).length() + 10.0:
                        return s
            s += way * STEP
        return None

    def seam(self, s, away):
        """The seam wedge at s along the line, its front turned turn degrees toward away (+1 or -1 along the line)."""
        seam, piece = self.seam_spec, self.seam_piece
        wall = self.wall_at(s)
        if wall is None:
            return None
        (x, y), (nx, ny), (tx, ty), floor, rim = wall
        height = rim - floor
        facing = math.degrees(math.atan2(ny, nx))
        lean, _, ceiling = self.wall_lean(self.tiles, x, y, floor, height, facing)[:3]
        ax, ay = tx * away, ty * away
        yaw = facing + seam['turn'] * (1.0 if nx * ay - ny * ax > 0.0 else -1.0)
        behind = self.depth(x, y, nx, ny, floor + seam['at'])
        if behind is None:
            self.build.warn(f'{self.group}: no wall behind the seam: left out')
            return None
        rnd = random.Random(f'{self.group} seam')
        scale_z = self.height_scale(piece, rnd, height, ceiling, seam['stretch'], seam['sink'])
        # Out of the wall at its height at; and its median front proud of the run's first front, where the wall
        # falls back into the corner by the rock (whichever stands further out).
        inset = behind - seam['out']
        if self.first is not None:
            (fx, fy), (gx, gy), front = self.first
            fx, fy = fx - gx * front, fy - gy * front
            facing_run = nx * gx + ny * gy
            if facing_run > 0.3:
                to_run = ((x - fx) * gx + (y - fy) * gy) / facing_run
                inset = min(inset, to_run - seam['proud'] + piece['front'])
        return self.place_piece(piece, (x - nx * inset, y - ny * inset, floor - seam['sink']), yaw,
                                max(lean - piece['lean'], 0.0), False, scale_z, 0)

    def lay(self, feature, points, level):
        """The seam and the run of panels. Returns how many pieces stood."""
        self.line = WallLine(feature['polygon'])
        stretch = stretch_of_wall(self.line, feature, points, self.spec['rampMargin'])
        if stretch is None:
            self.build.warn(f'{self.group}: no stretch of its wall is free of its gaps and its ramp: no panels')
            return 0
        start, end = stretch
        way, s0, s1 = 1.0, start, end
        placed = 0
        seam_at = None
        seam = self.seam_spec
        if seam:
            rock = [c for a in level if str(a.get_actor_label()) in seam.get('rock', [])
                    for c in a.get_components_by_class(unreal.StaticMeshComponent)]
            if not rock:
                self.build.warn(f'{self.group}: no {", ".join(seam.get("rock", []))} placed for its seam: none')
            else:
                # The run goes away from the rock: from the stretch's end nearer it.
                origin, _ = [a for a in level if str(a.get_actor_label()) in seam['rock']][0].get_actor_bounds(False)
                near_start = math.dist(self.line.at(start)[0], (origin.x, origin.y)) <= \
                    math.dist(self.line.at(end)[0], (origin.x, origin.y))
                way, s0, s1 = (1.0, start, end) if near_start else (-1.0, end, start)
                found = self.flank(rock, s0, -way)
                if found is None:
                    self.build.warn(f'{self.group}: its rock\'s flank wasn\'t found along the wall: no seam')
                else:
                    seam_at = found + way * seam['along']
                    s0 = found + way * seam['gap']
        # The panels from s0 toward s1, each centred where its near end overlaps the last one's far end.
        self.first = None
        s, index, previous = s0, 0, None
        while True:
            name, mirror = self.sequence[index % len(self.sequence)]
            width = self.pieces[name]['width']
            centre = s + way * (width * 0.5 - (self.spec['overlap'] if previous is not None else 0.0))
            if (s1 - centre) * way < 0.0:
                break
            index += 1
            if self.panel(centre, name, mirror, index) is not None:
                placed += 1
            previous = name
            s = centre + way * width * 0.5
        # The seam last: it stands proud of the run's first panel.
        seamed = seam_at is not None and self.seam(seam_at, way) is not None
        self.build.log(f'{self.group}: {placed} cliff panels along {abs(s1 - s0) / 100.0:.1f} m of its wall'
                       + (', and the seam by its rock' if seamed else ''))
        return placed + seamed


def feature_of(build, group):
    """The layout feature whose cliff group this is (its cliffGroup, else its id), with an outline."""
    return next((f for f in build.source.get('features', []) if f.get('cliffGroup', f.get('id')) == group
                 and f.get('polygon')), None)


def ready(build, meshes):
    """The groups level.cliffs.panels names whose pieces are all imported (the others keep their faces, with a
    warning)."""
    out = set()
    for group, spec in build.cliff_look.get('panels', {}).items():
        names = list(spec.get('pieces', {}))
        if spec.get('seam'):
            names.append(spec['seam'].get('piece', 'CliffSeam_A'))
        missing = [n for n in names if n not in meshes]
        if missing or not names:
            build.warn(f'{group}: no SM_{", SM_".join(missing) or "?"} yet: its faces stand instead of panels')
            continue
        if feature_of(build, group) is None:
            build.warn(f'{group}: no layout feature outlines its wall: its faces stand instead of panels')
            continue
        out.add(group)
    return out


def place(build, meshes, tiles, terrain_hit, wall_lean, groups=None):
    """Every group level.cliffs.panels names (and ready() finds ready, unless groups gives them) dressed with its run.
    Returns how many pieces stood. Needs the terrain and the rocks the seams meet standing (the whole build after the
    models, or "cliffs")."""
    groups = ready(build, meshes) if groups is None else groups
    if not groups or not tiles:
        return 0
    level = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
             if unreal.Name(build.tag) in a.tags]
    placed = 0
    for group in sorted(groups):
        spec = build.cliff_look['panels'][group]
        if spec.get('seam') and 'piece' not in spec['seam']:
            spec = dict(spec, seam=dict(spec['seam'], piece='CliffSeam_A'))
        run = Run(build, group, spec, meshes, tiles, terrain_hit, wall_lean)
        placed += run.lay(feature_of(build, group), build.layout.get('cliffs', {}).get(group, []), level)
    return placed
