"""The Stone Sentinel from the user's Tripo model (Art/Models/Creatures/Tripo/StoneSentinel.glb, made from
Art/Concepts/Creatures/StoneSentinel/upload): its shape. Not a model: Tools\\models.ps1 skips parts_*.py;
StoneSentinel.py runs it (LOOTER_SENTINEL_TRIPO, default on) and parts_sentinel_look.py textures what it makes.

Stone moves stiffly, so the statue is cut into rigid pieces, one per bone, each weighted wholly to it:
1. load(): Tripo's mesh (one watertight shell, 27.6k triangles, its front -Y, standing neutral with its fists at its
   sides), scaled to 2.37 m, its feet on the ground.
2. cut(): Exact booleans against cutter solids (CUTS), so every cut is capped. The joints are stone ball joints: the
   parent keeps a ball (or, at the hips and knees, a drum round the lateral axis) inside the child, so a bend opens on
   stone, never on a hole; where a child turns round the lateral axis (the legs, the arms' swing) the seam lies in a plane
   or drum the turn keeps, so it slides instead of gaping. The sleeves are fused to the robe down their inner side:
   the arm cut runs between them (x0 below).
3. retopo(): each piece remeshed on its own (QuadriFlow, Blender's built-in) to clean, even quads, about 21k triangles
   in all; its cap faces keep the _Cap slot (the look paints them as fresh-cut stone).
4. pose(): the pieces turned into the dormant statue: the hood bowed, the arms brought forward, the fists clasped (left
   over right) before the belt. The game's poses are offsets from this rest (Creatures/StoneSentinelCreaturePose.cpp).
Space: Blender metres, the statue's left +X, its front -Y, up +Z (lc.TO_BLENDER of creature space).
"""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Quaternion, Vector

import looter_textures as lt
import looter_tripo as tripo

GLB = os.path.join(lt.REPO, 'Art', 'Models', 'Creatures', 'Tripo', 'StoneSentinel.glb')
HEIGHT = 2.37
QUADS = 6600                 # QuadriFlow faces over the whole statue (two triangles each)
VOXEL = 0.005                # m: the clean-up pass before QuadriFlow
BIG = 3.0

# The joints (left side; the right mirrors x). Measured on the scaled model (slices and sections, 2026-10-29).
NECK = Vector((0.0, 0.20, 1.80))          # the head's pivot, at the back of the neck: bowing, the back of the
                                          # hood stays on its ball (radius NECK_R, the seam there) and the front sinks
NECK_R = 0.30
HOOD_PLANE = (Vector((0.0, -0.30, 1.74)), Vector((0.0, 0.36, 1.96)))   # the hood's lower edge, front to back
CHEST = Vector((0.0, 0.02, 1.56))         # spine_02's pivot; seam z 1.62, its ball inside
CHEST_Z, CHEST_R = 1.62, 0.22
WAIST = Vector((0.0, 0.0, 1.42))          # spine_01's pivot; seam z 1.48 (over the belt), its ball inside
WAIST_Z, WAIST_R = 1.48, 0.24
HIP_Y, HIP_Z, HIP_R = -0.03, 1.00, 0.42   # the drum the legs turn in (round the lateral axis)
KNEE_Y, KNEE_Z, KNEE_R = -0.02, 0.58, 0.47
ANKLE, ANKLE_R, ANKLE_Z = Vector((0.36, 0.02, 0.15)), 0.07, 0.15
SHOULDER = Vector((0.40, 0.04, 1.92))       # the upper arm's pivot, at the top of the shoulder under the mantle: it
                                          # swings round the lateral axis there (sliding on its flank seam) and, brought in,
                                          # tips round the forward axis there, so the shoulder's top never opens
