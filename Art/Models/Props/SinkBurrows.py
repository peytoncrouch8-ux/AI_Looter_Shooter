"""The Sink's spider dens (Docs/Archive/Work/2026-10-31-SinkDens/CONTRACT.md): burrow mouths in the pit walls that the floor's
spiders crawl out of when the player reaches the floor. A scripted model file (Art/README.md). Two shapes:

  SpiderBurrow_A  a low, wide mouth 1.70 x 0.95 m under a heavy brow of bedded rock, a sill of two flat slabs where the
                  spiders come out, an apron of rock spilling 1 m down the wall under it
  SpiderBurrow_B  a taller, leaning mouth 1.70 x 0.95 m, its lip broken away at the upper right (the piece lies on the
                  apron below), a rougher outline

Both: pivot (and SOCKET_Mouth) at the opening's centre on the wall plane, the front (-Y in Blender, +X in Unreal)
pointing out of the wall, +Z up. The opening's outline is the inner edge of the lip, exactly 1.70 x 0.95 m (a spider
1.25 m across squeezes out). A rock collar stands round it: its crest 15-35 cm proud of the wall (most under the brow,
least on the sill), crowned with jointed chunks of the walls' bedded rock that overhang the opening a little, blending
outward into a skirt whose edge curls 25 cm back into the wall, so a lumpy wall behind it never shows a seam.

The wall is not cut: its surface runs through the pivot plane, so all of the hole that shows lies in front of it. The
throat runs from the lip back to a near-black plug 3 cm out from the wall plane (the terrain never shows through the
mouth; the spiders come out of the black), darkening fast as it goes in (baked into the vertex occlusion, which
BurrowThroat's DiffuseAO 1 shows in sunlight too). Behind the plug the throat goes on 2.2 m into the wall, tapering,
for the record (never seen). Place the pivot at the most forward wall point across the opening (a few traces).

Silk (the Sink's web material, exactly as Sink.py makes it): strands across the upper part of the mouth and a few
broken ones hanging, threads glued out over the collar, a torn funnel lining just inside (the Sheet cell; the floor of
the throat left bare where the spiders walk out) and a mat of sheet web spilling over the sill and down the apron,
narrowing to a tongue. Cards are trimmed to the atlas's own alpha, so no cell is drawn empty.

Materials (slots): RockCliff (the walls' own rock: Cliffs.py's material, MI_RockCliff; the area build swaps it to
MI_RockCliff_Ransom), BurrowThroat (new: the RockCliff set, darker, no moss, DiffuseAO 1), Web (MI_Web).
No collision (the spiders move by code). No Nanite (a masked silk slot), LODs at 50% and 25%. Under 6k triangles.

    Tools\\artrun.ps1 -Script Art\\Models\\Props\\SinkBurrows.py -Preview
Previews: Saved/ArtPreviews/RansomsRest/SinkBurrows/<Model>_front.png, _quarter.png, _close.png, _spider.png
(a stand-in 1.25 m across half out of the mouth), _shade.png (the wall in shade).
"""
import math
import random

import bmesh
import bpy
import numpy as np
from mathutils import Euler, Matrix, Vector, noise
from mathutils.bvhtree import BVHTree

import looter_model as lm
import looter_textures as lt
import looter_webs as lw

lw.register()
# The walls' rock, exactly as Cliffs.py makes it (same name and properties: one shared instance, MI_RockCliff).
ROCK = lt.material('RockCliff', MossAmount=0.6)
# The throat: the same rock a little darker and cooler, no moss on its floor, and its baked occlusion darkening it in
# direct light as well as sky light (DiffuseAO 1), so the fade to black holds when the sun looks in.
THROAT = lt.material('RockCliff', name='BurrowThroat', tint=0xb4a89a, MossAmount=0.0, DiffuseAO=1.0)
# Ransom's Rest's rock (layout.json MI_RockCliff_Ransom: Tint 0.66, 0.58, 0.5 linear), for the previews only: the area
# build swaps MI_RockCliff to it, so the burrows are judged against the wall as it is in the game.
RANSOM_TINT = 0xd5c9bb
# The Sink's web material, exactly as Sink.py makes it (one shared instance, MI_Web).
WEB = lt.material('Webs', name='Web', master='WorldFoliage', WindStrength=4.0, WindSpeed=1.1, DiffuseAO=0.6)
UP = Vector((0.0, 0.0, 1.0))
FRONT = Vector((0.0, -1.0, 0.0))
OUT = FRONT                 # out of the wall
PREVIEW = 'RansomsRest/SinkBurrows'
COLUMNS = 64                # round the mouth
PLUG = 0.03                 # the plug's distance out from the wall plane (m)


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def at(x, out, z):
    """A point given across (x), out of the wall and up, in Blender's axes."""
    return Vector((x, -out, z))


# --- The mouth's outline ---

