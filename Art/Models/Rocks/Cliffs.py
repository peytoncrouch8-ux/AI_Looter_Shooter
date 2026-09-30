"""The cliff kit for the tutorial island: four large faces of weathered layered rock (RockCliff), CliffFace_A to
CliffFace_D, for the plateau's 8-10 m cliffs and the island's rim. A scripted model (see Art/README.md).

Each piece is a slab of rock whose front faces -Y. Its back is a flat plane at y = +depth/2 (it sits against the
terrain's slope), its top is broken, stepped rock around the piece's height (the plateau's grass meets it) and its
bottom sinks 1.3 m below the pivot, with fallen blocks (talus) at its foot. The pivot is on the ground, in the middle
of the width and the depth.

  piece         width  height  depth   use
  CliffFace_A   12 m   10 m    4.0 m   the main plateau face
  CliffFace_B   10 m    8 m    3.6 m   a lower plateau face
  CliffFace_C   15 m    6 m    3.2 m   wide and low: ramp sides, the rim, the lower course of a stack
  CliffFace_D    8 m   12 m    4.2 m   a tall buttress: corners, the highest spots

What makes it read as rock rather than masonry: columns of uneven width (1-5 m) between leaning, wandering joints,
some jutting out and some set back, joints that die out part way down; thick massive beds with a few thin soft
partings, folded a few degrees and thickening and pinching out along the face; diagonal fractures, wedge-shaped breaks
and a stepped, overhanging top edge with worn lips.

Tiling: every piece has the same beds (one shared table), and the folding fades out toward the piece's ends, so pieces
standing side by side at the same height carry each other's ledges on. Their ends curve back toward the back plane:
overlap neighbours by 1.5-2 m and the joint reads as a gully. For taller faces, stack them (the upper piece's buried
bottom behind the lower piece's top).

Triangles: a ~10k Nanite source with a 25% fallback (about 2.5k, the cliff budget) for Medium. Collision: three or four
convex hulls along the face and one per fallen block. Moss on up-facing rock comes from the material (MossAmount).
"""
import bisect
import math
import random

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector, noise

import looter_model as lm
import looter_textures as lt

SINK = 1.3        # how far a piece reaches below the ground (its pivot)
STEP = 0.1        # the building grid, before the mesh is reduced to its triangle budget
SOURCE_TRIS = 10000
FALLBACK = 25     # percent of the source that Medium draws
DENSITY = 180.0   # px per metre: the texture's banding at the scale of the beds (a cliff is seen from afar)

ROCK = lt.material('RockCliff', MossAmount=0.6)


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def bell(d, w):
    """1 at d = 0, falling smoothly to 0 at |d| = w."""
    t = d / w
    return (1.0 - t * t) ** 2 if abs(t) < 1.0 else 0.0


def noise1(x, seed):
    return noise.noise(Vector((x, seed * 7.31, seed * 3.17)))


def noise2(x, z, seed):
    return noise.noise(Vector((x, z, seed * 5.19)))


def make_beds():
    """The beds every piece shares, bottom to top: (bottom where pieces meet, hardness 0 soft .. 1 hard, kind). Mostly
    thick massive beds; now and then a thin soft parting between two of them (a shadowed ledge line)."""
    rnd = random.Random(2207)
    beds = []
    z = -4.0
    while z < 16.0:
        if beds and beds[-1][2] == 'massive' and rnd.random() < 0.45:
            thickness, hardness, kind = rnd.uniform(0.15, 0.4), rnd.uniform(0.0, 0.2), 'parting'
        else:
            thickness, hardness, kind = rnd.uniform(1.3, 3.2), rnd.uniform(0.55, 1.0), 'massive'
        beds.append((z, hardness, kind))
        z += thickness
    return beds, z


BEDS, BEDS_TOP = make_beds()


