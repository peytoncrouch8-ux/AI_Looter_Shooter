"""The ground past a grounded area's playable boundary (numpy only; Docs/Areas/RansomsRest.md: "Nothing past the
boundary can be walked"). Every closed edge stands at the foot of rock too steep to climb, except the open edges (a drop
that is part of play) and the blocked ones (layout.json boundary.blocked: runs of edges closed by something built, as
the parked train closes Stage Gap's mouth). The rest are "rock" edges.

The regional ridges' bands make most of that rock, but the ridges are laid out by their feet and the boundary is the
approved map, so in places a band stands back from the line or fades out. boundary.foot makes the rock a rule: past
every rock edge the ground stays level for "setback" (cm), then rises at "slope" (degrees) by at least "height" (cm),
and on behind it at "back" (degrees), easing back into the ground by "reach" (cm). foot() lifts the core's heights to
that profile, measured from the line's own ground, and never lowers them, so a band already higher stays as it is.
Each edge's rock stands straight out from it, so the rule holds along every edge's normal. Round a convex corner the
rock follows the corner, and it rounds off where the next edge is open or blocked (Stage Gap's mouth stays clear for the
line); in a reflex corner both edges' rock meets without a step. It never builds out over a drop: toward the
escarpment's lip it fades, and ground already well below the line (a gorge past a falls) is left.

rock_faces() dresses that rock as the cliff group "boundaryFoot", all along it: every DRESS_STEP m along each rock edge
and in each convex rock corner, a point at the foot of the first rock wall out from the line (the foot's rise and a
ridge's band where it stands close behind are one wall, up to where it eases), so the kit's pieces stand against it
from its foot. clear_of_rock() takes the ridges' own band points out of that stretch, so nothing doubles up there.

samples() measures what a player meets, every SAMPLE_STEP m along each rock edge: the rise from the line to RISE_RUN m
out, as an angle, and how far out the rock's foot is (where the ground first gets steeper than WALKABLE). summary()
writes it into layout_computed.json's boundary.rise, and terrain_check.py holds every sample's rise above WALKABLE and
its foot within FOOT_WITHIN wherever the layout asks for the rule.
"""
import math

import numpy as np

import area_cliffs
from area_math import sample, smoothstep

WALKABLE = 44.0       # degrees: the player can't walk up ground steeper than this (Unreal's default is 44.8)
RISE_RUN = 5.0        # meters out from the line over which the rise is measured
SAMPLE_STEP = 5.0     # meters between samples along a rock edge
FOOT_WITHIN = 2.5     # meters: the rock's foot is at most this far past the line (where the build's walls stand)
DRESS_STEP = 8.0      # meters between the rock's dressing points along an edge
DRESS_REACH = 16.0    # meters: a wall whose foot is within this of the line is the boundary's rock (the lift's reach)
EASE_REACH = 14.0     # meters more the profile runs on, to find where a wall that starts in the reach eases
EASED = 40.0          # degrees: a wall ends where its slope eases below this
STEEP = 45.0          # degrees: ground this steep starts a wall
ROCK = 58.0           # degrees: a wall is rock where it's somewhere this steep (a ridge's slope stays under it)
MERGE = 3.0           # meters of eased ground between two walls that still makes them one
FADE = 2.0            # meters: in a reflex corner where rock meets an open or blocked edge, the rock fades over this
FOOT = {'setback': 200.0, 'slope': 65.0, 'height': 600.0, 'back': 35.0, 'reach': 1600.0}  # boundary.foot's defaults


def edge_kinds(area):
    """Each boundary edge's kind (edge i runs from corner i to corner i + 1): 'open', 'blocked' or 'rock'."""
    spec = area.layout.get('boundary', {})
    count = len(area.boundary)
    kinds = ['open' if f else 'rock' for f in area.open_edges]
    for run in spec.get('blocked', []):
        a, b = int(run['from']), int(run['to'])
        if not 0 <= a < b <= count:
            raise ValueError(f'{area.path}: boundary.blocked run {run} is outside corners 0..{count}')
        for edge in range(a, b):
            if kinds[edge] == 'open':
                raise ValueError(f'{area.path}: boundary edge {edge} is both open and blocked')
            kinds[edge] = 'blocked'
    return kinds