class Mouth:
    """The opening's outline in the wall plane (x, z), its smoothed outward normals, and the collar's per-column numbers
    (crest height, skirt width), all as functions of the angle round the mouth."""

    def __init__(self, spec):
        self.spec = spec
        seed = spec['seed']
        a, b, p = 1.0, spec['height'] / spec['width'], spec['p']
        samples = 720
        raw = []
        for i in range(samples):
            th = 2.0 * math.pi * i / samples
            c, s = math.cos(th), math.sin(th)
            r = (abs(c / a) ** p + abs(s / b) ** p) ** (-1.0 / p)
            ring = Vector((c * 1.3, s * 1.3, seed * 0.37))
            r *= 1.0 + spec['wob'] * noise.noise(ring) + 0.025 * noise.noise(ring * 3.1 + Vector((0, 0, 5.0)))
            x, z = r * c, r * s
            if z < 0.0:
                z *= spec['sill']                          # a flatter sill where they come out
            t = math.radians(spec['tilt'])
            raw.append((x * math.cos(t) - z * math.sin(t), x * math.sin(t) + z * math.cos(t)))
        xs, zs = np.array([q[0] for q in raw]), np.array([q[1] for q in raw])
        sx = spec['width'] / (xs.max() - xs.min())
        sz = spec['height'] / (zs.max() - zs.min())
        xs = (xs - (xs.max() + xs.min()) * 0.5) * sx
        zs = (zs - (zs.max() + zs.min()) * 0.5) * sz
        self.pts = np.stack([xs, zs], axis=1)
        # Normals of a smoothed outline (a moving mean over 24 degrees), so the collar's columns never cross.
        k = 24
        pad = np.concatenate([self.pts[-k:], self.pts, self.pts[:k]])
        kernel = np.ones(2 * k + 1) / (2 * k + 1)
        smooth = np.stack([np.convolve(pad[:, 0], kernel, 'valid'), np.convolve(pad[:, 1], kernel, 'valid')], axis=1)
        tangent = np.roll(smooth, -3, axis=0) - np.roll(smooth, 3, axis=0)
        normal = np.stack([tangent[:, 1], -tangent[:, 0]], axis=1)
        self.nrm = normal / np.linalg.norm(normal, axis=1, keepdims=True)
        seg = np.linalg.norm(np.roll(self.pts, -1, axis=0) - self.pts, axis=1)
        self.arc = np.concatenate([[0.0], np.cumsum(seg)])
        self.samples = samples

    def _interp(self, table, th):
        f = (th % (2.0 * math.pi)) / (2.0 * math.pi) * self.samples
        i = int(math.floor(f)) % self.samples
        j = (i + 1) % self.samples
        t = f - math.floor(f)
        return table[i] * (1.0 - t) + table[j] * t

    def edge(self, th):
        q = self._interp(self.pts, th)
        return Vector((q[0], q[1]))

    def normal(self, th):
        q = self._interp(self.nrm, th)
        return Vector((q[0], q[1])).normalized()

    def angles_by_arc(self, count):
        """count angles spaced evenly along the outline (a wide mouth's sharp ends get as many as its long sides)."""
        total = self.arc[-1]
        out = []
        for k in range(count):
            target = total * k / count
            i = int(np.searchsorted(self.arc, target, side='right') - 1)
            i = min(i, self.samples - 1)
            t = (target - self.arc[i]) / max(self.arc[i + 1] - self.arc[i], 1e-9)
            out.append(2.0 * math.pi * (i + t) / self.samples)
        return out

    def circ(self, th, freq, salt):
        """Noise round the mouth, seamless."""
        return noise.noise(Vector((math.cos(th) * freq, math.sin(th) * freq, self.spec['seed'] * 0.91 + salt)))

    def crest(self, th):
        """How far the lip's crest stands out of the wall: most under the brow, least on the sill, broken in places."""
        spec = self.spec
        s = math.sin(th)
        c = spec['side'] + (spec['brow'] - spec['side']) * max(s, 0.0) ** 1.3 \
            - (spec['side'] - spec['sill_c']) * max(-s, 0.0) ** 1.2
        c += 0.035 * self.circ(th, 1.4, 1.0) + 0.015 * self.circ(th, 4.0, 2.0)
        for deg, width, drop in spec['breaks']:
            d = math.atan2(math.sin(th - math.radians(deg)), math.cos(th - math.radians(deg)))
            c -= drop * (1.0 - smoothstep(width * 0.5, width, abs(d)))
        return min(max(c, 0.11), 0.36)

    def skirt(self, th):
        """How far the skirt reaches out over the wall from the lip: furthest below (the apron under the sill)."""
        s = math.sin(th)
        return (0.36 + 0.1 * self.circ(th, 1.1, 3.0) + 0.08 * self.circ(th, 3.2, 4.0)
                + self.spec['apron'] * max(-s, 0.0) ** 1.6 + 0.08 * max(s, 0.0))


def beds(spec):
    """The rock's bedding: horizontal beds 7-30 cm thick, each standing its own amount proud (-1..1), with 1.5 cm soft
    steps between them, like the wall's beds."""
    rnd = random.Random(spec['seed'] * 13 + 1)
    cuts, z = [], -3.0
    while z < 3.0:
        z += rnd.uniform(0.07, 0.3)
        cuts.append((z, rnd.uniform(-1.0, 1.0)))

    def offset(height):
        value = 0.0
        last = 0.0
        for cut, level in cuts:
            value += (level - last) * smoothstep(cut - 0.015, cut + 0.015, height)
            last = level
        return value
    return offset


# The collar's stations, inside out: ('in', scale toward the middle, out share of the crest, out add, darkness) for
# the throat's rings, ('lip', d, ...) for the roll over the lip (d m outside the outline) and ('out', share of the
# skirt) outside the crest. The last two stations curl back into the wall.
THROAT_RINGS = [(0.70, 0.0, PLUG, 0.96), (0.75, 0.12, 0.03, 0.9), (0.82, 0.26, 0.02, 0.78), (0.89, 0.42, 0.01, 0.55),
                (0.95, 0.62, 0.0, 0.25)]
OUT_SHARES = [0.0, 0.06, 0.14, 0.24, 0.36, 0.5, 0.66, 0.83, 1.0]
LIP = 0.62


class Collar:
    """The collar's surface: station points per column, with the noise and bedding, so the lining, the mat and the
    strands can be laid over it."""

    def __init__(self, mouth):
        self.mouth = mouth
        self.spec = mouth.spec
        self.bed = beds(self.spec)
        self.offset = Vector((self.spec['seed'] * 1.7, 3.3, 7.1))

    def rough(self, x, z, amount):
        p = Vector((x, z * 1.6, 0.0)) + self.offset
        return amount * (0.045 * noise.fractal(p * 1.3, 0.6, 2.0, 4) + 0.012 * noise.noise(p * 7.0))

    def stations(self, th):
        """The column's points from the plug ring outward (and each one's darkness): the throat, the lip's roll, the
        crest, the skirt, the curl back into the wall."""
        m = self.mouth
        e, n = m.edge(th), m.normal(th)
        # The collar's own crest stands at LIP of the lip's height: the chunks set on it make the rest, so the rim
        # reads as broken blocks, not a smooth ring.
        c, w = m.crest(th) * LIP, m.skirt(th)
        pts = []
        for scale, share, add, dark in THROAT_RINGS:
            wob = 1.0 + 0.07 * noise.noise(Vector((e.x * 2.2, e.y * 2.2, scale * 3.0)) + self.offset)
            q = e * (scale * wob)
            out = share * c + add + (self.rough(q.x, q.y, 0.35) if scale > 0.72 else 0.0)
            pts.append((at(q.x, out, q.y), dark))
        for d, share in ((0.0, 0.8), (0.03, 0.95), (0.065, 1.0)):            # the lip's roll and its crest
            q = e + n * d
            out = c * share + self.rough(q.x, q.y, 0.6) + 0.02 * self.bed(q.y)
            pts.append((at(q.x, out, q.y), 0.0))
        # The skirt dives into the wall well before its edge (slower on the apron under the sill, which carries the
        # mat), so the wall cuts it along a wandering line, never along the skirt's own outline.
        dive = 0.15 * (1.0 - 0.6 * max(-math.sin(th), 0.0) ** 1.5)
        for share in OUT_SHARES[1:]:                                       # the skirt, out to its edge
            d = 0.065 + (w - 0.065) * share
            q = e + n * d
            fall = (1.0 - share) ** 1.6
            out = c * fall - dive * share ** 1.5
            # Lumps and the wall's beds standing out in ledges; big slow swells in the outer half, so the line where
            # the skirt meets the wall wanders instead of drawing an ellipse.
            swell = 0.07 * noise.noise(Vector((q.x * 1.7, q.y * 2.2, 0.0)) + self.offset * 0.5)
            out += (self.rough(q.x, q.y, 1.2) + 0.045 * self.bed(q.y)) * (1.0 - share ** 4)
            out += swell * math.sin(math.pi * share) ** 0.7
            pts.append((at(q.x, out, q.y), 0.0))
        for d, out in ((0.05, -0.28), (0.1, -0.5)):                         # the curl back into the wall
            q = e + n * (w + d)
            pts.append((at(q.x, out, q.y), 0.0))
        return pts

    def point(self, th, s):
        """A point on the column at angle th, s a fractional station index."""
        pts = self.stations(th)
        i = min(max(int(math.floor(s)), 0), len(pts) - 2)
        t = min(max(s - i, 0.0), 1.0)
        return pts[i][0].lerp(pts[i + 1][0], t)


