"""The five loot chest tiers (the user, 2026-10-27: "the better the chest the better the loot"), from the user's Tripo
models: Art/Concepts/Chests/Tier<N>/Tier<N>Chest.glb (each modeled CLOSED, in one piece, from the concept views beside
it; open_ref.png shows where the lid parts). A scripted model file (Art/README.md); Tools/models.ps1 exports it into
/Game/Art/Loot, the names Loot/ChestKinds.cpp loads:

  ChestTier1   olive drab case, strap latches, green latch light         60 cm across
  ChestTier2   orange hard case, grey straps, wire side handles, amber bar  72 cm
  ChestTier3   grey case, black straps, blue ring lock                    85 cm
  ChestTier4   dark purple carved stone, gold trim, violet orb            97 cm
  ChestTier5   red, silver and gold crowned chest, gold crest and gem    110 cm

Each tier exports its body (SM_ChestTier<N>) and lid (SM_ChestTier<N>_Lid), and the pieces the opening animates (the
user's "escalating show", 2026-10-28): tiers 1-2 two latches (_Latch_L, _Latch_R) that pop, tier 3 its ring lock
(_Lock) that spins and turns green, tier 4 the orb and gold trim as their own glow slots (the trim lights in a sweep),
tier 5 its crest (_Crest) that unfolds before the lid. Pivots and sockets (Blender: front -Y, up +Z; Unreal: front +X):
  body  SOCKET_Lid      the hinge: the back of the seam. The lid's origin; it pitches up about it (positive pitch).
        SOCKET_Loot     the floor of the hollow inside, 10 cm up.   SOCKET_Interact  the front, at the seam.
        SOCKET_Latch_L/_R (tiers 1-2) each latch's origin: its lower pivot behind it; a negative pitch swings its top
                        out (Latch_L is on the left as you face the chest).
        SOCKET_Light    (tiers 1-2) the front light's centre.
        SOCKET_Lock     (tier 3) the ring lock's origin and centre; it spins about the forward axis (roll).
        SOCKET_Orb      (tier 4, on the body or the lid, wherever the orb is) the orb's centre.
  lid   SOCKET_Crest    (tier 5) the crest's origin: its top hinge; a positive pitch swings its lower edge out and up.
  crest SOCKET_Gem      (tier 5) the gem's centre.
Glow slots (emissive, M_StylizedSurface: a dynamic instance can change Glow and the colour): ChestTier<N>Glow (the
tier's lights; on tier 3 the lock's ring is ChestTier3LockGlow, apart, so it alone can go green), ChestTier4TrimGlow (a gold
ribbon framing tier 4's front inside its gold edging, dim gold at rest), ChestTier<N>InnerGlow (tiers 3-5: a strip round the inside under the rim, to light as the
lid lifts). Textured slots: ChestTier<N> (Tripo's maps on the Gun master: its worn edges, grime and scratches),
ChestRimIron (the body's rim, bevelled into the hollow, and the pieces' seats), ChestLiningDark/Violet/Crimson (the
quilted cloth inside: the script-made set Art/Textures/ChestLining, tinted per tier; a padded cushion on the hollow's
floor and a padded panel in the lid's recess, whose walls are its lip; tier 5's lid is too thin for a recess and stays a
flat quilted cap). A piece's seat: the hole it leaves closed flat 2 mm into the case, its rim straightened onto the cut's
convex outline, the piece's back the same shape.

What each build does to the Tripo mesh: holes closed, flat panels made flat (vertices within a few mm of a panel's plane
snapped onto it: straighter edges, crisper silhouettes), sized (front -Y, floor at z 0), the animated pieces cut out
(the hole behind each closed), decimated to the tier's triangle budget, split at the seam (found as the groove nearest
the concept's seam height), the body hollowed and the lid's underside recessed (exact booleans), the lights overlaid as
crisp emissive plates, rings, an orb and a gem over Tripo's painted ones (tier 3's lock gets a steel face with turned
grooves and a keyhole that lights with its ring). Maps: Art/Textures/TripoChestTier<N>/ (1024 px
tiers 1-2, 2048 tiers 3-5), the colour map sharpened (unsharp mask) and graded a little toward the game's look. Small
props: no Nanite, LODs 50% and 20%. Collision: a box round each body and each lid; the pieces none.

    Tools\\artrun.ps1 -Script Art\\Models\\Loot\\ChestTiers.py -Preview
renders Saved/ChestFit/: Tier<N>.png (front, three-quarter, open: latches popped, crest unfolded) and Tiers.png (the
five side by side, closed and open). LOOTER_CHEST_TIERS=1,3 builds only those tiers.
"""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_model as lm
import looter_props as lp
import looter_textures as lt
import looter_tripo as tripo

GLB = 'Art/Concepts/Chests/Tier{0}/Tier{0}Chest.glb'
OUT = os.path.join(lt.REPO, 'Saved', 'ChestFit')
LODS = '50,20'


# --- Colour tests for the painted lights (sRGB 0..1) ---

def is_green(c):
    r, g, b = c
    return g > 0.45 and g > 1.3 * r and g > 1.05 * b


def is_amber(c):
    r, g, b = c
    return r > 0.8 and g > 0.72 and g > 0.8 * r and b < 0.8 * g


def is_blue(c):
    r, g, b = c
    return b > 0.5 and b > 1.35 * r and b > 1.1 * g


def is_violet(c):
    r, g, b = c
    return b > 0.55 and r > 0.4 and g < 0.6 * b and r + b > 1.1


def is_gem(c):
    r, g, b = c
    return r > 0.85 and g > 0.7 and b < 0.75 * g


def is_ember(c):
    r, g, b = c
    return r > 0.85 and 0.35 < g < 0.75 and b < 0.45


# --- The tiers. Places are in the GLB's own units (each file is 100 across, front -Y, floor 0): x across, z up. ---
# seam: the concept's seam height (the groove nearest it is used). latches/crest: (x0, x1, z0, z1) boxes of the pieces
# to cut out, margin: how far in front of the panel behind them the cut runs. lock: (x, z, radius).
# keep_body: |x| past which faces stay on the body above the seam (tier 2's side handles, which cross it).
# planar: how far (units) a vertex may move onto its panel's plane. depth: the depth's stretch (tier 5's sheet drew it
# slimmer than a chest holds). grade: (saturation, contrast) of the colour map.
TIERS = {
    1: dict(width=0.60, seam=35.5, maps=1024, tris=9000, glow=0x4dff6a, strength=2.5, tests=[is_green], plates=True,
            latches=[(-34.5, -22.2, 19.0, 39.0), (22.2, 34.5, 19.0, 39.0)], margin=0.7, planar=0.7, spare_lights=True,
            lining=('ChestLiningDark', 0x3a3734), grade=(1.0, 1.08)),
    2: dict(width=0.72, seam=28.3, maps=1024, tris=10000, glow=0xffbf3a, strength=3.0, tests=[is_amber], plates=True,
            latches=[(-32.0, -21.4, 19.6, 34.3), (21.4, 32.8, 19.6, 34.3)], margin=0.7, planar=0.7, keep_body=46.0,
            lining=('ChestLiningDark', 0x3a3734), grade=(0.9, 1.08)),
    3: dict(width=0.85, seam=26.6, maps=2048, tris=11500, glow=0x3d7bff, strength=4.0, tests=[is_blue], plates=True,
            lock=(0.0, 24.2, 7.8), margin=0.8, planar=0.7, inner=True,
            lining=('ChestLiningDark', 0x3a3734), grade=(1.0, 1.08)),
    4: dict(width=0.97, seam=28.3, maps=2048, tris=12500, glow=0xb04dff, strength=5.5, tests=[is_violet], orb=(0.0, 30.0),
            trim=True, planar=0.4, inner=True, lining=('ChestLiningViolet', 0x5a3482), grade=(1.0, 1.1)),
    5: dict(width=1.10, seam=29.6, maps=2048, tris=15000, glow=0xffa52e, strength=7.0, tests=[is_gem, is_ember],
            crest=(-6.0, 6.0, 21.5, 37.0), margin=0.6, seam_fixed=True, gem=(0.0, 30.0), planar=0.5, depth=1.45, wall=1.2, inner=True,
            lining=('ChestLiningCrimson', 0x8c1426), grade=(0.92, 1.08)),
}
OPEN_ANGLE = {1: 112.0, 2: 112.0, 3: 102.0, 4: 102.0, 5: 102.0}   # the stand-ins' (ChestKinds.cpp), for the previews

RIM = lt.material('MetalWorn', name='ChestRimIron', tint=0x3a3835)


def log(text):
    lt._log(f'chest tiers: {text}')


# --- Mesh helpers ---