def outward(corners):
    """Each edge's unit normal pointing out of the polygon (layout meters, x north and y east)."""
    d = np.roll(corners, -1, axis=0) - corners
    d /= np.maximum(np.linalg.norm(d, axis=1, keepdims=True), 1e-9)
    left = np.column_stack([-d[:, 1], d[:, 0]])
    x, y = corners[:, 0], corners[:, 1]
    clockwise = float(np.sum(x * np.roll(y, -1) - np.roll(x, -1) * y)) < 0.0
    return left if clockwise else -left


def foot_spec(area):
    """boundary.foot with its defaults filled in (meters and degrees), or None when the layout doesn't ask for it."""
    spec = area.layout.get('boundary', {}).get('foot')
    if spec is None:
        return None
    full = dict(FOOT, **{k: v for k, v in spec.items() if k in FOOT})
    return {'setback': full['setback'] / 100.0, 'slope': math.tan(math.radians(full['slope'])),
            'height': full['height'] / 100.0, 'back': math.tan(math.radians(full['back'])),
            'reach': full['reach'] / 100.0}


def profile(spec, d):
    """The rock's least rise (m) over the line's ground at d meters past the line."""
    face = np.clip(d - spec['setback'], 0.0, None) * spec['slope']
    top = spec['setback'] + spec['height'] / spec['slope']
    return np.where(d < top, face, spec['height'] + (d - top) * spec['back'])


def _convex(corners, normals):
    """Whether each corner is convex (the polygon turns inward there, leaving a wedge outside it between its two
    edges' outward normals)."""
    d = np.roll(corners, -1, axis=0) - corners          # edge i's direction
    return np.einsum('ij,ij->i', d, np.roll(normals, 1, axis=0)) < 0.0   # corner k: edge k against edge k - 1's normal


def _edge_lift(area, spec, h, pts, base, i, corners, normals, kinds, convex):
    """How far edge i's rock lifts points (N, 2) past it over heights h (0 where it doesn't reach), by the distance out
    from the edge's line, over the strip straight out from the edge. Its ends: round a convex corner the rock follows
    the corner (the distance from it), and where the neighbor is open or blocked it rounds off over the corner's
    outside angle; in a reflex corner the two edges' strips overlap, so their rock meets without a step (and stands
    nearer the other edge's line there), but next to an open or blocked neighbor the strip fades over its last FADE m.
    """
    count = len(corners)
    a, b = corners[i], corners[(i + 1) % count]
    length = float(np.linalg.norm(b - a))
    u = (b - a) / length
    along, out = (pts - a) @ u, (pts - a) @ normals[i]
    s = np.arange(0.0, length + 0.25, 0.25)
    z_line = sample(h, a[0] + u[0] * s, a[1] + u[1] * s, area.half)  # the line's own ground
    w = np.zeros(len(pts))
    dist = np.full(len(pts), np.inf)
    strip = (along >= 0.0) & (along <= length) & (out > 0.0)
    dist[strip], w[strip] = out[strip], 1.0
    for end, corner, other, sign in ((0.0, i, (i - 1) % count, -1.0), (length, (i + 1) % count, (i + 1) % count, 1.0)):
        rock_next = kinds[other] == 'rock'
        past = sign * (along - end)   # meters past this end, along the edge
        if convex[corner]:
            # The wedge outside the corner: past this edge's end and short of the neighbor's own strip.
            c = corners[corner]
            o = corners[(other + 1) % count] - corners[other]
            cap = (past > 0.0) & (sign * ((pts - c) @ (o / np.linalg.norm(o))) < 0.0)
            v = pts[cap] - c
            r = np.linalg.norm(v, axis=1)
            share = np.ones(len(r))
            if not rock_next:
                # 1 along this edge's normal, 0 along the open or blocked neighbor's.
                span = math.acos(float(np.clip(normals[i] @ normals[other], -1.0, 1.0)))
                turned = np.arccos(np.clip((v / np.maximum(r[:, None], 1e-9)) @ normals[i], -1.0, 1.0))
                share = smoothstep(0.0, 1.0, 1.0 - turned / max(span, 1e-6))
            dist[cap], w[cap] = r, share
        elif not rock_next:
            w *= np.where(strip, smoothstep(0.0, FADE, -past), 1.0)
    live = (dist < spec['reach']) & (w > 1e-4)
    line = np.interp(np.clip(along[live], 0.0, length), s, z_line)
    # Out toward the reach the rock eases back into the ground behind it.
    w[live] *= 1.0 - smoothstep(0.75 * spec['reach'], spec['reach'], dist[live])
    # Ground already well below the line past it is a drop (a gorge past a falls): the rock doesn't fill it.
    w[live] *= smoothstep(-4.0, -2.0, base[live] - line)
    lift = np.zeros(len(pts))
    lift[live] = np.maximum(line + profile(spec, dist[live]) - base[live], 0.0) * w[live]
    return lift