LIP_STATION = len(THROAT_RINGS)          # the station on the outline itself (the opening's edge)
CREST_STATION = LIP_STATION + 2


def build_collar(name, collar, angles):
    """The collar, the throat to its plug and the hidden throat behind it, as one mesh: materials RockCliff (0) and
    BurrowThroat (1); the attribute 'dark' (0..1, per vertex) is how much the throat's depth darkens it."""
    bm = bmesh.new()
    dark_layer = bm.verts.layers.float.new('dark')
    grid = []
    for th in angles:
        col = []
        for co, dark in collar.stations(th):
            v = bm.verts.new(co)
            v[dark_layer] = dark
            col.append(v)
        grid.append(col)
    rows = len(grid[0])
    n = len(grid)
    front_faces = []
    for k in range(n):
        a, b = grid[k], grid[(k + 1) % n]
        for r in range(rows - 1):
            f = bm.faces.new((a[r], b[r], b[r + 1], a[r + 1]))
            f.material_index = 1 if r < LIP_STATION - 1 else 0
            front_faces.append(f)
    # The plug: a fan over the innermost ring.
    ring = [col[0] for col in grid]
    middle = sum((v.co for v in ring), Vector()) / len(ring)
    middle.y = -PLUG
    centre = bm.verts.new(middle)
    centre[dark_layer] = 0.97
    for k in range(n):
        f = bm.faces.new((ring[(k + 1) % n], ring[k], centre))
        f.material_index = 1
        front_faces.append(f)
    # Behind the plug: the throat on 2.2 m into the wall, tapering, coarse, closed at its end. Never seen.
    back = []
    for depth, scale in ((0.0, 1.0), (0.6, 0.85), (1.4, 0.68), (2.2, 0.5)):
        row = []
        for k in range(0, n, 4):
            q = ring[k].co
            v = bm.verts.new(Vector((middle.x + (q.x - middle.x) * scale, depth + 0.0 - PLUG * (depth == 0.0),
                                     middle.z + (q.z - middle.z) * scale)))
            v[dark_layer] = 0.97
            row.append(v)
        back.append(row)
    m = len(back[0])
    for r in range(len(back) - 1):
        for k in range(m):
            f = bm.faces.new((back[r][k], back[r][(k + 1) % m], back[r + 1][(k + 1) % m], back[r + 1][k]))
            f.material_index = 1
    end = bm.verts.new(Vector((middle.x, 2.2, middle.z)))
    end[dark_layer] = 0.97
    for k in range(m):
        f = bm.faces.new((back[-1][k], back[-1][(k + 1) % m], end))
        f.material_index = 1
    bm.normal_update()
    # Turn the faces to look out of the rock. The grid and the plug are wound alike (neighbours run their shared edge
    # opposite ways), so one face on the crest where it stands out furthest, which faces the front, decides for all of
    # them. The hidden throat's faces look in at its axis.
    probe = min((f for f in front_faces if f.material_index == 0), key=lambda f: f.calc_center_median().y)
    if probe.normal.y > 0.0:
        for f in front_faces:
            f.normal_flip()
    front_set = set(front_faces)
    for f in bm.faces:
        if f in front_set:
            continue
        c = f.calc_center_median()
        if f.normal.dot(Vector((middle.x, c.y, middle.z)) - c) < 0.0:
            f.normal_flip()
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.materials.append(ROCK)
    mesh.materials.append(THROAT)
    return obj


# --- Rock chunks (Rocks.py's jointed blocks, bedded) ---

def soft_max(e, k):
    return 0.5 * (e + math.sqrt(e * e + k * k))


class Block:
    """A jointed block of bedded rock: a rounded box cut by a few worn planes and a flat bedding top and bottom."""

    def __init__(self, size, seed, cuts=5, soft=0.06, lumps=0.03, roundness=2.8):
        rnd = random.Random(seed)
        self.half = Vector(size) * 0.5
        self.mean = sum(size) / 3.0
        self.p = roundness
        self.lumps = lumps
        self.offset = Vector((rnd.uniform(0, 50), rnd.uniform(0, 50), rnd.uniform(0, 50)))
        self.planes = []
        start = rnd.uniform(0.0, 2.0 * math.pi)
        for i in range(cuts):
            az = start + i * 2.39996 + rnd.uniform(-0.3, 0.3)
            n = Vector((math.cos(az), math.sin(az), rnd.uniform(-0.35, 0.5))).normalized()
            self.planes.append((n, rnd.uniform(0.6, 0.84), soft * self.mean))
        for sign in (1.0, -1.0):                  # bedding planes, top and bottom
            n = Vector((rnd.uniform(-0.08, 0.08), rnd.uniform(-0.08, 0.08), sign)).normalized()
            self.planes.append((n, rnd.uniform(0.62, 0.75), soft * self.mean * 0.4))

    def extent(self, n):
        h = self.half
        return math.sqrt((n.x * h.x) ** 2 + (n.y * h.y) ** 2 + (n.z * h.z) ** 2)

    def point(self, d):
        h, p = self.half, self.p
        r = (abs(d.x) ** p + abs(d.y) ** p + abs(d.z) ** p) ** (-1.0 / p)
        pos = Vector((d.x * r * h.x, d.y * r * h.y, d.z * r * h.z))
        pos += d * (self.lumps * self.mean * noise.fractal(d * 1.1 + self.offset, 0.5, 2.0, 3))
        pos += d * (0.012 * self.mean * noise.noise(d * 5.0 + self.offset))
        for n, depth, k in self.planes:
            e = pos.dot(n) - depth * self.extent(n)
            pos -= n * soft_max(e, k)
        return pos


def reduce(obj, target):
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    if tris <= target:
        return
    mod = obj.modifiers.new('Reduce', 'DECIMATE')
    mod.ratio = target / tris
    mod.use_collapse_triangulate = True
    with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj],
                                   selected_editable_objects=[obj]):
        bpy.ops.object.modifier_apply(modifier=mod.name)


def block_object(name, size, seed, matrix, target, subdivisions=3, **shape):
    block = Block(size, seed, **shape)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdivisions, radius=1.0)
    for v in bm.verts:
        v.co = matrix @ block.point(v.co.normalized())
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    reduce(obj, target)
    lm.smooth(obj, 55.0)
    mesh.materials.append(ROCK)
    return obj