def mesh_arrays(me):
    n = len(me.polygons)
    normals, centers, areas = np.empty(n * 3), np.empty(n * 3), np.empty(n)
    me.polygons.foreach_get('normal', normals)
    me.polygons.foreach_get('center', centers)
    me.polygons.foreach_get('area', areas)
    return normals.reshape(-1, 3), centers.reshape(-1, 3), areas


def tris_of(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def tree_of(obj):
    me = obj.data
    return BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])


def apply_modifier(obj, mod):
    with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj],
                                   selected_editable_objects=[obj]):
        bpy.ops.object.modifier_apply(modifier=mod.name)


def fill_holes(obj, material_index=None, plane_z=None, plane_material=None):
    """Closes every hole in obj with flat faces. A fill takes one UV (a corner of the faces round it: one flat colour
    from the map) and its neighbours' material, or plane_material where it lies on z = plane_z (the seam's cap).
    Returns how many faces it added."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    edges = [e for e in bm.edges if e.is_boundary]
    if not edges:
        bm.free()
        return 0
    new = list(bmesh.ops.holes_fill(bm, edges=edges, sides=0)['faces'])
    # holes_fill leaves a hole whose rim isn't flat and simple (where a latch was cut out): a fan from its middle.
    left = {e for e in bm.edges if e.is_boundary}
    while left:
        start = left.pop()
        loop = [start.verts[0], start.verts[1]]
        while True:
            nxt = next((e for e in loop[-1].link_edges if e in left), None)
            if nxt is None:
                break
            left.discard(nxt)
            v = nxt.other_vert(loop[-1])
            if v is loop[0]:
                break
            loop.append(v)
        if len(loop) < 3:
            continue
        zs = [v.co.z for v in loop]
        if max(zs) - min(zs) < 1e-4:
            # Flat (the seam's cap): scanfill, which keeps a concave outline (a fan from the middle laid dark
            # wedges across the light window's notch, 2026-10-28).
            ring_edges = [bm.edges.get((a, b)) for a, b in zip(loop, loop[1:] + loop[:1])]
            made = bmesh.ops.triangle_fill(bm, use_beauty=True, use_dissolve=False,
                                           edges=[e for e in ring_edges if e is not None], normal=(0.0, 0.0, 1.0))
            if made['geom']:
                new += [f for f in made['geom'] if isinstance(f, bmesh.types.BMFace)]
                continue
        mid = bm.verts.new(sum((v.co for v in loop), Vector()) / len(loop))
        for a, b in zip(loop, loop[1:] + loop[:1]):
            try:
                new.append(bm.faces.new((a, b, mid)))
            except ValueError:
                pass
    uv = bm.loops.layers.uv.active
    fresh = set(new)
    for f in new:
        near = next((l for v in f.verts for l in v.link_loops if l.face not in fresh), None)
        for loop in f.loops:
            if uv is not None and near is not None:
                loop[uv].uv = near[uv].uv
        if plane_z is not None and plane_material is not None and all(abs(v.co.z - plane_z) < 1e-4 for v in f.verts):
            f.material_index = plane_material
        elif material_index is not None:
            f.material_index = material_index
        elif near is not None:
            f.material_index = near.face.material_index
    tri = bmesh.ops.triangulate(bm, faces=new)['faces']
    count = len(tri)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])   # closed again: every face outward
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return count


def outer_shell(obj):
    """Drops the inside walls of a model Tripo made hollow (tier 3: a case with walls, its inside surface joined to the
    outside through a few holes): a face none of whose rays (its normal and 24 round it) gets out is inside. Its holes
    are closed after. Returns how many faces went (0: the model was solid, as it should be)."""
    tree = tree_of(obj)
    me = obj.data
    normals, centers, areas = mesh_arrays(me)
    c = Vector(centers.mean(0))
    probes = [tree.ray_cast(c, Vector(d)) for d in ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0))]
    if not any(h[0] is not None and h[1].dot(Vector(d)) < 0.0
               for h, d in zip(probes, ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0)))):
        return 0
    rng = np.random.default_rng(7)
    dirs = rng.normal(size=(24, 3))
    dirs /= np.linalg.norm(dirs, axis=1)[:, None]
    inner = []
    for i, (n, p) in enumerate(zip(normals, centers)):
        n = Vector(n)
        start = Vector(p) + n * 1e-4
        out = tree.ray_cast(start, n)[0] is None
        if not out:
            for d in dirs:
                d = Vector(d)
                if d.dot(n) < 0.05:
                    d = d - n * (2.0 * d.dot(n))
                if tree.ray_cast(start, d)[0] is None:
                    out = True
                    break
        if not out:
            inner.append(i)
    tripo._delete_faces(obj, inner)
    # Shreds left where the walls met: anything but the biggest piece goes.
    groups = components(obj, list(range(len(obj.data.polygons))))
    if len(groups) > 1:
        tripo._delete_faces(obj, [f for g, a in groups[1:] for f in g])
    fill_holes(obj)
    return len(inner)


def planarize(obj, tol, bin_w):
    """Flat panels made flat: for each of the six axis directions, the planes the faces facing it lie on (peaks of
    their area along that axis), and every vertex of those faces within tol of a plane snapped onto it. Edges between
    panels straighten and the lumps Tripo leaves on flat plates go."""
    me = obj.data
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    normals, centers, areas = mesh_arrays(me)
    starts, totals = np.empty(len(me.polygons), int), np.empty(len(me.polygons), int)
    me.polygons.foreach_get('loop_start', starts)
    me.polygons.foreach_get('loop_total', totals)
    loop_verts = np.empty(len(me.loops), int)
    me.loops.foreach_get('vertex_index', loop_verts)
    face_of_loop = np.repeat(np.arange(len(me.polygons)), totals)
    moved = 0
    for axis in range(3):
        for sign in (-1.0, 1.0):
            sel = normals[:, axis] * sign > 0.94
            if sel.sum() < 10:
                continue
            vals, w = centers[sel, axis], areas[sel]
            lo = vals.min()
            nb = int((vals.max() - lo) / bin_w) + 3
            hist = np.bincount(((vals - lo) / bin_w).astype(int), weights=w, minlength=nb)
            smooth = np.convolve(hist, [0.25, 0.5, 0.25], 'same')
            total = w.sum()
            peaks = []
            for i in range(nb):
                left = smooth[i - 1] if i else 0.0
                right = smooth[i + 1] if i + 1 < nb else 0.0
                if smooth[i] >= left and smooth[i] >= right and smooth[i] > 0.04 * total:
                    c = lo + (i + 0.5) * bin_w
                    m = np.abs(vals - c) < 1.5 * bin_w
                    peaks.append(np.average(vals[m], weights=w[m]))
            if not peaks:
                continue
            pos = np.array(peaks)
            # Only vertices inside a panel (every face round them faces this way): a latch's, a strap's or a rim's own
            # small faces, snapped onto the big panel's plane behind them, folded flat (2026-10-28 first try).
            inside = np.bincount(loop_verts, weights=sel[face_of_loop].astype(float), minlength=len(co))
            every = np.bincount(loop_verts, minlength=len(co))
            verts = np.nonzero((inside == every) & (every > 0))[0]
            d = co[verts, axis][:, None] - pos[None, :]
            k = np.abs(d).argmin(1)
            dist = np.abs(d[np.arange(len(verts)), k])
            ok = dist < tol
            co[verts[ok], axis] = pos[k[ok]]
            moved += int(ok.sum())
    me.vertices.foreach_set('co', co.ravel())
    me.update()
    return moved


def _collapse(obj, target, keep=()):
    """keep: boxes (lo, hi) whose vertices the collapse spares (their edges cost more). Tier 1 only: round its light
    window a collapse left a spike; on tier 2 the spared front pushed the collapse into the case's corners and bent
    the whole box."""
    group = None
    if keep:
        group = obj.vertex_groups.new(name='_decimate')
        spared = [i for i, v in enumerate(obj.data.vertices)
                  if any(all(lo[k] <= v.co[k] <= hi[k] for k in range(3)) for lo, hi in keep)]
        group.add(list(range(len(obj.data.vertices))), 1.0, 'REPLACE')
        if spared:
            group.add(spared, 0.0, 'REPLACE')
    mod = obj.modifiers.new('_decimate', 'DECIMATE')
    mod.decimate_type = 'COLLAPSE'
    mod.ratio = target / tris_of(obj)
    mod.use_collapse_triangulate = True
    if group is not None:
        mod.vertex_group = group.name
    apply_modifier(obj, mod)
    left = obj.vertex_groups.get('_decimate')
    if left is not None:
        obj.vertex_groups.remove(left)
    return tris_of(obj)


def decimate(obj, target, keep=()):
    """Edge collapses down to target triangles. (Dissolving the flat panels first, even inside a UV island, smeared the
    map into spikes: Tripo's atlas isn't linear across a panel; collapse keeps it.)"""
    if target >= tris_of(obj):
        return tris_of(obj)
    return _collapse(obj, target, keep)


def _onto_hull(x, z, hull):
    """The nearest point to (x, z) on the closed polygon hull's edges."""
    best, at = None, (x, z)
    for (x0, z0), (x1, z1) in zip(hull, hull[1:] + hull[:1]):
        dx, dz = x1 - x0, z1 - z0
        t = max(0.0, min(1.0, ((x - x0) * dx + (z - z0) * dz) / max(dx * dx + dz * dz, 1e-12)))
        px, pz = x0 + t * dx, z0 + t * dz
        d = (px - x) ** 2 + (pz - z) ** 2
        if best is None or d < best:
            best, at = d, (px, pz)
    return at


