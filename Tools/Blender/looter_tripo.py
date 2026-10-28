"""Fits an AI-made mesh (Tripo image-to-3D, a .glb) into a scripted gun part, in looter_guns' gun space (cm: u along the
gun toward the muzzle, v up from the bore, w to the gun's left), keeping Tripo's baked maps.

Tripo exports one mesh in its own box (its pieces separate shells, any units) on one UV atlas, with a base-colour map
and, on some exports, a glTF metal-roughness map and an OpenGL normal map.
- load() brings it in clean (no root empty, no custom normals, split vertices merged, slivers dropped) and hands back
  its maps.
- write_set() writes them as a texture set, Art/Textures/<Set>/T_<Set>_BC.png (its own size), _ORM (occlusion 1,
  Tripo's roughness and metal, or guessed from the colours when it has none) and _N (green flipped to DirectX), so the
  model importer makes MI_<material> on the Gun master (wear, notches, maker's marks) like any textured part.
- fit() stretches the mesh onto the part's box in gun space, each axis on its own (Tripo's sizes drift; the concept's
  size table is the truth).
- islands() lists its pieces with their bounds; faces_where() picks faces by the colour under them (painted ember slots,
  a sight's cyan dot, which become glow slots); centroid() and hole_centre() measure them (a sight's line).
- split() cuts the mesh into one object per material for Gun.add(), keeping Tripo's UVs on the textured one.
The part keeps the scripted model's name, key and sockets, so the game can't tell the two apart.

    import looter_tripo as tripo
    obj, maps = tripo.load('Art/Models/Weapons/Tripo/BullpupBarrel_Carbine.glb')
    mat = tripo.write_set(maps, 'TripoBullpupBarrel', 'GunTripoBullpupBarrel', obj['TripoFile'])
    tripo.fit(obj, u=(51.9, 63.9), v=(-1.65, 1.65), w=3.3, forward='-Y', left='+X')
    glow = tripo.faces_where(obj, maps['BC'], tripo.is_ember)
    rest = [i for i in range(len(obj.data.polygons)) if i not in set(glow)]
    gun.add('Barrel_Carbine', *tripo.split(obj, {lg.EMBER: glow, mat: rest}, keep_uv=(mat,)))
"""
import os
import shutil

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import looter_guns as lg
import looter_textures as lt

MAX_SIZE = 2048   # px: guns lie in piles on the ground; no part map is bigger (the 8K Tripo body map is shrunk)
REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
_AXES = {'X': 0, 'Y': 1, 'Z': 2}


def _abs(path):
    return path if os.path.isabs(path) else os.path.join(REPO, path)


def _upstream_image(socket, depth=0):
    """The first image texture feeding socket, through any nodes between (the importer's mixes, separates, normal map)."""
    for link in socket.links:
        node = link.from_node
        if node.type == 'TEX_IMAGE' and node.image:
            return node.image
        if depth < 6:
            for inp in node.inputs:
                if inp.is_linked:
                    found = _upstream_image(inp, depth + 1)
                    if found:
                        return found
    return None