SHOULDER_DOME, SHOULDER_DOME_R = 0.0, 0.42   # no dome: where it swelled into the sleeve it left the flank thin sheets
ELBOW, ELBOW_R, ELBOW_Z = Vector((0.565, 0.07, 1.40)), 0.16, 1.40
WRIST, WRIST_R, WRIST_Z = Vector((0.62, -0.09, 1.04)), 0.07, 1.04
FIST = Vector((0.625, -0.10, 0.93))
# The arm's inner seam (x over z) between the sleeve and the robe it hangs against: down the flank, then out along
# the cuff, under the fist.
ARM_INNER = 0.27
HOOD_HALF = 0.335
# The seam runs through the trunk's solid stone, a little inside its flank: where it ran between the sleeve and the robe
# (fused there in Tripo's shell) it left thin sheets on both sides that stood off as fins once the arm came forward.
# Down the waist it steps out round the belt and the robe.
ARM_SEAM = [(ARM_INNER, 2.6), (ARM_INNER, 1.58), (0.34, 1.45), (0.445, 1.03), (0.50, 0.95)]
ARM_BOTTOM = 0.95
HAND_BOX = ((0.445, -0.42, 0.70), (0.95, 0.24, WRIST_Z))
HAND_INNER_LOW = (0.475, 0.86)              # the box's inner face leans out below the knuckles, clear of the robe
HAND_ROUND = ((0.60, -0.10), (0.15, 0.165))      # the fist's footprint (centre, radii)

BONES = ['pelvis', 'spine_01', 'spine_02', 'head', 'upperarm_l', 'lowerarm_l', 'hand_l', 'upperarm_r', 'lowerarm_r',
         'hand_r', 'thigh_l', 'calf_l', 'foot_l', 'thigh_r', 'calf_r', 'foot_r']


def log(message):
    print(f'SENTINEL: {message}', flush=True)


def mirror(v, s):
    return Vector((v[0] * s, v[1], v[2]))


# --- Solids ---

def _material(name, color):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.diffuse_color = color
    return mat


def solid(name, bm):
    bm.normal_update()
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.materials.append(_material('_Cap', (0.9, 0.3, 0.2, 1.0)))
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def hull(name, points):
    bm = bmesh.new()
    for p in points:
        bm.verts.new(p)
    made = bmesh.ops.convex_hull(bm, input=list(bm.verts))
    junk = [v for v in made['geom_interior'] + made['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(junk)), context='VERTS')
    return solid(name, bm)


def box(name, lo, hi):
    return hull(name, [Vector((x, y, z)) for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])])


def halfspace(name, point, normal):
    """The solid on normal's side of the plane through point (a big box)."""
    n = Vector(normal).normalized()
    a = n.orthogonal().normalized()
    b = n.cross(a)
    p = Vector(point)
    pts = [p + a * (i * BIG) + b * (j * BIG) + n * (k * BIG) for i in (-1, 1) for j in (-1, 1) for k in (0, 1)]
    return hull(name, pts)


def sphere(name, centre, radius):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=20, v_segments=10, radius=radius,
                              matrix=Matrix.Translation(Vector(centre)))
    return solid(name, bm)


def drum_or_below(name, y, z, radius, segments=40):
    """Everything under the level z plus the drum (a cylinder round the lateral, x, axis through (y, z)) standing on it:
    a child that turns round that axis inside its parent. Built as one profile swept along x (a boolean union of a box
    and a cylinder meets on the cylinder's own edges: Exact gets those wrong now and then)."""
    profile = [Vector((y - BIG, z)), Vector((y - radius, z))]
    profile += [Vector((y - radius * math.cos(math.pi * k / segments), z + radius * math.sin(math.pi * k / segments)))
                for k in range(1, segments)]
    profile += [Vector((y + radius, z)), Vector((y + BIG, z)), Vector((y + BIG, z - BIG)), Vector((y - BIG, z - BIG))]
    bm = bmesh.new()
    rings = [[bm.verts.new((x, p.x, p.y)) for p in profile] for x in (-BIG, BIG)]
    n = len(profile)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((rings[0][i], rings[0][j], rings[1][j], rings[1][i]))
    bm.faces.new(rings[0][::-1])
    bm.faces.new(rings[1])
    return solid(name, bm)


def _boolean(target, operand, op, self_check=True, tolerant=False):
    """target op operand (Exact), as a new object; neither input is changed. Faces from the operand keep its _Cap slot."""
    obj = target.copy()
    obj.data = target.data.copy()
    bpy.context.scene.collection.objects.link(obj)
    mod = obj.modifiers.new('cut', 'BOOLEAN')
    mod.operation = op
    mod.solver = 'EXACT'
    mod.object = operand
    mod.material_mode = 'TRANSFER'
    mod.use_self = self_check       # QuadriFlow's shell crosses itself in a few thin folds
    mod.use_hole_tolerant = tolerant
    deps = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(obj.evaluated_get(deps))
    old = obj.data
    obj.modifiers.clear()
    obj.data = me
    bpy.data.meshes.remove(old)
    return obj