def flatten_rim(obj, y, hull=None, reach=0.012):
    """The cut's rim (its open edge) moved onto the plane y = y (front-back) and, given hull (the cut's outline's
    convex hull, (x, z)), out onto that clean outline: the hole then closes as a flat plate with straight edges (a
    piece's seat), the piece's back the same shape."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for v in bm.verts:
        if any(e.is_boundary for e in v.link_edges):
            to = Vector((v.co.x, y, v.co.z))
            if hull and len(hull) >= 3:
                to.x, to.z = _onto_hull(v.co.x, v.co.z, hull)
            # Only the rim's own wobble: a far move (a stray piece of the cut, a deep strap) would drag the case's
            # panels with it (tier 2's sides once tapered).
            if (to - v.co).length < reach:
                v.co = to
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def extract(obj, faces, name, fill=None, close=True, flat=None):
    """Moves faces out of obj into a new object called name; both are closed again behind the cut (in the material
    fill, when given: a piece's mount)."""
    keep = set(faces)
    piece = obj.copy()
    piece.data = obj.data.copy()
    for k in list(piece.keys()):
        del piece[k]
    piece.name = piece.data.name = name
    bpy.context.scene.collection.objects.link(piece)
    tripo._delete_faces(piece, [i for i in range(len(obj.data.polygons)) if i not in keep])
    tripo._delete_faces(obj, list(keep))
    if flat is not None:
        # The piece's seat: the case's hole made a flat plate 2 mm into the case at y = flat (the panel's depth), its
        # rim straightened onto the cut's convex outline; the piece's back the same outline 1.6 mm in front of it.
        hull = hull_2d([(q.x, q.z) for q in boundary_points(piece)])
        flatten_rim(obj, flat + 0.002, hull)
        flatten_rim(piece, flat + 0.0004, hull)
    if not close:
        return piece
    index = None
    if fill is not None:
        for o in (piece, obj):
            if o.data.materials.find(fill.name) < 0:
                o.data.materials.append(fill)
    fill_holes(piece, material_index=piece.data.materials.find(fill.name) if fill else None)
    fill_holes(obj, material_index=obj.data.materials.find(fill.name) if fill else None)
    return piece


def components(obj, faces):
    """The edge-connected groups of faces, largest (by area) first: [(faces, area)]."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    wanted, seen, found = set(faces), set(), []
    for f0 in faces:
        if f0 in seen:
            continue
        stack, group, area = [f0], [], 0.0
        seen.add(f0)
        while stack:
            f = bm.faces[stack.pop()]
            group.append(f.index)
            area += f.calc_area()
            for e in f.edges:
                for g in e.link_faces:
                    if g.index in wanted and g.index not in seen:
                        seen.add(g.index)
                        stack.append(g.index)
        found.append((group, area))
    bm.free()
    found.sort(key=lambda t: -t[1])
    return found


class Patch:
    """A group of painted faces: bounds, centroid, mean normal, area (meters)."""

    def __init__(self, obj, faces):
        me = obj.data
        pts = np.array([me.vertices[v].co[:] for f in faces for v in me.polygons[f].vertices])
        self.faces = faces
        self.lo, self.hi = pts.min(0), pts.max(0)
        a = np.array([me.polygons[f].area for f in faces])
        c = np.array([me.polygons[f].center[:] for f in faces])
        n = np.array([me.polygons[f].normal[:] for f in faces])
        self.area = a.sum()
        self.center = (c * a[:, None]).sum(0) / self.area
        nn = (n * a[:, None]).sum(0)
        self.normal = nn / max(np.linalg.norm(nn), 1e-9)


def protruding(obj, inside, margin, around=None, share=None):
    """The faces whose centres pass inside((x, z)) and stand at least margin (m) in front of the panel behind them (the
    90th percentile, by area, of the front-facing faces there): a latch, a lock or a crest proud of its case."""
    normals, centers, areas = mesh_arrays(obj.data)
    region = np.array([inside(c[0], c[2]) for c in centers])
    front = region & (normals[:, 1] < -0.7)
    pick = 0.9
    if around is not None:   # the panel round it (a lock's case, a crest's plates): its middle depth by default
        front = ~region & np.array([around(c[0], c[2]) for c in centers]) & (normals[:, 1] < -0.7)
        pick = 0.5
    pick = share if share is not None else pick
    if not front.any():
        raise RuntimeError(f'{obj.name}: no front faces in a piece\'s box')
    ys, w = centers[front, 1], areas[front]
    order = np.argsort(ys)
    cum = np.cumsum(w[order]) / w.sum()
    backing = ys[order][np.searchsorted(cum, pick)]
    picked = [i for i in np.nonzero(region)[0] if centers[i, 1] < backing - margin]
    groups = components(obj, picked)
    if not groups:
        raise RuntimeError(f'{obj.name}: nothing stands {margin * 100:.1f} cm proud in a piece\'s box')
    big = groups[0][1]
    return [f for g, a in groups if a > 0.08 * big for f in g], backing


def find_seam(obj, guess, span):
    """The lid's seam: the horizontal groove nearest guess (z, m): where the chest is thinnest front to back and side to
    side against the heights just above and below it."""
    tree = tree_of(obj)
    co = np.array([v.co[:] for v in obj.data.vertices])
    lo, hi = co.min(0), co.max(0)

    def thickness(z):
        fb, ss = [], []
        for x in np.linspace(lo[0] * 0.7, hi[0] * 0.7, 17):
            a = tree.ray_cast(Vector((x, lo[1] - 1.0, z)), Vector((0.0, 1.0, 0.0)))[0]
            b = tree.ray_cast(Vector((x, hi[1] + 1.0, z)), Vector((0.0, -1.0, 0.0)))[0]
            if a is not None and b is not None:
                fb.append(b.y - a.y)
        for y in np.linspace(lo[1] * 0.6, hi[1] * 0.6, 7):
            a = tree.ray_cast(Vector((lo[0] - 1.0, y, z)), Vector((1.0, 0.0, 0.0)))[0]
            b = tree.ray_cast(Vector((hi[0] + 1.0, y, z)), Vector((-1.0, 0.0, 0.0)))[0]
            if a is not None and b is not None:
                ss.append(b.x - a.x)
        return (np.median(fb) if fb else 0.0) + (np.median(ss) if ss else 0.0)

    d = 0.01
    zs = np.arange(guess - span, guess + span, 0.001)
    t = {round(z, 4): thickness(z) for z in np.arange(guess - span - d, guess + span + d + 0.0005, 0.001)}
    get = lambda z: t[min(t, key=lambda k: abs(k - z))]
    scores = [(get(z) - 0.5 * (get(z - d) + get(z + d)), z) for z in zs]
    best, z = min(scores)
    if best > -0.0015:   # no groove worth the name: the concept's height
        log(f'seam: no clear groove near {guess:.3f} (best {best * 100:.2f} cm at {z:.3f}); kept the concept\'s')
        return guess
    log(f'seam at z {z:.3f} m (concept {guess:.3f}; groove {-best * 100:.2f} cm deep)')
    return float(z)


def fit_cavity(obj, z0, z1, wall, shrink=0.004):
    """The largest box (half sizes ax, ay round the part's middle) that stays at least wall inside the part between
    heights z0 and z1: rays out of its sides must leave through the part's surface no nearer than wall."""
    tree = tree_of(obj)
    co = np.array([v.co[:] for v in obj.data.vertices])
    lo, hi = co.min(0), co.max(0)
    cx, cy = (lo[0] + hi[0]) * 0.5, (lo[1] + hi[1]) * 0.5
    ax, ay = (hi[0] - lo[0]) * 0.5 - wall, (hi[1] - lo[1]) * 0.5 - wall

    def clear(p, d):
        hit, normal, _, dist = tree.ray_cast(Vector(p), Vector(d))
        return hit is not None and dist >= wall   # (a stray inside-out face mustn't stop it: the box grows from the middle)

    for _ in range(200):
        bad_x = bad_y = False
        for z in np.linspace(z0, z1, 5):
            for s in (-1.0, 1.0):
                for y in np.linspace(-ay * 0.85, ay * 0.85, 9):
                    if not clear((cx + s * ax, cy + y, z), (s, 0.0, 0.0)):
                        bad_x = True
                for x in np.linspace(-ax * 0.85, ax * 0.85, 13):
                    if not clear((cx + x, cy + s * ay, z), (0.0, s, 0.0)):
                        bad_y = True
                for t in (-1.0, 1.0):   # the corners, out along the diagonal
                    if not clear((cx + s * ax, cy + t * ay, z), (s * 0.7071, t * 0.7071, 0.0)):
                        bad_x = bad_y = True
        if not (bad_x or bad_y):
            return cx, cy, ax, ay
        ax -= shrink if bad_x else 0.0
        ay -= shrink if bad_y else 0.0
        if ax < 0.03 or ay < 0.03:
            break
    raise RuntimeError(f'{obj.name}: no room for a hollow between z {z0:.3f} and {z1:.3f}')


def surface_along(obj, points, d):
    """Where rays from points along d leave the part (z of each hit, for the floor and the lid's top)."""
    tree = tree_of(obj)
    hits = [tree.ray_cast(Vector(p), Vector(d))[0] for p in points]
    return [h.z for h in hits if h is not None]


def carve(obj, size, center, mat):
    """Cuts a box out of obj (an exact boolean); the new faces wear mat."""
    cutter = lp.block(size, center)
    cutter.name = '_cutter'
    if obj.data.materials.find(mat.name) < 0:
        obj.data.materials.append(mat)
    # The cutter wears obj's slots, its faces mat's index: the boolean's new faces keep that index.
    for m in obj.data.materials:
        cutter.data.materials.append(m)
    index = obj.data.materials.find(mat.name)
    for p in cutter.data.polygons:
        p.material_index = index
    mod = obj.modifiers.new('_carve', 'BOOLEAN')
    mod.operation = 'DIFFERENCE'
    mod.solver = 'EXACT'
    mod.object = cutter
    mod.material_mode = 'INDEX'
    apply_modifier(obj, mod)
    bpy.data.objects.remove(cutter)
    lo = Vector(center) - Vector(size) * 0.5 - Vector((1e-4,) * 3)
    hi = Vector(center) + Vector(size) * 0.5 + Vector((1e-4,) * 3)
    # Belt and braces: every face on the cutter's walls, inside it, is lining.
    for p in obj.data.polygons:
        c = p.center
        if all(lo[i] <= c[i] <= hi[i] for i in range(3)) and any(
                min(abs(c[i] - lo[i]), abs(c[i] - hi[i])) < 3e-4 and abs(p.normal[i]) > 0.98 for i in range(3)):
            p.material_index = index


def set_origin(obj, point):
    """The mesh stays where it is; its origin moves to point."""
    obj.data.transform(Matrix.Translation(-Vector(point)))
    obj.location = Vector(point)


# --- The lining, the cushions, the rims ---

LINING_SET = 'ChestLining'
LINING_DENSITY = 4096   # the box mapping's px per meter against the importer's 1024: a 25 cm tile, 4 per meter


def lining_set(size=512):
    """The chests' lining (script-made): grey quilted, padded cloth that a material's tint colours, diamond quilting
    about 6 cm across (4 per tile), puffed between stitched seams, a fine weave and a soft nap. Writes
    Art/Textures/ChestLining/T_ChestLining_BC, _N (DirectX), _ORM."""
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32) / size
    n = 4
    a, b = (xx + yy) * n, (xx - yy) * n
    d = np.minimum(np.abs(a - np.round(a)), np.abs(b - np.round(b)))   # 0 on a seam, 0.5 in a diamond's middle
    puff = np.clip(d / 0.5, 0.0, 1.0)
    rng = np.random.default_rng(11)
    nap = lt.blur(rng.random((size, size)).astype(np.float32), 1.5)
    nap = (nap - nap.mean()) / (nap.std() + 1e-6)
    weave = np.sin(xx * size * np.pi * 0.5) * np.sin(yy * size * np.pi * 0.5)
    seam = np.exp(-(d / 0.035) ** 2)
    stitch = seam * (np.sin((a + b) * np.pi * 12.0) > 0.0)   # dashes along the seams
    height = np.sqrt(puff) + 0.02 * weave + 0.03 * nap - 0.15 * stitch
    bc = 0.74 + 0.05 * (puff - 0.5) + 0.025 * nap - 0.22 * seam - 0.08 * stitch
    px = 0.25 / size
    h = height * 0.004
    gx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) / (2 * px)
    gr = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) / (2 * px)   # down the rows
    nrm = np.stack([-gx, -gr, np.ones_like(h)], axis=-1)     # DirectX: green down the rows
    nrm /= np.linalg.norm(nrm, axis=-1, keepdims=True)
    occ = np.clip(1.0 - 0.35 * seam - 0.12 * (1.0 - puff), 0.0, 1.0)
    orm = np.stack([occ, np.full_like(occ, 0.9), np.zeros_like(occ)], axis=-1)
    for kind, data in (('BC', np.repeat(np.clip(bc, 0, 1)[..., None], 3, -1)), ('N', nrm * 0.5 + 0.5), ('ORM', orm)):
        path = lt.texture_path(LINING_SET, kind)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        lt.write_png(path, lt.to8(data))


