"""Swaps a scripted building's mesh for a Tripo model (Art/Tripo/<Name>/<Name>.glb, made by Tools/tripo.py from
Art/Tripo/specs/buildings.json) fitted into the scripted building's place, so the game can't tell the two apart.

A Tripo building is one closed shell with its paint on one atlas. The scripted model (looter_buildings) stays the
source of everything the game reads: its footprint and height (the shell is stretched onto them, axis by axis, then
bent piecewise so chosen features land where the scripted ones stood: a porch deck's height, a front wall's plane),
its UCX hulls (kept as they are: they fit the same footprint), its sockets (kept, same names and places) and the parts
the story needs that a shell can't have (a door opening with the lamplit hall behind it, the stool under Delia's plate):
faces of the scripted mesh taken by box and joined to the shell, with the shell cut away where they go.

Materials: the shell wears one textured material, MI_Tripo<Name> on the World master (the same master as the house trim
sheet, so it answers light like the buildings beside it), its maps written by looter_tripo.write_set into
Art/Textures/Tripo<Name>/ (at most 2048 px). Vertex colour alpha is baked occlusion, as on every building.

Switch: LOOTER_BUILDINGS_TRIPO unset or 1 = every building with a Tripo swap; 0 = the scripted ones only; a comma list
of Tripo names (FarmhouseRansom,BarnRansom) = only those. A missing .glb leaves the scripted model.

    import looter_tripo_buildings as tb
    tb.swap(ransom, 'FarmhouseRansom', cut=[...], keep=[...], anchors={'z': [...]})
"""
import math
import os

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_textures as lt
import looter_tripo as tripo

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
_AXIS = {'x': 0, 'y': 1, 'z': 2}


def log(message):
    print(f'LOOTER: tripo building: {message}', flush=True)


def enabled(name):
    value = os.environ.get('LOOTER_BUILDINGS_TRIPO', '1').strip()
    if value in ('', '1'):
        return True
    if value == '0':
        return False
    return name in [v.strip() for v in value.split(',')]


def glb_path(name, variant=''):
    """Art/Tripo/<name>/<name><variant>.glb (variant '_grey': Tripo's first paint, kept beside a repaint)."""
    return os.path.join(REPO, 'Art', 'Tripo', name, f'{name}{variant}.glb')


# --- Measuring ---

def bounds(obj, faces=None):
    """(lo, hi) of obj's vertices (of these faces only when given), in its own space."""
    me = obj.data
    if faces is None:
        cos = [v.co for v in me.vertices]
    else:
        idx = {i for f in faces for i in me.polygons[f].vertices}
        cos = [me.vertices[i].co for i in idx]
    lo = Vector((min(c[k] for c in cos) for k in range(3)))
    hi = Vector((max(c[k] for c in cos) for k in range(3)))
    return lo, hi


def tree(obj):
    me = obj.data
    return BVHTree.FromPolygons([v.co for v in me.vertices], [list(p.vertices) for p in me.polygons], epsilon=0.0)


def probe(obj, origin, direction, reach=50.0, bvh=None):
    """The first hit of a ray (point, normal, face) or None."""
    bvh = bvh or tree(obj)
    hit = bvh.ray_cast(Vector(origin), Vector(direction).normalized(), reach)
    return None if hit[0] is None else hit