def load(path, merge=1e-5, sliver=0.02):
    """Imports the .glb at path (relative to the repo or absolute) as one mesh object at the origin: transforms baked,
    the importer's empties gone, custom normals cleared (Gun.build shades by angle), vertices split along UV seams merged
    (merge: a share of the mesh's size; the UVs stay per corner), pieces under sliver percent of the largest piece's area
    dropped (zero-area junk). Returns (object, {'BC': image, 'RM': image or None, 'N': image or None})."""
    path = _abs(path)
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == 'MESH']
    if not meshes:
        raise ValueError(f'{path}: no mesh')
    for o in meshes:
        world = o.matrix_world.copy()
        o.parent = None
        o.data.transform(world)
        o.matrix_world = Matrix()
    for o in new:
        if o.type != 'MESH':
            bpy.data.objects.remove(o)
    obj = meshes[0]
    if len(meshes) > 1:
        with bpy.context.temp_override(active_object=obj, object=obj, selected_objects=meshes,
                                       selected_editable_objects=meshes):
            bpy.ops.object.join()
    with bpy.context.temp_override(active_object=obj, object=obj, selected_objects=[obj],
                                   selected_editable_objects=[obj]):
        bpy.ops.mesh.customdata_custom_splitnormals_clear()
    maps = {'BC': None, 'RM': None, 'N': None}
    old = [m for m in obj.data.materials if m]
    for m in old:
        bsdf = next((n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None) if m.use_nodes else None
        if bsdf:
            maps['BC'] = maps['BC'] or _upstream_image(bsdf.inputs['Base Color'])
            maps['RM'] = maps['RM'] or _upstream_image(bsdf.inputs['Roughness']) or _upstream_image(bsdf.inputs['Metallic'])
            maps['N'] = maps['N'] or _upstream_image(bsdf.inputs['Normal'])
    if maps['BC'] is None:
        raise ValueError(f'{path}: no base-colour map')
    obj.data.materials.clear()
    for m in old:
        if not m.users:
            bpy.data.materials.remove(m)
    span = max(max(v.co[i] for v in obj.data.vertices) - min(v.co[i] for v in obj.data.vertices) for i in range(3))
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=merge * span)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=merge * span * 0.1)
    bm.to_mesh(obj.data)
    bm.free()
    found = islands(obj, gun_space=False)
    biggest = max(i.area for i in found)
    junk = [f for i in found if i.area < biggest * sliver * 0.01 for f in i.faces]
    if junk:
        _delete_faces(obj, junk)
    obj.name = '_TripoSource'
    obj['TripoFile'] = path   # kept only on the source; split() doesn't copy it onto the pieces
    return obj, maps


# --- The texture set ---