def cushion(size, center, mat, round_=0.006):
    """A padded block: a box rounded on every edge (three bevel segments), in the lining."""
    bm = lp.new_bmesh()
    bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Diagonal(Vector(size)).to_4x4())
    bmesh.ops.bevel(bm, geom=list(bm.edges), offset=round_, segments=3, profile=0.5, affect='EDGES', clamp_overlap=True)
    obj = lp.place(lp.mesh_object(bm), center)
    lt.assign(obj, mat)
    return obj


def bevel_rim(obj, seam, pair, new_index, offset=0.003):
    """Rounds the edges on the seam where the two materials in pair meet at an angle (the body's rim into the hollow,
    the lid's underside into its recess)."""
    idx = sorted(obj.data.materials.find(m.name) for m in pair)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    edges = [e for e in bm.edges if len(e.link_faces) == 2 and all(abs(v.co.z - seam) < 6e-4 for v in e.verts)
             and sorted(f.material_index for f in e.link_faces) == idx and e.calc_face_angle(0.0) > 0.9]
    made = 0
    if edges:
        res = bmesh.ops.bevel(bm, geom=edges, offset=offset, segments=1, affect='EDGES', clamp_overlap=True)
        for f in res['faces']:
            f.material_index = new_index
        made = len(res['faces'])
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return made


def hull_2d(points):
    pts = sorted(set((round(x, 5), round(z, 5)) for x, z in points))
    if len(pts) < 3:
        return pts
    cross = lambda o, a, b: (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for q in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], q) <= 0:
            lower.pop()
        lower.append(q)
    for q in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], q) <= 0:
            upper.pop()
        upper.append(q)
    return lower[:-1] + upper[:-1]