def median(values):
    values = sorted(values)
    return values[len(values) // 2] if values else None


def deck_height(obj, points, top, low=0.05, bvh=None):
    """The median height a ray straight down from `top` meets at each (x, y) of points (a porch deck under its roof:
    start under the roof), ignoring hits below low."""
    bvh = bvh or tree(obj)
    hits = []
    for x, y in points:
        hit = probe(obj, (x, y, top), (0, 0, -1), top + 1.0, bvh)
        if hit and hit[0].z > low:
            hits.append(hit[0].z)
    return median(hits)


def wall_plane(obj, axis, sign, points, start, bvh=None):
    """The median coordinate along axis where rays from start (on that axis), heading sign (+1/-1) along it, through each
    of points (the other two coordinates, in x, y, z order with the axis left out), meet the shell."""
    bvh = bvh or tree(obj)
    a = _AXIS[axis]
    hits = []
    for p in points:
        o = list(p)
        o.insert(a, start)
        d = [0.0, 0.0, 0.0]
        d[a] = sign
        hit = probe(obj, o, d, 100.0, bvh)
        if hit:
            hits.append(hit[0][a])
    return median(hits)


def socket(obj, name):
    """obj's SOCKET_<name> empty, or None."""
    return next((c for c in obj.children if c.name.split('.')[0] == f'SOCKET_{name}'), None)


def move_socket(obj, name, location, why=''):
    empty = socket(obj, name)
    if empty is None:
        log(f'{obj.name}: no SOCKET_{name} to move')
        return
    old = empty.location.copy()
    empty.location = Vector(location)
    log(f'{obj.name}: SOCKET_{name} moved {tuple(round(v, 2) for v in old)} -> '
        f'{tuple(round(v, 2) for v in location)} {why}')


def protrusion_top(shell, scripted, near, radius=2.5, above=0.5, low=2.0):
    """The top of whatever stands up out of the shell's roof (a chimney, a stovepipe) nearest near (x, y): shell
    vertices more than `above` m over the scripted roof at the same spot (or over open ground) and over `low` m up,
    within radius of near. Returns (x, y, z) of the highest one there, or None."""
    bvh = tree(scripted)
    best = None
    for v in shell.data.vertices:
        c = v.co
        if c.z < low or (Vector((c.x, c.y)) - Vector(near)).length > radius:
            continue
        hit = probe(scripted, (c.x, c.y, 60.0), (0, 0, -1), 80.0, bvh)
        roof = hit[0].z if hit else 0.0
        if c.z - roof > above and (best is None or c.z > best.z):
            best = c.copy()
    if best is None:
        return None
    # The middle of the top: the shell's vertices within 15 cm under the highest, round it.
    top = [v.co for v in shell.data.vertices
           if best.z - v.co.z < 0.15 and (Vector((v.co.x, v.co.y)) - Vector((best.x, best.y))).length < 0.6]
    x = sum(c.x for c in top) / len(top)
    y = sum(c.y for c in top) / len(top)
    return (x, y, best.z)


# --- Shaping ---

def orient(obj, front='+X'):
    """Turns the shell so its front (the axis Tripo exported it facing) faces -Y, the models' front (Art/README.md)."""
    turn = {'+X': -90.0, '-X': 90.0, '+Y': 180.0, '-Y': 0.0}[front]
    obj.data.transform(Matrix.Rotation(math.radians(turn), 4, 'Z'))
    obj.data.update()


def fit_box(obj, lo, hi):
    """Stretches the shell, each axis on its own, so its bounds are lo..hi. Returns the three scales."""
    me = obj.data
    a, b = bounds(obj)
    scale = [(hi[k] - lo[k]) / max(b[k] - a[k], 1e-6) for k in range(3)]
    for v in me.vertices:
        v.co = Vector((lo[k] + (v.co[k] - a[k]) * scale[k] for k in range(3)))
    me.update()
    return scale


def remap(obj, axis, pairs):
    """Bends the shell along one axis, piecewise linearly: each (from, to) moves the coordinate `from` to `to`; the
    shell's two ends stay put, and what lies between two anchors stretches evenly."""
    me = obj.data
    k = _AXIS[axis]
    lo, hi = bounds(obj)
    knots = sorted([(lo[k], lo[k])] + [tuple(p) for p in pairs] + [(hi[k], hi[k])])

    def f(c):
        for (a0, b0), (a1, b1) in zip(knots, knots[1:]):
            if c <= a1:
                t = 0.0 if a1 - a0 < 1e-9 else (c - a0) / (a1 - a0)
                return b0 + (b1 - b0) * t
        return c

    for v in me.vertices:
        co = v.co.copy()
        co[k] = f(co[k])
        v.co = co
    me.update()


def faces_in(obj, boxes, whole=False):
    """Faces of obj whose centre (every corner, with whole) lies inside any of boxes ((x0, y0, z0), (x1, y1, z1))."""
    me = obj.data
    out = []
    for p in me.polygons:
        pts = [me.vertices[i].co for i in p.vertices] if whole else [p.center]
        for b0, b1 in boxes:
            if all(all(b0[k] <= c[k] <= b1[k] for k in range(3)) for c in pts):
                out.append(p.index)
                break
    return out


def cut_box(obj, box, margin=0.6):
    """Cuts a clean box-shaped hole in the shell: the faces near the box are split along the box's side planes (x0, x1,
    y0, y1, z0, z1), then every face whose centre lies inside it goes. Returns the number of faces removed."""
    b0, b1 = Vector(box[0]), Vector(box[1])
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for k in range(3):
        for value in (b0[k], b1[k]):
            near = [f for f in bm.faces if all(b0[j] - margin <= f.calc_center_median()[j] <= b1[j] + margin
                                               for j in range(3))]
            if not near:
                continue
            geom = list({e for f in near for e in f.edges}) + near + list({v for f in near for v in f.verts})
            normal = Vector((0.0, 0.0, 0.0))
            normal[k] = 1.0
            co = Vector((0.0, 0.0, 0.0))
            co[k] = value
            bmesh.ops.bisect_plane(bm, geom=geom, plane_co=co, plane_no=normal)
    gone = [f for f in bm.faces if all(b0[j] <= f.calc_center_median()[j] <= b1[j] for j in range(3))]
    bmesh.ops.delete(bm, geom=gone, context='FACES')
    count = len(gone)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return count


def delete_faces(obj, faces):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[bm.faces[i] for i in faces], context='FACES')
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def sharp_by_angle(obj, angle=40.0):
    """Smooth shading with edges sharper than angle marked hard (exported as shaded in Blender)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    limit = math.radians(angle)
    for f in bm.faces:
        f.smooth = True
    for e in bm.edges:
        if len(e.link_faces) == 2:
            e.smooth = e.calc_face_angle(0.0) <= limit
        else:
            e.smooth = False
    bm.to_mesh(obj.data)
    bm.free()


def _clean_attributes(obj):
    me = obj.data
    for attr in list(me.color_attributes):
        me.color_attributes.remove(attr)
    while len(me.uv_layers) > 1:
        me.uv_layers.remove(me.uv_layers[-1])
    if me.uv_layers:
        me.uv_layers[0].name = 'UVMap'


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


# --- Scripted parts copied, and shapes straightened ---

def copy_faces(obj, boxes, matrix=None, name='_TripoPiece'):
    """A new object holding obj's faces whose centres lie in boxes (with its UVs, colours and materials), its mesh moved
    by matrix (a 4x4) when given."""
    piece = bpy.data.objects.new(name, obj.data.copy())
    bpy.context.scene.collection.objects.link(piece)
    piece.matrix_world = obj.matrix_world.copy()
    keep = set(faces_in(obj, boxes))
    delete_faces(piece, [i for i in range(len(piece.data.polygons)) if i not in keep])
    if matrix is not None:
        piece.data.transform(matrix)
        piece.data.update()
    return piece


def flatten(shell, group, threshold=0.2, exclude=()):
    """Straightens wavy surfaces (Tripo's roofs carry corrugation-like ridges): group(co, normal) names the plane a
    vertex belongs to (or None); each group's plane (z = ax + by + c) is fitted by least squares, refitted without
    vertices over threshold off it, and every vertex within threshold is moved straight up or down onto it. Vertices in
    exclude boxes (a dormer, a stovepipe) stay. Returns {group: (count, rms before, rms after)}."""
    import numpy as np
    me = shell.data
    me.update()
    groups = {}
    for v in me.vertices:
        c = v.co
        if any(all(b0[k] <= c[k] <= b1[k] for k in range(3)) for b0, b1 in exclude):
            continue
        key = group(c, v.normal)
        if key is not None:
            groups.setdefault(key, []).append(v.index)
    report = {}
    for key, idx in groups.items():
        if len(idx) < 50:   # too few to fit a plane on (a sliver of underside): left as it is
            continue
        pts = np.array([tuple(me.vertices[i].co) for i in idx])
        use = np.ones(len(pts), bool)
        for _ in range(3):
            a = np.c_[pts[use, 0], pts[use, 1], np.ones(use.sum())]
            coef = np.linalg.lstsq(a, pts[use, 2], rcond=None)[0]
            off = pts[:, 2] - (coef[0] * pts[:, 0] + coef[1] * pts[:, 1] + coef[2])
            use = np.abs(off) < threshold
        rms = float(np.sqrt((off[use] ** 2).mean())) if use.any() else 0.0
        for i, ok, o in zip(idx, use, off):
            if ok:
                co = me.vertices[i].co
                me.vertices[i].co = Vector((co.x, co.y, co.z - o))
        report[key] = (int(use.sum()), round(rms, 3), (round(float(coef[0]), 3), round(float(coef[1]), 3),
                                                         round(float(coef[2]), 3)))
    me.update()
    return report


def onto_trim(shell, faces, strip, material='HouseTrim', rotate=False, seed=1, axis=None):
    """Moves the shell's faces onto the house trim sheet: a slot for `material` (the scripted model's, so it is the same
    MI_HouseTrim), mapped onto `strip` at world scale (lt.trim_uv, align 'world', cut into bands); axis sets U's direction
    (along the eaves: a gentle porch roof otherwise counts as a floor and each triangle picks its own grain)."""
    import random
    mat = bpy.data.materials.get(material)
    if mat is None:
        log(f'{shell.name}: no {material} material to map onto')
        return 0
    me = shell.data
    if material not in [m.name for m in me.materials if m]:
        me.materials.append(mat)
    slot = [m.name if m else '' for m in me.materials].index(material)
    for i in faces:
        me.polygons[i].material_index = slot
    lt.trim_uv(shell, lambda f: f.material_index == slot, strip, u_offset=random.Random(seed).uniform(0.0, 6.4),
               rotate=rotate, align='world', cut=True, axis=axis)
    return len(faces)


# --- Collision: the scripted hulls refitted to the shell ---

def _directions(count=26):
    """Unit directions for a k-DOP: the 26 of a cube's faces, edges and corners."""
    out = []
    for x in (-1, 0, 1):
        for y in (-1, 0, 1):
            for z in (-1, 0, 1):
                if (x, y, z) != (0, 0, 0):
                    out.append(Vector((x, y, z)).normalized())
    return out[:count]