def _shrunk(image, size):
    """A copy of image at most size px wide, shrunk by halves (each a clean 2 x 2 average, so an 8K map's fine detail
    filters down instead of aliasing), the last step straight to size."""
    out = image.copy()
    while size and out.size[0] > size:
        w, h = out.size
        out.scale(max(size, w // 2), max(1, round(h * max(size, w // 2) / w)))
    return out


def png_width(path):
    """A PNG's width from its header (0 when it isn't a PNG)."""
    with open(path, 'rb') as f:
        head = f.read(24)
    return int.from_bytes(head[16:20], 'big') if head[1:4] == b'PNG' else 0


def pixels(image, size=None):
    """The image's pixels as a float array (rows top first, RGBA, the stored values: sRGB for a colour map, raw data for
    the others), at most size px wide when given (shrunk by halves) or scaled up to it."""
    src = image
    if size and image.size[0] > size:
        src = _shrunk(image, size)
    elif size and tuple(image.size) != (size, size):
        src = image.copy()
        src.scale(size, size)
    w, h = src.size
    buf = np.empty(w * h * 4, dtype=np.float32)
    src.pixels.foreach_get(buf)
    if src is not image:
        bpy.data.images.remove(src)
    return buf.reshape(h, w, 4)[::-1]


def guess_orm(rgb):
    """Occlusion, roughness, metallic from a base colour (sRGB 0..1, (H, W, 3)), for Tripo exports with colour only.
    Dark, grey texels read as gunmetal, saturated yellow ones as brass (metal, glossier); the rest (sand shells, the
    painted ember) as matte polymer. Occlusion stays 1: Gun.build bakes it into the vertices."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    hi, lo = rgb.max(axis=-1), rgb.min(axis=-1)
    sat = (hi - lo) / np.maximum(hi, 1e-4)
    ramp = lambda x, a, b_: np.clip((x - a) / (b_ - a), 0.0, 1.0)
    dark = ramp(-hi, -0.42, -0.26) * ramp(-sat, -0.38, -0.22)
    brass = (ramp(sat, 0.35, 0.5) * ramp(hi, 0.4, 0.55) * ramp(g / np.maximum(r, 1e-4), 0.6, 0.7)
             * ramp(-b / np.maximum(r, 1e-4), -0.55, -0.4))
    metal = np.maximum(dark * 0.85, brass)
    rough = 0.74 - 0.32 * metal
    return np.stack([np.ones_like(r), rough, metal], axis=-1)


def _save_colour(image, path, max_size=None):
    """Writes a colour map as an 8-bit RGB PNG with Blender's own writer (an 8K map through numpy would take gigabytes):
    save_render with the Standard view and no look hands the stored sRGB bytes through (Image.save on a packed image
    writes the packed JPEG bytes whatever the name)."""
    out = _shrunk(image, max_size)
    scene = bpy.context.scene
    fmt, view = scene.render.image_settings, scene.view_settings
    kept = (fmt.file_format, fmt.color_mode, fmt.color_depth, fmt.compression, view.view_transform, view.look,
            view.exposure, view.gamma)
    fmt.file_format, fmt.color_mode, fmt.color_depth, fmt.compression = 'PNG', 'RGB', '8', 90
    view.view_transform, view.look, view.exposure, view.gamma = 'Standard', 'None', 0.0, 1.0
    try:
        out.save_render(filepath=path, scene=scene)
    finally:
        (fmt.file_format, fmt.color_mode, fmt.color_depth, fmt.compression, view.view_transform, view.look,
         view.exposure, view.gamma) = kept
    size = out.size[0]
    bpy.data.images.remove(out)
    return size


def write_set(maps, set_name, material_name, source, master='Gun', orm_size=2048, max_size=MAX_SIZE):
    """Writes maps (load()'s) as the texture set set_name: T_<set>_BC.png (its own size, at most max_size), _ORM and, when
    Tripo made one, _N, unless they are all newer than source (the .glb they came from), so an export doesn't rewrite
    them. Returns the material material_name on master for that set (the importer brings the PNGs to
    /Game/Art/Textures/<set_name>)."""
    bc, orm, nrm = (lt.texture_path(set_name, k) for k in ('BC', 'ORM', 'N'))
    wanted = [bc, orm] + ([nrm] if maps.get('N') else [])
    stamp = os.path.getmtime(_abs(source))
    if not (all(os.path.exists(p) and os.path.getmtime(p) >= stamp for p in wanted)
            and png_width(bc) <= max_size):
        os.makedirs(os.path.dirname(bc), exist_ok=True)
        size = _save_colour(maps['BC'], bc, max_size)
        if maps.get('RM'):
            # glTF metal-roughness: green roughness, blue metal.
            rm = pixels(maps['RM'], min(orm_size, maps['RM'].size[0]))
            data = np.stack([np.ones_like(rm[..., 0]), rm[..., 1], rm[..., 2]], axis=-1)
        else:
            data = guess_orm(pixels(maps['BC'], min(orm_size, maps['BC'].size[0]))[..., :3])
        lt.write_png(orm, lt.to8(data))
        note = f'_ORM ({data.shape[0]} px, {"Tripo" if maps.get("RM") else "guessed"})'
        if maps.get('N'):
            n = pixels(maps['N'], min(max_size, maps['N'].size[0]))[..., :3]
            n[..., 1] = 1.0 - n[..., 1]   # glTF normal maps are OpenGL (green up); ours are DirectX
            lt.write_png(nrm, lt.to8(n))
            note += f', _N ({n.shape[0]} px)'
        lt._log(f'{set_name}: wrote T_{set_name}_BC ({size} px), {note}')
    elif os.path.exists(nrm) and not maps.get('N'):
        os.remove(nrm)
    return lt.material(set_name, name=material_name, master=master)


# --- Fitting ---

def fit(obj, u, v, w, forward='-X', left='-Y', up='+Z'):
    """Stretches obj (as load() left it; forward, left and up name the file's axes the gun's muzzle, its left side and
    up face) so its bounds run u[0]..u[1] along the gun and v[0]..v[1] up (cm), w cm across, centred on the bore's plane;
    then it lies in gun space (Blender: +X left, -Y forward, +Z up, meters)."""
    me = obj.data
    pick = lambda name: (_AXES[name[-1]], -1.0 if name[0] == '-' else 1.0)
    (fa, fs), (la, ls), (ua, us) = pick(forward), pick(left), pick(up)
    along = [x.co[fa] * fs for x in me.vertices]
    across = [x.co[la] * ls for x in me.vertices]
    high = [x.co[ua] * us for x in me.vertices]
    a0, a1, c0, c1, z0, z1 = min(along), max(along), min(across), max(across), min(high), max(high)
    su, sv, sw = (u[1] - u[0]) / (a1 - a0), (v[1] - v[0]) / (z1 - z0), w / (c1 - c0)
    cm = (c0 + c1) * 0.5
    for vert, a, c, z in zip(me.vertices, along, across, high):
        vert.co = lg.at(u[0] + (a - a0) * su, v[0] + (z - z0) * sv, (c - cm) * sw)
    # The map's determinant (Blender x = left, y = -forward, z = up): negative when the axes as named mirror the mesh,
    # which would turn its faces inside out.
    perm = (la, fa, ua)
    sign = -1.0 if sum(1 for i in range(3) for j in range(i + 1, 3) if perm[i] > perm[j]) % 2 else 1.0
    if -fs * ls * us * sign < 0.0:
        me.flip_normals()
    me.update()
    return su, sv, sw


class Island:
    """A connected piece: its face indices, bounds lo/hi ((u, v, w) cm in gun space, or the file's x, y, z), area (cm^2
    in gun space)."""
    __slots__ = ('index', 'faces', 'lo', 'hi', 'area')

    def __init__(self, index, faces, lo, hi, area):
        self.index, self.faces, self.lo, self.hi, self.area = index, faces, lo, hi, area

    @property
    def size(self):
        return tuple(b - a for a, b in zip(self.lo, self.hi))

    def __repr__(self):
        f = lambda t: '(' + ', '.join(f'{x:.2f}' for x in t) + ')'
        return f'Island {self.index}: {len(self.faces)} faces, area {self.area:.1f}, lo {f(self.lo)} hi {f(self.hi)}'


def gun_uvw(co):
    return (-co.y / lg.CM, co.z / lg.CM, co.x / lg.CM)


def islands(obj, gun_space=True):
    """The mesh's connected pieces, largest first."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    conv = gun_uvw if gun_space else (lambda co: tuple(co))
    scale = 1.0 / (lg.CM * lg.CM) if gun_space else 1.0
    seen, found = set(), []
    for face in bm.faces:
        if face.index in seen:
            continue
        stack, faces = [face], []
        seen.add(face.index)
        while stack:
            f = stack.pop()
            faces.append(f)
            for e in f.edges:
                for g in e.link_faces:
                    if g.index not in seen:
                        seen.add(g.index)
                        stack.append(g)
        pts = [conv(v.co) for f in faces for v in f.verts]
        lo = tuple(min(p[i] for p in pts) for i in range(3))
        hi = tuple(max(p[i] for p in pts) for i in range(3))
        found.append((sum(f.calc_area() for f in faces) * scale, [f.index for f in faces], lo, hi))
    bm.free()
    found.sort(key=lambda t: -t[0])
    return [Island(k, faces, lo, hi, area) for k, (area, faces, lo, hi) in enumerate(found)]


# --- Painted glows and measurements ---

def is_ember(rgb):
    """The painted ember glow (Tripo's orange): bright, saturated, red well over green (brass is yellower)."""
    r, g, b = rgb
    return r > 0.75 and g < 0.62 * r and b < 0.4 * r


def is_cyan(rgb):
    """A painted cyan dot or LED: blue and green high, red low."""
    r, g, b = rgb
    return b > 0.55 and g > 0.5 and r < 0.6 * b


def faces_where(obj, image, test, size=1024, corners=True, share=1.0):
    """Faces whose base colour passes test((r, g, b) sRGB 0..1): sampled at the face's UV centre and (corners) at each
    corner pulled a third of the way in; at least share of the samples must pass (1: all, so a face only half in a
    painted shape stays out)."""
    px = pixels(image, min(size, image.size[0]))[..., :3]
    h, w = px.shape[:2]
    me = obj.data
    uv = me.uv_layers.active.data
    hits = []
    for poly in me.polygons:
        pts = [uv[li].uv for li in poly.loop_indices]
        cu = sum(p[0] for p in pts) / len(pts)
        cv = sum(p[1] for p in pts) / len(pts)
        samples = [(cu, cv)] + ([(cu + (p[0] - cu) * 0.67, cv + (p[1] - cv) * 0.67) for p in pts] if corners else [])
        passed = sum(1 for s in samples
                     if test(px[min(h - 1, max(0, int((1.0 - (s[1] % 1.0)) * h))), min(w - 1, max(0, int((s[0] % 1.0) * w)))]))
        if passed >= share * len(samples) - 1e-6:
            hits.append(poly.index)
    return hits


def is_sand(rgb):
    """The sand polymer shell (what the shell paint recolours): light, warm, a little saturated, not the ember orange or
    the brass."""
    r, g, b = rgb
    hi, lo = max(r, g, b), min(r, g, b)
    sat = (hi - lo) / max(hi, 1e-4)
    return hi > 0.5 and sat < 0.42 and r >= b and g > 0.72 * r


def _linear(c):
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def _srgb(c):
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(np.maximum(c, 0.0), 1.0 / 2.4) - 0.055)


def paint_set(maps, set_name, paint_slot, tint, source, master='Gun', size=MAX_SIZE):
    """A paintable copy of the texture set set_name (write_set's, written first): <set_name>Paint, its colour map in linear
    light divided by the sand shell's own brightest colour (the 98th percentile of its texels, per channel), so it is
    near-white where Tripo painted sand and keeps Tripo's light and dark; tinted tint (the paint's default colour,
    0xRRGGBB) it reads as our sand, and any other paint recolours it as the scripted shells' GunPolymerSand. Its ORM and
    normal are the set's. Returns the material <paint_slot>_<set_name>: the game paints a slot named
    <paint>_<anything> with that paint (UWeaponModelComponent; the exporter folds "__" to "_")."""
    paint = set_name + 'Paint'
    bc = lt.texture_path(paint, 'BC')
    stamp = os.path.getmtime(_abs(source))
    if not (os.path.exists(bc) and os.path.getmtime(bc) >= stamp and os.path.getmtime(bc) >= os.path.getmtime(__file__)):
        os.makedirs(os.path.dirname(bc), exist_ok=True)
        rgb = pixels(maps['BC'], min(size, maps['BC'].size[0]))[..., :3]
        lin = _linear(rgb)
        r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
        hi, lo = rgb.max(axis=-1), rgb.min(axis=-1)
        sand = (hi > 0.5) & ((hi - lo) / np.maximum(hi, 1e-4) < 0.42) & (r >= b) & (g > 0.72 * r)   # as is_sand
        top = np.percentile(lin[sand], 98.0, axis=0) if sand.any() else np.ones(3)
        out = _srgb(np.clip(lin / np.maximum(top, 1e-3), 0.0, 1.0))
        lt.write_png(bc, lt.to8(out))
        lt._log(f'{paint}: wrote T_{paint}_BC ({rgb.shape[0]} px; the sand divided by linear {np.round(top, 3).tolist()})')
    for kind in ('ORM', 'N'):
        src, dst = lt.texture_path(set_name, kind), lt.texture_path(paint, kind)
        if os.path.exists(src) and not (os.path.exists(dst) and os.path.getmtime(dst) >= os.path.getmtime(src)):
            shutil.copyfile(src, dst)
        elif not os.path.exists(src) and os.path.exists(dst):
            os.remove(dst)
    return lt.material(paint, name=f'{paint_slot}_{set_name}', master=master, tint=tint)


def centroid(obj, faces):
    """The area-weighted centre of faces, (u, v, w) cm in gun space."""
    me = obj.data
    total, acc = 0.0, Vector()
    for i in faces:
        p = me.polygons[i]
        total += p.area
        acc += p.center * p.area
    return gun_uvw(acc / total) if total else None


def probe_u(obj, points, reach=30.0):
    """Where the mesh's front surface is (u, cm), seen along -u from reach cm ahead of the gun origin at each (v, w)
    point; the median of the hits (None if none hit)."""
    me = obj.data
    tree = BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])
    hits = []
    for v, w in points:
        hit = tree.ray_cast(lg.at(reach, v, w), Vector((0.0, 1.0, 0.0)), 2.0 * reach * lg.CM)[0]
        if hit is not None:
            hits.append(gun_uvw(hit)[0])
    return sorted(hits)[len(hits) // 2] if hits else None


def hole_centre(obj, u, v_range, w_range, depth=0.8, step=0.03):
    """The centre (v, w, cm) of the opening in a plate standing across the gun at u (a rear sight's aperture): rays along
    the gun through a grid over v_range x w_range, looking only depth cm either side of u; the misses no path of misses
    joins to the grid's edge are the hole. Returns (v, w, width cm) or None."""
    me = obj.data
    tree = BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [list(p.vertices) for p in me.polygons])
    vs = np.arange(v_range[0], v_range[1], step)
    ws = np.arange(w_range[0], w_range[1], step)
    hit = np.zeros((len(vs), len(ws)), bool)
    for i, v in enumerate(vs):
        for j, w in enumerate(ws):
            origin = lg.at(u - depth, v, w)
            hit[i, j] = tree.ray_cast(origin, Vector((0.0, -1.0, 0.0)), 2.0 * depth * lg.CM)[0] is not None
    outside = np.zeros_like(hit)
    stack = [(i, j) for i in range(len(vs)) for j in (0, len(ws) - 1)] + [(i, j) for j in range(len(ws)) for i in (0, len(vs) - 1)]
    while stack:
        i, j = stack.pop()
        if outside[i, j] or hit[i, j]:
            continue
        outside[i, j] = True
        for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            a, b = i + di, j + dj
            if 0 <= a < len(vs) and 0 <= b < len(ws):
                stack.append((a, b))
    hole = ~hit & ~outside
    if not hole.any():
        return None
    ii, jj = np.nonzero(hole)
    return float(vs[ii].mean()), float(ws[jj].mean()), float((ws[jj].max() - ws[jj].min()) + step)