def foot(area, h):
    """Lifts the core's heights past the rock edges to boundary.foot's profile (never lowering them; each edge's
    strip lifts on its own and the highest wins, so the rule holds straight out from every edge), fading toward the
    core's edge (the escarpment's lip: no rock builds out over the drop). Returns the heights (unchanged without
    boundary.foot)."""
    spec = foot_spec(area)
    if spec is None or area.boundary is None:
        return h
    corners = area.boundary
    kinds = edge_kinds(area)
    normals = outward(corners)
    convex = _convex(corners, normals)
    band = (area.play_edge < 0.0) & (area.play_edge > -spec['reach']) & (area.edge > 0.0)
    ii, jj = np.nonzero(band)
    pts = np.column_stack([area.grid.c[ii], area.grid.c[jj]])
    base = h[ii, jj].astype(np.float64)
    lift = np.zeros(len(pts))
    for i in range(len(corners)):
        if kinds[i] == 'rock':
            lift = np.maximum(lift, _edge_lift(area, spec, h, pts, base, i, corners, normals, kinds, convex))
    lift *= smoothstep(1.0, 4.0, area.edge[ii, jj])
    h = h.copy()
    h[ii, jj] += lift.astype(h.dtype)
    return h


def samples(area):
    """Every SAMPLE_STEP m along each rock edge (centered in it): the line's point, the rise from it to RISE_RUN m out
    along the edge's outward normal, as an angle (degrees); how far out the rock's foot is (m: the first point on that
    line where the ground itself is steeper than WALKABLE, the way the player's floor is judged, so rock met at a
    slant in a corner counts; None when there's none within RISE_RUN); and whether the line leaves the core instead
    (over the escarpment's lip: no ground to walk on)."""
    corners = area.boundary
    if corners is None:
        return []
    kinds = edge_kinds(area)
    normals = outward(corners)
    count = len(corners)
    steep = math.tan(math.radians(WALKABLE))
    d = np.arange(0.0, RISE_RUN + 1e-6, 0.1)
    e = 0.25  # meters either side for the ground's slope
    out = []
    for i in range(count):
        if kinds[i] != 'rock':
            continue
        a, b = corners[i], corners[(i + 1) % count]
        length = float(np.linalg.norm(b - a))
        n = max(1, int(round(length / SAMPLE_STEP)))
        for k in range(n):
            at = (k + 0.5) / n
            p = a + (b - a) * at
            x, y = p[0] + normals[i][0] * d, p[1] + normals[i][1] * d
            z = area.height(x, y)
            slope = np.hypot(area.height(x + e, y) - area.height(x - e, y),
                             area.height(x, y + e) - area.height(x, y - e)) / (2.0 * e)
            on_core = area.at(area.edge, x, y) > 0.0
            rise = float(z[-1] - z[0])
            climb = slope >= steep
            out.append({'edge': i, 'at': round(at, 3), 'point': p, 'angle': math.degrees(math.atan2(rise, RISE_RUN)),
                        'foot': float(d[int(np.argmax(climb))]) if climb.any() else None,
                        'drop': not bool(on_core.all())})
    return out