def kdop(points):
    """The extreme points of points along 26 directions: a convex hull of at most 26 corners around them (Unreal's
    hulls want few vertices; a building's parts are boxy, so this hugs them)."""
    pts = list(points)
    picks = {}
    for d in _directions():
        best = max(pts, key=lambda p: p.dot(d))
        picks[tuple(round(c, 4) for c in best)] = best
    return list(picks.values())


def _hull_box(hull):
    pts = [hull.matrix_world @ v.co for v in hull.data.vertices]
    return (Vector((min(p[k] for p in pts) for k in range(3))), Vector((max(p[k] for p in pts) for k in range(3))))


def _set_hull(hull, points):
    bm = bmesh.new()
    verts = [bm.verts.new(hull.matrix_world.inverted() @ p) for p in points]
    result = bmesh.ops.convex_hull(bm, input=verts)
    inside = [v for v in result['geom_interior'] + result['geom_unused'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.delete(bm, geom=list(set(inside)), context='VERTS')
    bm.to_mesh(hull.data)
    bm.free()


def _new_hull(obj, points):
    mesh = bpy.data.meshes.new('UCX_' + obj.name)
    hull = bpy.data.objects.new('UCX_' + obj.name, mesh)
    bpy.context.scene.collection.objects.link(hull)
    hull.parent = obj
    hull.display_type = 'WIRE'
    _set_hull(hull, points)
    return hull


def refit_hulls(obj, shell, keep=(), drop=(), margin=(0.25, 0.25, 0.03)):
    """Each of obj's UCX hulls (scripted, boxes round the scripted parts) becomes the convex hull (a k-DOP) of the shell's
    vertices inside its box grown by margin, so walls and roofs collide where the shell's are. A hull whose box centre
    lies in one of keep (scripted parts kept in the model) stays as it is; one whose centre lies in drop goes (parts the
    shell has elsewhere: porch posts), and so does one with too few shell vertices in it."""
    world = shell.matrix_world
    verts = [world @ v.co for v in shell.data.vertices]
    for hull in [c for c in obj.children if c.name.startswith('UCX_')]:
        lo, hi = _hull_box(hull)
        mid = (lo + hi) * 0.5
        inside = lambda box: all(box[0][k] <= mid[k] <= box[1][k] for k in range(3))
        if any(inside(b) for b in keep):
            continue
        if any(inside(b) for b in drop):
            bpy.data.objects.remove(hull)
            continue
        pts = [p for p in verts if all(lo[k] - margin[k] <= p[k] <= hi[k] + margin[k] for k in range(3))]
        if len(pts) < 12:
            # Nothing of the shell there (a scripted part it doesn't have): no invisible wall.
            log(f'{obj.name}: hull at {tuple(round(v, 2) for v in mid)} dropped (only {len(pts)} shell vertices round it)')
            bpy.data.objects.remove(hull)
            continue
        _set_hull(hull, kdop(pts))


def posts(shell, region, band=(1.3, 2.2), step=0.02, widest=0.45):
    """Thin upright things (porch posts) in region ((x0, y0, z0), (x1, y1, z1)), found by rays across it along +X at
    both of band's heights, every step in y: each pair of hits (into a post, out of it) no wider than widest is a slice
    of a post; slices at both heights, lined up in x, make one. (Vertices can't find them: a decimated post is a
    straight column with no vertices halfway up.) Returns [(lo, hi)] boxes, each spanning region's heights."""
    b0, b1 = Vector(region[0]), Vector(region[1])
    bvh = tree(shell)
    slices = {z: [] for z in band}
    y = b0.y
    while y <= b1.y:
        for z in band:
            x, inside = b0.x, None
            while x < b1.x:
                hit = bvh.ray_cast(Vector((x, y, z)), Vector((1, 0, 0)), b1.x - x)
                if hit[0] is None:
                    break
                face_out = hit[1].x > 0.0   # leaving a solid through a face that points +X
                if not face_out:
                    inside = hit[0].x
                elif inside is not None:
                    if hit[0].x - inside <= widest:
                        slices[z].append((inside, hit[0].x, y))
                    inside = None
                x = hit[0].x + 0.002
        y += step
    groups = []
    for x0, x1, yy in slices[band[0]]:
        mid = (x0 + x1) * 0.5
        if not any(abs((a0 + a1) * 0.5 - mid) < 0.12 and abs(ay - yy) < step * 1.5 for a0, a1, ay in slices[band[1]]):
            continue
        for g in groups:
            if abs(g['mid'] - mid) < 0.15 and yy - g['y1'] <= step * 2.5:
                g['x0'], g['x1'], g['y1'] = min(g['x0'], x0), max(g['x1'], x1), yy
                break
        else:
            groups.append(dict(mid=mid, x0=x0, x1=x1, y0=yy, y1=yy))
    out = []
    for g in groups:
        if g['y1'] - g['y0'] >= 0.04 and g['y1'] - g['y0'] <= widest:
            out.append((Vector((g['x0'], g['y0'] - step * 0.5, b0.z)), Vector((g['x1'], g['y1'] + step * 0.5, b1.z))))
    return out


def add_box_hull(obj, lo, hi):
    corners = [Vector((x, y, z)) for x in (lo.x, hi.x) for y in (lo.y, hi.y) for z in (lo.z, hi.z)]
    return _new_hull(obj, corners)


# --- The look: Tripo's paint graded onto the town's ---

def _opponent(px):
    """(L, rg, yb) of sRGB pixels (N, 3): L the mean, rg = r - g, yb = (r + g) / 2 - b."""
    return px.mean(1), px[:, 0] - px[:, 1], (px[:, 0] + px[:, 1]) * 0.5 - px[:, 2]


def _stats(path, stride=5):
    import numpy as np
    im = bpy.data.images.load(path, check_existing=False)
    buf = np.empty(im.size[0] * im.size[1] * 4, np.float32)
    im.pixels.foreach_get(buf)
    bpy.data.images.remove(im)
    px = buf.reshape(-1, 4)[::stride, :3]
    px = px[px.mean(1) > 0.03]   # Tripo's atlas padding is black
    return [(c.mean(), c.std()) for c in _opponent(px)]


def grade(set_name, ref_set='HouseTrim', light=0.92, pull=0.5):
    """Grades Tripo's colour map (Art/Textures/<set>/T_<set>_BC.png) onto the town's look: its two colour-difference
    channels (red-green, yellow-blue) take the mean and spread of the house trim sheet's (the paint every scripted
    building wears); its brightness moves `pull` of the way to the trim sheet's mean times light (Tripo's maps carry
    some baked shade, the trim sheet none; all the way washed the farmhouse's pale roof out), its spread kept. Tripo's
    warm, saturated wood (a repaint in our in-game light came out 3-4 times as saturated as the trim) lands in the same
    palette, its own variation (red frames, teal doors) kept relative. swap() rewrites the map from the .glb before
    grading, so the grade is applied once to Tripo's own colours on every export."""
    import numpy as np
    path = lt.texture_path(set_name, 'BC')
    ref = _stats(lt.texture_path(ref_set, 'BC'))
    im = bpy.data.images.load(path, check_existing=False)
    w, h = im.size
    buf = np.empty(w * h * 4, np.float32)
    im.pixels.foreach_get(buf)
    bpy.data.images.remove(im)
    px = buf.reshape(-1, 4)[:, :3]
    used = px.mean(1) > 0.03
    L, rg, yb = _opponent(px)
    out = []
    for k, (chan, (t_mean, t_std)) in enumerate(zip((L, rg, yb), ref)):
        m, s = chan[used].mean(), chan[used].std()
        if k == 0:
            out.append(chan + (t_mean * light - m) * pull)
        else:
            out.append(t_mean + (chan - m) * (t_std / max(s, 1e-4)))
    L2, rg2, yb2 = out
    b = L2 - 2.0 * yb2 / 3.0
    rpg = 2.0 * (yb2 + b)
    rgb = np.stack([(rpg + rg2) * 0.5, (rpg - rg2) * 0.5, b], axis=1)
    rgb[~used] = px[~used]
    rgb = np.clip(rgb, 0.0, 1.0).reshape(h, w, 3)[::-1]   # write_png takes rows top first
    lt.write_png(path, lt.to8(rgb))
    log(f'{set_name}: graded onto {ref_set}: L {L[used].mean():.3f}->{L2[used].mean():.3f}, '
        f'rg {rg[used].mean():.3f}+-{rg[used].std():.3f} -> {rg2[used].mean():.3f}, '
        f'yb {yb[used].mean():.3f}+-{yb[used].std():.3f} -> {yb2[used].mean():.3f}')


# --- The swap ---

FITS_FILE = os.path.join(REPO, 'Art', 'Models', 'Buildings', 'Tripo', 'parts_buildings.py')


def apply(obj, name, **parts):
    """The hook a building script calls on its finished model: runs the fit for Tripo model `name` in
    Art/Models/Buildings/Tripo/parts_buildings.py (FITS[name](obj, **parts): parts are the model's moving pieces, a
    windmill's fan), when the swap is on and the model exists."""
    if not enabled(name) or not os.path.exists(glb_path(name)):
        return swap(obj, name)   # logs why and leaves obj
    import importlib.util
    spec = importlib.util.spec_from_file_location('parts_buildings', FITS_FILE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.FITS[name](obj, **parts)

def swap(obj, name, front='+X', cut=(), keep=(), keep_whole=(), anchors=None, fallback=None, extra=None,
         set_name=None, faces_limit=None, grade_light=0.92, hulls_keep=(), hulls_drop=(), post_region=None,
         post_band=(1.3, 2.2), refit=True, fit=None, variant='', more_pieces=None, retexture=None):
    """Replaces the scripted model obj's mesh with the Tripo model `name`, fitted to obj's bounds.

    front      the axis the .glb's front faces (Tripo: +X).
    variant    which paint: '' the model's .glb, '_grey' Tripo's first paint when it was repainted.
    more_pieces a function (shell, scripted) returning more objects to join (scripted parts copied elsewhere: posts).
    retexture  a function (shell, scripted) called once the shell wears its Tripo material (slot 0): faces it
               moves onto other slots and maps (a roof onto the house trim sheet).
    fit        a function (shell, scripted) placing the shell itself, instead of the stretch onto the scripted bounds.
    anchors    {'z': [(tripo_value, target), ...], ...} or a function (shell, scripted) -> that dict, called after the
               box fit: features of the shell moved onto the scripted ones.
    cut        boxes of the shell to remove (where scripted parts go in).
    keep       boxes of the scripted mesh to keep (faces by centre); keep_whole: by every corner.
    extra      a function (shell, scripted) called last on the fitted shell (any fix of its own).
    fallback   the Nanite fallback's share of triangles (percent; what Medium draws).
    grade_light the brightness grade() gives the colour map (None: Tripo's colours as they came).
    refit      the scripted UCX hulls refitted to the shell (refit_hulls), except those centred in hulls_keep (kept
               scripted parts); those centred in hulls_drop go; post_region adds a box hull round each post found there.
    Returns obj (its mesh replaced), or obj unchanged when the swap is off or the .glb is missing."""
    path = glb_path(name, variant)
    if not enabled(name):
        log(f'{name}: off (LOOTER_BUILDINGS_TRIPO), scripted {obj.name} kept')
        return obj
    if not os.path.exists(path):
        log(f'{name}: no {os.path.relpath(path, REPO)}, scripted {obj.name} kept')
        return obj
    lo, hi = bounds(obj)
    shell, maps = tripo.load(path)
    orient(shell, front)
    if fit:
        fit(shell, obj)
    else:
        scale = fit_box(shell, lo, hi)
        log(f'{name}: fitted to {obj.name} {tuple(round(hi[k] - lo[k], 2) for k in range(3))} m, '
            f'stretch {tuple(round(s, 3) for s in scale)} (after auto_size)')
    if anchors:
        moves = anchors(shell, obj) if callable(anchors) else anchors
        for axis, pairs in moves.items():
            pairs = [p for p in pairs if p[0] is not None]
            if pairs:
                remap(shell, axis, pairs)
                log(f'{name}: {axis} anchors {[(round(a, 3), round(b, 3)) for a, b in pairs]}')
    if extra:
        extra(shell, obj)
    if faces_limit and triangles(shell) > faces_limit:
        # Collapse only (a planar dissolve smears a Tripo atlas into spikes: Docs/Pipeline/Assets.md, chests), before the
        # cuts so their edges stay clean. The model then keeps every triangle in its Nanite fallback, like every
        # building: Medium draws exactly what was checked here.
        before = triangles(shell)
        if shell.name not in bpy.context.scene.collection.objects:
            bpy.context.scene.collection.objects.link(shell)
        mod = shell.modifiers.new('Decimate', 'DECIMATE')
        mod.decimate_type = 'COLLAPSE'
        mod.ratio = faces_limit / before
        with bpy.context.temp_override(object=shell, active_object=shell):
            bpy.ops.object.modifier_apply(modifier=mod.name)
        log(f'{name}: decimated {before} -> {triangles(shell)} triangles')
    for box in cut:
        log(f'{name}: cut {cut_box(shell, box)} shell faces in {box}')
    if refit:
        before = len([c for c in obj.children if c.name.startswith('UCX_')])
        refit_hulls(obj, shell, keep=hulls_keep, drop=hulls_drop)
        found = posts(shell, post_region, band=post_band) if post_region else []
        for lo_, hi_ in found:
            add_box_hull(obj, lo_, hi_)
        log(f'{name}: hulls refitted to the shell ({before} scripted, {len(found)} posts found: '
            f'{[tuple(round(v, 2) for v in (a + b) * 0.5)[:2] for a, b in found]})')

    set_name = set_name or f'Tripo{name}'
    if grade_light is not None:
        # The grade works on Tripo's own colours: the map is written afresh from the .glb first.
        bc = lt.texture_path(set_name, 'BC')
        if os.path.exists(bc):
            os.remove(bc)
    mat = tripo.write_set(maps, set_name, set_name, path, master='World', orm_size=2048, max_size=2048)
    if grade_light is not None:
        grade(set_name, light=grade_light)
    _clean_attributes(shell)
    shell.data.materials.clear()
    shell.data.materials.append(mat)
    for p in shell.data.polygons:
        p.material_index = 0
    if retexture:
        retexture(shell, obj)
    sharp_by_angle(shell)
    shell.name = '_TripoShell'
    if shell.name not in bpy.context.scene.collection.objects:
        bpy.context.scene.collection.objects.link(shell)
    shell.matrix_world = obj.matrix_world.copy()

    # The scripted parts kept: a copy of the scripted mesh trimmed to the boxes.
    pieces = [shell]
    kept = sorted(set(faces_in(obj, keep)) | set(faces_in(obj, keep_whole, whole=True)))
    if kept:
        part = bpy.data.objects.new('_TripoKept', obj.data.copy())
        bpy.context.scene.collection.objects.link(part)
        part.matrix_world = obj.matrix_world.copy()
        keep_set = set(kept)
        delete_faces(part, [i for i in range(len(part.data.polygons)) if i not in keep_set])
        pieces.append(part)
        log(f'{name}: kept {len(kept)} scripted faces')
    if more_pieces:
        added = more_pieces(shell, obj)
        pieces += added
        log(f'{name}: {len(added)} more scripted pieces placed')

    # Occlusion on the shell (the kept parts have theirs): the shell and kept parts shade each other and the ground.
    lt.bake_vertex_ao(shell, ground=True, children=False)

    with bpy.context.temp_override(active_object=shell, object=shell, selected_objects=pieces,
                                   selected_editable_objects=pieces):
        bpy.ops.object.join()
    old = obj.data
    shell.data.name = obj.name
    obj.data = shell.data
    bpy.data.objects.remove(shell)
    if not old.users:
        bpy.data.meshes.remove(old)
    if fallback is not None:
        obj['Fallback'] = float(fallback)
    obj['TripoSource'] = os.path.relpath(path, REPO).replace('\\', '/')
    log(f'{name}: {obj.name} now {triangles(obj)} triangles, materials '
        f'{", ".join(m.name for m in obj.data.materials if m)}, fallback {obj.get("Fallback")}')
    return obj
