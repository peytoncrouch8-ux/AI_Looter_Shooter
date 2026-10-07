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
escarpment's lip it fades, and ground already well below the line (a gorge past a falls) is left. Where the lift stands
proud of the ground, foot_faces() dresses it as the cliff group "boundaryFoot".

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
MIN_LIFT = 2.0        # meters: raised rock this proud of the ground gets cliff dressing of its own
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
    core's edge (the escarpment's lip: no rock builds out over the drop). Keeps the lift as a raster (area.foot_lift)
    for foot_faces(). Returns the heights (unchanged without boundary.foot)."""
    spec = foot_spec(area)
    area.foot_lift = None
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
    area.foot_lift = np.zeros(h.shape, dtype=np.float32)
    area.foot_lift[ii, jj] = lift
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


def foot_faces(area):
    """The rock boundary.foot raised, as a cliff group: every CLIFF_STEP m along each rock edge where the lift stands
    at least MIN_LIFT m proud of the ground at the face's middle (elsewhere a ridge's band was there already, dressed
    with its own group), facing the line."""
    spec = foot_spec(area)
    lift = getattr(area, 'foot_lift', None)
    if spec is None or lift is None:
        return []
    corners = area.boundary
    kinds = edge_kinds(area)
    normals = outward(corners)
    half = 0.5 * spec['height'] / spec['slope']
    middle = spec['setback'] + half
    points = []
    for i in range(len(corners)):
        if kinds[i] != 'rock':
            continue
        a, b = corners[i], corners[(i + 1) % len(corners)]
        length = float(np.linalg.norm(b - a))
        count = max(1, int(round(length / area_cliffs.CLIFF_STEP)))
        for k in range(count):
            c = a + (b - a) * (k + 0.5) / count + normals[i] * middle
            if area.at(lift, *c) < MIN_LIFT or area.at(area.edge, *c) < 2.0:
                continue
            point = area_cliffs.face(area, c, -normals[i], half + 0.6, half + 0.6)
            if point:
                points.append(point)
    return points