def lip_chunks(spec, mouth, collar):
    """The broken lip: blocks set round the crest, half sunk in the collar and overhanging the opening a little."""
    parts = []
    for i, (deg, size, yaw, pitch, overhang) in enumerate(spec['chunks']):
        th = math.radians(deg)
        e, n = mouth.edge(th), mouth.normal(th)
        c = mouth.crest(th)
        ext = abs(n.x) * size[0] * 0.5 + abs(n.y) * size[2] * 0.5
        centre2 = e + n * (ext * (1.0 - overhang))
        out = c + 0.03 - size[1] * 0.5 * 0.72          # its front at the lip's height (the cuts take ~28% off)
        matrix = Matrix.LocRotScale(at(centre2.x, out, centre2.y),
                                    Euler((math.radians(pitch), 0.0, math.radians(yaw))), None)
        parts.append(block_object(f'_Chunk{i}', size, spec['seed'] * 100 + i, matrix, spec.get('chunk_tris', 160)))
    return parts


def ray_bvh(objects):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    verts, polys = [], []
    for o in objects:
        mesh = o.evaluated_get(depsgraph).to_mesh()
        base = len(verts)
        verts.extend(o.matrix_world @ v.co for v in mesh.vertices)
        polys.extend([base + i for i in p.vertices] for p in mesh.polygons)
        o.evaluated_get(depsgraph).to_mesh_clear()
    return BVHTree.FromPolygons(verts, polys, epsilon=0.0)


def fallen_and_grit(spec, mouth, collar, tree):
    """The piece broken off the lip lying on the apron (B), and grit and pebbles on the sill and trickled down the
    apron: what the spiders' coming and going knocks loose."""
    rnd = random.Random(spec['seed'] * 7 + 3)
    parts = []
    for i, (x, z, size, yaw) in enumerate(spec.get('fallen', ())):
        hit = tree.ray_cast(Vector((x, -3.0, z)), Vector((0.0, 1.0, 0.0)), 5.0)
        if hit[0] is None:
            continue
        loc = hit[0] + hit[1] * (size[2] * 0.15)
        rot = Vector((0.0, 0.0, 1.0)).rotation_difference(hit[1]).to_euler()
        rot.rotate(Euler((0.0, 0.0, math.radians(yaw))))
        parts.append(block_object(f'_Fallen{i}', size, spec['seed'] * 300 + i, Matrix.LocRotScale(loc, rot, None),
                                  170))
    sill_z = min(mouth.edge(th).y for th in np.linspace(0.0, 2.0 * math.pi, 90))
    stones = []
    for i in range(spec.get('sill_stones', 5)):          # on the sill, near its front
        x = rnd.uniform(-0.55, 0.55)
        hit = tree.ray_cast(Vector((x, -mouth.crest(-math.pi / 2) * rnd.uniform(0.55, 1.1), sill_z + 0.4)),
                            Vector((0.0, 0.0, -1.0)), 1.0)
        if hit[0] is not None:
            stones.append((hit[0], hit[1], rnd.uniform(0.035, 0.08)))
    for i in range(spec.get('apron_stones', 7)):         # trickled down the apron, mostly in a line under the sill
        x = rnd.gauss(spec.get('mat_x', 0.0), 0.28)
        z = sill_z - rnd.uniform(0.15, 0.95)
        hit = tree.ray_cast(Vector((x, -3.0, z)), Vector((0.0, 1.0, 0.0)), 5.0)
        if hit[0] is not None and hit[1].z > -0.2:
            stones.append((hit[0], hit[1], rnd.uniform(0.025, 0.06)))
    for i, (loc, nrm, size) in enumerate(stones):
        sz = (size, size * rnd.uniform(0.7, 0.95), size * rnd.uniform(0.45, 0.65))
        rot = Vector((0.0, 0.0, 1.0)).rotation_difference(nrm).to_euler()
        rot.rotate(Euler((0.0, 0.0, rnd.uniform(0.0, 6.28))))
        parts.append(block_object(f'_Pebble{i}', sz, spec['seed'] * 500 + i,
                                  Matrix.LocRotScale(loc + nrm * (sz[2] * 0.15), rot, None), 22, subdivisions=1,
                                  cuts=2, soft=0.3, lumps=0.08, roundness=2.2))
    return parts


def join(target, parts):
    if not parts:
        return target
    objs = [target] + parts
    with bpy.context.temp_override(active_object=target, selected_editable_objects=objs, object=target,
                                   selected_objects=objs):
        bpy.ops.object.join()
    return target


# --- Silk (Sink.py's cards: the same vertex colours, normals and lanes) ---

class Card:
    """A model being built: vertices with a position, a shading normal and their vertex colour (R wind weight, G sway
    phase, A occlusion), and faces with their own UVs per corner."""

    def __init__(self, name):
        self.name = name
        self.co, self.no, self.col = [], [], []
        self.faces, self.uvs = [], []

    def vert(self, co, normal, wind=0.0, phase=0.0, occ=1.0):
        n = Vector(normal)
        self.co.append(Vector(co))
        self.no.append(n.normalized() if n.length > 1e-9 else UP.copy())
        self.col.append((min(max(wind, 0.0), 1.0), phase % 1.0, 0.0, min(max(occ, 0.0), 1.0)))
        return len(self.co) - 1

    def face(self, verts, uvs):
        verts, uvs = list(verts), [tuple(u) for u in uvs]
        p = [self.co[i] for i in verts]
        geo = (p[2] - p[0]).cross(p[-1] - p[1]) if len(p) == 4 else (p[1] - p[0]).cross(p[2] - p[0])
        if geo.dot(sum((self.no[i] for i in verts), Vector())) < 0.0:
            verts.reverse()
            uvs.reverse()
        self.faces.append(tuple(verts))
        self.uvs.append(uvs)

    def build(self):
        mesh = bpy.data.meshes.new(self.name)
        mesh.from_pydata([tuple(c) for c in self.co], [], self.faces)
        uv = mesh.uv_layers.new(name='UVMap')
        for poly, uvs in zip(mesh.polygons, self.uvs):
            for li, value in zip(poly.loop_indices, uvs):
                uv.data[li].uv = value
        mesh.materials.append(WEB)
        mesh.polygons.foreach_set('use_smooth', [True] * len(self.faces))
        loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
        mesh.loops.foreach_get('vertex_index', loop_vert)
        lt._write_col(mesh, np.asarray(self.col, np.float32)[loop_vert])
        mesh.normals_split_custom_set_from_vertices([tuple(n) for n in self.no])
        mesh.update()
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


def lean_up(surface, up=0.6):
    """A shading normal for silk lying on the rock: the rock's own normal (it never flips: silk on an apron that
    leans out over the wall below faces a little down, and Sink.py's flip for free cards turned it into the wall,
    dark) turned toward the sky by up, so dusty silk reads lighter than the rock it lies on."""
    return (Vector(surface).normalized() + UP * up).normalized()


LANE_FILL = {'Cord': (8, 24), 'Rope': (8, 24), 'Tufted': (2, 30), 'Ribbon': (6, 26)}


def lane_rows(lane):
    y0, _ = lw.LANES[lane]
    f0, f1 = LANE_FILL[lane]
    top = lw.REGIONS['Strands'][1]
    return 1.0 - (top + y0 + f0 + 0.5) / lw.SIZE, 1.0 - (top + y0 + f1 - 0.5) / lw.SIZE, (f1 - f0) / 32.0


