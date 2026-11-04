"""The barge's kit pieces made for real (Main 6, the Pilot Light; the art audit's plan row 3: "the hose drum a cylinder on
a cube, the furnace hall cones and cylinders"; Docs/Work/2026-11-04-Art/CONTRACT.md "Round 3, more lanes", lane PROPS).
A scripted model file (Art/README.md): the folder names the Unreal folder, so every model here imports to
/Game/Art/Landfall/SM_<name>. The look is the user's Tripo dredge barge's (Art/Tripo/DredgeBarge/preview.png): rusted
riveted iron, oxide paint, brass fittings, soot; bevels, rivets and bolts are in the geometry.

Units in this file are Unreal centimetres in the kit's own frame (X forward, Y right, Z up): the code's numbers
(World/BargeRules.h, BargeHallRules.h, ABargeFurnace, AFurnaceDraftValve, ABargeHoseDrum) read here as they do there.
B() turns a point into Blender metres (Blender -Y is Unreal +X, Blender -X is Unreal +Y). Each model's pivot, sizes,
sockets, slots, collision and how the code places and scales it: Docs/Work/2026-11-04-Art/BARGE_PROPS.md.

  BargeFurnace       the soul furnace, 5 m across, 6 m to its crown (FurnaceRadius 250, FurnaceHeight 600): a riveted drum
                     of three lapped courses on a plinth, iron bands, a domed crown with three flanged pipe collars and a
                     manhole; on its +X face the soul cage: a flat riveted face plate in a niche of the drum (a sill and a
                     brow following its curve), a heavy brass porthole with bars, hinged and dogged, the heart's cavity
                     deep behind it (SOCKET_Heart inside the furnace, like the Tripo boiler's glowing port); a stoke door
                     at its back and two ash vents (their slots on the CinderGlow slot), two brass gauges, a ladder.
  BargeFurnaceBurst  the same furnace after the cage burst: the porthole hanging open on its hinge, its bars snapped and
                     bent out, the dog torn off, the cavity and the face round it blackened.
  BargeCageShard     one torn plate fragment with a bent bar stub: the burst's flung pieces (ABargeFurnace's Shards).
  BargeFurnaceGrate  one wedge of the floor grate ring (12 make it, GrateInner 340 to GrateOuter 520): a riveted frame,
                     six arc bars, an ember bed under them on the CinderGlow slot.
  BargeDraftValve    a draft valve on its pipe stub (3 m): floor flange, the valve body inline, the bonnet and stem out
                     toward the hall (+X), a bolted joint where the steam spits, a gauge, wall straps.
  BargeValveWheel    its brass handwheel (90 cm), apart so the code can spin it.
  BargePipe          a 3 m flanged pipe section, 50 cm across, stacked by the code up to the hall's ceiling (the
                     furnace's three pipes, the valves' risers).
  BargeDeckVent      the 2 x 2 m deck vent: a riveted coaming, six louvres, the fire pot under them on CinderGlow.
  BargeFurnaceVent   the same vent seen from the hall: a riveted hood under the deck, its throat on CinderGlow.
  BargeFireTongue    a tongue of furnace fire, 1 m across and tall (the vents' columns, the burning grates).
  BargeStack         the falling third stack, 3 m across, 14 m to its cap (StackRadius 150, StackHeight 1400): the
                     Tripo stacks' rings, collar, flared mouth and dome cap, a ladder, soot.
  BargeStackFoot     the deck plate it stands on, bolted (stays when the stack falls).
  BargeHoseDrum      the drum that turns: flanges, the hose wound on it, ratchet teeth, the axle.
  BargeDrumGantry    its housing on the forecastle (4 x 2.6 m, DrumGantryHalf 200 x 130): the riveted box with the
                     drum's well, the red gantry over it with the drum's top bearing, the jam lever's bracket and
                     quadrant, an inspection hatch.
  BargeDrumLever     the jam lever (iron arm, brass grip), pivot on its pin.
  BargeRadio         the stoker's wireless on the office desk (a riveted case 45 x 30 x 30 cm, dials, a brass key, a
                     short aerial).

Materials are the shared ones (MI_<name>, each made exactly as its first family makes it): IronBlack, BrassWorn,
PaintOxide, PaintCream, PaintBlack, CinderGlow (additive: the code swaps that slot for IronBlack where a piece is
cold); two new: BargeGunmetal (MetalRust tinted to the Tripo barge's blue-grey gunmetal: the plates of the furnace,
the stack, the housing, the vents, the pipes' runs) and HoseRubber (Polymer, near black). Texel density 1024 px/m (one
UV unit a metre on the 1024 px sets) on every face but BargeGunmetal's, at 512 px/m (DENSITY: MetalRust's rust patches
are painted for 320 px/m and shrink to a busy print at 1024). Nanite off (CinderGlow is additive; they are small or
movable), LODs 50% and 25% on the bigger ones; baked vertex occlusion and soot in its alpha.

    Tools\\artrun.ps1 -Script Art\\Models\\Landfall\\BargeKit.py -Preview     every model alone, the lineups and the
                                      assemblies into Saved/ArtPreviews/BargeKit/; '--only=Name,Name' builds just those
    Tools\\models.ps1 -Only BargeKit -Out Intermediate\\ArtExport_Props -NoImport     export test
Everything random comes from seeded random.Random and mathutils noise, nothing iterates a set of mesh elements, so
every export is the same.
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

import looter_buildings as kit
import looter_model as lm
import looter_textures as lt

ARGS = kit._args()
PREVIEW = lt.want_preview()
ONLY = next((a.split('=', 1)[1].split(',') for a in ARGS if a.startswith('--only=')), None)
# Round 2's previews (2026-11-04, after the game's look check) beside round 1's in Saved/ArtPreviews/BargeKit/.
OUT = os.path.join(lt.PREVIEW_DIR, 'BargeKit', 'r2')
TAU = 2.0 * math.pi

# --- The code's numbers (change them with the code: BARGE_PROPS.md says which axis each scales) ---

FURNACE_R = 250.0                      # BargeRules::FurnaceRadius
FURNACE_H = 600.0                      # BargeRules::FurnaceHeight
CAGE_UP = 250.0                        # BargeRules::CageUp
HEART = (150.0, 0.0, CAGE_UP)          # SOCKET_Heart: inside the furnace behind its porthole (ABargeFurnace's HeartOut
                                       # becomes -100; it was 30, a heart hanging outside the drum's face)
FURNACE_PIPES = ((-120.0, 150.0), (-120.0, -150.0), (-220.0, 0.0))   # ABargeFurnace::BeginPlay's pipe feet
PIPE_COLLAR_TOP = 640.0                # the code's pipes start here once the model is in (SOCKET_Pipe_N)
GRATES = 12                            # BargeRules::Grates
GRATE_IN = 340.0                       # BargeRules::GrateInner (FurnaceRadius + GrateSafeIn)
GRATE_OUT = 520.0                      # BargeRules::GrateOuter (HallRadius 700 - GrateSafeOut)
GRATE_MID = 0.5 * (GRATE_IN + GRATE_OUT)
VALVE_WHEEL = (45.0, 0.0, 130.0)       # AFurnaceDraftValve's WheelAt
VALVE_STEAM = (20.0, 0.0, 260.0)       # where its steam spits
VALVE_PIPE_X, VALVE_PIPE_R = -10.0, 22.5
VENT_HALF = 100.0                      # BargeRules::VentHalf
VENT_RIM_W = 12.0                      # BargeRules::VentRimWidth
VENT_SLATS = 6                         # BargeRules::VentSlats
STACK_R = 150.0                        # BargeRules::StackRadius
STACK_H = 1400.0                       # BargeRules::StackHeight
GANTRY_HALF = (200.0, 130.0)           # BargeRules::DrumGantryHalf
DRUM_UP = 50.0                         # the drum's base in the gantry (its well's floor)
LEVER_PIVOT = (140.0, -172.0, 96.0)    # the jam lever's pin on the gantry (SOCKET_LeverPivot): its grip clears the
                                       # housing's rim thrown in to roll +35 (toward the housing) and out to -35

# --- Materials (the shared names, made exactly as their first families make them) ---

IRON = lt.material('MetalWorn', name='IronBlack', tint=0x2e2c2a)
BRASS = lt.material('MetalWorn', name='BrassWorn', tint=0xc49c56)
OXIDE = lt.material('PaintWorn', name='PaintOxide', tint=0x8c3e2c)
CREAM = lt.material('PaintWorn', name='PaintCream', tint=0xe6dac2)
BLACK = lt.material('PaintWorn', name='PaintBlack', tint=0x2a2622)
# The plates' iron: MetalRust tinted to the Tripo barge's dark blue-grey gunmetal (its boiler casing and stacks), the
# paint gone to brown rust in patches. One new instance (MI_BargeGunmetal); MI_MetalRust's grey-green read pale and
# busy beside the Tripo hull.
# Round 2 (in the game, 2026-11-04): #8A98A8 read pale grey-white in the deck's low sun beside Tripo's brown iron, so
# darker and warmer.
RUST = lt.material('MetalRust', name='BargeGunmetal', tint=0x6a6560)
# UV units a metre: 1024 px/m (1.0) on every set but the rust plates', whose patches shrink to a 3-5 cm print at that
# density (previewed 2026-11-04): MetalRust is painted for 320 px/m, so its plates map at 512 px/m (2 m a repeat).
DENSITY = {'BargeGunmetal': 0.5}
RUBBER = lt.material('Polymer', name='HoseRubber', tint=0x36322e)
GLOW = lm.material('CinderGlow', 0xff6a1e, 'Glow', Glow=4.0, Variation=0.05)


def _preview_glow():
    """Only for the previews: the exporter reads the Glow property, not this."""
    bsdf = next(n for n in GLOW.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Emission Color'].default_value = lm.hex_color(0xff7a2a)
    bsdf.inputs['Emission Strength'].default_value = 2.5


# --- Frames (Unreal cm) ---

def B(p):
    """An Unreal point (cm) in Blender metres."""
    return Vector((-p[1] * 0.01, -p[0] * 0.01, p[2] * 0.01))


def Bdir(d):
    return Vector((-d[1], -d[0], d[2]))


def V(x=0.0, y=0.0, z=0.0):
    return Vector((x, y, z))


def T(x=0.0, y=0.0, z=0.0):
    return Matrix.Translation((x, y, z))


def Rx(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'X')


def Ry(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'Y')


def Rz(deg):
    """A yaw as Unreal's: +X turns toward +Y."""
    return Matrix.Rotation(math.radians(deg), 4, 'Z')


ALONG_X = Ry(90.0)     # a lathe's axis (local Z) along +X
ALONG_Y = Rx(-90.0)    # ... along +Y


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def polar(r, deg, z=0.0):
    a = math.radians(deg)
    return V(r * math.cos(a), r * math.sin(a), z)


# --- The model being built ---