def union(name, *solids):
    out = solids[0]
    for s in solids[1:]:
        nxt = _boolean(out, s, 'UNION')
        if out is not solids[0]:
            _remove(out)
        out = nxt
    out.name = name
    return out


def minus(name, a, b):
    out = _boolean(a, b, 'DIFFERENCE')
    out.name = name
    return out


def _remove(obj):
    me = obj.data
    bpy.data.objects.remove(obj)
    if me and not me.users:
        bpy.data.meshes.remove(me)


def _open_edges(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    n = sum(1 for e in bm.edges if not e.is_manifold)
    bm.free()
    return n


def _heal(obj):
    """Mends what a cut through a thin fold leaves: edges shared by more than two faces split apart, slivers and
    duplicate points merged, holes filled."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=1e-6)
    many = [e for e in bm.edges if len(e.link_faces) > 2]
    if many:
        bmesh.ops.split_edges(bm, edges=many)
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context='VERTS')
    open_ = [e for e in bm.edges if e.is_boundary]
    if open_:
        bmesh.ops.holes_fill(bm, edges=open_, sides=0)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data)
    bm.free()


def _bounds(obj):
    co = np.array([v.co[:] for v in obj.data.vertices]) if obj.data.vertices else np.zeros((1, 3))
    return co.min(0), co.max(0)


def split(piece, cutter, inside_name, outside_name):
    """(piece inside cutter, piece outside it), both capped; piece and cutter are removed."""
    # A boolean that lost track of inside and outside hands back the cutter's own far faces: nothing cut may reach past
    # the piece it came from. Exact misses now and then on a shell crossing itself: tried again other ways, then with
    # the cutter nudged under a millimetre.
    lo, hi = _bounds(piece)
    tries = [(True, False, 0.0), (False, False, 0.0), (True, True, 0.0), (True, False, 0.0007), (False, True, 0.0011)]
    for k, (self_check, tolerant, nudge) in enumerate(tries):
        cutter.location = (nudge, nudge * 0.7, -nudge * 0.5)
        bpy.context.view_layer.update()
        a = _boolean(piece, cutter, 'INTERSECT', self_check, tolerant)
        b = _boolean(piece, cutter, 'DIFFERENCE', self_check, tolerant)
        bad = [o for o in (a, b) if o.data.polygons and (any(_bounds(o)[0][i] < lo[i] - 0.01 for i in range(3))
                                                         or any(_bounds(o)[1][i] > hi[i] + 0.01 for i in range(3)))]
        if not bad:
            break
        _remove(a)
        _remove(b)
        if k == len(tries) - 1:
            raise RuntimeError(f'the cut of {piece.name} into {inside_name} / {outside_name} failed every way')
        log(f'cut of {piece.name} into {inside_name} / {outside_name} missed (try {k + 1}); again')
    a.name, b.name = inside_name, outside_name
    for o in (a, b):
        n = _open_edges(o)
        if n:
            _heal(o)
            m = _open_edges(o)
            if m:
                log(f'{o.name}: {n} open edges after the cut of {piece.name}, {m} after healing')
    _remove(piece)
    _remove(cutter)
    return a, b


# --- 1. Load ---

def load():
    obj, maps = tripo.load(GLB)
    me = obj.data
    co = np.array([v.co[:] for v in me.vertices])
    scale = HEIGHT / (co[:, 2].max() - co[:, 2].min())
    me.transform(Matrix.Translation((0.0, 0.0, -co[:, 2].min() * scale)) @ Matrix.Scale(scale, 4))
    me.materials.clear()
    me.materials.append(_material('_Skin', (0.6, 0.6, 0.6, 1.0)))
    for p in me.polygons:
        p.material_index = 0
        p.use_smooth = True
    me.update()
    log(f'Tripo model: {len(me.polygons)} triangles, scaled {scale:.4f} to {HEIGHT} m')
    return obj, maps


# --- 2. Cut ---

def _seam_x(z):
    """The arm seam's x (left side) at height z, before the shoulder dome: ARM_SEAM's broken line."""
    pts = ARM_SEAM[::-1]
    if z <= pts[0][1]:
        return pts[0][0]
    for (x0, z0), (x1, z1) in zip(pts, pts[1:]):
        if z <= z1:
            return x0 + (x1 - x0) * (z - z0) / (z1 - z0)
    return pts[-1][0]


def _arm_cutter(s, rows=56, cols=44):
    """The arm's side of its seam: x past the seam, which swells into a dome round the shoulder's lateral axis (a
    surface of revolution round it, so the arm swinging forward or up slides over the dome and the flank it bares is
    rounded stone, not a hole or a flat cut)."""
    sy, sz = SHOULDER.y, SHOULDER.z
    zs = [ARM_BOTTOM + (2.6 - ARM_BOTTOM) * j / rows for j in range(rows + 1)]
    ys = [-0.8 + 1.6 * i / cols for i in range(cols + 1)]

    def seam(y, z):
        r = math.hypot(y - sy, z - sz)
        dome = SHOULDER_DOME * max(0.0, 1.0 - (r / SHOULDER_DOME_R) ** 2) ** 2
        return _seam_x(z) + dome

    bm = bmesh.new()
    front = [[bm.verts.new((seam(y, z) * s, y, z)) for y in ys] for z in zs]
    back = [[bm.verts.new((1.3 * s, y, z)) for y in ys] for z in zs]
    for grid in (front, back):
        for j in range(rows):
            for i in range(cols):
                bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
    ring = [(j, 0) for j in range(rows)] + [(rows, i) for i in range(cols)] + \
           [(j, cols) for j in range(rows, 0, -1)] + [(0, i) for i in range(cols, 0, -1)]
    for k in range(len(ring)):
        (j0, i0), (j1, i1) = ring[k], ring[(k + 1) % len(ring)]
        bm.faces.new((front[j0][i0], front[j1][i1], back[j1][i1], back[j0][i0]))
    return solid('_ArmCutter', bm)




def _hand_cutter(s):
    (x0, y0, z0), (x1, y1, z1) = HAND_BOX
    xl, zl = HAND_INNER_LOW
    pts = [Vector((x * s, y, z)) for x in (x1,) for y in (y0, y1) for z in (z0, z1)]
    pts += [Vector((x0 * s, y, z)) for y in (y0, y1) for z in (zl, z1)]
    pts += [Vector((xl * s, y, z0)) for y in (y0, y1)]
    box_ = hull('_HandBox', pts)
    # Only round the fist: the cuff's hem hangs below the wrist all round it and stays the forearm's.
    (cx, cy), (rx, ry) = HAND_ROUND
    ring = [Vector(((cx + rx * math.cos(a)) * s, cy + ry * math.sin(a), z)) for z in (z0, z1)
            for a in (2.0 * math.pi * k / 32 for k in range(32))]
    round_ = hull('_HandRound', ring)
    cut = _boolean(box_, round_, 'INTERSECT')
    ball = sphere('_WristBall', mirror(WRIST, s), WRIST_R)
    out = minus('_HandCutter', cut, ball)
    for o in (box_, round_, cut):
        _remove(o)
    _remove(ball)
    return out


def _above_minus_ball(z, centre, radius, name):
    high = box('_High', (-BIG, -BIG, z), (BIG, BIG, BIG))
    ball = sphere('_Ball', centre, radius)
    out = minus(name, high, ball)
    _remove(high)
    _remove(ball)
    return out


def _below_minus_ball(z, centre, radius, name):
    """A child hanging under its parent: under the seam plane, outside the parent's ball."""
    low = box('_Low', (-BIG, -BIG, -BIG), (BIG, BIG, z))
    ball = sphere('_Ball', centre, radius)
    out = minus(name, low, ball)
    _remove(low)
    _remove(ball)
    return out


def cut(source, tag='P'):
    """The statue cut into its pieces: {bone: object named _<tag>_<bone>}; source is used up."""
    pieces = {}
    rest = source
    # The hood first, between its own sides (the shoulders beside it go with the arms).
    p0, p1 = HOOD_PLANE
    up = (p1 - p0).cross(Vector((1.0, 0.0, 0.0))).normalized()
    if up.z < 0.0:
        up = -up
    above = halfspace('_AboveHood', p0, up)
    ball = sphere('_NeckBall', NECK, NECK_R)
    hood = minus('_HoodCut', above, ball)
    middle = box('_Middle', (-HOOD_HALF, -BIG, 0.0), (HOOD_HALF, BIG, BIG))
    head = _boolean(hood, middle, 'INTERSECT')
    for o in (above, ball, hood, middle):
        _remove(o)
    pieces['head'], rest = split(rest, head, '_P_head', '_Rest')
    for s, sfx in ((1.0, 'l'), (-1.0, 'r')):
        pieces[f'hand_{sfx}'], rest = split(rest, _hand_cutter(s), f'_P_hand_{sfx}', '_Rest')
        arm, rest = split(rest, _arm_cutter(s), '_Arm', '_Rest')
        # The upper arm keeps its elbow ball: the lower arm is what lies under the seam, outside the ball.
        fore = _below_minus_ball(ELBOW_Z, mirror(ELBOW, s), ELBOW_R, "_ForeCutter")
        pieces[f'lowerarm_{sfx}'], pieces[f'upperarm_{sfx}'] = split(arm, fore, f'_P_lowerarm_{sfx}', f'_P_upperarm_{sfx}')
    legs, rest = split(rest, drum_or_below('_HipCutter', HIP_Y, HIP_Z, HIP_R), '_Legs', '_Rest')
    pieces['spine_02'], rest = split(rest, _above_minus_ball(CHEST_Z, CHEST, CHEST_R, '_ChestCutter'), '_P_spine_02', '_Rest')
    pieces['spine_01'], pieces['pelvis'] = split(rest, _above_minus_ball(WAIST_Z, WAIST, WAIST_R, '_WaistCutter'),
                                                 '_P_spine_01', '_P_pelvis')
    left = box('_LeftHalf', (0.0, -BIG, -BIG), (BIG, BIG, BIG))
    halves = dict(zip(('l', 'r'), split(legs, left, '_LegL', '_LegR')))
    for s, sfx in ((1.0, 'l'), (-1.0, 'r')):
        calf, pieces[f'thigh_{sfx}'] = split(halves[sfx], drum_or_below('_KneeCutter', KNEE_Y, KNEE_Z, KNEE_R),
                                             '_Calf', f'_P_thigh_{sfx}')
        foot = _below_minus_ball(ANKLE_Z, mirror(ANKLE, s), ANKLE_R, "_AnkleCutter")
        pieces[f'foot_{sfx}'], pieces[f'calf_{sfx}'] = split(calf, foot, f'_P_foot_{sfx}', f'_P_calf_{sfx}')
    for bone, obj in pieces.items():
        obj.name = obj.data.name = f'_{tag}_{bone}'
    rehome(pieces)
    for bone, obj in pieces.items():
        _report_piece(bone, obj)
    return pieces


def islands(obj):
    """Connected pieces of the mesh: [(face count, area, centre)], largest first."""
    found = tripo.islands(obj, gun_space=False)
    return [(len(i.faces), i.area, tuple((a + b) * 0.5 for a, b in zip(i.lo, i.hi))) for i in found]


def _report_piece(bone, obj):
    me = obj.data
    caps = sum(1 for p in me.polygons if me.materials[p.material_index].name == '_Cap') if me.materials else 0
    isl = islands(obj)
    extra = '; '.join(f'{n} faces {a * 1e4:.0f} cm2 at ({c[0]:+.2f},{c[1]:+.2f},{c[2]:+.2f})' for n, a, c in isl[1:6])
    log(f'piece {bone}: {len(me.polygons)} faces ({caps} cap), {len(isl)} islands{": " + extra if extra else ""}')


def _arm_bone(c):
    """The arm piece a loose bit at c belongs to (by height), or None when it isn't by an arm."""
    if abs(c[0]) < 0.30 or not 0.75 < c[2] < 1.8:
        return None
    sfx = 'l' if c[0] > 0.0 else 'r'
    part = 'hand' if c[2] < WRIST_Z else ('lowerarm' if c[2] < ELBOW_Z else 'upperarm')
    return f'{part}_{sfx}'


def _move_faces(src, faces, dst):
    """Moves src's faces (indices) into dst (None: drops them)."""
    if dst is not None:
        bit = src.copy()
        bit.data = src.data.copy()
        bpy.context.scene.collection.objects.link(bit)
        keep = set(faces)
        tripo._delete_faces(bit, [i for i in range(len(src.data.polygons)) if i not in keep])
        with bpy.context.temp_override(active_object=dst, object=dst, selected_objects=[dst, bit],
                                       selected_editable_objects=[dst, bit]):
            bpy.ops.object.join()
    tripo._delete_faces(src, faces)


def rehome(pieces, crumb=0.12):
    """Loose bits the cuts leave where the sleeves and fists touch the robe: slivers (under crumb of their piece's main
    body) go, wherever they are (moved with an arm they fly off it as shards); a big one by an arm joins it."""
    for bone, obj in pieces.items():
        kept = 1
        while True:
            # Islands are found again after each move: moving faces renumbers the rest.
            found = tripo.islands(obj, gun_space=False)
            if len(found) <= kept:
                break
            isl = found[kept]
            c = tuple((a + b) * 0.5 for a, b in zip(isl.lo, isl.hi))
            home = None if bone.startswith(('hand', 'lowerarm', 'upperarm')) else _arm_bone(c)
            if isl.area < found[0].area * crumb:
                home = None
                _move_faces(obj, isl.faces, None)
            elif home and home != bone:
                _move_faces(obj, isl.faces, pieces[home])
            else:
                kept += 1
                continue
            log(f'{len(isl.faces)} loose faces of {bone} at ({c[0]:+.2f}, {c[1]:+.2f}, {c[2]:+.2f}) '
                f'{"to " + home if home else "dropped"}')


# --- 3. Retopology ---

def _apply_voxel(obj, size):
    mod = obj.modifiers.new('voxel', 'REMESH')
    mod.mode = 'VOXEL'
    mod.voxel_size = size
    deps = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(obj.evaluated_get(deps))
    old = obj.data
    obj.modifiers.clear()
    obj.data = me
    bpy.data.meshes.remove(old)


def _clean_for_quadriflow(obj, dist=3e-4):
    """QuadriFlow refuses a mesh with any edge under 0.1 mm (a voxel pass or a cut leaves some): merged away."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=dist)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=dist)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data)
    bm.free()


def _materials_from(src, dst):
    """dst's faces take the slot (_Skin or _Cap) of the nearest face of src."""
    from mathutils.bvhtree import BVHTree
    sm = src.data
    tree = BVHTree.FromPolygons([v.co.copy() for v in sm.vertices], [list(p.vertices) for p in sm.polygons])
    caps = [sm.materials[p.material_index].name == '_Cap' for p in sm.polygons]
    dm = dst.data
    dm.materials.clear()
    dm.materials.append(bpy.data.materials['_Skin'])
    dm.materials.append(bpy.data.materials['_Cap'])
    for p in dm.polygons:
        index = tree.find_nearest(p.center)[2]
        p.material_index = 1 if index is not None and caps[index] else 0
        p.use_smooth = True


def _remesh(obj, want, voxel, factor, seed):
    """One try at obj remeshed to about want quads: a voxel pass (one clean solid: a cut through Tripo's shell, which
    crosses itself where parts touch, isn't always one), collapsed down to factor times the faces wanted (the voxel
    pass leaves slivers, and QuadriFlow refuses any edge under 0.1 mm), then QuadriFlow. None when it refused."""
    q = obj.copy()
    q.data = obj.data.copy()
    bpy.context.scene.collection.objects.link(q)
    _apply_voxel(q, voxel)
    dec = q.modifiers.new('dec', 'DECIMATE')
    dec.ratio = min(1.0, want * factor / max(len(q.data.polygons), 1))
    deps = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(q.evaluated_get(deps))
    old = q.data
    q.modifiers.clear()
    q.data = me
    bpy.data.meshes.remove(old)
    _clean_for_quadriflow(q, 1.5e-4)
    with bpy.context.temp_override(active_object=q, object=q, selected_objects=[q], selected_editable_objects=[q]):
        result = bpy.ops.object.quadriflow_remesh(target_faces=want, use_preserve_sharp=True,
                                                  use_preserve_boundary=False, use_mesh_symmetry=False,
                                                  smooth_normals=False, mode='FACES', seed=seed)
    if 'FINISHED' in result and want * 0.5 <= len(q.data.polygons) <= want * 2.0:
        return q
    _remove(q)
    return None


def _thin_caps(obj, angle=22.0):
    """Caps are mostly hidden (the joints' inner faces, the balls): their quads merged by a limited dissolve inside them
    (never across their rim, so seams keep their line) and triangulated, so the skin keeps the triangles."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    cap = obj.data.materials.find('_Cap')
    faces = [f for f in bm.faces if f.material_index == cap]
    inner = [e for e in {e for f in faces for e in f.edges} if all(g.material_index == cap for g in e.link_faces)]
    verts = [v for v in {v for e in inner for v in e.verts} if all(f.material_index == cap for f in v.link_faces)]
    bmesh.ops.dissolve_limit(bm, angle_limit=math.radians(angle), use_dissolve_boundaries=False, verts=verts,
                             edges=inner, delimit={'MATERIAL'})
    big = [f for f in bm.faces if f.material_index == cap and len(f.verts) > 4]
    if big:
        bmesh.ops.triangulate(bm, faces=big, quad_method='BEAUTY', ngon_method='BEAUTY')
    bm.to_mesh(obj.data)
    bm.free()


TRIES = ((VOXEL, 12.0), (VOXEL, 7.0), (VOXEL * 1.4, 10.0), (VOXEL * 2.0, 6.0), (VOXEL * 1.2, 16.0))


def retopo(source, quads=QUADS, seed=3):
    """The whole statue remeshed to even quads before it is cut (one remesh, so the cuts' seams are exact on both
    sides): a copy of source, which stays as the bake's source. The voxel pass fuses what Tripo's shell let cross
    itself (the cord on the robe, fingers, the sleeves on the flanks), so the cuts' booleans read a clean solid."""
    for k, (voxel, factor) in enumerate(TRIES):
        q = _remesh(source, quads, voxel, factor, seed)
        if q:
            break
    else:
        raise RuntimeError('QuadriFlow refused the statue every way')
    q.name = q.data.name = '_Low'
    # Triangulated for the cuts (Exact misreads bent quads now and then: open seams); tidy_caps pairs them back.
    bm = bmesh.new()
    bm.from_mesh(q.data)
    bmesh.ops.triangulate(bm, faces=list(bm.faces), quad_method='FIXED')
    bm.to_mesh(q.data)
    bm.free()
    q.data.materials.clear()
    q.data.materials.append(bpy.data.materials['_Skin'])
    for p in q.data.polygons:
        p.use_smooth = True
        p.material_index = 0
    log(f'retopo: {len(q.data.polygons)} quads (asked {quads}; try {k + 1})')
    return q


def tidy_caps(pieces):
    """A cut's caps carry the cutters' facets sliced to slivers: merged flat inside the caps and triangulated."""
    for obj in pieces.values():
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=2e-4)
        bm.to_mesh(obj.data)
        bm.free()
        _thin_caps(obj, angle=25.0)
        # The skin's triangles paired back into QuadriFlow's quads (each quad was split along one diagonal).
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        skin = obj.data.materials.find('_Skin')
        tris = [f for f in bm.faces if f.material_index == skin and len(f.verts) == 3]
        bmesh.ops.join_triangles(bm, faces=tris, angle_face_threshold=math.radians(30.0),
                                 angle_shape_threshold=math.radians(60.0), cmp_seam=False, cmp_sharp=False,
                                 cmp_uvs=False, cmp_vcols=False, cmp_materials=True)
        bm.to_mesh(obj.data)
        bm.free()