class Face:
    """The shape of one piece: the height of its top, and how far its front stands back from the front plane at (x, z)
    (negative: forward)."""

    def __init__(self, width, height, depth, seed):
        rnd = random.Random(seed)
        self.width, self.height, self.depth, self.seed = width, height, depth, seed
        self.front = -depth * 0.5 + 0.9
        self.back = depth * 0.5
        self.max_recess = self.back - self.front - 0.3
        half = width * 0.5
        self.end = min(2.4, width * 0.24)
        self._beds = {}

        # The beds fold a few degrees through the middle of the piece, and each boundary wanders on its own.
        self.dip = math.tan(math.radians(rnd.uniform(2.0, 4.5))) * rnd.choice((-1.0, 1.0))
        self.bed_wander = [(rnd.uniform(0.1, 0.35), rnd.uniform(0.0, 100.0)) for _ in BEDS]

        # Joints split the face into columns 1-5 m wide. They lean and wander; some die out part way down.
        self.joints = []
        x = -half + rnd.uniform(0.8, 3.0)
        while x < half - 0.8:
            reach = None if rnd.random() < 0.4 else height * rnd.uniform(0.3, 0.75)
            self.joints.append(dict(x=x, lean=math.tan(math.radians(rnd.uniform(-14.0, 14.0))),
                                    groove=rnd.uniform(0.3, 0.7), width=rnd.uniform(0.2, 0.45), reach=reach,
                                    wander=rnd.uniform(0.0, 100.0)))
            x += rnd.choice((rnd.uniform(1.0, 2.0), rnd.uniform(2.0, 3.5), rnd.uniform(3.5, 5.0)))
        columns = len(self.joints) + 1
        edges = [-half - 1.0] + [j['x'] for j in self.joints] + [half + 1.0]
        self.col_center = [(a + b) * 0.5 for a, b in zip(edges, edges[1:])]
        self.col_offset = []
        for _ in range(columns):
            roll = rnd.random()
            self.col_offset.append(rnd.uniform(-1.2, -0.5) if roll < 0.3 else
                                   rnd.uniform(0.3, 0.8) if roll < 0.55 else rnd.uniform(-0.2, 0.2))
        self.col_lean = [rnd.uniform(0.0, 0.06) for _ in range(columns)]
        self.col_turn = [rnd.uniform(-0.1, 0.1) for _ in range(columns)]
        self.col_top = []
        for _ in range(columns):
            roll = rnd.random()
            self.col_top.append(0.0 if roll < 0.35 else rnd.uniform(-0.8, -0.3) if roll < 0.65 else
                                rnd.uniform(-1.8, -0.9) if roll < 0.85 else rnd.uniform(0.1, 0.3))
        # A lip: the top bed juts out over an undercut (overhang, height of the lip).
        self.col_lip = [(rnd.uniform(0.15, 0.4), rnd.uniform(0.5, 1.0)) if rnd.random() < 0.6 else (0.0, 0.8)
                        for _ in range(columns)]
        # Short breaks in the top of wide columns: the top steps down on one side.
        self.steps = []
        for a, b in zip(edges, edges[1:]):
            if b - a > 3.0 and rnd.random() < 0.7:
                self.steps.append((rnd.uniform(a + 1.0, b - 1.0), rnd.uniform(-0.7, -0.3), rnd.choice((-1.0, 1.0))))
        # Wedge-shaped breaks in the top edge, where a wedge of rock has fallen out.
        self.notches = [(rnd.uniform(-half + 1.5, half - 1.5), rnd.uniform(0.6, 1.4), rnd.uniform(0.6, 1.5))
                        for _ in range(rnd.choice((1, 1, 2)))]
        # A wedge-shaped scar lower on the face (a block that came away), pointing down.
        self.scars = []
        if rnd.random() < 0.7:
            self.scars.append((rnd.uniform(-half + 2.0, half - 2.0), rnd.uniform(0.15, 0.45) * height,
                               rnd.uniform(1.5, 3.0), rnd.uniform(0.6, 1.3), rnd.uniform(0.35, 0.7)))
        # Diagonal fractures: a crack at 30-60 degrees, the block below it shifted back a little.
        self.fractures = []
        for _ in range(rnd.choice((1, 2, 2, 3))):
            angle = math.radians(rnd.uniform(30.0, 60.0)) * rnd.choice((-1.0, 1.0))
            self.fractures.append((rnd.uniform(-half + 1.0, half - 1.0), rnd.uniform(0.15, 0.7) * height,
                                   math.cos(angle), math.sin(angle), rnd.uniform(2.0, 5.0), rnd.uniform(0.18, 0.35),
                                   rnd.uniform(0.12, 0.22), rnd.uniform(0.08, 0.22)))

    # --- joints and columns ---

    def joint_x(self, joint, z):
        return joint['x'] + joint['lean'] * (z - self.height * 0.5) + 0.2 * noise1(z / 3.0 + joint['wander'], self.seed)

    def joint_strength(self, joint, z):
        """1 where the joint is open, fading to 0 below where it dies out."""
        return 1.0 if joint['reach'] is None else smoothstep(joint['reach'] - 0.6, joint['reach'] + 0.3, z)

    def per_column(self, x, z, values, sharp=0.25):
        """A value per column, changing across each joint (gradually where the joint has died out)."""
        value = values[0]
        for j, joint in enumerate(self.joints):
            w = sharp + 2.5 * (1.0 - self.joint_strength(joint, z))
            value += (values[j + 1] - values[j]) * smoothstep(-w, w, x - self.joint_x(joint, z))
        return value

    def ends(self, x):
        """0 in the middle of the piece, rising to 1 where its ends curve back."""
        return smoothstep(self.width * 0.5 - self.end, self.width * 0.5, abs(x))

    def top(self, x):
        """The height of the top at x: stepped column by column, broken by notches, lower where the ends curve back."""
        z = self.height + 0.1 * noise1(x / 2.2, self.seed + 9) + self.per_column(x, self.height, self.col_top, 0.18)
        for sx, delta, side in self.steps:
            z += delta * smoothstep(-0.15, 0.15, (x - sx) * side)
        for nx, nw, nd in self.notches:
            z -= nd * max(0.0, 1.0 - abs(x - nx) / nw) ** 1.2
        return z - 0.9 * self.ends(x) ** 2

    # --- beds ---

    def bed_bounds(self, x):
        """The bottoms of the beds at x (and the top of the last): folded and wandering in the middle of the piece,
        at the shared heights at its ends. A thin bed whose bounds meet has pinched out."""
        if x in self._beds:
            return self._beds[x]
        t = min(max((x + self.width * 0.5) / self.width, 0.0), 1.0)
        env = math.sin(math.pi * t) ** 0.7
        fold = (self.dip * x + 0.2 * noise1(x / 4.0 + 3.0, self.seed)) * env
        bounds, previous = [], -1e9
        for (base, hardness, kind), (amp, offset) in zip(BEDS, self.bed_wander):
            b = max(base + fold + amp * noise1(x / 3.0 + offset, self.seed) * env, previous)
            bounds.append(b)
            previous = b
        bounds.append(max(BEDS_TOP + fold, previous))
        self._beds[x] = bounds
        return bounds

    def bed_recess(self, x, z):
        bounds = self.bed_bounds(x)
        k = min(max(bisect.bisect_right(bounds, z) - 1, 0), len(BEDS) - 1)
        bottom, top = bounds[k], bounds[k + 1]
        _, hardness, kind = BEDS[k]
        u = (z - bottom) / (top - bottom) if top - bottom > 1e-3 else 0.5
        r = 0.1 * noise1(x / 2.0 + k * 3.3, self.seed) * (0.5 + (1.0 - hardness))
        if kind == 'parting':
            # A parting comes and goes along the face: here a deep shadow line, there closed up.
            open_ = smoothstep(-0.25, 0.25, noise1(x / 3.5 + k * 7.7, self.seed + 21) + 0.1)
            return r + (0.35 + 0.2 * (1.0 - hardness) - 0.1 * abs(2.0 * u - 1.0)) * open_ + 0.15 * (1.0 - open_)
        return r + (1.0 - hardness) * 0.3 + 0.08 * abs(2.0 * u - 1.0) ** 6

    # --- the front ---

    def recess(self, x, z, top):
        seed = self.seed
        # Columns: each stands forward or back, leans back a little and is turned a little.
        r = self.per_column(x, z, self.col_offset) + self.per_column(x, z, self.col_lean) * z
        r += self.per_column(x, z, self.col_turn) * (x - self.per_column(x, z, self.col_center))
        for joint in self.joints:
            s = self.joint_strength(joint, z)
            if s <= 0.0:
                continue
            d = abs(x - self.joint_x(joint, z))
            r += 0.25 * s * bell(d, 0.6)                                        # the columns' worn edges
            if d < joint['width']:
                r += joint['groove'] * s * (1.0 - d / joint['width']) ** 1.5    # the joint itself
        # Large forms, beds, weathering.
        r += 0.3 * noise2(x / 5.5, z / 9.0, seed) + 0.15 * noise2(x / 2.3, z / 4.0, seed + 1)
        r += self.bed_recess(x, z)
        groove = max(0.0, 1.0 - abs(noise1(x * 0.8 + 0.15 * noise1(z / 2.0, seed + 7), seed + 5)) / 0.06)
        r += 0.1 * groove ** 2                                                  # rain grooves, few and fine
        r += 0.08 * noise2(x / 1.4, z / 1.1, seed + 13)
        # Diagonal fractures.
        for fx, fz, dx, dz, length, depth, width, shift in self.fractures:
            px, pz = x - fx, z - fz
            along = px * dx + pz * dz
            if -0.3 < along < length + 0.3:
                across = -px * dz + pz * dx
                window = smoothstep(-0.3, 0.2, along) * (1.0 - smoothstep(length - 0.2, length + 0.3, along))
                if abs(across) < width:
                    r += depth * (1.0 - abs(across) / width) ** 1.5 * window
                r += shift * smoothstep(0.05, -0.05, across) * window
        # Wedge-shaped breaks: under each notch in the top, and scars lower down (pointing down).
        for nx, nw, nd in self.notches:
            reach = nd + 0.9
            if z > top - reach:
                f = (z - (top - reach)) / reach
                r += 0.35 * smoothstep(-0.1, 0.1, nw * 0.9 * f - abs(x - nx)) * smoothstep(0.0, 0.3, f)
        for sx, sz, sh, sw, sd in self.scars:
            if sz < z < sz + sh:
                f = (z - sz) / sh
                inside = smoothstep(-0.12, 0.12, sw * f - abs(x - sx))
                r += sd * inside * smoothstep(0.0, 0.15, f) * (1.0 - smoothstep(0.8, 1.0, f))
        # The top: a lip standing out over an undercut, worn round at the very top.
        overhang, lip = self.col_lip[self.column_at(x, top)]
        r -= overhang * smoothstep(top - lip - 0.12, top - lip + 0.05, z)
        r += 0.12 * bell(z - (top - lip - 0.35), 0.35) * (1.0 if overhang > 0.0 else 0.0)
        r += 0.2 * smoothstep(top - 0.3, top, z) ** 2
        r += 0.04 * max(z, 0.0)                                                 # leaning back
        r -= 0.4 * smoothstep(0.8, -1.0, z)                                     # the foot spreads
        r += (self.max_recess - 0.4) * self.ends(x) ** 1.4                      # the ends curve back
        return min(max(r, -1.6), self.max_recess)

    def column_at(self, x, z):
        return sum(1 for joint in self.joints if x > self.joint_x(joint, z))