def wiggle(points, amp, rnd, waves=1.5, keep_ends=True):
    pts = [Vector(p) for p in points]
    n = len(pts)
    ph = [rnd.uniform(0.0, 6.28) for _ in range(4)]
    out = []
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized()
        s1 = t.cross(UP if abs(t.z) < 0.9 else FRONT).normalized()
        s2 = t.cross(s1).normalized()
        f = i / max(n - 1, 1)
        ease = math.sin(math.pi * f) if keep_ends else min(1.0, 2.0 * f)
        out.append(p + (s1 * math.sin(2.0 * math.pi * waves * f + ph[0]) * 0.7 +
                        s1 * math.sin(2.0 * math.pi * waves * 2.3 * f + ph[1]) * 0.3 +
                        s2 * math.sin(2.0 * math.pi * waves * 1.3 * f + ph[2]) * 0.6) * (amp * ease))
    return out


def cord(card, points, width, lane, u0, wind, phase, occ=1.0, taper=None, normals=None):
    """A thread along a polyline on a lane of the Strands cell: two crossed strips hanging free, or one strip lying on
    a surface (normals: the surface's at each point; the strip lies across them)."""
    pts = [Vector(p) for p in points]
    count = len(pts)
    winds = list(wind) if isinstance(wind, (list, tuple)) else [wind] * count
    occs = list(occ) if isinstance(occ, (list, tuple)) else [occ] * count
    shares = [1.0] * count if taper is None else \
        [1.0 - (1.0 - taper) * (i / max(count - 1, 1)) ** 1.3 for i in range(count)]
    va, vb, share = lane_rows(lane)
    width *= share
    s = [0.0]
    for a, b in zip(pts, pts[1:]):
        s.append(s[-1] + (b - a).length)
    us = [(u0 + d * lw.PX_M) / lw.SIZE for d in s]
    frames = []
    for i in range(count):
        t = (pts[min(i + 1, count - 1)] - pts[max(i - 1, 0)]).normalized()
        if normals is not None:
            nrm = Vector(normals[i])
            frames.append((t.cross(nrm).normalized(), None, lean_up(nrm, 0.8)))
            continue
        ref = UP if abs(t.z) < 0.9 else FRONT
        s1 = t.cross(ref).normalized()
        s2 = t.cross(s1).normalized()
        up_perp = UP - t * t.dot(UP)
        front = FRONT - t * t.dot(FRONT)
        if front.length < 0.2:
            front = Vector((1.0, 0.0, 0.0)) - t * t.x
        n = up_perp + front.normalized() * 0.6
        frames.append((s1, s2, n if n.length > 1e-6 else front))
    for k in range(1 if normals is not None else 2):
        rows = []
        for i, p in enumerate(pts):
            side = frames[i][k] * (width * 0.5 * shares[i])
            rows.append((card.vert(p - side, frames[i][2], winds[i], phase, occs[i]),
                         card.vert(p + side, frames[i][2], winds[i], phase, occs[i])))
        for i in range(count - 1):
            (a0, b0), (a1, b1) = rows[i], rows[i + 1]
            card.face((a0, b0, b1, a1), [(us[i], va), (us[i], vb), (us[i + 1], vb), (us[i + 1], va)])


class Atlas:
    """The web atlas's alpha, to trim cards to the silk they show (cells with nothing over the mask aren't drawn)."""

    def __init__(self):
        image = bpy.data.images.load(lt.texture_path('Webs', 'BC'), check_existing=True)
        w, h = image.size
        px = np.empty(w * h * 4, dtype=np.float32)
        image.pixels.foreach_get(px)
        self.alpha = px.reshape(h, w, 4)[::-1, :, 3]          # row 0 at the top, as the cells are given
        self.size = w

    def covered(self, region, x0, x1, y0, y1, share=0.03):
        """Whether a cell's pixel box (x wraps along the band) holds silk."""
        rx, ry, rx1, ry1 = lw.REGIONS[region]
        width = rx1 - rx
        ys = slice(int(ry + max(min(y0, y1), 0)), int(ry + min(max(y0, y1), ry1 - ry)) + 1)
        a, b = sorted((x0, x1))
        cols = [int(rx + (x % width)) for x in np.arange(a, b + 1.0, 2.0)]
        block = self.alpha[ys][:, cols]
        return block.size > 0 and float((block > 1.0 / 3.0).mean()) > share


def band_uv(u_px, v_px):
    """UV of a point of the Sheet band (u along it in px, tiling; v across it, 0..384 px from its top)."""
    top = lw.REGIONS['Sheet'][1]
    return (u_px / lw.SIZE, 1.0 - (top + v_px) / lw.SIZE)


def band_card(card, atlas, grid, us, vs, keep_extra=None):
    """A card over a grid of points (grid[i][j] = (position, normal, wind, occ)), mapped onto the Sheet band (us[i][j]
    and vs[j]: pixels), each cell kept only if the atlas shows silk in it."""
    ids = {}

    def vid(i, j):
        if (i, j) not in ids:
            co, n, wind, occ, phase = grid[i][j]
            ids[(i, j)] = card.vert(co, n, wind, phase, occ)
        return ids[(i, j)]
    for i in range(len(grid) - 1):
        for j in range(len(vs) - 1):
            if grid[i][j] is None or grid[i + 1][j] is None or grid[i][j + 1] is None or grid[i + 1][j + 1] is None:
                continue
            if keep_extra is not None and not keep_extra(i, j):
                continue
            u0, u1 = min(us[i][j], us[i][j + 1]), max(us[i + 1][j], us[i + 1][j + 1])
            if not atlas.covered('Sheet', u0, u1, vs[j], vs[j + 1]):
                continue
            card.face((vid(i, j), vid(i + 1, j), vid(i + 1, j + 1), vid(i, j + 1)),
                      (band_uv(us[i][j], vs[j]), band_uv(us[i + 1][j], vs[j]), band_uv(us[i + 1][j + 1], vs[j + 1]),
                       band_uv(us[i][j + 1], vs[j + 1])))


def surface_at(tree, origin, direction, fallback, lift):
    """The first surface a ray meets face on (rock seen from behind is passed through), lifted off it by lift."""
    origin = Vector(origin)
    for _ in range(6):
        hit = tree.ray_cast(origin, direction, 4.0)
        if hit[0] is None:
            break
        if hit[1].dot(direction) < 0.0:
            return hit[0] + hit[1] * lift, hit[1]
        origin = hit[0] + direction * 1e-4
    return fallback, -direction