def _delete_faces(obj, faces):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    doomed = [bm.faces[i] for i in set(faces)]
    bmesh.ops.delete(bm, geom=doomed, context='FACES')
    bm.to_mesh(obj.data)
    bm.free()


def split(obj, groups, keep_uv=()):
    """groups: {material: [face index, ...]}. Returns one object per material holding those faces, mapped with
    lg.mapped (a tiled set's material at the gun's texel density); the materials in keep_uv keep Tripo's UVs (lg.fill_uvs
    leaves them be). Faces in no group are dropped. obj is removed."""
    pieces = []
    for mat, faces in groups.items():
        keep = set(faces)
        if not keep:
            continue
        piece = obj.copy()
        piece.data = obj.data.copy()
        if 'TripoFile' in piece:
            del piece['TripoFile']
        piece.name = piece.data.name = f'_Tripo_{mat.name}'
        bpy.context.scene.collection.objects.link(piece)
        _delete_faces(piece, [i for i in range(len(obj.data.polygons)) if i not in keep])
        pieces.append(lg.mapped(piece, mat))
        if mat in keep_uv:
            piece['KeepUV'] = 1
    mesh = obj.data
    bpy.data.objects.remove(obj)
    if not mesh.users:
        bpy.data.meshes.remove(mesh)
    return pieces