def passes(sample):
    """Whether a sample meets the rule: past the line the ground rises steeper than WALKABLE (over RISE_RUN m), its
    foot within FOOT_WITHIN; or the line drops off the core."""
    if sample['drop']:
        return True
    return sample['angle'] > WALKABLE and sample['foot'] is not None and sample['foot'] <= FOOT_WITHIN


def summary(area):
    """layout_computed.json's boundary.rise: the rule's numbers and every sample [edge, at, rise (degrees), foot (cm,
    or null), drop]."""
    found = samples(area)
    if not found:
        return None
    walk = [s for s in found if not s['drop']]
    feet = [s['foot'] for s in walk if s['foot'] is not None]
    return {
        'rule': foot_spec(area) is not None, 'walkable': WALKABLE, 'run': RISE_RUN * 100.0,
        'within': FOOT_WITHIN * 100.0, 'step': SAMPLE_STEP * 100.0,
        'least': round(min(s['angle'] for s in walk), 1) if walk else None,
        'farthestFoot': round(max(feet) * 100.0, 1) if len(feet) == len(walk) and feet else None,
        'failing': sum(not passes(s) for s in found),
        'samples': [[s['edge'], s['at'], round(s['angle'], 1), None if s['foot'] is None else round(s['foot'] * 100.0),
                     s['drop']] for s in found],
        'note': 'past every rock edge (closed, neither open nor blocked), every step cm along it: the rise from the '
                'line to run cm out as an angle (degrees), where the ground first gets steeper than walkable (cm out), '
                'and whether the line drops off the core; terrain_check.py holds each rise above walkable and each '
                'foot within the within cm where the layout asks for the rule (boundary.foot)'}


def _wall(area, p, direction, step=0.25):
    """The rock wall met going out from p along direction (unit, layout meters) within DRESS_REACH: (foot (2,), top
    (2,), the ground's height at each), or None. A wall starts where the ground gets steeper than STEEP (backed down to
    where it passes EASED) and ends where it eases below EASED; walls with no more than MERGE m of eased ground between
    them are one, and a wall counts only if somewhere it's steeper than ROCK (a rock face, not a ridge's slope) and
    rises at least MIN_FACE. The first such wall out from the line is the one returned. The profile stops at the core's
    edge (the escarpment's lip has its own dressing)."""
    d = np.arange(0.0, DRESS_REACH + EASE_REACH + step * 0.5, step)
    x, y = p[0] + direction[0] * d, p[1] + direction[1] * d
    z = area.height(x, y)
    on = area.at(area.edge, x, y) > 1.0
    if not on.all():
        d, x, y, z = (v[:int(np.argmin(on))] for v in (d, x, y, z))
    if len(z) < 8:
        return None
    slope = np.gradient(z, step)
    eased, steep, rock = (math.tan(math.radians(v)) for v in (EASED, STEEP, ROCK))
    walls, k = [], 0
    while k < len(slope):
        if slope[k] < steep:
            k += 1
            continue
        i0 = k
        while i0 > 0 and slope[i0 - 1] >= eased:
            i0 -= 1
        j = k
        while j < len(slope) and slope[j] >= eased:
            j += 1
        if walls and d[i0] - d[walls[-1][1]] <= MERGE:
            walls[-1][1] = j - 1
        else:
            walls.append([i0, j - 1])
        k = j
    for i0, i1 in walls:
        if d[i0] > DRESS_REACH:
            break
        if slope[i0:i1 + 1].max() >= rock and z[i1] - z[i0] >= area_cliffs.MIN_FACE:
            return np.array([x[i0], y[i0]]), np.array([x[i1], y[i1]]), float(z[i0]), float(z[i1])
    return None