def funnel_lining(card, spec, mouth, collar, tree, atlas, rnd, phase):
    """The torn funnel just inside the mouth: the Sheet band laid round the throat from the lip's roll to near the plug,
    3-5 cm off the rock, the band's ragged edges at the lip and deep in, the throat's floor left bare (where they walk
    out) and its torn ends narrowing to strings."""
    span = spec.get('funnel', (-25.0, 205.0))         # round over the top, from low on the right to low on the left
    start, stop = math.radians(span[0]), math.radians(span[1])
    columns = 30
    rows = [float(v) for v in np.linspace(0.0, 384.0, 13)]
    s_lip, s_deep = CREST_STATION - 0.3, 1.2
    grid, us = [], []
    arc = 0.0
    last = None
    middle = Vector((0.0, 0.0, 0.0))
    for i in range(columns + 1):
        f = i / columns
        th = start + (stop - start) * f
        # Torn back in places, and narrowing at its torn ends.
        torn = noise.noise(Vector((f * 6.0, 0.5, spec["seed"] + 2.5)))
        torn_depth = 0.55 + 0.45 * smoothstep(-0.3, 0.4, torn)
        taper = (0.35 + 0.65 * smoothstep(0.0, 0.25, min(f, 1.0 - f))) * torn_depth
        column, ucol = [], []
        for v in rows:
            t = 0.5 + (v - 192.0) / 384.0 / taper
            if t < -0.15 or t > 1.15:
                column.append(None)
                ucol.append(0.0)
                continue
            s = s_lip + (s_deep - s_lip) * min(max(t, 0.0), 1.0)
            p = collar.point(th, s)
            if t < 0.0:                                     # anchor strands out over the crest
                p = collar.point(th, s_lip + 1.5 * min(-t / 0.15, 1.0))
            axis = Vector((middle.x, p.y, middle.z))
            d = (p - axis)
            d.y = 0.0
            lift = 0.03 + 0.02 * (0.5 + 0.5 * noise.noise(p * 3.0 + Vector((7.0, 1.0, spec['seed']))))
            pos, n = surface_at(tree, axis + Vector((0.0, -0.02, 0.0)), d.normalized(), p, lift)
            if t < 0.0:
                pos, n = surface_at(tree, p + Vector((0.0, -0.6, 0.0)), Vector((0.0, 1.0, 0.0)), p, 0.012)
            depth = min(max(t, 0.0), 1.0)
            occ = (1.0 - 0.85 * depth ** 1.2) * (0.86 + 0.14 * (0.5 + 0.5 * math.sin(th)))
            column.append((pos, lean_up(n, 0.3), 0.0 if t < 0.0 else 0.05 * (1.0 - depth), occ, phase))
            ucol.append(0.0)
        if last is not None:
            arc += (mouth.edge(th) - last).length
        last = mouth.edge(th)
        grid.append(column)
        us.append([spec['seed'] * 37.0 + arc * lw.PX_M] * len(rows))
    # Its tears are the felt's own holes and the depth it's torn back to (cutting whole cells out of the dense felt
    # left square holes).
    band_card(card, atlas, grid, us, rows)


def silk_mat(card, spec, mouth, collar, tree, atlas, rnd, phase):
    """The mat spilling out of the mouth: the Sheet band laid over the sill and down the apron (along the band's
    length), its ragged edges at its sides, narrowing to a tongue as it goes down."""
    x0 = spec.get('mat_x', 0.0)
    bottom = -math.pi / 2
    s_start, s_end = LIP_STATION - 1.4, CREST_STATION + 2.0 + spec.get('mat_reach', 3.6)
    steps = 13
    path = [collar.point(bottom, s_start + (s_end - s_start) * k / steps) for k in range(steps + 1)]
    rows = [float(v) for v in np.linspace(0.0, 384.0, 11)]
    grid, us = [], []
    along = 0.0
    for k, g in enumerate(path):
        if k:
            along += (g - path[k - 1]).length
        f = k / steps
        # The band's half-width (m): narrowing down the apron, then pinched to a point, the silk gathered into a thread.
        half = 1.15 * (1.0 - 0.62 * f ** 1.2) * (0.08 + 0.92 * (1.0 - smoothstep(0.62, 1.0, f)))
        tangent = (path[min(k + 1, steps)] - path[max(k - 1, 0)]).normalized()
        gn = Vector((-1.0, 0.0, 0.0)).cross(tangent).normalized()       # out of the rock: up on the sill, out below
        drift = x0 + 0.06 * noise.noise(Vector((f * 2.0, 0.3, spec['seed'])))
        column, ucol = [], []
        for v in rows:
            x = drift + (v - 192.0) / 192.0 * half
            guide = Vector((x, g.y, g.z))
            lift = 0.012 + 0.02 * (0.5 + 0.5 * noise.noise(guide * 4.0 + Vector((0.0, 3.0, spec['seed']))))
            pos, n = surface_at(tree, guide + gn * 0.4, -gn, guide, lift)
            occ = 0.8 + 0.12 * min(1.0, f * 3.0) - 0.08 * f        # dusty: darker in the mouth and low down
            column.append((pos, lean_up(n, 0.8), 0.0, occ, phase))
            ucol.append(spec['seed'] * 53.0 + along * lw.PX_M)
        grid.append(column)
        us.append(ucol)
    band_card(card, atlas, grid, us, rows)


def strands(card, spec, mouth, collar, tree, rnd):
    """Threads across the upper part of the mouth (glued to the lip's rock where a ray from the middle meets it), a few
    broken ones hanging, and threads glued out over the collar."""
    def anchor(deg, out):
        th = math.radians(deg)
        e = mouth.edge(th)
        origin = at(0.0, out, 0.0)
        target = at(e.x, out, e.y)
        hit = tree.ray_cast(origin, (target - origin).normalized(), 3.0)
        if hit[0] is None:
            return target
        return hit[0] - (target - origin).normalized() * 0.01
    for deg_a, deg_b, sag, out, lane in spec['spans']:
        a, b = anchor(deg_a, out), anchor(deg_b, out)
        pts = [a.lerp(b, i / 8.0) - UP * (4.0 * sag * (i / 8.0) * (1.0 - i / 8.0)) for i in range(9)]
        pts = wiggle(pts, 0.012, rnd)
        winds = [0.25 * 4.0 * (i / 8.0) * (1.0 - i / 8.0) for i in range(9)]
        cord(card, pts, 0.045 if lane == 'Rope' else 0.04, lane, rnd.uniform(0.0, 1500.0), winds, rnd.random(),
             occ=0.8)
    for deg, length, out, lane in spec['hanging']:
        top = anchor(deg, out)
        pts = [top + Vector((rnd.uniform(-0.03, 0.03) * t * t, -0.02 * t, -length * t)) for t in np.linspace(0, 1, 7)]
        pts = wiggle(pts, 0.012 + 0.008 * length, rnd, waves=1.0 + length, keep_ends=False)
        winds = [0.9 * (i / 6.0) ** 1.2 for i in range(7)]
        cord(card, pts, 0.04, lane, rnd.uniform(0.0, 1500.0), winds, rnd.random(), occ=[0.75] + [0.85] * 6, taper=0.15)
    for deg, reach in spec['glued']:                 # out over the crest and down the collar
        th = math.radians(deg)
        pts, nrms = [], []
        for k in range(7):
            s = CREST_STATION - 1.0 + (reach + 1.0) * k / 6.0
            g = collar.point(th + 0.05 * math.sin(k * 1.3 + deg), s)
            pos, n = surface_at(tree, g + Vector((0.0, -0.6, 0.0)) + UP * 0.05, Vector((0.0, 1.0, -0.08)).normalized(),
                                g, 0.012)
            pts.append(pos)
            nrms.append(n)
        cord(card, pts, 0.04, 'Cord', rnd.uniform(0.0, 1500.0), 0.0, rnd.random(), occ=0.8, normals=nrms)