class Kit:
    """One model: parts added in Unreal cm with their UVs at 1024 px/m (1 UV unit a metre), material slots in the
    order first used, convex hulls (point lists) and sockets, finished into a Blender object by finish()."""

    def __init__(self, name, seed):
        self.name = name
        self.rng = random.Random(seed)
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')
        # Each part's own crease angle (degrees): an edge between two of its faces is hard past it. Round parts with
        # few sides stay round (a six-sided bar), boxes keep their corners.
        self.crease = self.bm.faces.layers.float.new('_crease')
        self.mats = []
        self.hulls = []
        self.sockets = []

    def tris(self):
        return sum(len(f.verts) - 2 for f in self.bm.faces)

    def slot(self, mat):
        if mat not in self.mats:
            self.mats.append(mat)
        return self.mats.index(mat)

    def offset(self):
        return self.rng.uniform(0.0, 1.0), self.rng.uniform(0.0, 1.0)

    def add(self, verts, faces, uvs, mat, smooth=True, keep=None, facing=None, crease=46.0):
        """Adds one part: verts (Unreal cm), faces (vertex indices), uvs (per face, a UV per corner), mat (one material
        or a list per face). A closed part is turned outward by its volume; an open one by facing (a face's index and
        the way it must face)."""
        bv = [B(v) for v in verts]
        if facing is None:
            volume = 0.0
            for f in faces:
                a = bv[f[0]]
                for i in range(1, len(f) - 1):
                    volume += a.dot(bv[f[i]].cross(bv[f[i + 1]]))
            flip = volume < 0.0
        else:
            index, want = facing
            f = faces[index]
            normal = (bv[f[1]] - bv[f[0]]).cross(bv[f[2]] - bv[f[0]])
            flip = normal.dot(Bdir(want)) < 0.0
        new = [self.bm.verts.new(p) for p in bv]
        mats = mat if isinstance(mat, list) else None
        slot = None if mats else self.slot(mat)
        for i, (f, w) in enumerate(zip(faces, uvs)):
            if keep is not None and not keep[i]:
                continue
            order = list(range(len(f)))[::-1] if flip else list(range(len(f)))
            try:
                face = self.bm.faces.new([new[f[j]] for j in order])
            except ValueError:
                continue
            face_mat = mats[i] if mats else mat
            face.material_index = self.slot(face_mat) if mats else slot
            face.smooth = smooth
            face[self.crease] = crease
            scale = DENSITY.get(face_mat.name, 1.0)
            for loop, j in zip(face.loops, order):
                loop[self.uv].uv = (w[j][0] * scale, w[j][1] * scale)

    # Parts.

    def lathe(self, profile, segs, mat, frame=None, loop=False, skip=None, phase=0.0, smooth=True):
        """A turned part about the frame's Z: profile [(r, z), ...] from end to end (r 0 closes an end) or a closed
        loop (a band, a ring). mat may be a function of the profile step. U runs round it (at the step's mean radius),
        V along the profile. skip(k, b) leaves out segment k of step b (a hole, closed by other parts)."""
        frame = frame if frame is not None else Matrix.Identity(4)
        pts = list(profile) + ([profile[0]] if loop else [])
        run = [0.0]
        for a, b in zip(pts, pts[1:]):
            run.append(run[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
        verts, rings = [], []
        for i, (r, z) in enumerate(pts):
            if loop and i == len(pts) - 1:
                rings.append(rings[0])
                continue
            if r <= 1e-6:
                verts.append(frame @ V(0.0, 0.0, z))
                rings.append([len(verts) - 1])
                continue
            ring = []
            for k in range(segs):
                a = math.radians(phase) + TAU * k / segs
                verts.append(frame @ V(r * math.cos(a), r * math.sin(a), z))
                ring.append(len(verts) - 1)
            rings.append(ring)
        ou, ov = self.offset()
        faces, uvs, keep, mats = [], [], [], []
        for b in range(len(pts) - 1):
            r0, r1 = rings[b], rings[b + 1]
            if len(r0) == 1 and len(r1) == 1:
                continue
            rad = max(0.5 * (pts[b][0] + pts[b + 1][0]), 0.5) * 0.01
            v0, v1 = run[b] * 0.01 + ov, run[b + 1] * 0.01 + ov
            for k in range(segs):
                u0, u1 = TAU * k / segs * rad + ou, TAU * (k + 1) / segs * rad + ou
                k1 = (k + 1) % segs
                if len(r0) == 1:
                    faces.append((r0[0], r1[k1], r1[k]))
                    uvs.append([((u0 + u1) * 0.5, v0), (u1, v1), (u0, v1)])
                elif len(r1) == 1:
                    faces.append((r0[k], r0[k1], r1[0]))
                    uvs.append([(u0, v0), (u1, v0), ((u0 + u1) * 0.5, v1)])
                else:
                    faces.append((r0[k], r0[k1], r1[k1], r1[k]))
                    uvs.append([(u0, v0), (u1, v0), (u1, v1), (u0, v1)])
                keep.append(skip is None or not skip(k, b))
                mats.append(mat(b) if callable(mat) else mat)
        self.add(verts, faces, uvs, mats, smooth=smooth, keep=keep, crease=70.0)

    def mesh_part(self, tb, frame, mat, smooth=True):
        """A part built in a bmesh in its own frame (cm), mapped box-wise in that frame, then placed."""
        tb.verts.index_update()
        tb.normal_update()
        verts = [frame @ v.co for v in tb.verts]
        ou, ov = self.offset()
        faces, uvs = [], []
        for f in tb.faces:
            n = f.normal
            axis = max(range(3), key=lambda i: abs(n[i]))
            ua, va = ((1, 2), (0, 2), (0, 1))[axis]
            faces.append(tuple(v.index for v in f.verts))
            uvs.append([(v.co[ua] * 0.01 + ou, v.co[va] * 0.01 + ov) for v in f.verts])
        tb.free()
        self.add(verts, faces, uvs, mat, smooth=smooth)

    def box(self, size, frame, mat, bevel=0.0, segs=1, smooth=True):
        """A box of size (x, y, z) about the frame's origin, its edges chamfered by bevel."""
        tb = bmesh.new()
        bmesh.ops.create_cube(tb, size=1.0)
        for v in tb.verts:
            v.co = V(v.co.x * size[0], v.co.y * size[1], v.co.z * size[2])
        if bevel > 0.0:
            bmesh.ops.bevel(tb, geom=list(tb.edges), offset=min(bevel, 0.45 * min(size)), segments=segs, profile=0.5,
                            affect='EDGES', clamp_overlap=True)
        self.mesh_part(tb, frame, mat, smooth)

    def prism(self, outline, z0, z1, frame, mat, smooth=False):
        """A flat piece: the outline [(x, y), ...] (one way round, convex or not) from z0 to z1 in the frame."""
        n = len(outline)
        frame = frame if frame is not None else Matrix.Identity(4)
        verts = [frame @ V(x, y, z0) for x, y in outline] + [frame @ V(x, y, z1) for x, y in outline]
        ou, ov = self.offset()
        faces = [tuple(range(n))[::-1], tuple(range(n, 2 * n))]
        uvs = [[(outline[i][0] * 0.01 + ou, outline[i][1] * 0.01 + ov) for i in range(n)][::-1],
               [(outline[i][0] * 0.01 + ou, outline[i][1] * 0.01 + ov) for i in range(n)]]
        run = 0.0
        for i in range(n):
            i1 = (i + 1) % n
            step = math.hypot(outline[i1][0] - outline[i][0], outline[i1][1] - outline[i][1]) * 0.01
            faces.append((i, i1, n + i1, n + i))
            uvs.append([(run + ou, z0 * 0.01 + ov), (run + step + ou, z0 * 0.01 + ov), (run + step + ou, z1 * 0.01 + ov),
                        (run + ou, z1 * 0.01 + ov)])
            run += step
        self.add(verts, faces, uvs, mat, smooth=smooth)

    def diced_prism(self, outline, z0, z1, frame, mat, cuts_x=(), cuts_y=()):
        """A prism cut by planes at these local x and y (vertices across a big flat piece, for its baked occlusion),
        mapped box-wise in its own frame."""
        tb = bmesh.new()
        bottom = [tb.verts.new(V(x, y, z0)) for x, y in outline]
        top = [tb.verts.new(V(x, y, z1)) for x, y in outline]
        tb.faces.new(bottom[::-1])
        tb.faces.new(top)
        n = len(outline)
        for i in range(n):
            i1 = (i + 1) % n
            tb.faces.new((bottom[i], bottom[i1], top[i1], top[i]))
        for value, normal in [(x, V(1.0, 0.0, 0.0)) for x in cuts_x] + [(y, V(0.0, 1.0, 0.0)) for y in cuts_y]:
            bmesh.ops.bisect_plane(tb, geom=tb.verts[:] + tb.edges[:] + tb.faces[:], plane_co=normal * value, plane_no=normal, dist=1e-4)
        self.mesh_part(tb, frame if frame is not None else Matrix.Identity(4), mat, smooth=False)

    def tube(self, path, radius, mat, sides=6, radii=None, caps=True, smooth=True, flat=1.0):
        """A round tube along path (Unreal points), its frames carried along so it never twists."""
        pts = [Vector(p) for p in path]
        n = len(pts)
        run = [0.0]
        for i in range(1, n):
            run.append(run[-1] + (pts[i] - pts[i - 1]).length)
        tangents = [(pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized() for i in range(n)]
        ref = V(0.0, 0.0, 1.0) if abs(tangents[0].z) < 0.95 else V(1.0, 0.0, 0.0)
        side = tangents[0].cross(ref).normalized()
        verts, rings = [], []
        for i, p in enumerate(pts):
            t = tangents[i]
            if i:
                side = tangents[i - 1].rotation_difference(t) @ side
                side = (side - t * side.dot(t)).normalized()
            up = side.cross(t).normalized()
            r = radii[i] if radii else radius
            ring = []
            for k in range(sides):
                a = TAU * k / sides
                verts.append(p + (side * math.cos(a) + up * math.sin(a) * flat) * r)
                ring.append(len(verts) - 1)
            rings.append(ring)
        ou, ov = self.offset()
        around = TAU * max(radii or [radius]) * 0.01
        faces, uvs = [], []
        for i in range(n - 1):
            for k in range(sides):
                k1 = (k + 1) % sides
                faces.append((rings[i][k], rings[i][k1], rings[i + 1][k1], rings[i + 1][k]))
                a0, a1 = k / sides * around + ov, (k + 1) / sides * around + ov
                uvs.append([(run[i] * 0.01 + ou, a0), (run[i] * 0.01 + ou, a1), (run[i + 1] * 0.01 + ou, a1),
                            (run[i + 1] * 0.01 + ou, a0)])
        facing = None
        if caps:
            for ring, end in ((rings[0], 0), (rings[-1], n - 1)):
                order = list(reversed(ring)) if end == 0 else list(ring)
                faces.append(tuple(order))
                u = run[end] * 0.01 + ou
                r = radii[end] if radii else radius
                uvs.append([(u + r * 0.01 * math.cos(TAU * ring.index(v) / sides),
                             ov + r * 0.01 * math.sin(TAU * ring.index(v) / sides)) for v in order])
        else:
            f = faces[0]
            facing = (0, (verts[f[0]] + verts[f[1]]) * 0.5 - pts[0])
        self.add(verts, faces, uvs, mat, smooth=smooth, facing=facing, crease=70.0)

    def sweep(self, path, profile, mat, up=(0.0, 0.0, 1.0), smooth=True):
        """A closed bar along path: its cross-section the 2D profile [(across, up), ...] in a frame turned by up."""
        pts = [Vector(p) for p in path]
        n, m = len(pts), len(profile)
        run = [0.0]
        for i in range(1, n):
            run.append(run[-1] + (pts[i] - pts[i - 1]).length)
        perim = [0.0]
        for i in range(m):
            a, b = profile[i], profile[(i + 1) % m]
            perim.append(perim[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
        verts, rings = [], []
        for i, p in enumerate(pts):
            t = (pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized()
            side = t.cross(Vector(up))
            if side.length < 1e-6:
                side = t.cross(V(1.0, 0.0, 0.0))
            side.normalize()
            upv = side.cross(t).normalized()
            ring = []
            for a, b in profile:
                verts.append(p + side * a + upv * b)
                ring.append(len(verts) - 1)
            rings.append(ring)
        ou, ov = self.offset()
        faces, uvs = [], []
        for i in range(n - 1):
            for j in range(m):
                j1 = (j + 1) % m
                faces.append((rings[i][j], rings[i][j1], rings[i + 1][j1], rings[i + 1][j]))
                uvs.append([(run[i] * 0.01 + ou, perim[j] * 0.01 + ov), (run[i] * 0.01 + ou, perim[j + 1] * 0.01 + ov),
                            (run[i + 1] * 0.01 + ou, perim[j + 1] * 0.01 + ov), (run[i + 1] * 0.01 + ou, perim[j] * 0.01 + ov)])
        for ring, end in ((rings[0], 0), (rings[-1], n - 1)):
            order = list(reversed(ring)) if end == 0 else list(ring)
            faces.append(tuple(order))
            uvs.append([(profile[ring.index(v)][0] * 0.01 + ou, profile[ring.index(v)][1] * 0.01 + ov) for v in order])
        self.add(verts, faces, uvs, mat, smooth=smooth, crease=70.0)

    def stud(self, at, normal, mat, r=1.9, h=1.2, sides=5, top=0.55, sink=0.4):
        """A rivet head (a low dome) or with top=1.0 and sides 6 a bolt head, standing out of a surface: no bottom."""
        n = Vector(normal).normalized()
        at = Vector(at)
        helper = V(0.0, 0.0, 1.0) if abs(n.z) < 0.9 else V(1.0, 0.0, 0.0)
        t = n.cross(helper).normalized()
        b = n.cross(t)
        verts = []
        for k in range(sides):
            a = TAU * (k + 0.5) / sides
            verts.append(at - n * sink + (t * math.cos(a) + b * math.sin(a)) * r)
        for k in range(sides):
            a = TAU * (k + 0.5) / sides
            verts.append(at + n * h + (t * math.cos(a) + b * math.sin(a)) * r * top)
        faces, uvs = [tuple(range(sides, 2 * sides))], []
        ou, ov = self.offset()
        uvs.append([(math.cos(TAU * (k + 0.5) / sides) * r * top * 0.01 + ou, math.sin(TAU * (k + 0.5) / sides) * r * top * 0.01 + ov)
                    for k in range(sides)])
        edge = TAU * r / sides * 0.01
        for k in range(sides):
            k1 = (k + 1) % sides
            faces.append((k, k1, sides + k1, sides + k))
            uvs.append([(k * edge + ou, ov), ((k + 1) * edge + ou, ov), ((k + 1) * edge + ou, (h + sink) * 0.01 + ov),
                        (k * edge + ou, (h + sink) * 0.01 + ov)])
        self.add(verts, faces, uvs, mat, facing=(0, n), crease=80.0 if top < 0.8 else 50.0)

    def rivet(self, at, normal, mat=IRON, r=1.9):
        self.stud(at, normal, mat, r=r, h=r * 0.62)

    def bolt(self, at, normal, mat=IRON, r=2.4):
        self.stud(at, normal, mat, r=r, h=r * 0.7, sides=6, top=0.85)

    def sheet(self, grid, mat, facing):
        """An open surface from a grid of Unreal points (rows of columns), mapped flat in X and Y (an ember bed)."""
        rows, cols = len(grid), len(grid[0])
        verts = [p for row in grid for p in row]
        ou, ov = self.offset()
        faces, uvs = [], []
        for i in range(rows - 1):
            for j in range(cols - 1):
                f = (i * cols + j, i * cols + j + 1, (i + 1) * cols + j + 1, (i + 1) * cols + j)
                faces.append(f)
                uvs.append([(verts[k].x * 0.01 + ou, verts[k].y * 0.01 + ov) for k in f])
        self.add(verts, faces, uvs, mat, facing=(0, facing), crease=80.0)

    # Collision and sockets.

    def hull(self, points):
        self.hulls.append([Vector(p) for p in points])

    def hull_box(self, lo, hi):
        self.hull([V(x, y, z) for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])])

    def socket(self, name, at, yaw=0.0, pitch=0.0):
        """A socket at an Unreal point, its front (+X) turned by yaw (Unreal's) and pitch (up positive)."""
        self.sockets.append((name, V(*at), yaw, pitch))

    # The end.

    def finish(self, lods='50,25', screens='0.45,0.18', ao=0.8, ground=True, collision=True, soot=None, sharp=46.0):
        """The model object: triangulated as every export does it, smooth by angle with face-weighted normals, its
        occlusion and soot baked into the vertex alpha, hulls and sockets under it."""
        loose = [v for v in self.bm.verts if not v.link_faces]
        if loose:
            bmesh.ops.delete(self.bm, geom=loose, context='VERTS')
        kit._triangulate(self.bm)
        self.bm.normal_update()
        for edge in self.bm.edges:
            if len(edge.link_faces) == 2:
                limit = min(f[self.crease] for f in edge.link_faces)
                edge.smooth = edge.calc_face_angle(0.0) < math.radians(limit)
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        if '_crease' in mesh.attributes:
            mesh.attributes.remove(mesh.attributes['_crease'])
        for mat in self.mats:
            mesh.materials.append(mat)
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        weighted = obj.modifiers.new('Weighted', 'WEIGHTED_NORMAL')
        weighted.mode = 'FACE_AREA'
        weighted.weight = 50
        weighted.keep_sharp = True
        obj['Nanite'] = 0
        if lods:
            obj['LODs'] = lods
            obj['LODScreens'] = screens
        if not collision:
            obj['Collision'] = 'None'
        triangles = sum(len(p.vertices) - 2 for p in mesh.polygons)
        if '--no-ao' not in ARGS:
            lt.bake_vertex_ao(obj, distance=ao, ground=ground)
            if soot is not None:
                darken(obj, soot)
        for points in self.hulls:
            hb = bmesh.new()
            verts = [hb.verts.new(B(p)) for p in points]
            result = bmesh.ops.convex_hull(hb, input=verts)
            inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
            bmesh.ops.delete(hb, geom=list(dict.fromkeys(inside)), context='VERTS')
            hull_mesh = bpy.data.meshes.new('UCX_' + self.name)
            hb.to_mesh(hull_mesh)
            hb.free()
            hull = bpy.data.objects.new('UCX_' + self.name, hull_mesh)
            bpy.context.scene.collection.objects.link(hull)
            hull.parent = obj
            hull.display_type = 'WIRE'
        for name, at, yaw, pitch in self.sockets:
            # A socket's front is the empty's -Y (Unreal +X): an Unreal yaw turns the other way about Blender's Z; a
            # pitch up turns its front (-Y) up, a negative turn about Blender's X.
            lm.socket(obj, name, B(at), (-pitch, 0.0, -yaw))
        print(f'BARGEKIT: {self.name}: {triangles} triangles, {len(self.hulls)} hulls, {len(self.sockets)} sockets, '
              f"materials {', '.join(m.name for m in mesh.materials)}", flush=True)
        return obj


def darken(obj, weight):
    """Lowers the baked occlusion (vertex alpha) by weight(Unreal point, Unreal normal) (0 to 1): soot and grime. M_World
    darkens the base colour with some of it and the ambient light with all of it."""
    import numpy as np
    mesh = obj.data
    col = mesh.color_attributes.get('Col')
    if col is None:
        return
    raw = np.empty(4 * len(mesh.loops), dtype=np.float32)
    col.data.foreach_get('color', raw)
    raw = raw.reshape(-1, 4)
    for poly in mesh.polygons:
        n = poly.normal
        nu = V(-n.y, -n.x, n.z)
        for li in poly.loop_indices:
            c = mesh.vertices[mesh.loops[li].vertex_index].co
            w = weight(V(-c.y * 100.0, -c.x * 100.0, c.z * 100.0), nu)
            if w > 0.0:
                raw[li, 3] *= 1.0 - min(w, 0.92)
    col.data.foreach_set('color', raw.ravel())
    mesh.update()


def streak(p, scale=6.0, seed=0.0):
    """Vertical soot streaks: noise round the part that barely changes with height."""
    a = math.atan2(p.y, p.x)
    return max(0.0, noise.noise(V(a * scale, p.z * 0.003, seed)))


# --- Shared assemblies ---

def louvre(m, F, w, h, slats, frame_mat=IRON, door=False):
    """A louvred vent (or a stoke door) on a surface: its frame F has +X out of the surface (0 at it), Y across, Z up.
    Slats over a glowing plate (CinderGlow) on a dark back plate, in a riveted frame."""
    t = 7.0
    m.box((4.5, w + 4.0, h + 4.0), F @ T(-1.6, 0.0, 0.0), IRON)
    m.box((0.6, w, h), F @ T(1.1, 0.0, 0.0), GLOW)
    for s in (-1.0, 1.0):
        m.box((8.0, w + 2.0 * t, t), F @ T(4.0, 0.0, s * (h + t) * 0.5), frame_mat, bevel=1.2)
        m.box((8.0, t, h), F @ T(4.0, s * (w + t) * 0.5, 0.0), frame_mat, bevel=1.2)
    pitch = h / slats
    for i in range(slats):
        z = -h * 0.5 + pitch * (i + 0.5)
        m.box((pitch * 1.25, w, 1.6), F @ T(4.0, 0.0, z) @ Ry(38.0), IRON)
    for sy in (-1.0, 1.0):
        for sz in (-1.0, 1.0):
            m.rivet(F @ V(8.0, sy * (w + t) * 0.5, sz * (h + t) * 0.5), (F.to_3x3() @ V(1.0, 0.0, 0.0)))
    if door:
        hx = F.to_3x3() @ V(1.0, 0.0, 0.0)
        for z in (-h * 0.5 + 9.0, h * 0.5 - 9.0):
            m.lathe([(0.0, -7.0), (3.6, -7.0), (3.6, 7.0), (0.0, 7.0)], 8, IRON, frame=F @ T(9.0, w * 0.5 + t * 0.5, z))
        m.tube([F @ V(8.0, -w * 0.5 + 6.0, -6.0), F @ V(15.0, -w * 0.5 + 6.0, -6.0), F @ V(15.0, -w * 0.5 + 6.0, 10.0),
                F @ V(8.0, -w * 0.5 + 6.0, 10.0)], 1.6, IRON, sides=6)
        m.rivet(F @ V(8.5, -w * 0.5 - t * 0.5, 0.0), hx)


def gauge(m, F, mat=BRASS):
    """A pressure gauge on a short nipple: F has +X out of the surface."""
    m.tube([F @ V(-3.0, 0.0, 0.0), F @ V(9.0, 0.0, 0.0)], 2.2, IRON, sides=6)
    D = F @ T(9.0, 0.0, 0.0) @ ALONG_X
    m.lathe([(0.0, 0.0), (11.5, 0.0), (13.0, 1.5), (13.0, 6.5), (12.0, 7.6), (10.8, 7.6), (10.6, 6.2), (0.0, 6.2)], 10, mat, frame=D)
    m.lathe([(0.0, 5.8), (10.4, 5.8), (10.4, 6.6), (0.0, 6.6)], 10, CREAM, frame=D)
    m.box((0.5, 0.9, 8.0), F @ T(16.0, 0.0, 0.0) @ Rx(-40.0) @ T(0.0, 0.0, 3.0), BLACK)


# --- The soul furnace ---

def furnace(name, burst=False):
    m = Kit(name, 3301 if burst else 3300)
    R = FURNACE_R
    SEG = 36
    BAY = 3                          # segments either side of +X the cage bay's hole takes (30 degrees)
    BZ0, BZ1 = 145.0, 355.0          # the hole's bottom and top
    c1, c2, c3 = R + 1.5, R, R - 1.5   # the three lapped courses, the lowest outermost

    def in_bay(k):
        return k < BAY or k >= SEG - BAY

    # The plinth and the drum (its hole where the bay goes in).
    m.lathe([(236.0, 0.0), (273.0, 0.0), (273.0, 10.0), (236.0, 14.0)], SEG, IRON, loop=True)
    prof = ([(0.0, 0.0), (c1, 0.0)] + [(c1, z) for z in (40.0, BZ0, 175.0)] + [(c1, 200.0), (c2, 200.0)]
            + [(c2, z) for z in (270.0, BZ1)] + [(c2, 400.0), (c3, 400.0), (c3, 521.0), (0.0, 521.0)])

    def hole(k, b):
        return in_bay(k) and prof[b][1] >= BZ0 - 0.1 and prof[b + 1][1] <= BZ1 + 0.1
    m.lathe(prof, SEG, RUST, skip=hole)

    # Bands: the foot's, the upper lap's, the crown's seat.
    def band(z0, z1, r_in, r_out):
        e = min(1.5, (z1 - z0) * 0.2)
        m.lathe([(r_in, z0), (r_out - 1.2, z0), (r_out, z0 + e), (r_out, z1 - e), (r_in, z1)], SEG, IRON, loop=True)
    band(40.0, 58.0, c1 - 4.0, c1 + 5.5)
    band(392.0, 408.0, c2 - 4.0, c2 + 5.0)
    band(508.0, 532.0, c3 - 6.0, R + 10.0)

    def clear(a, z):
        """Rivets keep off the bay, the door, the ash vents, the gauges and the ladder."""
        a = (a + 180.0) % 360.0 - 180.0
        if abs(a) < 34.0 and BZ0 - 14.0 < z < BZ1 + 14.0:
            return False
        if abs(abs(a) - 180.0) < 12.0 and z < 160.0:
            return False
        return True

    for z, r, step in ((49.0, c1 + 5.5, 20.0), (193.0, c1, 20.0), (400.0, c2 + 5.0, 24.0), (520.0, R + 10.0, 30.0)):
        for i in range(int(round(360.0 / step))):
            a = 5.0 + i * step
            if clear(a, z):
                m.rivet(polar(r, a, z), polar(1.0, a))
    # Vertical seams: a strap down each course's butt joint, riveted.
    for a in (60.0, -60.0, 135.0, -135.0):
        for z0, z1, r in ((58.0, 196.0, c1), (204.0, 392.0, c2), (408.0, 508.0, c3)):
            F = T(*polar(r + 0.6, a)) @ Rz(a)
            m.box((1.6, 7.0, z1 - z0), F @ T(0.0, 0.0, 0.5 * (z0 + z1)), IRON)
            count = 2 if z1 - z0 < 150.0 else 3
            for i in range(count):
                z = z0 + (z1 - z0) * (i + 0.5) / count
                m.rivet(F @ V(0.8, 0.0, z), polar(1.0, a), r=1.7)

    # The crown: a dome on the seat band, the manhole on top, the three pipe collars.
    m.lathe([(0.0, 531.0), (R, 531.0), (R, 534.0), (240.0, 546.0), (212.0, 562.0), (175.0, 574.0),
             (110.0, 588.0), (0.0, 600.0)], SEG, RUST)
    m.lathe([(0.0, 594.0), (40.0, 594.0), (40.0, 604.0), (37.0, 608.0), (0.0, 608.0)], 12, IRON)
    for i in range(6):
        m.bolt(polar(32.0, 30.0 + i * 60.0, 608.0), (0.0, 0.0, 1.0), r=2.2)
    m.box((3.0, 30.0, 3.0), T(0.0, 0.0, 610.0), IRON, bevel=0.8)
    for x, y in FURNACE_PIPES:
        m.lathe([(0.0, 548.0), (31.0, 548.0), (31.0, 624.0), (40.0, 626.0), (40.0, PIPE_COLLAR_TOP), (0.0, PIPE_COLLAR_TOP)],
                16, IRON, frame=T(x, y, 0.0))
        for i in range(4):
            m.bolt(V(x, y, 0.0) + polar(35.5, 45.0 + i * 90.0, PIPE_COLLAR_TOP), (0.0, 0.0, 1.0), r=2.0)

    # The soul cage: the heart hangs inside the furnace (SOCKET_Heart) behind a round barred brass porthole, set in a
    # flat riveted face plate across the drum's hole (its chord, flush with the hole's sides), a sill and a brow
    # following the drum's curve above and below it, the cage's cavity behind.
    HY = R * math.sin(math.radians(BAY * 360.0 / SEG))
    PF = math.sqrt(c2 * c2 - (HY + 2.0) ** 2) - 0.5     # the plate's front: where the hole's sides meet it
    PT = 8.0
    WR = 82.0                                            # the porthole's clear radius
    YZ_PLANE = Rz(90.0) @ Rx(90.0)                       # a prism's outline (y, z), its thickness along X
    steps = 14
    for s in (1.0, -1.0):
        half = [(-(HY + 2.0), CAGE_UP), (-(HY + 2.0), BZ1 + 2.0 if s > 0 else BZ0 - 2.0),
                (HY + 2.0, BZ1 + 2.0 if s > 0 else BZ0 - 2.0), (HY + 2.0, CAGE_UP)]
        half += [(WR * math.cos(math.pi * i / steps), CAGE_UP + s * WR * math.sin(math.pi * i / steps)) for i in range(steps + 1)]
        m.diced_prism(half, -PT, 0.0, T(PF, 0.0, 0.0) @ YZ_PLANE, RUST, cuts_x=(-60.0, 60.0),
                      cuts_y=(CAGE_UP + s * 60.0,))
    for z0, z1, front in ((BZ0 - 6.0, BZ0 + 2.0, c1 + 2.5), (BZ1 - 2.0, BZ1 + 9.0, c2 + 8.0)):
        crescent = [(PF - PT - 2.0, HY + 4.0), (PF - PT - 2.0, -(HY + 4.0))]
        crescent += [(math.sqrt(front * front - y * y), y) for y in (-(HY + 4.0) + 2.0 * (HY + 4.0) * i / 12 for i in range(13))]
        m.prism(crescent, z0, z1, None, RUST, smooth=True)
    m.box((1.6, 50.0, 7.0), T(c2 + 8.6, 0.0, BZ1 + 3.5), BRASS, bevel=0.5)   # the brass plate on the brow
    for y in (-110.0, -66.0, 66.0, 110.0):
        for z in (BZ0 + 8.0, BZ1 - 8.0):
            m.rivet(V(PF, y, z), (1.0, 0.0, 0.0), r=2.1)
    for y in (-(HY - 8.0), HY - 8.0):
        for z in (CAGE_UP - 70.0, CAGE_UP, CAGE_UP + 70.0):
            m.rivet(V(PF, y, z), (1.0, 0.0, 0.0), r=2.1)
    # The cavity the heart beats in, deep in the furnace, blackened.
    m.lathe([(WR + 1.0, 60.0), (WR + 5.0, 60.0), (WR + 5.0, PF - PT + 1.0), (WR + 1.0, PF - PT + 1.0)], 24, IRON, loop=True,
            frame=T(0.0, 0.0, CAGE_UP) @ ALONG_X)
    m.lathe([(0.0, 54.0), (WR + 5.0, 54.0), (WR + 5.0, 61.0), (0.0, 61.0)], 24, IRON, frame=T(0.0, 0.0, CAGE_UP) @ ALONG_X)
    # The porthole: a heavy brass ring with its bars, hinged on its +Y side, dogged shut on its -Y side. Burst, it hangs
    # open on its hinge, bars bent and snapped, the dog torn off.
    hinge = V(PF + 10.0, WR + 22.0, CAGE_UP)
    door = Matrix.Identity(4)
    if burst:
        door = T(*hinge) @ Rz(118.0) @ T(*(-hinge))
    m.lathe([(WR, PF - 2.0), (WR + 18.0, PF - 2.0), (WR + 18.0, PF + 14.0), (WR + 15.0, PF + 20.0), (WR + 6.0, PF + 21.0),
             (WR + 2.0, PF + 16.0)], 24, BRASS, loop=True, frame=door @ T(0.0, 0.0, CAGE_UP) @ ALONG_X)
    for k in range(12):
        a = math.radians(15.0 + k * 30.0)
        p = door @ V(PF + 20.4, (WR + 11.0) * math.cos(a), CAGE_UP + (WR + 11.0) * math.sin(a))
        m.bolt(p, door.to_3x3() @ V(1.0, 0.0, 0.0), r=2.2)
    BX = PF + 9.0
    bars = [(-48.0, None), (-16.0, None), (16.0, None), (48.0, None)]
    if not burst:
        for y, _ in bars:
            h = math.sqrt((WR + 2.0) ** 2 - y * y)
            m.tube([V(BX, y, CAGE_UP - h), V(BX, y, CAGE_UP + h)], 3.2, IRON, sides=6, caps=False)
        m.tube([V(BX + 3.5, -WR - 2.0, CAGE_UP), V(BX + 3.5, WR + 2.0, CAGE_UP)], 3.0, IRON, sides=6, caps=False)
    else:
        # Torn from the inside: two bars snapped to stubs, one bent out of the ring, one gone, the cross bar bent.
        h = math.sqrt((WR + 2.0) ** 2 - 48.0 ** 2)
        m.tube([door @ V(BX, -48.0, CAGE_UP - h), door @ V(BX, -48.0, CAGE_UP - 24.0), door @ V(BX + 2.0, -47.0, CAGE_UP - 18.0)], 3.2, IRON, sides=6)
        h = math.sqrt((WR + 2.0) ** 2 - 16.0 ** 2)
        m.tube([door @ V(BX, 16.0, CAGE_UP + h), door @ V(BX, 16.0, CAGE_UP + 30.0), door @ V(BX + 2.0, 17.0, CAGE_UP + 24.0)], 3.2, IRON, sides=6)
        m.tube([door @ V(BX, 48.0, CAGE_UP - h), door @ V(BX, 48.0, CAGE_UP - 20.0), door @ V(BX + 22.0, 44.0, CAGE_UP + 14.0),
                door @ V(BX + 40.0, 38.0, CAGE_UP + 40.0)], 3.2, IRON, sides=6)
        m.tube([door @ V(BX + 3.5, WR + 2.0, CAGE_UP), door @ V(BX + 3.5, 30.0, CAGE_UP), door @ V(BX + 24.0, 6.0, CAGE_UP + 10.0)], 3.0, IRON, sides=6)
    for z in (CAGE_UP - 46.0, CAGE_UP + 46.0):
        m.lathe([(0.0, z - 9.0), (4.2, z - 9.0), (4.2, z + 9.0), (0.0, z + 9.0)], 10, IRON, frame=T(*(hinge - V(0.0, 0.0, CAGE_UP))))
        m.box((10.0, 30.0, 6.0), door @ T(PF + 6.0, WR + 12.0, z), IRON, bevel=0.8)
        m.box((4.0, 12.0, 14.0), T(PF + 2.0, WR + 24.0, z), IRON, bevel=0.8)
    if not burst:
        m.box((7.0, 16.0, 9.0), T(PF + 9.0, -(WR + 20.0), CAGE_UP), IRON, bevel=1.0)
        m.tube([V(PF + 13.0, -(WR + 20.0), CAGE_UP), V(PF + 24.0, -(WR + 18.0), CAGE_UP + 4.0)], 1.8, IRON, sides=6)
    else:
        m.box((7.0, 16.0, 9.0), T(PF + 6.0, -(WR + 21.0), CAGE_UP - 6.0) @ Ry(25.0) @ Rz(-15.0), IRON, bevel=1.0)

    # The stoke door at the back, the two ash vents at the sides, the gauges, the ladder.
    louvre(m, T(-c1, 0.0, 108.0) @ Rz(180.0), 74.0, 62.0, 4, door=True)
    for s in (-1.0, 1.0):
        louvre(m, T(0.0, s * c1, 80.0) @ Rz(s * 90.0), 54.0, 24.0, 3)
    for a in (40.0, -40.0):
        gauge(m, T(*polar(c3, a, 445.0)) @ Rz(a))
    LX = -(R + 16.0)
    for y in (-22.0, 22.0):
        m.box((3.0, 5.0, 352.0), T(LX, y, 334.0), IRON)
        for z in (182.0, 330.0, 470.0):
            m.box((17.0, 4.0, 4.0), T(LX + 8.5, y, z), IRON)
    for i in range(12):
        z = 175.0 + i * 30.0
        m.tube([V(LX, -22.0, z), V(LX, 22.0, z)], 1.7, IRON, sides=6, caps=False)

    # Collision: the drum and crown (one hull), the cage bay.
    ring = []
    for k in range(16):
        a = k * 22.5
        ring += [polar(273.0, a, 0.0), polar(262.0, a, 532.0), polar(150.0, a, 580.0)]
    m.hull(ring + [V(0.0, 0.0, 610.0)])
    if burst:
        corners = [door @ V(x, y, z) for x in (PF - 2.0, PF + 22.0) for y in (-WR - 22.0, WR + 22.0) for z in (CAGE_UP - 102.0, CAGE_UP + 102.0)]
        m.hull(corners)

    m.socket('Heart', HEART)
    m.socket('Window', (PF + 21.0, 0.0, CAGE_UP))
    for i, (x, y) in enumerate(FURNACE_PIPES):
        m.socket(f'Pipe_{i + 1}', (x, y, PIPE_COLLAR_TOP))
    m.socket('Crown', (0.0, 0.0, 610.0), pitch=90.0)
    m.socket('StokeDoor', (-(c1 + 9.0), 0.0, 108.0), yaw=180.0)

    def soot(p, n):
        w = 0.5 * smoothstep(380.0, 600.0, p.z) + 0.32 * streak(p, 7.0, 1.3) * smoothstep(150.0, 520.0, p.z)
        if 50.0 < p.x < PF - 1.0 and math.hypot(p.y, p.z - CAGE_UP) < WR + 6.0:
            w += 0.85 if burst else 0.55
        if burst and p.x > PF - 2.0 and abs(p.y) < HY + 6.0 and BZ0 < p.z < BZ1 + 12.0 and n.x > 0.3:
            w += 0.45 * max(0.0, 1.0 - math.hypot(p.y, p.z - CAGE_UP) / 140.0) + 0.25 * streak(p, 18.0, 3.0)
        if p.x < -200.0 and abs(p.y) < 50.0 and 130.0 < p.z < 330.0:
            w += 0.45 * (1.0 - (p.z - 130.0) / 200.0)
        if abs(p.y) > 200.0 and abs(p.x) < 34.0 and 90.0 < p.z < 200.0:
            w += 0.4 * (1.0 - (p.z - 90.0) / 110.0)
        if burst and p.x > 150.0 and abs(p.y) < 150.0 and p.z > BZ1 - 20.0:
            w += 0.75 * max(0.0, 1.0 - (p.z - BZ1 + 20.0) / 230.0) * (0.6 + 0.4 * streak(p, 14.0, 5.0))
        return w
    return m.finish(ao=1.0, soot=soot)


def cage_shard(name):
    """A torn plate fragment from the cage's frame, bent, two rivets, a bent bar stub lying on it."""
    m = Kit(name, 3310)
    outline = [(-21.0, -9.0), (-8.0, -12.0), (3.0, -9.5), (12.0, -13.0), (22.0, -6.0), (18.0, 2.0), (23.0, 9.0),
               (9.0, 11.0), (-2.0, 8.0), (-12.0, 12.5), (-19.0, 5.0)]
    m.prism(outline, 0.0, 2.4, Matrix.Identity(4), IRON, smooth=False)
    part = m.bm
    # The fold: the +X half bent up 16 degrees about the fold line x 2.
    for v in part.verts:
        x = -v.co[1] * 100.0
        if x > 2.0:
            lift = (x - 2.0) * math.tan(math.radians(16.0)) * 0.01
            v.co.z += lift
    m.rivet(V(-12.0, 1.0, 2.4), (0.0, 0.0, 1.0), r=2.0)
    m.rivet(V(-4.0, -6.0, 2.4), (0.0, 0.0, 1.0), r=2.0)
    m.tube([V(-16.0, 14.0, 3.4), V(2.0, 10.0, 3.6), V(16.0, -2.0, 9.0)], 3.0, IRON, sides=6)
    return m.finish(lods=None, ao=0.25, collision=False, soot=lambda p, n: 0.35 + 0.3 * streak(p, 20.0, 2.0))


# --- The grate ring's wedge ---

def furnace_grate(name):
    """One wedge in its own frame: the pivot at the ring's middle radius on the wedge's bisector, on the floor; +X out
    along the radius (as ABargeFurnace::WedgeTransform lays it)."""
    m = Kit(name, 3320)
    half = 180.0 / GRATES - math.degrees(2.0 / GRATE_MID)    # a 4 cm gap between neighbours at the middle radius

    def P(rho, phi, z=0.0):
        a = math.radians(phi)
        return V(rho * math.cos(a) - GRATE_MID, rho * math.sin(a), z)

    def arc(rho, lo, hi, steps, z=0.0):
        return [P(rho, lo + (hi - lo) * i / steps, z) for i in range(steps + 1)]

    rail = [(-3.5, 0.0), (3.5, 0.0), (3.5, 7.8), (2.3, 9.0), (-2.3, 9.0), (-3.5, 7.8)]
    bar = [(-2.3, 1.5), (2.3, 1.5), (2.3, 6.6), (1.3, 7.5), (-1.3, 7.5), (-2.3, 6.6)]
    steps = 8
    edge = math.degrees(3.5 / GRATE_MID)
    m.sweep(arc(GRATE_OUT - 3.5, -half, half, steps), rail, IRON)
    m.sweep(arc(GRATE_IN + 3.5, -half, half, steps), rail, IRON)
    for s in (-1.0, 1.0):
        phi = s * (half - edge)
        m.sweep([P(GRATE_IN + 1.0, phi), P(GRATE_OUT - 1.0, phi)], rail, IRON)
    inner_phi = half - 2.0 * edge
    for i in range(6):
        rho = GRATE_IN + 7.0 + (GRATE_OUT - GRATE_IN - 14.0) * (i + 0.5) / 6.0
        lim = inner_phi + math.degrees(1.0 / rho)
        m.sweep(arc(rho, -lim, lim, steps), bar, IRON)
    for phi in (-half * 0.36, half * 0.36):
        m.sweep([P(GRATE_IN + 6.0, phi), P(GRATE_OUT - 6.0, phi)], [(-1.6, 0.6), (1.6, 0.6), (1.6, 5.4), (-1.6, 5.4)], IRON)
    # The pit under the bars: a dark floor and the ember bed on it.
    sector = [(p.x, p.y) for p in arc(GRATE_OUT - 2.0, -half + 0.5, half - 0.5, steps)] + \
             [(p.x, p.y) for p in arc(GRATE_IN + 2.0, half - 0.5, -half + 0.5, steps)]
    m.prism(sector, 0.0, 0.8, None, IRON)
    m.prism(sector, 0.8, 1.6, None, GLOW)
    for rho, phis in ((GRATE_OUT - 3.5, (-half * 0.8, 0.0, half * 0.8)), (GRATE_IN + 3.5, (-half * 0.7, half * 0.7))):
        for phi in phis:
            m.rivet(P(rho, phi, 9.0), (0.0, 0.0, 1.0), r=1.8)
    for s in (-1.0, 1.0):
        for rho in (GRATE_IN + 45.0, GRATE_OUT - 45.0):
            m.rivet(P(rho, s * (half - edge), 9.0), (0.0, 0.0, 1.0), r=1.8)
    pts = arc(GRATE_OUT, -half, half, 6) + arc(GRATE_IN, -half, half, 6)
    m.hull([p for p in pts] + [p + V(0.0, 0.0, 9.0) for p in pts])
    m.socket('Fire', (0.0, 0.0, 9.0), pitch=90.0)
    return m.finish(ao=0.35, soot=lambda p, n: 0.25 + 0.25 * streak(p, 30.0, 4.0))


# --- The draft valve, its wheel, the pipe sections ---

def pipe_mat(profile, r):
    """A pipe's lathe: its run (the profile's steps at the pipe's radius) in the plates' iron, flanges and bodies black."""
    return lambda b: RUST if abs(profile[b][0] - r) < 0.01 and abs(profile[b + 1][0] - r) < 0.01 else IRON


def draft_valve(name):
    """Pivot on the floor 30 cm in from the hall's wall (ValveIn), +X into the hall; the pipe against the wall."""
    m = Kit(name, 3330)
    X = VALVE_PIPE_X
    r = VALVE_PIPE_R
    prof = [(0.0, 0.0), (36.0, 0.0), (36.0, 5.0), (27.0, 8.5), (r, 12.0), (r, 70.0), (r, 92.0), (30.0, 94.0), (30.0, 101.0),
            (r + 4.0, 104.0), (33.0, 118.0), (34.0, 130.0), (33.0, 142.0), (r + 4.0, 156.0), (30.0, 159.0), (30.0, 166.0),
            (r, 168.0), (r, 200.0), (r, 250.0), (31.0, 251.0), (31.0, 258.5), (29.5, 259.5), (31.0, 260.5), (31.0, 268.0),
            (r, 269.0), (r, 291.0), (31.0, 292.0), (31.0, 300.0), (0.0, 300.0)]
    m.lathe(prof, 20, pipe_mat(prof, r), frame=T(X, 0.0, 0.0))
    for z, count, rr in ((5.0, 6, 31.0), (101.0, 6, 27.0), (268.0, 8, 27.5), (300.0, 8, 27.5)):
        for i in range(count):
            m.bolt(V(X, 0.0, 0.0) + polar(rr, 180.0 / count + i * 360.0 / count, z), (0.0, 0.0, 1.0), r=2.0)
    for i in range(6):
        m.bolt(V(X, 0.0, 0.0) + polar(27.0, 30.0 + i * 60.0, 159.0), (0.0, 0.0, -1.0), r=2.0)
    # The bonnet out toward the hall, its bolted flange, the gland and the stem through the wheel's hub.
    bonnet = T(0.0, 0.0, VALVE_WHEEL[2]) @ ALONG_X
    m.lathe([(0.0, X), (16.0, X), (16.0, 18.0), (21.0, 19.0), (21.0, 26.0), (14.0, 27.0), (11.0, 30.0), (11.0, 35.0),
             (8.0, 36.0), (3.4, 37.0), (3.4, VALVE_WHEEL[0] + 6.0), (2.4, VALVE_WHEEL[0] + 7.0), (0.0, VALVE_WHEEL[0] + 7.0)],
            16, IRON, frame=bonnet)
    for i in range(6):
        a = math.radians(30.0 + i * 60.0)
        m.bolt(V(26.0, 17.5 * math.cos(a), VALVE_WHEEL[2] + 17.5 * math.sin(a)), (1.0, 0.0, 0.0), r=1.9)
    # Wall straps holding it to the hall's wall, and a gauge on a nipple.
    for z in (52.0, 228.0):
        m.lathe([(r - 0.5, z - 3.5), (r + 2.5, z - 3.5), (r + 2.5, z + 3.5), (r - 0.5, z + 3.5)], 20, IRON, loop=True,
                frame=T(X, 0.0, 0.0))
        for s in (-1.0, 1.0):
            m.box((14.0, 3.0, 7.0), T(X - r - 5.0, s * 7.0, z), IRON, bevel=0.6)
            m.bolt(V(X - r - 8.0, s * 8.5, z), (0.0, s, 0.0), r=1.7)
    gauge(m, T(X + r * math.cos(math.radians(35.0)), r * math.sin(math.radians(35.0)), 205.0) @ Rz(35.0))
    # The drain cock by the floor.
    m.tube([V(X + r - 2.0, 0.0, 26.0), V(X + r + 9.0, 0.0, 26.0), V(X + r + 9.0, 0.0, 20.0)], 2.0, BRASS, sides=6)
    m.box((1.5, 7.0, 2.0), T(X + r + 9.0, 0.0, 29.5), BRASS, bevel=0.4)
    ring = [V(X, 0.0, 0.0) + polar(34.0, k * 45.0, z) for k in range(8) for z in (0.0, 300.0)]
    m.hull(ring)
    m.hull_box((X, -24.0, VALVE_WHEEL[2] - 24.0), (VALVE_WHEEL[0] + 8.0, 24.0, VALVE_WHEEL[2] + 24.0))
    m.socket('Wheel', VALVE_WHEEL)
    m.socket('Steam', VALVE_STEAM)
    m.socket('PipeTop', (X, 0.0, 300.0), pitch=90.0)
    return m.finish(ao=0.5, soot=lambda p, n: 0.2 * streak(p, 9.0, 3.0) + 0.25 * smoothstep(150.0, 300.0, p.z))


def valve_wheel(name):
    """The brass handwheel: pivot at its hub's middle, its axle along +X (the code spins it about X)."""
    m = Kit(name, 3335)
    rim_r, tube_r = 41.0, 3.4
    m.lathe([(rim_r + tube_r * math.cos(TAU * i / 8), tube_r * math.sin(TAU * i / 8) - 3.0) for i in range(8)], 36, BRASS,
            loop=True, frame=ALONG_X)
    m.lathe([(0.0, -4.5), (7.0, -4.5), (7.8, -3.6), (7.8, 3.6), (7.0, 4.5), (0.0, 4.5)], 14, BRASS, frame=ALONG_X)
    m.lathe([(0.0, 4.4), (4.6, 4.4), (4.6, 7.2), (3.8, 8.0), (0.0, 8.0)], 6, IRON, frame=ALONG_X)
    spoke = [(-1.6, -1.1), (1.6, -1.1), (1.6, 1.1), (-1.6, 1.1)]
    for i in range(5):
        a = 90.0 + i * 72.0
        d = V(0.0, math.cos(math.radians(a)), math.sin(math.radians(a)))
        m.sweep([d * 6.5, d * 22.0 + V(-1.5, 0.0, 0.0), d * (rim_r - 1.5) + V(-3.0, 0.0, 0.0)], spoke, BRASS, up=(1.0, 0.0, 0.0))
    # A spinner knob on the rim.
    knob = V(0.0, math.cos(math.radians(54.0)), math.sin(math.radians(54.0))) * rim_r + V(-3.0, 0.0, 0.0)
    m.lathe([(0.0, 0.0), (2.4, 0.0), (2.9, 3.0), (3.4, 8.0), (3.0, 12.0), (0.0, 12.5)], 8, IRON, frame=T(*knob) @ ALONG_X)
    return m.finish(lods='50', screens='0.25', ao=0.25, ground=False, collision=False)


def pipe(name):
    """A 3 m flanged pipe section, 50 cm across, along +Z from its pivot (the middle of its foot)."""
    m = Kit(name, 3340)
    r = 25.0
    prof = [(0.0, 0.0), (33.0, 0.0), (33.0, 7.0), (28.0, 9.0), (r, 12.0), (r, 70.0), (r, 146.0), (26.5, 148.0),
            (26.5, 152.0), (r, 154.0), (r, 230.0), (r, 288.0), (28.0, 291.0), (33.0, 293.0), (33.0, 300.0), (0.0, 300.0)]
    m.lathe(prof, 16, pipe_mat(prof, r))
    for i in range(8):
        a = 22.5 + i * 45.0
        m.bolt(polar(29.0, a, 7.0), (0.0, 0.0, 1.0), r=2.0)
        m.bolt(polar(29.0, a, 293.0), (0.0, 0.0, -1.0), r=2.0)
    F = Rz(90.0)
    m.box((1.4, 6.0, 136.0), F @ T(r + 0.4, 0.0, 82.0), IRON, bevel=0.4)
    m.box((1.4, 6.0, 132.0), F @ T(r + 0.4, 0.0, 221.0), IRON, bevel=0.4)
    for z in (24.0, 56.0, 88.0, 120.0, 172.0, 204.0, 236.0, 268.0):
        m.rivet(F @ V(r + 1.2, 0.0, z), polar(1.0, 90.0), r=1.6)
    m.hull([polar(33.0, k * 45.0, z) for k in range(8) for z in (0.0, 300.0)])
    m.socket('Top', (0.0, 0.0, 300.0), pitch=90.0)
    return m.finish(ao=0.5, soot=lambda p, n: 0.25 * streak(p, 6.0, 7.0))


# --- The vents ---

def deck_vent(name):
    """The 2 x 2 m deck vent: pivot at its middle on the deck; nothing of it goes under the deck (the deck is the
    hull's). A riveted coaming 8 cm high, six louvres, a dark pit with its ember bed (CinderGlow) under them."""
    m = Kit(name, 3350)
    H = VENT_HALF
    w = VENT_RIM_W
    up = 8.0
    for s in (-1.0, 1.0):
        m.box((2.0 * H, w, up), T(0.0, s * (H - w * 0.5), up * 0.5), RUST, bevel=1.5)
        m.box((w, 2.0 * (H - w), up), T(s * (H - w * 0.5), 0.0, up * 0.5), RUST, bevel=1.5)
    inside = 2.0 * (H - w)
    pitch = inside / VENT_SLATS
    for i in range(VENT_SLATS):
        x = -inside * 0.5 + pitch * (i + 0.5)
        m.box((12.0, inside, 1.8), T(x, 0.0, 5.0) @ Ry(28.0), RUST, bevel=0.5)
    for y in (-inside * 0.25, inside * 0.25):
        m.box((inside, 3.0, 3.0), T(0.0, y, 2.6), RUST, bevel=0.5)
    m.box((inside, inside, 0.8), T(0.0, 0.0, 0.4), IRON)
    grid = []
    n = 10
    for i in range(n + 1):
        row = []
        for j in range(n + 1):
            x = -inside * 0.5 + inside * j / n
            y = -inside * 0.5 + inside * i / n
            edge = min(1.0, (inside * 0.5 - max(abs(x), abs(y))) / 12.0)
            z = 1.0 + max(0.0, edge) * (0.9 + 0.8 * noise.noise(V(x * 0.08, y * 0.08, 3.0)))
            row.append(V(x, y, z))
        grid.append(row)
    m.sheet(grid, GLOW, (0.0, 0.0, 1.0))
    for s in (-1.0, 1.0):
        for t in (-H * 0.55, 0.0, H * 0.55):
            m.bolt(V(t, s * (H - w * 0.5), up), (0.0, 0.0, 1.0), r=2.1)
            m.bolt(V(s * (H - w * 0.5), t, up), (0.0, 0.0, 1.0), r=2.1)
    for s in (-1.0, 1.0):
        x = s * (H + 1.0)
        m.tube([V(x, -9.0, 3.0), V(x + s * 6.0, -7.0, 2.2), V(x + s * 9.0, 0.0, 1.8), V(x + s * 6.0, 7.0, 2.2),
                V(x, 9.0, 3.0)], 1.3, IRON, sides=6)
        for y in (-9.0, 9.0):
            m.box((3.0, 3.0, 4.0), T(x - s * 0.5, y, 2.5), IRON, bevel=0.5)
    m.socket('Fire', (0.0, 0.0, up), pitch=90.0)
    return m.finish(ao=0.35, collision=False, soot=lambda p, n: 0.35 * (1.0 - smoothstep(60.0, 100.0, max(abs(p.x), abs(p.y)))) + 0.2 * streak(p, 15.0, 1.0))


def furnace_vent(name):
    """The deck vent's other end, seen from the hall: a riveted hood hanging from the deck's underside (pivot at its
    top middle, everything below it), the flue's throat on CinderGlow."""
    m = Kit(name, 3360)
    m.box((150.0, 150.0, 5.0), T(0.0, 0.0, -2.5), IRON, bevel=1.0)
    for s in (-1.0, 1.0):
        for t in (-55.0, 0.0, 55.0):
            m.bolt(V(t, s * 68.0, -5.0), (0.0, 0.0, -1.0), r=2.1)
            m.bolt(V(s * 68.0, t, -5.0), (0.0, 0.0, -1.0), r=2.1)
    # The hood: a square frustum, 130 to 64 across.
    top, low, z0, z1 = 65.0, 32.0, -5.0, -58.0
    corners = [(1, 1), (-1, 1), (-1, -1), (1, -1)]
    verts = [V(sx * top, sy * top, z0) for sx, sy in corners] + [V(sx * low, sy * low, z1) for sx, sy in corners]
    faces = [(0, 1, 2, 3), (7, 6, 5, 4)] + [(i, 4 + i, 4 + (i + 1) % 4, (i + 1) % 4) for i in range(4)]
    ou, ov = m.offset()
    slant = math.hypot(top - low, z1 - z0)
    uvs = [[(v.x * 0.01 + ou, v.y * 0.01 + ov) for v in (verts[k] for k in f)] for f in faces[:2]]
    for i in range(4):
        uvs.append([(-top * 0.01 + ou, ov), (-low * 0.01 + ou, slant * 0.01 + ov), (low * 0.01 + ou, slant * 0.01 + ov), (top * 0.01 + ou, ov)])
    m.add(verts, faces, uvs, RUST, smooth=False)
    # Rivets in a row across each slanted face, a third of the way down.
    t = 0.35
    half = top + (low - top) * t
    zr = z0 + (z1 - z0) * t
    for axis in (0, 1):
        for s in (-1.0, 1.0):
            nrm = V(s * (z0 - z1), 0.0, -(top - low)) if axis == 0 else V(0.0, s * (z0 - z1), -(top - low))
            for u in (-0.6, -0.2, 0.2, 0.6):
                p = V(s * half, u * half, zr) if axis == 0 else V(u * half, s * half, zr)
                m.rivet(p, nrm.normalized(), r=1.9)
    # The collar (a ring: its throat open below), the bell mouth, the glowing flue and a damper lever.
    m.lathe([(26.0, z1 + 3.0), (31.0, z1 + 3.0), (31.0, -88.0), (37.0, -89.0), (38.0, -95.0), (42.0, -101.0), (40.0, -103.0),
             (34.0, -98.0), (26.0, -92.0)], 20, IRON, loop=True)
    m.lathe([(0.0, -70.0), (26.5, -70.0), (26.5, -69.0), (0.0, -69.0)], 16, GLOW)
    m.lathe([(0.0, -67.0), (27.0, -67.0), (27.0, -61.0), (0.0, -61.0)], 16, IRON)
    m.tube([V(31.0, 0.0, -76.0), V(52.0, 0.0, -76.0)], 1.4, IRON, sides=6)
    m.box((3.0, 2.0, 34.0), T(52.0, 0.0, -76.0) @ Ry(30.0) @ T(0.0, 0.0, -10.0), IRON, bevel=0.5)
    m.lathe([(0.0, 0.0), (4.0, 1.0), (4.8, 5.0), (4.0, 9.0), (0.0, 10.0)], 10, IRON,
            frame=T(52.0, 0.0, -76.0) @ Ry(30.0) @ T(0.0, 0.0, -32.0))
    m.socket('Glow', (0.0, 0.0, -103.0), pitch=-90.0)
    return m.finish(ao=0.4, ground=False, collision=False, soot=lambda p, n: 0.3 * smoothstep(-40.0, -100.0, p.z) + 0.25 * streak(p, 8.0, 6.0))


def fire_tongue(name):
    """A tongue of furnace fire on CinderGlow, 1 m across and tall: pivot at its foot's middle (scale it to the column:
    X and Y radius / 50, Z height / 100)."""
    m = Kit(name, 3370)
    rnd = random.Random(3371)
    # Flattened licks (ribbons twisting as they rise), splayed from the foot and curling back over the middle at their
    # tips, of different heights; the middle one tallest. Additive, so where they cross they burn brighter.
    licks = [(4.0, 0.0, 21.0, 100.0, 85.0)]
    for i, a in enumerate((10.0, 80.0, 150.0, 220.0, 290.0)):
        licks.append((rnd.uniform(17.0, 25.0), a + rnd.uniform(-14.0, 14.0), rnd.uniform(12.0, 16.0), rnd.uniform(46.0, 84.0),
                      rnd.uniform(55.0, 120.0) * (1.0 if i % 2 else -1.0)))
    for out, a0, r0, height, twist in licks:
        path, radii = [], []
        steps = 9
        for i in range(steps + 1):
            t = i / steps
            a = math.radians(a0 + twist * t)
            rr = out * (1.0 + 0.55 * math.sin(t * math.pi * 0.8)) * (1.0 - 0.6 * t) + 4.0 * math.sin(t * math.pi * 2.3 + a0)
            path.append(V(rr * math.cos(a), rr * math.sin(a), height * t))
            radii.append(max(0.5, r0 * (1.0 - t) ** 1.2 * (1.0 + 0.35 * math.sin(t * math.pi))))
        m.tube(path, r0, GLOW, sides=6, radii=radii, flat=0.45)
    return m.finish(lods=None, ao=0.1, ground=False, collision=False)


# --- The falling stack and its foot ---

def stack(name):
    """The kit's third stack in the Tripo stacks' look: pivot at its foot's middle, its length along +Z to its cap
    (StackHeight 1400), 3 m across (StackRadius 150)."""
    m = Kit(name, 3380)
    R = STACK_R
    SEG = 28
    bands = [(222.0, 238.0), (452.0, 468.0), (682.0, 698.0), (912.0, 928.0), (1132.0, 1148.0)]
    collar = (1196.0, 1232.0)
    prof, mats = [(0.0, 6.0), (R + 1.2, 6.0)], []
    z = 6.0
    course = 0
    edges = [b for b in bands] + [collar]
    for z0, z1 in edges:
        r = R + (1.2 if course % 2 == 0 else 0.0)
        step = (z0 - z) / max(1, int((z0 - z) / 110.0))
        zz = z + step
        while zz < z0 - 1.0:
            prof.append((r, zz))
            zz += step
        prof.append((r, z0))
        out = R + (8.0 if (z0, z1) == collar else 5.5)
        prof += [(out, z0 + 1.5), (out, z1 - 1.5), (R + (1.2 if course % 2 else 0.0), z1)]
        z = z1
        course += 1
    # The flared mouth with its rolled lip, the throat inside down to a soot floor.
    prof += [(R + 1.0, 1252.0), (R + 6.0, 1268.0), (R + 16.0, 1282.0), (R + 25.0, 1292.0), (R + 28.0, 1297.0), (R + 27.0, 1302.0),
             (R + 21.0, 1304.0), (R + 15.0, 1300.0), (R - 2.0, 1288.0), (R - 3.0, 1250.0), (0.0, 1250.0)]

    def mat(b):
        z0, z1 = prof[b][1], prof[b + 1][1]
        r0, r1 = prof[b][0], prof[b + 1][0]
        if max(r0, r1) > R + 5.0 and z1 < 1240.0 and z0 > 100.0:
            return IRON
        if z0 >= 1248.0 or z1 >= 1250.0:
            return IRON
        return RUST
    m.lathe(prof, SEG, mat)
    # The foot's bolted flange and its gussets (the ladder's side, +X, has none).
    m.lathe([(R - 2.0, 0.0), (R + 25.0, 0.0), (R + 25.0, 6.0), (R + 18.0, 9.0), (R + 3.0, 13.0), (R - 2.0, 13.0)], SEG, IRON, loop=True)
    for i in range(16):
        m.bolt(polar(R + 17.0, 11.25 + i * 22.5, 7.2), (0.0, 0.0, 1.0), r=2.4)
    for i in range(7):
        a = 45.0 + i * 45.0
        m.prism([(R - 1.0, 6.0), (R + 21.0, 6.0), (R - 1.0, 74.0)], -2.0, 2.0, Rz(a) @ Rx(90.0), IRON)
    # Rivets round each band and the collar.
    for z0, z1 in edges:
        rr = R + (8.0 if (z0, z1) == collar else 5.5)
        for i in range(14):
            a = 360.0 / 14 * (i + 0.5)
            if abs((a + 180.0) % 360.0 - 180.0) < 16.0:
                continue
            m.rivet(polar(rr, a, 0.5 * (z0 + z1)), polar(1.0, a), r=2.0)
    # The rain cap on four struts (a dome, as Tripo's), its finial.
    m.lathe([(0.0, 1334.0), (118.0, 1334.0), (123.0, 1338.0), (122.0, 1344.0), (112.0, 1356.0), (95.0, 1370.0),
             (70.0, 1382.0), (40.0, 1391.0), (12.0, 1396.0), (0.0, 1396.0)], SEG, OXIDE)
    m.lathe([(0.0, 1395.0), (6.0, 1395.0), (7.0, 1397.5), (5.0, 1400.0), (0.0, 1400.0)], 8, IRON)
    for i in range(4):
        a = 45.0 + i * 90.0
        F = Rz(a)
        m.sweep([F @ V(R + 18.0, 0.0, 1300.0), F @ V(R - 8.0, 0.0, 1320.0), F @ V(104.0, 0.0, 1336.0)],
                [(-3.0, -1.3), (3.0, -1.3), (3.0, 1.3), (-3.0, 1.3)], IRON, up=tuple(F.to_3x3() @ V(0.0, 1.0, 0.0)))
    # The ladder up its +X side, and three guy eyes on the collar.
    LX = R + 17.0
    for y in (-22.0, 22.0):
        m.box((2.6, 5.0, 1180.0), T(LX, y, 640.0), IRON, bevel=0.5)
        for z in (120.0, 360.0, 600.0, 840.0, 1080.0):
            m.box((17.0, 3.6, 3.6), T(R + 8.5, y, z), IRON, bevel=0.4)
    for i in range(36):
        zr = 78.0 + i * 31.5
        m.tube([V(LX, -22.0, zr), V(LX, 22.0, zr)], 1.7, IRON, sides=6, caps=False)
    for a in (60.0, 180.0, 300.0):
        F = T(*polar(R + 8.0, a, 1214.0)) @ Rz(a)
        m.box((6.0, 5.0, 12.0), F @ T(3.0, 0.0, 0.0), IRON, bevel=0.6)
        m.lathe([(4.2 + 1.6 * math.cos(TAU * i / 6), 1.6 * math.sin(TAU * i / 6)) for i in range(6)], 10, IRON, loop=True,
                frame=F @ T(10.0, 0.0, 0.0) @ ALONG_Y)
    # Collision: the foot, the tube in three lengths (so it lands on the deck along its length), the cap.
    m.hull([polar(R + 26.0, k * 22.5, z) for k in range(16) for z in (0.0, 13.0)] + [polar(R + 1.0, k * 22.5, 74.0) for k in range(16)])
    ladder = [V(LX + 2.0, y, 0.0) for y in (-25.0, 25.0)]
    for z0, z1 in ((0.0, 470.0), (470.0, 940.0), (940.0, 1306.0)):
        pts = [polar(R + 9.0, k * 30.0, z) for k in range(12) for z in (z0, z1)]
        pts += [p + V(0.0, 0.0, z) for p in ladder for z in (z0, min(z1, 1232.0))]
        if z1 > 1250.0:
            pts += [polar(R + 28.0, k * 30.0, z) for k in range(12) for z in (1292.0, 1304.0)]
        m.hull(pts)
    m.hull([polar(124.0, k * 30.0, z) for k in range(12) for z in (1306.0, 1344.0)] + [polar(70.0, k * 30.0, 1383.0) for k in range(12)]
           + [V(0.0, 0.0, 1400.0)])
    m.socket('Smoke', (0.0, 0.0, 1300.0), pitch=90.0)
    m.socket('Top', (0.0, 0.0, 1400.0), pitch=90.0)

    def soot(p, n):
        w = 0.62 * smoothstep(700.0, 1300.0, p.z) + 0.35 * streak(p, 9.0, 2.2) * smoothstep(300.0, 1250.0, p.z)
        if p.z > 1246.0 and math.hypot(p.x, p.y) < R + 16.0:
            w += 0.5
        if p.z > 1330.0:
            w += 0.25 * (1.0 - smoothstep(1336.0, 1360.0, p.z)) + 0.2 * streak(p, 12.0, 9.0)
        return w
    return m.finish(lods='50,25', screens='0.5,0.2', ao=1.2, soot=soot)


def stack_foot(name):
    """The doubler plate the stack is bolted to: stays on the deck when the stack falls (pivot at its middle on the
    deck, the stack's foot on it)."""
    m = Kit(name, 3385)
    R = STACK_R
    m.box((380.0, 380.0, 3.0), T(0.0, 0.0, 1.5), RUST, bevel=1.0)
    for i in range(20):
        m.bolt(polar(R + 36.0, i * 18.0, 3.0), (0.0, 0.0, 1.0), r=2.4)
    for i in range(16):
        a = 11.25 + i * 22.5
        p = polar(R + 17.0, a, 3.0)
        m.lathe([(0.0, 0.0), (1.4, 0.0), (1.4, 3.4 + (i % 3)), (0.9, 4.2 + (i % 3)), (0.0, 4.4 + (i % 3))], 6, IRON, frame=T(*p))
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            m.bolt(V(x * 176.0, y * 176.0, 3.0), (0.0, 0.0, 1.0), r=2.8)
    m.hull_box((-190.0, -190.0, 0.0), (190.0, 190.0, 4.0))

    def soot(p, n):
        d = math.hypot(p.x, p.y)
        return 0.55 * max(0.0, 1.0 - abs(d - (R + 8.0)) / 40.0) + 0.2 * streak(p, 10.0, 3.0)
    return m.finish(ao=0.4, soot=soot)


# --- The hose drum, its gantry and the jam lever ---

def hose_drum(name):
    """The drum that turns (about its own Z): pivot at its foot's middle on its axis."""
    m = Kit(name, 3390)
    SEG = 32
    core, flange = 75.0, 110.0
    hose_r = 8.5
    m.lathe([(0.0, 14.0), (core, 14.0), (core, 120.0), (core, 226.0), (0.0, 226.0)], SEG, IRON)
    m.lathe([(0.0, 0.0), (flange - 2.0, 0.0), (flange, 2.0), (flange, 7.0), (flange - 2.0, 9.0), (core + 16.0, 10.0),
             (core + 3.0, 14.0), (core, 20.0), (0.0, 20.0)], SEG, RUST)
    m.lathe([(0.0, 220.0), (core, 220.0), (core + 3.0, 226.0), (core + 16.0, 230.0), (flange - 2.0, 231.0), (flange, 233.0),
             (flange, 238.0), (flange - 2.0, 240.0), (0.0, 240.0)], SEG, RUST)
    # The hose wound on it, one layer, its ends tucked into the core's slots.
    helix_r = core + hose_r + 0.5
    z0, z1 = 28.0, 212.0
    pitch = 2.0 * hose_r + 0.6
    turns = (z1 - z0) / pitch
    steps = int(turns * 20)
    path = [V(core - 6.0, 0.0, z0 - 4.0)]
    for i in range(steps + 1):
        t = i / steps
        a = TAU * turns * t
        path.append(V(helix_r * math.cos(a), helix_r * math.sin(a), z0 + (z1 - z0) * t))
    a_end = TAU * turns
    path.append(V((core - 6.0) * math.cos(a_end), (core - 6.0) * math.sin(a_end), z1 + 4.0))
    m.tube(path, hose_r, RUBBER, sides=8)
    # Ratchet teeth round the top flange's rim (the jam lever's pawl bites them), ribs and rivets on top.
    for i in range(32):
        a0 = math.radians(i * 360.0 / 32)
        a1 = math.radians((i + 0.82) * 360.0 / 32)
        tooth = [((flange - 1.0) * math.cos(a0), (flange - 1.0) * math.sin(a0)),
                 ((flange + 4.0) * math.cos(a0), (flange + 4.0) * math.sin(a0)),
                 ((flange - 1.0) * math.cos(a1), (flange - 1.0) * math.sin(a1))]
        m.prism(tooth, 232.0, 239.0, None, IRON, smooth=False)
    for i in range(8):
        a = 22.5 + i * 45.0
        m.prism([(core + 4.0, 0.0), (flange - 4.0, 0.0), (flange - 4.0, 2.5), (core + 4.0, 7.0)], -2.0, 2.0, Rz(a) @ Rx(90.0) @ T(0.0, 240.0, 0.0), IRON)
        m.rivet(polar(flange - 10.0, a + 22.5, 240.0), (0.0, 0.0, 1.0), r=2.0)
        m.rivet(polar(core + 8.0, a + 22.5, 240.0), (0.0, 0.0, 1.0), r=1.8)
    m.lathe([(0.0, 240.0), (22.0, 240.0), (22.0, 246.0), (15.0, 250.0), (14.0, 270.0), (11.0, 278.0), (0.0, 278.0)], 16, BRASS)
    for i in range(6):
        m.bolt(polar(18.0, 30.0 + i * 60.0, 248.0), (0.0, 0.0, 1.0), r=1.8)
    m.socket('Axle', (0.0, 0.0, 278.0), pitch=90.0)
    return m.finish(lods='50,25', ao=0.6, ground=False, collision=False,
                    soot=lambda p, n: 0.2 * streak(p, 11.0, 5.0) + 0.15 * smoothstep(200.0, 278.0, p.z))


def drum_gantry(name):
    """The housing over the drum on the forecastle: pivot at its middle on the deck, 4 x 2.6 m (DrumGantryHalf), the
    housing 1.4 m high with the drum's well, the gantry over it to 3.7 m."""
    m = Kit(name, 3395)
    hx, hy = GANTRY_HALF
    top = 140.0
    well = 118.0
    # The plates: two ends, two sides, the top in four pieces round the well.
    for s in (-1.0, 1.0):
        m.box((6.0, 2.0 * hy - 4.0, top - 6.0), T(s * (hx - 3.0), 0.0, (top - 6.0) * 0.5), RUST, bevel=1.0)
        m.box((2.0 * hx - 4.0, 6.0, top - 6.0), T(0.0, s * (hy - 3.0), (top - 6.0) * 0.5), RUST, bevel=1.0)
    steps = 10
    for qx, qy in ((1, 1), (-1, 1), (-1, -1), (1, -1)):
        arc = []
        for i in range(steps + 1):
            a = math.radians(90.0 * i / steps)
            arc.append((qx * well * math.cos(a), qy * well * math.sin(a)))
        outline = [(qx * hx, 0.0), (qx * hx, qy * hy), (0.0, qy * hy)] + arc[::-1]
        m.prism(outline, top - 6.0, top, None, RUST, smooth=False)
    m.lathe([(well, 44.0), (well + 6.0, 44.0), (well + 6.0, top - 6.0), (well, top - 6.0)], 32, IRON, loop=True)
    m.lathe([(0.0, 40.0), (well + 4.0, 40.0), (well + 4.0, 46.0), (0.0, 46.0)], 32, IRON)
    m.lathe([(well - 3.0, top - 0.5), (well + 6.0, top - 0.5), (well + 6.0, top + 3.0), (well - 3.0, top + 3.0)], 32, IRON, loop=True)
    # Round 2 (the PIE look check: "a plain stand-in box next to the Tripo detail"): the housing dressed as her machinery.
    # Angle irons riveted down every corner, round the top (a lipped cap: a shadow line) and along the foot (bolted to
    # the deck); a lap seam along each long side; stiffener ribs; a raised hatch with dogs; on the aft end (-X, the well
    # deck's view) the fairlead's gear case with the hose coming out of it and down a hawse into the deck, a gauge and the
    # Company's plate; on the fore end a louvred vent.
    A, t = 11.0, 2.6                     # an angle iron's leg and thickness
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            m.box((t, A, top - 4.0), T(sx * (hx + t * 0.5), sy * (hy - A * 0.5 + t), 0.5 * (top - 4.0) + 2.0), RUST, bevel=0.6)
            m.box((A, t, top - 4.0), T(sx * (hx - A * 0.5 + t), sy * (hy + t * 0.5), 0.5 * (top - 4.0) + 2.0), RUST, bevel=0.6)
            for z in (22.0, 54.0, 86.0, 118.0):
                m.rivet(V(sx * (hx + t), sy * (hy - 5.0), z), (sx, 0.0, 0.0), RUST)
                m.rivet(V(sx * (hx - 5.0), sy * (hy + t), z), (0.0, sy, 0.0), RUST)
    for s in (-1.0, 1.0):
        # the top's angles (a leg down each face, a leg on the top, proud of it: the cap's lip) and the foot's
        m.box((2.0 * hx + 2.0 * t, t, A), T(0.0, s * (hy + t * 0.5), top - A * 0.5 + 1.0), RUST, bevel=0.6)
        m.box((2.0 * hx + 2.0 * t, A + t, 2.2), T(0.0, s * (hy - A * 0.5 + t), top + 1.1), RUST, bevel=0.6)
        m.box((t, 2.0 * hy, A), T(s * (hx + t * 0.5), 0.0, top - A * 0.5 + 1.0), RUST, bevel=0.6)
        m.box((A + t, 2.0 * hy, 2.2), T(s * (hx - A * 0.5 + t), 0.0, top + 1.1), RUST, bevel=0.6)
        m.box((2.0 * hx + 14.0, 8.0, 2.4), T(0.0, s * (hy + 4.0), 1.2), RUST)
        m.box((8.0, 2.0 * hy, 2.4), T(s * (hx + 4.0), 0.0, 1.2), RUST)
        m.box((2.0 * hx + 2.0 * t, t, 9.0), T(0.0, s * (hy + t * 0.5), 6.5), RUST, bevel=0.5)
        m.box((t, 2.0 * hy, 9.0), T(s * (hx + t * 0.5), 0.0, 6.5), RUST, bevel=0.5)
        for x in (-180.0, -120.0, -60.0, 0.0, 60.0, 120.0, 180.0):
            m.rivet(V(x, s * (hy + t), top - 4.5), (0.0, s, 0.0), RUST)
        for y in (-80.0, 0.0, 80.0):
            m.rivet(V(s * (hx + t), y, top - 4.5), (s, 0.0, 0.0), RUST)
        for x in (-150.0, -50.0, 50.0, 150.0):
            m.bolt(V(x, s * (hy + 6.0), 2.4), (0.0, 0.0, 1.0), r=2.0)
        for y in (-70.0, 70.0):
            m.bolt(V(s * (hx + 6.0), y, 2.4), (0.0, 0.0, 1.0), r=2.0)
    # The lap seam along each long side (round the hatch on +Y), riveted, and three stiffener ribs a side.
    for s, spans in ((1.0, ((-hx + 10.0, -177.0), (-73.0, hx - 10.0))), (-1.0, ((-hx + 10.0, hx - 10.0),))):
        for x0, x1 in spans:
            m.box((x1 - x0, 1.8, 9.0), T(0.5 * (x0 + x1), s * (hy + 0.9), 72.0), RUST, bevel=0.4)
            count = max(2, int((x1 - x0) / 40.0))
            for i in range(count):
                m.rivet(V(x0 + (x1 - x0) * (i + 0.5) / count, s * (hy + 1.8), 72.0), (0.0, s, 0.0), RUST, r=1.7)
        for x in ((-40.0, 50.0, 132.0) if s > 0 else (-150.0, -60.0, 40.0)):
            m.box((3.4, 6.0, top - 24.0), T(x, s * (hy + 3.0), 0.5 * (top - 24.0) + 11.0), RUST, bevel=0.8)
    for x, y in ((160.0, -88.0), (-160.0, 88.0)):
        m.lathe([(0.0, top), (22.0, top), (22.0, top + 2.5), (19.0, top + 3.5), (0.0, top + 3.5)], 16, IRON, frame=T(x, y, 0.0))
        for i in range(6):
            m.bolt(V(x, y, 0.0) + polar(18.0, 30.0 + i * 60.0, top + 2.6), (0.0, 0.0, 1.0), r=1.6)
    # The raised inspection hatch on +Y: a riveted coaming, the door on it (stiffened), five dogs, two hinges, a grip.
    hcx, hcz = -125.0, 70.0
    for z in (hcz - 34.0, hcz + 34.0):
        m.box((96.0, 4.0, 8.0), T(hcx, hy + 2.0, z), RUST, bevel=0.6)
    for x in (hcx - 44.0, hcx + 44.0):
        m.box((8.0, 4.0, 60.0), T(x, hy + 2.0, hcz), RUST, bevel=0.6)
    for x, z in ((hcx - 30.0, hcz - 34.0), (hcx + 30.0, hcz + 34.0),
                 (hcx - 44.0, hcz - 20.0), (hcx + 44.0, hcz + 20.0)):
        m.rivet(V(x, hy + 4.0, z), (0.0, 1.0, 0.0), RUST, r=1.8)
    m.box((84.0, 3.0, 64.0), T(hcx, hy + 5.5, hcz), OXIDE, bevel=1.2)
    for z in (hcz - 12.0, hcz + 12.0):
        m.box((70.0, 1.6, 4.0), T(hcx + 3.0, hy + 7.8, z), OXIDE, bevel=0.5)
    for x, z, turn in ((hcx - 18.0, hcz + 33.0, 30.0), (hcx + 18.0, hcz + 33.0, -30.0), (hcx - 18.0, hcz - 33.0, -30.0),
                       (hcx + 18.0, hcz - 33.0, 30.0), (hcx + 43.0, hcz, 90.0)):
        m.lathe([(0.0, 0.0), (2.8, 0.0), (2.8, 5.5), (2.2, 6.2), (0.0, 6.2)], 8, IRON, frame=T(x, hy + 3.0, z) @ ALONG_Y)
        m.box((2.6, 2.2, 11.0), T(x, hy + 8.6, z) @ Ry(turn) @ T(0.0, 0.0, 3.5), IRON, bevel=0.5)
    for z in (hcz - 18.0, hcz + 18.0):
        m.lathe([(0.0, z - 7.0), (3.0, z - 7.0), (3.0, z + 7.0), (0.0, z + 7.0)], 8, IRON, frame=T(hcx - 47.0, hy + 6.5, 0.0))
        m.box((26.0, 1.4, 5.0), T(hcx - 34.0, hy + 7.6, z), IRON, bevel=0.4)
    m.tube([V(hcx + 28.0, hy + 7.0, hcz - 8.0), V(hcx + 28.0, hy + 14.0, hcz - 8.0), V(hcx + 28.0, hy + 14.0, hcz + 8.0),
            V(hcx + 28.0, hy + 7.0, hcz + 8.0)], 1.5, IRON, sides=6)
    # The aft end: the fairlead's gear case (a rounded top, a bolted flange), the bell-mouthed fairlead with two brass
    # rollers, the hose out of it and down a hawse into the deck; the gauge and the Company's plate beside it.
    ex = -hx
    m.box((16.0, 96.0, 84.0), T(ex - 8.0, 0.0, 62.0), RUST, bevel=2.0)
    m.lathe([(0.0, -48.0), (16.0, -48.0), (16.0, 48.0), (0.0, 48.0)], 16, RUST, frame=T(ex, 0.0, 104.0) @ ALONG_Y)
    m.box((2.6, 112.0, 6.0), T(ex - 1.3, 0.0, 17.0), IRON, bevel=0.5)
    for s in (-1.0, 1.0):
        m.box((2.6, 6.0, 100.0), T(ex - 1.3, s * 51.0, 69.0), IRON, bevel=0.5)
        for z in (32.0, 66.0, 100.0):
            m.bolt(V(ex - 2.6, s * 51.0, z), (-1.0, 0.0, 0.0), r=1.9)
    for y in (-34.0, 0.0, 34.0):
        m.bolt(V(ex - 2.6, y, 17.0), (-1.0, 0.0, 0.0), r=1.9)
    m.box((3.0, 70.0, 1.6), T(ex - 16.5, 0.0, 88.0), RUST, bevel=0.4)
    lead_z = 46.0
    mouth = T(ex - 16.0, 0.0, lead_z) @ Ry(-90.0)
    m.lathe([(11.0, 0.0), (19.0, 0.0), (23.0, 4.0), (22.5, 8.5), (19.0, 10.5), (13.0, 8.0)], 14, IRON, loop=True, frame=mouth)
    for i in range(6):
        a = math.radians(30.0 + i * 60.0)
        m.bolt(V(ex - 16.0, 26.0 * math.cos(a), lead_z + 24.0 * math.sin(a)), (-1.0, 0.0, 0.0), r=1.7)
    for y in (-10.5, 10.5):
        m.lathe([(0.0, lead_z - 13.0), (3.2, lead_z - 13.0), (3.2, lead_z + 13.0), (0.0, lead_z + 13.0)], 8, BRASS,
                frame=T(ex - 22.0, y, 0.0))
    hawse_x = ex - 35.0
    m.tube([V(ex - 6.0, 0.0, lead_z), V(ex - 24.0, 0.0, lead_z), V(ex - 30.0, 0.0, lead_z - 4.0), V(hawse_x + 1.0, 0.0, lead_z - 13.0),
            V(hawse_x, 0.0, lead_z - 26.0), V(hawse_x, 0.0, 10.0), V(hawse_x, 0.0, -12.0)], 8.5, RUBBER, sides=8)
    m.lathe([(10.0, 0.0), (19.0, 0.0), (19.0, 2.4), (15.0, 3.0), (14.0, 8.0), (10.0, 8.0)], 16, IRON, loop=True, frame=T(hawse_x, 0.0, 0.0))
    for i in range(6):
        m.bolt(V(hawse_x, 0.0, 0.0) + polar(17.0, i * 60.0, 2.6), (0.0, 0.0, 1.0), r=1.6)
    gauge(m, T(ex, 88.0, 96.0) @ Rz(180.0))
    # The Company's plate: brass, a raised border, rivets, the lamp in a lozenge and two lines of stamped letters (no
    # real company: the barge's owners, VC on her stencils).
    plate = T(ex, -86.0, 90.0) @ Rz(180.0)
    m.box((1.4, 38.0, 22.0), plate @ T(0.7, 0.0, 0.0), BRASS, bevel=0.4)
    for z in (-10.0, 10.0):
        m.box((1.0, 36.0, 1.6), plate @ T(1.9, 0.0, z), BRASS, bevel=0.3)
    for y in (-17.0, 17.0):
        m.box((1.0, 1.6, 18.0), plate @ T(1.9, y, 0.0), BRASS, bevel=0.3)
        for z in (-7.0, 7.0):
            m.rivet(plate @ V(1.4, y * 0.88, z), plate.to_3x3() @ V(1.0, 0.0, 0.0), IRON, r=1.2)
    m.prism([(0.0, -6.0), (4.5, 0.0), (0.0, 6.0), (-4.5, 0.0)], 1.4, 2.6, plate @ T(0.0, -8.0, 1.0) @ Rz(90.0) @ Rx(90.0), BLACK,
            smooth=False)
    for z in (3.0, -3.5):
        m.box((0.8, 16.0 if z > 0 else 12.0, 2.2), plate @ T(1.9, 5.0, z), BLACK)
    # The fore end: a louvred vent for the gear's motor, and a seam with its rivets.
    vent = T(hx, -60.0, 74.0)
    m.box((3.0, 64.0, 40.0), vent @ T(-0.5, 0.0, 0.0), IRON)
    for s in (-1.0, 1.0):
        m.box((5.0, 72.0, 6.0), vent @ T(2.5, 0.0, s * 23.0), RUST, bevel=0.8)
        m.box((5.0, 6.0, 40.0), vent @ T(2.5, s * 33.0, 0.0), RUST, bevel=0.8)
        for z in (-23.0, 23.0):
            m.rivet(vent @ V(5.0, s * 33.0, z), (1.0, 0.0, 0.0), RUST, r=1.7)
    for i in range(5):
        m.box((7.0, 60.0, 1.4), vent @ T(3.0, 0.0, -16.0 + 8.0 * i) @ Ry(35.0), RUST)
    m.box((1.8, 9.0, top - 24.0), T(hx + 0.9, 40.0, 0.5 * (top - 24.0) + 11.0), RUST, bevel=0.4)
    for z in (24.0, 50.0, 76.0, 102.0):
        m.rivet(V(hx + 1.8, 40.0, z), (1.0, 0.0, 0.0), RUST, r=1.7)
    # The gantry over the well: two legs, knee braces, the crossbeam, the drum's top bearing (its axle in it).
    legx = 165.0
    beam_z = 352.0
    for s in (-1.0, 1.0):
        m.box((42.0, 42.0, 3.0), T(s * legx, 0.0, top + 1.5), OXIDE, bevel=0.8)
        m.box((20.0, 26.0, beam_z - top), T(s * legx, 0.0, 0.5 * (top + beam_z)), OXIDE, bevel=1.6)
        for c in (-1.0, 1.0):
            m.bolt(V(s * legx + c * 15.0, 15.0, top + 3.0), (0.0, 0.0, 1.0), r=1.8)
        a = math.degrees(math.atan2(beam_z - 300.0, 55.0))
        m.box((76.0, 8.0, 6.0), T(s * (legx - 32.0), 0.0, 0.5 * (300.0 + beam_z) - 4.0) @ Ry(s * a), OXIDE, bevel=0.8)
    m.box((2.0 * legx + 30.0, 28.0, 28.0), T(0.0, 0.0, beam_z + 14.0), OXIDE, bevel=1.6)
    for s in (-1.0, 1.0):
        for x in range(-6, 7, 2):
            if x == 0:
                continue
            m.rivet(V(x * 26.0, s * 14.2, beam_z + 14.0), (0.0, s, 0.0), r=1.8)
    m.lathe([(0.0, beam_z - 30.0), (28.0, beam_z - 30.0), (30.0, beam_z - 27.0), (30.0, beam_z), (0.0, beam_z)], 20, IRON)
    for i in range(4):
        m.bolt(polar(24.0, 45.0 + i * 90.0, beam_z - 30.0), (0.0, 0.0, -1.0), r=2.0)
    # The jam lever's bracket (two cheeks round its pin, along X) and its quadrant, on the -Y side.
    # In the YZ plane: a prism's outline (y, z) with its thickness along X (YZ_PLANE maps local x, y, z to U y, z, x).
    px, py, pz = LEVER_PIVOT
    YZ_PLANE = Rz(90.0) @ Rx(90.0)
    cheek = [(-hy, pz - 22.0), (py + 6.0, pz - 14.0), (py - 4.0, pz - 6.0), (py - 4.0, pz + 6.0), (py + 6.0, pz + 14.0), (-hy, pz + 22.0)]
    for c in (-1.0, 1.0):
        m.prism(cheek, -1.6, 1.6, T(px + c * 9.0, 0.0, 0.0) @ YZ_PLANE, IRON, smooth=False)
    m.lathe([(0.0, -14.0), (3.0, -14.0), (3.0, 14.0), (0.0, 14.0)], 8, IRON, frame=T(px, py, pz) @ ALONG_X)
    for c in (-1.0, 1.0):
        m.lathe([(0.0, 0.0), (4.2, 0.0), (4.2, 3.0), (0.0, 3.0)], 6, IRON,
                frame=T(px + c * 12.0, py, pz) @ ALONG_X @ (Rx(180.0) if c < 0 else Matrix.Identity(4)))
    # The quadrant the lever's latch drops into, thrown out (roll -35) or in (roll +35: toward the housing), on a strut.
    lo_a, hi_a = -42.0, 36.0
    quad = [(py + 64.0 * math.sin(math.radians(lo_a + (hi_a - lo_a) * i / 10)), pz + 64.0 * math.cos(math.radians(lo_a + (hi_a - lo_a) * i / 10)))
            for i in range(11)]
    quad += [(py + 74.0 * math.sin(math.radians(lo_a + (hi_a - lo_a) * i / 10)), pz + 74.0 * math.cos(math.radians(lo_a + (hi_a - lo_a) * i / 10)))
             for i in range(10, -1, -1)]
    qx = px + 6.0
    m.prism(quad, -1.6, 1.6, T(qx, 0.0, 0.0) @ YZ_PLANE, IRON, smooth=False)
    for a in (-35.0, 32.0):
        m.box((3.0, 4.0, 4.0), T(qx, py + 77.0 * math.sin(math.radians(a)), pz + 77.0 * math.cos(math.radians(a))) @ Rx(-a), BRASS, bevel=0.6)
    m.box((3.0, 4.0, 53.0), T(qx, py, pz + 38.5), IRON, bevel=0.6)
    # Collision: the housing, the two legs, the beam, the lever's bracket.
    m.hull_box((-hx - 6.0, -hy - 6.0, 0.0), (hx + 6.0, hy + 6.0, top + 3.0))
    for s in (-1.0, 1.0):
        m.hull_box((s * legx - 12.0, -14.0, top), (s * legx + 12.0, 14.0, beam_z))
    m.hull_box((-legx - 16.0, -15.0, beam_z), (legx + 16.0, 15.0, beam_z + 28.0))
    m.hull_box((px - 14.0, py - 8.0, pz - 22.0), (px + 16.0, -hy, pz + 22.0))
    m.socket('Drum', (0.0, 0.0, DRUM_UP))
    m.socket('LeverPivot', LEVER_PIVOT)
    m.socket('Sparks', (0.0, -hy + 20.0, top + 30.0), pitch=60.0)

    def soot(p, n):
        w = 0.2 * streak(p, 6.0, 4.0)
        if math.hypot(p.x, p.y) < well + 8.0 and p.z < top + 4.0:
            w += 0.5
        # Round the fairlead and its hawse (the hose's grease and the furnace's smuts), streaking up the gear case.
        d = (p - V(ex - 16.0, 0.0, lead_z)).length
        if d < 60.0:
            w += 0.6 * (1.0 - d / 60.0)
        if p.x < ex + 2.0 and abs(p.y) < 52.0 and p.z > lead_z:
            w += 0.3 * max(0.0, 1.0 - (p.z - lead_z) / 80.0) * (0.5 + streak(p, 22.0, 6.0))
        if math.hypot(p.x - hawse_x, p.y) < 40.0 and p.z < 12.0:
            w += 0.45
        return w
    return m.finish(lods='50,25', ao=0.8, soot=soot)


def drum_lever(name):
    """The jam lever: pivot on its pin's middle (the pin along X: the code rolls it about X), the arm up +Z, the brass
    grip at its top."""
    m = Kit(name, 3398)
    m.lathe([(0.0, -7.0), (7.0, -7.0), (7.5, -6.0), (7.5, 6.0), (7.0, 7.0), (0.0, 7.0)], 12, IRON, frame=ALONG_X)
    m.prism([(-2.6, -6.0), (2.6, -6.0), (2.0, 90.0), (-2.0, 90.0)], -1.6, 1.6, Rz(90.0) @ Rx(90.0), IRON, smooth=False)
    m.prism([(-3.0, -2.0), (3.0, -2.0), (0.0, -15.0)], -1.4, 1.4, Rz(90.0) @ Rx(90.0) @ T(0.0, 0.0, 0.0), IRON, smooth=False)
    m.lathe([(0.0, 86.0), (3.0, 86.0), (3.4, 90.0), (3.8, 96.0), (3.6, 104.0), (4.1, 108.0), (3.4, 111.0), (0.0, 112.0)], 10, BRASS)
    m.tube([V(0.0, -2.6, 18.0), V(0.0, -2.6, 80.0)], 0.8, IRON, sides=5)
    m.box((6.0, 4.0, 4.0), T(0.0, -2.0, 18.0), IRON, bevel=0.6)
    m.box((4.0, 5.0, 7.0), T(0.0, -3.0, 82.0), BRASS, bevel=0.6)
    return m.finish(lods=None, ao=0.2, ground=False, collision=False)


# --- The stoker's wireless ---

def radio(name):
    """A Company wireless set: pivot at its foot's middle, the dials' face +X; 30 deep (X), 45 wide (Y), 30 high."""
    m = Kit(name, 3400)
    dx, dy, dz = 30.0, 45.0, 28.0
    m.box((dx, dy, dz), T(0.0, 0.0, 2.0 + dz * 0.5), OXIDE, bevel=1.2, segs=2)
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            m.box((5.0, 5.0, 2.0), T(sx * (dx * 0.5 - 3.5), sy * (dy * 0.5 - 3.5), 1.0), IRON, bevel=0.5)
            m.box((3.2, 3.2, dz + 0.6), T(sx * (dx * 0.5 - 1.0), sy * (dy * 0.5 - 1.0), 2.0 + dz * 0.5), IRON, bevel=0.6)
    m.box((dx + 1.0, dy + 1.0, 1.6), T(0.0, 0.0, 2.0 + dz - 3.5), IRON, bevel=0.5)
    front = dx * 0.5
    m.box((1.0, dy - 6.0, dz - 8.0), T(front + 0.3, 0.0, 2.0 + dz * 0.5 - 1.0), BLACK, bevel=0.3)
    for y in (-10.0, 10.0):
        D = T(front + 0.8, y, 19.0) @ ALONG_X
        m.lathe([(0.0, 0.0), (6.2, 0.0), (6.4, 1.2), (5.6, 1.6), (5.2, 1.0), (0.0, 1.0)], 16, BRASS, frame=D)
        m.lathe([(0.0, 0.6), (5.2, 0.6), (5.2, 1.1), (0.0, 1.1)], 16, CREAM, frame=D)
        m.box((0.3, 0.5, 4.4), T(front + 2.0, y, 19.0) @ Rx(30.0 if y < 0 else -55.0) @ T(0.0, 0.0, 1.6), BLACK)
    for y in (-15.0, 0.0, 15.0):
        D = T(front + 0.8, y, 8.5) @ ALONG_X
        m.lathe([(0.0, 0.0), (3.0, 0.0), (3.0, 0.8), (2.2, 1.2), (2.2, 3.0), (1.8, 3.4), (0.0, 3.4)], 10, IRON, frame=D)
    m.box((0.6, 9.0, 3.4), T(front + 0.6, 0.0, 13.5), CREAM, bevel=0.2)
    for i in range(8):
        m.rivet(V(front, -dy * 0.5 + 3.0 + (dy - 6.0) * i / 7, 2.0 + dz - 2.0), (1.0, 0.0, 0.0), r=0.9)
    # The brass key on its iron base, on top toward the front.
    m.box((10.0, 6.0, 1.4), T(7.0, 12.0, 2.0 + dz + 0.7), IRON, bevel=0.4)
    m.box((12.0, 1.4, 1.0), T(8.0, 12.0, 2.0 + dz + 2.6) @ Ry(-4.0), BRASS, bevel=0.3)
    m.lathe([(0.0, 0.0), (1.8, 0.0), (1.9, 0.8), (1.6, 1.6), (0.0, 1.8)], 10, BLACK, frame=T(13.5, 12.0, 2.0 + dz + 3.0))
    m.lathe([(0.0, 0.0), (0.8, 0.0), (0.8, 2.6), (0.0, 2.6)], 6, BRASS, frame=T(3.0, 12.0, 2.0 + dz + 1.4))
    # The aerial at the back left corner, terminals on the back, carry handles at the sides.
    ax, ay = -dx * 0.5 + 4.0, -dy * 0.5 + 4.5
    m.lathe([(0.0, 0.0), (2.2, 0.0), (2.4, 1.0), (1.6, 1.8), (2.0, 2.6), (1.4, 3.6), (0.0, 3.6)], 10, CREAM, frame=T(ax, ay, 2.0 + dz))
    m.tube([V(ax, ay, 2.0 + dz + 3.0), V(ax, ay, 2.0 + dz + 36.0)], 0.5, IRON, sides=5)
    m.lathe([(0.0, 0.0), (1.2, 0.3), (1.3, 1.3), (0.8, 2.4), (0.0, 2.6)], 8, BRASS, frame=T(ax, ay, 2.0 + dz + 35.0))
    for y in (8.0, 15.0):
        m.lathe([(0.0, 0.0), (1.2, 0.0), (1.2, 2.0), (1.8, 2.2), (1.8, 3.2), (0.0, 3.4)], 8, BRASS,
                frame=T(-dx * 0.5, y, 10.0) @ Ry(-90.0))
    for s in (-1.0, 1.0):
        y = s * (dy * 0.5 + 0.5)
        m.tube([V(-7.0, y, 22.0), V(-7.0, y + s * 3.0, 24.0), V(7.0, y + s * 3.0, 24.0), V(7.0, y, 22.0)], 0.9, IRON, sides=6)
    m.hull_box((-dx * 0.5, -dy * 0.5, 0.0), (dx * 0.5 + 3.0, dy * 0.5, 2.0 + dz + 3.0))
    m.socket('Speaker', (front + 1.0, 0.0, 16.0))
    return m.finish(lods='50', screens='0.2', ao=0.2, soot=lambda p, n: 0.15 * streak(p, 20.0, 8.0))


# --- Build ---

BUILDERS = [
    ('BargeFurnace', lambda n: furnace(n)),
    ('BargeFurnaceBurst', lambda n: furnace(n, burst=True)),
    ('BargeCageShard', cage_shard),
    ('BargeFurnaceGrate', furnace_grate),
    ('BargeDraftValve', draft_valve),
    ('BargeValveWheel', valve_wheel),
    ('BargePipe', pipe),
    ('BargeDeckVent', deck_vent),
    ('BargeFurnaceVent', furnace_vent),
    ('BargeFireTongue', fire_tongue),
    ('BargeStack', stack),
    ('BargeStackFoot', stack_foot),
    ('BargeHoseDrum', hose_drum),
    ('BargeDrumGantry', drum_gantry),
    ('BargeDrumLever', drum_lever),
    ('BargeRadio', radio),
]

models = {}
for model_name, build in BUILDERS:
    if ONLY is None or model_name in ONLY:
        models[model_name] = build(model_name)


# --- Previews (--preview): each alone, the lineup, and the pieces put together as the code lays them ---

def _copy(obj, at=(0.0, 0.0, 0.0), yaw=0.0, scale=(1.0, 1.0, 1.0), roll=0.0, pitch=0.0):
    """A linked copy of a model placed by an Unreal transform (cm, degrees; scale in Unreal's axes)."""
    copy = obj.copy()
    copy.name = 'PV_' + obj.name
    bpy.context.scene.collection.objects.link(copy)
    copy.location = B(at)
    copy.rotation_mode = 'XYZ'
    copy.rotation_euler = (math.radians(-pitch), math.radians(roll), math.radians(-yaw))
    copy.scale = (scale[1], scale[0], scale[2])
    bpy.context.view_layer.update()
    return copy


def _clear(copies):
    for c in copies:
        bpy.data.objects.remove(c)


if PREVIEW:
    _preview_glow()
    for model_name, obj in models.items():
        view = (-1.0, -1.6, 0.7)
        if model_name in ('BargeFurnaceVent',):
            view = (-1.0, -1.6, -0.5)
        lt.preview([obj], os.path.join(OUT, model_name + '.png'), view=view, fit=1.0,
                   ground=model_name not in ('BargeFurnaceVent', 'BargeValveWheel', 'BargeDrumLever'))
    if 'BargeFurnace' in models:
        f = models['BargeFurnace']
        lt.preview([f], os.path.join(OUT, 'BargeFurnace_front.png'), view=(-0.25, -1.6, 0.35), fit=0.95)
        lt.preview([f], os.path.join(OUT, 'BargeFurnace_back.png'), view=(0.9, 1.6, 0.5), fit=1.0)
        lt.preview([f], os.path.join(OUT, 'BargeFurnace_porthole.png'), view=(-0.45, -1.6, 0.05), fit=0.42)
    if 'BargeFurnaceBurst' in models:
        lt.preview([models['BargeFurnaceBurst']], os.path.join(OUT, 'BargeFurnaceBurst_front.png'), view=(-0.35, -1.6, 0.35), fit=0.95)
    if 'BargeDrumGantry' in models:
        # Its ends at a deck-walker's eye: the aft end (the well deck's view) and the fore end (the forecastle's), its +Y side.
        g = models['BargeDrumGantry']
        lt.preview([g], os.path.join(OUT, 'BargeDrumGantry_aft.png'), view=(0.45, 1.6, 0.22), fit=0.8)
        lt.preview([g], os.path.join(OUT, 'BargeDrumGantry_fore.png'), view=(-0.45, -1.6, 0.22), fit=0.8)
        lt.preview([g], os.path.join(OUT, 'BargeDrumGantry_side.png'), view=(-1.6, 0.35, 0.25), fit=0.85)
    # The lineup: every model side by side (their pivots on one line, the front of each toward the camera).
    if len(models) > 1:
        copies, x = [], 0.0
        order = [n for n, _ in BUILDERS if n in models]
        for n in order:
            obj = models[n]
            lo = min((obj.matrix_world @ Vector(c)).x for c in obj.bound_box)
            hi = max((obj.matrix_world @ Vector(c)).x for c in obj.bound_box)
            copy = obj.copy()
            copy.name = 'PV_' + n
            bpy.context.scene.collection.objects.link(copy)
            copy.location = (x - lo, 0.0, 0.0)
            x += (hi - lo) + 0.8
            copies.append(copy)
        bpy.context.view_layer.update()
        lt.preview(copies, os.path.join(OUT, 'lineup.png'), resolution=(1920, 720), view=(0.0, -1.6, 0.35), fit=0.75)
        _clear(copies)
        small = [n for n in order if n not in ('BargeFurnace', 'BargeFurnaceBurst', 'BargeStack', 'BargeDrumGantry')]
        copies, x = [], 0.0
        for n in small:
            obj = models[n]
            lo = min((obj.matrix_world @ Vector(c)).x for c in obj.bound_box)
            hi = max((obj.matrix_world @ Vector(c)).x for c in obj.bound_box)
            copy = obj.copy()
            copy.name = 'PV_' + n
            bpy.context.scene.collection.objects.link(copy)
            copy.location = (x - lo, 0.0, 0.0)
            x += (hi - lo) + 0.4
            copies.append(copy)
        bpy.context.view_layer.update()
        lt.preview(copies, os.path.join(OUT, 'lineup_small.png'), resolution=(1920, 720), view=(0.0, -1.6, 0.45), fit=0.75)
        _clear(copies)
    # The hall: the furnace, the twelve wedges, the three valves with their wheels and risers, the furnace's pipes up
    # to the ceiling (HallHeight 900), as ABargeFurnace and ABargeBoarding lay them (OfficeAngle 0).
    need = ('BargeFurnace', 'BargeFurnaceGrate', 'BargeDraftValve', 'BargeValveWheel', 'BargePipe')
    if all(n in models for n in need):
        copies = [_copy(models['BargeFurnace'])]
        for wedge in range(GRATES):
            a = (wedge + 0.5) * 360.0 / GRATES
            copies.append(_copy(models['BargeFurnaceGrate'], tuple(polar(GRATE_MID, a)), yaw=a))
        for a in (60.0, 180.0, 300.0):
            at = polar(700.0 - 30.0, a)
            yaw = a + 180.0
            copies.append(_copy(models['BargeDraftValve'], tuple(at), yaw=yaw))
            F = T(*at) @ Rz(yaw)
            copies.append(_copy(models['BargeValveWheel'], tuple(F @ V(*VALVE_WHEEL)), yaw=yaw))
            top = F @ V(VALVE_PIPE_X, 0.0, 300.0)
            for k in range(2):
                copies.append(_copy(models['BargePipe'], tuple(top + V(0.0, 0.0, 300.0 * k)), yaw=yaw, scale=(0.9, 0.9, 1.0)))
        for x, y in FURNACE_PIPES:
            copies.append(_copy(models['BargePipe'], (x, y, PIPE_COLLAR_TOP), scale=(1.1, 1.1, (900.0 - PIPE_COLLAR_TOP) / 300.0)))
        lt.preview(copies, os.path.join(OUT, 'assembly_hall.png'), resolution=(1600, 900), view=(-1.0, -1.3, 0.55), fit=0.85)
        lt.preview(copies, os.path.join(OUT, 'assembly_hall_eye.png'), resolution=(1600, 900), view=(-0.3, -1.6, 0.12), fit=0.62)
        _clear(copies)
    # The deck: the gantry with its drum and lever, the stack on its foot, two vents (one burning), the radio.
    need = ('BargeDrumGantry', 'BargeHoseDrum', 'BargeDrumLever', 'BargeStack', 'BargeStackFoot', 'BargeDeckVent', 'BargeFireTongue')
    if all(n in models for n in need):
        copies = [_copy(models['BargeDrumGantry']), _copy(models['BargeHoseDrum'], (0.0, 0.0, DRUM_UP), yaw=20.0),
                  _copy(models['BargeDrumLever'], LEVER_PIVOT, roll=-25.0),
                  _copy(models['BargeStackFoot'], (-700.0, 200.0, 0.0)), _copy(models['BargeStack'], (-700.0, 200.0, 0.0)),
                  _copy(models['BargeDeckVent'], (-150.0, -450.0, 0.0)), _copy(models['BargeDeckVent'], (300.0, -500.0, 0.0)),
                  _copy(models['BargeFireTongue'], (300.0, -500.0, 8.0), scale=(2.6, 2.6, 4.5))]
        if 'BargeRadio' in models:
            copies.append(_copy(models['BargeRadio'], (330.0, 180.0, 0.0), yaw=200.0))
        lt.preview(copies, os.path.join(OUT, 'assembly_deck.png'), resolution=(1600, 900), view=(1.0, -1.4, 0.45), fit=0.8)
        _clear(copies)
        copies = [_copy(models['BargeDrumGantry']), _copy(models['BargeHoseDrum'], (0.0, 0.0, DRUM_UP), yaw=20.0),
                  _copy(models['BargeDrumLever'], LEVER_PIVOT, roll=-25.0)]
        lt.preview(copies, os.path.join(OUT, 'assembly_gantry.png'), resolution=(1600, 900), view=(-0.7, -1.6, 0.5), fit=0.9)
        _clear(copies)
    if 'BargeFurnaceBurst' in models and 'BargeCageShard' in models:
        copies = [_copy(models['BargeFurnaceBurst'])]
        rnd = random.Random(6006)
        for i in range(12):
            copies.append(_copy(models['BargeCageShard'], (rnd.uniform(420.0, 900.0), rnd.uniform(-380.0, 380.0), 0.0),
                                yaw=rnd.uniform(0.0, 360.0), scale=(rnd.uniform(0.7, 1.3),) * 3))
        lt.preview(copies, os.path.join(OUT, 'assembly_burst.png'), resolution=(1600, 900), view=(-0.6, -1.6, 0.55), fit=0.8)
        _clear(copies)