def _stations(area):
    """Where the rock gets a dressing point, in order along the boundary: every DRESS_STEP m along each rock edge
    (looking straight out from it), and at each convex corner between two rock edges (looking out along the corner's
    bisector, into the wedge where the rock follows the corner). [(point, direction)]."""
    corners = area.boundary
    kinds = edge_kinds(area)
    normals = outward(corners)
    convex = _convex(corners, normals)
    count = len(corners)
    out = []
    for i in range(count):
        if kinds[i] != 'rock':
            continue
        if convex[i] and kinds[(i - 1) % count] == 'rock':
            bisector = normals[(i - 1) % count] + normals[i]
            out.append((corners[i], bisector / max(float(np.linalg.norm(bisector)), 1e-9)))
        a, b = corners[i], corners[(i + 1) % count]
        n = max(1, int(round(float(np.linalg.norm(b - a)) / DRESS_STEP)))
        out += [(a + (b - a) * (k + 0.5) / n, normals[i]) for k in range(n)]
    return out


def rock_faces(area):
    """The cliff group "boundaryFoot": the rock wall past every rock edge, dressed all along it. Each point stands at
    the wall's foot (where it meets the ground below it, a little past the line), faces the line, and reaches up to
    where the wall eases (the foot's rise, and a ridge's band where it stands close behind), in stacked courses over the
    kit's tallest piece; the pieces stand against the wall from its foot as they do on a plateau's cliff. Empty without
    boundary.foot."""
    if foot_spec(area) is None or area.boundary is None:
        return []
    points = []
    for p, direction in _stations(area):
        wall = _wall(area, p, direction)
        if wall is None:
            continue
        foot, top, z_foot, z_top = wall
        point = {'location': [area_cliffs._cm(foot[0]), area_cliffs._cm(foot[1]), area_cliffs._cm(z_foot)],
                 'top': area_cliffs._cm(z_top), 'height': area_cliffs._cm(z_top - z_foot),
                 'yaw': area_cliffs._yaw(*-direction)}
        stacked = area_cliffs.courses(area, foot, top, z_foot, z_top)
        if stacked:
            point['courses'] = stacked
        points.append(point)
    return points


def clear_of_rock(area, points):
    """A ridge band's dressing points, less those standing where boundary.foot's rock is dressed (straight out from a
    rock edge or in a convex rock corner's wedge, within DRESS_REACH of the line): rock_faces() covers that wall from
    its foot, and the foot's lift has raised the ground in front of the band there. All of them without the rule."""
    if foot_spec(area) is None or area.boundary is None or not points:
        return points
    corners = area.boundary
    kinds = edge_kinds(area)
    normals = outward(corners)
    convex = _convex(corners, normals)
    count = len(corners)
    xy = np.array([p['location'][:2] for p in points], dtype=np.float64) / 100.0
    dressed = np.zeros(len(points), dtype=bool)
    for i in range(count):
        if kinds[i] != 'rock':
            continue
        a, b = corners[i], corners[(i + 1) % count]
        length = float(np.linalg.norm(b - a))
        along, out = (xy - a) @ ((b - a) / length), (xy - a) @ normals[i]
        dressed |= (along >= 0.0) & (along <= length) & (out > 0.0) & (out <= DRESS_REACH)
        if convex[i] and kinds[(i - 1) % count] == 'rock':
            # The wedge outside the corner: past the end of the edge before it and short of this edge's own strip.
            before = corners[(i - 1) % count]
            past = (xy - a) @ ((a - before) / max(float(np.linalg.norm(a - before)), 1e-9)) > 0.0
            dressed |= past & (along < 0.0) & (np.linalg.norm(xy - a, axis=1) <= DRESS_REACH)
    return [p for p, gone in zip(points, dressed) if not gone]