# --- The models ---

SPECS = {
    'SpiderBurrow_A': dict(
        seed=41, width=1.70, height=0.95, p=2.5, wob=0.05, sill=0.8, tilt=3.0,
        brow=0.33, side=0.25, sill_c=0.16, breaks=[(55.0, 0.35, 0.07), (205.0, 0.3, 0.06)], apron=0.55,
        # (degrees round the mouth, size across / out / up, yaw, pitch, share overhanging the opening)
        chunks=[(92.0, (0.66, 0.42, 0.3), 6.0, 3.0, 0.3), (125.0, (0.5, 0.4, 0.36), -12.0, -4.0, 0.25),
                (62.0, (0.52, 0.38, 0.27), 14.0, 5.0, 0.2), (168.0, (0.3, 0.4, 0.48), -6.0, 0.0, 0.25),
                (10.0, (0.34, 0.42, 0.42), 9.0, -3.0, 0.2), (215.0, (0.44, 0.36, 0.3), 20.0, 4.0, 0.15),
                (328.0, (0.42, 0.36, 0.26), -18.0, -5.0, 0.15), (258.0, (0.52, 0.36, 0.13), 4.0, 2.0, 0.3),
                (290.0, (0.44, 0.32, 0.12), -9.0, -3.0, 0.3)],
        fallen=[(0.62, -1.05, (0.3, 0.24, 0.18), 30.0)],
        spans=[(150.0, 38.0, 0.16, 0.2, 'Rope'), (118.0, 66.0, 0.1, 0.24, 'Cord'), (168.0, 104.0, 0.09, 0.16, 'Tufted'),
               (82.0, 14.0, 0.13, 0.18, 'Cord')],
        hanging=[(108.0, 0.42, 0.22, 'Tufted'), (58.0, 0.28, 0.2, 'Cord'), (142.0, 0.22, 0.18, 'Ribbon')],
        glued=[(100.0, 2.0), (138.0, 1.6), (34.0, 2.2), (176.0, 1.4), (4.0, 1.5)],
        mat_x=0.05, mat_reach=3.6),
    'SpiderBurrow_B': dict(
        seed=57, width=1.70, height=0.95, p=2.15, wob=0.09, sill=0.9, tilt=-8.0,
        brow=0.35, side=0.23, sill_c=0.15, breaks=[(32.0, 0.6, 0.12), (245.0, 0.25, 0.05), (150.0, 0.2, 0.04)],
        apron=0.65,
        chunks=[(108.0, (0.72, 0.44, 0.33), -8.0, 4.0, 0.3), (148.0, (0.46, 0.4, 0.42), 10.0, -3.0, 0.25),
                (74.0, (0.38, 0.36, 0.28), -16.0, 6.0, 0.2), (186.0, (0.3, 0.42, 0.52), 5.0, 0.0, 0.25),
                (352.0, (0.32, 0.4, 0.36), -7.0, 3.0, 0.2), (224.0, (0.42, 0.36, 0.3), 16.0, -4.0, 0.15),
                (312.0, (0.48, 0.34, 0.2), 8.0, 5.0, 0.15), (268.0, (0.62, 0.36, 0.12), -5.0, -2.0, 0.3)],
        fallen=[(0.78, -1.2, (0.4, 0.3, 0.24), -25.0), (0.5, -0.95, (0.18, 0.16, 0.12), 60.0)],
        spans=[(160.0, 52.0, 0.2, 0.22, 'Rope'), (130.0, 84.0, 0.08, 0.25, 'Cord'), (176.0, 120.0, 0.1, 0.18, 'Cord'),
               (96.0, 22.0, 0.16, 0.2, 'Tufted')],
        hanging=[(118.0, 0.5, 0.2, 'Ribbon'), (70.0, 0.32, 0.22, 'Tufted'), (40.0, 0.2, 0.15, 'Cord')],
        glued=[(112.0, 2.2), (150.0, 1.8), (60.0, 1.8), (188.0, 1.5), (350.0, 1.3)],
        mat_x=-0.12, mat_reach=4.0, funnel=(-10.0, 215.0)),
}