def boundary_points(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    pts = [v.co.copy() for v in bm.verts if any(e.is_boundary for e in v.link_edges)]
    bm.free()
    return pts


def split_at_seam(obj, seam, close=True):
    """obj cut at z = seam: returns (below or None, above or None); with close, each half is closed (extract's)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], dist=1e-6, plane_co=(0.0, 0.0, seam),
                           plane_no=(0.0, 0.0, 1.0))
    bm.to_mesh(obj.data)
    bm.free()
    above = [p.index for p in obj.data.polygons if p.center.z > seam]
    if not above:
        return obj, None
    if len(above) == len(obj.data.polygons):
        return None, obj
    return obj, extract(obj, above, obj.name + '_up', close=close)


def trim_ribbon(obj, inset, width, step, lift=0.001):
    """A ribbon on obj's front face along a rounded rectangle inset from its front outline (x across, z up): each
    point found by a ray from the front, the ribbon laid across the surface's normal, lift proud of it."""
    co = np.array([v.co[:] for v in obj.data.vertices])
    lo, hi = co.min(0), co.max(0)
    tree = tree_of(obj)
    x0, x1, z0, z1 = lo[0] + inset, hi[0] - inset, lo[2] + inset, hi[2] - inset
    r = min(inset, (x1 - x0) * 0.2, (z1 - z0) * 0.3)
    path = []
    corners = [(x1 - r, z1 - r, 0.0), (x0 + r, z1 - r, 90.0), (x0 + r, z0 + r, 180.0), (x1 - r, z0 + r, 270.0)]
    for i, (cx, cz, start) in enumerate(corners):
        for k in range(7):
            a = math.radians(start + 90.0 * k / 6)
            path.append((cx + r * math.cos(a), cz + r * math.sin(a)))
        nx, nz, _ = corners[(i + 1) % 4]
        ex, ez = path[-1]
        tx, tz = (nx + r * math.cos(math.radians(start + 90.0)), nz + r * math.sin(math.radians(start + 90.0)))
        length = math.hypot(tx - ex, tz - ez)
        for k in range(1, max(1, int(length / step))):
            f = k / max(1, int(length / step))
            path.append((ex + (tx - ex) * f, ez + (tz - ez) * f))
    hits = []
    for x, z in path:
        hit, normal, _, _ = tree.ray_cast(Vector((x, lo[1] - 1.0, z)), Vector((0.0, 1.0, 0.0)))
        hits.append((hit, normal) if hit is not None else None)
    bm = lp.new_bmesh()
    rows = []
    n = len(path)
    for i in range(n):
        if hits[i] is None:
            rows.append(None)
            continue
        hit, normal = hits[i]
        prev, nxt = path[i - 1], path[(i + 1) % n]
        tangent = Vector((nxt[0] - prev[0], 0.0, nxt[1] - prev[1])).normalized()
        across = normal.cross(tangent).normalized()
        base = hit + normal * lift
        rows.append((bm.verts.new(base - across * width * 0.5), bm.verts.new(base + across * width * 0.5)))
    for i in range(n):
        a_, b_ = rows[i], rows[(i + 1) % n]
        if a_ and b_:
            bm.faces.new((a_[0], b_[0], b_[1], a_[1]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    for f in bm.faces:   # facing out of the chest (forward)
        if f.normal.y > 0.0:
            f.normal_flip()
    return lp.mesh_object(bm, '_TrimStrip')


# --- Overlays: the lights, crisp ---

def plate(p, mat, proud=0.0015, thick=0.003):
    """A thin emissive plate over a painted light, on the face its patch faces, standing just proud of it."""
    axis = int(np.argmax(np.abs(p.normal)))
    sign = 1.0 if p.normal[axis] > 0.0 else -1.0
    outer = p.hi[axis] if sign > 0.0 else p.lo[axis]
    size = [max((p.hi[i] - p.lo[i]) * 0.94, 0.005) for i in range(3)]
    size[axis] = thick
    center = [(p.lo[i] + p.hi[i]) * 0.5 for i in range(3)]
    center[axis] = outer + sign * (proud + thick * 0.5)
    obj = lp.block(size, center, bevel=min(size[i] for i in range(3) if i != axis) * 0.2)
    lt.assign(obj, mat)
    lt.box_uv(obj, 'MetalWorn')
    return obj


def oriented(obj, center, normal):
    q = Vector(normal).normalized().to_track_quat('Z', 'Y')
    obj.data.transform(q.to_matrix().to_4x4())
    return lp.place(obj, center)


def ring(major, minor, center, normal, mat, segments=32, sides=6):
    profile = [(major + math.cos(a) * minor, math.sin(a) * minor) for a in (2.0 * math.pi * k / sides for k in range(sides))]
    obj = lp.lathe(profile, segments=segments, closed=True)[0]
    lt.assign(obj, mat)
    lt.box_uv(obj, 'MetalWorn')
    return oriented(obj, center, normal)


def orb(radius, center, mat):
    bm = lp.new_bmesh()
    bmesh.ops.create_uvsphere(bm, u_segments=20, v_segments=12, radius=radius)
    obj = lp.mesh_object(bm)
    lt.assign(obj, mat)
    lt.box_uv(obj, 'MetalWorn')
    return lp.place(obj, center)


def gem(radius, height, center, normal, mat, sides=8):
    obj = lp.lathe([(0.0, -height * 0.4), (radius, 0.0), (radius * 0.55, height * 0.45), (0.0, height * 0.5)],
                   segments=sides)[0]
    lt.assign(obj, mat)
    lt.box_uv(obj, 'MetalWorn')
    return oriented(obj, center, normal)


def join_into(target, parts):
    """Merges parts into target (target keeps its name and origin)."""
    parts = [p for p in parts if p is not None]
    if not parts:
        return target
    bpy.context.view_layer.update()   # the join maps world into target's space by matrix_world: set_origin's move must be in it
    with bpy.context.temp_override(active_object=target, object=target, selected_objects=[target] + parts,
                                   selected_editable_objects=[target] + parts):
        bpy.ops.object.join()
    return target


# --- Maps ---

def write_maps(maps, set_name, mat_name, source, size, grade):
    """Tripo's maps as the texture set set_name (looter_tripo.write_set, always rewritten so the sharpening runs once),
    the colour map then sharpened (an unsharp mask: Tripo's maps come out soft) and graded (saturation, contrast round
    the middle grey) toward the game's stylized realism."""
    for kind in ('BC', 'ORM', 'N'):
        path = lt.texture_path(set_name, kind)
        if os.path.exists(path):
            os.remove(path)
    mat = tripo.write_set(maps, set_name, mat_name, source, master='Gun', orm_size=size, max_size=size)
    path = lt.texture_path(set_name, 'BC')
    img = bpy.data.images.load(path, check_existing=False)
    rgb = tripo.pixels(img)[..., :3].astype(np.float32)
    bpy.data.images.remove(img)
    sigma = 1.2 * size / 1024.0
    soft = lt.blur(rgb, sigma)
    sharp = rgb + 0.85 * (rgb - soft)
    sat, contrast = grade
    luma = (sharp @ np.array([0.299, 0.587, 0.114], np.float32))[..., None]
    sharp = luma + (sharp - luma) * sat
    sharp = 0.5 + (sharp - 0.5) * contrast
    lt.write_png(path, lt.to8(np.clip(sharp, 0.0, 1.0)))
    for image in bpy.data.images:   # the preview material shows the sharpened map
        if image.filepath and os.path.normcase(os.path.abspath(bpy.path.abspath(image.filepath))) == os.path.normcase(os.path.abspath(path)):
            image.reload()
    log(f'{set_name}: maps {size} px, colour sharpened (sigma {sigma:.1f} px) and graded {grade}')
    return mat


# --- A tier ---

def build(t):
    spec = TIERS[t]
    name = f'ChestTier{t}'
    path = GLB.format(t)
    obj, maps = tripo.load(path)
    for k in list(obj.keys()):
        del obj[k]
    me = obj.data
    co = np.array([v.co[:] for v in me.vertices])
    lo, hi = co.min(0), co.max(0)
    raw_w = hi[0] - lo[0]
    s = spec['width'] / raw_w
    u = lambda units: units * s   # the GLB's units to meters
    me.transform(Matrix.Diagonal((s, s * spec.get('depth', 1.0), s, 1.0))
                 @ Matrix.Translation((-(lo[0] + hi[0]) * 0.5, -(lo[1] + hi[1]) * 0.5, -lo[2])))
    me.update()
    closed = fill_holes(obj)
    inner = outer_shell(obj)
    if inner:
        log(f'{name}: the model was hollow: {inner} inside faces dropped')
    moved = planarize(obj, u(spec['planar']), u(0.25))
    co = np.array([v.co[:] for v in me.vertices])
    lo, hi = co.min(0), co.max(0)
    log(f'{name}: {tris_of(obj)} triangles in; {closed} hole faces filled, {moved} vertices flattened onto panels; '
        f'{(hi - lo)[0] * 100:.1f} x {(hi - lo)[1] * 100:.1f} x {(hi - lo)[2] * 100:.1f} cm')

    # Materials: Tripo's maps on the Gun master; the lights picked from the colour map.
    mat = write_maps(maps, 'Tripo' + name, name, path, spec['maps'], spec['grade'])
    glow = lm.material(name + 'Glow', spec['glow'], Glow=spec['strength'], Variation=0.0)
    lt.assign(obj, mat)
    picked = []
    for test in spec['tests']:
        picked += tripo.faces_where(obj, maps['BC'], test, size=2048, share=0.6)
    picked = sorted(set(picked))
    patches = [Patch(obj, g) for g, a in components(obj, picked) if a > u(0.8) ** 2]
    lt.assign(obj, glow, faces=[f for p in patches for f in p.faces])
    trim_strip = None
    if spec.get('trim'):
        # The gold trim's light: a thin emissive ribbon laid on the carved front (1 mm proud), framing it inside the
        # gold edging, its own slot so the opening can light it in a sweep. (Strips copied from the gold-painted faces
        # came out as blotches: the trim's painted lines are thinner than the mesh's faces.)
        trim = lm.material(name + 'TrimGlow', 0xd9a84a, Glow=0.35, Variation=0.0)
        trim_strip = trim_ribbon(obj, inset=u(4.0), width=u(0.7), step=0.008)
        lt.assign(trim_strip, trim)
        lt.box_uv(trim_strip, 'MetalWorn')
        log(f'{name}: gold trim ribbon, {len(trim_strip.data.polygons)} faces')
    for img in maps.values():
        if img:
            bpy.data.images.remove(img)
    log(f'{name}: {len(patches)} painted lights ({sum(len(p.faces) for p in patches)} faces)')

    seam = u(spec['seam']) if spec.get('seam_fixed') else find_seam(obj, u(spec['seam']), u(4.0))

    # The pieces the opening animates, cut out of the dense mesh.
    pieces, sockets, lid_sockets = [], [], []
    for side, box in zip(('L', 'R'), spec.get('latches', [])):
        x0, x1, z0, z1 = (u(v) for v in box)
        faces, backing = protruding(obj, lambda x, z: x0 <= x <= x1 and z0 <= z <= z1, u(spec['margin']))
        latch = extract(obj, faces, f'{name}_Latch_{side}', fill=RIM, flat=backing)
        lc = np.array([v.co[:] for v in latch.data.vertices])
        pivot = ((lc[:, 0].min() + lc[:, 0].max()) * 0.5, backing, lc[:, 2].min())
        set_origin(latch, pivot)
        pieces.append(latch)
        sockets.append(('Latch_' + side, pivot))
        log(f'{latch.name}: {len(faces)} faces cut, {(lc.max(0) - lc.min(0))[2] * 100:.1f} cm tall, pivot '
            f'{tuple(round(c, 3) for c in pivot)}')
    lock = None
    if spec.get('lock'):
        lx, lz, lr = (u(v) for v in spec['lock'])
        # Everything standing off the case's panel inside the circle (the housing disc and its ring): the panel is
        # the deepest of the front faces in a band round the housing.
        faces, backing = protruding(obj, lambda x, z: (x - lx) ** 2 + (z - lz) ** 2 <= lr * lr, u(spec['margin']),
                                    around=lambda x, z: (lr * 1.8) ** 2 <= (x - lx) ** 2 + (z - lz) ** 2 <= (lr * 2.6) ** 2,
                                    share=0.9)
        ring_patches = [p for p in patches if (p.center[0] - lx) ** 2 + (p.center[2] - lz) ** 2 <= lr * lr]
        radius = lr * 0.6
        if ring_patches:
            pts = np.array([obj.data.polygons[f].center[:] for p in ring_patches for f in p.faces])
            lx, lz = float(pts[:, 0].mean()), float(pts[:, 2].mean())
            radius = float(np.median(np.hypot(pts[:, 0] - lx, pts[:, 2] - lz)))
        lock = extract(obj, faces, f'{name}_Lock', fill=RIM, flat=backing)
        lc = np.array([v.co[:] for v in lock.data.vertices])
        # The ring's face: the lock's front within the ring.
        ring_front = lc[(lc[:, 0] - lx) ** 2 + (lc[:, 2] - lz) ** 2 <= (radius * 1.35) ** 2][:, 1]
        front_y = float(ring_front.min()) if len(ring_front) else float(lc[:, 1].min())
        pivot = (lx, backing, lz)
        set_origin(lock, pivot)
        pieces.append(lock)
        sockets.append(('Lock', pivot))
        patches = [p for p in patches if p not in ring_patches]
        lock_ring = (lx, lz, radius, front_y)
        log(f'{lock.name}: {len(faces)} faces cut, ring at x {lx:.3f} z {lz:.3f} radius {lock_ring[2] * 100:.1f} cm')
    crest = None
    if spec.get('crest'):
        x0, x1, z0, z1 = (u(v) for v in spec['crest'])
        pad = u(4.0)
        faces, backing = protruding(obj, lambda x, z: x0 <= x <= x1 and z0 <= z <= z1, u(spec['margin']),
                                    around=lambda x, z: x0 - pad <= x <= x1 + pad and z0 - pad <= z <= z1 + pad)
        crest = extract(obj, faces, f'{name}_Crest', fill=RIM, flat=backing)
        lc = np.array([v.co[:] for v in crest.data.vertices])
        pivot = ((lc[:, 0].min() + lc[:, 0].max()) * 0.5, backing, lc[:, 2].max())
        set_origin(crest, pivot)
        pieces.append(crest)
        log(f'{crest.name}: {len(faces)} faces cut, {(lc.max(0) - lc.min(0))[2] * 100:.1f} cm tall')

    # The hollow and the lid's recess, measured on the dense mesh (closed and clean: decimation can leave the odd
    # inside-out face that fools the inside test).
    wall = u(spec['wall']) if spec.get('wall') else max(u(3.0), 0.018)
    floor_guess = seam * 0.22
    cx, cy, ax, ay = fit_cavity(obj, max(floor_guess, 0.02), seam - 0.005, wall)
    bottoms = surface_along(obj, [(cx + i * ax, cy + j * ay, seam - 0.01) for i in (-1, 0, 1) for j in (-1, 0, 1)],
                            (0.0, 0.0, -1.0))
    floor = max(max(bottoms) + wall, floor_guess) if bottoms else floor_guess
    top = hi[2]
    lax = lay = 0.0
    # The lid's recess: as deep as half the lid where it can be, else shallower and with thinner walls (tier 5's
    # lid is a thin crown).
    for share_, wall_ in ((0.5, 0.8), (0.3, 0.6), (0.15, 0.5)):
        try:
            _, _, lax, lay = fit_cavity(obj, seam + 0.003, seam + (top - seam) * share_, wall * wall_)
            lax, lay = min(lax, ax), min(lay, ay)
            break
        except RuntimeError:
            lax = lay = 0.0
    roof = seam
    if lay < 0.06:
        lax = lay = 0.0
    if lax > 0.0:
        tops = surface_along(obj, [(cx + i * lax, cy + j * lay, seam + 0.003) for i in (-1, 0, 1) for j in (-1, 0, 1)],
                             (0.0, 0.0, 1.0))
        roof = min(min(tops) - wall * 0.5, seam + (top - seam) * 0.55) if tops else seam + (top - seam) * 0.4

    # The budget: the pieces keep 60%, the rest goes to the case.
    # What comes after (the hollow's booleans, the rim's bevel, the cushions and lights) costs about 3500.
    budget = spec['tris'] - sum(tris_of(p) for p in pieces) - 3500
    keep = [(Vector(p.lo) - Vector((0.08,) * 3), Vector(p.hi) + Vector((0.08,) * 3)) for p in patches] \
        if spec.get('spare_lights') else ()
    decimate(obj, budget, keep)

    # Lid and body, parted at the seam.
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], dist=1e-6, plane_co=(0.0, 0.0, seam),
                           plane_no=(0.0, 0.0, 1.0))
    bm.to_mesh(obj.data)
    bm.free()
    keep = u(spec['keep_body']) if spec.get('keep_body') else None
    lid_faces = [p.index for p in obj.data.polygons
                 if p.center.z > seam and not (keep and abs(p.center.x) > keep)]
    lid = extract(obj, lid_faces, name + '_Lid')
    body = obj
    body.name = body.data.name = name
    for part in (body, lid):
        if part.data.materials.find(RIM.name) < 0:
            part.data.materials.append(RIM)
    rim_index = {part: part.data.materials.find(RIM.name) for part in (body, lid)}
    # extract() closed both halves; the caps on the seam become the rim.
    for part in (body, lid):
        for poly in part.data.polygons:
            if all(abs(part.data.vertices[v].co.z - seam) < 1e-4 for v in poly.vertices):
                poly.material_index = rim_index[part]

    # Hollow the body, recess the lid's underside.
    lining_set()
    lining = lt.material(LINING_SET, name=spec['lining'][0], tint=spec['lining'][1], master='World')
    bc = np.array([v.co[:] for v in body.data.vertices])
    blo, bhi = bc.min(0), bc.max(0)
    carve(body, (2 * ax, 2 * ay, seam - floor + 0.02), (cx, cy, (floor + seam + 0.02) * 0.5), lining)
    if roof > seam + 0.008:
        carve(lid, (2 * lax, 2 * lay, roof - seam + 0.02), (cx, cy, (roof + seam - 0.02) * 0.5), lining)
    # The lid's underside round its recess is lining too (only the body's rim is the iron gasket): a wide iron cap
    # read as a black slab under tier 5's thin crown.
    lid_rim, lid_lining = lid.data.materials.find(RIM.name), lid.data.materials.find(lining.name)
    if lid_lining < 0:
        lid.data.materials.append(lining)
        lid_lining = len(lid.data.materials) - 1
    for poly in lid.data.polygons:
        if poly.material_index == lid_rim:
            poly.material_index = lid_lining
    rounded = bevel_rim(body, seam, (RIM, lining), body.data.materials.find(RIM.name))
    rounded += bevel_rim(lid, seam, (lining, lining), lid.data.materials.find(lining.name))
    for part in (body, lid):
        lt.box_uv(part, LINING_SET, texel_density=LINING_DENSITY, faces=lining)
    for part in [body, lid] + pieces:
        lt.box_uv(part, 'MetalWorn', faces=RIM)
    log(f'{name}: hollow {2 * ax * 100:.1f} x {2 * ay * 100:.1f} cm, floor at {floor * 100:.1f} cm, seam at '
        f'{seam * 100:.1f} of {hi[2] * 100:.1f} cm; lid recess {2 * lax * 100:.1f} x '
        f'{2 * lay * 100:.1f} cm; {rounded} rim bevel faces')

    # The lights, crisp.
    body_extra, lid_extra = [], []

    def owner(z):
        return lid_extra if z > seam else body_extra

    if spec.get('plates'):
        for p in patches:
            if p.area < u(1.5) ** 2:
                continue
            owner(p.center[2]).append(plate(p, glow))
        main = max(patches, key=lambda p: p.area) if patches else None
        if main is not None and t <= 2:
            sockets.append(('Light', tuple(main.center)))
    if lock is not None:
        lx, lz, radius, front = lock_ring
        ring_glow = lm.material(name + 'LockGlow', spec['glow'], Glow=spec['strength'], Variation=0.0)
        minor = max(u(0.45), 0.0025)
        r = ring(radius, minor, (lx, front - 0.002, lz), (0.0, -1.0, 0.0), ring_glow)
        # A crisp lock face over Tripo's smear inside the ring: a dark steel disc, two turned grooves, a keyhole that
        # lights with the ring (so it turns green with it).
        face_mat = lt.material('MetalWorn', name='ChestTier3LockFace', tint=0x2b2f37)
        steel = lt.material('MetalWorn', name='ChestTier3LockSteel', tint=0x8d96a3)
        disc_r = radius * 1.3   # out past the ring, over the last of Tripo's smear
        disc = lp.lathe([(0.0, 0.0), (disc_r, 0.0), (disc_r, 0.0025), (0.0, 0.0025)], segments=40)[0]
        lt.assign(disc, face_mat)
        lt.box_uv(disc, 'MetalWorn')
        oriented(disc, (lx, front + 0.001, lz), (0.0, -1.0, 0.0))
        face_y = front + 0.001 - 0.0025
        parts_ = [r, disc]
        for k in (0.55, 0.33):
            parts_.append(ring(disc_r * k, 0.0009, (lx, face_y - 0.0002, lz), (0.0, -1.0, 0.0), steel, segments=40,
                               sides=4))
        hole = lp.lathe([(0.0, 0.0), (disc_r * 0.09, 0.0), (disc_r * 0.09, 0.0012), (0.0, 0.0012)], segments=16)[0]
        lt.assign(hole, ring_glow)
        lt.box_uv(hole, 'MetalWorn')
        oriented(hole, (lx, face_y + 0.0004, lz + disc_r * 0.08), (0.0, -1.0, 0.0))
        slot = lp.mesh_object(lp.new_bmesh())
        bm = bmesh.new()
        w0, w1, h0, h1 = disc_r * 0.04, disc_r * 0.08, lz + disc_r * 0.05, lz - disc_r * 0.24
        y0, y1 = face_y + 0.0004, face_y - 0.0008
        quad = [(lx - w0, h0), (lx + w0, h0), (lx + w1, h1), (lx - w1, h1)]
        f0 = [bm.verts.new((x, y1, z)) for x, z in quad]
        b0 = [bm.verts.new((x, y0, z)) for x, z in quad]
        bm.faces.new(f0)
        bm.faces.new(list(reversed(b0)))
        for i in range(4):
            bm.faces.new((f0[i], b0[i], b0[(i + 1) % 4], f0[(i + 1) % 4]))
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        bm.to_mesh(slot.data)
        bm.free()
        lt.assign(slot, ring_glow)
        lt.box_uv(slot, 'MetalWorn')
        parts_ += [hole, slot]
        join_into(lock, parts_)
    if spec.get('orb'):
        ox, oz = (u(v) for v in spec['orb'])
        near = [p for p in patches if abs(p.center[0] - ox) < u(8) and abs(p.center[2] - oz) < u(8)]
        p = max(near, key=lambda p: p.area) if near else None
        if p is not None:
            radius = max(p.hi[0] - p.lo[0], p.hi[2] - p.lo[2]) * 0.42
            ox, oz, oy = p.center[0], p.center[2], p.lo[1] + radius * 0.45
        else:
            radius, oy = u(3.0), blo[1] + u(1.0)
        owner(oz).append(orb(radius, (ox, oy, oz), glow))
        (lid_sockets if oz > seam else sockets).append(('Orb', (ox, oy, oz)))
        log(f'{name}: orb {radius * 200:.1f} cm across at z {oz * 100:.1f} cm ({"lid" if oz > seam else "body"})')
    if crest is not None:
        gx, gz = (u(v) for v in spec['gem'])
        near = [p for p in patches if abs(p.center[0] - gx) < u(6) and abs(p.center[2] - gz) < u(6)]
        p = max(near, key=lambda p: p.area) if near else None
        if p is not None:
            radius = max(p.hi[0] - p.lo[0], p.hi[2] - p.lo[2]) * 0.5
            gx, gz, gy = p.center[0], p.center[2], p.lo[1]
        else:
            radius, gy = u(2.5), crest.location.y - u(2.0)
        gem_glow = lm.material(name + 'GemGlow', 0xffd36b, Glow=spec['strength'] * 1.2, Variation=0.0)
        g = gem(radius, radius * 0.9, (gx, gy - 0.001, gz), (0.0, -1.0, 0.0), gem_glow)
        join_into(crest, [g])
        lm.socket(crest, 'Gem', (gx - crest.location.x, gy - crest.location.y, gz - crest.location.z))
        lid_sockets.append(('Crest', tuple(crest.location)))
        log(f'{name}: gem {radius * 200:.1f} cm across')
    if spec.get('inner'):
        inner = lm.material(name + 'InnerGlow', spec['glow'], Glow=spec['strength'] * 0.6, Variation=0.0)
        z = seam - 0.022
        h, t_ = 0.008, 0.002
        for sx in (-1.0, 1.0):
            body_extra.append(lp.block((t_, 2 * ay - 0.004, h), (cx + sx * (ax - t_ * 0.5 - 0.0005), cy, z)))
        for sy in (-1.0, 1.0):
            body_extra.append(lp.block((2 * ax - 0.004, t_, h), (cx, cy + sy * (ay - t_ * 0.5 - 0.0005), z)))
        for o in body_extra[-4:]:
            lt.assign(o, inner)
            lt.box_uv(o, 'MetalWorn')
    # The padding: a cushion on the hollow's floor, a padded panel in the lid's recess (the recess's walls its lip).
    pad = cushion((2 * ax - 0.008, 2 * ay - 0.008, 0.018), (cx, cy, floor + 0.009), lining)
    lt.box_uv(pad, LINING_SET, texel_density=LINING_DENSITY)
    body_extra.append(pad)
    if roof > seam + 0.012:
        panel = cushion((2 * lax - 0.018, 2 * lay - 0.018, 0.01), (cx, cy, roof - 0.005), lining, round_=0.004)
        lt.box_uv(panel, LINING_SET, texel_density=LINING_DENSITY)
        lid_extra.append(panel)
    if trim_strip is not None:
        below, above = split_at_seam(trim_strip, seam, close=False)
        body_extra += [below] if below else []
        lid_extra += [above] if above else []
    join_into(body, body_extra)
    join_into(lid, lid_extra)

    # Hinge, sockets, collision, finish.
    near_seam = bc[np.abs(bc[:, 2] - seam) < 0.01]
    hinge = (0.0, float(near_seam[:, 1].max()) if len(near_seam) else float(bhi[1]), seam)
    set_origin(lid, hinge)
    lid['OpenAngle'] = OPEN_ANGLE[t]
    front = float(near_seam[:, 1].min()) if len(near_seam) else float(blo[1])
    lm.socket(body, 'Lid', hinge)
    lm.socket(body, 'Loot', (cx, cy, floor + 0.10))
    lm.socket(body, 'Interact', (0.0, front - 0.03, seam - 0.04))
    for sname, at in sockets:
        lm.socket(body, sname, at)
    for sname, at in lid_sockets:
        lm.socket(lid, sname, tuple(Vector(at) - Vector(hinge)))
    lp.hull_box(body, (bhi[0] - blo[0], bhi[1] - blo[1], seam), ((bhi[0] + blo[0]) * 0.5, (bhi[1] + blo[1]) * 0.5,
                                                                seam * 0.5))
    lc_ = np.array([v.co[:] for v in lid.data.vertices])
    llo, lhi = lc_.min(0), lc_.max(0)
    lp.hull_box(lid, tuple(lhi - llo), tuple((lhi + llo) * 0.5))
    for part in [body, lid] + pieces:
        used = {p.material_index for p in part.data.polygons}
        for i in reversed(range(len(part.data.materials))):   # no empty slots in the export
            if i not in used:
                part.data.materials.pop(index=i)
        lm.smooth(part, 35.0)
        part.data.update()
        lt.bake_vertex_ao(part, distance=0.3, ground=part is body)
        part['Nanite'] = 0
        part['LODs'] = LODS
    for part in [body, lid] + pieces:
        counts = np.bincount([p.material_index for p in part.data.polygons], minlength=len(part.data.materials))
        log(f'  {part.name}: ' + ', '.join(f'{m.name} {c}' for m, c in zip(part.data.materials, counts) if c))
    total = sum(tris_of(p) for p in [body, lid] + pieces)
    log(f'{name}: body {tris_of(body)}, lid {tris_of(lid)}, ' + ', '.join(f'{p.name[len(name) + 1:]} {tris_of(p)}'
                                                                          for p in pieces) + f'; total {total} triangles')
    return dict(tier=t, body=body, lid=lid, pieces=pieces, hinge=hinge, width=spec['width'], seam=seam, front=front)


# --- Previews (with --preview) ---

def stage():
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = 48
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    for o in scene.objects:
        if o.name.startswith('UCX_'):
            o.hide_render = True
    for mat in bpy.data.materials:
        if mat.get('Glow') and mat.use_nodes:
            bsdf = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
            bsdf.inputs['Emission Color'].default_value = bsdf.inputs['Base Color'].default_value
            bsdf.inputs['Emission Strength'].default_value = min(float(mat['Glow']), 1.5)
    mesh = bpy.data.meshes.new('_Ground')
    mesh.from_pydata([(-20.0, -20.0, 0.0), (20.0, -20.0, 0.0), (20.0, 20.0, 0.0), (-20.0, 20.0, 0.0)], [], [(0, 1, 2, 3)])
    ground = bpy.data.objects.new('_Ground', mesh)
    scene.collection.objects.link(ground)
    gm = bpy.data.materials.new('_GroundMat')
    gm.use_nodes = True
    next(n for n in gm.node_tree.nodes if n.type == 'BSDF_PRINCIPLED').inputs['Base Color'].default_value = (0.16, 0.14, 0.12, 1)
    mesh.materials.append(gm)
    sun = bpy.data.objects.new('_Sun', bpy.data.lights.new('_Sun', 'SUN'))
    scene.collection.objects.link(sun)
    sun.data.energy, sun.data.color = 3.5, (1.0, 0.9, 0.78)
    sun.rotation_euler = (math.radians(50), 0.0, math.radians(-35))
    world = bpy.data.worlds.new('_Sky')
    world.use_nodes = True
    bg = next(n for n in world.node_tree.nodes if n.type == 'BACKGROUND')
    bg.inputs['Color'].default_value = (0.42, 0.5, 0.62, 1.0)
    bg.inputs['Strength'].default_value = 0.8
    scene.world = world


def socket_of(obj, name):
    return next((c for c in obj.children if c.name.split('.')[0] == 'SOCKET_' + name), None)


def pose(model, at, opened):
    """Stands a tier at `at` (body origin), lid opened by `opened` (0..1), latches popped, crest unfolded."""
    body = model['body']
    body.matrix_world = Matrix.Translation(at)
    bpy.context.view_layer.update()
    lid = model['lid']
    lid.matrix_world = socket_of(body, 'Lid').matrix_world @ Matrix.Rotation(math.radians(-OPEN_ANGLE[model['tier']] * opened), 4, 'X')
    bpy.context.view_layer.update()
    for p in model['pieces']:
        tag = p.name.split('_', 1)[1]
        if tag.startswith('Latch'):
            p.matrix_world = socket_of(body, tag).matrix_world @ Matrix.Rotation(math.radians(70.0 * opened), 4, 'X')
        elif tag == 'Lock':
            p.matrix_world = socket_of(body, 'Lock').matrix_world @ Matrix.Rotation(math.radians(90.0 * opened), 4, 'Y')
        elif tag == 'Crest':
            p.matrix_world = socket_of(lid, 'Crest').matrix_world @ Matrix.Rotation(math.radians(-60.0 * opened), 4, 'X')
    bpy.context.view_layer.update()


def shoot(path, shown, eye, target, lens, size=(900, 700)):
    scene = bpy.context.scene
    visible = set()
    for m in shown:
        for o in [m['body'], m['lid']] + m['pieces']:
            visible.add(o)
    for o in scene.objects:
        if o.type == 'MESH' and not o.name.startswith('_Ground'):
            o.hide_render = o not in visible
    cam = bpy.data.objects.get('_Camera') or bpy.data.objects.new('_Camera', bpy.data.cameras.new('_Camera'))
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam.location = Vector(eye)
    cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens, cam.data.clip_start, cam.data.clip_end = lens, 0.02, 200.0
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    return path


def load_rgb(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    buf = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(buf)
    bpy.data.images.remove(img)
    return buf.reshape(h, w, 4)[::-1, :, :3]


def sheet(paths, out):
    rows = [np.concatenate([load_rgb(p) for p in row], axis=1) for row in paths]
    width = max(r.shape[1] for r in rows)
    rows = [np.pad(r, ((0, 0), (0, width - r.shape[1]), (0, 0))) for r in rows]
    lt.write_png(out, lt.to8(np.concatenate(rows, axis=0)))


def previews(models):
    os.makedirs(OUT, exist_ok=True)
    stage()
    for m in models:
        t, w = m['tier'], m['width']
        h = m['body'].dimensions.z + m['lid'].dimensions.z
        c = (0.0, 0.0, h * 0.45)
        shots = []
        pose(m, (0.0, 0.0, 0.0), 0.0)
        shots.append(shoot(os.path.join(OUT, f'_T{t}_front.png'), [m], (0.0, -w * 2.4, h * 0.55), c, 50))
        shots.append(shoot(os.path.join(OUT, f'_T{t}_34.png'), [m], (w * 1.4, -w * 1.9, h * 1.6), c, 50))
        shots.append(shoot(os.path.join(OUT, f'_T{t}_near.png'), [m], (-w * 0.1, m['front'] - w * 0.8, m['seam'] + w * 0.12),
                           (-w * 0.17, m['front'], m['seam'] - w * 0.03), 50))
        pose(m, (0.0, 0.0, 0.0), 1.0)
        shots.append(shoot(os.path.join(OUT, f'_T{t}_open.png'), [m], (w * 0.9, -w * 2.0, h * 2.6), c, 45))
        # The opened lid's inside, straight on (its inner face turns forward and up as it opens).
        a = math.radians(OPEN_ANGLE[t])
        hy, d = m['hinge'][1], 2.0 * m['hinge'][1]
        target = Vector((0.0, hy + 0.5 * d * math.cos(math.pi - a), m['seam'] + 0.5 * d * math.sin(a)))
        normal = Vector((0.0, -math.sin(a), -math.cos(a)))
        shots.append(shoot(os.path.join(OUT, f'_T{t}_lid.png'), [m], target + normal * (w * 1.05) + Vector((w * 0.12, 0, 0)),
                           target, 40))
        sheet([shots], os.path.join(OUT, f'Tier{t}.png'))
        for p in shots:
            if not os.environ.get('LOOTER_CHEST_KEEP'):
                os.remove(p)
        log(f'rendered Saved/ChestFit/Tier{t}.png')
    if len(models) > 1:
        x, rows = 0.0, []
        for m in models:
            m['at'] = (x + m['width'] * 0.5, 0.0, 0.0)
            x += m['width'] + 0.35
        mid = x * 0.5 - 0.175
        for opened, tag in ((0.0, 'closed'), (1.0, 'open')):
            for m in models:
                pose(m, m['at'], opened)
            rows.append([shoot(os.path.join(OUT, f'_row_{tag}.png'), models, (mid + 0.6, -6.6, 2.6), (mid, 0.0, 0.2),
                               38, size=(1800, 560))])
        sheet(rows, os.path.join(OUT, 'Tiers.png'))
        for row in rows:
            os.remove(row[0])
        log('rendered Saved/ChestFit/Tiers.png')


wanted = os.environ.get('LOOTER_CHEST_TIERS', '')
chosen = [int(x) for x in wanted.split(',') if x.strip().isdigit()] or sorted(TIERS)
built = []
for tier in chosen:
    built.append(build(tier))
    # The next tier's build mustn't see this one's leftovers.
    for o in list(bpy.data.objects):
        if o.name.startswith('_'):
            bpy.data.objects.remove(o)

if lt.want_preview():
    previews(built)