# --- 4. The dormant pose ---

BOW = math.radians(12.0)                 # the hood bowed over the hands: what the waking HeadLift (12) undoes
# Off: the fists stay at the sides as the user modelled them. Brought together before the belt (on), the arms must swing
# forward and in a long way, and Tripo's sleeves are fused to the robe down the flanks: the cut faces between them (and
# thin sheets of sleeve or mantle on either side) stand bare at rest. The waking still lifts the bowed head.
CLASP_HANDS = False
CLASP = {1.0: Vector((0.085, -0.53, 1.12)), -1.0: Vector((-0.08, -0.50, 1.07))}   # the fists' centres, left over right
FIST_TWIST = {1.0: math.radians(-55.0), -1.0: math.radians(55.0)}                  # knuckles turned forward, palms in
ELBOW_POLE = Vector((0.9, 0.3, -0.35))     # the elbows bend out and back (x mirrored)


def _in_then_forward(way, wanted, s):
    """The turn taking way onto wanted as a tip in round the forward (y) axis, then a swing round the lateral (x) one."""
    # The tip sets x: way.x cos a + way.z sin a (Blender: a turn round +y by a takes z into x).
    r = math.hypot(way.x, way.z)
    a = math.asin(max(-1.0, min(1.0, wanted.x / r))) - math.atan2(way.x, way.z)
    tip = Quaternion((0.0, 1.0, 0.0), a)
    tipped = tip @ way
    yz0 = math.atan2(tipped.z, tipped.y)
    yz1 = math.atan2(wanted.z, wanted.y)
    return Quaternion((1.0, 0.0, 0.0), yz1 - yz0) @ tip