def build(name, spec):
    mouth = Mouth(spec)
    collar = Collar(mouth)
    obj = build_collar(name, collar, mouth.angles_by_arc(COLUMNS))
    lm.smooth(obj, 70.0)
    join(obj, lip_chunks(spec, mouth, collar))
    tree = ray_bvh([obj])
    join(obj, fallen_and_grit(spec, mouth, collar, tree))
    lt.box_uv(obj, 'RockCliff', seed=spec['seed'])
    # Occlusion: the wall behind it shades the skirt where it meets it (a temporary wall, removed after).
    wall = bpy.data.objects.new('WallOccluder', bpy.data.meshes.new('WallOccluder'))
    wall.data.from_pydata([(-6, 0.0, -6), (6, 0.0, -6), (6, 0.0, 6), (-6, 0.0, 6)], [], [(0, 1, 2, 3)])
    bpy.context.scene.collection.objects.link(wall)
    wall.parent = obj
    lt.bake_vertex_ao(obj, samples=32, distance=0.7, ground=False, children=False)
    bpy.data.objects.remove(wall)
    # The throat's fade to near black, over the baked occlusion.
    mesh = obj.data
    dark = np.zeros(len(mesh.vertices), np.float32)
    mesh.attributes['dark'].data.foreach_get('value', dark)
    loop_vert = np.empty(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get('vertex_index', loop_vert)
    raw = lt._read_col(mesh)
    raw[:, 3] *= 1.0 - dark[loop_vert]
    lt._write_col(mesh, raw)
    mesh.attributes.remove(mesh.attributes['dark'])
    # Silk.
    tree = ray_bvh([obj])
    atlas = Atlas()
    rnd = random.Random(spec['seed'] * 11)
    card = Card(name + '_Silk')
    phase = rnd.random()
    funnel_lining(card, spec, mouth, collar, tree, atlas, rnd, phase)
    silk_mat(card, spec, mouth, collar, tree, atlas, rnd, phase)
    strands(card, spec, mouth, collar, tree, rnd)
    silk = card.build()
    silk.parent = obj
    lm.socket(obj, 'Mouth', (0.0, 0.0, 0.0), (0.0, 0.0, 0.0))
    obj['Nanite'] = 0
    obj['LODs'] = '50,25'
    obj['Collision'] = 'None'
    return obj


models = [build(name, spec) for name, spec in SPECS.items()]


def report(obj):
    parts = [obj] + [c for c in obj.children if c.type == 'MESH']
    tris = sum(len(p.vertices) - 2 for o in parts for p in o.data.polygons)
    co = np.array([o.matrix_world @ Vector(c) for o in parts for c in o.bound_box])
    size = co.max(axis=0) - co.min(axis=0)
    mats = []
    for o in parts:
        mats += [m.name for m in o.data.materials if m.name not in mats]
    sockets = [c.name.split('.')[0] for c in obj.children if c.name.startswith('SOCKET_')]
    each = ', '.join(f'{o.name} {sum(len(p.vertices) - 2 for p in o.data.polygons)}' for o in parts)
    print(f'BURROW: {obj.name}: {tris} triangles ({each}), '
          f'{size[0]:.2f} x {size[1]:.2f} x {size[2]:.2f} m (x {co.min(axis=0)[0]:.2f}..{co.max(axis=0)[0]:.2f}, '
          f'y {co.min(axis=0)[1]:.2f}..{co.max(axis=0)[1]:.2f}, z {co.min(axis=0)[2]:.2f}..{co.max(axis=0)[2]:.2f}), '
          f'materials {", ".join(mats)}, sockets {", ".join(sockets) or "-"}', flush=True)


for model in models:
    report(model)


# --- Previews: each burrow in a lit rock wall ---

def preview_wall():
    """A stand-in pit wall (the RockCliff rock, lumpy and bedded, its surface 0-15 cm behind the wall plane) and the
    floor 2.2 m under the mouth."""
    bm = bmesh.new()
    nx, nz = 60, 50
    verts = []
    for j in range(nz + 1):
        row = []
        for i in range(nx + 1):
            x, z = -4.0 + 8.0 * i / nx, -2.3 + 6.0 * j / nz
            p = Vector((x * 0.8, z * 1.4, 4.0))
            y = 0.02 + 0.07 * (0.5 + 0.5 * noise.fractal(p, 0.6, 2.0, 3)) + 0.04 * abs(math.sin(z * 4.1 + x * 0.3))
            row.append(bm.verts.new((x, y, z)))
        verts.append(row)
    for j in range(nz):
        for i in range(nx):
            bm.faces.new((verts[j][i], verts[j + 1][i], verts[j + 1][i + 1], verts[j][i + 1]))
    floor = [bm.verts.new(c) for c in ((-6, -8, -2.2), (6, -8, -2.2), (6, 0.3, -2.2), (-6, 0.3, -2.2))]
    bm.faces.new(floor)
    bm.normal_update()
    for f in bm.faces:
        if f.normal.y > 0.3:
            f.normal_flip()
    mesh = bpy.data.meshes.new('_Wall')
    bm.to_mesh(mesh)
    bm.free()
    wall = bpy.data.objects.new('_Wall', mesh)
    bpy.context.scene.collection.objects.link(wall)
    mesh.materials.append(ROCK)
    mesh.materials.append(lt.material('GroundDirt', name='_PreviewFloor', tint=0x6a625a))
    mesh.polygons[-1].material_index = 1
    lm.smooth(wall, 60.0)
    lt.box_uv(wall, 'RockCliff', seed=5)
    return wall


def stand_in_spider():
    """A dark ellipsoid 1.25 m across, 0.95 m long, 0.45 m high, half out of the mouth on the sill: the size check."""
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=24, v_segments=12, radius=1.0)
    for v in bm.verts:
        v.co = Vector((v.co.x * 0.62, v.co.y * 0.48, v.co.z * 0.22))
    mesh = bpy.data.meshes.new('_Spider')
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new('_Spider', mesh)
    bpy.context.scene.collection.objects.link(obj)
    mat = bpy.data.materials.new('_SpiderStandIn')
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = (0.08, 0.06, 0.05, 1.0)
    mesh.materials.append(mat)
    return obj


def render(objects, out_png, eye, target, sun_dir, sun_energy=3.0, lens=32.0, resolution=(1500, 1000)):
    scene = bpy.context.scene
    hidden = {o: o.hide_render for o in scene.objects}
    shown = set()
    for o in objects:
        shown.add(o)
        shown.update(c for c in o.children if c.type == 'MESH')
    for o in scene.objects:
        o.hide_render = o not in shown
    cam = bpy.data.objects.new('_Cam', bpy.data.cameras.new('_Cam'))
    cam.data.lens = lens
    scene.collection.objects.link(cam)
    cam.location = eye
    cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    sun.data.energy = sun_energy
    sun.data.color = (1.0, 0.86, 0.7)
    sun.data.angle = math.radians(1.5)
    sun.rotation_euler = (-Vector(sun_dir).normalized()).to_track_quat('-Z', 'Y').to_euler()
    scene.collection.objects.link(sun)
    world = lt._preview_world(scene)
    scene.world = world
    scene.camera = cam
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = 48
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.render.filepath = out_png
    bpy.ops.render.render(write_still=True)
    print(f'BURROW: preview {out_png}', flush=True)
    bpy.data.objects.remove(cam)
    bpy.data.objects.remove(sun)
    bpy.data.worlds.remove(world)
    for o, value in hidden.items():
        if o.name in scene.objects:
            o.hide_render = value


if lt.want_preview():
    for node in WEB.node_tree.nodes:               # previews clip where Unreal does (1/3), not at a half
        if node.type == 'MATH' and node.operation == 'GREATER_THAN':
            node.inputs[1].default_value = 1.0 / 3.0
    wall = preview_wall()
    # The rock as the area wears it (previews never export, so swapping the slots here changes nothing shipped).
    ransom = lt.material('RockCliff', name='_PreviewRansomRock', tint=RANSOM_TINT, MossAmount=0.12)
    for obj in [wall] + models:
        for i, slot in enumerate(obj.data.materials):
            if slot is ROCK:
                obj.data.materials[i] = ransom
    spider = stand_in_spider()
    sunlit = (-0.55, -0.75, 0.42)                 # low afternoon sun from the front left
    shade = (0.3, 0.9, 0.5)                       # the sun behind the wall: sky light only on its face
    for model in models:
        spider.location = (0.0, -0.55, -0.47 + 0.24)
        render([model, wall], lt.preview_path(PREVIEW, model.name + '_front'), (0.15, -5.2, -0.45), (0.0, 0.0, -0.25),
               sunlit)
        render([model, wall], lt.preview_path(PREVIEW, model.name + '_quarter'), (-3.2, -3.6, -0.6), (0.0, 0.0, -0.3),
               sunlit)
        render([model, wall], lt.preview_path(PREVIEW, model.name + '_close'), (0.6, -2.3, 0.2), (0.0, 0.0, -0.2),
               sunlit, lens=28.0)
        render([model, wall], lt.preview_path(PREVIEW, model.name + '_shade'), (0.4, -5.0, -0.5), (0.0, 0.0, -0.25),
               shade)
        render([model, wall, spider], lt.preview_path(PREVIEW, model.name + '_spider'), (1.6, -4.4, -0.7),
               (0.0, 0.0, -0.3), sunlit)