# --- Fallen blocks ---

def fallen_block(size, seed):
    """An angular block broken off the face: a rough box, its facets merged and its edges worn round. Returns the
    part and the points of its collision hull."""
    rnd = random.Random(seed)
    sx, sy, sz = size, size * rnd.uniform(0.6, 0.9), size * rnd.uniform(0.45, 0.7)
    # Points scattered over a rough ellipsoid: their hull is an irregular chunk, not a box.
    points = []
    for _ in range(14):
        d = Vector((rnd.gauss(0.0, 1.0), rnd.gauss(0.0, 1.0), rnd.gauss(0.0, 1.0))).normalized()
        points.append(Vector((d.x * sx, d.y * sy, max(d.z, -0.6) * sz)) * (0.5 * rnd.uniform(0.75, 1.0)))
    bm = bmesh.new()
    for p in points:
        bm.verts.new(p)
    hull = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in hull['geom_interior'] + hull['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    bmesh.ops.dissolve_limit(bm, angle_limit=math.radians(10.0), verts=bm.verts[:], edges=bm.edges[:])
    bmesh.ops.bevel(bm, geom=list(bm.edges), offset=size * 0.09, segments=2, affect='EDGES', clamp_overlap=True)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    for v in bm.verts:
        v.co += v.co.normalized() * size * 0.025 * noise.noise(v.co * (3.0 / size) + Vector((seed, 0.0, 0.0)))
    hull_points = [v.co.copy() for v in bm.verts]
    mesh = bpy.data.meshes.new('_block')
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new('_block', mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj, hull_points


def talus(face, seed):
    """Blocks fallen at the foot of the face: two or three big ones, half buried and leaning on it, with a smaller
    piece or two beside each."""
    rnd = random.Random(seed)
    half = face.width * 0.5
    spots = []
    for x in sorted(rnd.uniform(-half + 1.4, half - 1.4) for _ in range(rnd.choice((2, 3, 3)))):
        size = min(max(face.height * rnd.uniform(0.15, 0.26), 1.2), 3.0)
        spots.append((x, size, 0.0))
        for _ in range(rnd.choice((1, 1, 2))):
            spots.append((x + rnd.choice((-1.0, 1.0)) * size * rnd.uniform(0.6, 0.95), size * rnd.uniform(0.3, 0.5),
                          size * rnd.uniform(0.1, 0.4)))
    blocks = []
    for x, size, forward in spots:
        if abs(x) > half - 0.6:
            continue
        part, points = fallen_block(size, rnd.randint(0, 10 ** 6))
        # Lean it on the most forward rock across its width, sunk into the foot by a fifth of its depth.
        foot = min(face.front + face.recess(sx, sz, face.top(sx))
                   for sx in (x - size * 0.4, x, x + size * 0.4) for sz in (0.0, 0.4, 0.9))
        matrix = Matrix.LocRotScale(Vector((x, foot - size * 0.3 - forward, size * rnd.uniform(0.22, 0.32))),
                                    Euler((math.radians(rnd.uniform(-25.0, 25.0)), math.radians(rnd.uniform(-25.0, 25.0)),
                                           rnd.uniform(0.0, 2.0 * math.pi))), None)
        part.data.transform(matrix)
        # Big blocks get a collision hull; the player steps over the small pieces.
        blocks.append((part, [matrix @ p for p in points] if size >= 0.7 else None))
    return blocks


# --- Building ---

def build(name, width, height, depth, seed, material_seed):
    face = Face(width, height, depth, seed)
    half = width * 0.5
    columns = int(round(width / STEP)) + 1
    xs = [-half + width * i / (columns - 1) for i in range(columns)]
    rows_front = int(round((height + SINK) / STEP)) + 1
    rows_cap, rows_back, rows_bottom = 16, 4, 4
    zb = -SINK

    profiles = []   # per column: the closed loop of points (front up, over the top, down the back, along the bottom)
    for x in xs:
        top = face.top(x)
        loop = []
        for j in range(rows_front):
            z = zb + (top - zb) * j / (rows_front - 1)
            loop.append(Vector((x, face.front + face.recess(x, z, top), z)))
        y_top = loop[-1].y
        for k in range(1, rows_cap + 1):
            t = k / rows_cap
            y = y_top + (face.back - y_top) * t
            z = top + 0.07 * noise.noise(Vector((x * 1.3, y * 1.3, seed + 3.0))) * (1.0 - t) - 0.35 * t ** 3
            loop.append(Vector((x, y, z)))
        z_back = loop[-1].z
        for k in range(1, rows_back):
            loop.append(Vector((x, face.back, z_back + (zb - z_back) * k / rows_back)))
        y_foot = loop[0].y
        for k in range(rows_bottom + 1):
            loop.append(Vector((x, face.back + (y_foot - face.back) * k / (rows_bottom + 1), zb)))
        profiles.append(loop)

    bm = bmesh.new()
    verts = [[bm.verts.new(p) for p in loop] for loop in profiles]
    count = len(profiles[0])
    for i in range(columns - 1):
        a, b = verts[i], verts[i + 1]
        for j in range(count):
            n = (j + 1) % count
            bm.faces.new((a[j], b[j], b[n], a[n]))
    caps = [bm.faces.new(verts[0]), bm.faces.new(list(reversed(verts[-1])))]
    bmesh.ops.triangulate(bm, faces=caps, ngon_method='BEAUTY')   # a zigzag across the thin end, no slivers
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)

    blocks = talus(face, seed + 101)
    parts = [obj] + [part for part, _ in blocks]
    with bpy.context.temp_override(active_object=obj, object=obj, selected_objects=parts,
                                   selected_editable_objects=parts):
        bpy.ops.object.join()
    obj.name = name
    obj.data.name = name

    clean(obj)
    reduce(obj, SOURCE_TRIS)
    clean(obj)
    lm.smooth(obj, 55.0)
    lt.assign(obj, ROCK)
    lt.box_uv(obj, 'RockCliff', texel_density=DENSITY, seed=material_seed)
    lt.bake_vertex_ao(obj, distance=2.0)
    obj['Fallback'] = FALLBACK
    hulls(obj, face, profiles, xs)
    for _, points in blocks:
        if points:
            hull(obj, points)
    return obj


def sliver(face):
    """A face with no area to speak of for its size (float noise on collinear points is about 1e-6)."""
    longest = max(e.calc_length() for e in face.edges)
    return face.calc_area() < max(1e-4 * longest * longest, 1e-9)


def clean(obj):
    """Welds duplicate vertices and removes zero-area faces (collinear points on the end caps, collapses left by the
    reduction): they have no tangents in Unreal."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-4)
    bmesh.ops.dissolve_degenerate(bm, dist=1e-4, edges=bm.edges[:])
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 3])
    # Slivers (three points on a line) have no short edge to collapse: turning their long edge makes two proper
    # triangles of the sliver and its neighbour.
    for _ in range(6):
        slivers = [f for f in bm.faces if sliver(f)]
        if not slivers:
            break
        edges = {max(f.edges, key=lambda e: e.calc_length()) for f in slivers}
        bmesh.ops.rotate_edges(bm, edges=[e for e in edges if len(e.link_faces) == 2], use_ccw=False)
        bmesh.ops.dissolve_degenerate(bm, dist=1e-4, edges=bm.edges[:])
        bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 3])
    # Whatever is left lies wholly on a line (the thin end caps, where the reduction lines points up): it covers
    # nothing, so it goes (a slit of zero width on the hidden end).
    flat = [f for f in bm.faces if sliver(f)]
    if flat:
        bmesh.ops.delete(bm, geom=flat, context='FACES_ONLY')
    bm.to_mesh(obj.data)
    bm.free()


def reduce(obj, target):
    """Collapses the building grid to about target triangles; the ledges and joints keep their vertices."""
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    if tris <= target:
        return
    mod = obj.modifiers.new('Reduce', 'DECIMATE')
    mod.ratio = target / tris
    mod.use_collapse_triangulate = True
    with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj], selected_editable_objects=[obj]):
        bpy.ops.object.modifier_apply(modifier=mod.name)


def hull(obj, points):
    """A convex collision hull of obj around points."""
    bm = bmesh.new()
    for p in points:
        bm.verts.new(p)
    result = bmesh.ops.convex_hull(bm, input=bm.verts)
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    mesh = bpy.data.meshes.new('UCX_' + obj.name)
    bm.to_mesh(mesh)
    bm.free()
    part = bpy.data.objects.new('UCX_' + obj.name, mesh)
    bpy.context.scene.collection.objects.link(part)
    part.parent = obj
    part.display_type = 'WIRE'


def hulls(obj, face, profiles, xs):
    """Convex hulls along the face, each about 4 m wide: slices every metre follow the front's outline (its most
    forward point in each height band) and close against the back plane."""
    pieces = max(2, math.ceil(face.width / 4.2))
    for c in range(pieces):
        x0 = -face.width * 0.5 + face.width * c / pieces
        x1 = -face.width * 0.5 + face.width * (c + 1) / pieces
        slices = max(3, int(round((x1 - x0) / 1.0)) + 1)
        points = []
        for s in range(slices):
            xs_ = x0 + (x1 - x0) * s / (slices - 1)
            near = [i for i, x in enumerate(xs) if abs(x - xs_) <= 0.5]
            top = max(p.z for i in near for p in profiles[i])
            bands = [-SINK, 0.0, 0.5] + [top * f for f in (0.2, 0.35, 0.5, 0.65, 0.8, 0.92)] + [top]
            for z in bands:
                candidates = [p.y for i in near for p in profiles[i] if abs(p.z - z) <= 0.45 and p.y < face.back - 0.05]
                if candidates:
                    points.append(Vector((xs_, min(candidates), z)))
            points.append(Vector((xs_, face.back, top)))
            points.append(Vector((xs_, face.back, -SINK)))
        hull(obj, points)


PIECES = [
    ('CliffFace_A', 12.0, 10.0, 4.0, 11, 1),
    ('CliffFace_B', 10.0, 8.0, 3.6, 23, 2),
    ('CliffFace_C', 15.0, 6.0, 3.2, 37, 3),
    ('CliffFace_D', 8.0, 12.0, 4.2, 53, 4),
]
models = [build(*piece) for piece in PIECES]


def preview_as_placed(model, path, **options):
    """lt.preview stands its ground under the lowest point; this shows the piece as placed, cut at its pivot's
    ground (on a temporary copy)."""
    copy = model.copy()
    copy.data = model.data.copy()
    bpy.context.scene.collection.objects.link(copy)
    bm = bmesh.new()
    bm.from_mesh(copy.data)
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=(0.0, 0.0, 0.0),
                           plane_no=(0.0, 0.0, 1.0), clear_inner=True)
    bm.to_mesh(copy.data)
    bm.free()
    lt.preview([copy], path, **options)
    mesh = copy.data
    bpy.data.objects.remove(copy)
    bpy.data.meshes.remove(mesh)


if lt.want_preview():
    for model in models:
        preview_as_placed(model, lt.preview_path('RocksProps', model.name), view=(-0.6, -1.6, 0.3), fit=0.88)