def two_bone(root, target, upper, lower, pole):
    """The elbow between root and target keeping both lengths, bent toward pole."""
    to = target - root
    d = min(max(to.length, abs(upper - lower) + 1e-3), (upper + lower) * 0.999)
    way = to.normalized()
    along = (upper * upper - lower * lower + d * d) / (2.0 * d)
    height = math.sqrt(max(upper * upper - along * along, 0.0))
    bend = (pole - way * pole.dot(way)).normalized()
    return root + way * along + bend * height


def _about(pivot, rot, moved_pivot=None):
    """v -> moved_pivot + rot (v - pivot), as a 4x4."""
    m = rot.to_matrix().to_4x4()
    return Matrix.Translation(moved_pivot if moved_pivot is not None else pivot) @ m @ Matrix.Translation(-pivot)


def rest_pose():
    """{bone: 4x4} taking each piece from Tripo's stance into the dormant statue, and the posed joints {name: point}."""
    xf = {b: Matrix.Identity(4) for b in BONES}
    joints = {}
    xf['head'] = _about(NECK, Quaternion((1.0, 0.0, 0.0), BOW))
    for s, sfx in ((1.0, 'l'), (-1.0, 'r')):
        sh, el, wr, fi = (mirror(p, s) for p in (SHOULDER, ELBOW, WRIST, FIST))
        if not CLASP_HANDS:
            joints.update({f'shoulder_{sfx}': sh, f'elbow_{sfx}': el, f'wrist_{sfx}': wr, f'fist_{sfx}': fi})
            continue
        # In round the forward axis, then forward round the lateral one: no turn round the arm itself, which would swing
        # the back of its slab out from the flank like a wing.
        el2 = two_bone(sh, CLASP[s], (el - sh).length, (fi - el).length, mirror(ELBOW_POLE, s))
        upper = _in_then_forward(el - sh, el2 - sh, s)
        xf[f'upperarm_{sfx}'] = _about(sh, upper)
        # The forearm keeps the upper arm's twist: only the bend between them is added.
        rest_way = upper @ (fi - el).normalized()
        fore = rest_way.rotation_difference((CLASP[s] - el2).normalized()) @ upper
        xf[f'lowerarm_{sfx}'] = _about(el, fore, el2)
        wr2 = xf[f'lowerarm_{sfx}'] @ wr
        # The fist turns round the wrist's own upright (Tripo's stance), so its seam with the cuff (level there) slides.
        hand = fore @ Quaternion((0.0, 0.0, 1.0), FIST_TWIST[s])
        xf[f'hand_{sfx}'] = _about(wr, hand, wr2)
        joints.update({f'shoulder_{sfx}': sh, f'elbow_{sfx}': el2, f'wrist_{sfx}': wr2,
                       f'fist_{sfx}': xf[f'hand_{sfx}'] @ fi})
    joints['neck'] = NECK.copy()
    return xf, joints


def pose(objects, xf):
    for bone, obj in objects.items():
        obj.data.transform(xf[bone])
        obj.data.update()
